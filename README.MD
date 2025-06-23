# CDash Programming Language Compiler

CDash is a simple compiled programming language inspired by C and C++ designed to generate assembly code and `.exe` files. It supports basic syntax including variable declaration, input/output, and string concatenation.

---
## Syntaxes

 🔢 Feature           | ✅ Syntax Example                                              | 📝 Description                                  |
|----------------------|---------------------------------------------------------------|-------------------------------------------------|
| Variable Declaration | `let name = "John";`<br>`let age = 25;`                      | Declares a variable with a string or number     |
| User Input           | `let name = input("Enter your name: ");`                     | Gets input from user and stores in a variable   |
| Output / Print       | `print("Hello");`<br>`print(name);`<br>`print("Hi " + name);`| Outputs text or variable values to console      |

## Features

- Variable declarations with `let`
- Console input using `input(prompt)`
- Console output using `print()`
- String concatenation with `+` operator inside `print`
- Compilation to NASM assembly and linking to `.exe`
- Simple syntax inspired by C-style languages

---

## Usage

### Compile CDash compiler

```bash
g++ main.cpp -o CCDash
```

To run your `.cdash` file

```bash
./CCDash example.cdash -o example.exe
```

After it compiled. Make sure that it generated two files called `temp.asm` or `temp.o`.

### Learning to use

Here is a simple code for starters.

```c++
let name = "John Doe";
print(name);
```

Here is the user input code.

```c++
let name = "Whats your name?";
print("Hello");
print(name);
```

If you want to contact us, contact us at mmbk88221@gmail.com
