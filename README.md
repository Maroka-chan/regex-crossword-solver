# Regex Crossword Solver Written in C

The purpose of this project was to solve simple puzzles such as the one seen below.
The program can solve larger puzzles, but it has quite a few [limitations](#limitations).

![regexp-crossword](https://blukat.me/assets/2016/01/crossword.png)

# How to run

## Build

```bash
$ make
```

## Run

```bash
$ ./main
2           # Size of the table (both width and height)
HE|LL|O+    # size*2 lines (first half are rows regexp's, second half are column regexp's)
[PLEASE]+
[^SPEAK]+
EP|IP|EF
```

### Output

```bash
HE
LP
```



# Limitations
- No support for four-sided puzzles
- Can currently only parse square puzzles
- Currently only tries to solve puzzles using capital letters
- Only parses a limited subset of regex features
  - Does not support any character (.), character escapes, ranges etc.

# Problems
- Alternation does not work when inside of parentheses

# Resources Used
https://swtch.com/~rsc/regexp/regexp1.html

https://github.com/hermanschaaf/regex-crossword-solver
