# SYSC4001_A2_P3
# Mithushan Ravichandramohan 101262467
# Gabriel Bugarija 101262776

This repository contains a simulator for interrupt handling and simple process management for the assignment.

Key Files:
- `interrupts.cpp` - main simulator implementation
- `interrupts.hpp` - helper functions, data structures and utilities
- `trace.txt`, `vector_table.txt`, `device_table.txt`, `external_files.txt` - example input files
- `build.sh` - build script (for Unix / WSL / Git Bash)

Build & Run instructions :

1. From the project root (where `interrupts.cpp` is located):

```powershell
# create bin folder (if not present)
New-Item -ItemType Directory -Path bin -Force

# compile with g++ (MinGW/MSYS2 installed and on PATH)
g++ -g -O0 -I . -o .\bin\interrupts.exe .\interrupts.cpp

# run the program (order of args matters):
.\bin\interrupts.exe .\trace.txt .\vector_table.txt .\device_table.txt .\external_files.txt

# view outputs
Get-Content .\execution.txt
Get-Content .\system_status.txt
```

Quick run using WSL / Bash:

```bash
# run build script
bash build.sh

# run binary produced by build.sh
./bin/interrupts ./trace.txt ./vector_table.txt ./device_table.txt ./external_files.txt

# view outputs
cat execution.txt


## Contents

- `interrupts.cpp` — main simulator logic (parses traces, simulates interrupts, process management)
- `interrupts.hpp` — helper structs, memory partitions, parsing utilities and I/O helpers
- `trace.txt` — default example input trace
- `vector_table.txt`, `device_table.txt`, `external_files.txt` — input tables used by the simulator
- `program1.txt`, `program2.txt` — example external program traces used for `EXEC` testing
- `trace_exec.txt`, `trace_fork.txt` — additional traces used by the test runner
- `tests.ps1` — PowerShell test runner (builds and runs tests, saving outputs per test)

## Prerequisites

- A C++ compiler supporting C++11 or newer (g++ recommended). On Windows use MSYS2/MinGW-w64 or WSL.
- PowerShell (for `tests.ps1`) or a POSIX shell for `build.sh`.

## Build & run (PowerShell)

From the repository root:

```powershell
# create bin folder (if not present)
New-Item -ItemType Directory -Path bin -Force

# compile with g++ (MSYS2 / MinGW-w64 on PATH)
g++ -g -O0 -I . -o .\bin\interrupts.exe .\interrupts.cpp

# run the program (order of args matters):
.\bin\interrupts.exe .\trace.txt .\vector_table.txt .\device_table.txt .\external_files.txt

# view outputs
Get-Content .\execution.txt
Get-Content .\system_status.txt
```

## Build & run (WSL / Bash)

```bash
# build
bash build.sh

# run
./bin/interrupts ./trace.txt ./vector_table.txt ./device_table.txt ./external_files.txt

# view outputs
cat execution.txt
cat system_status.txt
```

## Inputs and outputs

Inputs (command line order):
1. Trace file — a line-based trace of activities (e.g. `CPU, 6`, `SYSCALL, 4`, `EXEC program1, 3`, `FORK, 2`, `IF_CHILD, 0`, `IF_PARENT, 0`, `ENDIF, 0`, ...).
2. Vector table — list of ISR addresses used by the interrupt boilerplate.
3. Device delays/table — per-device delay values (one int per line).
4. External files table — mapping `program_name, size` for external programs.

Outputs (written to current working directory):
- `execution.txt` — the step-by-step simulated execution log (timestamps, actions)
- `system_status.txt` — human-readable snapshots of the PCB/memory state emitted at certain events (FORK, EXEC, END_IO wake)

## Tests

Run the included PowerShell test runner which compiles and runs three example traces and saves outputs per test:

```powershell
.\tests.ps1

# results are saved as
# execution_original.txt, system_status_original.txt
# execution_exec.txt, system_status_exec.txt
# execution_fork.txt, system_status_fork.txt
```

Use these saved files to inspect and verify behavior across scenarios.

## Part III — Implementation notes & assumptions

- END_IO wake logic
	- When an `END_IO` interrupt completes, the simulator checks the `wait_queue`.
	- If a process is waiting (first-in), it is woken: a `system_status` snapshot is emitted and the scheduler is logged as selecting that process to run. The woken PCB becomes the current process for subsequent trace processing.
	- If no process is waiting, the scheduler is still invoked and the event is logged.

- FORK behavior and memory semantics
	- On `FORK`, the simulator creates a child PCB with a unique PID. The parent is pushed onto the `wait_queue` (simulates parent blocking) and the child is given CPU.
	- The child attempts to obtain its own memory partition by calling `allocate_memory(&child)`. If allocation succeeds, the partition number is set and the allocation is logged in `system_status`. If allocation fails (no partition big enough / empty), the failure is logged and `child.partition_number` remains -1.
	- This choice implements separate memory allocation per child. If you prefer the child to share the parent's partition instead, the code can be adjusted accordingly.

- EXEC recursion and external programs
	- `EXEC <program>` looks up `<program>` in `external_files.txt` to obtain size, simulates loading time (15 ms per MB), frees the current partition (if any), and allocates a new partition for the program.
	- If `<program>.txt` exists, it is read as a trace and executed recursively by `simulate_trace`. Sample `program1.txt` and `program2.txt` are provided.

- wait_queue scope
	- Currently, `wait_queue` is passed by value into recursive calls (a shallow copy). This keeps wakeups local to the simulation context. If you want a global/shared wait queue across nested executions, we should change function signatures to pass it by reference. I can update this if required by the assignment.


