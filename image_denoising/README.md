### Image De-noising Using Iterative Conditional Modes, Gibbs Sampling, Mean Field Variational Bayes in The Ising Model

This is a third year coursework in a team of three. We used the Ising Model to represent a noisy image as generated by a latent clean image. In the Ising Model, we model each observed pixel `Y_ij` as generated by a latent `X_ij`. All the `X_ij` lie in a Markov Random Field where neighbouring nodes are connected. As the posterior distribution over the latents is computationally intractable to compute, we employ the methods above to recover it.

Some of the Python code is included. Our report is also included. I am responsible for most of the implementation for Iterative Conditional Modes and all of the Gibbs implementation. In the report, I wrote the theoretical explanation of the Ising Model and ICM in Q1 and also all of Q2, Q3 and Q4 on Gibbs. I made small contributions to other sections of the report too. The remainder was done by my talented fellow students Louis (@Lun3x) and Luke (@LukeStorry). As I'm not responsible for that code, it's not included. We achieved a First Class grade with a total mark of 90%.

**The brief for the coursework is in [coursework_brief.pdf](coursework_brief.pdf). Our report is in [report.pdf](report.pdf)**