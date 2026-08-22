# LSM Language Specification & Comprehensive Reference Manual

Welcome to the definitive engineering reference for **LSM** (Language for Systems & Machines).

LSM is a compiled, statically typed systems programming language engineered to provide direct, mechanical hardware sympathy without sacrificing modern language ergonomics, type safety, or expressive syntax.

---

## Table of Contents

1. [Top-Level Compilation Directives](https://www.google.com/search?q=%231-top-level-compilation-directives)
2. [Lexical Structure & Grammar Fundamentals](https://www.google.com/search?q=%232-lexical-structure--grammar-fundamentals)
3. [Type System & Memory Layouts](https://www.google.com/search?q=%233-type-system--memory-layouts)
4. [Control Flow & Pattern Matching](https://www.google.com/search?q=%234-control-flow--pattern-matching)
5. [Functions, Procedures & Methods](https://www.google.com/search?q=%235-functions-procedures--methods)
6. [Low-Level Intrinsics & Bare-Metal Subsystems](https://www.google.com/search?q=%236-low-level-intrinsics--bare-metal-subsystems)
7. [Hardware Register Mapping & Assembly Interop](https://www.google.com/search?q=%237-hardware-register-mapping--assembly-interop)
8. [Foreign Function Interface (FFI)](https://www.google.com/search?q=%238-foreign-function-interface-ffi)
9. [Concurrency & Task Scheduling](https://www.google.com/search?q=%239-concurrency--task-scheduling)
10. [Attributes & Meta-Programming Intrinsics](https://www.google.com/search?q=%2310-attributes--meta-programming-intrinsics)
11. [Complete Operator & Token Precedence Matrix](https://www.google.com/search?q=%2311-complete-operator--token-precedence-matrix)

---

## 1. Top-Level Compilation Directives

Directives configure the compilation target, runtime assumptions, and foreign module resolution at the file root.

### `fulldev`

The `fulldev` directive establishes a freestanding bare-metal compilation profile. It disables standard runtime wrappers, strips default OS runtime initializers, and targets flat memory segments directly.

```lsm
fulldev

```

### `import`

Imports external LSM compilation units, local modules, or foreign C header declarations.

```lsm
import "kernel/vga.lsm";
import "drivers/keyboard.lsm";
import "libc.h";

```

---

## 2. Lexical Structure & Grammar Fundamentals

### Identifiers & Literals

* **Identifiers:** Sequence of alphanumeric characters and underscores, beginning with `[a-zA-Z_]`.
* **Integer Literals:** Decimal (`42`, `1000`) or hexadecimal prefixed with `0x`/`0X` (`0xFF00`, `0x1000`).
* **Floating-Point Literals:** IEEE 754 double-precision numbers containing a decimal point (`3.14159`, `0.05`).
* **String Literals:** UTF-8 null-terminated sequences enclosed in double quotes (`"LSM OS Kernel"`).
* **Comments:**
* Single-line: `// comment`
* Multi-line: `/* block comment */`



### Keywords Reference

| Keyword | Category | Description |
| --- | --- | --- |
| `fct` | Declaration | Defines a value-returning function |
| `proc` | Declaration | Defines a procedure (void return) |
| `let` | Declaration | Declares a local variable or fixed-size stack buffer |
| `class` | OOP | Declares a polymorphic class |
| `extends` | OOP | Establishes single-inheritance between classes |
| `this` | OOP | Explicit self-reference within method scopes |
| `new` | OOP / Alloc | Instantiates records or classes |
| `interface` | Polymorphism | Declares abstract method contracts |
| `enum` | Types | Declares enumerated algebraic variants |
| `rec` | Types | Declares a flat, memory-mapped structure (struct) |
| `type` | Types | Defines an alias for an existing type |
| `array` | Types | Declares array buffers |
| `slice` | Types | Declares non-owning views over contiguous memory |
| `extern` | FFI | Declares dynamic library imports with C calling conventions |
| `if` / `else` | Branching | Conditional execution constructs |
| `while` / `for` | Loops | Iteration and conditional looping constructs |
| `in` / `to` | Ranges | Bound specifiers for loops and slices |
| `switch` / `case` / `default` | Branching | Multi-way jump table execution |
| `match` | Pattern Matching | Algebraic pattern deconstruction |
| `break` / `continue` | Flow Control | Loop termination and cycle skipping |
| `return` | Flow Control | Yields a value and exits the active call frame |
| `defer` | Scope Control | Schedules execution upon scope exit |
| `try` / `catch` / `throw` | Error Handling | Exception propagation and capture blocks |
| `go` / `chan` | Concurrency | Thread/fiber dispatch and typed message channels |
| `print` / `input` | Built-ins | Native runtime I/O expressions |
| `nil` / `true` / `false` | Literals | Primitive constants |
| `Ok` / `Err` / `Some` / `None` | Algebraic Types | Option/Result sum type constructors |
| `and` / `or` / `not` | Logical | Word-based logical operators |

---

## 3. Type System & Memory Layouts

LSM utilizes static type checking with local type inference and compile-time monomorphization.

### Variable Bindings

```lsm
let inferred_val = 100;
let explicit_int: Int = 0x200;
let ratio: Float = 1.618;
let label: String = "Kernel Entry";

```

### Records (`rec`) & Fixed Buffers

Records represent contiguous, unpadded (or custom-aligned) memory layouts suitable for hardware MMIO and binary protocols.

```lsm
#[align("16")]
rec TaskControlBlock {
    pid;
    stack_pointer;
    state;
    priority;
}

fct init_stack() -> void {
    let tcb = new TaskControlBlock();
    tcb.pid = 1;
    tcb.priority = 10;
}

```

### Fixed Stack Arrays

Arrays are declared with explicit element counts and types:

```lsm
let framebuffer: [Int; 1024];
framebuffer[0] = 0xFF00FF;

```

---

## 4. Control Flow & Pattern Matching

### Conditional Branching

```lsm
if status == 0 {
    print("Ready");
} else if status == 1 {
    print("Pending");
} else {
    print("Fault");
}

```

### Switch Statements

```lsm
switch (opcode) {
    case 0x01: {
        handle_read();
    }
    case 0x02: {
        handle_write();
    }
    default: {
        handle_unknown();
    }
}

```

### Pattern Matching (`match`)

Deconstructs Result (`Ok`/`Err`) and Option (`Some`/`None`) algebraic sum types:

```lsm
match probe_device(0x1F0) {
    Ok(dev_id) => {
        print("Device identified:");
        print(dev_id);
    }
    Err(code) => {
        print("Device fault:");
        print(code);
    }
}

```

---

## 5. Functions, Procedures & Methods

LSM cleanly separates value-returning functions (`fct`) from state-modifying procedures (`proc`).

### Syntax & Signatures

```lsm
// Returning function
fct compute_hash(seed: Int, salt: Int) -> Int {
    let hash = (seed ^ salt) * 0x5BD1E995;
    return hash;
}

// Side-effect procedure
proc flush_tlb() {
    unsafe {
        asm("mov %cr3, %rax; mov %rax, %cr3");
    }
}

```

### Generic Functions

Generics are monomorphized at compile time to guarantee zero runtime dispatch overhead:

```lsm
fct max_val<T>(a: T, b: T) -> T {
    if a > b {
        return a;
    }
    return b;
}

```

---

## 6. Low-Level Intrinsics & Bare-Metal Subsystems

All unmanaged operations, pointer dereferences, and port commands must reside within an `unsafe` block.

```lsm
unsafe {
    // Hardware operations permitted
}

```

### Intrinsics Reference Table

| Intrinsic | Category | Signature | Description |
| --- | --- | --- | --- |
| `volatile` | Compiler Barrier | `volatile(addr, [val])` | Forces direct, uncached memory access |
| `ptr` | Pointer Cast | `ptr<Type>(expr)` | Casts an integer to a strongly typed pointer |
| `setreg` | Registers | `setreg(reg, val)` | Writes a value into a CPU register |
| `getreg` | Registers | `getreg(reg)` | Reads the current value of a CPU register |
| `cpu_halt` / `halt` | CPU Control | `cpu_halt();` | Executes processor halt instruction (`HLT`) |
| `cpu_cli` / `cli` | Interrupts | `cpu_cli();` | Clears interrupt flag (disables IRQs) |
| `cpu_sti` / `sti` | Interrupts | `cpu_sti();` | Sets interrupt flag (enables IRQs) |
| `inb` / `outb` | I/O Ports | `inb(port)`, `outb(port, byte)` | Reads/writes an 8-bit port byte |
| `inw` / `outw` | I/O Ports | `inw(port)`, `outw(port, word)` | Reads/writes a 16-bit port word |
| `inl` / `outl` | I/O Ports | `inl(port)`, `outl(port, dword)` | Reads/writes a 32-bit port doubleword |
| `port_in` / `port_out` | I/O Ports | `port_in(p)`, `port_out(p, v)` | Generalized port I/O primitives |
| `mmio_read8..64` | Memory-Mapped I/O | `mmio_readN(addr)` | Reads 8/16/32/64 bits from physical address |
| `mmio_write8..64` | Memory-Mapped I/O | `mmio_writeN(addr, val)` | Writes 8/16/32/64 bits to physical address |
| `atomic_add` | Atomics | `atomic_add(addr, val)` | Atomically increments target memory |
| `atomic_sub` | Atomics | `atomic_sub(addr, val)` | Atomically decrements target memory |
| `atomic_cas` | Atomics | `atomic_cas(addr, new, exp)` | Atomic Compare-And-Swap |
| `atomic_load` | Atomics | `atomic_load(addr)` | Atomically loads memory value |
| `atomic_store` | Atomics | `atomic_store(addr, val)` | Atomically stores memory value |

---

## 7. Hardware Register Mapping & Assembly Interop

Direct register variables interface natively with x86_64 general-purpose registers:

| Register | Width | Semantic Role |
| --- | --- | --- |
| `rax` | 64-bit | Primary Accumulator / Return Register |
| `rbx` | 64-bit | Base Register |
| `rcx` | 64-bit | Counter / Loop Register |
| `rdx` | 64-bit | I/O Pointer / Data Register |
| `rsi` | 64-bit | Source Index |
| `rdi` | 64-bit | Destination Index |
| `rbp` | 64-bit | Frame Base Pointer |
| `rsp` | 64-bit | Stack Pointer |
| `r8` .. `r15` | 64-bit | General Purpose Extended Registers |

### Direct Register & ASM Example

```lsm
proc switch_context(stack_top: Int) {
    unsafe {
        setreg(rsp, stack_top);
        asm("ret");
    }
}

```

---

## 8. Foreign Function Interface (FFI)

LSM declares native linkage bindings with dynamic libraries (`.dll`, `.so`) using the `extern` keyword.

```lsm
fulldev

extern "kernel32.dll" fct GetStdHandle(nStdHandle: Int) -> Int;
extern "kernel32.dll" fct WriteConsoleA(
    hConsoleOutput: Int, 
    lpBuffer: String, 
    nNumberOfCharsToWrite: Int, 
    lpNumberOfCharsWritten: Int, 
    lpReserved: Int
) -> Int;

fct main() -> Int {
    let hStdOut = GetStdHandle(-11);
    let msg = "Native Win32 Subsystem via LSM FFI\n";
    WriteConsoleA(hStdOut, msg, 35, 0, 0);
    return 0;
}

```

---

## 9. Concurrency & Task Scheduling

LSM provides structured concurrency mechanisms for thread dispatching and synchronization.

* **Lightweight Spawning (`go`):** Dispatches a function asynchronously into the toolchain thread pool.
* **Channels (`chan`):** Typed communication pipelines providing synchronized data passing.
* **Resource Deferral (`defer`):** Guarantees cleanup upon exiting enclosing lexical scopes.

```lsm
fct worker_routine(c: chan) -> void {
    let data = 0xAA;
    c <- data; // Send to channel
}

fct main() -> Int {
    let comm_pipe = chan: Int;
    go worker_routine(comm_pipe);

    let received = <-comm_pipe; // Receive from channel
    return 0;
}

```

---

## 10. Attributes & Meta-Programming Intrinsics

Attributes modify symbol generation, compiler inlining policies, and memory alignment rules.

### Supported Attributes

```lsm
#[inline]          // Forces compiler inlining pass
#[naked]           // Emits raw function without standard prologue/epilogue
#[interrupt]       // Configures interrupt-frame handling (IRETQ alignment)
#[no_mangle]       // Retains exact symbol identifier in binary output
#[section(".text.boot")] // Emits function into specific ELF/Binary section
#[align("32")]     // Aligns record layout to specified byte boundary

```

### Compile-Time Intrinsics

* `@if`: Evaluates condition at compile-time and selectively parses AST blocks.
* `@offsetof(Record, Field)`: Evaluates field byte offset within memory records at compile time.

---

## 11. Complete Operator & Token Precedence Matrix

Evaluation precedence ordered from highest to lowest:

| Precedence Level | Operators | Associativity | Description |
| --- | --- | --- | --- |
| **1. Primary / Call** | `()`, `[]`, `.`, `?.`, `++`, `--` | Left-to-Right | Function calls, array index, member access, postfix ops |
| **2. Unary** | `-`, `!`, `~`, `&`, `*` | Right-to-Left | Negation, logical NOT, bitwise NOT, address-of, dereference |
| **3. Factor** | `*`, `/`, `%` | Left-to-Right | Multiplication, division, modulo |
| **4. Term** | `+`, `-` | Left-to-Right | Addition, subtraction |
| **5. Shift** | `<<`, `>>` | Left-to-Right | Bitwise left and right shifts |
| **6. Comparison** | `<`, `<=`, `>`, `>=` | Left-to-Right | Relational comparisons |
| **7. Equality** | `==`, `!=` | Left-to-Right | Value equality and inequality |
| **8. Bitwise AND** | `&` | Left-to-Right | Bitwise AND |
| **9. Bitwise XOR** | `^` | Left-to-Right | Bitwise XOR |
| **10. Bitwise OR** | `|` | Left-to-Right | Bitwise OR |
| **11. Logical AND** | `&&`, `and` | Left-to-Right | Short-circuit boolean AND |
| **12. Logical OR** | `||`, `or` | Left-to-Right | Short-circuit boolean OR |
| **13. Ternary** | `? :` | Right-to-Left | Conditional expression |
| **14. Range** | `..`, `..=` | Non-associative | Exclusive and inclusive range generation |
| **15. Channel** | `<-` | Left-to-Right | Channel send and receive |
| **16. Assignment** | `=`, `+=`, `-=`, `*=`, `/=` | Right-to-Left | Value assignment and compound modification |
