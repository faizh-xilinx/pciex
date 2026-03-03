#include "pciex.h"
#include "pci_regs.h"
#include <stdio.h>

static void print_bar_size(uint64_t size)
{
    if (size >= (1ULL << 30))
        printf("%lu GB", size >> 30);
    else if (size >= (1ULL << 20))
        printf("%lu MB", size >> 20);
    else if (size >= (1ULL << 10))
        printf("%lu KB", size >> 10);
    else
        printf("%lu B", size);
}

void show_device_summary(const pci_device_t *dev)
{
    printf("%s%04x:%02x:%02x.%x%s  %s[%04x:%04x]%s  %s%s%s  (rev %02x)\n",
           clr(CLR_BOLD),
           dev->bdf.domain, dev->bdf.bus, dev->bdf.dev, dev->bdf.func,
           clr(CLR_RESET),
           clr(CLR_DIM), dev->vendor_id, dev->device_id, clr(CLR_RESET),
           clr(CLR_CYAN),
           pci_class_name(dev->class_code, dev->subclass),
           clr(CLR_RESET),
           dev->revision);
}

void show_header(const pci_device_t *dev)
{
    printf("\n%s── Header ──%s\n", clr(CLR_BOLD), clr(CLR_RESET));
    printf("  Vendor:      %s0x%04x%s\n", clr(CLR_CYAN), dev->vendor_id, clr(CLR_RESET));
    printf("  Device:      %s0x%04x%s\n", clr(CLR_CYAN), dev->device_id, clr(CLR_RESET));
    printf("  SubVendor:   0x%04x\n", dev->subsys_vendor_id);
    printf("  SubDevice:   0x%04x\n", dev->subsys_id);
    printf("  Revision:    0x%02x\n", dev->revision);
    printf("  Class:       %s%s%s [%02x:%02x:%02x]\n",
           clr(CLR_GREEN), pci_class_name(dev->class_code, dev->subclass), clr(CLR_RESET),
           dev->class_code, dev->subclass, dev->prog_if);
    printf("  Header Type: %s%s%s%s\n",
           clr(CLR_YELLOW),
           dev->header_type == 0 ? "Normal" :
           dev->header_type == 1 ? "Bridge" :
           dev->header_type == 2 ? "CardBus" : "Unknown",
           (cfg_read8(dev, PCI_HEADER_TYPE) & PCI_HEADER_MULTI_FUNC) ? " (multi-function)" : "",
           clr(CLR_RESET));

    printf("\n  %sCommand:%s  0x%04x  [", clr(CLR_BOLD), clr(CLR_RESET), dev->command);
    if (dev->command & PCI_CMD_IO_SPACE)    printf(" I/O");
    if (dev->command & PCI_CMD_MEM_SPACE)   printf(" Memory");
    if (dev->command & PCI_CMD_BUS_MASTER)  printf(" BusMaster");
    if (dev->command & PCI_CMD_INTX_DISABLE) printf(" INTx-");
    printf(" ]\n");

    printf("  %sStatus:%s   0x%04x  [", clr(CLR_BOLD), clr(CLR_RESET), dev->status);
    if (dev->status & PCI_STATUS_CAP_LIST)  printf(" CapList");
    if (dev->status & PCI_STATUS_66MHZ)     printf(" 66MHz");
    if (dev->status & PCI_STATUS_FAST_B2B)  printf(" FastB2B");
    if (dev->status & PCI_STATUS_PARITY_ERR) printf(" ParErr");
    printf(" ]\n");
}

void show_bars(const pci_device_t *dev)
{
    if (dev->num_bars == 0)
        return;

    printf("\n%s── BARs ──%s\n", clr(CLR_BOLD), clr(CLR_RESET));
    for (int i = 0; i < dev->num_bars; i++) {
        const pci_bar_t *bar = &dev->bars[i];
        printf("  BAR%d: %s0x%012lx%s  %s  %s%s",
               bar->index,
               clr(CLR_CYAN), bar->addr, clr(CLR_RESET),
               bar->is_io ? "I/O " : "Mem ",
               bar->is_64bit ? "64-bit " : "32-bit ",
               bar->prefetchable ? "prefetchable" : "non-prefetchable");
        if (bar->size > 0) {
            printf("  %s[size=", clr(CLR_GREEN));
            print_bar_size(bar->size);
            printf("]%s", clr(CLR_RESET));
        }
        printf("\n");
    }
}

void show_caps(const pci_device_t *dev)
{
    if (dev->num_caps == 0)
        return;

    bool has_std = false, has_ext = false;
    for (int i = 0; i < dev->num_caps; i++) {
        if (dev->caps[i].is_extended)
            has_ext = true;
        else
            has_std = true;
    }

    if (has_std) {
        printf("\n%s── Capabilities ──%s\n", clr(CLR_BOLD), clr(CLR_RESET));
        for (int i = 0; i < dev->num_caps; i++) {
            const pci_cap_t *cap = &dev->caps[i];
            if (cap->is_extended)
                continue;
            printf("  [%s%02x%s] @ 0x%02x  %s%s%s\n",
                   clr(CLR_YELLOW), cap->id, clr(CLR_RESET),
                   cap->offset,
                   clr(CLR_GREEN), pci_cap_name(cap->id), clr(CLR_RESET));
        }
    }

    if (has_ext) {
        printf("\n%s── Extended Capabilities ──%s\n", clr(CLR_BOLD), clr(CLR_RESET));
        for (int i = 0; i < dev->num_caps; i++) {
            const pci_cap_t *cap = &dev->caps[i];
            if (!cap->is_extended)
                continue;
            printf("  [%s%04x%s] @ 0x%03x v%d  %s%s%s\n",
                   clr(CLR_YELLOW), cap->id, clr(CLR_RESET),
                   cap->offset, cap->version,
                   clr(CLR_GREEN), pci_ext_cap_name(cap->id), clr(CLR_RESET));
        }
    }
}

void show_cap_detail(const pci_device_t *dev, const pci_cap_t *cap)
{
    if (!cap->is_extended) {
        switch (cap->id) {
        case PCI_CAP_ID_PM:     decode_pm_cap(dev, cap->offset);     break;
        case PCI_CAP_ID_MSI:    decode_msi_cap(dev, cap->offset);    break;
        case PCI_CAP_ID_MSIX:   decode_msix_cap(dev, cap->offset);   break;
        case PCI_CAP_ID_EXP:    decode_pcie_cap(dev, cap->offset);   break;
        case PCI_CAP_ID_VPD:    decode_vpd_cap(dev, cap->offset);    break;
        case PCI_CAP_ID_PCIX:   decode_pcix_cap(dev, cap->offset);   break;
        case PCI_CAP_ID_VENDOR: decode_vendor_cap(dev, cap->offset); break;
        case PCI_CAP_ID_SSVID:  decode_ssvid_cap(dev, cap->offset);  break;
        case PCI_CAP_ID_AF:     decode_af_cap(dev, cap->offset);     break;
        default: break;
        }
    } else {
        switch (cap->id) {
        case PCI_EXT_CAP_ID_AER:    decode_aer_cap(dev, cap->offset);        break;
        case PCI_EXT_CAP_ID_DSN:    decode_dsn_cap(dev, cap->offset);        break;
        case PCI_EXT_CAP_ID_SRIOV:  decode_sriov_cap(dev, cap->offset);      break;
        case PCI_EXT_CAP_ID_ATS:    decode_ats_cap(dev, cap->offset);        break;
        case PCI_EXT_CAP_ID_PASID:  decode_pasid_cap(dev, cap->offset);      break;
        case PCI_EXT_CAP_ID_PRI:    decode_pri_cap(dev, cap->offset);        break;
        case PCI_EXT_CAP_ID_IDE:    decode_ide_cap(dev, cap->offset);        break;
        case PCI_EXT_CAP_ID_DOE:    decode_doe_cap(dev, cap->offset);        break;
        case PCI_EXT_CAP_ID_ACS:    decode_acs_cap(dev, cap->offset);        break;
        case PCI_EXT_CAP_ID_ARI:    decode_ari_cap(dev, cap->offset);        break;
        case PCI_EXT_CAP_ID_DPC:    decode_dpc_cap(dev, cap->offset);        break;
        case PCI_EXT_CAP_ID_L1PM:   decode_l1pm_cap(dev, cap->offset);       break;
        case PCI_EXT_CAP_ID_PTM:    decode_ptm_cap(dev, cap->offset);        break;
        case PCI_EXT_CAP_ID_LTR:    decode_ltr_cap(dev, cap->offset);        break;
        case PCI_EXT_CAP_ID_DVSEC:  decode_dvsec_cap(dev, cap->offset);      break;
        case PCI_EXT_CAP_ID_REBAR:  decode_rebar_cap(dev, cap->offset);      break;
        case PCI_EXT_CAP_ID_VF_REBAR: decode_rebar_cap(dev, cap->offset);    break;
        case PCI_EXT_CAP_ID_TPH:    decode_tph_cap(dev, cap->offset);        break;
        case PCI_EXT_CAP_ID_SECPCI: decode_secpci_cap(dev, cap->offset);     break;
        case PCI_EXT_CAP_ID_DLNK:   decode_dlnk_cap(dev, cap->offset);      break;
        case PCI_EXT_CAP_ID_16GT:   decode_16gt_cap(dev, cap->offset);       break;
        case PCI_EXT_CAP_ID_LMR:    decode_lmr_cap(dev, cap->offset);        break;
        case PCI_EXT_CAP_ID_32GT:   decode_32gt_cap(dev, cap->offset);       break;
        case PCI_EXT_CAP_ID_VC:     decode_vc_cap(dev, cap->offset);         break;
        case PCI_EXT_CAP_ID_VC2:    decode_vc_cap(dev, cap->offset);         break;
        case PCI_EXT_CAP_ID_PWR:    decode_pwr_cap(dev, cap->offset);        break;
        case PCI_EXT_CAP_ID_MCAST:  decode_mcast_cap(dev, cap->offset);      break;
        case PCI_EXT_CAP_ID_VENDOR: decode_vendor_ext_cap(dev, cap->offset);  break;
        default: break;
        }
    }
}

void show_hexdump(const pci_device_t *dev)
{
    printf("\n%s── Config Space Hexdump (%zu bytes) ──%s\n",
           clr(CLR_BOLD), dev->cfg_size, clr(CLR_RESET));

    for (size_t i = 0; i < dev->cfg_size; i += 16) {
        printf("  %s%03zx:%s ", clr(CLR_DIM), i, clr(CLR_RESET));
        for (size_t j = 0; j < 16 && (i + j) < dev->cfg_size; j++) {
            if (j == 8) printf(" ");
            uint8_t b = dev->cfg[i + j];
            if (b == 0x00)
                printf("%s%02x%s ", clr(CLR_DIM), b, clr(CLR_RESET));
            else if (b == 0xFF)
                printf("%s%02x%s ", clr(CLR_RED), b, clr(CLR_RESET));
            else
                printf("%02x ", b);
        }
        printf("\n");
    }
}

void show_all(const pci_device_t *dev)
{
    show_device_summary(dev);
    show_header(dev);
    show_bars(dev);
    show_caps(dev);

    /* Detailed decode for each capability */
    for (int i = 0; i < dev->num_caps; i++) {
        const pci_cap_t *cap = &dev->caps[i];
        const char *name = cap->is_extended ?
            pci_ext_cap_name(cap->id) : pci_cap_name(cap->id);

        printf("\n%s── %s @ 0x%03x ──%s\n",
               clr(CLR_BOLD), name, cap->offset, clr(CLR_RESET));
        show_cap_detail(dev, cap);
    }
}
