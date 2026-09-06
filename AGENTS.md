# Repository Guidelines

## Project Structure & Module Organization

This is a small C++23 compiler project for the Cherry language (namespace
`avium`). Production code is in `src/`: AST types and visitors (`ast.hxx`,
`astvisitor.hxx`), AST Lisp output (`astlisp.*`), lexical tokens (`lexeme.*`),
scanner and parser (`scanner.*`, `parser.*`, `diagnostics.*`), and the
executable entry point (`main.cxx`). Catch2 v3 tests live in `tests/`, with a
separate `tests/CMakeLists.txt`. Language documentation and the formal grammar
are in `specs/`; longer teaching material is in `book/`.
Runnable Cherry source examples are kept in `examples/` as `.bas` files.

## Build, Test, and Development Commands

Configure and build from a disposable build directory:

```sh
cmake -S . -B build
cmake --build build
```

Run the complete test suite with:

```sh
ctest --test-dir build --output-on-failure
```

The default executable is `build/prunus`. CMake discovers Catch2 v3 and builds
one `build/tests/prunus-tests` executable containing AST, astlisp, lexeme,
scanner, and parser tests.

## Coding Style & Naming Conventions

Use C++23 and format every C/C++ source or header with the repository’s
`_clang-format` file. A typical check is:

```sh
clang-format --dry-run --Werror src/*.cxx src/*.hxx tests/*.cxx
```

Use four spaces, no tabs; classes and public types use `PascalCase`, methods
and variables use `camelCase`, and private data members use a leading
underscore (for example, `_line`). Keep code in the `avium` namespace.

## Testing Guidelines

Add or update Catch2 v3 tests under `tests/` when changing behavior. Name test
files after the module (`ast_test.cxx`, `scanner_test.cxx`) and group cases with
tags such as `[scanner]`. Every test should be deterministic and pass through
CTest before submission; no separate coverage threshold is currently defined.

## Commit & Pull Request Guidelines

Existing history uses short, focused commit subjects, including Armenian
descriptions. Follow the same practice: describe one change in the imperative
or concise present tense and avoid unrelated cleanup. Pull requests should
explain the behavior change, identify affected modules, list validation
commands (especially `ctest`), and include relevant specification updates.

## Architecture Notes

The intended pipeline is lexeme scanning → parsing → AST → semantic analysis →
execution/backend. Keep lexical rules aligned with `specs/language.md` and
syntax aligned with `specs/syntax.md`; scanner errors are represented as
`Token::None` and parser errors are collected by `Diagnostics`.
