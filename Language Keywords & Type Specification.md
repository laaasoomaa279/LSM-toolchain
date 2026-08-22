# LSM Language Reference & Keywords Specification

## Core Keywords

* `fct`: Declares a standard function that computes and returns a value.
* `proc`: Declares a procedure (a subroutine that performs side effects without returning a value).
* `let`: Binds a new variable to a value with optional type annotations or fixed-size array allocations.
* `nil`: Represents a null/empty pointer or the absence of a value.
* `true`: Boolean literal for true.
* `false`: Boolean literal for false.
* `this`: Refers to the current instance inside class methods.
* `new`: Instantiates a new class instance and invokes its constructor.
* `type`: Declares a custom type alias.
* `rec`: Defines a plain-old-data record structure with configurable memory alignment.
* `enum`: Defines an enumeration type containing distinct variants.
* `array`: Declares an array structure.
* `slice`: Declares a dynamically-sized view into contiguous memory.
* `input`: Built-in expression keyword for reading user input.
* `print`: Built-in expression/statement for writing output to standard streams.

## Control Flow & Branching

* `if`: Evaluates a condition to execute a block of code conditionally.
* `else`: Alternative execution branch when an `if` condition evaluates to false.
* `while`: Repeats execution of a block as long as a condition holds true.
* `for`: Iterates over collections or integer ranges.
* `in`: Specifies the target collection or range in a `for` loop.
* `to`: Specifies the upper bound in legacy loop ranges.
* `break`: Immediately terminates the innermost loop.
* `continue`: Skips to the next iteration of the innermost loop.
* `switch`: Evaluates an expression against multiple matching values.
* `case`: Defines a branch value inside a `switch` statement.
* `default`: Fallback branch in a `switch` statement when no `case` matches.
* `return`: Returns execution and an optional value from a function.

## Error Handling & Pattern Matching

* `match`: Performs pattern matching over expressions and algebraic types.
* `Ok`: Constructs or matches a successful `Result` variant wrapping a payload.
* `Err`: Constructs or matches an error `Result` variant wrapping a payload.
* `Some`: Constructs or matches an `Option` variant containing a value.
* `None`: Represents an empty `Option` variant containing no value.
* `try`: Encloses a block of code that may produce exceptions.
* `catch`: Catches and handles exceptions thrown inside a `try` block.
* `throw`: Throws an exception or error expression.

## Object-Oriented & Modular Programming

* `class`: Declares an object-oriented class with fields and methods.
* `extends`: Inherits members and behavior from a base class.
* `interface`: Declares an abstract interface contract of required method signatures.
* `import`: Imports modules or external source files into the current compilation unit.
* `extern`: Declares foreign function interface (FFI) bindings from shared libraries (e.g., DLLs, .so).

## Concurrency & Resource Management

* `go`: Spawns an expression or function call as an asynchronous lightweight task.
* `chan`: Declares a synchronous or buffered communication channel between tasks.
* `defer`: Defers the execution of a statement or function call until the enclosing function returns.

## Bare-Metal & Hardware Control (ALSM)

* `fulldev`: Module-level directive enabling unrestricted low-level system access.
* `unsafe`: Encloses code blocks performing direct memory, pointer, or hardware operations.
* `asm`: Inlines raw target machine assembly strings directly into the code stream.
* `volatile`: Enforces volatile memory reads/writes to prevent compiler optimization on memory-mapped addresses.
* `ptr`: Performs explicit pointer casting to a target type.
* `port_in`: Reads an unsigned integer from a specified hardware I/O port.
* `port_out`: Writes an integer value to a specified hardware I/O port.
* `inb`: Direct 8-bit port input instruction.
* `outb`: Direct 8-bit port output instruction.
* `inw`: Direct 16-bit port input instruction.
* `outw`: Direct 16-bit port output instruction.
* `inl`: Direct 32-bit port input instruction.
* `outl`: Direct 32-bit port output instruction.
* `mmio_read`: Reads raw bytes from a specified physical memory address.
* `mmio_write`: Writes raw bytes to a specified physical memory address.
* `setreg`: Writes an explicit value directly into a designated CPU register.
* `getreg`: Reads the current value from a designated CPU register.
* `cpu_halt` / `halt`: Halts CPU instruction execution until the next interrupt.
* `cpu_cli` / `disable_interrupts`: Disables CPU hardware interrupts (CLI instruction).
* `cpu_sti` / `enable_interrupts`: Enables CPU hardware interrupts (STI instruction).
* `atomic_add`: Atomically adds a value to a memory location.
* `atomic_sub`: Atomically subtracts a value from a memory location.
* `atomic_cas`: Performs an atomic compare-and-swap operation.
* `atomic_load`: Atomically reads a value from a memory location.
* `atomic_store`: Atomically writes a value to a memory location.

## Built-in Data Types

* `Int` / `Int64` / `i64`: 64-bit signed integer.
* `Int32` / `i32`: 32-bit signed integer.
* `Int16`: 16-bit signed integer.
* `Int8`: 8-bit signed integer.
* `Float` / `Float64` / `f64`: 64-bit double-precision floating-point number.
* `String` / `str`: UTF-8 encoded text string.
* `Bool`: Boolean value (`true` or `false`).
* `Ptr`: Native 64-bit memory pointer.
* `Ptr32`: 32-bit memory pointer for 32-bit targets.
* `void`: Represents the absence of a type or return value.
* `dynamic`: Dynamically-resolved type used when static annotations are omitted.