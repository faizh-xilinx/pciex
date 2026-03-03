#include "pciex.h"
#include <stdio.h>
#include <unistd.h>

bool is_root(void)
{
    return geteuid() == 0;
}

bool require_root(const char *feature)
{
    if (is_root())
        return true;
    fprintf(stderr, "%s%s%s requires root access. Run: %ssudo pciex ...%s\n",
            clr(CLR_YELLOW), feature, clr(CLR_RESET),
            clr(CLR_BOLD), clr(CLR_RESET));
    return false;
}

bool confirm_destructive(const char *action, const pci_bdf_t *bdf, bool force)
{
    if (!force) {
        fprintf(stderr, "Destructive operation requires %s--force%s flag\n",
                clr(CLR_RED), clr(CLR_RESET));
        return false;
    }

    char buf[8] = {0};
    fprintf(stderr, "%sWARNING%s: %s on %04x:%02x:%02x.%x. Continue? [y/N] ",
            clr(CLR_RED), clr(CLR_RESET), action,
            bdf->domain, bdf->bus, bdf->dev, bdf->func);
    fflush(stderr);

    if (!fgets(buf, sizeof(buf), stdin))
        return false;

    return (buf[0] == 'y' || buf[0] == 'Y');
}
