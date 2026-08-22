
# LSM Language Reference Manual

A comprehensive specification and technical reference for the **LSM** systems programming language.

---

## 1. Top-Level Directives

### `fulldev`
Specifies that the compilation unit is operating in freestanding/bare-metal mode without runtime wrappers, boilerplate setup, or standard runtime assumptions
```lsm
fulldev

```

### `import`

Imports external LSM source units, header definitions, or modules. Paths are resolved relative to working include paths or relative file paths.

```lsm
import "kernel/vga.lsm";
import "kernel/system.lsm";
import "apps/editor.lsm";

```

---

## 2. Keywords

| Keyword | Description |
| --- | --- |
| `fct` | Declares a named function signature.

 |
| `let` | Declares a local or mutable variable.

 |
| `if` | Conditional branch execution.

 |
| `else` | Fallback conditional branch execution.

 |
| `while` | Loop execution evaluated before every iteration.

 |
| `return` | Returns control and an optional expression value from the current function.

 |
| `unsafe` | Encloses unmanaged memory access, MMIO operations, and hardware intrinsics.

 |

---

## 3. Data Types

| Type | Size / Representation | Description |
| --- | --- | --- |
| `Int` | 64-bit integer | Standard numeric and pointer/address type in 64-bit mode. Supports signed/unsigned integer arithmetic.

 |
| `void` | 0-bit / Unit | Indicates no return value from a function signature.

 |

---

## 4. Functions & Signatures

Functions are declared using the `fct` keyword, followed by typed parameter lists and explicit return types (`-> Type`).

### Void Functions

```lsm
fct kernel_init() -> void {
    let bg = 0;
    vga_clear(bg);
}

```

### Functions with Parameters and Return Values

```lsm
fct add_values(a: Int, b: Int) -> Int {
    let result = a + b;
    return result;
}

```

---

## 5. Variables & Assignment

Variables are defined using `let`. Variables can hold immediate numeric literals, evaluation expressions, or memory pointers.

```lsm
let screen_width = 1920;
let base_addr = 16777216;
let is_active = 1;

```

Reassignment uses standard `=` syntax:

```lsm
let count = 0;
count = count + 1;

```

---

## 6. Control Flow

### `if` / `else if` / `else`

Conditional branches evaluate integer truthiness (`0` is false, non-zero is true).

```lsm
if (dir == 1) {
    head_y = head_y - 1;
} else if (dir == 2) {
    head_y = head_y + 1;
} else {
    head_x = head_x + 1;
}

```

### `while`

Loops execute while condition expressions evaluate to non-zero values.

```lsm
let i = 0;
while (i < 100) {
    vga_putc(32, i, 0, 8);
    i = i + 1;
}

```

---

## 7. Operators

### Arithmetic Operators

* `+` : Addition


* `-` : Subtraction / Negation


* `*` : Multiplication


* `/` : Integer Division



### Comparison Operators

* `==` : Equality test


* `!=` : Inequality test


* `<`  : Less than


* `<=` : Less than or equal


* `>`  : Greater than


* `>=` : Greater than or equal



### Logical Operators

* `&&` : Logical AND


* `||` : Logical OR



---

## 8. Low-Level Memory & Hardware Intrinsics

All low-level memory manipulations, raw pointer dereferences, and I/O bus operations must be enclosed within an `unsafe` block.

### Memory-Mapped I/O Intrinsics

#### `mmio_write(address: Int, value: Int) -> void`

Writes an 8-bit/32-bit byte or dword directly to the target absolute physical memory address.

```lsm
unsafe {
    mmio_write(0x60000 + offset, 1);
}

```

#### `mmio_read(address: Int) -> Int`

Reads an 8-bit/32-bit value directly from the specified absolute physical memory address.

```lsm
unsafe {
    let byte_val = mmio_read(buffer_ptr + index);
}

```

### High-Performance Memory Block Copy

#### `fast_copy(dst: Int, src: Int, dwords: Int) -> void`

Executes hardware-accelerated memory block transfer (`rep movsd` instruction equivalent) for high-speed framebuffer swapping and double buffering.

```lsm
fct vga_flip() -> void {
    let src = get_back_buffer();
    let dst = get_vbe_framebuffer();
    let total_dwords = 2073600; // 1920 * 1080
    fast_copy(dst, src, total_dwords);
}

```

### CPU Control Intrinsics

#### `cpu_cli() -> void`

Disables maskable hardware interrupts by clearing the interrupt flag (`cli` opcode).

```lsm
unsafe {
    cpu_cli();
}

```

---

## 9. Character and String Literals

In LSM bare-metal mode, string sequences and ASCII characters are represented through explicit ASCII character codes or sequential byte buffers:

* `32` : Space (`' '`)


* `35` : Hash (`'#'`)


* `46` : Dot (`'.'`)


* `48` - `57` : Digits (`'0'` - `'9'`)


* `65` - `90` : Uppercase letters (`'A'` - `'Z'`)


* `97` - `122` : Lowercase letters (`'a'` - `'z'`)


* `10` : Newline (`'\n'`)


* `8` : Backspace (`'\b'`)


* `27` : Escape (`ESC`)



---

## 10. Complete Bare-Metal Example

```lsm
fulldev

import "kernel/vga.lsm";
import "kernel/keyboard.lsm";

fct main() -> void {
    let bg_color = 0;
    vga_clear(bg_color);

    // Draw Character 'A' (ASCII 65) at Column 10, Row 5 with Color 14 (Yellow)
    vga_putc(65, 10, 5, 14);
    vga_flip();

    let running = 1;
    while (running == 1) {
        let key = kbd_getc();
        if (key == 27) { // ESC key
            running = 0;
        }
    }
}

```
---

## 11. Lexer Specification & Token Reference

### 11.1 Keywords & Modifiers

| Token Enum | Source Text | Description |
| :--- | :--- | :--- |
| `TOKEN_FULLDEV` | `fulldev` | Freestanding bare-metal translation unit directive |
| `TOKEN_IMPORT` | `import` | External file and module inclusion |
| `TOKEN_FCT` | `fct` | Function definition keyword |
| `TOKEN_LET` | `let` | Variable binding and allocation |
| `TOKEN_IF` | `if` | Conditional branch initiation |
| `TOKEN_ELSE` | `else` | Alternative branch execution |
| `TOKEN_WHILE` | `while` | Pre-condition loop construct |
| `TOKEN_RETURN` | `return` | Function control exit and return pass |
| `TOKEN_UNSAFE` | `unsafe` | Direct memory access and intrinsic execution block |

---

### 11.2 Built-in Type Tokens

| Token Enum | Source Text | Semantic Representation |
| :--- | :--- | :--- |
| `TOKEN_TYPE_INT` | `Int` | 64-bit signed/unsigned machine word / pointer integer |
| `TOKEN_TYPE_VOID` | `void` | Empty/unit return type |

---

### 11.3 Literals & Identifiers

| Token Enum | Pattern / Format | Example Match | Description |
| :--- | :--- | :--- | :--- |
| `TOKEN_IDENTIFIER` | `[a-zA-Z_][a-zA-Z0-9_]*` | `vga_clear`, `cmd_buf`, `x` | User-defined variable, function, or symbol name |
| `TOKEN_INT_LITERAL` | `[0-9]+` \| `0x[0-9a-fA-F]+` | `1920`, `0x60000`, `0` | Immediate base-10 integer or base-16 hex constant |
| `TOKEN_STRING_LITERAL`| `"[^"]*"` | `"kernel/vga.lsm"` | Filepath string literal for module imports |

---

### 11.4 Operators & Punctuators

| Token Enum | Symbol | Category | Description |
| :--- | :--- | :--- | :--- |
| `TOKEN_PLUS` | `+` | Arithmetic | Addition |
| `TOKEN_MINUS` | `-` | Arithmetic | Subtraction / Negation |
| `TOKEN_STAR` | `*` | Arithmetic | Multiplication |
| `TOKEN_SLASH` | `/` | Arithmetic | Division |
| `TOKEN_ASSIGN` | `=` | Assignment | Value assignment to variable |
| `TOKEN_EQ` | `==` | Comparison | Equality comparison |
| `TOKEN_NEQ` | `!=` | Comparison | Inequality comparison |
| `TOKEN_LT` | `<` | Comparison | Less than |
| `TOKEN_LTE` | `<=` | Comparison | Less than or equal |
| `TOKEN_GT` | `>` | Comparison | Greater than |
| `TOKEN_GTE` | `>=` | Comparison | Greater than or equal |
| `TOKEN_AND` | `&&` | Logical | Logical AND |
| `TOKEN_OR` | `\|\|` | Logical | Logical OR |
| `TOKEN_ARROW` | `->` | Structure | Return type separator |
| `TOKEN_COLON` | `:` | Structure | Type annotation separator |
| `TOKEN_SEMICOLON` | `;` | Delimiter | Statement terminator |
| `TOKEN_COMMA` | `,` | Delimiter | Argument and parameter list separator |
| `TOKEN_LPAREN` | `(` | Delimiter | Expression / argument grouping start |
| `TOKEN_RPAREN` | `)` | Delimiter | Expression / argument grouping end |
| `TOKEN_LBRACE` | `{` | Delimiter | Block scope opening |
| `TOKEN_RBRACE` | `}` | Delimiter | Block scope closing |

---

### 11.5 Whitespace, Comments & Special Tokens

| Token Enum | Regex / Syntax | Action |
| :--- | :--- | :--- |
| `TOKEN_WHITESPACE` | `[ \t\r\n]+` | Skipped by Lexer (token boundary delimiter) |
| `TOKEN_COMMENT_LINE`| `//[^\n]*` | Single-line comment; discarded during lexical analysis |
| `TOKEN_EOF` | `\0` | End-Of-File marker signalling source stream completion |
