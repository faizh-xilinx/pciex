#include "pciex.h"
#include <stdio.h>
#include <unistd.h>

static bool use_color;

void color_init(bool force_color)
{
    use_color = force_color || isatty(STDOUT_FILENO);
}

static const char *color_codes[] = {
    [CLR_RESET]   = "\033[0m",
    [CLR_BOLD]    = "\033[1m",
    [CLR_RED]     = "\033[31;1m",
    [CLR_GREEN]   = "\033[32;1m",
    [CLR_YELLOW]  = "\033[33;1m",
    [CLR_BLUE]    = "\033[34;1m",
    [CLR_CYAN]    = "\033[36;1m",
    [CLR_MAGENTA] = "\033[35;1m",
    [CLR_DIM]     = "\033[2m",
};

const char *clr(color_t c)
{
    if (!use_color)
        return "";
    return color_codes[c];
}
