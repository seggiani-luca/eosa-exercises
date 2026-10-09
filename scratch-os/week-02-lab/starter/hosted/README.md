# Hosted execution environment

Exercise **A3** is completed in `swap.c` in this directory.

The program built here runs on the same simulated Cortex-M3 as the
firmware image, under the same QEMU command, compiled from the same
instruction set. It differs in one respect: it is linked as a **hosted**
program, so the C standard library is present and `printf` can reach
your terminal.

`swap.c` is supplied and already builds and runs. Three pieces are
marked `TODO`. Run it before changing anything:

```sh
make run
```

The `Makefile` is provided. Constructing a build is exercise A2's
subject, one directory up; this build is scaffolding for A3.
