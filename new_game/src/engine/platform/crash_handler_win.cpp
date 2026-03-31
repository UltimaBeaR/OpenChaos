// Windows crash handler: uses SetUnhandledExceptionFilter for detailed crash info.
// Isolated in a separate TU because windows.h conflicts with project type definitions
// (ULONG = unsigned long vs uint32_t, DWORD = unsigned long vs unsigned int).
#ifdef _WIN32

#include <windows.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")

#include <stdio.h>
#include <stdint.h>
#include <time.h>

static LONG WINAPI crash_exception_handler(EXCEPTION_POINTERS* ep)
{
    FILE* f = fopen("crash_log.txt", "w");
    if (!f) return EXCEPTION_CONTINUE_SEARCH;

    time_t raw = time(nullptr);
    struct tm* lt = localtime(&raw);
    fprintf(f, "Crash at %04d-%02d-%02d %02d:%02d:%02d\n\n",
            lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday,
            lt->tm_hour, lt->tm_min, lt->tm_sec);

    DWORD code = ep->ExceptionRecord->ExceptionCode;
    fprintf(f, "Exception: 0x%08lX (%s)\n",
            code,
            code == EXCEPTION_ACCESS_VIOLATION ? "ACCESS_VIOLATION" :
            code == EXCEPTION_INT_DIVIDE_BY_ZERO ? "INT_DIVIDE_BY_ZERO" :
            code == EXCEPTION_STACK_OVERFLOW ? "STACK_OVERFLOW" :
            "OTHER");
    fprintf(f, "Address: 0x%p\n", ep->ExceptionRecord->ExceptionAddress);

    // RVA = crash address - module base
    HMODULE hMod = GetModuleHandle(nullptr);
    uintptr_t base = (uintptr_t)hMod;
    uintptr_t addr = (uintptr_t)ep->ExceptionRecord->ExceptionAddress;
    fprintf(f, "Module base: 0x%p\n", (void*)base);
    fprintf(f, "RVA: 0x%llx\n", (unsigned long long)(addr - base));

    // Access violation details
    if (code == EXCEPTION_ACCESS_VIOLATION && ep->ExceptionRecord->NumberParameters >= 2) {
        fprintf(f, "Access type: %s, target address: 0x%p\n",
                ep->ExceptionRecord->ExceptionInformation[0] == 0 ? "READ" :
                ep->ExceptionRecord->ExceptionInformation[0] == 1 ? "WRITE" : "EXECUTE",
                (void*)ep->ExceptionRecord->ExceptionInformation[1]);
    }

    // Registers
    CONTEXT* ctx = ep->ContextRecord;
    fprintf(f, "\nRegisters:\n");
    fprintf(f, "RAX=%016llx RBX=%016llx RCX=%016llx RDX=%016llx\n", ctx->Rax, ctx->Rbx, ctx->Rcx, ctx->Rdx);
    fprintf(f, "RSI=%016llx RDI=%016llx RBP=%016llx RSP=%016llx\n", ctx->Rsi, ctx->Rdi, ctx->Rbp, ctx->Rsp);
    fprintf(f, "R8 =%016llx R9 =%016llx R10=%016llx R11=%016llx\n", ctx->R8, ctx->R9, ctx->R10, ctx->R11);
    fprintf(f, "R12=%016llx R13=%016llx R14=%016llx R15=%016llx\n", ctx->R12, ctx->R13, ctx->R14, ctx->R15);
    fprintf(f, "RIP=%016llx\n", ctx->Rip);

    // Stack walk with symbol names
    fprintf(f, "\nStack trace:\n");
    HANDLE hProcess = GetCurrentProcess();
    HANDLE hThread = GetCurrentThread();
    SymInitialize(hProcess, nullptr, TRUE);

    STACKFRAME64 sf = {};
    sf.AddrPC.Offset = ctx->Rip;
    sf.AddrPC.Mode = AddrModeFlat;
    sf.AddrFrame.Offset = ctx->Rbp;
    sf.AddrFrame.Mode = AddrModeFlat;
    sf.AddrStack.Offset = ctx->Rsp;
    sf.AddrStack.Mode = AddrModeFlat;

    for (int i = 0; i < 32; i++) {
        if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64, hProcess, hThread, &sf, ctx,
                         nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr))
            break;
        uintptr_t frame_rva = sf.AddrPC.Offset - base;
        // Try to resolve symbol name
        char sym_buf[sizeof(SYMBOL_INFO) + 256];
        SYMBOL_INFO* sym = (SYMBOL_INFO*)sym_buf;
        sym->SizeOfStruct = sizeof(SYMBOL_INFO);
        sym->MaxNameLen = 255;
        DWORD64 displacement = 0;
        if (SymFromAddr(hProcess, sf.AddrPC.Offset, &displacement, sym)) {
            fprintf(f, "  [%2d] RVA 0x%08llx  %s+0x%llx\n", i,
                    (unsigned long long)frame_rva, sym->Name, (unsigned long long)displacement);
        } else {
            fprintf(f, "  [%2d] RVA 0x%08llx\n", i, (unsigned long long)frame_rva);
        }
    }

    SymCleanup(hProcess);
    fflush(f);
    fclose(f);
    return EXCEPTION_CONTINUE_SEARCH;
}

extern "C" void install_crash_handler(void)
{
    SetUnhandledExceptionFilter(crash_exception_handler);
}

#endif // _WIN32
