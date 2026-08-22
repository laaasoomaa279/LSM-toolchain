# LSM Programming Language: Complete Guide (Zero to Hero)

Welcome to the official developer guide for **LSM** (Language for Systems & Machines).

LSM is a modern, statically typed, high-performance systems programming language designed from the ground up for low-level software engineering, operating system kernels, bare-metal development, and high-performance applications.

---

## 1. Introduction & Core Philosophy

LSM combines expressive, modern syntax with direct, zero-cost hardware access.

* **High-Level Productivity:** Expressive functions, pattern matching, structured error handling, generic typing, and clean control flow.
* **Low-Level Control:** Pointer dereferencing, direct register manipulation, port I/O, volatile memory access, inline assembly, and freestanding execution.
* **Zero Runtime Overhead:** Direct native compilation to x86_64, ARM64, and flat bare-metal binary images.

---

## 2. Setting Up & CLI Basics

The LSM compiler toolchain operates directly via the command line.

### Running a File in JIT Mode

To instantly compile and execute code via the multi-threaded JIT engine:

```bash
./lsm run main.lsm

```

### Compiling to Native Executable

```bash
./lsm build main.lsm -o main.exe

```

### Compiling with Maximum Optimizations (-O3)

```bash
./lsm build main.lsm -O3 -o optimized_program

```

### Compiling a Bare-Metal Kernel Image

```bash
./lsm build --baremetal --bootable kernel.lsm -o os_image.img

```

---

## 3. Basic Syntax & Variables

### Entry Point

Every program begins execution at the `main` function or the `_start` entry point:

```lsm
fct main() -> Int {
    print("Welcome to LSM!");
    return 0;
}

```

### Variables and Types

Variables are declared using the `let` keyword. You can declare variables with automatic type inference or explicit static typing.

```lsm
fct main() -> Int {
    // Inferred types
    let count = 10;
    let price = 49.99;
    let title = "System Boot";

    // Explicit types
    let score: Int = 100;
    let ratio: Float = 3.14159;
    let message: String = "Engine Online";

    print(score);
    return 0;
}

```

### Supported Primitive Literals

* **Integers:** Decimal (`42`), Hexadecimal (`0x2A`)
* **Floats:** Double-precision values (`12.34`)
* **Strings:** UTF-8 byte sequences (`"Hello World"`)
* **Booleans:** `true`, `false`
* **Pointers / Empty References:** `nil`

---

## 4. Functions and Procedures

LSM distinguishes between value-returning functions (`fct`) and void procedures (`proc`).

### Returning Functions (`fct`)

```lsm
fct add(a: Int, b: Int) -> Int {
    let result = a + b;
    return result;
}

```

### Void Procedures (`proc`)

```lsm
proc log_status(code: Int) {
    print("Execution code:");
    print(code);
}

```

### Generic Functions

```lsm
fct identity<T>(value: T) -> T {
    return value;
}

```

---

## 5. Control Flow

### If-Else Statements

```lsm
fct check_temperature(temp: Int) -> void {
    if temp >= 100 {
        print("Boiling!");
    } else if temp <= 0 {
        print("Freezing!");
    } else {
        print("Normal");
    }
}

```

### While Loops

```lsm
fct count_down(start: Int) -> void {
    let current = start;
    while current > 0 {
        print(current);
        current--;
    }
}

```

### For Loops

```lsm
fct iterate_range() -> void {
    for i in 0..10 {
        print(i);
    }
}

```

### Switch Statements

```lsm
fct parse_command(cmd: Int) -> void {
    switch (cmd) {
        case 1: {
            print("Action: Start");
        }
        case 2: {
            print("Action: Stop");
        }
        default: {
            print("Action: Unknown");
        }
    }
}

```

---

## 6. Data Structures: Records & Classes

### Records (`rec`)

Records provide flat, cache-efficient memory layouts ideal for system structures.

```lsm
rec Point {
    x;
    y;
}

fct create_point(x_pos: Int, y_pos: Int) -> Int {
    let p = new Point();
    p.x = x_pos;
    p.y = y_pos;
    return p.x + p.y;
}

```

### Memory Alignment Attributes

```lsm
#[align("16")]
rec AlignedBuffer {
    data;
    size;
}

```

### Classes & Methods

```lsm
class Counter {
    value;

    fct increment() -> Int {
        this.value = this.value + 1;
        return this.value;
    }
}

```

---

## 7. Arrays and Buffer Allocations

### Fixed-Size Stack Arrays

```lsm
fct array_demo() -> void {
    let buffer: [Int; 4];
    buffer[0] = 10;
    buffer[1] = 20;
    buffer[2] = 30;
    buffer[3] = 40;

    let index = 0;
    while index < 4 {
        print(buffer[index]);
        index++;
    }
}

```

---

## 8. Pattern Matching & Error Types

LSM provides expressive algebraic constructs for functional error handling.

```lsm
fct fetch_status(code: Int) -> Ok(Int) {
    if code == 0 {
        return Ok(200);
    }
    return Err(500);
}

fct handle_status() -> void {
    let res = fetch_status(0);

    match res {
        Ok(code) => {
            print("Operation succeeded with code:");
            print(code);
        }
        Err(err) => {
            print("Operation failed!");
        }
    }
}

```

---

## 9. Low-Level & Bare-Metal Programming

To enable complete freestanding execution, use the `fulldev` mode.

```lsm
fulldev

```

### Unsafe Blocks & Pointer Arithmetic

```lsm
proc manipulate_memory(address: Int) {
    unsafe {
        // Cast integer address to typed pointer
        let raw_ptr = ptr<Int>(address);
        
        // Dereference and assign
        *raw_ptr = 0xAA55;
    }
}

```

### Volatile Memory-Mapped I/O (MMIO)

```lsm
proc write_vga(offset: Int, character: Int) {
    unsafe {
        let vga_base = 0xB8000;
        let dest = vga_base + offset;
        mmio_write16(dest, character);
        let val = mmio_read16(dest);
    }
}

```

### Port I/O Operations

```lsm
proc write_serial(port: Int, data: Int) {
    unsafe {
        outb(port, data);
        let status = inb(port);
    }
}

```

### CPU Control & Inline Assembly

```lsm
proc halt_system() {
    unsafe {
        cpu_cli();
        asm("hlt");
    }
}

```

---

## 10. External C / Win32 / POSIX Interoperability

LSM can link and call external dynamic library symbols with zero glue code.

```lsm
fulldev

extern "kernel32.dll" fct Sleep(ms: Int) -> void;
extern "kernel32.dll" fct Beep(dwFreq: Int, dwDuration: Int) -> Int;

fct main() -> Int {
    print("Playing sound...");
    Beep(750, 300);
    Sleep(500);
    print("Done!");
    return 0;
}

```

---

## 11. Complete Practical Project: Fibonacci Search & Memory Sorter

```lsm
fulldev

fct bubble_sort(items: [Int; 5]) -> void {
    let i = 0;
    while i < 5 {
        let j = 0;
        while j < 4 {
            if items[j] > items[j + 1] {
                let temp = items[j];
                items[j] = items[j + 1];
                items[j + 1] = temp;
            }
            j++;
        }
        i++;
    }
}

fct main() -> Int {
    let numbers: [Int; 5];
    numbers[0] = 64;
    numbers[1] = 12;
    numbers[2] = 99;
    numbers[3] = 3;
    numbers[4] = 45;

    print("Sorting array...");
    bubble_sort(numbers);

    let k = 0;
    while k < 5 {
        print(numbers[k]);
        k++;
    }

    return 0;
}

```
