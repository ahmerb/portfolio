### Simple Kernel for ARM Cortex-A8

I started with an extremely primitive kernel consisting of not much more than a simple interrupt table that called an empty assembly method which called a C function upon a timer or reset interrupt. I then added simple process management, pcb tables, system calls including fork, exec and kill, command-line arguments, a priority based scheduler and interprocess communication via pipes.

It is written mostly in C and some ARM assembly. The main stuff is in the file [hilevel.c](./kernel/hilevel.c).

The full kernel is included, but wrappers around IO hardware devices such as the timer (simulated with ARM's QEMU) were not written by me and so I've removed them.

The kernel source is in the folder [kernel](./kernel/kernel/).

There are some example user processes to be run in user space are also included in the folder [user](./kernel/user). Two user program, called `philosopher` and `waiter` are included. These demonstrate interprocess communication by simulating the [Dining Philosophers Problem](https://en.wikipedia.org/wiki/Dining_philosophers_problem). A console program is also provided, which is executed by default on a reset.

I also wrote a C library that uses inline asm to make traps. Its used by programs in user space. Its in [libc](./kernel/libc.c). 