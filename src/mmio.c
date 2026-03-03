#include "pciex.h"
#include "pci_regs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <inttypes.h>

#define MAX_MMIO_MAPS 8
#define MAP_FREE ((void *)0)

static struct {
    void *ret_addr;
    void *base;
    size_t map_len;
} mmio_maps[MAX_MMIO_MAPS];

static long pagesize;

static void init_pagesize(void)
{
    if (pagesize == 0)
        pagesize = sysconf(_SC_PAGESIZE);
}

static int get_bar_phys_addr(const pci_device_t *dev, int bar_index, uint64_t *out_start)
{
    char path[512];
    FILE *fp;
    char line[256];
    int line_num = 0;

    snprintf(path, sizeof(path), "%s/resource", dev->sysfs_path);
    fp = fopen(path, "r");
    if (!fp)
        return -1;

    while (fgets(line, sizeof(line), fp) && line_num <= bar_index) {
        uint64_t start = 0, end = 0, flags = 0;

        if (sscanf(line, "%" SCNx64 " %" SCNx64 " %" SCNx64, &start, &end, &flags) != 3) {
            line_num++;
            continue;
        }

        if (line_num == bar_index) {
            fclose(fp);
            *out_start = start;
            return 0;
        }
        line_num++;
    }

    fclose(fp);
    return -1;
}

static int find_free_map_slot(void)
{
    for (int i = 0; i < MAX_MMIO_MAPS; i++)
        if (mmio_maps[i].ret_addr == MAP_FREE)
            return i;
    return -1;
}

static void store_map(void *ret_addr, void *base, size_t map_len)
{
    int i = find_free_map_slot();
    if (i >= 0) {
        mmio_maps[i].ret_addr = ret_addr;
        mmio_maps[i].base = base;
        mmio_maps[i].map_len = map_len;
    }
}

static void clear_map(void *ret_addr)
{
    for (int i = 0; i < MAX_MMIO_MAPS; i++) {
        if (mmio_maps[i].ret_addr == ret_addr) {
            mmio_maps[i].ret_addr = MAP_FREE;
            return;
        }
    }
}

void *mmio_map_bar(const pci_device_t *dev, int bar_index, size_t offset, size_t length)
{
    char path[512];
    int fd;
    void *base;
    size_t map_offset, map_extra, map_len;
    void *ret;

    init_pagesize();
    if (pagesize <= 0 || length == 0)
        return NULL;

    map_offset = offset & ~(pagesize - 1);
    map_extra = offset - map_offset;
    map_len = ((length + map_extra + pagesize - 1) / pagesize) * pagesize;

    snprintf(path, sizeof(path), "%s/resource%d", dev->sysfs_path, bar_index);
    fd = open(path, O_RDWR | O_SYNC);
    if (fd >= 0) {
        base = mmap(NULL, map_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, (off_t)map_offset);
        close(fd);
        if (base != MAP_FAILED) {
            ret = (char *)base + map_extra;
            store_map(ret, base, map_len);
            return ret;
        }
    }

    uint64_t phys_start;
    if (get_bar_phys_addr(dev, bar_index, &phys_start) < 0)
        return NULL;

    fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0)
        return NULL;

    base = mmap(NULL, map_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
                (off_t)(phys_start + map_offset));
    close(fd);
    if (base == MAP_FAILED)
        return NULL;

    ret = (char *)base + map_extra;
    store_map(ret, base, map_len);
    return ret;
}

void mmio_unmap(void *addr, size_t length)
{
    (void)length;

    for (int i = 0; i < MAX_MMIO_MAPS; i++) {
        if (mmio_maps[i].ret_addr == addr) {
            munmap(mmio_maps[i].base, mmio_maps[i].map_len);
            clear_map(addr);
            return;
        }
    }
}

int mmio_read_region(const pci_device_t *dev, int bar_index, size_t offset, size_t length, uint8_t *buf)
{
    void *addr = mmio_map_bar(dev, bar_index, offset, length);
    if (!addr)
        return -1;
    memcpy(buf, addr, length);
    mmio_unmap(addr, length);
    return 0;
}

static const pci_cap_t *find_msix_cap(const pci_device_t *dev)
{
    for (int i = 0; i < dev->num_caps; i++) {
        if (!dev->caps[i].is_extended && dev->caps[i].id == PCI_CAP_ID_MSIX)
            return &dev->caps[i];
    }
    return NULL;
}

void show_msix_table(const pci_device_t *dev)
{
    if (!require_root("MSI-X table dump"))
        return;

    const pci_cap_t *cap = find_msix_cap(dev);
    if (!cap) {
        printf("No MSI-X capability found\n");
        return;
    }

    uint16_t flags = cfg_read16(dev, cap->offset + PCI_MSIX_FLAGS);
    uint32_t table_reg = cfg_read32(dev, cap->offset + PCI_MSIX_TABLE);
    int bir = table_reg & PCI_MSIX_BIR_MASK;
    size_t table_byte_offset = ((table_reg >> 3) & 0x1FFFFFF) * 4;
    int table_size = (flags & PCI_MSIX_FLAGS_TBLSIZE) + 1;
    size_t map_len = (size_t)table_size * 16;

    void *table = mmio_map_bar(dev, bir, table_byte_offset, map_len);
    if (!table) {
        printf("Failed to map MSI-X table\n");
        return;
    }

    printf("\n%s── MSI-X Table (%d entries) ──%s\n", clr(CLR_BOLD), table_size, clr(CLR_RESET));

    for (int i = 0; i < table_size; i++) {
        uint32_t *ent = (uint32_t *)((char *)table + (size_t)i * 16);
        uint32_t addr_lo = ent[0];
        uint32_t addr_hi = ent[1];
        uint32_t data = ent[2];
        uint32_t ctrl = ent[3];
        uint64_t addr = ((uint64_t)addr_hi << 32) | addr_lo;
        bool masked = (ctrl & 1) != 0;

        printf("  [%2d] addr=0x%016" PRIx64 "  data=0x%04x  %s\n",
               i, addr, data & 0xFFFF, masked ? "masked" : "unmasked");
    }

    mmio_unmap(table, map_len);
}

static void hexdump_region(const uint8_t *buf, size_t offset, size_t length)
{
    for (size_t i = 0; i < length; i += 16) {
        printf("  %s%03zx:%s ", clr(CLR_DIM), offset + i, clr(CLR_RESET));
        for (size_t j = 0; j < 16 && (i + j) < length; j++) {
            if (j == 8)
                printf(" ");
            uint8_t b = buf[i + j];
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

void show_mmio_dump(const pci_device_t *dev, int bar_index, size_t offset, size_t length)
{
    if (!require_root("MMIO dump"))
        return;

    uint8_t *buf = malloc(length);
    if (!buf) {
        fprintf(stderr, "Failed to allocate buffer\n");
        return;
    }

    if (mmio_read_region(dev, bar_index, offset, length, buf) < 0) {
        fprintf(stderr, "Failed to read MMIO region\n");
        free(buf);
        return;
    }

    printf("\n%s── MMIO BAR%d @ +0x%zx (%zu bytes) ──%s\n",
           clr(CLR_BOLD), bar_index, offset, length, clr(CLR_RESET));
    hexdump_region(buf, offset, length);
    free(buf);
}
