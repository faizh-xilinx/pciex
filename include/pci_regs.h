#ifndef PCIEX_PCI_REGS_H
#define PCIEX_PCI_REGS_H

/*
 * PCI Configuration Space Register Offsets and Masks
 * Reference: PCI Local Bus Specification 3.0, PCIe Base Spec 6.0
 */

/* Type 0/1 Common Header (0x00 - 0x3F) */
#define PCI_VENDOR_ID           0x00
#define PCI_DEVICE_ID           0x02
#define PCI_COMMAND             0x04
#define PCI_STATUS              0x06
#define PCI_REVISION_ID         0x08
#define PCI_PROG_IF             0x09
#define PCI_SUBCLASS            0x0A
#define PCI_CLASS               0x0B
#define PCI_CACHE_LINE_SIZE     0x0C
#define PCI_LATENCY_TIMER       0x0D
#define PCI_HEADER_TYPE         0x0E
#define PCI_BIST                0x0F

/* Type 0 Header */
#define PCI_BAR0                0x10
#define PCI_BAR1                0x14
#define PCI_BAR2                0x18
#define PCI_BAR3                0x1C
#define PCI_BAR4                0x20
#define PCI_BAR5                0x24
#define PCI_CARDBUS_CIS         0x28
#define PCI_SUBSYS_VENDOR_ID    0x2C
#define PCI_SUBSYS_ID           0x2E
#define PCI_ROM_BAR             0x30
#define PCI_CAP_PTR             0x34
#define PCI_INTR_LINE           0x3C
#define PCI_INTR_PIN            0x3D
#define PCI_MIN_GNT             0x3E
#define PCI_MAX_LAT             0x3F

/* Command Register Bits */
#define PCI_CMD_IO_SPACE        (1 << 0)
#define PCI_CMD_MEM_SPACE       (1 << 1)
#define PCI_CMD_BUS_MASTER      (1 << 2)
#define PCI_CMD_SPECIAL_CYCLE   (1 << 3)
#define PCI_CMD_MWI             (1 << 4)
#define PCI_CMD_VGA_PALETTE     (1 << 5)
#define PCI_CMD_PARITY_ERR      (1 << 6)
#define PCI_CMD_SERR            (1 << 8)
#define PCI_CMD_FAST_B2B        (1 << 9)
#define PCI_CMD_INTX_DISABLE    (1 << 10)

/* Status Register Bits */
#define PCI_STATUS_INTX         (1 << 3)
#define PCI_STATUS_CAP_LIST     (1 << 4)
#define PCI_STATUS_66MHZ        (1 << 5)
#define PCI_STATUS_FAST_B2B     (1 << 7)
#define PCI_STATUS_PARITY_ERR   (1 << 8)
#define PCI_STATUS_DEVSEL_MASK  (3 << 9)
#define PCI_STATUS_SIG_TGT_ABT  (1 << 11)
#define PCI_STATUS_RCV_TGT_ABT  (1 << 12)
#define PCI_STATUS_RCV_MST_ABT  (1 << 13)
#define PCI_STATUS_SIG_SYS_ERR  (1 << 14)
#define PCI_STATUS_DET_PARITY   (1 << 15)

/* BAR Decoding */
#define PCI_BAR_IO_MASK         0x01
#define PCI_BAR_MEM_TYPE_MASK   0x06
#define PCI_BAR_MEM_TYPE_32     0x00
#define PCI_BAR_MEM_TYPE_1M     0x02
#define PCI_BAR_MEM_TYPE_64     0x04
#define PCI_BAR_PREFETCH        0x08
#define PCI_BAR_IO_ADDR_MASK    0xFFFFFFFC
#define PCI_BAR_MEM_ADDR_MASK   0xFFFFFFF0

/* Header Type */
#define PCI_HEADER_TYPE_NORMAL  0x00
#define PCI_HEADER_TYPE_BRIDGE  0x01
#define PCI_HEADER_TYPE_CARDBUS 0x02
#define PCI_HEADER_MULTI_FUNC   0x80

/* Standard Capability IDs */
#define PCI_CAP_ID_PM           0x01    /* Power Management */
#define PCI_CAP_ID_AGP          0x02    /* AGP */
#define PCI_CAP_ID_VPD          0x03    /* Vital Product Data */
#define PCI_CAP_ID_SLOT_ID      0x04    /* Slot Identification */
#define PCI_CAP_ID_MSI          0x05    /* Message Signalled Interrupts */
#define PCI_CAP_ID_CHSWP        0x06    /* CompactPCI HotSwap */
#define PCI_CAP_ID_PCIX         0x07    /* PCI-X */
#define PCI_CAP_ID_HT           0x08    /* HyperTransport */
#define PCI_CAP_ID_VENDOR       0x09    /* Vendor-Specific */
#define PCI_CAP_ID_DBG          0x0A    /* Debug port */
#define PCI_CAP_ID_CCRC         0x0B    /* CompactPCI Central Resource */
#define PCI_CAP_ID_SHPC         0x0C    /* PCI Hot-Plug */
#define PCI_CAP_ID_SSVID        0x0D    /* Bridge subsystem vendor/device ID */
#define PCI_CAP_ID_AGP3         0x0E    /* AGP 8x */
#define PCI_CAP_ID_SECDEV       0x0F    /* Secure Device */
#define PCI_CAP_ID_EXP          0x10    /* PCI Express */
#define PCI_CAP_ID_MSIX         0x11    /* MSI-X */
#define PCI_CAP_ID_SATA         0x12    /* SATA Data/Index Conf. */
#define PCI_CAP_ID_AF           0x13    /* PCI Advanced Features */
#define PCI_CAP_ID_EA           0x14    /* Enhanced Allocation */
#define PCI_CAP_ID_FPB          0x15    /* Flattening Portal Bridge */

/* MSI Capability */
#define PCI_MSI_FLAGS           0x02
#define PCI_MSI_FLAGS_64BIT     (1 << 7)
#define PCI_MSI_FLAGS_ENABLE    (1 << 0)
#define PCI_MSI_FLAGS_QMASK     0x0E
#define PCI_MSI_FLAGS_QSIZE     0x70
#define PCI_MSI_FLAGS_PERVEC    (1 << 8)
#define PCI_MSI_ADDR_LO         0x04
#define PCI_MSI_ADDR_HI         0x08
#define PCI_MSI_DATA_32         0x08
#define PCI_MSI_DATA_64         0x0C

/* MSI-X Capability */
#define PCI_MSIX_FLAGS          0x02
#define PCI_MSIX_FLAGS_ENABLE   (1 << 15)
#define PCI_MSIX_FLAGS_MASK     (1 << 14)
#define PCI_MSIX_FLAGS_TBLSIZE  0x07FF
#define PCI_MSIX_TABLE          0x04
#define PCI_MSIX_PBA            0x08
#define PCI_MSIX_BIR_MASK       0x07

/* PCI Express Capability */
#define PCI_EXP_FLAGS           0x02
#define PCI_EXP_FLAGS_VERSION   0x000F
#define PCI_EXP_FLAGS_TYPE      0x00F0
#define PCI_EXP_DEVCAP          0x04
#define PCI_EXP_DEVCTL          0x08
#define PCI_EXP_DEVSTA          0x0A
#define PCI_EXP_LNKCAP          0x0C
#define PCI_EXP_LNKCTL          0x10
#define PCI_EXP_LNKSTA          0x12
#define PCI_EXP_SLTCAP          0x14
#define PCI_EXP_SLTCTL          0x18
#define PCI_EXP_SLTSTA          0x1A
#define PCI_EXP_RTCTL           0x1C
#define PCI_EXP_RTCAP           0x1E
#define PCI_EXP_RTSTA           0x20
#define PCI_EXP_DEVCAP2         0x24
#define PCI_EXP_DEVCTL2         0x28
#define PCI_EXP_DEVSTA2         0x2A
#define PCI_EXP_LNKCAP2         0x2C
#define PCI_EXP_LNKCTL2         0x30
#define PCI_EXP_LNKSTA2         0x32

/* PCIe Device Types */
#define PCI_EXP_TYPE_ENDPOINT   0x0
#define PCI_EXP_TYPE_LEG_END    0x1
#define PCI_EXP_TYPE_ROOT_PORT  0x4
#define PCI_EXP_TYPE_UPSTREAM   0x5
#define PCI_EXP_TYPE_DOWNSTREAM 0x6
#define PCI_EXP_TYPE_PCI_BRIDGE 0x7
#define PCI_EXP_TYPE_PCIE_BRIDGE 0x8
#define PCI_EXP_TYPE_RC_END     0x9
#define PCI_EXP_TYPE_RC_EC      0xA

/* PCIe Link Speed */
#define PCI_EXP_LNKSTA_SPEED   0x000F
#define PCI_EXP_LNKSTA_WIDTH   0x03F0
#define PCI_EXP_LNKCAP_SPEED   0x000F
#define PCI_EXP_LNKCAP_WIDTH   0x03F0

/* Power Management Capability */
#define PCI_PM_CAP              0x02
#define PCI_PM_CTRL             0x04
#define PCI_PM_CTRL_STATE_MASK  0x0003
#define PCI_PM_CAP_D1           (1 << 9)
#define PCI_PM_CAP_D2           (1 << 10)

/* Extended Configuration Space (offset >= 0x100) */
#define PCI_EXT_CAP_START       0x100
#define PCI_EXT_CAP_ID_MASK     0x0000FFFF
#define PCI_EXT_CAP_VER_MASK    0x000F0000
#define PCI_EXT_CAP_NEXT_MASK   0xFFF00000

/* Extended Capability IDs */
#define PCI_EXT_CAP_ID_AER      0x0001  /* Advanced Error Reporting */
#define PCI_EXT_CAP_ID_VC       0x0002  /* Virtual Channel */
#define PCI_EXT_CAP_ID_DSN      0x0003  /* Device Serial Number */
#define PCI_EXT_CAP_ID_PWR      0x0004  /* Power Budgeting */
#define PCI_EXT_CAP_ID_RCLD     0x0005  /* RC Link Declaration */
#define PCI_EXT_CAP_ID_RCILC    0x0006  /* RC Internal Link Control */
#define PCI_EXT_CAP_ID_RCEC     0x0007  /* RC Event Collector Assoc */
#define PCI_EXT_CAP_ID_MFVC     0x0008  /* Multi-Function VC */
#define PCI_EXT_CAP_ID_VC2      0x0009  /* Virtual Channel (2nd) */
#define PCI_EXT_CAP_ID_RCRB     0x000A  /* RCRB Header */
#define PCI_EXT_CAP_ID_VENDOR   0x000B  /* Vendor-Specific */
#define PCI_EXT_CAP_ID_CAC      0x000C  /* Config Access Correlation */
#define PCI_EXT_CAP_ID_ACS      0x000D  /* Access Control Services */
#define PCI_EXT_CAP_ID_ARI      0x000E  /* Alt Routing-ID Interp */
#define PCI_EXT_CAP_ID_ATS      0x000F  /* Address Translation Services */
#define PCI_EXT_CAP_ID_SRIOV    0x0010  /* SR-IOV */
#define PCI_EXT_CAP_ID_MRIOV    0x0011  /* MR-IOV */
#define PCI_EXT_CAP_ID_MCAST    0x0012  /* Multicast */
#define PCI_EXT_CAP_ID_PRI      0x0013  /* Page Request Interface */
#define PCI_EXT_CAP_ID_RSVD     0x0014  /* Reserved */
#define PCI_EXT_CAP_ID_REBAR    0x0015  /* Resizable BAR */
#define PCI_EXT_CAP_ID_DPA      0x0016  /* Dynamic Power Alloc */
#define PCI_EXT_CAP_ID_TPH      0x0017  /* TLP Processing Hints */
#define PCI_EXT_CAP_ID_LTR      0x0018  /* Latency Tolerance Report */
#define PCI_EXT_CAP_ID_SECPCI   0x0019  /* Secondary PCIe */
#define PCI_EXT_CAP_ID_PMUX     0x001A  /* Protocol Multiplexing */
#define PCI_EXT_CAP_ID_PASID    0x001B  /* Process Address Space ID */
#define PCI_EXT_CAP_ID_LNR      0x001C  /* LN Requester */
#define PCI_EXT_CAP_ID_DPC      0x001D  /* Downstream Port Containment */
#define PCI_EXT_CAP_ID_L1PM     0x001E  /* L1 PM Substates */
#define PCI_EXT_CAP_ID_PTM      0x001F  /* Precision Time Measurement */
#define PCI_EXT_CAP_ID_MPCIE    0x0020  /* PCIe over M-PHY */
#define PCI_EXT_CAP_ID_FRS      0x0021  /* FRS Queueing */
#define PCI_EXT_CAP_ID_RTR      0x0022  /* Readiness Time Reporting */
#define PCI_EXT_CAP_ID_DVSEC    0x0023  /* Designated Vendor-Specific */
#define PCI_EXT_CAP_ID_VF_REBAR 0x0024  /* VF Resizable BAR */
#define PCI_EXT_CAP_ID_DLNK     0x0025  /* Data Link Feature */
#define PCI_EXT_CAP_ID_16GT     0x0026  /* 16.0 GT/s Phy Layer */
#define PCI_EXT_CAP_ID_LMR      0x0027  /* Lane Margining at Rx */
#define PCI_EXT_CAP_ID_HIER_ID  0x0028  /* Hierarchy ID */
#define PCI_EXT_CAP_ID_NPEM     0x0029  /* Native PCIe Encl Mgmt */
#define PCI_EXT_CAP_ID_32GT     0x002A  /* 32.0 GT/s Phy Layer */
#define PCI_EXT_CAP_ID_DOE      0x002E  /* Data Object Exchange */
#define PCI_EXT_CAP_ID_IDE      0x0030  /* Integrity & Data Encryption */

/* SR-IOV Extended Capability */
#define PCI_SRIOV_CAP           0x04
#define PCI_SRIOV_CTRL          0x08
#define PCI_SRIOV_STATUS        0x0A
#define PCI_SRIOV_INIT_VF       0x0C
#define PCI_SRIOV_TOTAL_VF      0x0E
#define PCI_SRIOV_NUM_VF        0x10
#define PCI_SRIOV_FUNC_LINK     0x12
#define PCI_SRIOV_VF_OFFSET     0x14
#define PCI_SRIOV_VF_STRIDE     0x16
#define PCI_SRIOV_VF_DID        0x1A
#define PCI_SRIOV_SUP_PGSIZE    0x1C
#define PCI_SRIOV_SYS_PGSIZE    0x20
#define PCI_SRIOV_BAR0          0x24
#define PCI_SRIOV_CTRL_VFE      (1 << 0)
#define PCI_SRIOV_CTRL_MSE      (1 << 3)
#define PCI_SRIOV_CTRL_ARI      (1 << 4)

/* ATS Extended Capability */
#define PCI_ATS_CAP             0x04
#define PCI_ATS_CTRL            0x06
#define PCI_ATS_CTRL_ENABLE     (1 << 15)

/* PASID Extended Capability */
#define PCI_PASID_CAP           0x04
#define PCI_PASID_CTRL          0x06
#define PCI_PASID_CTRL_ENABLE   (1 << 0)

/* PRI Extended Capability */
#define PCI_PRI_CTRL            0x04
#define PCI_PRI_STATUS          0x06
#define PCI_PRI_CTRL_ENABLE     (1 << 0)

#endif /* PCIEX_PCI_REGS_H */
