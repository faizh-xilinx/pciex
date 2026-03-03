#include "pciex.h"
#include "pci_regs.h"
#include <stdio.h>
#include <inttypes.h>

int pci_read_bar_sizes(pci_device_t *dev)
{
    char path[512];
    FILE *fp;
    char line[256];
    int bar_idx = 0;

    snprintf(path, sizeof(path), "%s/resource", dev->sysfs_path);
    fp = fopen(path, "r");
    if (!fp)
        return -1;

    while (fgets(line, sizeof(line), fp) && bar_idx < PCI_NUM_BARS) {
        uint64_t start = 0, end = 0, flags = 0;

        if (sscanf(line, "%" SCNx64 " %" SCNx64 " %" SCNx64, &start, &end, &flags) != 3)
            break;

        if (start != 0 && end >= start) {
            uint64_t size = end - start + 1;
            for (int i = 0; i < dev->num_bars; i++) {
                if (dev->bars[i].addr == start ||
                    (dev->bars[i].addr & 0xFFFFFFFFFFFFFFF0ULL) == (start & 0xFFFFFFFFFFFFFFF0ULL)) {
                    dev->bars[i].size = size;
                    break;
                }
            }
        }
        bar_idx++;
    }

    fclose(fp);
    return 0;
}

void format_bar_size(uint64_t size, char *buf, size_t buflen)
{
    if (size == 0) {
        snprintf(buf, buflen, "unknown");
    } else if (size >= (1ULL << 30)) {
        snprintf(buf, buflen, "%lu GB", (unsigned long)(size >> 30));
    } else if (size >= (1ULL << 20)) {
        snprintf(buf, buflen, "%lu MB", (unsigned long)(size >> 20));
    } else if (size >= (1ULL << 10)) {
        snprintf(buf, buflen, "%lu KB", (unsigned long)(size >> 10));
    } else {
        snprintf(buf, buflen, "%lu B", (unsigned long)size);
    }
}
