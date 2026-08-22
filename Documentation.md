# LSM Language Specification & Reference Manual

Complete technical documentation for the **LSM** programming language, covering syntax constructs, type systems, bare-metal intrinsics, low-level hardware registers, and lexer token specifications.

---

## 1. Top-Level Directives

### `fulldev`
Instructs the compiler to generate a freestanding bare-metal binary with zero runtime assumptions, no startup wrappers, and direct memory layout control.

```lsm
fulldev

```

### `import`

Imports another LSM source module or external C/system header.

```lsm
import "kernel/vga.lsm";
import "drivers/keyboard.lsm";
import "libc.h";

```

---

## 2. Keywords & Language Constructs

### Core Keywords Table

| Keyword | Category | Description |
| --- | --- | --- |
| `fct` | Declaration | Declares a standard function |
| `proc` | Declaration | Declares a procedure (function without a mandatory return value) |
| `let` | Declaration | Declares a variable or fixed-size array |
| `class` | OOP | Declares an object-oriented class structure |
| `extends` | OOP | Specifies class inheritance |
| `this` | OOP | Refers to the current class instance |
| `new` | OOP | Instantiates a class instance |
| `interface` | Polymorphism | Declares an interface contract |
| `enum` | Types | Declares an enumeration type with variants |
| `rec` | Types | Declares a low-level memory record / struct |
| `type` | Types | Creates a type alias |
| `array` | Types | Declares dynamic or sized array types |
| `slice` | Types | Declares an array slice view |
| `extern` | Foreign Interface | Declares external foreign library functions |
| `if` | Control Flow | Conditional branch execution |
| `else` | Control Flow | Fallback conditional branch |
| `while` | Control Flow | Standard conditional loop construct |
| `for` | Control Flow | Iterator / range-based loop construct |
| `in` | Control Flow | Range / collection member specifier in loops |
| `to` | Control Flow | Upper bound specifier in range constructs |
| `switch` | Control Flow | Multi-way branching statement |
| `case` | Control Flow | Individual case branch in switch construct |
| `default` | Control Flow | Fallback case branch in switch construct |
| `match` | Pattern Matching | Pattern matching evaluation |
| `break` | Control Flow | Terminates execution of the current loop or block |
| `continue` | Control Flow | Skips to the next iteration of the current loop |
| `return` | Control Flow | Exits the current function with an optional return value |
| `go` | Concurrency | Spawns a concurrent coroutine / lightweight task |
| `chan` | Concurrency | Declares a communication channel |
| `defer` | Control Flow | Defers execution of a call until the enclosing scope exits |
| `try` | Error Handling | Begins an exception handling block |
| `catch` | Error Handling | Handles errors caught from a `try` block |
| `throw` | Error Handling | Throws an error object |
| `print` | Built-in | Built-in output expression |
| `input` | Built-in | Built-in input expression |
| `nil` | Literals | Null/nil reference constant |
| `true` | Literals | Boolean true literal |
| `false` | Literals | Boolean false literal |
| `Ok` | Result Type | Pattern/constructor for successful result values |
| `Err` | Result Type | Pattern/constructor for error result values |
| `Some` | Option Type | Pattern/constructor for present optional values |
| `None` | Option Type | Pattern/constructor for absent optional values |
| `and` | Logical | Logical AND operator |
| `or` | Logical | Logical OR operator |
| `not` | Logical | Logical NOT operator |

---

## 3. Low-Level, Hardware & Bare-Metal Intrinsics

All low-level memory manipulations, register accesses, and port operations must be enclosed inside an `unsafe` block.

| Keyword / Intrinsic | Category | Syntax / Parameters | Description |
| --- | --- | --- | --- |
| `unsafe` | Safety Boundary | `unsafe { ... }` | Encapsulates unmanaged memory and hardware calls |
| `asm` | Inline Assembly | `asm("instruction")` | Emits raw machine instructions inline |
| `volatile` | Compiler Barrier | `volatile(addr)` | Forces direct uncached memory load/store |
| `ptr` | Pointer Cast | `ptr<Type>(expr)` | Performs raw memory pointer casting |
| `setreg` | Register Control | `setreg(reg, value)` | Sets the value of a hardware CPU register |
| `getreg` | Register Control | `getreg(reg)` | Retrieves the current value of a CPU register |
| `cpu_halt` / `halt` | CPU State | `cpu_halt();` | Suspends CPU execution until the next interrupt |
| `cpu_cli` / `cli` | Interrupts | `cpu_cli();` | Clears Interrupt Flag (disables interrupts) |
| `cpu_sti` / `sti` | Interrupts | `cpu_sti();` | Sets Interrupt Flag (enables interrupts) |
| `port_in` | I/O Ports | `port_in(port)` | Reads a raw value from an I/O port address |
| `port_out` | I/O Ports | `port_out(port, val)` | Writes a raw value to an I/O port address |
| `inb` | I/O Ports | `inb(port)` | Reads an 8-bit byte from an I/O port |
| `outb` | I/O Ports | `outb(port, byte)` | Writes an 8-bit byte to an I/O port |
| `inw` | I/O Ports | `inw(port)` | Reads a 16-bit word from an I/O port |
| `outw` | I/O Ports | `outw(port, word)` | Writes a 16-bit word to an I/O port |
| `inl` | I/O Ports | `inl(port)` | Reads a 32-bit dword from an I/O port |
| `outl` | I/O Ports | `outl(port, dword)` | Writes a 32-bit dword to an I/O port |
| `mmio_read` | Memory-Mapped I/O | `mmio_read(addr)` | Reads an untyped word from physical memory |
| `mmio_write` | Memory-Mapped I/O | `mmio_write(addr, val)` | Writes an untyped word to physical memory |
| `mmio_read8` | Memory-Mapped I/O | `mmio_read8(addr)` | Reads an 8-bit byte from physical memory |
| `mmio_read16` | Memory-Mapped I/O | `mmio_read16(addr)` | Reads a 16-bit word from physical memory |
| `mmio_read32` | Memory-Mapped I/O | `mmio_read32(addr)` | Reads a 32-bit dword from physical memory |
| `mmio_read64` | Memory-Mapped I/O | `mmio_read64(addr)` | Reads a 64-bit qword from physical memory |
| `mmio_write8` | Memory-Mapped I/O | `mmio_write8(addr, val)` | Writes an 8-bit byte to physical memory |
| `mmio_write16` | Memory-Mapped I/O | `mmio_write16(addr, val)` | Writes a 16-bit word to physical memory |
| `mmio_write32` | Memory-Mapped I/O | `mmio_write32(addr, val)` | Writes a 32-bit dword to physical memory |
| `mmio_write64` | Memory-Mapped I/O | `mmio_write64(addr, val)` | Writes a 64-bit qword to physical memory |
| `atomic_add` | Atomics | `atomic_add(addr, val)` | Atomically adds a value to memory address |
| `atomic_sub` | Atomics | `atomic_sub(addr, val)` | Atomically subtracts a value from memory address |
| `atomic_cas` | Atomics | `atomic_cas(addr, new, exp)` | Atomic Compare-And-Swap operation |
| `atomic_load` | Atomics | `atomic_load(addr)` | Atomically loads a value from memory address |
| `atomic_store` | Atomics | `atomic_store(addr, val)` | Atomically stores a value to memory address |
| `disable_interrupts` | CPU State | `disable_interrupts();` | Disables maskable hardware interrupts |
| `enable_interrupts` | CPU State | `enable_interrupts();` | Enables maskable hardware interrupts |

---

## 4. Hardware Registers

The compiler recognizes native register keywords for `setreg`, `getreg`, and hardware variable bindings:

| Register Keyword | Architecture | Register Bit-Width | Description |
| --- | --- | --- | --- |
| `rax` | x86_64 | 64-bit | Accumulator Register |
| `rbx` | x86_64 | 64-bit | Base Register |
| `rcx` | x86_64 | 64-bit | Counter Register |
| `rdx` | x86_64 | 64-bit | Data Register |
| `rsi` | x86_64 | 64-bit | Source Index Register |
| `rdi` | x86_64 | 64-bit | Destination Index Register |
| `rbp` | x86_64 | 64-bit | Base Pointer Register |
| `rsp` | x86_64 | 64-bit | Stack Pointer Register |
| `r8` | x86_64 | 64-bit | General Purpose Register 8 |
| `r9` | x86_64 | 64-bit | General Purpose Register 9 |
| `r10` | x86_64 | 64-bit | General Purpose Register 10 |
| `r11` | x86_64 | 64-bit | General Purpose Register 11 |
| `r12` | x86_64 | 64-bit | General Purpose Register 12 |
| `r13` | x86_64 | 64-bit | General Purpose Register 13 |
| `r14` | x86_64 | 64-bit | General Purpose Register 14 |
| `r15` | x86_64 | 64-bit | General Purpose Register 15 |

---

## 5. Operators & Punctuators

### Arithmetic & Assignment Operators

| Symbol | Name | Description |
| --- | --- | --- |
| `+` | Addition | Adds two numeric operands |
| `+=` | Add-Assign | Adds operand to variable and stores result |
| `++` | Postfix Increment | Increments variable by 1 |
| `-` | Subtraction | Subtracts right operand or negates expression |
| `-=` | Sub-Assign | Subtracts operand from variable and stores result |
| `--` | Postfix Decrement | Decrements variable by 1 |
| `*` | Multiplication | Multiplies two numeric operands |
| `*=` | Mul-Assign | Multiplies variable by operand and stores result |
| `/` | Division | Divides left operand by right operand |
| `/=` | Div-Assign | Divides variable by operand and stores result |
| `%` | Modulo | Computes remainder of integer division |
| `=` | Assignment | Assigns value to variable or memory location |

### Comparison & Logical Operators

| Symbol | Name | Description |
| --- | --- | --- |
| `==` | Equality | Evaluates whether two values are equal |
| `!=` | Inequality | Evaluates whether two values are not equal |
| `<` | Less Than | Evaluates whether left operand is strictly smaller |
| `<=` | Less Than or Equal | Evaluates whether left operand is smaller or equal |
| `>` | Greater Than | Evaluates whether left operand is strictly larger |
| `>=` | Greater Than or Equal | Evaluates whether left operand is larger or equal |
| `&&` | Logical AND | Logical short-circuit AND |
| `||` | Logical OR | Logical short-circuit OR |
| `!` | Logical NOT | Inverts boolean truth value |

### Bitwise Operators

| Symbol | Name | Description |
| --- | --- | --- |
| `&` | Bitwise AND / Address | Computes bitwise AND or retrieves pointer address |
| `|` | Bitwise OR | Computes bitwise OR |
| `^` | Bitwise XOR | Computes bitwise Exclusive OR |
| `~` | Bitwise NOT | Computes bitwise binary NOT complement |
| `<<` | Shift Left | Performs bitwise left shift |
| `>>` | Shift Right | Performs bitwise right shift |

### Delimiters & Structural Symbols

| Symbol | Name | Description |
| --- | --- | --- |
| `(` | Left Parenthesis | Opens expression grouping or parameter list |
| `)` | Right Parenthesis | Closes expression grouping or parameter list |
| `{` | Left Brace | Opens a block scope |
| `}` | Right Brace | Closes a block scope |
| `[` | Left Bracket | Opens index access, attributes, or array type |
| `]` | Right Bracket | Closes index access, attributes, or array type |
| `,` | Comma | Separates list elements, parameters, and arguments |
| `.` | Dot | Accesses object methods and record fields |
| `?.` | Safe Navigation | Safe member access operator |
| `:` | Colon | Type annotation separator |
| `;` | Semicolon | Statement terminator |
| `?` | Question Mark | Ternary conditional operator condition separator |
| `->` | Thin Arrow | Return type specifier in function signatures |
| `=>` | Fat Arrow | Lambda definition or match branch separator |
| `<-` | Channel Send / Recv | Channel send/receive operator |
| `..` | Exclusive Range | Constructs a half-open range `[start, end)` |
| `..=` | Inclusive Range | Constructs a closed range `[start, end]` |
| `#` | Attribute Macro | Initiates an attribute annotation block (`#[...]`) |
| `@` | Compiler Intrinsic | Initiates compile-time evaluation (e.g., `@if`, `@offsetof`) |

---

## 6. Complete Lexer Token Reference

Full mapping of tokens generated by `Lexer.cpp`:

| `LsmTokenType` Enum Value | Source Literal / Token Example |
| --- | --- |
| `FullDev` | `fulldev` |
| `Unsafe` | `unsafe` |
| `Asm` | `asm` |
| `Volatile` | `volatile` |
| `Ptr` | `ptr` |
| `SetReg` | `setreg` |
| `GetReg` | `getreg` |
| `CpuHalt` | `cpu_halt`, `halt` |
| `CpuCli` | `cpu_cli`, `cli` |
| `CpuSti` | `cpu_sti`, `sti` |
| `PortIn` | `port_in` |
| `PortOut` | `port_out` |
| `MmioRead` | `mmio_read` |
| `MmioWrite` | `mmio_write` |
| `RegRax` .. `RegR15` | `rax`, `rbx`, `rcx`, `rdx`, `rsi`, `rdi`, `rbp`, `rsp`, `r8` .. `r15` |
| `Inb` .. `Outl` | `inb`, `outb`, `inw`, `outw`, `inl`, `outl` |
| `AtomicAdd` .. `AtomicStore` | `atomic_add`, `atomic_sub`, `atomic_cas`, `atomic_load`, `atomic_store` |
| `True` / `False` | `true`, `false` |
| `Print` / `Input` | `print`, `input` |
| `Fct` / `Proc` | `fct`, `proc` |
| `Rec` / `Array` / `Slice` | `rec`, `array`, `slice` |
| `Return` | `return` |
| `If` / `Else` | `if`, `else` |
| `While` / `For` / `In` / `To` | `while`, `for`, `in`, `to` |
| `Break` / `Continue` | `break`, `continue` |
| `Switch` / `Case` / `Default` | `switch`, `case`, `default` |
| `Class` / `Extends` / `This` / `New` | `class`, `extends`, `this`, `new` |
| `Import` / `Extern` | `import`, `extern` |
| `Go` / `Chan` / `Defer` | `go`, `chan`, `defer` |
| `Interface` / `Enum` / `Type` | `interface`, `enum`, `type` |
| `And` / `Or` / `Not` | `and`, `or`, `not` |
| `Try` / `Catch` / `Throw` | `try`, `catch`, `throw` |
| `Let` / `Nil` | `let`, `nil` |
| `Match` | `match` |
| `Ok` / `Err` | `Ok`, `Err` |
| `Some` / `None` | `Some`, `None` |
| `Ident` | Custom identifiers (`vga_clear`, `x`, `status`) |
| `Int` | Integer literals (`1920`, `0x1000`) |
| `Float` | Floating-point literals (`3.14159`) |
| `String` | String literals (`"kernel.lsm"`, `"Hello"`) |
| `Hash` / `At` | `#`, `@` |
| `Plus` .. `Shr` | `+`, `+=`, `++`, `-`, `-=`, `--`, `*`, `*=`, `/`, `/=`, `%`, `=`, `==`, `!=`, `>`, `>=`, `<`, `<=`, `&`, `&&`, `|`, `||`, `^`, `~`, `<<`, `>>` |
| `LParen` .. `QuestionSafe` | `(`, `)`, `{`, `}`, `[`, `]`, `,`, `.`, `:`, `;`, `?`, `=>`, `->`, `<-`, `..`, `..=`, `?.` |
| `Eof` | End-Of-File boundary marker |

