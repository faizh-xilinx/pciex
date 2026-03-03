#define _POSIX_C_SOURCE 199309L

#include "pciex.h"
#include "pci_regs.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define PCI_BRIDGE_CONTROL      0x3E
#define PCI_BRIDGE_CTL_BUS_RESET  (1 << 6)
#define PCI_EXP_DEVCAP_FLR     (1U << 28)
#define PCI_EXP_DEVCTL_BCR_FLR (1 << 15)
#define PCI_EXP_LNKCTL_RETRAIN (1 << 5)

static const pci_cap_t *find_pcie_cap(const pci_device_t *dev)
{
    for (int i = 0; i < dev->num_caps; i++) {
        const pci_cap_t *cap = &dev->caps[i];
        if (cap->id == PCI_CAP_ID_EXP && !cap->is_extended)
            return cap;
    }
    return NULL;
}

int pci_cfg_write32(pci_device_t *dev, uint16_t off, uint32_t val)
{
    char path[512];
    FILE *fp;
    uint32_t le = val;

    snprintf(path, sizeof(path), "%s/config", dev->sysfs_path);
    fp = fopen(path, "r+b");
    if (!fp)
        return -1;

    if (fseek(fp, (long)off, SEEK_SET) != 0) {
        fclose(fp);
        return -1;
    }
    if (fwrite(&le, 4, 1, fp) != 1) {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    return 0;
}

int pci_cfg_write16(pci_device_t *dev, uint16_t off, uint16_t val)
{
    char path[512];
    FILE *fp;
    uint16_t le = val;

    snprintf(path, sizeof(path), "%s/config", dev->sysfs_path);
    fp = fopen(path, "r+b");
    if (!fp)
        return -1;

    if (fseek(fp, (long)off, SEEK_SET) != 0) {
        fclose(fp);
        return -1;
    }
    if (fwrite(&le, 2, 1, fp) != 1) {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    return 0;
}

int pci_cfg_write8(pci_device_t *dev, uint16_t off, uint8_t val)
{
    char path[512];
    FILE *fp;

    snprintf(path, sizeof(path), "%s/config", dev->sysfs_path);
    fp = fopen(path, "r+b");
    if (!fp)
        return -1;

    if (fseek(fp, (long)off, SEEK_SET) != 0) {
        fclose(fp);
        return -1;
    }
    if (fwrite(&val, 1, 1, fp) != 1) {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    return 0;
}

void cmd_write_config(pci_device_t *dev, uint16_t offset, uint32_t value,
                      int width, bool force)
{
    if (!require_root("Config write"))
        return;
    if (!confirm_destructive("Config write", &dev->bdf, force))
        return;

    uint32_t old_val = 0, new_val = 0;

    if (width == 1) {
        old_val = cfg_read8(dev, offset);
        if (pci_cfg_write8(dev, offset, (uint8_t)value) < 0) {
            fprintf(stderr, "%sWrite failed%s\n", clr(CLR_RED), clr(CLR_RESET));
            return;
        }
        pci_cfg_read(dev);
        new_val = cfg_read8(dev, offset);
        printf("%s0x%04" PRIx16 "%s: 0x%02" PRIx32 " -> 0x%02" PRIx32 "\n",
               clr(CLR_CYAN), offset, clr(CLR_RESET), old_val, new_val);
    } else if (width == 2) {
        old_val = cfg_read16(dev, offset);
        if (pci_cfg_write16(dev, offset, (uint16_t)value) < 0) {
            fprintf(stderr, "%sWrite failed%s\n", clr(CLR_RED), clr(CLR_RESET));
            return;
        }
        pci_cfg_read(dev);
        new_val = cfg_read16(dev, offset);
        printf("%s0x%04" PRIx16 "%s: 0x%04" PRIx32 " -> 0x%04" PRIx32 "\n",
               clr(CLR_CYAN), offset, clr(CLR_RESET), old_val, new_val);
    } else if (width == 4) {
        old_val = cfg_read32(dev, offset);
        if (pci_cfg_write32(dev, offset, value) < 0) {
            fprintf(stderr, "%sWrite failed%s\n", clr(CLR_RED), clr(CLR_RESET));
            return;
        }
        pci_cfg_read(dev);
        new_val = cfg_read32(dev, offset);
        printf("%s0x%04" PRIx16 "%s: 0x%08" PRIx32 " -> 0x%08" PRIx32 "\n",
               clr(CLR_CYAN), offset, clr(CLR_RESET), old_val, new_val);
    }
}

void cmd_flr(pci_device_t *dev, bool force)
{
    if (!require_root("FLR"))
        return;
    if (!confirm_destructive("FLR", &dev->bdf, force))
        return;

    char path[512];
    snprintf(path, sizeof(path), "%s/reset", dev->sysfs_path);
    FILE *fp = fopen(path, "w");
    if (fp) {
        if (fwrite("1", 1, 1, fp) == 1) {
            fclose(fp);
            printf("%sFLR via sysfs reset%s\n", clr(CLR_GREEN), clr(CLR_RESET));
            return;
        }
        fclose(fp);
    }

    const pci_cap_t *cap = find_pcie_cap(dev);
    if (!cap) {
        fprintf(stderr, "%sNo PCIe capability%s\n", clr(CLR_RED), clr(CLR_RESET));
        return;
    }

    uint32_t devcap = cfg_read32(dev, cap->offset + PCI_EXP_DEVCAP);
    if (!(devcap & PCI_EXP_DEVCAP_FLR)) {
        fprintf(stderr, "%sDevice does not support FLR%s\n",
                clr(CLR_RED), clr(CLR_RESET));
        return;
    }

    uint16_t devctl = cfg_read16(dev, cap->offset + PCI_EXP_DEVCTL);
    devctl |= PCI_EXP_DEVCTL_BCR_FLR;
    if (pci_cfg_write16(dev, (uint16_t)(cap->offset + PCI_EXP_DEVCTL), devctl) < 0) {
        fprintf(stderr, "%sFLR write failed%s\n", clr(CLR_RED), clr(CLR_RESET));
        return;
    }
    printf("%sFLR initiated via DevCtl%s\n", clr(CLR_GREEN), clr(CLR_RESET));
}

void cmd_sbreset(pci_device_t *dev, bool force)
{
    if (!require_root("Secondary bus reset"))
        return;
    if (!confirm_destructive("Secondary bus reset", &dev->bdf, force))
        return;

    if (dev->header_type != PCI_HEADER_TYPE_BRIDGE) {
        fprintf(stderr, "%sNot a bridge device (header_type=0x%02x)%s\n",
                clr(CLR_RED), dev->header_type, clr(CLR_RESET));
        return;
    }

    uint16_t bctl = cfg_read16(dev, PCI_BRIDGE_CONTROL);
    bctl |= PCI_BRIDGE_CTL_BUS_RESET;
    if (pci_cfg_write16(dev, PCI_BRIDGE_CONTROL, bctl) < 0) {
        fprintf(stderr, "%sBridge control write failed%s\n",
                clr(CLR_RED), clr(CLR_RESET));
        return;
    }
    {
        struct timespec ts = { .tv_sec = 0, .tv_nsec = 100000000 };
        nanosleep(&ts, NULL);
    }
    bctl &= ~PCI_BRIDGE_CTL_BUS_RESET;
    if (pci_cfg_write16(dev, PCI_BRIDGE_CONTROL, bctl) < 0) {
        fprintf(stderr, "%sBridge control clear failed%s\n",
                clr(CLR_RED), clr(CLR_RESET));
        return;
    }
    printf("%sSecondary bus reset done%s\n", clr(CLR_GREEN), clr(CLR_RESET));
}

void cmd_retrain(pci_device_t *dev, bool force)
{
    if (!require_root("Link retrain"))
        return;
    if (!confirm_destructive("Link retrain", &dev->bdf, force))
        return;

    const pci_cap_t *cap = find_pcie_cap(dev);
    if (!cap) {
        fprintf(stderr, "%sNo PCIe capability%s\n", clr(CLR_RED), clr(CLR_RESET));
        return;
    }

    uint16_t lnkctl = cfg_read16(dev, cap->offset + PCI_EXP_LNKCTL);
    lnkctl |= PCI_EXP_LNKCTL_RETRAIN;
    if (pci_cfg_write16(dev, (uint16_t)(cap->offset + PCI_EXP_LNKCTL), lnkctl) < 0) {
        fprintf(stderr, "%sLnkCtl write failed%s\n", clr(CLR_RED), clr(CLR_RESET));
        return;
    }
    sleep(1);

    if (pci_cfg_read(dev) < 0) {
        fprintf(stderr, "%sConfig re-read failed%s\n", clr(CLR_RED), clr(CLR_RESET));
        return;
    }
    uint16_t lnksta = cfg_read16(dev, cap->offset + PCI_EXP_LNKSTA);
    uint8_t speed = lnksta & PCI_EXP_LNKSTA_SPEED;
    uint8_t width = (lnksta & PCI_EXP_LNKSTA_WIDTH) >> 4;
    printf("%sRetrain done: %s x%u%s\n",
           clr(CLR_GREEN), pci_link_speed_name(speed), width, clr(CLR_RESET));
}
