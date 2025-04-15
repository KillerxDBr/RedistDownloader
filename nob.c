#define NTDDI_VERSION NTDDI_WIN10
#define _WIN32_WINNT _WIN32_WINNT_WIN10
#define WINVER _WIN32_WINNT_WIN10
#define _UCRT

#define NOB_STRIP_PREFIX
#define NOB_IMPLEMENTATION
#include "nob.h"

#define CLIBS ""
#define CFLAGS "-Wall", "-Wextra", "-march=nocona", "-mtune=generic", "-O2"

#define VARLOG(v, fmt) nob_log(NOB_INFO, "%s: " fmt, #v, v);

int main(int argc, char **argv) {
    NOB_GO_REBUILD_URSELF_PLUS(argc, argv, "nob.h");

    bool force = false;
    for (int i = 0; i < argc; ++i) {
        force = strcasecmp("-f", argv[i]) == 0;
        if (force)
            break;
    }

    int result = 0;
    Cmd cmd = { 0 };

    if (force || nob_needs_rebuild("resource.o", (const char *[]){ "resource.rc", "manifest.xml" }, 2) != 0) {
        cmd_append(&cmd, "windres", "-o", "resource.o", "-i", "resource.rc");
        if (!cmd_run_sync_and_reset(&cmd))
            return_defer(1);
    }

    if (force || nob_needs_rebuild("RedistDownloader.exe", (const char *[]){ "RedistDownloader.c", "resource.o" }, 2) != 0) {
        cmd_append(&cmd, "gcc", "-Iinclude/", "-o", "RedistDownloader.exe", "RedistDownloader.c", "resource.o", CLIBS, CFLAGS);
        if (!cmd_run_sync_and_reset(&cmd))
            return_defer(1);
    }

defer:
    cmd_free(cmd);
    return result;
}
