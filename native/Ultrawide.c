#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>

static HMODULE self;
static LONG started;
static WCHAR ini[MAX_PATH], logpath[MAX_PATH];
static const BYTE barsPattern[] = {0x48,0x81,0xEC,0xD0,0,0,0,0xF6,0x41,0x2C,1};
static const BYTE fovPattern[] = {0x35,0xFA,0x0E,0x3C,0xD8,0xF5,0x7F,0x3F};
static void InstallUIHooks(void);

static void Log(const char *s) {
    HANDLE f = CreateFileW(logpath, FILE_APPEND_DATA, FILE_SHARE_READ, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (f != INVALID_HANDLE_VALUE) { DWORD written; WriteFile(f, s, (DWORD)lstrlenA(s), &written, 0); CloseHandle(f); }
}
static void Paths(void) {
    GetModuleFileNameW(self, ini, MAX_PATH);
    DWORD i = (DWORD)lstrlenW(ini);
    while (i && ini[i-1] != L'\\') --i;
    ini[i] = 0;
    lstrcpyW(logpath, ini);
    lstrcatW(ini, L"Ace7Ultrawide.ini");
    lstrcatW(logpath, L"Ace7Ultrawide.log");
}
#include "StandaloneUI.c"
static int Match(const BYTE *p, const BYTE *pattern, SIZE_T size) {
    for (SIZE_T i = 0; i < size; ++i) if (p[i] != pattern[i]) return 0;
    return 1;
}
/* Scan only committed, readable image pages. Encrypted code becomes visible
   after SteamStub finishes unpacking; all signatures must be unique. */
static int Find(BYTE *base, SIZE_T size, const BYTE *pattern, SIZE_T length, BYTE **found) {
    int count = 0;
    BYTE *end = base + size;
    for (BYTE *p = base; p < end;) {
        MEMORY_BASIC_INFORMATION m;
        if (!VirtualQuery(p, &m, sizeof(m))) break;
        BYTE *next = (BYTE *)m.BaseAddress + m.RegionSize;
        BYTE *limit = next < end ? next : end;
        DWORD access = m.Protect & 0xFF;
        if (m.State == MEM_COMMIT && !(m.Protect & PAGE_GUARD) &&
            (access == PAGE_READONLY || access == PAGE_READWRITE || access == PAGE_WRITECOPY ||
             access == PAGE_EXECUTE_READ || access == PAGE_EXECUTE_READWRITE || access == PAGE_EXECUTE_WRITECOPY)) {
            for (; p + length <= limit; ++p) {
                if (*p == *pattern && Match(p, pattern, length)) { *found = p; if (++count > 1) return count; }
            }
        }
        if (next <= p) break;
        p = next;
    }
    return count;
}
static int WritePatch(BYTE *p, const BYTE *value, SIZE_T length) {
    DWORD old, ignored;
    if (!VirtualProtect(p, length, PAGE_EXECUTE_READWRITE, &old)) return 0;
    for (SIZE_T i = 0; i < length; ++i) p[i] = value[i];
    FlushInstructionCache(GetCurrentProcess(), p, length);
    if (!VirtualProtect(p, length, old, &ignored)) Log("Warning: could not restore page protection.\r\n");
    return Match(p, value, length);
}
/* atan(x) for ultrawide x>1, transformed into a rapidly converging series. */
static double HorizontalFov(int width, int height) {
    double x = (double)width / height * 9.0 / 16.0;
    double z = (x - 1.0) / (x + 1.0), term = z, sum = z;
    for (int i = 1; i < 48; ++i) { term *= -z*z; sum += term / (2*i+1); }
    return 2.0 * (0.78539816339744830962 + sum) * 57.2957795130823208768;
}
static DWORD WINAPI Worker(void *unused) {
    (void)unused;
    Paths();
    Log("Ace7Ultrawide v0.12.0-rc.1: standalone native runtime patch starting.\r\n");
    if (!GetPrivateProfileIntW(L"Ultrawide", L"Enabled", 1, ini)) { Log("Disabled in INI.\r\n"); return 0; }
    int width = GetPrivateProfileIntW(L"Ultrawide", L"Width", 3440, ini);
    int height = GetPrivateProfileIntW(L"Ultrawide", L"Height", 1440, ini);
    if (width < 1 || height < 1 || (double)width / height <= 16.0/9.0 || (double)width / height > 4.0) {
        Log("Unsupported resolution; no patches applied.\r\n"); return 0;
    }
    BYTE *base = (BYTE *)GetModuleHandleW(0);
    IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)base;
    IMAGE_NT_HEADERS64 *nt = (IMAGE_NT_HEADERS64 *)(base + dos->e_lfanew);
    SIZE_T size = nt->OptionalHeader.SizeOfImage;
    int value = (int)(129591.0 * HorizontalFov(width, height) - 10681633.0 + 0.5);
    BYTE fov[3] = {(BYTE)value, (BYTE)(value >> 8), (BYTE)(value >> 16)}, zero = 0;
    if (value < 0 || value > 0xFFFFFF) { Log("FOV out of range.\r\n"); return 0; }
    for (int attempt = 0; attempt < 300; ++attempt) {
        BYTE *bars = 0, *angle = 0;
        int b = Find(base, size, barsPattern, sizeof(barsPattern), &bars);
        int f = Find(base, size, fovPattern, sizeof(fovPattern), &angle);
        if (b > 1 || f > 1) { Log("Ambiguous signature; no patches applied.\r\n"); return 0; }
        if (b == 1 && f == 1) {
            /* Recheck both immediately before modifying memory. */
            if (!Match(bars, barsPattern, sizeof(barsPattern)) || !Match(angle, fovPattern, sizeof(fovPattern))) continue;
            if (!WritePatch(angle, fov, 3)) { Log("FOV write failed.\r\n"); return 0; }
            if (!WritePatch(bars + 10, &zero, 1)) {
                WritePatch(angle, fovPattern, 3);
                Log("Black-bar write failed; FOV rolled back.\r\n"); return 0;
            }
            Log("SUCCESS: black bars disabled and aspect-corrected FOV applied in memory. EXE untouched.\r\n");
            InstallUIHooks();
            return 0;
        }
        Sleep(200);
    }
    Log("Timed out waiting for unpacked signatures; no patches applied.\r\n");
    return 0;
}
__declspec(dllexport) void InitializeASI(void) {
    if (InterlockedCompareExchange(&started, 1, 0) == 0) {
        HANDLE thread = CreateThread(0, 0, Worker, 0, 0, 0);
        if (thread) CloseHandle(thread);
    }
}
BOOL WINAPI DllMain(HINSTANCE module, DWORD reason, LPVOID reserved) {
    (void)reserved;
    if (reason == DLL_PROCESS_ATTACH) {
        self = module;
        DisableThreadLibraryCalls(module);
        InitializeASI();
    }
    return TRUE;
}
/* No CRT dependency; floating point is used only by the worker. */
int _fltused = 0;
