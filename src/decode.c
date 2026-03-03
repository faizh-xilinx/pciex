#include "pciex.h"
#include "pci_regs.h"
#include <stdio.h>

#define INDENT "    "

void decode_pm_cap(const pci_device_t *dev, uint16_t off)
{
    uint16_t cap = cfg_read16(dev, off + PCI_PM_CAP);
    uint16_t ctrl = cfg_read16(dev, off + PCI_PM_CTRL);
    uint8_t state = ctrl & PCI_PM_CTRL_STATE_MASK;

    const char *states[] = {"D0", "D1", "D2", "D3hot"};

    printf(INDENT "PMC:   0x%04x  D1%s D2%s AuxPwr%s PMEClk%s\n",
           cap,
           (cap & PCI_PM_CAP_D1) ? "+" : "-",
           (cap & PCI_PM_CAP_D2) ? "+" : "-",
           (cap & (1 << 11)) ? "+" : "-",
           (cap & (1 << 3)) ? "+" : "-");
    printf(INDENT "PMCSR: 0x%04x  State=%s%s%s  PME_En%s PME_Status%s\n",
           ctrl,
           clr(CLR_GREEN), states[state], clr(CLR_RESET),
           (ctrl & (1 << 8)) ? "+" : "-",
           (ctrl & (1 << 15)) ? "+" : "-");
}

void decode_msi_cap(const pci_device_t *dev, uint16_t off)
{
    uint16_t flags = cfg_read16(dev, off + PCI_MSI_FLAGS);
    bool enabled = flags & PCI_MSI_FLAGS_ENABLE;
    bool is_64   = flags & PCI_MSI_FLAGS_64BIT;
    int  mmc     = 1 << ((flags & PCI_MSI_FLAGS_QMASK) >> 1);
    int  mme     = 1 << ((flags & PCI_MSI_FLAGS_QSIZE) >> 4);

    printf(INDENT "Enable%s %s%s%s  Vectors: %d/%d  64bit%s PerVector%s\n",
           enabled ? "+" : "-",
           enabled ? clr(CLR_GREEN) : clr(CLR_DIM),
           enabled ? "ENABLED" : "DISABLED",
           clr(CLR_RESET),
           mme, mmc,
           is_64 ? "+" : "-",
           (flags & PCI_MSI_FLAGS_PERVEC) ? "+" : "-");

    uint32_t addr_lo = cfg_read32(dev, off + PCI_MSI_ADDR_LO);
    if (is_64) {
        uint32_t addr_hi = cfg_read32(dev, off + PCI_MSI_ADDR_HI);
        uint16_t data = cfg_read16(dev, off + PCI_MSI_DATA_64);
        printf(INDENT "Address: 0x%08x%08x  Data: 0x%04x\n",
               addr_hi, addr_lo, data);
    } else {
        uint16_t data = cfg_read16(dev, off + PCI_MSI_DATA_32);
        printf(INDENT "Address: 0x%08x  Data: 0x%04x\n", addr_lo, data);
    }
}

void decode_msix_cap(const pci_device_t *dev, uint16_t off)
{
    uint16_t flags = cfg_read16(dev, off + PCI_MSIX_FLAGS);
    uint32_t table = cfg_read32(dev, off + PCI_MSIX_TABLE);
    uint32_t pba   = cfg_read32(dev, off + PCI_MSIX_PBA);

    bool enabled = flags & PCI_MSIX_FLAGS_ENABLE;
    bool masked  = flags & PCI_MSIX_FLAGS_MASK;
    int  tbl_sz  = (flags & PCI_MSIX_FLAGS_TBLSIZE) + 1;

    printf(INDENT "Enable%s %s%s%s  Masked%s  Table Size: %s%d%s\n",
           enabled ? "+" : "-",
           enabled ? clr(CLR_GREEN) : clr(CLR_DIM),
           enabled ? "ENABLED" : "DISABLED",
           clr(CLR_RESET),
           masked ? "+" : "-",
           clr(CLR_CYAN), tbl_sz, clr(CLR_RESET));
    printf(INDENT "Table: BAR=%s%d%s  offset=0x%05x\n",
           clr(CLR_YELLOW), table & PCI_MSIX_BIR_MASK, clr(CLR_RESET),
           table & ~PCI_MSIX_BIR_MASK);
    printf(INDENT "PBA:   BAR=%s%d%s  offset=0x%05x\n",
           clr(CLR_YELLOW), pba & PCI_MSIX_BIR_MASK, clr(CLR_RESET),
           pba & ~PCI_MSIX_BIR_MASK);
}

void decode_pcie_cap(const pci_device_t *dev, uint16_t off)
{
    uint16_t flags  = cfg_read16(dev, off + PCI_EXP_FLAGS);
    uint32_t devcap = cfg_read32(dev, off + PCI_EXP_DEVCAP);
    uint16_t devctl = cfg_read16(dev, off + PCI_EXP_DEVCTL);
    uint32_t lnkcap = cfg_read32(dev, off + PCI_EXP_LNKCAP);
    uint16_t lnksta = cfg_read16(dev, off + PCI_EXP_LNKSTA);

    uint8_t version = flags & PCI_EXP_FLAGS_VERSION;
    uint8_t type    = (flags & PCI_EXP_FLAGS_TYPE) >> 4;

    uint8_t  cap_speed = lnkcap & PCI_EXP_LNKCAP_SPEED;
    uint8_t  cap_width = (lnkcap & PCI_EXP_LNKCAP_WIDTH) >> 4;
    uint8_t  sta_speed = lnksta & PCI_EXP_LNKSTA_SPEED;
    uint8_t  sta_width = (lnksta & PCI_EXP_LNKSTA_WIDTH) >> 4;
    uint16_t mps = 128 << ((devcap >> 0) & 0x07);
    uint16_t mrrs = 128 << ((devctl >> 12) & 0x07);
    uint16_t cur_mps = 128 << ((devctl >> 5) & 0x07);

    printf(INDENT "Version: %d  Type: %s%s%s\n",
           version, clr(CLR_CYAN), pci_exp_type_name(type), clr(CLR_RESET));
    printf(INDENT "DevCap:  MaxPayload %d bytes  PhantFunc %d  ExtTag%s\n",
           mps, (devcap >> 3) & 0x03, (devcap & (1 << 5)) ? "+" : "-");
    printf(INDENT "DevCtl:  CorrErr%s NonFatalErr%s FatalErr%s UnsupReq%s\n",
           (devctl & (1 << 0)) ? "+" : "-",
           (devctl & (1 << 1)) ? "+" : "-",
           (devctl & (1 << 2)) ? "+" : "-",
           (devctl & (1 << 3)) ? "+" : "-");
    printf(INDENT "         MaxPayload %d bytes  MaxReadReq %d bytes\n",
           cur_mps, mrrs);

    bool speed_ok = (sta_speed >= cap_speed);
    bool width_ok = (sta_width >= cap_width);

    printf(INDENT "LnkCap:  %s%s%s x%d\n",
           clr(CLR_CYAN), pci_link_speed_name(cap_speed), clr(CLR_RESET), cap_width);
    printf(INDENT "LnkSta:  %s%s%s x%s%d%s",
           speed_ok ? clr(CLR_GREEN) : clr(CLR_RED),
           pci_link_speed_name(sta_speed), clr(CLR_RESET),
           width_ok ? clr(CLR_GREEN) : clr(CLR_RED),
           sta_width, clr(CLR_RESET));
    if (!speed_ok || !width_ok)
        printf("  %s(DOWNGRADED)%s", clr(CLR_RED), clr(CLR_RESET));
    printf("\n");
}

void decode_sriov_cap(const pci_device_t *dev, uint16_t off)
{
    uint16_t ctrl     = cfg_read16(dev, off + PCI_SRIOV_CTRL);
    uint16_t total_vf = cfg_read16(dev, off + PCI_SRIOV_TOTAL_VF);
    uint16_t num_vf   = cfg_read16(dev, off + PCI_SRIOV_NUM_VF);
    uint16_t vf_off   = cfg_read16(dev, off + PCI_SRIOV_VF_OFFSET);
    uint16_t vf_str   = cfg_read16(dev, off + PCI_SRIOV_VF_STRIDE);
    uint16_t vf_did   = cfg_read16(dev, off + PCI_SRIOV_VF_DID);

    bool vfe = ctrl & PCI_SRIOV_CTRL_VFE;
    bool mse = ctrl & PCI_SRIOV_CTRL_MSE;
    bool ari = ctrl & PCI_SRIOV_CTRL_ARI;

    printf(INDENT "Ctrl:     VFEnable%s %s%s%s  VF_MSE%s  ARI%s\n",
           vfe ? "+" : "-",
           vfe ? clr(CLR_GREEN) : clr(CLR_DIM),
           vfe ? "ENABLED" : "DISABLED",
           clr(CLR_RESET),
           mse ? "+" : "-",
           ari ? "+" : "-");
    printf(INDENT "TotalVFs: %s%d%s  NumVFs: %s%d%s\n",
           clr(CLR_CYAN), total_vf, clr(CLR_RESET),
           clr(CLR_CYAN), num_vf, clr(CLR_RESET));
    printf(INDENT "VF Offset: %d  VF Stride: %d  VF DeviceID: 0x%04x\n",
           vf_off, vf_str, vf_did);

    for (int i = 0; i < 6; i++) {
        uint32_t bar = cfg_read32(dev, off + PCI_SRIOV_BAR0 + i * 4);
        if (bar == 0)
            continue;
        bool is_64 = ((bar & PCI_BAR_MEM_TYPE_MASK) >> 1) == 2;
        bool pf = (bar & PCI_BAR_PREFETCH) != 0;
        printf(INDENT "VF BAR%d:  0x%08x  %s %s%s\n",
               i, bar & PCI_BAR_MEM_ADDR_MASK,
               is_64 ? "64-bit" : "32-bit",
               pf ? "prefetchable" : "non-prefetchable",
               is_64 ? " (consumes next BAR)" : "");
        if (is_64) i++;
    }
}

void decode_ats_cap(const pci_device_t *dev, uint16_t off)
{
    uint16_t cap  = cfg_read16(dev, off + PCI_ATS_CAP);
    uint16_t ctrl = cfg_read16(dev, off + PCI_ATS_CTRL);
    bool enabled = ctrl & PCI_ATS_CTRL_ENABLE;

    printf(INDENT "Invalidate Queue Depth: %d  Page Aligned: %s\n",
           cap & 0x1F, (cap & 0x20) ? "Yes" : "No");
    printf(INDENT "Enable%s %s%s%s  STU: %d\n",
           enabled ? "+" : "-",
           enabled ? clr(CLR_GREEN) : clr(CLR_DIM),
           enabled ? "ENABLED" : "DISABLED",
           clr(CLR_RESET),
           ctrl & 0x1F);
}

void decode_pasid_cap(const pci_device_t *dev, uint16_t off)
{
    uint16_t cap  = cfg_read16(dev, off + PCI_PASID_CAP);
    uint16_t ctrl = cfg_read16(dev, off + PCI_PASID_CTRL);
    bool enabled = ctrl & PCI_PASID_CTRL_ENABLE;

    printf(INDENT "Max PASID Width: %d  Exec%s Priv%s\n",
           cap & 0x1F,
           (cap & (1 << 1)) ? "+" : "-",
           (cap & (1 << 2)) ? "+" : "-");
    printf(INDENT "Enable%s %s%s%s\n",
           enabled ? "+" : "-",
           enabled ? clr(CLR_GREEN) : clr(CLR_DIM),
           enabled ? "ENABLED" : "DISABLED",
           clr(CLR_RESET));
}

void decode_pri_cap(const pci_device_t *dev, uint16_t off)
{
    uint16_t ctrl   = cfg_read16(dev, off + PCI_PRI_CTRL);
    uint16_t status = cfg_read16(dev, off + PCI_PRI_STATUS);
    bool enabled = ctrl & PCI_PRI_CTRL_ENABLE;

    printf(INDENT "Enable%s %s%s%s  Reset%s\n",
           enabled ? "+" : "-",
           enabled ? clr(CLR_GREEN) : clr(CLR_DIM),
           enabled ? "ENABLED" : "DISABLED",
           clr(CLR_RESET),
           (ctrl & (1 << 1)) ? "+" : "-");
    printf(INDENT "Status: ResponseFailure%s UnexpectedPageReq%s Stopped%s PRGResp%s\n",
           (status & (1 << 0)) ? "+" : "-",
           (status & (1 << 8)) ? "+" : "-",
           (status & (1 << 8)) ? "+" : "-",
           (status & (1 << 15)) ? "+" : "-");
}

void decode_aer_cap(const pci_device_t *dev, uint16_t off)
{
    uint32_t uesta = cfg_read32(dev, off + 0x04);
    uint32_t cesta = cfg_read32(dev, off + 0x10);

    printf(INDENT "UESta: 0x%08x", uesta);
    if (uesta) {
        printf("  %s[", clr(CLR_RED));
        if (uesta & (1 << 4))  printf("DLP ");
        if (uesta & (1 << 5))  printf("SDES ");
        if (uesta & (1 << 12)) printf("TLP_PFX ");
        if (uesta & (1 << 13)) printf("ECRC ");
        if (uesta & (1 << 14)) printf("UR ");
        if (uesta & (1 << 15)) printf("ACS ");
        if (uesta & (1 << 16)) printf("IE ");
        if (uesta & (1 << 17)) printf("MC_BLK ");
        if (uesta & (1 << 18)) printf("AtomOp_Egr ");
        if (uesta & (1 << 20)) printf("POISONED_TLP ");
        printf("]%s", clr(CLR_RESET));
    } else {
        printf("  %s(clean)%s", clr(CLR_GREEN), clr(CLR_RESET));
    }
    printf("\n");

    printf(INDENT "CESta: 0x%08x", cesta);
    if (cesta) {
        printf("  [");
        if (cesta & (1 << 0))  printf("RxErr ");
        if (cesta & (1 << 6))  printf("BadTLP ");
        if (cesta & (1 << 7))  printf("BadDLLP ");
        if (cesta & (1 << 8))  printf("Rollover ");
        if (cesta & (1 << 12)) printf("Timeout ");
        if (cesta & (1 << 13)) printf("AdvNonFatal ");
        printf("]");
    } else {
        printf("  %s(clean)%s", clr(CLR_GREEN), clr(CLR_RESET));
    }
    printf("\n");
}

void decode_dsn_cap(const pci_device_t *dev, uint16_t off)
{
    uint32_t lo = cfg_read32(dev, off + 0x04);
    uint32_t hi = cfg_read32(dev, off + 0x08);

    printf(INDENT "Serial: %s%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x%s\n",
           clr(CLR_CYAN),
           (hi >> 24) & 0xFF, (hi >> 16) & 0xFF, (hi >> 8) & 0xFF, hi & 0xFF,
           (lo >> 24) & 0xFF, (lo >> 16) & 0xFF, (lo >> 8) & 0xFF, lo & 0xFF,
           clr(CLR_RESET));
}

void decode_ide_cap(const pci_device_t *dev, uint16_t off)
{
    uint32_t cap = cfg_read32(dev, off + 0x04);
    uint32_t ctrl = cfg_read32(dev, off + 0x08);

    printf(INDENT "Capability: 0x%08x\n", cap);
    printf(INDENT "  Link IDE Streams:     %d\n", cap & 0xFF);
    printf(INDENT "  Selective IDE Streams: %d\n", (cap >> 8) & 0xFF);
    printf(INDENT "  TEE-Limited Stream:   %s\n", (cap & (1 << 29)) ? "Yes" : "No");
    printf(INDENT "  Flow-Through IDE:     %s\n", (cap & (1 << 25)) ? "Yes" : "No");
    printf(INDENT "Control:    0x%08x\n", ctrl);
}

void decode_doe_cap(const pci_device_t *dev, uint16_t off)
{
    uint32_t cap  = cfg_read32(dev, off + 0x04);
    uint32_t ctrl = cfg_read32(dev, off + 0x08);
    uint32_t sta  = cfg_read32(dev, off + 0x0C);

    printf(INDENT "DOE Cap:  0x%08x  IntSupport%s\n",
           cap, (cap & (1 << 0)) ? "+" : "-");
    printf(INDENT "DOE Ctrl: 0x%08x  Abort%s IntEn%s Go%s\n",
           ctrl,
           (ctrl & (1 << 0)) ? "+" : "-",
           (ctrl & (1 << 1)) ? "+" : "-",
           (ctrl & (1 << 31)) ? "+" : "-");
    printf(INDENT "DOE Sta:  0x%08x  Busy%s IntSta%s Error%s Ready%s\n",
           sta,
           (sta & (1 << 0)) ? "+" : "-",
           (sta & (1 << 1)) ? "+" : "-",
           (sta & (1 << 2)) ? "+" : "-",
           (sta & (1 << 31)) ? "+" : "-");
}
