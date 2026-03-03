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
    MODE_SRIOV,
    MODE_HEXDUMP,
} run_mode_t;

static void usage(const char *prog)
{
    printf("pciex %s — PCIe Config Space Explorer\n\n", PCIEX_VERSION);
    printf("Usage:\n");
    printf("  %s                   List all PCI devices\n", prog);
    printf("  %s <BDF>             Full decode of a device\n", prog);
    printf("  %s -c <BDF>          Capability tree only\n", prog);
    printf("  %s -b <BDF>          BAR decode only\n", prog);
    printf("  %s -m <BDF>          MSI-X detail only\n", prog);
    printf("  %s -s <BDF>          SR-IOV detail only\n", prog);
    printf("  %s --raw <BDF>       Hex dump of config space\n", prog);
    printf("  %s -h, --help        Show this help\n", prog);
    printf("\nBDF format: [domain:]bus:dev.func  (e.g., c4:00.0 or 0000:c4:00.0)\n");
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

    color_init(force_color);

    /* Parse arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--color") == 0) {
            force_color = true;
            color_init(true);
        } else if (strcmp(argv[i], "-c") == 0) {
            mode = MODE_CAPS;
        } else if (strcmp(argv[i], "-b") == 0) {
            mode = MODE_BARS;
        } else if (strcmp(argv[i], "-m") == 0) {
            mode = MODE_MSIX;
        } else if (strcmp(argv[i], "-s") == 0) {
            mode = MODE_SRIOV;
        } else if (strcmp(argv[i], "--raw") == 0) {
            mode = MODE_HEXDUMP;
        } else if (argv[i][0] != '-') {
            bdf_str = argv[i];
            if (mode == MODE_LIST)
                mode = MODE_ALL;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            usage(argv[0]);
            return 1;
        }
    }

    /* List mode — no BDF given */
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

    /* Single device mode */
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

    switch (mode) {
    case MODE_ALL:
        show_all(&dev);
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

    default:
        break;
    }

    return 0;
}
