# LSM Infrastructure & Toolchain

The unified compiler, SSA intermediate optimizer, multi-target code generator, and runtime toolchain for **LSM** (Language for Systems & Machines)—a statically typed, high-performance systems programming language engineered to bridge zero-cost bare-metal hardware control with modern, high-level developer ergonomics.

---

## Technical Overview

LSM is architected to eliminate the traditional tradeoff between high-level expressive abstraction and low-level mechanical sympathy. It delivers first-class primitives for bare-metal operating system development, freestanding kernel programming, and custom hypervisors, while simultaneously providing a modern, expression-oriented syntax equipped with algebraic data types, pattern matching, monomorphized generics, and memory-safe concurrency pipelines.

The toolchain contains an entirely self-reliant compilation pipeline: an integrated lexer/parser frontend, a Static Single Assignment (SSA) optimization middle-end, native multi-architecture encoders (x86_64, ARM64, and 16/32/64-bit flat binaries), an embedded ELF linker, an interactive multi-threaded JIT runtime engine, and an embedded Language Server Protocol (LSP) server with zero external dependencies.

---

## Core Engineering Paradigms

* **Modern High-Level Abstractions:**
* Clean, expression-driven syntax featuring type inference, explicit static type annotations, and first-class algebraic return types (`Ok`, `Err`, `Some`, `None`).
* Exhaustive pattern matching (`match`) and expressive multi-condition switching (`switch`).
* Parametric polymorphism through compile-time monomorphized generics on functions, records, and classes.
* Native concurrency primitives including non-blocking thread spawning (`go`) and typed communication channels (`chan`).
* Modern resource cleanup semantics via deferred execution (`defer`).


* **Uncompromising Hardware & Systems Control:**
* Explicit freestanding kernel mode (`fulldev`) providing predictable execution with absolute zero-runtime overhead.
* Scoped `unsafe` execution blocks for verified, direct physical memory manipulation.
* Direct port-mapped I/O (`inb`, `outb`, `inw`, `outw`, `inl`, `outl`) and memory-mapped I/O operations (`mmio_read8..64`, `mmio_write8..64`).
* Architecture-level CPU management (`cpu_cli`, `cpu_sti`, `cpu_halt`) and atomic operations (`atomic_add`, `atomic_sub`, `atomic_cas`, `atomic_load`, `atomic_store`).
* First-class register bindings (`setreg`, `getreg`), explicit record alignment attributes (`#[align("N")]`), and zero-overhead inline assembly (`asm`).


* **Self-Contained Multi-Target Compiler Pipeline:**
* **Static Single Assignment (SSA) Middle-End:** Dead Code Elimination (DCE), Loop-Invariant Code Motion (LICM), Constant Propagation, and Register Allocation passes.
* **Unified Native Backends:** Multi-architecture instruction encoders covering 64-bit hosted x86_64, AArch64/ARM64, and dedicated 16-bit real-mode, 32-bit protected-mode, and 64-bit long-mode bare-metal flat binary emitters.
* **Dual Execution Modes:** Instant in-memory multi-threaded execution via the internal JIT engine or standalone binary generation via the built-in ELF/PE object linkers.



---

## Compiler Pipeline Architecture

```text
               +----------------------------------------+
               |         LSM Source Code (.lsm)         |
               +----------------------------------------+
                                   |
                                   v
             [ Frontend: Lexical & Syntax Analysis ]
                                   |
        +--------------------------+--------------------------+
        |                                                     |
        v                                                     v
Concrete Syntax Tree (CST)                         Abstract Syntax Tree (AST)
        |                                                     |
        v                                                     v
[ LSP Diagnostics / Server ]                       [ Monomorphization Engine ]
                                                              |
                                                              v
                                                   [ Static Type Checking ]
                                                   [ Layout Reflection    ]
                                                              |
                                                              v
                                                   [ Scope Lowering Pass  ]
                                                              |
                                                              v
                                                   [ SSA IR Construction  ]
                                                              |
                                                              v
                                                   [ Middle-End Optimizer ]
                                                   (DCE, LICM, ConstProp)
                                                              |
                                                              v
                                                   [ Register Allocation  ]
                                                              |
                                                              v
                                                   [ Machine Code Encoders]
                                                   - x86_64 Hosted / JIT
                                                   - ARM64 Native
                                                   - Bare-Metal x86 (16/32/64)
                                                              |
                                 +----------------------------+----------------------------+
                                 |                                                         |
                                 v                                                         v
                     [ Embedded JIT Engine ]                                   [ Native Linker / Builder ]
                                 |                                                         |
                                 v                                                         v
                     Interactive Process JIT                               Native ELF / Flat Boot Binary

```

---

## Language Specifications

### 1. High-Level Ergonomics & Pattern Matching


### 2. Bare-Metal Driver Development & Low-Level Control


### 3. Dynamic Native Interoperability


## Toolchain Installation & Source Build

### Prerequisites

* Standard C++17 compliant toolchain (`g++` >= 9.0, `clang++` >= 10.0, or MSVC 2019+)
* Standard Make or direct CLI invocation

### Building via POSIX Shell (Linux / macOS)

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

### Building via PowerShell (Windows)

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

---

## CLI Reference & Usage

| Command | Target | Description |
| --- | --- | --- |
| `lsm run <file.lsm> [-O3]` | Host Memory | Instant compilation and multi-threaded JIT execution |
| `lsm build <file.lsm> -o <bin> [-O3]` | Native Executable | Static binary compilation via built-in ELF/Object builders |
| `lsm build --baremetal <file.lsm> -o <bin>` | Flat Binary | Generates freestanding machine code for OS kernels (LSM 7 Engine) |
| `lsm build --baremetal --modern <file.lsm>` | Flat Binary | Generates modern long-mode kernel binaries (LSM 8 Engine) |
| `lsm build --baremetal --bootable <file.lsm>` | Bootable Disk | Generates bootable disk image media (.img) |
| `lsm --lsp` | Daemon | Starts the JSON-RPC Language Server Protocol daemon |

---

## Showcase: Production Deployments

* **Oshi OS:** An advanced 64-bit graphical operating system featuring high-resolution linear framebuffer rendering ($1920 \times 1080$), a multi-tasking kernel, composited window manager, terminal emulator, memory-mapped storage drivers, and native toolkits—written 100% in LSM without external C runtimes.

---

## License

The LSM Toolchain infrastructure and all associated components are licensed under the **MIT License**.
