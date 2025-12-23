# hd — Simple Hex Dump Utility (C++20)

`hd` is a small, fast, and readable **hex dump command-line utility** written in modern C++20.  
It is designed to behave like classic Unix hexdump tools while keeping the codebase clean, safe, and easy to understand.

This project was originally written as a learning and utility tool and later modernized to idiomatic C++20.

---

## Features

- Hex + ASCII dump format
- Optional address and ASCII display
- Start dump at arbitrary offsets
- Limit number of bytes dumped
- Supports large (64-bit) files
- Zero global state
- No raw memory management
- Clean, portable C++20 code

---

## Example Output

```text
00000000   48 65 6c 6c 6f 2c 20 77  6f 72 6c 64 21 0a      Hello, world!.


