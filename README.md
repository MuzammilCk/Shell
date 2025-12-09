# TSH - Tiny Enhanced Shell

A professional, feature-rich C implementation of a Unix shell, designed to demonstrate systems programming concepts including process control, signal handling, and memory management.

## Features

### System Enhancements
- **Process Group Management**: Implements rigorous foreground/background process group handling (`setpgid`, `tcsetpgrp` logic simulated via signal forwarding).
- **Signal Multiplexing**: Professional handling of `SIGINT` (Ctrl+C) and `SIGTSTP` (Ctrl+Z). Signals are trapped and forwarded to the foreground process group, ensuring the shell itself remains responsive.
- **Job Control**: Full support for job states:
    - **Foreground**: Standard execution.
    - **Background**: Execute with `&`.
    - **Stopped**: Suspend with Ctrl+Z.
    - **Resume**: Use `bg` to continue in background, `fg` to bring to foreground.
- **Memory Safety**: Audited memory management for zero leaks during standard operation. `free_jobs` and `free_history` ensure clean shutdown.
- **Environment Variables**: Builtin `export` and `unset` commands, with `$?` exit status expansion.
- **Wildcards (Globbing)**: Support for `*` and `?` wildcard expansion in command arguments.

### User Experience
- **Custom Prompt**: Informative, colored prompt showing `user@host:path$`.
- **Line Editing**: Custom raw-mode `readline` implementation providing:
    - **History Navigation**: Up/Down arrow keys.
    - **Tab Completion**: Auto-completion for filenames in the current directory.
    - **Editing**: Backspace and insertion.
    - No external dependency on `libreadline`.

## Architecture

The project is modularized for maintainability:

```
.
├── include/
│   ├── builtins.h     # Builtin command prototypes
│   ├── job_control.h  # Job management structs and signals
│   ├── parser.h       # Command parsing logic
│   └── readline.h     # Raw mode input handling
├── src/
│   ├── main.c         # Entry point, REPL, signal initialization
│   ├── builtins.c     # Implementation of cd, jobs, fg, bg, history
│   ├── job_control.c  # Job list maintenance and SIGCHLD handler
│   ├── parser.c       # Tokenizer and command parser
│   └── readline.c     # Terminal raw mode and history logic
└── Makefile           # Robust build system
```

## Building

To build the shell, simply run:

```bash
make
```

To build with debug symbols:

```bash
make debug
```

## Testing

An integration test suite is provided in `test_shell.py`. It requires Python 3.

```bash
python3 test_shell.py
```

## Usage

Start the shell:

```bash
./tsh
```

### Supported Builtins
- `cd [dir]`: Change directory.
- `pwd`: Print working directory.
- `exit`: Exit the shell.
- `jobs`: List background/stopped jobs.
- `fg %jid`: Bring job to foreground.
- `bg %jid`: Resume stopped job in background.
- `export KEY=VALUE`: Set environment variable.
- `unset KEY`: Unset environment variable.
- `history`: Show command history.

## Systems Concepts Demonstrated
- **Waitpid with WUNTRACED**: Correctly detecting stopped children.
- **Signal Masking/Forwarding**: Ensuring signals reach the correct process group.
- **Termios Raw Mode**: Implementing custom input handling at the terminal driver level.
