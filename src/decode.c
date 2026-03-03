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

/* ── Standard Capability Decoders ──────────────────────────────────── */

void decode_vpd_cap(const pci_device_t *dev, uint16_t off)
{
    uint16_t addr = cfg_read16(dev, off + PCI_VPD_ADDR);
    uint32_t data = cfg_read32(dev, off + PCI_VPD_DATA);

    printf(INDENT "Address: 0x%04x  Flag%s  Data: 0x%08x\n",
           addr & 0x7FFF,
           (addr & PCI_VPD_ADDR_FLAG) ? "+" : "-",
           data);
}

void decode_pcix_cap(const pci_device_t *dev, uint16_t off)
{
    uint16_t cmd = cfg_read16(dev, off + PCI_PCIX_CMD);
    uint32_t sta = cfg_read32(dev, off + PCI_PCIX_STATUS);

    const char *mmc[] = {"512", "1024", "2048", "4096"};
    printf(INDENT "Command: 0x%04x  ERO%s  MaxMemRead=%s  MaxSplit=%d\n",
           cmd,
           (cmd & (1 << 0)) ? "+" : "-",
           mmc[(cmd >> 2) & 0x03],
           1 << ((cmd >> 4) & 0x07));
    printf(INDENT "Status:  0x%08x  Bus=%d Dev=%d Func=%d  64bit%s 133MHz%s\n",
           sta,
           (sta >> 8) & 0xFF, (sta >> 3) & 0x1F, sta & 0x07,
           (sta & (1 << 17)) ? "+" : "-",
           (sta & (1 << 18)) ? "+" : "-");
}

void decode_vendor_cap(const pci_device_t *dev, uint16_t off)
{
    uint8_t len = cfg_read8(dev, off + 0x02);

    printf(INDENT "Length: %s%d%s bytes\n", clr(CLR_CYAN), len, clr(CLR_RESET));
    if (len > 3 && len <= 64) {
        printf(INDENT "Data:  ");
        for (int i = 3; i < len && (size_t)(off + i) < dev->cfg_size; i++)
            printf("%02x ", cfg_read8(dev, off + i));
        printf("\n");
    }
}

void decode_ssvid_cap(const pci_device_t *dev, uint16_t off)
{
    uint16_t svid = cfg_read16(dev, off + 0x04);
    uint16_t ssid = cfg_read16(dev, off + 0x06);

    printf(INDENT "Subsystem Vendor ID: %s0x%04x%s\n",
           clr(CLR_CYAN), svid, clr(CLR_RESET));
    printf(INDENT "Subsystem ID:        %s0x%04x%s\n",
           clr(CLR_CYAN), ssid, clr(CLR_RESET));
}

void decode_af_cap(const pci_device_t *dev, uint16_t off)
{
    uint8_t cap  = cfg_read8(dev, off + PCI_AF_CAP);
    uint8_t ctrl = cfg_read8(dev, off + PCI_AF_CTRL);
    uint8_t sta  = cfg_read8(dev, off + PCI_AF_STATUS);

    printf(INDENT "AFCap:  TP%s FLR%s\n",
           (cap & (1 << 0)) ? "+" : "-",
           (cap & (1 << 1)) ? "+" : "-");
    printf(INDENT "AFCtrl: Initiate_FLR%s\n",
           (ctrl & (1 << 0)) ? "+" : "-");
    printf(INDENT "AFSta:  TP%s\n",
           (sta & (1 << 0)) ? "+" : "-");
}

/* ── Extended Capability Decoders ──────────────────────────────────── */

void decode_acs_cap(const pci_device_t *dev, uint16_t off)
{
    uint16_t cap  = cfg_read16(dev, off + PCI_ACS_CAP);
    uint16_t ctrl = cfg_read16(dev, off + PCI_ACS_CTRL);

    printf(INDENT "ACSCap:  SrcValid%s TransBlk%s ReqRedir%s CmpltRedir%s UpFwd%s EgressCtrl%s DirTrans%s\n",
           (cap & PCI_ACS_CAP_VALID) ? "+" : "-",
           (cap & PCI_ACS_CAP_BLOCK) ? "+" : "-",
           (cap & PCI_ACS_CAP_REQ_RED) ? "+" : "-",
           (cap & PCI_ACS_CAP_CMPLT_RED) ? "+" : "-",
           (cap & PCI_ACS_CAP_FORWARD) ? "+" : "-",
           (cap & PCI_ACS_CAP_EGRESS) ? "+" : "-",
           (cap & PCI_ACS_CAP_TRANS) ? "+" : "-");
    printf(INDENT "ACSCtrl: SrcValid%s TransBlk%s ReqRedir%s CmpltRedir%s UpFwd%s EgressCtrl%s DirTrans%s\n",
           (ctrl & (1 << 0)) ? "+" : "-",
           (ctrl & (1 << 1)) ? "+" : "-",
           (ctrl & (1 << 2)) ? "+" : "-",
           (ctrl & (1 << 3)) ? "+" : "-",
           (ctrl & (1 << 4)) ? "+" : "-",
           (ctrl & (1 << 5)) ? "+" : "-",
           (ctrl & (1 << 6)) ? "+" : "-");
}

void decode_ari_cap(const pci_device_t *dev, uint16_t off)
{
    uint16_t cap  = cfg_read16(dev, off + PCI_ARI_CAP);
    uint16_t ctrl = cfg_read16(dev, off + PCI_ARI_CTRL);

    printf(INDENT "ARICap:  MFVC%s ACS%s  NextFunc: %d\n",
           (cap & (1 << 0)) ? "+" : "-",
           (cap & (1 << 1)) ? "+" : "-",
           (cap >> 8) & 0xFF);
    printf(INDENT "ARICtrl: MFVC%s ACS%s  FuncGroup: %d\n",
           (ctrl & (1 << 0)) ? "+" : "-",
           (ctrl & (1 << 1)) ? "+" : "-",
           (ctrl >> 4) & 0x07);
}

void decode_dpc_cap(const pci_device_t *dev, uint16_t off)
{
    uint16_t cap    = cfg_read16(dev, off + PCI_DPC_CAP);
    uint16_t ctrl   = cfg_read16(dev, off + PCI_DPC_CTRL);
    uint16_t status = cfg_read16(dev, off + PCI_DPC_STATUS);
    uint16_t src_id = cfg_read16(dev, off + PCI_DPC_SOURCE_ID);

    const char *triggers[] = {"Disabled", "ERR_FATAL", "ERR_NONFATAL", "ERR_COR+ERR_FATAL+ERR_NONFATAL"};

    printf(INDENT "DPCCap:  IntMsgNum=%d  RPExt%s PoisonedTLP_Blk%s SW_Trigger%s\n",
           cap & 0x1F,
           (cap & (1 << 5)) ? "+" : "-",
           (cap & (1 << 6)) ? "+" : "-",
           (cap & (1 << 7)) ? "+" : "-");
    printf(INDENT "DPCCtrl: Trigger=%s%s%s  IntEn%s ErrCorEn%s\n",
           clr(CLR_CYAN), triggers[(ctrl >> 1) & 0x03], clr(CLR_RESET),
           (ctrl & (1 << 3)) ? "+" : "-",
           (ctrl & (1 << 8)) ? "+" : "-");

    bool triggered = status & (1 << 0);
    printf(INDENT "DPCSta:  Triggered%s %s%s%s  IntSta%s  Source: %s0x%04x%s\n",
           triggered ? "+" : "-",
           triggered ? clr(CLR_RED) : clr(CLR_GREEN),
           triggered ? "CONTAINMENT ACTIVE" : "OK",
           clr(CLR_RESET),
           (status & (1 << 3)) ? "+" : "-",
           clr(CLR_YELLOW), src_id, clr(CLR_RESET));
}

void decode_l1pm_cap(const pci_device_t *dev, uint16_t off)
{
    uint32_t cap   = cfg_read32(dev, off + PCI_L1PM_CAP);
    uint32_t ctrl1 = cfg_read32(dev, off + PCI_L1PM_CTRL1);
    uint32_t ctrl2 = cfg_read32(dev, off + PCI_L1PM_CTRL2);

    printf(INDENT "L1PMCap:  PCI-PM_L1.2%s PCI-PM_L1.1%s ASPM_L1.2%s ASPM_L1.1%s L1PM%s\n",
           (cap & (1 << 0)) ? "+" : "-",
           (cap & (1 << 1)) ? "+" : "-",
           (cap & (1 << 2)) ? "+" : "-",
           (cap & (1 << 3)) ? "+" : "-",
           (cap & (1 << 4)) ? "+" : "-");
    printf(INDENT "L1PMCtl1: 0x%08x  PCI-PM_L1.2%s PCI-PM_L1.1%s ASPM_L1.2%s ASPM_L1.1%s\n",
           ctrl1,
           (ctrl1 & (1 << 0)) ? "+" : "-",
           (ctrl1 & (1 << 1)) ? "+" : "-",
           (ctrl1 & (1 << 2)) ? "+" : "-",
           (ctrl1 & (1 << 3)) ? "+" : "-");
    printf(INDENT "L1PMCtl2: 0x%08x\n", ctrl2);
}

void decode_ptm_cap(const pci_device_t *dev, uint16_t off)
{
    uint32_t cap  = cfg_read32(dev, off + PCI_PTM_CAP);
    uint32_t ctrl = cfg_read32(dev, off + PCI_PTM_CTRL);

    printf(INDENT "PTMCap:  Requester%s Responder%s Root%s  Granularity: %d\n",
           (cap & (1 << 0)) ? "+" : "-",
           (cap & (1 << 1)) ? "+" : "-",
           (cap & (1 << 2)) ? "+" : "-",
           (cap >> 8) & 0xFF);
    printf(INDENT "PTMCtrl: Enable%s %s%s%s  RootSelect%s  Granularity: %d\n",
           (ctrl & (1 << 0)) ? "+" : "-",
           (ctrl & (1 << 0)) ? clr(CLR_GREEN) : clr(CLR_DIM),
           (ctrl & (1 << 0)) ? "ENABLED" : "DISABLED",
           clr(CLR_RESET),
           (ctrl & (1 << 1)) ? "+" : "-",
           (ctrl >> 8) & 0xFF);
}

void decode_ltr_cap(const pci_device_t *dev, uint16_t off)
{
    uint16_t snoop   = cfg_read16(dev, off + PCI_LTR_MAX_SNOOP);
    uint16_t nosnoop = cfg_read16(dev, off + PCI_LTR_MAX_NOSNOOP);

    const char *scales[] = {"1ns", "32ns", "1024ns", "32768ns", "1048576ns", "33554432ns", "?", "?"};

    printf(INDENT "Max Snoop Latency:    %s%d%s x %s\n",
           clr(CLR_CYAN), snoop & 0x3FF, clr(CLR_RESET),
           scales[(snoop >> 10) & 0x07]);
    printf(INDENT "Max No-Snoop Latency: %s%d%s x %s\n",
           clr(CLR_CYAN), nosnoop & 0x3FF, clr(CLR_RESET),
           scales[(nosnoop >> 10) & 0x07]);
}

void decode_dvsec_cap(const pci_device_t *dev, uint16_t off)
{
    uint32_t hdr1 = cfg_read32(dev, off + PCI_DVSEC_HEADER1);
    uint32_t hdr2 = cfg_read32(dev, off + PCI_DVSEC_HEADER2);

    uint16_t vendor_id = hdr1 & 0xFFFF;
    uint8_t  rev       = (hdr1 >> 16) & 0x0F;
    uint16_t length    = (hdr1 >> 20) & 0xFFF;
    uint16_t dvsec_id  = hdr2 & 0xFFFF;

    printf(INDENT "DVSEC Vendor: %s0x%04x%s  Rev: %d  Length: %d  DVSEC ID: %s0x%04x%s\n",
           clr(CLR_CYAN), vendor_id, clr(CLR_RESET),
           rev, length,
           clr(CLR_YELLOW), dvsec_id, clr(CLR_RESET));
}

void decode_rebar_cap(const pci_device_t *dev, uint16_t off)
{
    for (int i = 0; i < 6; i++) {
        uint32_t cap  = cfg_read32(dev, off + PCI_REBAR_CAP + i * 8);
        uint32_t ctrl = cfg_read32(dev, off + PCI_REBAR_CTRL + i * 8);

        if (cap == 0 && ctrl == 0)
            break;

        uint8_t  bar_idx = (ctrl >> 8) & 0x07;
        uint8_t  nbars   = (ctrl >> 5) & 0x07;
        uint32_t sizes   = (cap >> 4) & 0x0FFFFF;
        uint8_t  cur_sz  = (ctrl >> 8) & 0x1F;

        printf(INDENT "BAR%d:  Sizes=[", bar_idx);
        for (int s = 0; s < 20; s++) {
            if (sizes & (1 << s))
                printf(" %dMB", 1 << s);
        }
        printf(" ]  Current: %s%dMB%s  NumBARs: %d\n",
               clr(CLR_GREEN), 1 << cur_sz, clr(CLR_RESET), nbars);
    }
}

void decode_tph_cap(const pci_device_t *dev, uint16_t off)
{
    uint32_t cap = cfg_read32(dev, off + PCI_TPH_CAP);

    printf(INDENT "TPHCap:  NoST%s IntVec%s DevSpec%s\n",
           (cap & (1 << 0)) ? "+" : "-",
           (cap & (1 << 1)) ? "+" : "-",
           (cap & (1 << 2)) ? "+" : "-");
    printf(INDENT "         ExtTPHReq%s  STTableLoc=%d  STTableSize=%d\n",
           (cap & (1 << 8)) ? "+" : "-",
           (cap >> 9) & 0x03,
           (cap >> 16) & 0x7FF);
}

void decode_secpci_cap(const pci_device_t *dev, uint16_t off)
{
    uint32_t lnkctl3  = cfg_read32(dev, off + PCI_SECPCI_LNKCTL3);
    uint32_t lane_err = cfg_read32(dev, off + PCI_SECPCI_LANE_ERR_STA);

    printf(INDENT "LnkCtl3:     0x%08x  Equalization%s LinkEqReqIntEn%s\n",
           lnkctl3,
           (lnkctl3 & (1 << 0)) ? "+" : "-",
           (lnkctl3 & (1 << 1)) ? "+" : "-");
    printf(INDENT "LaneErrSta:  0x%08x", lane_err);
    if (lane_err)
        printf("  %s(errors on lanes)%s", clr(CLR_RED), clr(CLR_RESET));
    else
        printf("  %s(clean)%s", clr(CLR_GREEN), clr(CLR_RESET));
    printf("\n");
}

void decode_dlnk_cap(const pci_device_t *dev, uint16_t off)
{
    uint32_t cap    = cfg_read32(dev, off + PCI_DLNK_CAP);
    uint32_t status = cfg_read32(dev, off + PCI_DLNK_STATUS);

    printf(INDENT "DLFeatureCap:  ScaledFlowCtrl%s  LocalDLFeatureSupp: 0x%03x\n",
           (cap & (1 << 0)) ? "+" : "-",
           (cap >> 0) & 0xFFF);
    printf(INDENT "DLFeatureSta:  RemoteDLFeatureSupp: 0x%03x  Valid%s\n",
           (status >> 0) & 0xFFF,
           (status & (1 << 31)) ? "+" : "-");
}

void decode_16gt_cap(const pci_device_t *dev, uint16_t off)
{
    uint32_t cap  = cfg_read32(dev, off + PCI_16GT_CAP);
    uint32_t ctrl = cfg_read32(dev, off + PCI_16GT_CTRL);
    uint32_t sta  = cfg_read32(dev, off + PCI_16GT_STATUS);

    printf(INDENT "16GTCap:  0x%08x\n", cap);
    printf(INDENT "16GTCtrl: 0x%08x\n", ctrl);
    printf(INDENT "16GTSta:  0x%08x  EqComplete%s Phase1%s Phase2%s Phase3%s\n",
           sta,
           (sta & (1 << 0)) ? "+" : "-",
           (sta & (1 << 1)) ? "+" : "-",
           (sta & (1 << 2)) ? "+" : "-",
           (sta & (1 << 3)) ? "+" : "-");
}

void decode_lmr_cap(const pci_device_t *dev, uint16_t off)
{
    uint16_t cap = cfg_read16(dev, off + PCI_LMR_PORT_CAP);
    uint16_t sta = cfg_read16(dev, off + PCI_LMR_PORT_STATUS);

    printf(INDENT "LMRCap:   MarginingReady%s SoftwareReady%s\n",
           (cap & (1 << 0)) ? "+" : "-",
           (cap & (1 << 1)) ? "+" : "-");
    printf(INDENT "LMRSta:   MarginingReady%s SoftwareReady%s\n",
           (sta & (1 << 0)) ? "+" : "-",
           (sta & (1 << 1)) ? "+" : "-");
}

void decode_32gt_cap(const pci_device_t *dev, uint16_t off)
{
    uint32_t cap  = cfg_read32(dev, off + PCI_32GT_CAP);
    uint32_t ctrl = cfg_read32(dev, off + PCI_32GT_CTRL);
    uint32_t sta  = cfg_read32(dev, off + PCI_32GT_STATUS);

    printf(INDENT "32GTCap:  0x%08x  ModTS%s\n",
           cap, (cap & (1 << 0)) ? "+" : "-");
    printf(INDENT "32GTCtrl: 0x%08x\n", ctrl);
    printf(INDENT "32GTSta:  0x%08x  EqComplete%s Phase1%s Phase2%s Phase3%s\n",
           sta,
           (sta & (1 << 0)) ? "+" : "-",
           (sta & (1 << 1)) ? "+" : "-",
           (sta & (1 << 2)) ? "+" : "-",
           (sta & (1 << 3)) ? "+" : "-");
}

void decode_vc_cap(const pci_device_t *dev, uint16_t off)
{
    uint32_t cap1 = cfg_read32(dev, off + PCI_VC_PORT_CAP1);
    uint32_t cap2 = cfg_read32(dev, off + PCI_VC_PORT_CAP2);
    uint16_t ctrl = cfg_read16(dev, off + PCI_VC_PORT_CTRL);
    uint16_t sta  = cfg_read16(dev, off + PCI_VC_PORT_STATUS);

    uint8_t ext_vc_cnt  = cap1 & 0x07;
    uint8_t lp_ext_vc   = (cap1 >> 4) & 0x07;
    uint8_t ref_clk     = (cap1 >> 8) & 0x03;

    printf(INDENT "PortCap1: ExtVCCount=%s%d%s  LowPriExtVC=%d  RefClk=%d\n",
           clr(CLR_CYAN), ext_vc_cnt, clr(CLR_RESET),
           lp_ext_vc, ref_clk);
    printf(INDENT "PortCap2: ARBTableOffset=0x%06x  ARBCap=0x%02x\n",
           (cap2 >> 8) & 0xFFFFFF, cap2 & 0xFF);
    printf(INDENT "PortCtrl: 0x%04x  PortSta: 0x%04x  ARBTableValid%s\n",
           ctrl, sta, (sta & (1 << 0)) ? "+" : "-");
}

void decode_pwr_cap(const pci_device_t *dev, uint16_t off)
{
    uint32_t data = cfg_read32(dev, off + PCI_PWR_DATA);
    uint32_t cap  = cfg_read32(dev, off + PCI_PWR_CAP);

    printf(INDENT "PwrData: 0x%08x  BudgetSys%s\n",
           data, (cap & (1 << 0)) ? "+" : "-");
    printf(INDENT "PwrCap:  0x%08x\n", cap);
}

void decode_mcast_cap(const pci_device_t *dev, uint16_t off)
{
    uint16_t cap  = cfg_read16(dev, off + PCI_MCAST_CAP);
    uint16_t ctrl = cfg_read16(dev, off + PCI_MCAST_CTRL);

    printf(INDENT "MCastCap:  MaxGroup=%d  WindowSizeReq=%d\n",
           (cap >> 0) & 0x3F, (cap >> 8) & 0x3F);
    printf(INDENT "MCastCtrl: NumGroup=%d  Enable%s %s%s%s\n",
           ctrl & 0x3F,
           (ctrl & (1 << 15)) ? "+" : "-",
           (ctrl & (1 << 15)) ? clr(CLR_GREEN) : clr(CLR_DIM),
           (ctrl & (1 << 15)) ? "ENABLED" : "DISABLED",
           clr(CLR_RESET));
}

void decode_vendor_ext_cap(const pci_device_t *dev, uint16_t off)
{
    uint32_t hdr = cfg_read32(dev, off + 0x04);

    printf(INDENT "VSEC ID: %s0x%04x%s  Rev: %d  Length: %d\n",
           clr(CLR_CYAN), hdr & 0xFFFF, clr(CLR_RESET),
           (hdr >> 16) & 0x0F,
           (hdr >> 20) & 0xFFF);
}
