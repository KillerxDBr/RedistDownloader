#if !defined(_WIN32) || !_WIN32
#error Windows Only...
#endif

#define NTDDI_VERSION NTDDI_WIN10
#define _WIN32_WINNT _WIN32_WINNT_WIN10
#define WINVER _WIN32_WINNT_WIN10

#define _CRT_SECURE_NO_WARNINGS
#pragma comment(lib, "shell32.lib") // CommandLineToArgvW in nob_win32_uft8_cmdline_args

// clang-format off
#ifndef UNICODE
#  define UNICODE
#endif // UNICODE

#ifndef _UNICODE
#  define _UNICODE
#endif // _UNICODE
// clang-format on

#include <locale.h>

#define NOB_STRIP_PREFIX
#define NOB_IMPLEMENTATION
#include "nob.h"

#define FLAGS_CAP 4
#define FLAG_IMPLEMENTATION
#include "flag.h"

// clang-format off
#if defined(_MSC_VER)
#  define CLIBS "shell32.lib", "kernel32.lib", "ole32.lib", "user32.lib"
#  define CFLAGS "/W4", "/O2", "/nologo", "/D_CRT_SECURE_NO_WARNINGS"
#else
#  define CLIBS "-lshell32", "-lkernel32", "-lole32", "luser32"
#  define CFLAGS "-Wall", "-Wextra", "-march=nocona", "-mtune=generic", "-O2", "-s", "-D_CRT_SECURE_NO_WARNINGS"
#endif

#if defined(_MSC_VER)
#  ifndef CC
#    define CC "cl"
#  endif // CC

#  define INC_PATH(path) "-I", path
#  define OUT_PATH(path) nob_temp_sprintf("/Fe:%s", (path))

#  ifndef RES
#    define RES "rc"
#  endif // RES

#  define RES_FILE "resource.res"
#  define RES_OUT(path) "/fo", path

#else

#  ifndef CC
#    if defined(__GNUC__)
#      define CC "gcc"
#    elif defined(__clang__)
#      define CC "clang"
#    else
#      define CC "cc"
#    endif
#  endif // CC

#  define INC_PATH(path) "-I", path
#  define OUT_PATH(path) "-o", path

#  ifndef RES
#    define RES "windres"
#  endif // RES

#  define RES_FILE "resource.o"
#  define RES_OUT(path) OUT_PATH(path)
#endif
// clang-format on

void usage(FILE *stream) {
    fprintf(stream, "Usage: %s [OPTIONS]\n", flag_program_name());
    fprintf(stream, "OPTIONS:\n");
    flag_print_options(stream);
}

int main(int argc, char **argv) {
    setlocale(LC_CTYPE, ".UTF8");

    {
        int bkp_argc = argc;
        char **bkp_argv = argv;

        if (!nob_win32_uft8_cmdline_args(&argc, &argv)) {
            nob_log(NOB_WARNING, "Could not generate Win32 UTF-8 Command Line Arguments, using default...");
            argc = bkp_argc;
            argv = bkp_argv;
        }
    }

    NOB_GO_REBUILD_URSELF_PLUS(argc, argv, "nob.h");

    bool *force = flag_bool("f", false, "Force Rebuild");
    bool *clean = flag_bool("c", false, "Clean build artifacts");
    bool *help = flag_bool("h", false, "Print this help to stdout and exit with 0");

    if (!flag_parse(argc, argv)) {
        usage(stderr);
        flag_print_error(stderr);
        exit(1);
    }

    if (*help) {
        usage(stdout);
        exit(0);
    }

    int result = 0;
    Cmd cmd = { 0 };

    if (*clean) {
        if (!nob_delete_file(RES_FILE) || !nob_delete_file("RedistDownloader.exe"))
            return_defer(1);
        *force = true;
    }

    if (*force || nob_needs_rebuild(RES_FILE, (const char *[]){ "resource.rc", "manifest.xml" }, 2) != 0) {
        cmd_append(&cmd, RES, "/nologo", "/8", "/d", "UNICODE", "/d", "_UNICODE", RES_OUT(RES_FILE), "resource.rc");
        if (!cmd_run_sync_and_reset(&cmd)) {
            nob_log(NOB_ERROR, "Could not generate '%s' file", RES_FILE);
            return_defer(2);
        }
    }

    const char *deps[] = { "RedistDownloader.c", RES_FILE, "nob.h", "flag.h" };
    if (*force || nob_needs_rebuild("RedistDownloader.exe", deps, NOB_ARRAY_LEN(deps)) != 0) {
        cmd_append(&cmd, CC, INC_PATH("include/"), OUT_PATH("RedistDownloader.exe"), "RedistDownloader.c", RES_FILE, CLIBS, CFLAGS);
        if (!cmd_run_sync_and_reset(&cmd)) {
            nob_log(NOB_ERROR, "Could not compile source files");
            return_defer(3);
        }
    }

defer:
    cmd_free(cmd);
    return result;
}
