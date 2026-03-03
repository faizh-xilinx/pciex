#include "pciex.h"
#include "pci_regs.h"

const char *pci_class_name(uint8_t class_code, uint8_t subclass)
{
    switch (class_code) {
    case 0x00:
        return subclass == 0x01 ? "VGA-Compatible Device" : "Unclassified";
    case 0x01:
        switch (subclass) {
        case 0x00: return "SCSI Bus Controller";
        case 0x01: return "IDE Controller";
        case 0x02: return "Floppy Disk Controller";
        case 0x04: return "RAID Controller";
        case 0x05: return "ATA Controller";
        case 0x06: return "SATA Controller";
        case 0x07: return "SAS Controller";
        case 0x08: return "NVM Controller";
        default:   return "Mass Storage Controller";
        }
    case 0x02:
        switch (subclass) {
        case 0x00: return "Ethernet Controller";
        case 0x01: return "Token Ring Controller";
        case 0x02: return "FDDI Controller";
        case 0x03: return "ATM Controller";
        case 0x04: return "ISDN Controller";
        case 0x80: return "Other Network Controller";
        default:   return "Network Controller";
        }
    case 0x03:
        switch (subclass) {
        case 0x00: return "VGA Controller";
        case 0x01: return "XGA Controller";
        case 0x02: return "3D Controller";
        default:   return "Display Controller";
        }
    case 0x04: return "Multimedia Controller";
    case 0x05: return "Memory Controller";
    case 0x06:
        switch (subclass) {
        case 0x00: return "Host Bridge";
        case 0x01: return "ISA Bridge";
        case 0x02: return "EISA Bridge";
        case 0x03: return "MCA Bridge";
        case 0x04: return "PCI-to-PCI Bridge";
        case 0x07: return "CardBus Bridge";
        case 0x09: return "PCI-to-PCI Bridge (Semi)";
        default:   return "Bridge Device";
        }
    case 0x07: return "Communication Controller";
    case 0x08: return "System Peripheral";
    case 0x09: return "Input Device Controller";
    case 0x0A: return "Docking Station";
    case 0x0B: return "Processor";
    case 0x0C:
        switch (subclass) {
        case 0x00: return "FireWire Controller";
        case 0x03: return "USB Controller";
        case 0x05: return "SMBus Controller";
        case 0x07: return "IPMI Interface";
        default:   return "Serial Bus Controller";
        }
    case 0x0D: return "Wireless Controller";
    case 0x0E: return "Intelligent Controller";
    case 0x0F: return "Satellite Controller";
    case 0x10: return "Encryption Controller";
    case 0x11: return "Signal Processing Controller";
    case 0x12: return "Processing Accelerator";
    case 0x13: return "Non-Essential Instrumentation";
    case 0xFF: return "Unassigned Class";
    default:   return "Unknown Device Class";
    }
}

const char *pci_cap_name(uint8_t id)
{
    switch (id) {
    case PCI_CAP_ID_PM:      return "Power Management";
    case PCI_CAP_ID_AGP:     return "AGP";
    case PCI_CAP_ID_VPD:     return "Vital Product Data";
    case PCI_CAP_ID_SLOT_ID: return "Slot Identification";
    case PCI_CAP_ID_MSI:     return "MSI";
    case PCI_CAP_ID_CHSWP:   return "CompactPCI HotSwap";
    case PCI_CAP_ID_PCIX:    return "PCI-X";
    case PCI_CAP_ID_HT:      return "HyperTransport";
    case PCI_CAP_ID_VENDOR:  return "Vendor-Specific";
    case PCI_CAP_ID_DBG:     return "Debug Port";
    case PCI_CAP_ID_CCRC:    return "CompactPCI Central Resource";
    case PCI_CAP_ID_SHPC:    return "PCI Hot-Plug";
    case PCI_CAP_ID_SSVID:   return "Bridge Subsystem VID";
    case PCI_CAP_ID_AGP3:    return "AGP 8x";
    case PCI_CAP_ID_SECDEV:  return "Secure Device";
    case PCI_CAP_ID_EXP:     return "PCI Express";
    case PCI_CAP_ID_MSIX:    return "MSI-X";
    case PCI_CAP_ID_SATA:    return "SATA";
    case PCI_CAP_ID_AF:      return "Advanced Features";
    case PCI_CAP_ID_EA:      return "Enhanced Allocation";
    case PCI_CAP_ID_FPB:     return "Flattening Portal Bridge";
    default:                 return "Unknown";
    }
}

const char *pci_ext_cap_name(uint16_t id)
{
    switch (id) {
    case PCI_EXT_CAP_ID_AER:    return "Advanced Error Reporting";
    case PCI_EXT_CAP_ID_VC:     return "Virtual Channel";
    case PCI_EXT_CAP_ID_DSN:    return "Device Serial Number";
    case PCI_EXT_CAP_ID_PWR:    return "Power Budgeting";
    case PCI_EXT_CAP_ID_RCLD:   return "RC Link Declaration";
    case PCI_EXT_CAP_ID_RCILC:  return "RC Internal Link Control";
    case PCI_EXT_CAP_ID_RCEC:   return "RC Event Collector";
    case PCI_EXT_CAP_ID_MFVC:   return "Multi-Function VC";
    case PCI_EXT_CAP_ID_VC2:    return "Virtual Channel (2nd)";
    case PCI_EXT_CAP_ID_RCRB:   return "RCRB Header";
    case PCI_EXT_CAP_ID_VENDOR: return "Vendor-Specific Extended";
    case PCI_EXT_CAP_ID_CAC:    return "Config Access Correlation";
    case PCI_EXT_CAP_ID_ACS:    return "Access Control Services";
    case PCI_EXT_CAP_ID_ARI:    return "Alt Routing-ID";
    case PCI_EXT_CAP_ID_ATS:    return "Address Translation Services";
    case PCI_EXT_CAP_ID_SRIOV:  return "SR-IOV";
    case PCI_EXT_CAP_ID_MRIOV:  return "MR-IOV";
    case PCI_EXT_CAP_ID_MCAST:  return "Multicast";
    case PCI_EXT_CAP_ID_PRI:    return "Page Request Interface";
    case PCI_EXT_CAP_ID_REBAR:  return "Resizable BAR";
    case PCI_EXT_CAP_ID_DPA:    return "Dynamic Power Alloc";
    case PCI_EXT_CAP_ID_TPH:    return "TLP Processing Hints";
    case PCI_EXT_CAP_ID_LTR:    return "Latency Tolerance Report";
    case PCI_EXT_CAP_ID_SECPCI: return "Secondary PCIe";
    case PCI_EXT_CAP_ID_PMUX:   return "Protocol Multiplexing";
    case PCI_EXT_CAP_ID_PASID:  return "Process Address Space ID";
    case PCI_EXT_CAP_ID_LNR:    return "LN Requester";
    case PCI_EXT_CAP_ID_DPC:    return "Downstream Port Containment";
    case PCI_EXT_CAP_ID_L1PM:   return "L1 PM Substates";
    case PCI_EXT_CAP_ID_PTM:    return "Precision Time Measurement";
    case PCI_EXT_CAP_ID_FRS:    return "FRS Queueing";
    case PCI_EXT_CAP_ID_RTR:    return "Readiness Time Reporting";
    case PCI_EXT_CAP_ID_DVSEC:  return "Designated Vendor-Specific";
    case PCI_EXT_CAP_ID_VF_REBAR: return "VF Resizable BAR";
    case PCI_EXT_CAP_ID_DLNK:   return "Data Link Feature";
    case PCI_EXT_CAP_ID_16GT:   return "Physical Layer 16.0 GT/s";
    case PCI_EXT_CAP_ID_LMR:    return "Lane Margining at Receiver";
    case PCI_EXT_CAP_ID_HIER_ID: return "Hierarchy ID";
    case PCI_EXT_CAP_ID_NPEM:   return "Native PCIe Encl Mgmt";
    case PCI_EXT_CAP_ID_32GT:   return "Physical Layer 32.0 GT/s";
    case PCI_EXT_CAP_ID_DOE:    return "Data Object Exchange";
    case PCI_EXT_CAP_ID_IDE:    return "Integrity & Data Encryption";
    default:                    return "Unknown Extended";
    }
}

const char *pci_exp_type_name(uint8_t type)
{
    switch (type) {
    case PCI_EXP_TYPE_ENDPOINT:    return "Endpoint";
    case PCI_EXP_TYPE_LEG_END:     return "Legacy Endpoint";
    case PCI_EXP_TYPE_ROOT_PORT:   return "Root Port";
    case PCI_EXP_TYPE_UPSTREAM:    return "Upstream Port";
    case PCI_EXP_TYPE_DOWNSTREAM:  return "Downstream Port";
    case PCI_EXP_TYPE_PCI_BRIDGE:  return "PCI/PCI-X to PCIe Bridge";
    case PCI_EXP_TYPE_PCIE_BRIDGE: return "PCIe to PCI/PCI-X Bridge";
    case PCI_EXP_TYPE_RC_END:      return "Root Complex Endpoint";
    case PCI_EXP_TYPE_RC_EC:       return "Root Complex Event Collector";
    default:                       return "Unknown Type";
    }
}

const char *pci_link_speed_name(uint8_t speed)
{
    switch (speed) {
    case 1:  return "2.5 GT/s";
    case 2:  return "5.0 GT/s";
    case 3:  return "8.0 GT/s";
    case 4:  return "16.0 GT/s";
    case 5:  return "32.0 GT/s";
    case 6:  return "64.0 GT/s";
    default: return "Unknown";
    }
}
