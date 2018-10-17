# Stage 4: Power Attack

I implemented the differential power analysis attack. This attack does not follow a specific paper, but a general DPA attack strategy described in lectures / the DPA book by Prof. Elisabeth Oswald. I have implemented
the attack in C + OpenMP. Also require OpenSSL for testing AES keys.

In this attack, the attack situation is one in which the attack ier attacking XTS-AES (used for encrypting hard drives). XTS-AES.blockDec is more complicated than a typical attack against AES. One must understand the XTS-AES framework specification in detail. I describe my attack
this in the following. The XTS-AES spec is also included [here](IEEE-1619-2007.pdf).

## Files

- [attack.[ch]](attack.c) is the implementation of the differential power attack.
- [21452.D](21452.D) a linux executable that simulates XTS-AES block decryption. It outputs a 1-block XTS-AES plaintext and a power consumption trace.
- [discussion.txt](discussion.txt) contains discussion and detailed answers for the following questions.
  - As a security consultant, you have been working on a prototype of the
    attack target; before the product is deployed, the vendor is willing to
    make any alteration necessary (in software or hardware) to prevent the
    attack.  Explain the options available, and which one you would
    recommend.
  - The vendor of the attack target opts to replace the 8-bit Intel 8051
    micro-processor with a modern, 32-bit ARM Cortex-M0; they supplement it
    with hardware support for that allows single-cycle operations on 32
    bits of the AES state.  Doing so will naturally reduce the latency of
    encryption or decryption, but they *also* want to know if it might make
    the device more secure wrt.  DPA attacks: what is your assessment, and
    why ?
  - The vendor of the attack target decides to use AES-192 rather than
    AES-128; their argument is that longer keys will naturally improve
    security in general *and* vs.  DPA attacks.  Explain if and why you
    agree or disagree with this argument.

## Attack params

The following parameters are passed to the `launch_attack` function, which is the entry point to the attack.

### NUM_I
The number of sector addresses i to generate. I.e., the number of interactions with target device D.

### NUM_POWER_SAMPLES
The number of time samples from each trace to use. To recover Key2, I use the first NUM_POWER_SAMPLES samples
from each power trace. For Key1, I use the last NUM_POWER_SAMPLES samples from each trace.
This param would probably be better named 'LEN_POWER_TRACES', but oh-well.

### Choosing parameter values
I have observed (by printing it out during attack) that the max correlation rho_ck,ct is usually around 0.975.
Using the equations and table from Elisabeth's slides, this means that using
20 traces (i.e. NUM_I=20) will give allow me to distinguish between the two distributions (correlation with
incorrect key hypothesis vs correlation with correct key hypothesis) with confidence greater than 0.9999.

## Recovering Key2

The first step of XTS-AES.blockDec(Key=Key1||Key2, C, i, j) is to encrypt the sector address i under Key2, i.e.
AES-Enc(Key2, i). So, I generate NUM_I random AES-128 plaintexts to act as sector addresses. I send these to
attack target D. I receive the power traces and plaintexts as a result of decryption from D.

I run the DPA attack from lectures. I attack the SubBytes operation in the first round. This recovers round
key 0, which is the cipher key, so Key2. So, the power model I use is `SubBytes(AddRoundKey(d_i, k_j))`.
I take the hamming weights of these under every possible key hypothesis and find the correlation with the
power trace at every time t. The key hypothesis for the byte I choose is the one that gives the highest
correlation with a power trace (at any time t).

I use Pearson's correlation coefficient for correlation measure.
I use a lookup table (size 256) for finding Hamming Weights.
I use OpenMP to spawn threads and search for key bytes in parallel to speed up attack

## Recovering Key1

We now have Key2. This means that for each i, we can calculate the output of the encryption
AES-Enc(Key2, i). We also used a fixed j=0 every time we interacted with the target. This means
that alpha^j=1. So, we also know T = AES-Enc(Key2, i) * alpha^j = AES-Enc(Key2, i).
We also have each P as we given this by the attack target when we interacted.
We can XOR P and T together to get PP.
We then attack the final round of decryption AES-Dec(Key1, CC)
The key used in the final round is round key rk^(0) = Key1.
Let target = input to InvSubBytes in final round
We attack the final following steps:
  PP = AddRoundKey(rk^(0)=Key1, InvSubBytes(target))
We hypothesise key values and we know PP.
We then run the same power model as before to retrieve the target, which is what we take the hamming weight off:
  target = SubBytes(AddRoundKey(PP, K)).
We use the final NUM_POWER_SAMPLES power traces.
The power traces will still continue to give the correct correlations with this power model.
This is because, even though the algorithm computes AddRoundKey(Key1, InvSubBytes(target)),
its still doing the same thing, but in the reverse order. So, at some time value t,
the correlation will still be observed with the hamming weights.

## Robustness

If attack fails (returns key that doesn't pass test), then we try again with twice the NUM_I and
NUM_POWER_SAMPLES.
At time of writing, this does not work. However, as discussed above, we have used formula to come to
a reasoned initial choice of starting parameters s.t. attack should succeed first time.

## Running the attack

```bash
$ make clean all
$ ./attack ./21452.D  # note, requires './' in './21452.D'
```