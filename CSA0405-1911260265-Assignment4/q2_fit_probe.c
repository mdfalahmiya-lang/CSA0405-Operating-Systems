#include <stdio.h>
#include <string.h>

#define BLOCKS 32

char disk[BLOCKS];

int alloc_contiguous(char file, int size)
{
    for (int start = 0; start + size <= BLOCKS; start++) {
        int fits = 1;
        for (int k = 0; k < size; k++) {
            if (disk[start + k] != '.') { fits = 0; break; }
        }
        if (!fits) continue;
        for (int k = 0; k < size; k++) disk[start + k] = file;
        return start;
    }
    return -1;
}

int alloc_indexed(char file, int size, int index[])
{
    int got = 0, idx = -1;
    for (int b = 0; b < BLOCKS && got <= size; b++) {
        if (disk[b] != '.') continue;
        if (idx < 0) idx = b;
        else index[got++] = b;
        if (got == size) break;
    }
    if (idx < 0 || got < size) return -1;
    disk[idx] = (char)(file + 32);
    for (int i = 0; i < size; i++) disk[index[i]] = file;
    return idx;
}

void release(char file)
{
    for (int b = 0; b < BLOCKS; b++) if (disk[b] == file) disk[b] = '.';
}

void reset_state(void)
{
    memset(disk, '.', BLOCKS);
    alloc_contiguous('A', 6);
    alloc_contiguous('B', 8);
    alloc_contiguous('C', 5);
    alloc_contiguous('D', 7);
    release('B');
    release('D');
}

void free_stats(int *total, int *largest)
{
    *total = 0;
    *largest = 0;
    int run = 0;
    for (int b = 0; b < BLOCKS; b++) {
        if (disk[b] == '.') {
            (*total)++;
            run++;
            if (run > *largest) *largest = run;
        } else {
            run = 0;
        }
    }
}

int main(void)
{
    int total, largest;

    reset_state();
    free_stats(&total, &largest);
    printf("State after A(6) B(8) C(5) D(7), release B and D\n");
    printf("%.32s\n", disk);
    printf("free blocks: %d   largest contiguous run: %d\n\n", total, largest);

    printf("E(10) contiguous: start %d\n", alloc_contiguous('E', 10));
    printf("%.32s\n\n", disk);

    reset_state();
    printf("E(14) contiguous: start %d\n", alloc_contiguous('E', 14));
    int index[20];
    int ib = alloc_indexed('E', 14, index);
    printf("E(14) indexed: index block %d, data blocks", ib);
    for (int i = 0; i < 14; i++) printf(" %d", index[i]);
    printf("\n%.32s\n", disk);
    return 0;
}
