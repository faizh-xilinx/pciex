#include "pciex.h"
#include "pci_regs.h"

void pci_parse_header(pci_device_t *dev)
{
    dev->vendor_id       = cfg_read16(dev, PCI_VENDOR_ID);
    dev->device_id       = cfg_read16(dev, PCI_DEVICE_ID);
    dev->command         = cfg_read16(dev, PCI_COMMAND);
    dev->status          = cfg_read16(dev, PCI_STATUS);
    dev->revision        = cfg_read8(dev, PCI_REVISION_ID);
    dev->prog_if         = cfg_read8(dev, PCI_PROG_IF);
    dev->subclass        = cfg_read8(dev, PCI_SUBCLASS);
    dev->class_code      = cfg_read8(dev, PCI_CLASS);
    dev->header_type     = cfg_read8(dev, PCI_HEADER_TYPE) & 0x7F;
    dev->subsys_vendor_id = cfg_read16(dev, PCI_SUBSYS_VENDOR_ID);
    dev->subsys_id       = cfg_read16(dev, PCI_SUBSYS_ID);
}

void pci_parse_bars(pci_device_t *dev)
{
    int i;
    dev->num_bars = 0;

    if (dev->header_type != PCI_HEADER_TYPE_NORMAL)
        return;

    for (i = 0; i < PCI_NUM_BARS; i++) {
        uint32_t bar_val = cfg_read32(dev, PCI_BAR0 + i * 4);
        pci_bar_t *bar = &dev->bars[dev->num_bars];

        if (bar_val == 0)
            continue;

        bar->index = i;
        bar->is_io = (bar_val & PCI_BAR_IO_MASK) != 0;

        if (bar->is_io) {
            bar->addr = bar_val & PCI_BAR_IO_ADDR_MASK;
            bar->is_64bit = false;
            bar->prefetchable = false;
        } else {
            uint8_t mem_type = (bar_val & PCI_BAR_MEM_TYPE_MASK) >> 1;
            bar->prefetchable = (bar_val & PCI_BAR_PREFETCH) != 0;
            bar->addr = bar_val & PCI_BAR_MEM_ADDR_MASK;
            bar->is_64bit = (mem_type == 2);

            if (bar->is_64bit && i < PCI_NUM_BARS - 1) {
                uint32_t bar_hi = cfg_read32(dev, PCI_BAR0 + (i + 1) * 4);
                bar->addr |= (uint64_t)bar_hi << 32;
                i++;
            }
        }

        bar->size = 0;
        dev->num_bars++;
    }
}

void pci_parse_caps(pci_device_t *dev)
{
    dev->num_caps = 0;

    /* Standard capabilities */
    if (dev->status & PCI_STATUS_CAP_LIST) {
        uint8_t pos = cfg_read8(dev, PCI_CAP_PTR) & 0xFC;

        while (pos && pos != 0xFF && dev->num_caps < MAX_CAPS) {
            uint8_t id = cfg_read8(dev, pos);
            uint8_t next = cfg_read8(dev, pos + 1) & 0xFC;

            if (id == 0xFF)
                break;

            pci_cap_t *cap = &dev->caps[dev->num_caps++];
            cap->offset = pos;
            cap->id = id;
            cap->is_extended = false;
            cap->version = 0;

            pos = next;
        }
    }

    /* Extended capabilities (PCIe, offset >= 0x100) */
    if (dev->cfg_size > PCI_CFG_SPACE_SIZE) {
        uint16_t pos = PCI_EXT_CAP_START;

        while (pos && pos < dev->cfg_size - 4 && dev->num_caps < MAX_CAPS) {
            uint32_t header = cfg_read32(dev, pos);
            uint16_t id   = header & PCI_EXT_CAP_ID_MASK;
            uint8_t  ver  = (header & PCI_EXT_CAP_VER_MASK) >> 16;
            uint16_t next = (header & PCI_EXT_CAP_NEXT_MASK) >> 20;

            if (header == 0 || header == 0xFFFFFFFF)
                break;

            pci_cap_t *cap = &dev->caps[dev->num_caps++];
            cap->offset = pos;
            cap->id = id;
            cap->is_extended = true;
            cap->version = ver;

            pos = next;
        }
    }
}
