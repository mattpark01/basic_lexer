# The Splang Programming Language

Welcome to the Splang Programming Language - a harmonious blend of simplicity, safety, and versatility. Splang marries the effortless syntax of Python with the memory safety and performance of Rust, tailored for both high-level applications and systems programming. Moreover, its seamless interoperability with C opens a gateway to a vast ecosystem, making Splang an ideal choice for developers seeking efficiency, readability, and the power to operate close to the metal.

### Development Status: Under Construction 🚧 Do not use in production.

## Current Features

- **Simple Syntax**: Splang's syntax is designed to be easy to read and write, with clear structure and minimal boilerplate.
- **Static Typing**: Offers the safety and performance of static typing, making your programs more robust and faster.
- **Memory Management**: Manual and automated memory management capabilities to suit different scenarios and optimization needs.
- **Interoperability**: Designed for seamless integration with C libraries, allowing you to leverage existing native libraries.
- **Rich Standard Library**: Comes with a comprehensive standard library that covers common programming needs, from data manipulation to networking.

## Installation

Currently, Splang can be compiled from source. Here are the steps to get it up and running on your machine:

1. Clone the Splang repository:

   ```bash
   git clone https://github.com/mattpark01/splang.git
   ```

2. Navigate into the Splang directory:

   ```bash
   cd splang
   ```

3. Build Splang from source (make sure you have GCC or Clang installed):

   ```bash
   make
   ```

4. Optionally, install Splang to your system:

   ```bash
   sudo make install
   ```

## Getting Started

Here's a simple Splang program to get you started:

```splang
let greeting = "Hello, Splang!"
print(greeting)
```

Save this code in a file named `hello.sp`, and run it using the Splang interpreter:

```bash
splang hello.sp
```

## License

Splang is licensed under the MIT License.

## Acknowledgments

Splang is inspired by languages such as Python, Rust, and C.
