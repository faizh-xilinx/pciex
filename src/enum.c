#include "pciex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

int pci_parse_bdf(const char *str, pci_bdf_t *bdf)
{
    unsigned int domain = 0, bus = 0, dev = 0, func = 0;

    /* Try domain:bus:dev.func */
    if (sscanf(str, "%x:%x:%x.%x", &domain, &bus, &dev, &func) == 4)
        goto ok;
    /* Try bus:dev.func (assume domain 0) */
    domain = 0;
    if (sscanf(str, "%x:%x.%x", &bus, &dev, &func) == 3)
        goto ok;
    /* Try bus:dev (assume func 0) */
    if (sscanf(str, "%x:%x", &bus, &dev) == 2) {
        func = 0;
        goto ok;
    }
    return -1;

ok:
    bdf->domain = (uint16_t)domain;
    bdf->bus    = (uint8_t)bus;
    bdf->dev    = (uint8_t)dev;
    bdf->func   = (uint8_t)func;
    return 0;
}

int pci_open_device(pci_device_t *dev, const pci_bdf_t *bdf)
{
    memset(dev, 0, sizeof(*dev));
    dev->bdf = *bdf;

    snprintf(dev->sysfs_path, sizeof(dev->sysfs_path),
             SYSFS_PCI_PATH "/%04x:%02x:%02x.%x",
             bdf->domain, bdf->bus, bdf->dev, bdf->func);

    if (pci_cfg_read(dev) < 0)
        return -1;

    pci_parse_header(dev);
    pci_parse_bars(dev);
    pci_parse_caps(dev);
    return 0;
}

static int bdf_compare(const void *a, const void *b)
{
    const pci_device_t *da = a, *db = b;

    if (da->bdf.domain != db->bdf.domain)
        return (int)da->bdf.domain - (int)db->bdf.domain;
    if (da->bdf.bus != db->bdf.bus)
        return (int)da->bdf.bus - (int)db->bdf.bus;
    if (da->bdf.dev != db->bdf.dev)
        return (int)da->bdf.dev - (int)db->bdf.dev;
    return (int)da->bdf.func - (int)db->bdf.func;
}

int pci_enum_devices(pci_device_t **out_devs, int *out_count)
{
    DIR *dir;
    struct dirent *ent;
    pci_device_t *devs = NULL;
    int count = 0, capacity = 0;

    dir = opendir(SYSFS_PCI_PATH);
    if (!dir) {
        perror("Cannot open " SYSFS_PCI_PATH);
        return -1;
    }

    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_name[0] == '.')
            continue;

        pci_bdf_t bdf;
        if (pci_parse_bdf(ent->d_name, &bdf) < 0)
            continue;

        if (count >= capacity) {
            capacity = capacity ? capacity * 2 : 32;
            devs = realloc(devs, capacity * sizeof(*devs));
            if (!devs) {
                closedir(dir);
                return -1;
            }
        }

        if (pci_open_device(&devs[count], &bdf) == 0)
            count++;
    }

    closedir(dir);
    qsort(devs, count, sizeof(*devs), bdf_compare);
    *out_devs = devs;
    *out_count = count;
    return 0;
}

void pci_free_devices(pci_device_t *devs, int count)
{
    (void)count;
    free(devs);
}
