## Overview  

A simple 32-bit CPU emulator implementation.  
It supports basic arithmetic operations, stack manipulation, registers and I/O.

---

## Languages and Tools

- **C (C99) + POSIX**  
- **Make** (for compilation)  
- **Valgrind** (for memory correctness testing)  

---

## Project Structure

| File            | Description                                                                 |
|-----------------|-----------------------------------------------------------------------------|
| `cpu.h`         | Public interface of the emulator – structure and function declarations      |
| `cpu.c`         | Core CPU logic – execution cycle implementation                             |
| `main.c`        | Program entry point – argument parsing, loading input file                  |
| `compiler.c`    | Text (assembler) instructions into binary file (provided by school)         |
| `Makefile`      | Build automation using `gcc`                                                |
| `README.md`     | Project documentation                                                       |

---

## How to Run

1. **Compile the project**  
   ```bash
   cmake -S . -Bbuild
   cd build
   make
   ./cpu (run|trace) [stack_capacity(optional)] FILE
   ```
   
2. **Create a binary file**
<p align="center">Use compiler.c to compile from *.txt* or *.asm* to *.bin*</p>
<p align="center">
  <img src="https://i.imgur.com/jHFDeqa.png" width="40%" alt="instructions .txt"/>
  <img src="https://i.imgur.com/uVDds32.png" width="40%" alt="instructions .bin"/>
</p>
<p align="center">
<img src="https://i.imgur.com/7UmYoES.png", width="80%" alt="terminal"/>
</p>

3. **Run the program**
   ```bash
   ./cpu (run|trace) [stack_capacity(optional)] FILE
   ```
   
