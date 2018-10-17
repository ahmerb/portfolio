import sys, subprocess
from timeit import default_timer as timer
import random
from Crypto.Util import number # pip install pycrypto

target_D = None
target_R = None

ATTACK_TARGET = ""
CONF_FILE     = ""


###################################################
## Attack Device Simulation
###################################################

# the Montgomery parameters (uninitialised here)
omega  = 0
rho    = 0
rho_sq = 0

# word size and base
w = 64
b = 1 << w

# bitsize of N (uninitialised here)
l_N = 0

# XXX TODO : MontMul should take a param N too (it works bc its globally assigned in Main)
def MontMul(x, y):
    t = x * y
    u = (t + (t * omega % rho) * N) / rho
    red = False
    if u >= N:
        u = u - N
        red = True
    return (u, red)

def convert_to_mform(ctext):
    ctext_mform, _ = MontMul(ctext, rho_sq) # convert ctext to mont form
    return ctext_mform

def convert_to_mforms(ctexts):
    return map(convert_to_mform, ctexts)

def sq_and_mul_init(ctexts):
    # do the first 1.5 iterations
    def sq_and_mul_ctext(m_temp, ctext_mform):
        # sq for current bit iter
        m_temp, _ = MontMul(m_temp, m_temp)

        # first key bit is 1 so we do a multiply
        m_temp, _ = MontMul(m_temp, ctext_mform)

        # sq operation in next step
        m_temp, _ = MontMul(m_temp, m_temp)

        return m_temp

    # convert all the ciphertexts to montgomery form
    ctexts_mform = map(convert_to_mform, ctexts)

    # in sq and multiply, we start with a 1 (the identity elem of the group Z_N)
    # convert this 1 into Mont form
    m_temp, _ = MontMul(1, rho_sq)

    # sq and multiply each cipher text for the current bit
    m_temps = map(lambda c: sq_and_mul_ctext(m_temp, c), ctexts_mform)

    # for each ctext, return temporary value of result, and value
    # of each ctext in Mont form
    return m_temps, ctexts_mform

def sq_and_mul(ctext, m_temp):
    # 1. do the case when the key bit is zero
    #  -> there is only a square (in i+1th step)
    mtemp_caseBitNotSet, red_caseBitNotSet = MontMul(m_temp, m_temp)

    # 2. do the case when the key bit is one
    #  -> there is a multiply in ith step then a sq in i+1th step
    tmp, _ = MontMul(m_temp, ctext)
    mtemp_caseBitSet, red_caseBitSet = MontMul(tmp, tmp)

    return mtemp_caseBitSet, red_caseBitSet, mtemp_caseBitNotSet, red_caseBitNotSet

def init_mont_param_rho(N, e):
    global rho, rho_sq
    t = 1
    while t <= N:
        t *= b
    rho = t
    rho_sq = pow(t, 2, N)

def init_mont_param_omega(N, e):
    global omega
    # we only use omega mod b=2, so dont need it mod rho
    # omega = N % 2
    # omega = 0 - omega + N
    omega = (-number.inverse(N, rho)) % rho

def init_mont_params(N, e):
    init_mont_param_rho(N, e)
    init_mont_param_omega(N, e)


###################################################
## ATTACK
###################################################

# Attack Parameters
CTEXT_BITLEN = 1024 # NOTE attack work for all moduli size -> python 2.6 doesn't have N.bit_length(): use len(bits(N)[2:])
CTEXTS_N = 7000
diff_threshold = 5.0

# Error Detection
errors_n = 0 # the bits in a row we've encountered where we've had a low confidence level
error_already_handled = False # true if in the phase where detected an error & have gone back & now seeing if fixed
correct_seen_since_error_handled = 0 # if in above phase, the confident bits in a row since error detection and going back

def calc_mtemps_for_new_ctexts(ctexts_mform_new, d, N):
    return map(lambda x: pow(x, d, N), ctexts_mform_new)

def test(d, e, N):
    m = 0x1043289432 # a random message
    c = pow(m, e, N)
    m_prime = pow(c, d, N)
    return m == m_prime

def gen_rand_ctexts(n):
    return [ random.getrandbits(CTEXT_BITLEN) for i in range(n) ]

def get_ctext_times(ctexts):
    def get_ctext_time(ctext):
        time, _ = target_D_interact(ctext)
        return time

    return map(get_ctext_time, ctexts)

def get_next_bit(ctexts, ctext_times, m_temps, ctexts_mform):
    # for each set M_k, the total times of each ctext in the set
    # and the total number of ctexts in the set
    M_1 = [0, 0] # [total_time, size(M_k)]
    M_2 = [0, 0]
    M_3 = [0, 0]
    M_4 = [0, 0]

    # store all the m_temps for when d_i = 1 and when d_i = 0
    m_temps_bitSet    = []
    m_temps_bitNotSet = []

    # capture the append operations for the two arrays because
    # 'dot's inside python loops is less efficient
    m_temps_bitSet_append    = m_temps_bitSet.append
    m_temps_bitNotSet_append = m_temps_bitNotSet.append

    for i in range(CTEXTS_N):
        # for cases where d_i is or is not set, determine if there is a reduction,
        # and compute the corresponding m_temp
        mtemp_caseBitSet, red_caseBitSet, mtemp_caseBitNotSet, red_caseBitNotSet = sq_and_mul(ctexts_mform[i], m_temps[i])

        # store the mtemps when the d_i = 1 and for when d_i = 0
        m_temps_bitSet_append(mtemp_caseBitSet)
        m_temps_bitNotSet_append(mtemp_caseBitNotSet)

        # for the case when d_i = 1, seperate the mtemps into two groups,
        #  when there was a reduction (M_1) and when there wasn't (M_2)
        if red_caseBitSet:
            M_1[0] += ctext_times[i]
            M_1[1] += 1
        else:
            M_2[0] += ctext_times[i]
            M_2[1] += 1

        # now the same for case when d_i = 0, separating into M_3 and M_4
        if red_caseBitNotSet:
            M_3[0] += ctext_times[i]
            M_3[1] += 1
        else:
            M_4[0] += ctext_times[i]
            M_4[1] += 1

    # now, we calculate statistics to determine which seperation
    # makes more sense, and use this to choose d_i

    # first calculate the mean time for each M_k
    mean_F1 = float( M_1[0] ) / M_1[1]
    mean_F2 = float( M_2[0] ) / M_2[1]
    mean_F3 = float( M_3[0] ) / M_3[1]
    mean_F4 = float( M_4[0] ) / M_4[1]

    print "Seperated into M_k's."
    print "mean_F1 = %f\nmean_F2 = %f\nmean_F3 = %f\nmean_F4 = %f" % (mean_F1, mean_F2, mean_F3, mean_F4)

    # next, the difference in means between M1 vs M2 and M3 vs M4
    diff0 = abs(mean_F3 - mean_F4)
    diff1 = abs(mean_F1 - mean_F2)
    print "diff0 = %f\ndiff1 = %f" % (diff0, diff1)

    # the difference between the two differences
    diff = abs(diff0 - diff1)
    print "diff = %f" % diff

    # choose bit based on which seperation makes more sense
    # return the bit we choose, our level of confidence in it
    # and the new corresponding mtemps after this iter
    if (diff1 > diff0) and (diff > diff_threshold):
        m_temps = m_temps_bitSet
        return 1, diff, m_temps
    elif (diff1 < diff0) and (diff > diff_threshold):
        m_temps = m_temps_bitNotSet
        return 0, diff, m_temps
    else:
        # our confidence level is below the threshold.
        # if we encounter this for numerous bits in a row, then we
        # have detected an error.
        # return the bit for the higher diff, even though diff < threshold
        if diff1 >= diff0:
            m_temps = m_temps_bitSet
            return 1, diff, m_temps
        else:
            m_temps = m_temps_bitNotSet
            return 0, diff, m_temps


def reset_attack(N, e):
    print "ATTACK RESET\n"
    # reset d
    d = 1

    # gen new ciphertexts but use 5000 more samples
    CTEXTS_N += 2000
    ctexts = gen_rand_ctexts(CTEXTS_N)
    ctext_times = get_ctext_times(ctexts)
    init_mont_params(N, e)
    print "\nInit'd Mont params."
    print "omega = %X" % omega
    print "rho   = %X" % rho
    m_temps, ctexts_mform = sq_and_mul_init(ctexts)
    return d, ctexts, ctext_times, m_temps, ctexts_mform


def timing_attack(N, e):
    global CTEXTS_N, errors_n, error_already_handled, correct_seen_since_error_handled

    # create random ciphertexts
    ctexts = gen_rand_ctexts(CTEXTS_N)

    # run all the ciphertexts with the target device
    ctext_times = get_ctext_times(ctexts)

    # we know the first key bit d_i = 1
    d = 1

    # we don't know how many bits d has, so we might have already found it
    if test(d, e, N):
        return d

    # we might have also found d if len(d) = 2
    # we can't use the attack for the last bit so we guess and try both
    # cases manually
    d0 = d + d # d << 1 (d = 0b10)
    if test(d0, e, N):
        return d0

    d1 = d + d + 1 # (d << 1) | 1 (d = 0b11)
    if test(d1, e, N):
        return d1

    # otherwise, len(d) > 2, and we start the rest of our attack

    # init Montgomery params rho and omega (they are globals)
    init_mont_params(N, e)

    print "Init'd Mont params."
    print "omega = %X" % omega
    print "rho   = %X" % rho
    print "\n"

    # convert all ciphertexts to Montgomery form and
    # do square and multiply for the first iter plus
    # square only for the next iter too
    m_temps, ctexts_mform = sq_and_mul_init(ctexts)

    # in case we need to roll back due to error detection, keep
    # track of m_temps values for the last threshold=3 bits of d
    m_temps_before1 = []
    m_temps_before2 = []
    m_temps_before3 = []

    while True:
        # keep track of the last three m_temps values in case we need to roll back
        # upon error detection
        m_temps_before3 = m_temps_before2
        m_temps_before2 = m_temps_before1
        m_temps_before1 = m_temps

        # determine which bit is the next along with how confident we are of this
        bit, confidence_level, m_temps = get_next_bit(ctexts, ctext_times, m_temps, ctexts_mform)

        if confidence_level <= diff_threshold:
            # we were not confident in either bit choice
            print "Not confident in either bit choice. Chose bit = %s with diff = %f" % (bin(bit), confidence_level)

            # incr counter of the bits in a row we have seen of which we
            # are unconfident in
            errors_n += 1
            print "We have been unconfident *%d* times in a row" % errors_n

            # if we have seen 3 in a row of which we are unconfident in,
            # and we are in a phase we were already detected an error and went
            # backwards and flipped the bit: then with both cases we are still
            # uncertain. So, go back and ALSO increase the sample size
            if len(bin(d)) > 3 and errors_n >= 3 and error_already_handled:
                print "We have been unconfident more times in a row than the threshold of 3"
                print "We have already gone backwards and flipped the bit which started our unconfidence."
                print "We will now add 2000 more ctexts and go back 3 bits of the key d and try again"

                # use more sample ciphertexts
                ctexts_new = gen_rand_ctexts(2000) ; CTEXTS_N += 2000
                ctext_times_new = get_ctext_times(ctexts_new)
                ctexts.extend(ctexts_new)
                ctext_times.extend(ctext_times_new)

                print "Added ctexts. CTEXTS_N = %d" % CTEXTS_N

                # remove the last 3 bits (go back to last state we were confident)
                d = d >> 3

                print "Removed last three bits of d. d = %s" % bin(d)

                # we need to store the new ctexts in mform
                ctexts_mform_new = convert_to_mforms(ctexts_new)
                ctexts_mform.extend(ctexts_mform_new)

                # sq and multiply the ctexts by all the d we deem to be accurate so far
                m_temps_new = calc_mtemps_for_new_ctexts(ctexts_mform_new, d, N)

                # roll back m_temps by three sq and multiplies with bad last three key bits
                m_temps = m_temps_before3
                m_temps_before1 = []
                m_temps_before2 = []
                m_temps_before3 = []

                # add new m_temps
                m_temps.extend(m_temps_new)

                # now, try to determine the next bit again
                # if we are confident, then continue
                # else, just abort for now TODO loop in this section i.e. add even more ctexts
                m_temps_before3 = m_temps_before2
                m_temps_before2 = m_temps_before1
                m_temps_before1 = m_temps
                print "Trying again with more ctexts. Calling get_next_bit ..."
                bit, confidence_level, m_temps = get_next_bit(ctexts, ctext_times, m_temps, ctexts_mform)
                if confidence_level <= diff_threshold:
                    print "F*ck it failed. Restarting entirely with brand new and more ciphertexts.\n"
                    d, ctexts, ctext_times, m_temps, ctexts_mform = reset_attack(N, e)

                else:
                    print "Success."
                    # reset error detection state too
                    errors_n = 0
                    error_already_handled = False
                    correct_seen_since_error_handled = 0

            # if we have seen 3 in a row of which we are unconfident in,
            # then go back 3 bits (to the bit which started the unconfidence)
            # and flip it and see if that increases our confidence
            elif len(bin(d))-2 > 3 and errors_n >= 3 and not error_already_handled:
                print "We have been unconfident more times in a row than the threshold of 3"
                print "So, we presume we made an error in choice 3 bits ago."

                # go back 3 bits and make the different choice

                # get the bit from 3 iters ago
                wrong_bit = (d >> (2)) & 1

                # remove the bits after what we are confident about
                d = d >> 3

                print "Removed last three bits of d. d = %s" % bin(d)

                # set the bit to append to the other choice (flip wrong_bit)
                bit = (~wrong_bit & 1)

                print "The flipped bit to append to d is %s" % bin(bit)

                # reset errors_n counter
                errors_n = 0

                # roll back m_temps by 3 sq and multiplies
                m_temps = m_temps_before3
                m_temps_before1 = []
                m_temps_before2 = []
                m_temps_before3 = []

                # mark that we've detected an error.
                # so, if flipping the wrong bit doesn't increase the confidence
                # level, we'll add more ctext samples
                error_already_handled = True
                correct_seen_since_error_handled = 0

        else:
            # we were confident enough in our bit choice

            # so, we have made 0 choices in a row of which we are unsure
            errors_n = 0

            # if we are in a phase were we went back iters because we thought
            # we may have selected a wrong bit multiple 3 times in a row, but now
            # we have selected a bit with which we are confident off, then
            # increment a counter of how many bits in a row we have since seen
            # with increased confidence
            if error_already_handled:
                print "Confident in bit choice after error handling."
                correct_seen_since_error_handled += 1

            # if we have seen enough in a row with increased confidence since
            # flipping the bit, then assume we fixed an error and reset
            # our error detection state
            if error_already_handled and correct_seen_since_error_handled >= 3:
                print "Confident in bit choice after error handling 3 times in a row. Resetting error detection state."
                error_already_handled = False
                correct_seen_since_error_handled = 0

        #######
        # we are now done dealing with error detection.

        # update d with the next bit
        d = d + d + bit # (d << 1) | bit - adding is faster than shifting in python

        print "\nDetermined next bit. d = %s\n" % bin(d)

        # check if we have found d
        # we can't use attack on the last bit of d so we need to
        # check both cases
        d0 = d + d
        if test(d0, e, N):
            print "\nDetermined next bit. d = %s\n" % bin(d0)
            return d0
        d1 = d + d + 1
        if test(d1, e, N):
            print "\nDetermined next bit. d = %s\n" % bin(d1)
            return d1

        # if d >= N, and we havent found the correct d, then the attack has failed
        # in actual fact, if d >= phi_N we have failed, but we don't know phi_N
        # or N's factors (else we could find d easily). However, we do know that
        # phi_N < N, so we know we've failed if d >= N
        if d >= N:
            print "Attack failed. d = %s" % bin(d)
            # reset, use new and more ciphertexts and try again
            d, ctexts, ctext_times, m_temps, ctexts_mform = reset_attack(N, e)



###################################################
## Target Device I/O
###################################################

# count interactions with each target
interactions_D = 0
interactions_R = 0

def create_target_D():
    global target_D
    target_D = subprocess.Popen( args   = ATTACK_TARGET + ".D",
                                 stdout = subprocess.PIPE,
                                 stdin  = subprocess.PIPE )

def create_target_R():
    global target_R
    target_R = subprocess.Popen( args   = ATTACK_TARGET + ".R",
                                 stdout = subprocess.PIPE,
                                 stdin  = subprocess.PIPE )

def target_D_interact(c):
    """serialises c::int as hex integer string, sends it to D
       and returns the result time::int and m::int"""
    global interactions_D

    target_D.stdin.write("%X\n" % c); target_D.stdin.flush()
    time = int(target_D.stdout.readline().strip(), 10)
    m    = int(target_D.stdout.readline().strip(), 16)
    interactions_D += 1
    return time, m

def target_R_interact(c, N, d):
    """serialised c,N,d::int as hex integer strings, sends them
       to R and returns the result time::int and m::int"""
    global interactions_R

    target_R.stdin.write("%X\n" % c); target_R.stdin.flush()
    target_R.stdin.write("%X\n" % N); target_R.stdin.flush()
    target_R.stdin.write("%X\n" % d); target_R.stdin.flush()
    time = int(target_R.stdin.readline().strip(), 10)
    m    = int(target_r.stdin.readline().strip(), 16)
    interactions_R += 1
    return time, m

def close_target_D():
    target_D.terminate()

def close_target_R():
    target_R.terminate()


###################################################
## Read Attack Params
###################################################

def read_attack_params():
    with open(CONF_FILE, 'r') as f:
        N = int(f.readline().strip(), 16)
        e = int(f.readline().strip(), 16)
    return N, e


###################################################
## Main
###################################################

if __name__ == "__main__":
    if len(sys.argv) >= 3:
        # get attack target and conf file locations
        ATTACK_TARGET = sys.argv[1]
        CONF_FILE     = sys.argv[2]

        # create attack targets D and R
        targetD = create_target_D()
        targetR = create_target_R()

        # parse the attack params to get the public key
        N, e = read_attack_params()
        print "Read attack params (RSA public key)."
        print "N = %X" % N
        print "e = %X" % e
        print "\n"

        # execute the timing attack and track the time taken
        start = timer()
        d = timing_attack(N, e)
        end = timer()

        # print a summary
        print "Attack complete."
        print "Elapsed time        = %f" % (end - start)
        print "Interactions with D = %d" % (interactions_D)
        print "Interactions with R = %d" % (interactions_R)
        print "Total interactions  = %d" % (interactions_D + interactions_R)
        print "Recovered d (hex)   = 0x%X" % (d)
        print "Recovered d (bin)   = %s"   % (bin(d))

        # all done, just close the subprocesses
        close_target_D()
        close_target_R()

    else:
        # just testing my result here
        CONF_FILE = sys.argv[1]
        #N = 0x854B2B0C8991452EF8AD02D101D694BDC6B84B43B55FC9A94ED25EC805797AB996ABB36CE02D90F55736006694D5CBE12D3CE01635EE0625A12B24B94E2CB46DAE72F91AEF22D063813E7451D6154308FDA7BBEB61252CF0CB179E1B1167F7A1A468B0486EFEE9611EA168A931588B356F8A48FA50F4E51655D979A0DDA10B8F
        #e = 0x705A852581D95EC3B5BE8C8A820C64E3BF729AA1454CF772442CB783F66FD3CFB645DF999BEE9EA73626305891B143F58654627FEA65B0ECF1F66E2248865D47463B6F69B8D00278813B7B19A5EE88CA7AA2E7FA960F71AE915DAE447245631FA3AEB86E9018E2EB7152E5F0E1045640203ED4E4772F849055898B4F3AD053B7
        N, e = read_attack_params()
        d = 0b1001101100000001111001000010010111011001100111101011110000110111
        result = test(d, e, N)
        if result:
            print "SUCCESS"
        else:
            print "FAIL"

        # now do lots of tests
        ms = [ random.getrandbits(50) for i in range(100) ]
        cs = map( lambda m: pow(m, e, N), list(ms) )
        m_primes = map( lambda c: pow(c, d, N), list(cs) )
        if ms == m_primes:
            print "SUCCESS"
        else:
            print "FAIL"
