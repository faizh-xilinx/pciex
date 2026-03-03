#ifndef PCIEX_H
#define PCIEX_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define PCIEX_VERSION "0.2.0"

#define PCI_CFG_SPACE_SIZE      256
#define PCI_EXT_CFG_SPACE_SIZE  4096
#define PCI_NUM_BARS            6

#define SYSFS_PCI_PATH "/sys/bus/pci/devices"

/* ── BDF ─────────────────────────────────────────────────────────── */

typedef struct {
    uint16_t domain;
    uint8_t  bus;
    uint8_t  dev;
    uint8_t  func;
} pci_bdf_t;

/* ── BAR ─────────────────────────────────────────────────────────── */

typedef struct {
    uint8_t  index;
    bool     is_io;
    bool     is_64bit;
    bool     prefetchable;
    uint64_t addr;
    uint64_t size;
} pci_bar_t;

/* ── Capability ──────────────────────────────────────────────────── */

typedef struct {
    uint16_t offset;
    uint8_t  id;
    bool     is_extended;
    uint8_t  version;      /* extended caps only */
} pci_cap_t;

#define MAX_CAPS 64

/* ── Device ──────────────────────────────────────────────────────── */

typedef struct {
    pci_bdf_t bdf;
    char      sysfs_path[256];

    /* Raw config space */
    uint8_t   cfg[PCI_EXT_CFG_SPACE_SIZE];
    size_t    cfg_size;

    /* Parsed header */
    uint16_t  vendor_id;
    uint16_t  device_id;
    uint16_t  subsys_vendor_id;
    uint16_t  subsys_id;
    uint8_t   revision;
    uint8_t   class_code;
    uint8_t   subclass;
    uint8_t   prog_if;
    uint8_t   header_type;
    uint16_t  command;
    uint16_t  status;

    /* BARs */
    pci_bar_t bars[PCI_NUM_BARS];
    int       num_bars;

    /* Capabilities */
    pci_cap_t caps[MAX_CAPS];
    int       num_caps;
} pci_device_t;

/* ── Color output ────────────────────────────────────────────────── */

typedef enum {
    CLR_RESET = 0,
    CLR_BOLD,
    CLR_RED,
    CLR_GREEN,
    CLR_YELLOW,
    CLR_BLUE,
    CLR_CYAN,
    CLR_MAGENTA,
    CLR_DIM,
} color_t;

void color_init(bool force_color);
const char *clr(color_t c);

/* ── Config space access ─────────────────────────────────────────── */

int  pci_cfg_read(pci_device_t *dev);
uint8_t  cfg_read8(const pci_device_t *dev, uint16_t off);
uint16_t cfg_read16(const pci_device_t *dev, uint16_t off);
uint32_t cfg_read32(const pci_device_t *dev, uint16_t off);

/* ── Device enumeration ──────────────────────────────────────────── */

int  pci_parse_bdf(const char *str, pci_bdf_t *bdf);
int  pci_enum_devices(pci_device_t **devs, int *count);
void pci_free_devices(pci_device_t *devs, int count);
int  pci_open_device(pci_device_t *dev, const pci_bdf_t *bdf);

/* ── Header parsing ──────────────────────────────────────────────── */

void pci_parse_header(pci_device_t *dev);
void pci_parse_bars(pci_device_t *dev);
void pci_parse_caps(pci_device_t *dev);

/* ── Display / decode ────────────────────────────────────────────── */

void show_device_summary(const pci_device_t *dev);
void show_header(const pci_device_t *dev);
void show_bars(const pci_device_t *dev);
void show_caps(const pci_device_t *dev);
void show_cap_detail(const pci_device_t *dev, const pci_cap_t *cap);
void show_hexdump(const pci_device_t *dev);
void show_all(const pci_device_t *dev);

/* ── Capability decoders ─────────────────────────────────────────── */

/* Standard capability decoders */
void decode_pm_cap(const pci_device_t *dev, uint16_t off);
void decode_msi_cap(const pci_device_t *dev, uint16_t off);
void decode_msix_cap(const pci_device_t *dev, uint16_t off);
void decode_pcie_cap(const pci_device_t *dev, uint16_t off);
void decode_vpd_cap(const pci_device_t *dev, uint16_t off);
void decode_pcix_cap(const pci_device_t *dev, uint16_t off);
void decode_vendor_cap(const pci_device_t *dev, uint16_t off);
void decode_ssvid_cap(const pci_device_t *dev, uint16_t off);
void decode_af_cap(const pci_device_t *dev, uint16_t off);

/* Extended capability decoders */
void decode_aer_cap(const pci_device_t *dev, uint16_t off);
void decode_dsn_cap(const pci_device_t *dev, uint16_t off);
void decode_sriov_cap(const pci_device_t *dev, uint16_t off);
void decode_ats_cap(const pci_device_t *dev, uint16_t off);
void decode_pasid_cap(const pci_device_t *dev, uint16_t off);
void decode_pri_cap(const pci_device_t *dev, uint16_t off);
void decode_ide_cap(const pci_device_t *dev, uint16_t off);
void decode_doe_cap(const pci_device_t *dev, uint16_t off);
void decode_acs_cap(const pci_device_t *dev, uint16_t off);
void decode_ari_cap(const pci_device_t *dev, uint16_t off);
void decode_dpc_cap(const pci_device_t *dev, uint16_t off);
void decode_l1pm_cap(const pci_device_t *dev, uint16_t off);
void decode_ptm_cap(const pci_device_t *dev, uint16_t off);
void decode_ltr_cap(const pci_device_t *dev, uint16_t off);
void decode_dvsec_cap(const pci_device_t *dev, uint16_t off);
void decode_rebar_cap(const pci_device_t *dev, uint16_t off);
void decode_tph_cap(const pci_device_t *dev, uint16_t off);
void decode_secpci_cap(const pci_device_t *dev, uint16_t off);
void decode_dlnk_cap(const pci_device_t *dev, uint16_t off);
void decode_16gt_cap(const pci_device_t *dev, uint16_t off);
void decode_lmr_cap(const pci_device_t *dev, uint16_t off);
void decode_32gt_cap(const pci_device_t *dev, uint16_t off);
void decode_vc_cap(const pci_device_t *dev, uint16_t off);
void decode_pwr_cap(const pci_device_t *dev, uint16_t off);
void decode_mcast_cap(const pci_device_t *dev, uint16_t off);
void decode_vendor_ext_cap(const pci_device_t *dev, uint16_t off);

/* ── Lookup tables ───────────────────────────────────────────────── */

const char *pci_class_name(uint8_t class_code, uint8_t subclass);
const char *pci_cap_name(uint8_t id);
const char *pci_ext_cap_name(uint16_t id);
const char *pci_exp_type_name(uint8_t type);
const char *pci_link_speed_name(uint8_t speed);

#endif /* PCIEX_H */
