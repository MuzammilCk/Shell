# How to Run TSH on Windows

This shell project is built using **POSIX** system calls (`fork`, `exec`, `pipe`, `signals`, `termios`), which are native to Linux and Unix systems. These concepts do not translate directly to standard Windows.

To run this project on Windows, you **must** use a Linux environment. The best way to do this is **WSL (Windows Subsystem for Linux)**.

## Step 1: Install WSL (if not installed)

1. Open PowerShell as Administrator.
2. Run: `wsl --install`
3. Restart your computer if prompted.
4. After restart, open "Ubuntu" from your Start menu and set up a username/password.

## Step 2: Install Build Tools

Inside your WSL terminal (Ubuntu), install the necessary compiler and tools:

```bash
sudo apt update
sudo apt install build-essential make gcc valgrind -y
```

## Step 3: Access Your Project

WSL can access your Windows files. 
Navigate to your project directory. For example, if your project is in `D:\projects\Shell-1`, run:

```bash
cd /mnt/d/projects/Shell-1
```

## Step 4: Build and Run

Now you can compile and run the shell as intended:

```bash
# Clean previous builds
make clean

# Build
make

# Run
./tsh
```

## Troubleshooting

- **`make` not found**: Run `sudo apt install make`.
- **`fork` undefined**: You are likely trying to compile with MinGW or standard Windows GCC. Please switch to WSL.
- **`glob.h` not found**: This is a POSIX header, available in WSL/Linux.

## Why WSL?
This project demonstrates "Systems Engineering" concepts like Process Control and Signals. Windows handles these differently (e.g., `CreateProcess` instead of `fork`). Using WSL allows you to write and run professional-grade Unix systems code directly on your Windows machine.
