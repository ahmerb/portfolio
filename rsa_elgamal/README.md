# Implementations of RSA and ElGamal Cryptosystems

Third-year coursework in which implemented RSA and ElGamal cryptosystems in C using GMP. Some features of GMP are not used. Instead, I implement many optimisations manually. The implementations make no effort to be resistant to side-channel attacks, but are efficient.

In particular, I implemented optimisations like:
 - Use of Chinese Remainder Theorem
 - Sliding Window Exponentiation
 - Montgomery Multiplication

I also ensured I used a reasoned and proper approach to pseudorandom key generation for ElGamal ephemeral keys.

More details are given below, which is copied from the README I included when submitting the assignment.

---

## Compiling and running my code

Should work as the examples in the question text.

  ```
  make
  ./modmul stage1 < stage1.input > stage1.test.output
  ./modmul stage2 < stage2.input > stage2.test.output
  ./modmul stage3 < stage3.input > stage3.test.output
  ./modmul stage4 < stage4.input > stage4.test.output
  ```

## Cryptographically Secure Pseudo Random Number Generation

To seed a CSPRNG, we need a source of sufficient entropy. On Linux, the special
devices `/dev/random` and `/dev/urandom` allow access to environmental noise
collected from device drivers and other sources[1].

The generator keeps an estimate of the number of entropic bits in the entropy
pool. /dev/random will block if the estimate of entropy is too low. This could
cause ElGamal encryptions, where we need to generate an ephemeral key k, to
become very slow as we wait for enough new environmental noise to occur.

/dev/urandom does not block. However, we must address the case that there is not
enough bits of entropy for sufficient cryptographic security. As the computer
runs, more environmental noise occurs and so entropy increases. Therefore, right
after startup, there might not be enough entropy. However, Linux saves a random
seed upon shutdown, ready for the next boot. Thus, the only time there may not
be enough entropy is upon booting the computer for the very first time.

There are lots of conflicting opinions online. Some of them are given in [2]
and [3]. I have followed the explanations provided by [4] and have chosen
to use /dev/urandom. To summarise Hühn: /dev/urandom does not block; we only require
entropy at the beginning anyway (apart from occassional reseeds); only upon
first boot is there too little entropy; our cryptographic algorithms only
require computational security, not information-theoretic security and so
*truly* random generator (i.e. some quantum device) is not required.

Once we have a seed, we need to seed a generator. GMP offers an implementation
of the Mersenne Twister. I opt to use this. However, its Wiki does state that this
is not cryptographically secure. This is then a weakness in my implementation.
I plan to replace this with an crypto-secure implementation from a 3rd party
library, but at time of writing, I have not yet done this. An example of a practical
CS-PRNG is the Yarrow algorithm, which MacOS's /dev/random uses.
The libray libtomcrypt [5] contains an implementation of Yarrow. Its a modular and
portable and **open-source** libary, so I would use this. It also contains an
implementation of Fortuna, which its docs state is 'faster and more secure'.


## Sliding Window Exponentiation

I follow the pseudocode given in the slides. All modular exponentiations performed
by `stage1`, `stage2` etc use calls to the function `sliding_window_expm` which
I implemented. The implementation does not itself use `mpz_powm` or `mpz_pow`.
Precomputing the table T is done by setting `T[0]<-b mod N` and then
`T[i] <- T[i-1] * b^2 mod N`. I choose a max sliding window size of 4, else
precomputation would become far too large an overhead.


## Montgomery Multiplication

This implementation is incomplete and is thus not called by `stage1`, `stage2` etc.
The function

  `void mulm( mpz_t r, mpz_t x, mpz_t y, mpz_t N )`

performs (or attempts to perform) Montgomery multiplication via the following process.

  1. precompute the Montgomery params omega and rho squared
  2. calculate x_hat and y_hat  (x and y in Montgomery representation) using ZN-MontMul({x,y}_hat, rho^2 mod N)
  3. calculate r_hat=ZN-MontMul(x_hat,y_hat) (r=x*y in Montgomery representation)
  4. calculate r=ZN-MontMul(r_hat,1) (r_hat in standard base-b representation)

ZN-MontMul(r,x,y,N) is implemented by the function `Z_N_montmul`.
This follows the algorithm Z_N-MontMul given in the slides, also noting that division and mod by the
base (`mp_bits_per_limb`) is performed easily. Division by taking the least significant limb and mod by
right-shifting the limbs by 1 place (The function `shift_limbs_1` performs this).

The function `Z_N_montmul` seg faults on the line:

  `mpn_mul( tmp1, &y_i, 1, x_limbs, l_N );`

inside the loop body.

Furthermore, my implementation of Montgomery Multiplication is totally inefficient - I use 4 Montgomery
multiplications + precomputing Montgomery params to implement 1 single modular multiplication.
This is because we have to convert to and from Montgomery representation of integers. The solution to this
was presented in lecture but I have not had time to implement it.


## References


[1] Torvalds, Linus (2005-04-16). "Linux Kernel drivers/char/random.c comment documentation @ 1da177e4". Retrieved 2014-07-22.
    https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git/tree/drivers/char/random.c?id=refs/tags/v3.15.6#n52

[2] https://security.stackexchange.com/a/42955

[3] Linux man page entry for /dev/random and /dev/urandom.
    http://man7.org/linux/man-pages/man4/random.4.html

[4] Thomas Hühn. Myths about /dev/urandom. Retrieved 2018-02-16.
     https://www.2uo.de/myths-about-urandom/

[5] libtomcrypt. https://github.com/libtom/libtomcrypt