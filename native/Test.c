#include "Ultrawide.c"
void TestMain(void) {
    BYTE *memory = VirtualAlloc(0, 8192, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (!memory) ExitProcess(10);
    BYTE *found = 0;
    for (SIZE_T i = 0; i < sizeof(barsPattern); ++i) memory[64+i] = barsPattern[i];
    DWORD old;
    if (!VirtualProtect(memory+4096, 4096, PAGE_NOACCESS, &old)) ExitProcess(11);
    if (Find(memory, 8192, barsPattern, sizeof(barsPattern), &found) != 1 || found != memory+64) ExitProcess(12);
    for (SIZE_T i = 0; i < sizeof(barsPattern); ++i) memory[128+i] = barsPattern[i];
    if (Find(memory, 8192, barsPattern, sizeof(barsPattern), &found) != 2) ExitProcess(13);
    memory[128] = 0;
    BYTE zero = 0;
    if (!WritePatch(memory+74, &zero, 1) || memory[74] != 0) ExitProcess(14);
    MEMORY_BASIC_INFORMATION m;
    VirtualQuery(memory, &m, sizeof(m));
    if (m.Protect != PAGE_READWRITE) ExitProcess(15);
    if (Find(memory, 8192, barsPattern, sizeof(barsPattern), &found) != 0) ExitProcess(16);
    double fov = HorizontalFov(3440,1440);
    if (fov < 106.687782 || fov > 106.687784) ExitProcess(17);
    double wide = HorizontalFov(5120,1440);
    if (wide < 126.86 || wide > 126.88) ExitProcess(18);
    VirtualFree(memory, 0, MEM_RELEASE);
    ExitProcess(0);
}
