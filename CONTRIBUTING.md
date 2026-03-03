# Contributing to pciex

Thanks for your interest in contributing! Here's how to get started.

## Getting Started

1. Fork the repository
2. Clone your fork: `git clone git@github.com:<your-username>/pciex.git`
3. Create a branch: `git checkout -b my-feature`
4. Make your changes
5. Build and test: `make clean && make`
6. Commit: `git commit -m "Add feature X"`
7. Push: `git push origin my-feature`
8. Open a Pull Request

## Build Requirements

- GCC or Clang with C11 support
- Linux with sysfs (`/sys/bus/pci/devices/`)
- GNU Make

## Code Style

- C11 standard
- 8-space tabs for indentation (kernel style)
- Function names: `module_verb_noun()` (e.g., `pci_parse_header()`)
- All warnings enabled: `-Wall -Wextra -Werror`
- No external dependencies — this is intentional

## Adding a New Capability Decoder

This is the easiest way to contribute. Here's how:

1. Add register defines to `include/pci_regs.h`
2. Add a `decode_xxx_cap()` function in `src/decode.c`
3. Add the function prototype to `include/pciex.h`
4. Wire it into `show_cap_detail()` in `src/display.c`
5. If it's a new extended cap ID, add the name to `pci_ext_cap_name()` in `src/names.c`

## Reporting Bugs

Please include:
- Output of `pciex --version`
- Output of `uname -r`
- The BDF of the device that shows the issue
- Expected vs actual output

## Feature Requests

Open an issue with the `enhancement` label. Describe the use case — why do you need this feature and how would you use it?
