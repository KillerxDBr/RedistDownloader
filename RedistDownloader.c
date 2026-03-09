#if !defined(_WIN32) || !(_WIN32)
#error Windows Only...
#endif

#define NTDDI_VERSION NTDDI_WIN10
#define _WIN32_WINNT  _WIN32_WINNT_WIN10
#define WINVER        _WIN32_WINNT_WIN10

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif // _CRT_SECURE_NO_WARNINGS

#ifndef UNICODE
#define UNICODE
#endif // UNICODE

#ifndef _UNICODE
#define _UNICODE
#endif // _UNICODE

#define STRICT_TYPED_ITEMIDS
#define WIN32_LEAN_AND_MEAN
#define NOGDICAPMASKS     // CC_*, LC_*, PC_*, CP_*, TC_*, RC_
#define NOVIRTUALKEYCODES // VK_*
#define NOWINSTYLES       // WS_*, CS_*, ES_*, LBS_*, SBS_*, CBS_*
#define NOSYSMETRICS      // SM_*
#define NOMENUS           // MF_*
#define NOICONS           // IDI_*
#define NOKEYSTATES       // MK_*
#define NOSYSCOMMANDS     // SC_*
#define NORASTEROPS       // Binary and Tertiary raster ops
#define OEMRESOURCE       // OEM Resource values
#define NOATOM            // Atom Manager routines
#define NOCLIPBOARD       // Clipboard routines
#define NOCOLOR           // Screen colors
#define NODRAWTEXT        // DrawText() and DT_*
#define NOGDI             // All GDI defines and routines
#define NOKERNEL          // All KERNEL defines and routines
#define NOMEMMGR          // GMEM_*, LMEM_*, GHND, LHND, associated routines
#define NOMETAFILE        // typedef METAFILEPICT
#define NOMINMAX          // Macros min(a,b) and max(a,b)
#define NOOPENFILE        // OpenFile(), OemToAnsi, AnsiToOem, and OF_*
#define NOSCROLL          // SB_* and scrolling routines
#define NOSERVICE         // All Service Controller routines, SERVICE_ equates, etc.
#define NOSOUND           // Sound driver routines
#define NOTEXTMETRIC      // typedef TEXTMETRIC and associated routines
#define NOWH              // SetWindowsHook and WH_*
#define NOWINOFFSETS      // GWL_*, GCL_*, associated routines
#define NOCOMM            // COMM driver routines
#define NOKANJI           // Kanji support stuff.
#define NOHELP            // Help engine interface.
#define NOPROFILER        // Profiler interface.
#define NODEFERWINDOWPOS  // DeferWindowPos routines
#define NOMCX             // Modem Configuration Extensions
#define NOIME
#define NOCRYPT
#define NOAPISET
#include <windows.h>

#include <sdkddkver.h>
#include <shlobj.h>
#include <shlwapi.h>

#ifdef strdup
#undef strdup
#endif

#define strdup _strdup

#define FLAGS_CAP 4
#define FLAG_IMPLEMENTATION
#include "flag.h"

#define NOB_IMPLEMENTATION
#include "nob.h"

#include <locale.h>

static char *strndup(const char *str, size_t size) {
    char *result = (char *)malloc(size + 1);
    if (result) {
        memcpy(result, str, size);
        result[size] = '\0';
    }
    return result;
}

#define nob_sv_to_cstr(sv) strndup((sv).data, (sv).count)

typedef int8_t i8;
typedef uint8_t u8;

typedef int16_t i16;
typedef uint16_t u16;

typedef int32_t i32;
typedef uint32_t u32;

typedef int64_t i64;
typedef uint64_t u64;

typedef enum {
    ENTRY_UNDEFINED = 0,

    ENTRY_LINK,
    ENTRY_EXE,

    ENTRY_MAX,
} EntryType;

typedef struct {
    const char *fileName;
    const char *url;
    char **args;
    EntryType type;
} Resource;

typedef struct {
    Resource *items;
    size_t count;
    size_t capacity;
} Resources;

// clang-format off
// https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist
Resource vc_resources[] = {
    {.fileName = "vcredist_2005_x86.exe", .url = "https://download.microsoft.com/download/8/B/4/8B42259F-5D70-43F4-AC2E-4B208FD8D66A/vcredist_x86.EXE"},
    {.fileName = "vcredist_2005_x64.exe", .url = "https://download.microsoft.com/download/8/B/4/8B42259F-5D70-43F4-AC2E-4B208FD8D66A/vcredist_x64.EXE"},
    
    {.fileName = "vcredist_2008_x86.exe", .url = "https://download.microsoft.com/download/5/D/8/5D8C65CB-C849-4025-8E95-C3966CAFD8AE/vcredist_x86.exe"},
    {.fileName = "vcredist_2008_x64.exe", .url = "https://download.microsoft.com/download/5/D/8/5D8C65CB-C849-4025-8E95-C3966CAFD8AE/vcredist_x64.exe"},
    
    {.fileName = "vcredist_2010_x86.exe", .url = "https://download.microsoft.com/download/1/6/5/165255E7-1014-4D0A-B094-B6A430A6BFFC/vcredist_x86.exe"},
    {.fileName = "vcredist_2010_x64.exe", .url = "https://download.microsoft.com/download/1/6/5/165255E7-1014-4D0A-B094-B6A430A6BFFC/vcredist_x64.exe"},
    
    {.fileName = "vcredist_2012_x86.exe", .url = "https://download.microsoft.com/download/1/6/B/16B06F60-3B20-4FF2-B699-5E9B7962F9AE/VSU_4/vcredist_x86.exe"},
    {.fileName = "vcredist_2012_x64.exe", .url = "https://download.microsoft.com/download/1/6/B/16B06F60-3B20-4FF2-B699-5E9B7962F9AE/VSU_4/vcredist_x64.exe"},
    
    {.fileName = "vcredist_2013_x86.exe", .url = "https://aka.ms/highdpimfc2013x86enu"},
    {.fileName = "vcredist_2013_x64.exe", .url = "https://aka.ms/highdpimfc2013x64enu"},
    
    {.fileName = "vcredist_2015_x86.exe", .url = "https://aka.ms/vc14/vc_redist.x86.exe"},
    {.fileName = "vcredist_2015_x64.exe", .url = "https://aka.ms/vc14/vc_redist.x64.exe"},  
};
// clang-format on

#define NUM_URLS NOB_ARRAY_LEN(vc_resources)
#define TMP_DIR  "KxD.VCD.App\\"

typedef struct Flags {
    bool noRedownload;
    bool skipInstall;
} Flags;

static_assert(sizeof(Flags) < FLAGS_CAP, "Increase FLAGS_CAP");

bool open_folder_in_explorer(const char *path);
bool get_java_link(Flags flags);
bool get_extra_resources(Flags flags);
bool IsExeInPath(const char *exe);
i64 get_file_size(const char *path);
bool parse_config_file(void);

Nob_Cmd cmd               = {};
Nob_File_Paths fp         = {};
Resources extra_resources = {};
char program_path[MAX_PATH];

void usage(FILE *stream) {
    fprintf(stream, "Usage: %s [OPTIONS]\n", flag_program_name());
    fprintf(stream, "OPTIONS:\n");
    flag_print_options(stream);
}

int main(int argc, char **argv) {
    int result = 0;
    setlocale(LC_CTYPE, ".UTF8");

    {
        int bkp_argc    = argc;
        char **bkp_argv = argv;

        if (!nob_win32_utf8_cmdline_args(&argc, &argv)) {
            nob_log(NOB_WARNING, "Could not generate Win32 UTF-8 Command Line Arguments, using default...");
            nob_temp_reset();
            argc = bkp_argc;
            argv = bkp_argv;
        }
    }

    bool *skipInstall  = flag_bool("s", false, "Skip instalation");
    bool *noRedownload = flag_bool("nr", false, "Dont Redownload Files");
    bool *help         = flag_bool("h", false, "Print this help to stdout and exit with 0");

    if (!flag_parse(argc, argv)) {
        usage(stderr);
        flag_print_error(stderr);
        system("pause");
        exit(1);
    }

    if (*help) {
        usage(stdout);
        system("pause");
        exit(0);
    }

    Flags flags = {
        .noRedownload = *noRedownload,
        .skipInstall  = *skipInstall,
    };

    {
        wchar_t wPath[MAX_PATH];
        if (!GetModuleFileNameW(NULL, wPath, MAX_PATH)) {
            nob_log(NOB_ERROR, "GetModuleFileNameW: %s", nob_win32_error_message(GetLastError()));
            return 1;
        }

        if (!WideCharToMultiByte(CP_UTF8, WCMBFlags, wPath, -1, program_path, MAX_PATH, NULL, NULL)) {
            nob_log(NOB_ERROR, "WideCharToMultiByte: %s", nob_win32_error_message(GetLastError()));
            return 1;
        }

        *(char *)nob_path_name(program_path) = 0;

        if (!GetTempPathW(MAX_PATH, wPath)) {
            nob_log(NOB_ERROR, "Could not get temp directory: %s", nob_win32_error_message(GetLastError()));
            return 1;
        }

        if (!SetCurrentDirectoryW(wPath)) {
            nob_log(NOB_ERROR, "Could not set current directory to %ls: %s", wPath, nob_win32_error_message(GetLastError()));
            return 1;
        }
    }

    if (!nob_mkdir_if_not_exists(TMP_DIR))
        return 1;
    if (!SetCurrentDirectoryW(L".\\" TEXT(TMP_DIR))) {
        nob_log(NOB_ERROR, "Could not set current directory to .\\" TMP_DIR ": %s", nob_win32_error_message(GetLastError()));
        return 1;
    }
    if (!parse_config_file())
        nob_return_defer(1);
    if (!IsExeInPath("curl")) {
        nob_log(NOB_ERROR, "curl nao encontrado...");
        nob_return_defer(1);
    }

    if (!flags.noRedownload) {
        fp.count = 0;
        if (!nob_read_entire_dir("./", &fp))
            nob_return_defer(1);

        for (size_t i = 0; i < fp.count; ++i) {
            if (strcmp(fp.items[i], ".") == 0 || strcmp(fp.items[i], "..") == 0)
                continue;

            // nob_log(NOB_INFO, "entry: %s", fp.items[i]);

            if (!nob_delete_file(fp.items[i]))
                nob_return_defer(1);
        }
    }

    for (size_t i = 0; i < NUM_URLS; ++i) {
        if (nob_file_exists(vc_resources[i].fileName) > 0 && flags.noRedownload) {
            continue;
        }
        puts("-------------------------------------------------------------------------------------");
        nob_log(NOB_INFO, "File: '%s' -> Link: '%s'", vc_resources[i].fileName, vc_resources[i].url);
        nob_cmd_append(&cmd, "curl", "-L", "-o", vc_resources[i].fileName, vc_resources[i].url);

        bool rst = nob_cmd_run_sync_and_reset(&cmd);
        Sleep(100);
        if (!rst || nob_file_exists(vc_resources[i].fileName) == 0 || get_file_size(vc_resources[i].fileName) < 1) {
            nob_log(NOB_ERROR, "Could not download \"%s\"", vc_resources[i].fileName);
            if (get_file_size(vc_resources[i].fileName) < 1) {
                // nob_delete_file(vc_resources[i].fileName);
                nob_return_defer(1);
            }
        }
    }

    if (!(flags.skipInstall)) {
        for (size_t i = 0; i < NUM_URLS; ++i) {
            if (nob_file_exists(vc_resources[i].fileName) < 1 && flags.noRedownload)
                continue;
            nob_log(NOB_INFO, "Running '%s'", vc_resources[i].fileName);
            if (i < 4) {
                nob_cmd_append(&cmd, vc_resources[i].fileName, "/Q");
            } else {
                nob_cmd_append(&cmd, vc_resources[i].fileName, "/install", "/quiet", "/norestart");
            }
            if (!nob_cmd_run_sync_and_reset(&cmd)) {
                DWORD err = GetLastError();
                if (err != 0) {
                    nob_log(NOB_ERROR, "Could not run file '%s': %s", vc_resources[i].fileName, nob_win32_error_message(err));
                    nob_delete_file(vc_resources[i].fileName);
                } else {
                    nob_log(NOB_ERROR, "Could not run file '%s': %s", vc_resources[i].fileName, nob_win32_error_message(ERROR_PROCESS_ABORTED));
                }
                // nob_return_defer(1);
            }
            Sleep(500);
        }
    }

    if (!get_java_link(flags))
        nob_return_defer(1);
    if (!get_extra_resources(flags))
        nob_return_defer(1);

    if (flags.skipInstall) {
        if (!open_folder_in_explorer(NULL))
            MessageBoxW(NULL, L"Nao foi possivel abrir pasta de arquivos", L"ERRO", MB_TOPMOST | MB_OK | MB_ICONERROR);
    }
defer:
    for (size_t i = 0; i < extra_resources.count; ++i) {
        if (extra_resources.items[i].fileName)
            free((void *)extra_resources.items[i].fileName);
        if (extra_resources.items[i].url)
            free((void *)extra_resources.items[i].url);
        if (extra_resources.items[i].args)
            free((void *)extra_resources.items[i].args);
    }

    nob_da_free(extra_resources);
    nob_cmd_free(cmd);
    nob_da_free(fp);

    return result;
}

bool open_folder_in_explorer(const char *path) {
    bool result           = true;
    wchar_t *tmp          = NULL;
    PIDLIST_ABSOLUTE pidl = NULL;

    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    wchar_t wstr_path[MAX_PATH];

    if (SUCCEEDED(hr)) {
        if (path != NULL && path[0]) {
            if (!MultiByteToWideChar(CP_UTF8, MBWCFlags, path, -1, wstr_path, MAX_PATH)) {
                nob_log(NOB_ERROR, "MultiByteToWideChar failed: %s", nob_win32_error_message(GetLastError()));
                nob_return_defer(false);
            }
            if (PathIsRelativeW(wstr_path)) {
                errno_t e;
                tmp = _wcsdup(wstr_path);
                assert(tmp != NULL);
                if (!GetCurrentDirectoryW(MAX_PATH, wstr_path)) {
                    nob_log(NOB_ERROR, "GetCurrentDirectoryW failed: %s", nob_win32_error_message(GetLastError()));
                    nob_return_defer(false);
                }

                e = wcscat_s(wstr_path, MAX_PATH, L"\\");
                if (e) {
                    nob_log(NOB_ERROR, "Failed to concatenate wide string: %s", strerror(e));
                    nob_return_defer(false);
                }

                e = wcscat_s(wstr_path, MAX_PATH, tmp);
                if (e) {
                    nob_log(NOB_ERROR, "Failed to concatenate wide string: %s", strerror(e));
                    nob_return_defer(false);
                }

                // wprintf(L"path: \"%s\"\n", wstr_path);
                if (!PathResolve(wstr_path, NULL, 0)) {
                    nob_log(NOB_ERROR, "PathResolve failed: %s", nob_win32_error_message(GetLastError()));
                    nob_return_defer(false);
                }
                // wprintf(L"path: \"%s\"\n", wstr_path);
            }
        } else {
            if (!GetCurrentDirectoryW(MAX_PATH, wstr_path)) {
                nob_log(NOB_ERROR, "GetCurrentDirectoryW failed: %s", nob_win32_error_message(GetLastError()));
                nob_return_defer(false);
            }
        }

        hr = SHParseDisplayName(wstr_path, NULL, &pidl, 0, NULL); // SHParseDisplayName Crash with ASAN
        if (hr == S_OK) {
            SHELLEXECUTEINFOW sei;
            ZeroMemory(&sei, sizeof(sei));

            sei.cbSize   = sizeof(sei);
            sei.fMask    = SEE_MASK_IDLIST;
            sei.lpVerb   = L"explore";
            sei.lpIDList = (void *)pidl;
            sei.nShow    = SW_SHOWDEFAULT;

            if (!ShellExecuteExW(&sei)) {
                nob_log(NOB_ERROR, "ShellExecuteExW falhou: %s", nob_win32_error_message(GetLastError()));
                nob_return_defer(false);
            }
        } else {
            nob_log(NOB_ERROR, "SHParseDisplayName falhou: %s", nob_win32_error_message(GetLastError()));
            nob_return_defer(false);
        }
    } else {
        nob_log(NOB_ERROR, "Nao foi possivel iniciar COM: %s", nob_win32_error_message(GetLastError()));
        nob_return_defer(false);
    }

defer:
    if (tmp)
        free(tmp);
    if (pidl)
        ILFree(pidl);

    CoUninitialize();
    return result;
}

i64 get_file_size(const char *path) {
    i64 result     = 0;
    wchar_t *wPath = NULL;
    HANDLE f       = INVALID_HANDLE_VALUE;

    const size_t mark = nob_temp_save();

    int n = MultiByteToWideChar(CP_UTF8, MBWCFlags, path, -1, NULL, 0);
    if (!n) {
        nob_log(NOB_ERROR, "Failed to convert string to UTF16: %s", nob_win32_error_message(GetLastError()));
        nob_return_defer(-1);
    }

    wPath = nob_temp_alloc(n * sizeof(wchar_t));
    assert(wPath != NULL);

    if (!MultiByteToWideChar(CP_UTF8, MBWCFlags, path, -1, wPath, n)) {
        nob_log(NOB_ERROR, "Failed to convert string to UTF16: %s", nob_win32_error_message(GetLastError()));
        nob_return_defer(-1);
    }

    f = CreateFileW(wPath,                 //
                    FILE_READ_ATTRIBUTES,  //
                    0,                     //
                    NULL,                  //
                    OPEN_EXISTING,         //
                    FILE_ATTRIBUTE_NORMAL, //
                    NULL);                 //

    if (f == INVALID_HANDLE_VALUE) {
        nob_log(NOB_ERROR, "Failed to open file '%s': %s", path, nob_win32_error_message(GetLastError()));
        nob_return_defer(-1);
    }

    LARGE_INTEGER sz = {};
    if (!GetFileSizeEx(f, &sz)) {
        nob_log(NOB_ERROR, "Failed to get file size: %s", nob_win32_error_message(GetLastError()));
        nob_return_defer(-1);
    }

    result = sz.QuadPart;

defer:
    if (f != INVALID_HANDLE_VALUE)
        CloseHandle(f);
    nob_temp_rewind(mark);
    return result;
}

bool IsExeInPath(const char *exe) {
    Nob_Cmd cmdFinder   = {};
    Nob_Cmd_Redirect cr = {};

    Nob_Fd nullOutput = nob_fd_open_for_write("NUL");
    if (nullOutput == NOB_INVALID_FD) {
        nob_log(NOB_ERROR, "Nao foi possivel redirecionar a saida de texto...");
    } else {
        cr.fdout = &nullOutput;
        cr.fderr = &nullOutput;
    }
    nob_cmd_append(&cmdFinder, "where.exe", "/Q", exe);
    bool result = nob_cmd_run_sync_redirect(cmdFinder, cr);

    if (result) {
        nob_log(NOB_INFO, "\"%s\" encontrado no PATH", exe);
    } else {
        nob_log(NOB_ERROR, "\"%s\" nao encontrado no PATH", exe);
    }

    if (nullOutput != NOB_INVALID_FD)
        nob_fd_close(nullOutput);

    nob_cmd_free(cmdFinder);

    return result;
}

#define JDL_FILE "manual.jsp"
#define JDL_URL  "www.java.com/pt-br/download/manual.jsp"

bool get_java_link(Flags flags) {
    bool result = true;

    Nob_String_Builder sb = {};

    const char *x86exe = "java-x86.exe";
    const char *x64exe = "java-x64.exe";

    if (!(nob_file_exists(JDL_FILE) > 0 && flags.noRedownload)) {
        nob_cmd_append(&cmd, "curl", "-L", "-o", JDL_FILE, JDL_URL, "-H", "User-Agent: Wget/1.25.0");
        if (!nob_cmd_run_sync_and_reset(&cmd)) {
            nob_log(NOB_ERROR, "Nao foi possivel fazer o download do arquivo '" JDL_FILE "'");
            nob_return_defer(false);
        }
        Sleep(100);
    }

    int exists = nob_file_exists(JDL_FILE);
    if (exists < 1) {
        nob_log(NOB_ERROR, "Nao foi possivel encontrar o arquivo '" JDL_FILE "': %s", nob_win32_error_message(GetLastError()));
        nob_return_defer(false);
    } else {
        if (!nob_read_entire_file(JDL_FILE, &sb))
            nob_return_defer(false);

        Nob_String_View sv = nob_sv_trim(nob_sb_to_sv(sb));
        // <div class="rw-inpagetab" id="jre8-windows">
        nob_sv_chop_by_sv(&sv, SV("Instruções de instalação do software Java para Windows On-line"));

        Nob_String_View sv2;
        Nob_String_View x86jre = {};
        Nob_String_View x64jre = {};

        while (sv2 = nob_sv_chop_by_sv(&sv, SV("<td><a href=")), sv.count > 0) {
            if (x86jre.data != NULL && x64jre.data != NULL)
                break;
            sv2 = nob_sv_trim(nob_sv_chop_by_delim(&sv2, '\n'));
            if (nob_sv_end_with(sv2, "title=\"Fazer download do software Java para Windows Off-line\">Windows Off-line</a><br />")) {
                x86jre = sv2;
                continue;
            }
            if (nob_sv_end_with(sv2, "title=\"Fazer download do software Java para Windows (64 bits)\">Windows Off-line (64 bits)</a><br />")) {
                x64jre = sv2;
                continue;
            }
        }
        assert(x86jre.data != NULL && x64jre.data != NULL);

        // removing ""
        nob_sv_chop_by_delim(&x86jre, '"');
        nob_sv_chop_by_delim(&x64jre, '"');
        x86jre = nob_sv_chop_by_delim(&x86jre, '"');
        x64jre = nob_sv_chop_by_delim(&x64jre, '"');

        const char *x86link = nob_temp_sv_to_cstr(x86jre);
        const char *x64link = nob_temp_sv_to_cstr(x64jre);

        // VARLOG(x86link, "%s");
        // VARLOG(x64link, "%s");

        if (nob_file_exists(x86exe) < 1 || !flags.noRedownload) {
            nob_cmd_append(&cmd, "curl", "-L", "-o", x86exe, x86link);
            if (!nob_cmd_run_sync_and_reset(&cmd)) {
                nob_log(NOB_ERROR, "Nao foi possivel fazer o download do arquivo '%s'", x86exe);
                nob_return_defer(false);
            }
        }

        if (nob_file_exists(x64exe) < 1 || !flags.noRedownload) {
            nob_cmd_append(&cmd, "curl", "-L", "-o", x64exe, x64link);
            if (!nob_cmd_run_sync_and_reset(&cmd)) {
                nob_log(NOB_ERROR, "Nao foi possivel fazer o download do arquivo '%s'", x64exe);
                nob_return_defer(false);
            }
        }
    }

    if (!(flags.skipInstall)) {
        DWORD err;
        if (nob_file_exists(x86exe) > 0) {
            nob_cmd_append(&cmd, x86exe, "/s");
            if (!nob_cmd_run_sync_and_reset(&cmd)) {
                err = GetLastError();
                if (err != 0) {
                    nob_log(NOB_ERROR, "Nao foi possivel instalar '%s': %s", x86exe, nob_win32_error_message(err));
                    nob_delete_file(x86exe);
                } else
                    nob_log(NOB_ERROR, "Nao foi possivel instalar '%s': %s", x86exe, nob_win32_error_message(ERROR_PROCESS_ABORTED));
                nob_return_defer(false);
            }
        }

        if (nob_file_exists(x64exe) > 0) {
            nob_cmd_append(&cmd, x64exe, "/s");
            if (!nob_cmd_run_sync_and_reset(&cmd)) {
                err = GetLastError();
                if (err != 0) {
                    nob_log(NOB_ERROR, "Nao foi possivel instalar '%s': %s", x64exe, nob_win32_error_message(err));
                    nob_delete_file(x64exe);
                } else
                    nob_log(NOB_ERROR, "Nao foi possivel instalar '%s': %s", x64exe, nob_win32_error_message(ERROR_PROCESS_ABORTED));
                nob_return_defer(false);
            }
        }
    }

defer:
    nob_sb_free(sb);
    return result;
}

#if 1
bool get_extra_resources(Flags flags) {
    bool result           = true;
    Nob_String_Builder sb = {};
    char *exe             = NULL;

    for (size_t i = 0; i < extra_resources.count; ++i) {
        if (extra_resources.items[i].type != ENTRY_LINK)
            continue;
        if (nob_file_exists(extra_resources.items[i].fileName) > 0 && flags.noRedownload)
            continue;
        nob_cmd_append(&cmd, "curl", "-L", "-o", extra_resources.items[i].fileName, extra_resources.items[i].url);
        if (!nob_cmd_run_sync_and_reset(&cmd)) {
            nob_log(NOB_ERROR, "Nao foi possivel fazer o download do arquivo '%s'", extra_resources.items[i].fileName);
            nob_return_defer(false);
        }
    }

    if (!(flags.skipInstall)) {
        const size_t mark = nob_temp_save();
        for (size_t i = 0; i < extra_resources.count; ++i) {
            nob_temp_rewind(mark);

            switch (extra_resources.items[i].type) {
            case ENTRY_EXE:
                exe = nob_temp_sprintf("%s%s", program_path, extra_resources.items[i].fileName);
                break;
            case ENTRY_LINK:
                exe = (char *)extra_resources.items[i].fileName;
                break;
            default:
                NOB_UNREACHABLE("get_extra_resources");
                break;
            }

            if (nob_file_exists(exe) < 1)
                continue;
            nob_cmd_append(&cmd, exe);
            if (extra_resources.items[i].args) {
                char **arg = extra_resources.items[i].args;
                while (*arg != NULL) {
                    nob_cmd_append(&cmd, *arg);
                    arg++;
                }
            }

            if (!nob_cmd_run_sync_and_reset(&cmd)) {
                DWORD err = GetLastError();
                nob_log(NOB_ERROR, "Nao foi possivel instalar '%s': %s", exe, nob_win32_error_message(err != 0 ? err : ERROR_PROCESS_ABORTED));
                nob_return_defer(false);
            }
        }
        nob_temp_rewind(mark);
    }

defer:
    nob_sb_free(sb);
    return result;
}
#endif

#define CFG_FILE "config.csv"

bool parse_config_file(void) {
    bool result           = true;
    Nob_String_Builder sb = {};

    const char *cfgFile = nob_temp_sprintf("%s%s", program_path, CFG_FILE);
    nob_log(NOB_INFO, "Config File Path: '%s'", cfgFile);

    if (nob_file_exists(cfgFile) < 1) {
        const char cfgHeader[] =                                                                                          //
            "# Linhas que começam com '#' sao ignoradas,,\n"                                                              //
            "# Para links extras, adicione nesse arquivo,\n"                                                              //
            "# Tipo(link/exe), Nome Do Executavel, Link de Download, Argumentos da linha de comando (opcional)\n"         //
            "# link, vcredist_2015_x64.exe, https://aka.ms/vs/17/release/vc_redist.x64.exe, /install /quiet /norestart\n" //
            "# exe, C:\\Redists\\2015\\vcredist_2015_x64.exe, , /install /quiet /norestart\n";                            //
        if (!nob_write_entire_file(cfgFile, cfgHeader, sizeof(cfgHeader) - 1)) {
            nob_return_defer(false);
        }
    } else {
        if (!nob_read_entire_file(cfgFile, &sb)) {
            nob_return_defer(false);
        }
        Nob_String_View content = nob_sv_trim(nob_sb_to_sv(sb));
        Nob_String_View line;
        Nob_String_View value;
        Resource r;

        // for (int i = 1; ; ++i) {
        while (content.count) {
            line = nob_sv_trim(nob_sv_chop_by_delim(&content, '\n'));

            // printf("Line %02d: '" SV_Fmt "' -> %s\n", i, SV_Arg(sv2), (*sv2.data == '#') ? "Skipping Line..." : "Processing Line");

            if (*line.data == '#')
                continue;

            value = nob_sv_trim(nob_sv_chop_by_delim(&line, ','));
            // printf("type: '" SV_Fmt "'\n", SV_Arg(value));
            if (value.count == 0)
                continue;

            memset(&r, 0, sizeof(r));

            if (nob_sv_eq(value, SV("link")))
                r.type = ENTRY_LINK;
            else if (nob_sv_eq(value, SV("exe")))
                r.type = ENTRY_EXE;

            assert(r.type > ENTRY_UNDEFINED && r.type < ENTRY_MAX);

            value = nob_sv_trim(nob_sv_chop_by_delim(&line, ','));
            // printf("fileName: '" SV_Fmt "'\n", SV_Arg(value));
            if (value.count == 0)
                continue;
            r.fileName = nob_sv_to_cstr(value);

            value = nob_sv_trim(nob_sv_chop_by_delim(&line, ','));
            // printf("url: '" SV_Fmt "'\n", SV_Arg(value));
            if (r.type == ENTRY_LINK) {
                if (!value.count)
                    continue;
                r.url = nob_sv_to_cstr(value);
            } else {
                r.url = NULL;
            }

            // value = nob_sv_trim(nob_sv_chop_by_delim(&line, '#'));
            value = nob_sv_trim(line);
            nob_log(NOB_INFO, "args: \"" SV_Fmt "\"", SV_Arg(value));

            if (value.count) {
                wchar_t *cmdLineW = calloc(value.count + 1, sizeof(wchar_t));
                assert(cmdLineW != NULL);

                if (!MultiByteToWideChar(CP_UTF8, MBWCFlags, value.data, value.count, cmdLineW, value.count + 1)) {
                    nob_log(NOB_ERROR, "MultiByteToWideChar: %s", nob_win32_error_message(GetLastError()));
                    free((void *)cmdLineW);
                    nob_return_defer(false);
                }
                int arg_count   = -1;
                wchar_t **wargv = CommandLineToArgvW(cmdLineW, &arg_count);
                free((void *)cmdLineW);
                if (wargv == NULL) {
                    nob_log(NOB_ERROR, "CommandLineToArgvW: %s", nob_win32_error_message(GetLastError()));
                    nob_return_defer(false);
                }

                int n;
                size_t char_count        = 0;
                const size_t header_size = sizeof(char *) * (arg_count + 1);
                for (int i = 0; i < arg_count; ++i) {
                    n = WideCharToMultiByte(CP_UTF8, WCMBFlags, wargv[i], -1, NULL, 0, NULL, NULL);
                    if (!n) {
                        nob_log(NOB_ERROR, "MultiByteToWideChar: %s", nob_win32_error_message(GetLastError()));
                        nob_return_defer(false);
                    }
                    char_count += n;
                }

                const size_t alloc_size = char_count + header_size;
                char **arg_vector       = malloc(alloc_size);
                assert(arg_vector != NULL);

                size_t bytes_writed = 0;
                for (int i = 0; i < arg_count; ++i) {
                    char *buf       = ((char *)arg_vector + header_size + bytes_writed);
                    size_t buf_size = alloc_size - ((char *)buf - (char *)arg_vector);
                    n               = WideCharToMultiByte(CP_UTF8, WCMBFlags, wargv[i], -1, buf, buf_size, NULL, NULL);

                    if (!n) {
                        nob_log(NOB_ERROR, "MultiByteToWideChar: %s", nob_win32_error_message(GetLastError()));
                        free(arg_vector);
                        nob_return_defer(false);
                    }

                    bytes_writed += n;
                    arg_vector[i] = buf;
                }
                arg_vector[arg_count] = NULL;
                r.args                = arg_vector;
                LocalFree(wargv);
            } else {
                r.args = NULL;
            }

            nob_da_append(&extra_resources, r);
            // puts("----------------------------------");
        }
        // system("pause");
        // exit(0);
    }

defer:
    nob_sb_free(sb);
    return result;
}
