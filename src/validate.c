#include "pciex.h"
#include "pci_regs.h"
#include <inttypes.h>
#include <stdio.h>

static const pci_cap_t *find_cap(const pci_device_t *dev, uint16_t id, bool is_ext)
{
    for (int i = 0; i < dev->num_caps; i++) {
        if (dev->caps[i].is_extended == is_ext && dev->caps[i].id == (id & 0xFF))
            return &dev->caps[i];
    }
    return NULL;
}

void show_validation(const pci_device_t *dev)
{
    int issues = 0;

    printf("── Validation ──\n");

    const pci_cap_t *pcie = find_cap(dev, PCI_CAP_ID_EXP, false);
    if (pcie) {
        uint32_t lnkcap = cfg_read32(dev, pcie->offset + PCI_EXP_LNKCAP);
        uint16_t lnksta = cfg_read16(dev, pcie->offset + PCI_EXP_LNKSTA);
        uint8_t cap_speed = lnkcap & PCI_EXP_LNKCAP_SPEED;
        uint8_t cap_width = (lnkcap & PCI_EXP_LNKCAP_WIDTH) >> 4;
        uint8_t sta_speed = lnksta & PCI_EXP_LNKSTA_SPEED;
        uint8_t sta_width = (lnksta & PCI_EXP_LNKSTA_WIDTH) >> 4;
        if (sta_speed < cap_speed || sta_width < cap_width) {
            printf("    %s[WARN]%s  Link downgraded: capable %s x%d, running %s x%d\n",
                   clr(CLR_RED), clr(CLR_RESET),
                   pci_link_speed_name(cap_speed), cap_width,
                   pci_link_speed_name(sta_speed), sta_width);
            issues++;
        }
    }

    const pci_cap_t *aer = find_cap(dev, PCI_EXT_CAP_ID_AER, true);
    if (aer) {
        uint32_t uesta = cfg_read32(dev, aer->offset + 0x04);
        uint32_t cesta = cfg_read32(dev, aer->offset + 0x10);
        if (uesta != 0 || cesta != 0) {
            printf("    %s[WARN]%s  AER errors: UESta=0x%08x CESta=0x%08x\n",
                   clr(CLR_YELLOW), clr(CLR_RESET), uesta, cesta);
            issues++;
        }
    }

    bool bar_overlap = false;
    for (int i = 0; i < dev->num_bars && !bar_overlap; i++) {
        const pci_bar_t *a = &dev->bars[i];
        if (a->addr == 0 || a->size == 0)
            continue;
        uint64_t a_end = a->addr + a->size;
        for (int j = i + 1; j < dev->num_bars; j++) {
            const pci_bar_t *b = &dev->bars[j];
            if (b->addr == 0 || b->size == 0)
                continue;
            uint64_t b_end = b->addr + b->size;
            if (a->addr < b_end && b->addr < a_end) {
                printf("    %s[ERR]%s   BAR overlap: BAR%d [0x%016" PRIx64 ", +0x%" PRIx64 ") vs BAR%d [0x%016" PRIx64 ", +0x%" PRIx64 ")\n",
                       clr(CLR_RED), clr(CLR_RESET),
                       a->index, (uint64_t)a->addr, (uint64_t)a->size,
                       b->index, (uint64_t)b->addr, (uint64_t)b->size);
                bar_overlap = true;
                issues++;
                break;
            }
        }
    }
    if (!bar_overlap && dev->num_bars > 0) {
        bool any_with_size = false;
        for (int i = 0; i < dev->num_bars; i++) {
            if (dev->bars[i].addr != 0 && dev->bars[i].size != 0) {
                any_with_size = true;
                break;
            }
        }
        if (any_with_size)
            printf("    %s[OK]%s    No BAR overlaps detected\n", clr(CLR_GREEN), clr(CLR_RESET));
    }

    const pci_cap_t *dpc = find_cap(dev, PCI_EXT_CAP_ID_DPC, true);
    if (dpc) {
        uint16_t status = cfg_read16(dev, dpc->offset + PCI_DPC_STATUS);
        if (status & 1) {
            uint16_t source = cfg_read16(dev, dpc->offset + PCI_DPC_SOURCE_ID);
            printf("    %s[ERR]%s   DPC containment active — source: 0x%04x\n",
                   clr(CLR_RED), clr(CLR_RESET), source);
            issues++;
        }
    }

    const pci_cap_t *pasid = find_cap(dev, PCI_EXT_CAP_ID_PASID, true);
    if (pasid) {
        uint16_t ctrl = cfg_read16(dev, pasid->offset + PCI_PASID_CTRL);
        if (!(ctrl & PCI_PASID_CTRL_ENABLE)) {
            printf("    %s[INFO]%s  PASID capable but not enabled\n",
                   clr(CLR_YELLOW), clr(CLR_RESET));
            issues++;
        }
    }

    const pci_cap_t *ats = find_cap(dev, PCI_EXT_CAP_ID_ATS, true);
    if (ats) {
        uint16_t ctrl = cfg_read16(dev, ats->offset + PCI_ATS_CTRL);
        if (!(ctrl & PCI_ATS_CTRL_ENABLE)) {
            printf("    %s[INFO]%s  ATS capable but not enabled\n",
                   clr(CLR_YELLOW), clr(CLR_RESET));
            issues++;
        }
    }

    if (!(dev->command & PCI_CMD_BUS_MASTER)) {
        printf("    %s[WARN]%s  Bus Master disabled\n",
               clr(CLR_YELLOW), clr(CLR_RESET));
        issues++;
    }

    if (issues == 0)
        printf("    All checks passed\n");
}
