# Fault Attack against AES-128 only requring a single Fault

I have implemented the fault attack described in [5].
Implementation in C + OpenMP. Also requires OpenSSL for testing AES keys.
The attack only requires a *single* fault. It is split into two stages which
each drastically reduce the search space for an exhaustive search.
Faults are not physically injected: I use an executable which takes a parameter to tell it to induce random faults.

## Files

 - [attack.[ch]](attack.c) contains the implementation of the attack.
 - [21452.D](21452.D) is a linux executable that simulates AES-128. It allows inducing a randomised fault at a specific (i,j) in the state matrix, at a specific round, and before or after a specific AES round function (`AddRoundKey` / `SubBytes` / `ShiftRows` / `MixColumns`).
 - [discussion.txt](discussion.txt) contains detailed discussions where I answer the following questions.
    - Consider that instead of influencing data (e.g., the AES state), a
      fault might influence control-flow (e.g., a conditional statement)
      somehow during execution.  Stating any assumptions you make, give an
      alternative attack strategy based on this ability.
    - Outline the methods available for hardening this AES implementation
      against fault attacks; for each method, explain any assumptions you
      make and compare it with the others wrt.  metrics such as robustness,
      latency, and memory footprint.
    - The question outlines one method to induce faults (i.e., via a clock
      glitch).  For the same attack target, construct a set of alternative
      methods and explain whether they seem applicable given the context.
    - One way to reduce the effectiveness of DPA attacks on AES is to use a
      masking countermeasure.  Explain if and why the *same* countermeasure
      could also have an impact on the effectiveness of fault attacks.
  

## Precomputation

To speed up stage 2 of the attack, I precompute a multiplication table
for GF(2^8). This is called `MulTab` in code.

For stage 1 and stage 2 of the attack, I need to use the AES SBox and
inverse SBox. I store these as a lookup table. The values are taken from
Wikipedia.

I also store the AES rcon as a lookup table. This is used to evolve
round keys. (used in stage 2). Also taken from Wikipedia.


## Setup attack

I generate a random message to send to the attack target D. I ensure
that the message is all nonzero.

One thing to note is that, to make indexing a lot easier, I pad the
message (m) and faulty and fault-free ciphertexts (c, c_faulty) and key
hypotheses with a starting zero. This makes indexing consistent with whats
used in the papers. I.e., k_guess in code corresponds to a k_1 in the paper.

## Interaction

I send the random message to D to get the fault free encryption, c.
I also induce a fault at the 8th round, in SubBytes round function,
before the round function, at row i=0, at column j=0 in the AES state
matrix. This gives me the faulty ciphertext c_faulty.
I then start stage1 of the attack.

If the faulty ciphertext we receive from D is actually a zero fault, then
this is equivalent to no fault being induced. For the attack to work, we must
ensure that this doesn't happen. So, if we receive a c_faulty==c from D, then
we interact again (and induce another fault).

## Stage1

I use the formulas from the paper to reduce all the possible key
hypotheses.
To do operations in GF(2^8), I have implemented the functions
gf28_mult, gf28_sub and gf28_add.
The following functions use each system of equations from the paper
to reduce the possible key hypotheses.
  - stage1_equations1 for k1, k8, k11, k14
  - stage1_equations2 for k2, k5, k12, k15
  - stage1_equations3 for k3, k6, k9 , k16
  - stage1_equations4 for k4, k7, k10, k13

I use the structs

```c
typedef struct {
  Set1Hypothesis ** set1;
  Set2Hypothesis ** set2;
  Set3Hypothesis ** set3;
  Set4Hypothesis ** set4;
  KeyHypothesesNs ns; // lengths of above lists
} KeyHypotheses;

typedef struct {
  byte k1;
  byte k8;
  byte k11;
  byte k14;
} Set1Hypothesis;

.
.
.

```

to store all of the possible key hypotheses after stage1. The top
level struct is stored in Structs of Arrays fashion (as opposed to
arrays of structs) to improve runtime. I still opted to use structs
rather than plain arrays for readability.

NOTE todo: the arrays `SetXHypothesis ** setX` are fixed capacity 512.
Its possible to get longer than this (if I remember quickly).
Either use dynamic arrays or just increase the capacity.


## Stage 2

We now have a reduced set of possible key hypotheses. I now
implement stage two from [1]. We loop through all possible key hypotheses,
from stage1, and then

  - Check this key hypothesis satisfies the equations from the paper for stage2
  - If it does, then
    - We invert this key hypothesis by evolving it backwards 10 times. This
      gives us the actual cipher key (the key hypotheses are for the final round key).
    - Use OpenSSL/aes to test this cipher key hypothesis. I.e., check encrypting the message
      we send to D under this key gives us the fault free ciphertext we received from D.
    - If so, we have found the key, else, keep searching.

I use OpenMP to search this key hypothesis space using multiple threads at once.


## Running the attack

```bash
$ make clean all
$ ./attack ./21452.D  # note, requires './' in './21452.D'
```

## References

[1] Michael Tunstall, Debdeep Mukhopadhyay, and Subidh Ali. 2011. Differential fault analysis of the advanced encryption standard using a single fault. In Proceedings of the 5th IFIP WG 11.2 international conference on Information security theory and practice: security and privacy of mobile devices in wireless communication (WISTP'11), Claudio A. Ardagna and Jianying Zhou (Eds.). Springer-Verlag, Berlin, Heidelberg, 224-233

