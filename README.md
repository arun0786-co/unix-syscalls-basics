# Unix system call basics

Two small C programs. The first is a tiny file-based shell that reads and
writes files using only printf and scanf, by redirecting file descriptors with
dup2. The second shows fork, exec, wait and orphaned processes. Written for
Assignment 0 of Operating Systems and Networks at IIIT Hyderabad (2025) and
reworked later.

## Build

```
make          # builds Part-1/fileManagement, Part-2/processManagement, Part-2/writer
make test     # 51 checks, including the buffering bugs described below
make clean    # removes the binaries and Part-2/newfile.txt
```

Each program is a separate compile, so writer can also be built on its own:
`cc -Wall -o Part-2/writer Part-2/writer.c`.

## Part 1: File management

```
cd Part-1
./fileManagement
```

The program creates `folder_<pid>` in the current directory with an empty
`content.txt` and `logs.txt`, then reads commands until `STOP`.

| Command       | Effect                                                  | Logged as     |
|---------------|---------------------------------------------------------|---------------|
| `INPUT`       | prompts `INPUT -> ` and appends the next line to content.txt | `INPUT`  |
| `PRINT`       | prints all of content.txt                               | `PRINT`       |
| `FIRST <n>`   | prints the first n lines of content.txt                 | `FIRST <n>`   |
| `LAST <n>`    | prints the last n lines of content.txt                  | `LAST <n>`    |
| `LOG <n>`     | prints the last n lines of logs.txt                     | not logged    |
| `STOP`        | exits                                                   | `STOP`        |
| anything else | prints `Unknown command.`                               | not logged    |

### Reading and writing files with only printf and scanf

printf always writes to file descriptor 1 and scanf always reads from file
descriptor 0. To append a line to a file, the program opens the file, keeps a
copy of fd 1 with `dup(1)`, points fd 1 at the file with `dup2(fd, 1)`, calls
printf, and then puts the saved descriptor back. Reading a file works the same
way with fd 0 and scanf.

stdio keeps a buffer above each descriptor, so that buffer has to be empty
whenever the descriptor under it is switched:

- stdout is flushed before fd 1 is pointed at a file and again before it is
  switched back. Otherwise a prompt still waiting in the buffer (such as
  `Enter command:`) is written into the file.
- stdin is unbuffered (`setvbuf(stdin, NULL, _IONBF, 0)`). A buffered stdin
  reads ahead, and when input comes from a pipe it can pull in many lines of
  future commands at once. Those buffered bytes would then be returned while
  fd 0 points at the file. After a file has been read to the end,
  `clearerr(stdin)` resets the end-of-file flag before the terminal is read
  again.

`LAST <n>` and `LOG <n>` read the file twice: once to count the lines and once
to print the last n.

### Assumptions

- No line is longer than 1024 characters (given in the spec).
- Commands are case-sensitive. Spaces and tabs around words are ignored, and so
  is a trailing `\r`, so input files with Windows line endings work.
- `n` must be a non-negative integer. A missing, negative or non-numeric `n`,
  an extra word (`PRINT x`, `INPUT hello`) and unrecognized words are all
  unknown commands: `Unknown command.` is printed and nothing is logged.
- A blank command line is ignored.
- `n = 0` prints nothing. If `n` is larger than the number of lines, every line
  is printed.
- An empty line typed at `INPUT -> ` is stored as an empty line.
- If input ends (Ctrl-D, or the end of a piped file) without `STOP`, the
  program exits without logging `STOP`.
- Errors such as a failed `open` are reported with printf, since printf is the
  only output function allowed.

## Part 2: Process creation

```
cd Part-2
./processManagement
```

Run it from `Part-2/`, because Task 2 runs `./writer` by relative path.

- **Task 1** forks. The child adds 10 to `x` and prints 35, the parent
  subtracts 5 and prints 20. After fork each process has its own copy of `x`,
  so neither sees the other's change.
- **Task 2** forks, and the child calls `execvp("./writer")`. writer writes
  `getppid()` to `newfile.txt` and exits, while the parent waits for it. The
  parent also prints its own PID so it can be compared with the file.
- **Task 3** forks. The parent prints its PID and returns from main without
  waiting, so it exits first. The child sleeps 2 seconds, then prints its own
  PID and `getppid()`.

stdout is flushed before every fork. fork copies the parent's memory, and
printf's buffer is part of that memory, so text that has not been flushed yet
would be printed by both processes. This shows up when the output is piped
(`./processManagement | cat`), because stdout is then fully buffered instead
of line buffered.

### Comparing Task 2 and Task 3

Output from one run on Linux:

```
TASK 2
parent pid: 841 (compare with newfile.txt)
TASK 3
parent pid: 841
child pid: 845, parent pid: 1

$ cat newfile.txt
841
```

**Why they differ.** exec replaces the program a process is running, but the
process keeps its PID and its parent. In Task 2 writer is still the child of
processManagement, and that parent is alive (blocked in `waitpid`) when writer
calls `getppid()`, so the file holds the parent's PID. In Task 3 the parent
has already exited by the time the child calls `getppid()` after its 2 second
sleep. A process whose parent has exited is an orphan, and the kernel gives it
a new parent, so `getppid()` no longer returns the original PID.

On Linux the new parent is the nearest living ancestor that has marked itself
a child subreaper with `prctl(PR_SET_CHILD_SUBREAPER)`, or PID 1 if there is
none. In a desktop session `systemd --user` is often a subreaper, so the
printed parent PID can be that process instead of 1. On macOS the new parent
is launchd, which is PID 1.

**Role of init/systemd.** init is the first user-space process the kernel
starts, and it always has PID 1. On most Linux distributions it is systemd.
All other user processes are its descendants. It adopts orphaned processes and
calls `wait()` on them when they exit. Without that, an exited orphan would
stay a zombie forever: a process's entry in the process table, which holds its
exit status, is only removed once its parent collects that status with
`wait()`. systemd also starts and supervises system services and handles
shutdown.
