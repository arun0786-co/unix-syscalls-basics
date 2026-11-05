#!/usr/bin/env python3
"""Tests for both parts. usage: make && python3 tests/run.py"""
import os
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FILE = os.path.join(ROOT, "Part-1", "fileManagement")
PROC = os.path.join(ROOT, "Part-2", "processManagement")
WRITER = os.path.join(ROOT, "Part-2", "writer")
failures, passed = [], 0


def check(name, good, detail=""):
    global passed
    if good:
        passed += 1
        print("ok   " + name)
    else:
        failures.append(name)
        print(f"FAIL {name}: {detail!r}")


def same(name, got, want):
    check(name, got == want, f"got {got!r}, want {want!r}")


def session(*lines, stdin=None):
    """Run the file program in a new directory. Returns stdout and its folder."""
    work = tempfile.mkdtemp(prefix="fm")
    data = stdin if stdin is not None else "".join(line + "\n" for line in lines)
    p = subprocess.run([FILE], input=data, cwd=work, text=True, capture_output=True, timeout=10)
    folder = [d for d in os.listdir(work) if d.startswith("folder_")]
    return p.stdout, os.path.join(work, folder[0]) if folder else None, p


def read(folder, name):
    with open(os.path.join(folder, name)) as f:
        return f.read()


def text_of(out):
    """Only what the program printed, without its prompts."""
    return re.sub(r"(Enter command:|INPUT ->) ?", "", out)


def test_files():
    out, folder, p = session("STOP")
    check("creates its folder", folder and folder.rsplit("_")[-1].isdigit(), folder)
    same("content.txt starts empty", read(folder, "content.txt"), "")
    same("exit status", p.returncode, 0)
    same("the folder has the pid of a real run", os.path.isdir(folder), True)

    _, folder, _ = session("INPUT", "one", "INPUT", "two", "STOP")
    same("input appends lines", read(folder, "content.txt"), "one\ntwo\n")
    same("log records commands", read(folder, "logs.txt"), "INPUT\nINPUT\nSTOP\n")

    # the two ways a stdio buffer goes wrong when a descriptor is switched
    _, folder, _ = session("INPUT", "one", "STOP")
    check("a prompt never lands in the file", "Enter command" not in read(folder, "content.txt"), read(folder, "content.txt"))
    out, folder, _ = session("INPUT", "a", "PRINT", "INPUT", "b", "PRINT", "STOP")
    same("commands after a read are not lost", read(folder, "content.txt"), "a\nb\n")
    check("reading the file does not eat the next commands", text_of(out).count("a\n") == 2, out)
    out, folder, _ = session("INPUT", "a", "PRINT", "PRINT", "STOP")
    same("a file read to its end can be read again", text_of(out).count("a\n"), 2)

    lines = [f"line{i}" for i in range(1, 6)]
    setup = [x for line in lines for x in ("INPUT", line)]
    body = lambda *cmd: text_of(session(*setup, *cmd, "STOP")[0])  # noqa: E731
    check("print shows all", all(line in body("PRINT") for line in lines))
    got = body("FIRST 2")
    check("first", "line1" in got and "line2" in got and "line3" not in got, got)
    got = body("LAST 2")
    check("last", "line4" in got and "line5" in got and "line3" not in got, got)
    check("first 0 prints nothing", "line" not in body("FIRST 0"))
    check("last 0 prints nothing", "line" not in body("LAST 0"))
    check("first more than there are", all(line in body("FIRST 99") for line in lines))
    check("last more than there are", all(line in body("LAST 99") for line in lines))

    out, _, _ = session("INPUT", "x", "PRINT", "LOG 2", "STOP")
    check("log prints its last lines", "PRINT" in text_of(out) and text_of(out).count("INPUT") <= 2, out)
    _, folder, _ = session("LOG 1", "STOP")
    same("log is not logged", read(folder, "logs.txt"), "STOP\n")

    for bad in ["NOPE", "PRINT x", "INPUT hello", "FIRST", "FIRST -1", "LAST abc", "LOG", "print"]:
        out, folder, _ = session(bad, "STOP")
        check(f"unknown command {bad!r}", "Unknown command." in out, out)
        same(f"{bad!r} is not logged", read(folder, "logs.txt"), "STOP\n")
    out, folder, _ = session("", "   ", "STOP")
    check("a blank line is ignored", "Unknown" not in out, out)
    _, folder, _ = session("INPUT", "", "STOP")
    same("an empty line can be stored", read(folder, "content.txt"), "\n")
    out, folder, p = session("INPUT", "a", stdin="INPUT\na\n")
    same("end of input without STOP exits", p.returncode, 0)
    same("and logs no STOP", read(folder, "logs.txt"), "INPUT\n")
    _, folder, _ = session(stdin="INPUT\r\nabc\r\nSTOP\r\n")
    check("windows line endings work", read(folder, "logs.txt") == "INPUT\nSTOP\n", read(folder, "logs.txt"))
    long_line = "x" * 1000
    _, folder, _ = session("INPUT", long_line, "STOP")
    same("a long line is stored whole", read(folder, "content.txt"), long_line + "\n")


def test_only_printf_and_scanf():
    # the assignment's rule for file contents: these would be a way around it
    src = open(os.path.join(ROOT, "Part-1", "fileManagement.c")).read()
    code = re.sub(r"/\*.*?\*/", "", src, flags=re.S)
    banned = ["fgets", "fputs", "fprintf", "fscanf", "fread", "fwrite", "getchar", "putchar", "puts", "fopen", "read", "write"]
    found = [name for name in banned if re.search(r"\b" + name + r"\s*\(", code)]
    same("part 1 uses only printf and scanf for contents", found, [])


def test_processes():
    work = tempfile.mkdtemp(prefix="pm")
    for b in (PROC, WRITER):
        shutil.copy(b, work)
    p = subprocess.run(["./processManagement"], cwd=work, capture_output=True, text=True, timeout=10)
    out = p.stdout
    # the child of task 3 outlives the parent and still holds the pipe, so run()
    # waits for it, and its line is in the output
    same("exit status", p.returncode, 0)
    check("task 1: the child adds", re.search(r"child pid: \d+, x: 35", out), out)
    check("task 1: the parent subtracts, not seeing it", re.search(r"parent pid: \d+, x: 20", out), out)
    m = re.search(r"parent pid: (\d+) \(compare", out)
    wrote = open(os.path.join(work, "newfile.txt")).read().strip()
    check("task 2: the file holds the parent's pid", m and m.group(1) == wrote, (out, wrote))
    task3 = out[out.index("TASK 3"):]
    parent = re.search(r"^parent pid: (\d+)$", task3, re.M)
    child = re.search(r"child pid: (\d+), parent pid: (\d+)", task3)
    check("task 3: the orphan has a new parent", parent and child and child.group(2) != parent.group(1), task3)
    # stdout is a pipe here, so it is fully buffered. if it were not flushed before
    # each fork, these headings would be printed twice.
    for head in ("TASK 1", "TASK 2", "TASK 3"):
        same(f"{head} is printed once through a pipe", out.count(head), 1)

    p = subprocess.run([os.path.join(work, "processManagement")], cwd=tempfile.mkdtemp(),
                       capture_output=True, text=True, timeout=10)
    check("no writer: a failed exec is reported", "writer failed" in p.stdout and "execvp" in p.stderr, (p.stdout, p.stderr))


def main():
    for name, fn in sorted(globals().items()):
        if name.startswith("test_"):
            fn()
    print(f"\n{passed} passed, {len(failures)} failed" + (f": {failures}" if failures else ""))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
