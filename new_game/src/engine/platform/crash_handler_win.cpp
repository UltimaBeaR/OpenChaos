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

// Shared flag from host.cpp — prevents multiple handlers from overwriting crash_log.txt.
extern volatile bool g_exit_log_written;

static void write_crash_timestamp(FILE* f, const char* label)
{
    time_t raw = time(nullptr);
    struct tm* lt = localtime(&raw);
    fprintf(f, "%s at %04d-%02d-%02d %02d:%02d:%02d\n\n",
            label,
            lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday,
            lt->tm_hour, lt->tm_min, lt->tm_sec);
}

static LONG WINAPI crash_exception_handler(EXCEPTION_POINTERS* ep)
{
    g_exit_log_written = true;

    // For stack overflow: use a small static buffer and WriteFile as fallback,
    // since fprintf/localtime may need stack space we don't have.
    HANDLE hFile = CreateFileA("crash_log.txt", GENERIC_WRITE, 0, nullptr,
                               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return EXCEPTION_CONTINUE_SEARCH;

    // Minimal header via WriteFile (no CRT, minimal stack usage).
    DWORD code = ep->ExceptionRecord->ExceptionCode;
    {
        // Format: "Crash (exception)\nException: 0xNNNNNNNN\nAddress: 0xNNNN\n"
        char buf[256];
        int len = 0;

        // Timestamp via GetLocalTime (Win32 API, no CRT)
        SYSTEMTIME st;
        GetLocalTime(&st);
        len = wsprintfA(buf, "Crash (exception) at %04d-%02d-%02d %02d:%02d:%02d\r\n\r\n",
                        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
        DWORD written;
        WriteFile(hFile, buf, len, &written, nullptr);

        len = wsprintfA(buf, "Exception: 0x%08lX (%s)\r\n",
                        code,
                        code == EXCEPTION_ACCESS_VIOLATION ? "ACCESS_VIOLATION" :
                        code == EXCEPTION_INT_DIVIDE_BY_ZERO ? "INT_DIVIDE_BY_ZERO" :
                        code == EXCEPTION_STACK_OVERFLOW ? "STACK_OVERFLOW" :
                        "OTHER");
        WriteFile(hFile, buf, len, &written, nullptr);

        len = wsprintfA(buf, "Address: 0x%p\r\n", ep->ExceptionRecord->ExceptionAddress);
        WriteFile(hFile, buf, len, &written, nullptr);
    }

    // Now switch to FILE* for the rest (registers, stack walk) — if this crashes,
    // at least the header above is already on disk.
    FlushFileBuffers(hFile);
    CloseHandle(hFile);

    FILE* f = fopen("crash_log.txt", "a"); // append to what we wrote above
    if (!f) return EXCEPTION_CONTINUE_SEARCH;

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

// Console event handler: catches Ctrl+C, console window close, logoff, shutdown.
static BOOL WINAPI console_ctrl_handler(DWORD ctrl_type)
{
    if (g_exit_log_written) return FALSE;
    g_exit_log_written = true;

    const char* desc =
        ctrl_type == CTRL_C_EVENT        ? "Ctrl+C" :
        ctrl_type == CTRL_BREAK_EVENT    ? "Ctrl+Break" :
        ctrl_type == CTRL_CLOSE_EVENT    ? "Console window closed" :
        ctrl_type == CTRL_LOGOFF_EVENT   ? "User logoff" :
        ctrl_type == CTRL_SHUTDOWN_EVENT ? "System shutdown" : "Unknown console event";

    FILE* f = fopen("crash_log.txt", "w");
    if (f) {
        write_crash_timestamp(f, "Terminated (console event)");
        fprintf(f, "Type: %s (code %lu)\n", desc, (unsigned long)ctrl_type);
        fflush(f);
        fclose(f);
    }

    return FALSE; // Let default handler terminate the process
}

extern "C" void install_crash_handler(void)
{
    SetUnhandledExceptionFilter(crash_exception_handler);
    SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
}

#endif // _WIN32
