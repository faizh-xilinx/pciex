#include "pciex.h"
#include <stdio.h>
#include <string.h>

int pci_cfg_read(pci_device_t *dev)
{
    char path[512];
    FILE *fp;
    size_t n;

    snprintf(path, sizeof(path), "%s/config", dev->sysfs_path);
    fp = fopen(path, "rb");
    if (!fp) {
        fprintf(stderr, "Cannot open %s\n", path);
        return -1;
    }

    memset(dev->cfg, 0, sizeof(dev->cfg));
    n = fread(dev->cfg, 1, sizeof(dev->cfg), fp);
    fclose(fp);

    if (n < 64) {
        fprintf(stderr, "Short read from %s: %zu bytes\n", path, n);
        return -1;
    }

    dev->cfg_size = n;
    return 0;
}

uint8_t cfg_read8(const pci_device_t *dev, uint16_t off)
{
    if (off >= dev->cfg_size)
        return 0xFF;
    return dev->cfg[off];
}

uint16_t cfg_read16(const pci_device_t *dev, uint16_t off)
{
    if ((size_t)(off + 1) >= dev->cfg_size)
        return 0xFFFF;
    return (uint16_t)dev->cfg[off] | ((uint16_t)dev->cfg[off + 1] << 8);
}

uint32_t cfg_read32(const pci_device_t *dev, uint16_t off)
{
    if ((size_t)(off + 3) >= dev->cfg_size)
        return 0xFFFFFFFF;
    return (uint32_t)dev->cfg[off]
         | ((uint32_t)dev->cfg[off + 1] << 8)
         | ((uint32_t)dev->cfg[off + 2] << 16)
         | ((uint32_t)dev->cfg[off + 3] << 24);
}
