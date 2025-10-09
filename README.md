
Features
- Quotes-aware tokenization (single and double quotes)
- Multiple pipelines (cmd1 | cmd2 | cmd3 ...)
- Input (`<`) and output (`>`, `>>`) redirection
- Background execution with `&`
- In-memory command history with `history` builtin
- Basic job control: `jobs`, `fg %jid`
- Common builtins: `cd`, `pwd`, `help`, `exit`

Build
-----

Requires: gcc (POSIX/Linux)

From the project root run:

```bash
make
```

This produces the `tsh` executable.

Run
---

Start the shell:

```bash
./tsh
```

Examples:

- Run a pipeline: ls -l | grep ".c" | wc -l
- Redirect output: echo hello > out.txt
- Append output: echo more >> out.txt
- Background job: sleep 10 &
- List jobs: jobs
- Bring job to foreground: fg %1

Notes
-----

This is intentionally small and educational. It is not a full replacement for bash or zsh. Use it to demonstrate systems programming, process control, and signal handling on your resume.
