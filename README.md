# System Programming for Systems on Chip Practical Works

This repository contains the implementations for the practical works for EPFL’s System Programming for Systems-on-Chip course (2025), completed in collaboration with Edwin Chen (@chenedwin54288)

## 1. Getting to Know the Virtual Prototype
- **Objective**: Set up the development environment, verify the toolchain with basic applications, and complete a first C programming task by implementing an unsigned integer to string conversion function and using it to print numbers in base-20.
- **Assignment Details**: [PDF: Practical Work 1 - Getting to Know the Virtual Prototype](./assignment_documents/pw1.pdf)


## 2. The Mandelbrot Set
- **Objective**: Transform a floating-point Mandelbrot implementation into a fixed-point version, then design a custom floating-point format to improve execution efficiency on an embedded processor without a standard FPU.
- **Assignment Details**: [PDF: Practical Work 2 - The Mandelbrot Set](./assignment_documents/pw2.pdf)

## 3. Profiling and Memory Distance
- **Objective**: Profile Mandelbrot execution using hardware performance counters, study the impact of CPU-memory distance, and explore low-level exception handling and system call mechanisms on OpenRISC.
- **Assignment Details**: [PDF: Practical Work 3 - Profiling and Memory Distance](./assignment_documents/pw3.pdf)

## 4. Caches
- **Objective**: Analyze structure memory layouts, understand cache behavior, and optimize data structures and algorithms for improved cache efficiency, including the interaction between I/O devices and caches.
- **Assignment Details**: [PDF: Practical Work 4 - Caches](./assignment_documents/pw4.pdf)

## 5. PIO, Interrupts, and Latencies
- **Objective**: Implement polling- and interrupt-driven I/O for buttons, dip-switches, and joystick inputs, then measure and analyze interrupt latency in an interactive Mandelbrot-based application.
- **Assignment Details**: [PDF: Practical Work 5 - PIO, Interrupts, and Latencies](./assignment_documents/pw5.pdf)

## 6. DMA and Compiler Options
- **Objective**: Evaluate the effect of compiler optimization levels on performance and code size, then use DMA and scratch-pad memory to accelerate Mandelbrot rendering and overlap communication with computation.
- **Assignment Details**: [PDF: Practical Work 6 - DMA and Compiler Options](./assignment_documents/pw6.pdf)

## 7. Coroutines and Multitasking
- **Objective**: Explore coroutine fundamentals and build a cooperative task manager on OpenRISC, first for a single-core system and then for a dual-core setup with synchronization primitives.
- **Assignment Details**: [PDF: Practical Work 7 - Coroutines and Multitasking](./assignment_documents/pw7.pdf)
