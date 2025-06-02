## Overview  

A simple 32-bit CPU emulator implementation.  
It supports basic arithmetic operations, stack manipulation, registers and I/O.

---

## Languages and Tools

- **C (C99) + POSIX**  
- **CMake** (for compilation)  
- **Valgrind** (for memory correctness testing)  

---

## Project Structure

| File            | Description                                                                 |
|-----------------|-----------------------------------------------------------------------------|
| `cpu.h`         | Public interface of the emulator – structure and function declarations      |
| `cpu.c`         | Core CPU logic – execution cycle implementation                             |
| `main.c`        | Program entry point – argument parsing, loading input file                  |
| `compiler.c`    | Text (assembler) instructions into binary file (provided by school)         |
| `CMakeLists`    | Build automation using `cmake`                                                |

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
<p align="center">Use compiler.c to compile from <code>.txt</code> or <code>.asm</code> to <code>.bin</code></p>
<p align="center">
  <img src="https://i.imgur.com/jHFDeqa.png" width="40%" alt="instructions .txt"/>
  <img src="https://i.imgur.com/uVDds32.png" width="40%" alt="instructions .bin"/>
</p>
<p align="center">
<img src="https://i.imgur.com/7UmYoES.png", width="80%" alt="terminal"/>
</p>

3. **Run the program**
   ```bash
   ./cpu (run|trace) [stack_capacity(optional)] FILE.bin
   ```
<p align="center"><code>run</code> -> evaluates whole program | <code>trace</code> -> press enter for next step</p>
<p align="center">
  <img src="https://i.imgur.com/pWOcRXA.png" width="40%" alt="./cpu run"/>
  <img src="https://i.imgur.com/13Fru5M.png" width="40%" alt="./cpu trace"/>
</p>

## 🧾 Instructions and CPU Behavior

The emulator executes instructions only when its status code is `CPU_OK`. If any error occurs during execution, the CPU status is updated accordingly, and the instruction is **not** executed. The instruction pointer remains unchanged in such cases.

### Status Codes (`enum cpu_status`)
| Code                          | Meaning                                     |
|-------------------------------|---------------------------------------------|
| `CPU_OK`                      | Everything is fine                          |
| `CPU_HALTED`                  | Program finished execution (`halt`)         |
| `CPU_ILLEGAL_INSTRUCTION`     | Unknown instruction encountered             |
| `CPU_ILLEGAL_OPERAND`         | Invalid register or operand used            |
| `CPU_INVALID_ADDRESS`         | Invalid memory access attempted             |
| `CPU_INVALID_STACK_OPERATION` | Stack overflow/underflow or invalid index   |
| `CPU_DIV_BY_ZERO`             | Division by zero attempted                  |
| `CPU_IO_ERROR`                | Invalid or malformed input encountered      |

---

### Instruction Set

- `nop` – Do nothing.
- `halt` – Halt program execution and set CPU status to `CPU_HALTED`.
- `add REG` – Add the value of register `REG` to register `A`.
- `sub REG` – Subtract the value of register `REG` from register `A`.
- `mul REG` – Multiply register `A` by the value of register `REG`.
- `div REG` – Divide register `A` by the value of register `REG`.  
  If `REG` contains `0`, do not execute and set status to `CPU_DIV_BY_ZERO`.
- `inc REG` – Increment the value of register `REG`.
- `dec REG` – Decrement the value of register `REG`.
- `loop INDEX` – If register `C` is not zero, jump to instruction at index `INDEX`.
- `movr REG NUM` – Store constant number `NUM` into register `REG`.
- `load REG NUM` – Load value from stack at offset `D + NUM` (from top) into register `REG`.  
  If index is invalid, set status to `CPU_INVALID_STACK_OPERATION`.
- `store REG NUM` – Store value from register `REG` to stack at offset `D + NUM`.  
  If index is invalid, set status to `CPU_INVALID_STACK_OPERATION`.
- `in REG` – Read a decimal number from input into register `REG`.  
  On malformed input, set status to `CPU_IO_ERROR`.  
  On EOF, set `C = 0` and `REG = -1`.
- `get REG` – Read a single byte from input and store it in `REG`.  
  On EOF, same behavior as `in`.
- `out REG` – Output the value of register `REG` as a decimal number.
- `put REG` – If `REG` is between 0–255, output it as an ASCII character.  
  Otherwise, set status to `CPU_ILLEGAL_OPERAND`.
- `swap REG1 REG2` – Swap values between `REG1` and `REG2`.
- `push REG` – Push value from register `REG` to the stack.  
  On stack overflow, set status to `CPU_INVALID_STACK_OPERATION`.
- `pop REG` – Pop value from the stack into register `REG`.  
  On stack underflow, set status to `CPU_INVALID_STACK_OPERATION`.
