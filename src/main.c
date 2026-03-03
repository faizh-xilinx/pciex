#include "pciex.h"
#include "pci_regs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    MODE_LIST,
    MODE_ALL,
    MODE_CAPS,
    MODE_BARS,
    MODE_MSIX,
    MODE_MSIX_TABLE,
    MODE_SRIOV,
    MODE_HEXDUMP,
    MODE_VALIDATE,
    MODE_WATCH,
    MODE_MMIO,
    MODE_WRITE,
    MODE_FLR,
    MODE_SBRESET,
    MODE_RETRAIN,
} run_mode_t;

static void usage(const char *prog)
{
    printf("pciex %s — PCIe Config Space Explorer\n\n", PCIEX_VERSION);
    printf("Usage:\n");
    printf("  %s                                  List all PCI devices\n", prog);
    printf("  %s <BDF>                            Full decode of a device\n", prog);
    printf("  %s -c <BDF>                         Capability tree only\n", prog);
    printf("  %s -b <BDF>                         BAR decode with sizes\n", prog);
    printf("  %s -m <BDF>                         MSI-X detail only\n", prog);
    printf("  %s --msix-table <BDF>               MSI-X vector table dump [root]\n", prog);
    printf("  %s -s <BDF>                         SR-IOV detail only\n", prog);
    printf("  %s --raw <BDF>                      Hex dump of config space\n", prog);
    printf("  %s --validate <BDF>                 Run validation checks\n", prog);
    printf("  %s --watch <BDF> [cap]              Live register poll [root]\n", prog);
    printf("  %s --mmio <BDF> <bar> <off> [len]   Read BAR MMIO region [root]\n", prog);
    printf("  %s --write <BDF> <off> <val> --force  Write config register [root]\n", prog);
    printf("  %s --flr <BDF> --force              Function Level Reset [root]\n", prog);
    printf("  %s --sbreset <BDF> --force          Secondary Bus Reset [root]\n", prog);
    printf("  %s --retrain <BDF> --force          Link retrain [root]\n", prog);
    printf("  %s -h, --help                       Show this help\n", prog);
    printf("\nBDF format: [domain:]bus:dev.func  (e.g., c4:00.0 or 0000:c4:00.0)\n");
    printf("[root] = requires sudo for full output\n");
}

static const pci_cap_t *find_cap(const pci_device_t *dev, uint16_t id, bool extended)
{
    for (int i = 0; i < dev->num_caps; i++) {
        if (dev->caps[i].is_extended == extended && dev->caps[i].id == id)
            return &dev->caps[i];
    }
    return NULL;
}

int main(int argc, char **argv)
{
    run_mode_t mode = MODE_LIST;
    const char *bdf_str = NULL;
    bool force_color = false;
    bool force = false;
    const char *watch_filter = NULL;
    int mmio_bar = 0;
    unsigned long mmio_off = 0;
    unsigned long mmio_len = 256;
    uint16_t write_off = 0;
    uint32_t write_val = 0;

    color_init(force_color);

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--color") == 0) {
            force_color = true;
            color_init(true);
        } else if (strcmp(argv[i], "--force") == 0) {
            force = true;
        } else if (strcmp(argv[i], "-c") == 0) {
            mode = MODE_CAPS;
        } else if (strcmp(argv[i], "-b") == 0) {
            mode = MODE_BARS;
        } else if (strcmp(argv[i], "-m") == 0) {
            mode = MODE_MSIX;
        } else if (strcmp(argv[i], "--msix-table") == 0) {
            mode = MODE_MSIX_TABLE;
        } else if (strcmp(argv[i], "-s") == 0) {
            mode = MODE_SRIOV;
        } else if (strcmp(argv[i], "--raw") == 0) {
            mode = MODE_HEXDUMP;
        } else if (strcmp(argv[i], "--validate") == 0) {
            mode = MODE_VALIDATE;
        } else if (strcmp(argv[i], "--watch") == 0) {
            mode = MODE_WATCH;
        } else if (strcmp(argv[i], "--mmio") == 0) {
            mode = MODE_MMIO;
        } else if (strcmp(argv[i], "--write") == 0) {
            mode = MODE_WRITE;
        } else if (strcmp(argv[i], "--flr") == 0) {
            mode = MODE_FLR;
        } else if (strcmp(argv[i], "--sbreset") == 0) {
            mode = MODE_SBRESET;
        } else if (strcmp(argv[i], "--retrain") == 0) {
            mode = MODE_RETRAIN;
        } else if (argv[i][0] != '-') {
            if (!bdf_str) {
                bdf_str = argv[i];
                if (mode == MODE_LIST)
                    mode = MODE_ALL;
            } else if (mode == MODE_MMIO) {
                if (mmio_bar == 0 && mmio_off == 0) {
                    mmio_bar = (int)strtol(argv[i], NULL, 0);
                } else if (mmio_off == 0) {
                    mmio_off = strtoul(argv[i], NULL, 0);
                } else {
                    mmio_len = strtoul(argv[i], NULL, 0);
                }
            } else if (mode == MODE_WRITE) {
                if (write_off == 0 && write_val == 0) {
                    write_off = (uint16_t)strtoul(argv[i], NULL, 0);
                } else {
                    write_val = (uint32_t)strtoul(argv[i], NULL, 0);
                }
            } else if (mode == MODE_WATCH) {
                watch_filter = argv[i];
            }
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            usage(argv[0]);
            return 1;
        }
    }

    if (mode == MODE_LIST) {
        pci_device_t *devs;
        int count;

        if (pci_enum_devices(&devs, &count) < 0)
            return 1;

        for (int i = 0; i < count; i++)
            show_device_summary(&devs[i]);

        pci_free_devices(devs, count);
        return 0;
    }

    if (!bdf_str) {
        fprintf(stderr, "Error: BDF required for this mode\n\n");
        usage(argv[0]);
        return 1;
    }

    pci_bdf_t bdf;
    if (pci_parse_bdf(bdf_str, &bdf) < 0) {
        fprintf(stderr, "Invalid BDF: %s\n", bdf_str);
        return 1;
    }

    pci_device_t dev;
    if (pci_open_device(&dev, &bdf) < 0) {
        fprintf(stderr, "Cannot open device %04x:%02x:%02x.%x\n",
                bdf.domain, bdf.bus, bdf.dev, bdf.func);
        return 1;
    }

    pci_read_bar_sizes(&dev);

    switch (mode) {
    case MODE_ALL:
        show_all(&dev);
        show_validation(&dev);
        break;

    case MODE_CAPS:
        show_device_summary(&dev);
        show_caps(&dev);
        for (int i = 0; i < dev.num_caps; i++) {
            const pci_cap_t *cap = &dev.caps[i];
            const char *name = cap->is_extended ?
                pci_ext_cap_name(cap->id) : pci_cap_name(cap->id);
            printf("\n%s── %s @ 0x%03x ──%s\n",
                   clr(CLR_BOLD), name, cap->offset, clr(CLR_RESET));
            show_cap_detail(&dev, cap);
        }
        break;

    case MODE_BARS:
        show_device_summary(&dev);
        show_bars(&dev);
        break;

    case MODE_MSIX: {
        show_device_summary(&dev);
        const pci_cap_t *cap = find_cap(&dev, PCI_CAP_ID_MSIX, false);
        if (cap) {
            printf("\n%s── MSI-X ──%s\n", clr(CLR_BOLD), clr(CLR_RESET));
            decode_msix_cap(&dev, cap->offset);
        } else {
            printf("  No MSI-X capability found\n");
        }
        break;
    }

    case MODE_MSIX_TABLE:
        show_device_summary(&dev);
        show_msix_table(&dev);
        break;

    case MODE_SRIOV: {
        show_device_summary(&dev);
        const pci_cap_t *cap = find_cap(&dev, PCI_EXT_CAP_ID_SRIOV, true);
        if (cap) {
            printf("\n%s── SR-IOV ──%s\n", clr(CLR_BOLD), clr(CLR_RESET));
            decode_sriov_cap(&dev, cap->offset);
        } else {
            printf("  No SR-IOV capability found\n");
        }
        break;
    }

    case MODE_HEXDUMP:
        show_device_summary(&dev);
        show_hexdump(&dev);
        break;

    case MODE_VALIDATE:
        show_device_summary(&dev);
        show_validation(&dev);
        break;

    case MODE_WATCH:
        show_device_summary(&dev);
        if (!require_root("Watch mode"))
            return 1;
        watch_device(&dev, watch_filter, 2);
        break;

    case MODE_MMIO:
        show_device_summary(&dev);
        show_mmio_dump(&dev, mmio_bar, mmio_off, mmio_len);
        break;

    case MODE_WRITE:
        show_device_summary(&dev);
        cmd_write_config(&dev, write_off, write_val, 4, force);
        break;

    case MODE_FLR:
        show_device_summary(&dev);
        cmd_flr(&dev, force);
        break;

    case MODE_SBRESET:
        show_device_summary(&dev);
        cmd_sbreset(&dev, force);
        break;

    case MODE_RETRAIN:
        show_device_summary(&dev);
        cmd_retrain(&dev, force);
        break;

    default:
        break;
    }

    return 0;
}
