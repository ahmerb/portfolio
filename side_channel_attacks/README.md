# Differential Side Channel and Fault Analysis Attacks on RSA and AES

An extremely intensive third-year coursework in which I implemented three different differential side-channel and one differential fault attack against RSAES-OAEP, RSA (Montgomery multiplication), AES-128 and XTS-AES block decyption. The side-channel attacks are against error messages, power consumption and time.

Implementing the attacks require a deep understanding of all of the following:
  - The underlying cryptography.
  - How BigNum libraries are implemented, including optimisation techniques like Montgomery multiplication.
  - The formal specifications for RSAES-OAEP, RSA (Montgomery multiplication), AES-128 and XTS-AES as defined by PKCS / IEEE etc.
  - Differential side-channel and fault analysis attacks. In particular, I implement sophisticated attacks from multiple papers.


## Folders

There are four directories, corresponding to each attack. Each directory contains are README with detailed explanation; copy of the relevant paper and specifications; attack implementation; a linux executable that simulates the attack target; a detailed discussion answering questions about the attack.
