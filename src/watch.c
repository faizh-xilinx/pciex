#include "pciex.h"
#include "pci_regs.h"
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>

#define CFG_MAX 4096

static volatile sig_atomic_t running = 1;

static void sigint_handler(int sig)
{
    (void)sig;
    running = 0;
}

static bool cap_filter_to_range(const pci_device_t *dev, const char *filter,
                                uint16_t *start, uint16_t *end)
{
    uint16_t id;
    bool is_ext;
    size_t len = strlen(filter);

    if (strncasecmp(filter, "AER", len) == 0) {
        id = PCI_EXT_CAP_ID_AER;
        is_ext = true;
    } else if (strncasecmp(filter, "SRIOV", len) == 0) {
        id = PCI_EXT_CAP_ID_SRIOV;
        is_ext = true;
    } else if (strncasecmp(filter, "MSIX", len) == 0) {
        id = PCI_CAP_ID_MSIX;
        is_ext = false;
    } else if (strncasecmp(filter, "IDE", len) == 0) {
        id = PCI_EXT_CAP_ID_IDE;
        is_ext = true;
    } else if (strncasecmp(filter, "DOE", len) == 0) {
        id = PCI_EXT_CAP_ID_DOE;
        is_ext = true;
    } else if (strncasecmp(filter, "PCIE", len) == 0 || strncasecmp(filter, "LNKSTA", len) == 0) {
        id = PCI_CAP_ID_EXP;
        is_ext = false;
    } else {
        return false;
    }

    for (int i = 0; i < dev->num_caps; i++) {
        const pci_cap_t *cap = &dev->caps[i];
        if (cap->is_extended != is_ext || cap->id != (id & 0xFF))
            continue;
        *start = cap->offset;
        if (is_ext) {
            uint32_t hdr = cfg_read32(dev, cap->offset);
            uint16_t next = (hdr & PCI_EXT_CAP_NEXT_MASK) >> 20;
            *end = next ? next : (uint16_t)dev->cfg_size;
        } else {
            uint16_t end_off = cap->offset + 64;
            *end = end_off < dev->cfg_size ? end_off : (uint16_t)dev->cfg_size;
        }
        return true;
    }
    return false;
}

void watch_device(pci_device_t *dev, const char *cap_filter, int interval_sec)
{
    uint8_t prev[CFG_MAX];
    uint16_t cmp_start = 0;
    uint16_t cmp_end = (uint16_t)dev->cfg_size;

    signal(SIGINT, sigint_handler);
    running = 1;

    if (cap_filter && cap_filter[0] != '\0') {
        if (cap_filter_to_range(dev, cap_filter, &cmp_start, &cmp_end))
            printf("Watching cap range 0x%03x–0x%03x\n", cmp_start, cmp_end);
        else
            printf("Cap '%s' not found, watching full config\n", cap_filter);
    }

    memcpy(prev, dev->cfg, dev->cfg_size);

    while (running) {
        sleep((unsigned)interval_sec);
        if (!running)
            break;

        if (pci_cfg_read(dev) < 0)
            continue;

        int changes = 0;
        uint16_t end = cmp_end < dev->cfg_size ? cmp_end : (uint16_t)dev->cfg_size;

        for (uint16_t off = cmp_start; off < end; off++) {
            if (dev->cfg[off] != prev[off]) {
                if (changes == 0) {
                    time_t now = time(NULL);
                    struct tm *tm = localtime(&now);
                    char buf[32];
                    strftime(buf, sizeof(buf), "%H:%M:%S", tm);
                    printf("[%s] ", buf);
                }
                printf("%s0x%03x: 0x%02x -> 0x%02x%s ",
                       clr(CLR_RED), off, prev[off], dev->cfg[off], clr(CLR_RESET));
                changes++;
            }
        }

        if (changes == 0) {
            time_t now = time(NULL);
            struct tm *tm = localtime(&now);
            char buf[32];
            strftime(buf, sizeof(buf), "%H:%M:%S", tm);
            printf("%s[%s] no changes%s\n", clr(CLR_DIM), buf, clr(CLR_RESET));
        } else {
            printf("\n");
        }

        memcpy(prev, dev->cfg, dev->cfg_size);
    }

    printf("Watch stopped\n");
}
