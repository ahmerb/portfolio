# Timing attack on RSA (Montgomery multiplication)

I implement the attack on the square presented in [1] by Dhem et al.
Implementation in Python 2.
Requires the external library pycrypto. I use this to compute a modular
inverse to compute the Montgomery parameter omega.

Execute with

`bash$ python attack.py ./21452 21452.conf`

## Files

 - [attack.py](attack.py) contains the implementation of the attack. I've made some (but not comprehensive) effort to make it as efficient as possible, using typical techniques in Python.
 - [21452.D](21452.D) is a linux executable that simulates an implementation of RSA decryption that *does not use the Chinese Remainder Theorem*, and, *uses Montgomery multiplication*. It also outputs the time taken to execute (in clock cycles).
 - [21452.conf](21452.conf) is an example input to RSA decryption. The first line is a public modulus `N`. The second line is a corresponding public exponent `e`.
 - [discussion.txt](discussion.txt) contains detailed answers to the following questions.
    - The vendor of the attack target is concerned that news of this attack
    could scare off potential customers; they will make any alteration
    necessary (in software or hardware) to prevent the attack.  Explain the
    options they have, and which one you would recommend.
    - The vendor of the attack target is concerned that a similar attack may
    also apply to their implementation of ElGamal encryption.  Explain the
    potential for such an attack, and how it differs wrt.  the case of RSA.
    - Numerous factors might produce noise within measurements of execution
    time: based on the given attack target and context, outline at least
    two examples.
    - Imagine you read a research paper that suggests a larger key (e.g.,
    2048-bit rather than 1024-bit) could help to prevent this attack.
    Explain whether and why you think this is right (or wrong).


## Error Detection
I have implemented some form of error detection, as in the Dhem et al paper.
My implementation algorithm is as follows:

  1. If we are not confident in either separation (below the confidence threshold), then
     continue as normal with the bit with which we are still more confident with
     But, if we chose the wrong bit, the remaining separations will be meaningless.
  3. If we determine 3 bits in a row with confidence below the threshold, then backtrack by
     3 bits.
       - Remove the last 3 bits of R
       - Flip the bit that caused the confidence level to drop.
       - We need to roll back the m_temps by 3 square and multiplies. To do this, I simply maintain
         lists for the values of m_temps for the last 3 bit choices.
  4. Now, if the next three bit choices after backtracking give a high confidence again,
     we can conclude that we had initially made the wrong choice but we've now fixed it.
     So, reset the error detection state (go to Step 1)
  5. But, if the next three bit choices after backtracking also give a low confidence again,
     then the following possibilities have occurred:
       - We had chosen the correct bit initially and have now chosen an incorrect bit
       - The error occurred further back than the bit we flipped so we need to backtrack even more.
       - The ciphertext times have too much noise. They may also be unluckily bad choices of ciphertext.
         So, We need more ciphertexts.
       - Our confidence threshold parameter is just set unrealistically high.
  6. My approach to handle this is as follows:
       - If this is the first time this has happened (for this error), then backtrack 3 bits again,
         but, add 2000 more ciphertexts. Compute their m_temps for the key bits we're confident in and
         get their run times with d.
       - If this isn't the first time this has happened for this error (i.e. we've already tried the above),
         then, we completely reset. Remove all key bits. Delete all ciphertexts. Generate new ciphertexts but
         use double the number of ciphertexts. Start again from key d=0b1.

The last step is drastic action but I have not had time to implement a more sophisticated algorithm.
I have found that performance of error detection is more dependent on the starting input parameters,
so if I had more time I would have implemented a calibration stage first.


## Montgomery Multiplication

The target devices use CIOS based Montgomery multiplication.
Python uses a BigNum lib under the hood and abstracts away access to limbs etc.
So, I used the alternative formulation given in Dan's extra lecture slides, i.e.
the MonPro algorithm from the paper [2] by Koç, Acar and Kaliski.

## References

[1] Jean-François Dhem, François Koeune, Philippe-Alexandre Leroux, Patrick Mestré, Jean-Jacques Quisquater, and Jean-Louis Willems. 1998. A Practical Implementation of the Timing Attack. In Proceedings of the The International Conference on Smart Card Research and Applications (CARDIS '98), Jean-Jacques Quisquater and Bruce Schneier (Eds.). Springer-Verlag, London, UK, UK, 167-182.

[2] Çetin Kaya Koç, Tolga Acar, and Burton S. Kaliski, Jr.. 1996. Analyzing and Comparing Montgomery Multiplication Algorithms. IEEE Micro 16, 3 (June 1996), 26-33. DOI: https://doi.org/10.1109/40.502403
