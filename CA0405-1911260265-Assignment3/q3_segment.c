/* Q3: translate (segment, offset) addresses with a segment table. */
#include <stdio.h>

struct segment {
    const char *name;
    unsigned base;
    unsigned limit;
};

static struct segment table[] = {
    {"code",  4096,  2048},
    {"data",  16384, 4096},
    {"stack", 65536, 8192},
    {"heap",  32768, 6144},
};

static void translate(unsigned seg, unsigned offset)
{
    if (seg >= sizeof table / sizeof table[0]) {
        printf("(%u, %5u) -> invalid segment\n", seg, offset);
        return;
    }
    if (offset >= table[seg].limit) {
        printf("(%u, %5u) -> segmentation fault: offset %u >= limit %u of %s\n",
               seg, offset, offset, table[seg].limit, table[seg].name);
        return;
    }
    printf("(%u, %5u) -> %s base %u + %u = physical %u\n",
           seg, offset, table[seg].name, table[seg].base, offset,
           table[seg].base + offset);
}

int main(void)
{
    unsigned pairs[][2] = {
        {0, 0}, {0, 2047}, {0, 2048},
        {1, 100}, {1, 4095},
        {2, 700}, {3, 6000}, {3, 6144},
    };
    for (unsigned i = 0; i < sizeof pairs / sizeof pairs[0]; i++)
        translate(pairs[i][0], pairs[i][1]);
    return 0;
}
