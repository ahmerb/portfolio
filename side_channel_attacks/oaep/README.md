# A Chosen Ciphertext Attack on RSA Optimal Asymmetric Encryption Padding (OAEP) as Standardized in PKCS #1 v2.0

I implemented the attack given in the [paper](Manger-RSAES-OAEP_Attack.pdf) by Manger [1].
Implementation is in Python 2.

After recovering the encoded message, OAEP-Decoding is performed using MGF1 and SHA-1 to recover
the secret message.

Execute with

`bash$ python attack.py ./21452.D 21452.conf`

The file [21452.conf](21452.conf) contains an example input for RSA, for which you can recover the secret message. The input is in the format:

 - `N` - RSA modulus (as hex integer string)
 - `e` - RSA public exponent (as hex integer string)
 - `l` - RSAES-OAEP label (as octet string)
 - `c` - RSAES-OAEP ciphertext (as octet string)

 The file [21452.D](21452.D) contains a linux executable which simulates running RSAES-OAEP encryption. It outputs error codes, which are exploited as a side channel. In particular, it differentiates between two different errors. These are discussed in the RSA-OAEP specification, as published by RSA Labs / PKCS.

 The file [attack.py](attack.py) contains the actual attack implementation.

 In the file [discussion.txt](discussion.txt), I give detailed answers to the following questions.

  - Consider the parameters N = 3551, e = 5, d = 1373, and c = 888, all represented in decimal.  Use them to explain, in your own words, the principle of this attack (e.g., how and why it works).
  - To prevent the attack, the vendor suggests altering the software: the idea is that no matter what error occurs, the same error code will be produced. Explain whether (and why, or why not) this change alone is sufficient.
  - This attack is based on the fact that decrypting a ciphertext of the form c = (f^e)\*c' mod N produces a plaintext of the form f\*m' mod N. Show why this fact is true.
  - Imagine you are a security consultant, employed by the device vendor. The vendor has heard that exponent and message blinding can be used to prevent some side-channel attacks on RSA. Explain whether these techniques would prevent this attack, and why.

## References

[1] James Manger. 2001. A Chosen Ciphertext Attack on RSA Optimal Asymmetric Encryption Padding (OAEP) as Standardized in PKCS #1 v2.0. In Proceedings of the 21st Annual International Cryptology Conference on Advances in Cryptology (CRYPTO '01), Joe Kilian (Ed.). Springer-Verlag, London, UK, UK, 230-238.
