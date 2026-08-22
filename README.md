
# LSM Toolchain

The official compiler, assembler, and toolchain infrastructure for **LSM**—a statically typed, high-performance systems programming language designed for bare-metal kernel development, operating systems, JIT execution, and systems-level tooling.

---

## Overview

LSM combines readable, modern syntax with direct low-level hardware control. The toolchain provides modular frontend parsing, SSA intermediate optimization passes, native code generators (x86_64, ARM64, Bare-Metal targets), an embedded ELF linker, an interactive JIT engine, and Language Server Protocol (LSP) tooling without external runtime dependencies.

---

## Key Features

* **Bare-Metal & OS Ready:**
  * Direct memory access through dedicated `unsafe` blocks and memory-mapped I/O (`mmio_read`, `mmio_write`).
  * Freestanding execution mode (`fulldev` directive) for zero-runtime kernels.
  * Direct hardware register and port manipulation support.
* **Multi-Target Backend Support:**
  * **x86_64 & ARM64:** Native instruction encoding passes.
  * **Bare-Metal Encoders:** Dedicated 16-bit, 32-bit, and 64-bit flat binary encoders.
  * **ELF Linker:** Built-in `ELFBuilder` for binary object generation.
* **Middle-End & Runtime Infrastructure:**
  * Static Single Assignment (`SSABuilder`) intermediate representation.
  * JIT execution engine (`JITEngine`) for dynamic compilation.
  * Integrated Language Server (`LanguageServer`) for editor tooling and autocompletion.

---

## Toolchain Architecture

```text
LSM Source Code (.lsm)
       │
       ▼
   Lexer & Parser  ───►  Abstract Syntax Tree (AST)
       │
       ▼
 Semantic Analysis ───►  Type Checking & Scope Resolution
       │
       ▼
 Intermediate Pass ───►  SSA IR Builder & Optimization
       │
       ▼
  Code Generators  ───►  x86_64 / ARM64 / Bare-Metal Encoders / JIT Engine
       │
       ▼
   ELF Linker      ───►  Native Executables / Flat Binaries (.bin / .exe)

---

## Building from Source

### Prerequisites

* C++17 compliant compiler (`g++` >= 9.0 or `clang++` >= 10.0 / MSVC)
* Pre-built `lsm.exe` binary is included, but you can build the compiler directly using the CLI command below.

### Direct Build Command (PowerShell / Windows CLI)

```powershell
g++ -std=c++17 -O2 -I. -Isrc -Isrc/frontend/parser -Isrc/frontend/parser/modules `
    main.cpp `
    src/frontend/lexer/Lexer.cpp `
    src/frontend/parser/Parser.cpp `
    src/backend/x86_64/X86_64Encoder.cpp `
    src/backend/arm64/ARM64Encoder.cpp `
    src/backend/baremetal/BareMetalX86Encoder.cpp `
    src/backend/baremetal/BareMetalX86_32Encoder.cpp `
    src/backend/baremetal/BareMetalX86ModernEncoder.cpp `
    src/backend/linker/ELFBuilder.cpp `
    src/backend/jit/JITEngine.cpp `
    src/middleend/ssa/SSABuilder.cpp `
    src/runtime/sched/Scheduler.cpp `
    tools/lsp/LanguageServer.cpp `
    -o lsm.exe

```

### Direct Build Command (Linux / Bash)

```bash
g++ -std=c++17 -O2 -I. -Isrc -Isrc/frontend/parser -Isrc/frontend/parser/modules \
    main.cpp \
    src/frontend/lexer/Lexer.cpp \
    src/frontend/parser/Parser.cpp \
    src/backend/x86_64/X86_64Encoder.cpp \
    src/backend/arm64/ARM64Encoder.cpp \
    src/backend/baremetal/BareMetalX86Encoder.cpp \
    src/backend/baremetal/BareMetalX86_32Encoder.cpp \
    src/backend/baremetal/BareMetalX86ModernEncoder.cpp \
    src/backend/linker/ELFBuilder.cpp \
    src/backend/jit/JITEngine.cpp \
    src/middleend/ssa/SSABuilder.cpp \
    src/runtime/sched/Scheduler.cpp \
    tools/lsp/LanguageServer.cpp \
    -o lsm

```

---

## Usage Guide

### 1. Compiling to Flat Binary (Bare-Metal)

```bash
./lsm compile input.lsm -o output.bin

```

### 2. Compiling with Module Includes

```bash
./lsm compile -I ./kernel -I ./apps main.lsm -o main.bin

```

---

## Syntax Overview

### Functions & Control Flow

```lsm
fct calc_sum(a: Int, b: Int) -> Int {
    let result = a + b;
    return result;
}

```

### Unsafe Hardware & Memory Operations

```lsm
fct write_vga(addr: Int, val: Int) -> void {
    unsafe {
        mmio_write(addr, val);
        let readback = mmio_read(addr);
    }
}

```

---

## Showcase: Powered by LSM

* **[Oshi OS](https://www.google.com/search?q=https://github.com/laaasoomaa279):** A 64-bit bare-metal operating system featuring high-resolution graphics ($1920 \times 1080$), desktop launcher, interactive shell, text editor, fixed-point calculator, and games—built 100% in LSM.

---

## License

This project is licensed under the MIT License.

```

```
