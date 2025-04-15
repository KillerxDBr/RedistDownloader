#if !_WIN32
#error Windows Only...
#endif

#define NTDDI_VERSION NTDDI_WIN10
#define _WIN32_WINNT _WIN32_WINNT_WIN10
#define WINVER _WIN32_WINNT_WIN10
#define _UCRT

#define NOB_STRIP_PREFIX
#define NOB_IMPLEMENTATION
#include "nob.h"

// clang-format off
#if defined(_MSC_VER)
#  define CLIBS "shell32.lib", "kernel32.lib"
#  define CFLAGS "/W4", "/O2", "/nologo", "/D_CRT_SECURE_NO_WARNINGS"
#else
#  define CLIBS "-lshell32", "-lkernel32"
#  define CFLAGS "-Wall", "-Wextra", "-march=nocona", "-mtune=generic", "-O2", "-s"
#endif

#if defined(_MSC_VER)
#  define CC "cl"
#  define INC_PATH(path) "-I", path
#  define OUT_PATH(path) nob_temp_sprintf("/Fe:%s", (path))

#  define RES_FILE "resource.res"
#  define RES "rc"
#  define RES_OUT(path) "/fo", path
#else
#  if defined(__GNUC__)
#    define CC "gcc"
#  elif defined(__clang__)
#    define CC "clang"
#  else
#    define CC "cc"
#  endif

#  define INC_PATH(path) "-I", path
#  define OUT_PATH(path) "-o", path

#  define RES_FILE "resource.o"
#  define RES "windres"
#  define RES_OUT(path) OUT_PATH(path)
#endif
// clang-format on

int main(int argc, char **argv) {
    NOB_GO_REBUILD_URSELF_PLUS(argc, argv, "nob.h");

    bool force = false;
    bool clean = false;
    for (int i = 0; i < argc; ++i) {
        bool used = false;
        if (!used && _stricmp("-f", argv[i]) == 0) {
            force = true;
            used = true;
        }
        if (!used && _stricmp("-c", argv[i]) == 0) {
            clean = true;
            continue;
        }
        if (force && clean)
            break;
    }

    int result = 0;
    Cmd cmd = { 0 };

    if (clean) {
        if (!nob_delete_file(RES_FILE) || !nob_delete_file("RedistDownloader.exe"))
            nob_return_defer(1);
        force = true;
    }

    if (force || nob_needs_rebuild(RES_FILE, (const char *[]){ "resource.rc", "manifest.xml" }, 2) != 0) {
        cmd_append(&cmd, RES, RES_OUT(RES_FILE), "resource.rc");
        if (!cmd_run_sync_and_reset(&cmd))
            return_defer(1);
    }

    if (force || nob_needs_rebuild("RedistDownloader.exe", (const char *[]){ "RedistDownloader.c", RES_FILE }, 2) != 0) {
        cmd_append(&cmd, CC, INC_PATH("include/"), OUT_PATH("RedistDownloader.exe"), "RedistDownloader.c", RES_FILE, CLIBS, CFLAGS);
        if (!cmd_run_sync_and_reset(&cmd))
            return_defer(1);
    }

defer:
    cmd_free(cmd);
    return result;
}
