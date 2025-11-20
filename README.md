# Building a Memory Allocator in C

## Step 1: Understanding the Problem.
When we call malloc(100) we need:
- A way to get memory from the OS.
- A way to track which memory is free vc. used.
- A way to give memory back `free()` will do this.

## Strategy:
I will use the `sbrk()` to get memory from the OS, then manage it myself with a linked list of blocks.

## Step 2: Alignment and Align Macro
- To start with most CPU (especially 64-bit ones) work fastest when data starts at an address that is a multiple of 8 bytes.
Misaligned access might:
1. Slow down the CPU.
2. Cause bus errors on strict architectures.
3. Break alignment of pointers stored in the block.

We'll have to enforce that every user allocation starts at an 8-byte boundary like so:

`#define ALIGNMENT 8
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))`

This formula:

Adds 7 (ALIGNMENT - 1) to force a round-up.

Clears the last 3 bits using bitwise AND with a mask.
Example: ~(8 - 1) → ~7 → ...11111000.

So ALIGN(x) always becomes a multiple of 8.
