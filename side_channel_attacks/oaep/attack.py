import binascii
import math
import sys
import subprocess
import hashlib


ATTACK_TARGET = ""    # name of simulated attack executable
CONF_FILE     = ""    # path to conf file including attack params
target_in     = None  # ref to stdin  of attack target
target_out    = None  # ref to stdout of attack target
target        = None  # ref to           attack target
oracle_calls  = 0     # total number of oracle calls made


###################################################
## Error Codes from the target device
###################################################

from collections import namedtuple

ErrorCodesClass = namedtuple( 'ERR', [
  'SUCCESS', 'Y_GTE_B', 'Y_LT_B',
  'PLAINTEXT_OUT_OF_RANGE', 'CTEXT_OUT_OF_RANGE',
  'MESSAGE_TOO_LONG', 'CTEXT_LENGTH_INVALID_N',
  'CTEXT_LENGTH_INVALID_H'
])

ERR = ErrorCodesClass( 0, 1, 2, 3, 4, 5, 6, 7 )

# can now access error codes with ERR.SUCCESS etc


###################################################
## Helpers
###################################################

def ceil_div( a, b ):
    """ceiling division that works with bignum ints"""
    return int(-(-a / b))

def oct_bin_xor( seq1, seq2 ):
    if len( seq1 ) != len( seq2 ):
        raise ValueError( "oct_bin_xor: len(seq1)!=len(seq2)")
    return ''.join( ( chr( ord( x ) ^ ord( y ) ) for ( x, y ) in zip( seq1, seq2 ) ) )


###################################################
## Target Device I/O
###################################################

def target_spawn():
    global target_in
    global target_out
    global target

    target = subprocess.Popen( args   = ATTACK_TARGET,
                               stdout = subprocess.PIPE,
                               stdin  = subprocess.PIPE )
    target_in  = target.stdin
    target_out = target.stdout

def target_send( string ):
    target_in.write( "%s\n" % string ) ; target_in.flush()

def target_read():
    return target_out.readline().strip()

def target_terminate():
    target.terminate()

def target_challenge( l_hat, c_hat ):
    """sends label l and ciphertext c both in octet string form to the target"""
    target_spawn()
    target_send( l_hat )
    target_send( c_hat )
    error = int( target_read() )
    target_terminate()
    return error

def create_oracles( N_len, N, e, c, l_hat ):
    def oracle( x ):
        global oracle_calls
        oracle_calls += 1
        x_hat = int_to_oct_str( x, N_len )
        return target_challenge( l_hat, x_hat )

    def oracle_f( f ):
        # x = f^e * c (mod n)
        x = pow( f, e, N )
        x = ( x * c ) % N
        return oracle( x )

    return oracle, oracle_f


###################################################
## Number representation conversion
###################################################

def oct_seq_to_str( oct_seq ):
    return binascii.b2a_hex( "".join( [ chr( t ) for t in oct_seq ] ) )

def oct_str_to_seq( oct_str ):
    return [ ord( t ) for t in binascii.a2b_hex( oct_str ) ]

def hex_str_to_int( hex_str ):
    return int( hex_str, 16 )

def oct_str_to_int( oct_str ):
    # convert to octet sequence
    oct_seq = oct_str_to_seq( oct_str )

    # convert to little-endian integer
    return sum( [ oct_seq[ i ] * 2 ** ( 8 * ( len( oct_seq ) - i - 1 ) ) for i in range( len( oct_seq ) ) ] )

def int_to_oct_seq( x, x_len ):
    if x >= 256^x_len:
        pass#raise ValueError( "integer too large, x_len=%d" % x_len )

    digits = []
    while x:
        digits.append( int( x % 256 ) )
        x /= 256 # change to bit shift?

    for i in range( x_len - len( digits ) ):
        digits.append( 0 )

    digits = digits[ ::-1 ]
    return digits

def int_to_oct_str( x, x_len ):
    oct_seq = int_to_oct_seq( x, x_len )
    oct_str = oct_seq_to_str( oct_seq )
    return oct_str

def bin_to_oct_seq( bin_str ):
    return oct_str_to_seq( binascii.b2a_hex( bin_str ) )

def i2osp(x, x_len):
    """
    returns octet string in binary string format e.g. "\xff\xff\xff",
    unlike int_to_oct_str which returns an octet string in as hexadecimal
    string e.g. "FFFF".
    """
    # check x will fit into an octet str of desired length
    if x > 256 ** x_len:
        pass#raise ValueError( "integer too large, x_len=%d" % x_len )

    h = hex( x )[ 2 : ]
    if h[ -1 ] == 'L':
        h = h[ : -1 ]
    if len( h ) & 1 == 1:
        h = '0%s' % h
    x = binascii.a2b_hex( h )

    # pad with zeros upto x_len and return the binary octet string
    return '\x00' * int( x_len - len( x ) ) + x


###################################################
## Read Manger Attack Params
###################################################

def read_attack_params():
    # read in the parameters
    with open( CONF_FILE , 'r' ) as f:
        N_hat = f.readline().strip()
        e_hat = f.readline().strip()
        l_hat = f.readline().strip()
        c_hat = f.readline().strip()

    # convert params to integers
    N = hex_str_to_int( N_hat )
    e = hex_str_to_int( e_hat )
    l = oct_str_to_int( l_hat )
    c = oct_str_to_int( c_hat )

    # length of N in octet string representation
    N_len = int( len( N_hat ) / 2 )

    return N_len, N_hat, e_hat, l_hat, c_hat, N, e, l, c


###################################################
## Manger Attack
###################################################

def manger_attack( N_len, N_hat, e_hat, l_hat, c_hat, N, e, l, c ):
    # create oracles for interacting with target device
    oracle, oracle_f = create_oracles( N_len, N, e, c, l_hat )

    # set B
    k = N_len
    B = pow( 2, 8 * ( k - 1 ) ) # change to bitshift?

    # if N < 2*B then we need to search range of f*m in [0,B) and [n,2B)
    if 2 * B >= N:
      # XXX NotYetImplemented
      raise RuntimeError( "2B >= N: case not implemented yet" )

    # ERROR CODES:
    #   Y_GTE_B  -> y >= B
    #   Y_LT_B   -> y <  B

    # Step 1
    f_1 = 2
    error = -1
    while ( error != ERR.Y_GTE_B ):
        error = oracle_f( f_1 )
        if error == ERR.Y_LT_B:
            f_1 = 2 * f_1
        elif error != ERR.Y_GTE_B:
            raise RuntimeError( "attack error step 1: error=%d" % error )

    # Step 2
    f_2 = int( ( ( N + B ) / B ) * ( f_1 / 2 ) )
    error = -1
    while ( error != ERR.Y_LT_B ):
        error = oracle_f( f_2 )
        if error == ERR.Y_GTE_B:
            f_2 = f_2 + int( f_1 / 2 )
        elif error != ERR.Y_LT_B:
            raise RuntimeError( "attack error step 2: error=%d" % error )

    # Step 3
    m_min = ceil_div( N, f_2 )
    m_max = int( ( N + B ) / f_2 )

    error = -1
    while ( m_min != m_max ):
        f_tmp = int( ( 2 * B ) / ( m_max - m_min ) )
        i     = int( ( f_tmp * m_min ) / N )
        f_3   = ceil_div( i * N, m_min )

        error = oracle_f( f_3 )

        if error == ERR.Y_GTE_B:
            m_min = ceil_div( i * N + B, f_3 )
        elif error == ERR.Y_LT_B:
            m_max = int( ( i * N + B ) / f_3 )
        else:
            raise RuntimeError( "attack error step 3=%d" % error )

    # Attack Complete
    return m_min


###################################################
## EME-OAEP
###################################################

def eme_oaep_decode( em_bin, l_bin ):
    print "----------------------------------------------"
    print "EME-OAEP-Decode:"

    emLen = len( em_bin )
    hLen = hashlib.sha1().digest_size

    print( "emLen = %d" % emLen )
    print( "hLen  = %d" % hLen )

    em_str = binascii.b2a_hex( em_bin )
    print( "em_str = %s" % em_str )

    y, maskedSeed, maskedDB = em_bin[ : 1 ], em_bin[ 1 : hLen + 1 ], em_bin[ 1 + hLen : ]

    print "y          = %3d %s" % ( len( y ), binascii.b2a_hex( y ) )
    print "maskedSeed = %3d %s" % ( len( maskedSeed ), binascii.b2a_hex( maskedSeed ) )
    print "maskedDB   = %3d %s" % ( len( maskedDB ), binascii.b2a_hex( maskedDB ) )

    seedMask = mgf1_sha1( maskedDB, hLen )

    print "seedMask   = %s" % binascii.b2a_hex( seedMask )

    seed = oct_bin_xor( maskedSeed, seedMask )

    print "seed       = %s" % binascii.b2a_hex( seed )

    dbMask = mgf1_sha1( seed, emLen - hLen - 1 )

    print "dbMask     = %s" % binascii.b2a_hex( dbMask )

    DB = oct_bin_xor( maskedDB, dbMask )

    print "DB         = %s" % binascii.b2a_hex( DB )

    lHash_prime = DB[ : hLen ]

    print "lHash_prime= %d %s" % (len(binascii.b2a_hex( lHash_prime )), binascii.b2a_hex( lHash_prime ))

    remaining = DB[ hLen : ]

    print "remaining  = %s" % binascii.b2a_hex( remaining )

    ix = remaining.find('\x01')
    if ix == -1:
        raise ValueError( "decoding error" )

    if remaining[ : ix ].strip('\x00') != '':
        raise ValueError( "decoding error" )

    m_bin = remaining[ ix + 1 : ] # +1 as remaining[ix] = '\x01'

    print "m          = %s" % binascii.b2a_hex( m_bin )

    lHash = hashlib.sha1( l_bin ).digest()

    if lHash_prime != lHash:
        raise ValueError( "decoding error" )

    return m_bin


def mgf1_sha1( seed, l ):
    hLen = hashlib.sha1().digest_size

    if l > pow( 2, 32 ) * hLen:
        raise ValueError( "mask too long" )

    T = ''
    for i in range( 0, ceil_div( l, hLen ) ):
        C = i2osp( i, 4 )
        T = T + hashlib.sha1( seed + C ).hexdigest()
    return binascii.a2b_hex( T[ : 2 * l ] )


###################################################
## Tests
###################################################

def test_mgf():
    seed    = 'aafd12f659cae63489b479e5076ddec2f06cb58f'
    correct = '06e1deb2369aa5a5c707d82c8e4e93248ac783dee0b2c04626f5aff93edcfb25c9c2b3ff8ae10e839a2ddb4cdcfe4ff47728b4a1b7c1362baad29ab48d2869d5024121435811591be392f982fb3e87d095aeb40448db972f3ac14eaff49c8c3b7cfc951a51ecd1dde61264'

    seed_bin = binascii.a2b_hex( seed )
    seed_seq = oct_str_to_seq( seed )
    seed_str = binascii.b2a_hex( seed_bin )
    print "seed_bin = %s" % seed_bin
    print "seed_seq = %s" % seed_seq
    print "seed_str = %s" % seed_str

    result = binascii.b2a_hex( mgf1_sha1_2( seed_bin, 107 ) )

    success = correct == result
    print "TEST RESULT = %s" % success
    print "result  = %s" % result
    print "correct = %s" % correct


###################################################
## Main and launch attack
###################################################

def exec_attack():
    # read in attack parameters
    N_len, N_hat, e_hat, l_hat, c_hat, N, e, l, c = read_attack_params()

    # then attempt the Manger attack to recover the encoded msg
    em = manger_attack( N_len, N_hat, e_hat, l_hat, c_hat, N, e, l, c )
    #em = int("A2DA6027779631D7FB268F2D30E6C3B079B7B679B32F25F1A177E70487A40B5F8C186BE4B57DF3F9844B19B92DBFCF89DADCA5FC57D6274716E6DE318E2C89597578ECE6E5171EA4700FDB17320D7219FD45BC2F99F744BA190D53759CAA86BD45429F36977BA1C372ED90D2ADF67F4EA727111997FC0CBDF7347417505406", 16)

    # print the output of the attack
    print "----------------------------------------------"
    print "ENCODED MESSAGE RECOVERED"
    print "em               = %X" % ( em )
    print "em^e mod N       = %X" % ( pow( em, e, N ) )
    print "c                = %X" % ( c )
    print "\n"

    # decode the message to recover the secret
    em_bin = i2osp( em,  N_len )
    l_bin  = i2osp( l, ceil_div( len( l_hat ), 2 ) )
    m_bin  = eme_oaep_decode( em_bin, l_bin )

    # print the output
    print "\n\n----------------------------------------------"
    print "MESSAGE DECODED. SUMMARY:"
    print "m                = %s" % binascii.b2a_hex( m_bin )
    print "interactions     = %d" % oracle_calls


if __name__ == "__main__" :
    if ( len( sys.argv ) >= 3 ):
        # set the attack params
        ATTACK_TARGET = sys.argv[ 1 ]
        CONF_FILE     = sys.argv[ 2 ]

        # execute the attack
        exec_attack()

    else:
        # run tests
        test_mgf()
