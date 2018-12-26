# Functionally Reduced And-Inverter Graph (FRAIG)
- This is the final project of Data Structure and Programming
  - Term: Fall 2015
  - Lecturer: [Prof. Chung-Yang (Ric) Huang](http://cc.ee.ntu.edu.tw/~ric/).
- In this project, I implemented a special circuit representation, FRAIG, from a circuit description file.
- The program performs the following processes:
  - Parse a circuit description file in the AIGER format.
  - Sweep out the gates that cannot be reached from primary outputs (excluding primary inputs). After this operation, all the gates that are originally “defined-but-not-used” will be deleted.
  - Perform trivial circuit optimizations without altering the functionality, such as replacing a always-inverse fan-ins of an AND gate by a constant zero.
  - Perform structural hash to merge the structurally equivalent signals (i.e. replace a gate with its functionally equivalent one) by comparing their gate types and permuting their inputs.
  - Simulate boolean logic to group potentially equivalent gates into functionally equivalent candidate (FEC) pair.
  - Use a boolean satisfiability solver to formally prove or disprove FEC pair and merge equivalent gates.
- My program ranks top 5% among more than a hundred of students.

## Requirements
Due to some old libraries that are compiled without the flag `-fPIC`, compilation must be done by gcc/g++ version that is older than 4.8.5.

Compilation is tested on gcc/g++ 4.8.5.

## Specification
`spec/FraigProject.pdf`

## File Descriptions
- `include/*`: Satisfiability solver header files (provided by lecturer).
- `lib/*`: Libraries that need to be linked (provided by lecturer).
- `src/*`: Source files (some written by me, others provided by lecturer)
- `tests.fraig`: Testcases and Dofiles.
- Makefile: GNU makefile.

## Compilation
Type `make` in this directory, then a binary file named `fraig` will be compiled.

Sometimes you may need to type "some" `make clean`s, then "some" `make`s.

---

Compilation flags are declared in `src/Makefile.in`.

## Usage
There are many commands, so please check the specification.
