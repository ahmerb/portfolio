### OpenMP & Serial Optimisations for a Dense Linear Algebra Program

[jacobi](./jacobi/)

A third-year individual project for the unit High Performance Computing. The optimisations are targeted for running on the Bristol BlueCrystal Phase 3 supercomputer. You can see the times achieved running on Blue Crystal in the two reports.

This project was split across two courseworks. In the first, we performed serial optimisations given a baseline implementation of the Jacobi Iterative Method. In the second, we used OpenMP to parallelise the program using the shared memeory paradigm.

Makefiles are included but you will need `icc` (Intel C/C++ Compiler).

Optimisations included false sharing, load balance, thread affinity, branch prediction, loop transformations and vectorisation. I achieved First Class marks.