# pciex — PCIe Config Space Explorer

A fast, colorful, zero-dependency command-line utility to read and decode PCI/PCIe configuration space on Linux. Built for hardware engineers, driver developers, and anyone who needs more than `lspci -vvv`.

## Why pciex?

| | `lspci -vvv` | `pciex` |
|---|---|---|
| Colored output | No | Yes (auto-detects TTY) |
| Link downgrade warnings | No | Yes (red highlight) |
| MSI-X table/PBA BIR decode | Basic | Detailed with BAR mapping |
| SR-IOV VF BAR breakdown | Minimal | Full with stride/offset |
| PCIe 6.0 caps (IDE, DOE) | No | Yes |
| AER error flag decode | Partial | Full with color highlights |
| Hex dump with coloring | No | Yes (00=dim, FF=red) |
| Dependencies | libpci, pci.ids | None |
| Config space access | ioctl / libpci | sysfs (no root for basic read) |
| BAR size detection | Yes | Yes (from sysfs, no writes) |
| MSI-X vector table dump | No | Yes [root] |
| MMIO BAR read | No | Yes [root] |
| Validation warnings | No | Yes (link downgrade, AER, BAR overlap) |
| Live register watch | No | Yes [root] |
| Config space write | No (setpci) | Yes [root+force] |
| Device reset (FLR/SBR) | No | Yes [root+force] |
| Link retrain | No | Yes [root+force] |

## Quick Start

```bash
git clone https://github.com/faizh-xilinx/pciex.git
cd pciex
make
./pciex              # list all devices
./pciex c4:00.0      # full decode of a device
```

## Installation

### Build from source

```bash
git clone https://github.com/faizh-xilinx/pciex.git
cd pciex
make
```

### Install system-wide

```bash
sudo make install              # installs to /usr/local/bin
```

### Uninstall

```bash
sudo make uninstall
```

### Requirements

- Linux with sysfs (`/sys/bus/pci/devices/`)
- GCC or Clang with C11 support
- GNU Make

No external libraries. No pci.ids database needed. Just C and a kernel.

## Usage

### List all PCI devices

```
$ pciex

0000:00:00.0  [8086:1237]  Host Bridge  (rev 02)
0000:00:01.0  [8086:7000]  ISA Bridge  (rev 00)
0000:00:02.0  [1013:00b8]  VGA Controller  (rev 00)
0000:c4:00.0  [10ee:50a4]  Ethernet Controller  (rev 01)
0000:c4:00.1  [10ee:50a4]  Ethernet Controller  (rev 01)
```

### Full decode of a device

```
$ pciex c4:00.0

0000:c4:00.0  [10ee:50a4]  Ethernet Controller  (rev 01)

── Header ──
  Vendor:      0x10ee
  Device:      0x50a4
  Class:       Ethernet Controller [02:00:00]
  Header Type: Normal (multi-function)

  Command:  0x0007  [ I/O Memory BusMaster ]
  Status:   0x0010  [ CapList ]

── BARs ──
  BAR0: 0x0000f0000000  Mem  64-bit prefetchable
  BAR2: 0x0000f3000000  Mem  32-bit non-prefetchable
  BAR4: 0x0000f3010000  Mem  32-bit non-prefetchable

── Capabilities ──
  [01] @ 0x40  Power Management
  [11] @ 0x60  MSI-X
  [10] @ 0x70  PCI Express

── Extended Capabilities ──
  [0001] @ 0x100 v1  Advanced Error Reporting
  [0003] @ 0x140 v1  Device Serial Number
  [0010] @ 0x160 v1  SR-IOV
  [000f] @ 0x1a0 v1  Address Translation Services
  [001b] @ 0x200 v1  Process Address Space ID
  [0013] @ 0x220 v1  Page Request Interface
  [0030] @ 0x300 v1  Integrity & Data Encryption
  [002e] @ 0x380 v1  Data Object Exchange

── MSI-X @ 0x60 ──
    Enable+  ENABLED  Masked-  Table Size: 8
    Table: BAR=2  offset=0x00000
    PBA:   BAR=2  offset=0x00ff8

── PCI Express @ 0x70 ──
    Version: 2  Type: Endpoint
    DevCap:  MaxPayload 512 bytes  PhantFunc 0  ExtTag+
    DevCtl:  CorrErr+ NonFatalErr+ FatalErr+ UnsupReq+
             MaxPayload 256 bytes  MaxReadReq 512 bytes
    LnkCap:  16.0 GT/s x8
    LnkSta:  16.0 GT/s x8

── SR-IOV @ 0x160 ──
    Ctrl:     VFEnable+  ENABLED  VF_MSE+  ARI+
    TotalVFs: 4  NumVFs: 4
    VF Offset: 6  VF Stride: 1  VF DeviceID: 0x50a5
    VF BAR0:  0xf4000000  64-bit prefetchable (consumes next BAR)
    VF BAR2:  0xf5000000  32-bit non-prefetchable
    VF BAR4:  0xf5100000  32-bit non-prefetchable
```

### Capability tree only

```
$ pciex -c c4:00.0
```

Shows only the capability list with detailed decodes — no header or BARs.

### BAR decode with sizes

```
$ pciex -b c4:00.0

── BARs ──
  BAR0: 0x0000f0000000  Mem  64-bit prefetchable  [size=64 MB]
  BAR2: 0x0000f3000000  Mem  32-bit non-prefetchable  [size=64 KB]
  BAR4: 0x0000f3010000  Mem  32-bit non-prefetchable  [size=64 KB]
```

Shows BAR addresses, types, and sizes (from sysfs resource file).

### MSI-X detail

```
$ pciex -m c4:00.0

── MSI-X ──
    Enable+  ENABLED  Masked-  Table Size: 8
    Table: BAR=2  offset=0x00000
    PBA:   BAR=2  offset=0x00ff8
```

Quickly see which BAR backs the MSI-X table and PBA, how many vectors are allocated, and whether MSI-X is enabled.

### SR-IOV detail

```
$ pciex -s c4:00.0

── SR-IOV ──
    Ctrl:     VFEnable+  ENABLED  VF_MSE+  ARI+
    TotalVFs: 4  NumVFs: 4
    VF Offset: 6  VF Stride: 1  VF DeviceID: 0x50a5
    VF BAR0:  0xf4000000  64-bit prefetchable
    VF BAR2:  0xf5000000  32-bit non-prefetchable
    VF BAR4:  0xf5100000  32-bit non-prefetchable
```

See VF count, VF BAR layout, stride, and enable status at a glance.

### Validation

```
$ pciex --validate c4:00.0

WARN: Link downgraded (cap 16.0 GT/s x8, current 8.0 GT/s x4)
INFO: AER uncorrectable errors pending
OK: No BAR overlap detected
```

Runs automated checks for link downgrade, AER errors, and BAR overlap.

### MSI-X vector table

```
$ sudo pciex --msix-table c4:00.0

── MSI-X Vector Table ──
  [0] addr=0xf3000000  data=0x00000001  ctrl=0x0000
  [1] addr=0xf3000010  data=0x00000002  ctrl=0x0000
  ...
```

Dumps the MSI-X vector table and PBA from MMIO. Requires root.

### Live watch

```
$ sudo pciex --watch c4:00.0 AER
```

Polls the specified register(s) in real-time and shows diffs when values change. Requires root.

### MMIO read

```
$ sudo pciex --mmio c4:00.0 0 0x0 64
```

Reads from a BAR: `--mmio <BDF> <BAR_index> <offset> <bytes>`. Requires root.

### Config write

```
$ sudo pciex --write c4:00.0 0x04 0x0007 --force
```

Writes to config space. Requires root and `--force` with interactive confirmation.

### Device reset

```
$ sudo pciex --flr c4:00.0 --force
```

Performs Function Level Reset (FLR) or Secondary Bus Reset (SBR). Requires root and `--force`.

### Link retrain

```
$ sudo pciex --retrain c4:00.0 --force
```

Triggers link retraining. Requires root and `--force`.

### Hex dump of config space

```
$ pciex --raw c4:00.0
```

Color-coded hex dump of the full configuration space:
- `00` bytes are dimmed (easy to skip visually)
- `FF` bytes are red (often indicates unconfigured/absent regions)
- All other bytes in normal color

### BDF format

pciex accepts BDF (Bus:Device.Function) in these formats:

| Format | Example | Meaning |
|---|---|---|
| `domain:bus:dev.func` | `0000:c4:00.0` | Full BDF with domain |
| `bus:dev.func` | `c4:00.0` | Domain defaults to 0000 |
| `bus:dev` | `c4:00` | Function defaults to 0 |

### Root Access

pciex reads from sysfs (`/sys/bus/pci/devices/*/config`).

**Without root:** You get 64-byte config space (header, BARs, standard caps), BAR sizes from the sysfs resource file, and basic decode. No writes, no MMIO, no extended caps.

**With root:** Full 4096-byte extended config space (all extended caps), MSI-X table dump, MMIO read, live watch, config write, device reset, and link retrain.

**Destructive operations** (config write, FLR, bus reset, link retrain) require both root and the `--force` flag, with interactive confirmation before proceeding.

## Capabilities Decoded (35 decoders)

### Standard Capabilities

| ID | Name | Decode |
|---|---|---|
| 0x01 | Power Management | Power state, D1/D2 support, PME enable/status |
| 0x03 | Vital Product Data | Address, flag, data register |
| 0x05 | MSI | Enable, vectors, 32/64-bit address, data |
| 0x07 | PCI-X | Command, status, bus/dev/func, 64-bit, 133MHz |
| 0x09 | Vendor-Specific | Length, raw hex data dump |
| 0x0D | Bridge Subsystem VID | Subsystem vendor and device ID |
| 0x10 | PCI Express | Type, link speed/width, MPS, MRRS, downgrade warning |
| 0x11 | MSI-X | Enable, table size, table/PBA BAR and offset |
| 0x13 | Advanced Features | FLR support, TP status |

### Extended Capabilities

| ID | Name | Decode |
|---|---|---|
| 0x0001 | AER | Uncorrectable/correctable error status with flag names |
| 0x0002 | Virtual Channel | VC count, arbitration, port status |
| 0x0003 | Device Serial Number | Formatted serial number |
| 0x0004 | Power Budgeting | Power data, capability |
| 0x000B | Vendor-Specific Ext | VSEC ID, revision, length |
| 0x000D | ACS | Source validation, translation blocking, redirect, egress |
| 0x000E | ARI | MFVC, ACS, next function, function group |
| 0x000F | ATS | Enable, invalidate queue depth, STU |
| 0x0010 | SR-IOV | VF enable, count, stride, offset, VF BARs |
| 0x0012 | Multicast | Max/num groups, window size, enable |
| 0x0013 | PRI | Enable, reset, status flags |
| 0x0015 | Resizable BAR | Supported sizes, current size per BAR |
| 0x0017 | TPH | ST table location/size, extended requester |
| 0x0018 | LTR | Max snoop/no-snoop latency with scale |
| 0x0019 | Secondary PCIe | Link equalization, lane error status |
| 0x001B | PASID | Enable, max width, exec/priv |
| 0x001D | DPC | Trigger policy, containment status, source ID |
| 0x001E | L1 PM Substates | PCI-PM/ASPM L1.1/L1.2 support and enable |
| 0x001F | PTM | Requester/responder/root, granularity, enable |
| 0x0023 | DVSEC | Vendor ID, revision, length, DVSEC ID |
| 0x0024 | VF Resizable BAR | Same as Resizable BAR for VFs |
| 0x0025 | Data Link Feature | Scaled flow control, remote support, valid |
| 0x0026 | 16.0 GT/s Phy Layer | Equalization status (phase 1/2/3) |
| 0x0027 | Lane Margining | Margining/software ready status |
| 0x002A | 32.0 GT/s Phy Layer | Modified TS, equalization status |
| 0x002E | DOE | Capability, control, status (busy/error/ready) |
| 0x0030 | IDE | Link/selective streams, TEE-limited, flow-through |

All other capabilities are listed with their ID, offset, and name.

## Project Structure

```
pciex/
├── Makefile              Build system
├── LICENSE               MIT License
├── README.md             This file
├── CONTRIBUTING.md       How to contribute
├── .gitignore
├── include/
│   ├── pciex.h           Data structures and API
│   └── pci_regs.h        PCI register definitions (standard + extended)
└── src/
    ├── main.c            CLI argument parsing and run modes
    ├── config.c           sysfs config space reader
    ├── enum.c             BDF parser and device enumeration
    ├── parse.c            Header, BAR, and capability linked-list walker
    ├── names.c            Human-readable names for classes, caps, speeds
    ├── decode.c           Capability decoders
    ├── display.c          Colored terminal output and formatting
    ├── color.c            ANSI color abstraction (auto-detects TTY)
    ├── privilege.c       Root detection, sudo hints, --force confirmation
    ├── barsize.c          BAR size detection from sysfs resource file
    ├── mmio.c             MMIO BAR access and MSI-X table dump
    ├── validate.c         Automated validation warnings
    ├── watch.c            Live register polling with diff
    └── write.c            Config write, FLR, bus reset, link retrain
```

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

The easiest way to contribute is adding a new capability decoder. Each decoder is a single function in `src/decode.c` — see the existing decoders for the pattern.

### Good first issues

- Add `--diff` mode to compare two devices
- Add pci.ids vendor/device name lookup
- Improve class/subclass name coverage in `src/names.c`
- Add `--json` output mode

## Roadmap

- [ ] `--json` output for scripting and CI pipelines
- [ ] `--diff <BDF1> <BDF2>` to compare two devices side-by-side
- [ ] pci.ids database integration for vendor/device names
- [ ] `man` page
- [ ] Packaging for apt, dnf, brew, AUR

## License

MIT License. See [LICENSE](LICENSE).
