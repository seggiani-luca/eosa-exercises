# EOSA scratch-os

This repo contains exercises from my Embedded Operating Systems and Architectures course.
The target OSes are compiled for ARM Cortex-M3 using the [arm-none-eabi](https://learn.arm.com/install-guides/gcc/arm-gnu/) toolchain.

## sratch-os

These are official course exercises I'm uploading mainly to:
- Keep track of them across machines;
- Share solutions with colleagues.

They are under the MIT licence, I didn't write them, and I don't take any responsibility for the solutions (though they likely worked on my machine). 

## own-os

`own-os` is a replica of the official course OS I'm working on in my spare time.
I try to reimplement what's in the official material, expanding on topics i find interesting / I think I need more practice on.

Features for now (updated lazily are):
- Boots into C code from ARM assembly;
- Implements a tiny subset of libc I/O via QEMU semihosting (`printf()` and `fgets()` available, plus some helpers such as `atoi()`).
