#define NTDDI_VERSION NTDDI_WIN10
#define _WIN32_WINNT _WIN32_WINNT_WIN10
#define WINVER _WIN32_WINNT_WIN10
#define _UCRT

#undef UNICODE
#undef _UNICODE

#include <stdint.h>
#include <stdio.h>

#define FLAG_IMPLEMENTATION
#include "flag.h"

#define NOB_IMPLEMENTATION
#include "nob.h"

typedef struct {
    const char *fileName;
    const char *url;
    const char *args;
} Resource;

typedef struct {
    Resource *items;
    size_t count;
    size_t capacity;
} Resources;

#define DL_DIR "downloads\\"

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
    
    {.fileName = "vcredist_2015_x86.exe", .url = "https://aka.ms/vs/17/release/vc_redist.x86.exe"},
    {.fileName = "vcredist_2015_x64.exe", .url = "https://aka.ms/vs/17/release/vc_redist.x64.exe"},  
};
// clang-format on

#define NUM_URLS NOB_ARRAY_LEN(vc_resources)

#ifdef USE_LIBCURL
static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream) { return fwrite(ptr, size, nmemb, (FILE *)stream); }
#endif // USE_LIBCURL

Nob_Cmd cmd = { 0 };
Nob_String_Builder tmpPath = { 0 };
Resources extra_resources = { 0 };

bool IsExeInPath(const char *exe);
int64_t get_file_size(const char *path);
bool delete_folder_recursively(const char *path);
bool get_java_link(bool skipInstall);
bool get_extra_resources(bool skipInstall);
bool parse_config_file(void);

void usage(FILE *stream) {
    fprintf(stream, "Usage: ./RedistDownloader [OPTIONS]\n");
    fprintf(stream, "OPTIONS:\n");
    flag_print_options(stream);
}

#define CFG_FILE "config.csv"

int main(int argc, char **argv) {
    int result = 0;

    bool *help = flag_bool("help", false, "Print this help to stdout and exit with 0");
    bool *skipInstall = flag_bool("s", false, "Skip instalation");

    if (!flag_parse(argc, argv)) {
        usage(stderr);
        flag_print_error(stderr);
        exit(1);
    }

    if (*help) {
        usage(stdout);
        exit(0);
    }

    nob_da_reserve(&tmpPath, MAX_PATH + 1);
    assert(tmpPath.capacity > 0);

    if (!parse_config_file())
        nob_return_defer(1);

    tmpPath.count = GetTempPathA((DWORD)tmpPath.capacity, tmpPath.items);
    if (tmpPath.count == 0) {
        nob_log(NOB_ERROR, "Could not get TEMP Path: %s", nob_win32_error_message(GetLastError()));
        nob_return_defer(1);
    }
    if (tmpPath.count >= tmpPath.capacity) {
        nob_log(NOB_ERROR, "Could not get TEMP Path: %s", nob_win32_error_message(ERROR_BUFFER_OVERFLOW));
        nob_return_defer(1);
    }

    nob_sb_append_cstr(&tmpPath, "KxD.VCD.App\\");
    const size_t rootTmpCount = tmpPath.count;
    nob_sb_append_null(&tmpPath);

    if (nob_file_exists(tmpPath.items) > 0) {
        delete_folder_recursively(tmpPath.items);
    }

    if (!nob_mkdir_if_not_exists(tmpPath.items)) {
        nob_return_defer(1);
    }
    tmpPath.count--;

    nob_sb_append_cstr(&tmpPath, DL_DIR);
    nob_sb_append_null(&tmpPath);

    if (!nob_mkdir_if_not_exists(tmpPath.items)) {
        nob_return_defer(1);
    }

    tmpPath.count--;

    if (!IsExeInPath("curl")) {
        nob_log(NOB_ERROR, "curl.exe nao encontrado...");
        nob_return_defer(1);
    }

    const size_t tmpPathSize = tmpPath.count;
    for (size_t i = 0; i < NUM_URLS; ++i) {
        tmpPath.count = tmpPathSize;
        nob_sb_append_cstr(&tmpPath, vc_resources[i].fileName);
        nob_sb_append_null(&tmpPath);

        puts("-------------------------------------------------------------------------------------");
        nob_log(NOB_INFO, "File: '%s' -> Link: '%s'", tmpPath.items, vc_resources[i].url);
        nob_cmd_append(&cmd, "curl", "-L", "-o", tmpPath.items, vc_resources[i].url);

        bool rst = nob_cmd_run_sync_and_reset(&cmd);
        Sleep(100);
        if (!rst || nob_file_exists(tmpPath.items) == 0 || get_file_size(tmpPath.items) < 1) {
            nob_log(NOB_ERROR, "Could not download \"%s\"", tmpPath.items);
            if (get_file_size(tmpPath.items) < 1)
                // nob_delete_file(tmpPath.items);
                nob_return_defer(1);
        }
    }

    puts("-------------------------------------------------------------------------------------");

    if (!*skipInstall)
        for (size_t i = 0; i < NUM_URLS; ++i) {
            tmpPath.count = tmpPathSize;
            nob_sb_append_cstr(&tmpPath, vc_resources[i].fileName);
            nob_sb_append_null(&tmpPath);

            nob_log(NOB_INFO, "Running '%s'", tmpPath.items);
            nob_cmd_append(&cmd, tmpPath.items);
            if (i < 4) {
                nob_cmd_append(&cmd, "/Q");
            } else {
                nob_cmd_append(&cmd, "/install", "/quiet", "/norestart");
            }
            if (!nob_cmd_run_sync_and_reset(&cmd)) {
                DWORD err = GetLastError();
                if (err != 0)
                    nob_log(NOB_ERROR, "Could not run file '%s': %s", tmpPath.items, nob_win32_error_message(err));
                else
                    nob_log(NOB_ERROR, "Could not run file '%s': %s", tmpPath.items, nob_win32_error_message(ERROR_PROCESS_ABORTED));
                nob_return_defer(1);
            }
            Sleep(500);
        }

    tmpPath.count = tmpPathSize;
    if (!get_java_link(*skipInstall))
        nob_return_defer(1);

    tmpPath.count = tmpPathSize;
    if (!get_extra_resources(*skipInstall))
        nob_return_defer(1);

    tmpPath.count = rootTmpCount;
    nob_sb_append_null(&tmpPath);
    if (!*skipInstall) {
        delete_folder_recursively(tmpPath.items);
    } else {
        nob_log(NOB_INFO, "Arquivos baixados salvos em: \"%s\"", tmpPath.items);
        // TODO: Abrir pasta de arquivos
        // ShellExecuteA(NULL, "open", tmpPath.items, NULL, NULL, 10);
    }

defer:
    for (size_t i = 0; i < extra_resources.count; ++i) {
        free((void *)extra_resources.items[i].fileName);
        free((void *)extra_resources.items[i].url);

        if (extra_resources.items[i].args)
            free((void *)extra_resources.items[i].args);
    }

    nob_da_free(extra_resources);
    nob_sb_free(tmpPath);
    nob_cmd_free(cmd);

    system("pause");

    return result;
}

bool IsExeInPath(const char *exe) {
    Nob_Cmd cmd = { 0 };
    Nob_Cmd_Redirect cr = { 0 };

    Nob_Fd nullOutput = nob_fd_open_for_write("NUL");
    if (nullOutput == NOB_INVALID_FD) {
        nob_log(NOB_ERROR, "Nao foi possivel redirecionar a saida de texto...");
    } else {
        cr.fdout = &nullOutput;
        cr.fderr = &nullOutput;
    }
    nob_cmd_append(&cmd, "where.exe", "/Q", exe);
    bool result = nob_cmd_run_sync_redirect(cmd, cr);

    if (result) {
        nob_log(NOB_INFO, "\"%s\" encontrado no PATH", exe);
    } else {
        nob_log(NOB_ERROR, "\"%s\" nao encontrado no PATH", exe);
    }

    if (nullOutput != NOB_INVALID_FD)
        nob_fd_close(nullOutput);

    nob_cmd_free(cmd);

    return result;
}

/*
-1 -> Error
0+ -> File Size
*/
int64_t get_file_size(const char *path) {
    int64_t result;

    FILE *f = fopen(path, "rb");
    if (f == NULL)
        nob_return_defer(-1);

    if (fseek(f, 0, SEEK_END) < 0)
        nob_return_defer(-1);

#ifndef _WIN32
    result = ftell(f);
#else
    result = _ftelli64(f);
#endif

defer:
    if (result < 0)
        nob_log(NOB_ERROR, "Could not get file size from \"%s\": %s", path, strerror(errno));

    if (f)
        fclose(f);

    return result;
}

bool delete_folder_recursively(const char *path) {
    SHFILEOPSTRUCTA sf;
    memset(&sf, 0, sizeof(sf));

    Nob_String_Builder sb = { 0 };
    nob_sb_append_cstr(&sb, path);

    nob_sb_append_null(&sb);
    nob_sb_append_null(&sb);

    sf.wFunc = FO_DELETE;

    sf.pFrom = sb.items;
    sf.fFlags = FOF_NO_UI;

    // https://learn.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shfileoperationa#return-value
    int rst = SHFileOperationA(&sf); // This shit return some pre Win32 errors... THANK YOU MICROSOFT

    if (rst)
        nob_log(NOB_ERROR, "SHFileOperationA failed: 0x%X", rst);

    nob_sb_free(sb);

    return rst == 0;
}
#define JDL_FILE "manual.jsp"
#define JDL_URL "www.java.com/pt-br/download/manual.jsp"

bool get_extra_resources(bool skipInstall) {
    bool result = true;
    cmd.count = 0;

    Nob_String_Builder sb = { 0 };

    char *exe;
    for (size_t i = 0; i < extra_resources.count; ++i) {
        exe = nob_temp_sprintf(SV_Fmt "%s", (int)tmpPath.count, tmpPath.items, extra_resources.items[i].fileName);
        nob_cmd_append(&cmd, "curl", "-o", exe, "-L", extra_resources.items[i].url);
        if (!nob_cmd_run_sync_and_reset(&cmd)) {
            nob_log(NOB_ERROR, "Nao foi possivel fazer o download do arquivo '%s'", exe);
            nob_return_defer(false);
        }
    }

    if (!skipInstall) {
        const size_t tmpBk = nob_temp_save();
        for (size_t i = 0; i < extra_resources.count; ++i) {
            nob_temp_rewind(tmpBk);
            exe = nob_temp_sprintf(SV_Fmt "%s", (int)tmpPath.count, tmpPath.items, extra_resources.items[i].fileName);

            nob_cmd_append(&cmd, exe);
            if (extra_resources.items[i].args && *extra_resources.items[i].args) {
                const char *arg;
                Nob_String_View args = nob_sv_trim(nob_sv_from_cstr(extra_resources.items[i].args));
                while (args.count > 0) {
                    arg = nob_temp_sv_to_cstr(nob_sv_chop_by_delim(&args, ' '));
                    nob_cmd_append(&cmd, arg);
                }
            }

            if (!nob_cmd_run_sync_and_reset(&cmd)) {
                nob_log(NOB_ERROR, "Nao foi possivel instalar '%s'", exe);
                nob_return_defer(false);
            }
        }
        nob_temp_rewind(tmpBk);
    }

defer:
    nob_sb_free(sb);
    return result;
}

bool get_java_link(bool skipInstall) {
    bool result = true;

    Nob_String_Builder sb = { 0 };

    cmd.count = 0;
    const char *htmlFile = nob_temp_sprintf(SV_Fmt JDL_FILE, (int)tmpPath.count, tmpPath.items);

    nob_cmd_append(&cmd, "curl", "-L", "-o", htmlFile, "\"" JDL_URL "\"", "-H", "\"User-Agent:", "Wget/1.25.0\"");
    if (!nob_cmd_run_sync_and_reset(&cmd)) {
        nob_log(NOB_ERROR, "Nao foi possivel fazer o download do arquivo '%s'", JDL_FILE);
        nob_return_defer(false);
    }

    Sleep(500);

    int exists = nob_file_exists(htmlFile);
    if (exists < 0)
        nob_return_defer(false);
    if (exists == 0) {
        nob_log(NOB_ERROR, "Nao foi possivel encontrar o arquivo '%s': %s", htmlFile, nob_win32_error_message(GetLastError()));
        nob_return_defer(false);
    }

    if (!nob_read_entire_file(htmlFile, &sb))
        nob_return_defer(false);

    Nob_String_View sv = nob_sv_trim(nob_sb_to_sv(sb));
    nob_sv_chop_by_sv(&sv,
                      SV("Instruções de instalação do software Java para Windows On-line")); // <div class="rw-inpagetab" id="jre8-windows">

    Nob_String_View sv2;
    Nob_String_View x86jre = { 0 };
    Nob_String_View x64jre = { 0 };

    while (sv2 = nob_sv_chop_by_sv(&sv, SV("<td><a href=")), sv.count > 0) {
        sv2 = nob_sv_trim(nob_sv_chop_by_delim(&sv2, '\n'));
        if (nob_sv_end_with(sv2, "title=\"Fazer download do software Java para Windows Off-line\">Windows Off-line</a><br />")) {
            x86jre = sv2;
            continue;
        }
        if (nob_sv_end_with(sv2, "title=\"Fazer download do software Java para Windows (64 bits)\">Windows Off-line (64 bits)</a><br />")) {
            x64jre = sv2;
            continue;
        }
        if (x86jre.data != NULL && x64jre.data != NULL)
            break;
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

    const char *x86exe = nob_temp_sprintf(SV_Fmt "%s", (int)tmpPath.count, tmpPath.items, "java-x86.exe");
    const char *x64exe = nob_temp_sprintf(SV_Fmt "%s", (int)tmpPath.count, tmpPath.items, "java-x64.exe");

    nob_cmd_append(&cmd, "curl", "-o", x86exe, "-L", x86link);
    if (!nob_cmd_run_sync_and_reset(&cmd)) {
        nob_log(NOB_ERROR, "Nao foi possivel fazer o download do arquivo '%s'", x86exe);
        nob_return_defer(false);
    }

    nob_cmd_append(&cmd, "curl", "-o", x64exe, "-L", x64link);
    if (!nob_cmd_run_sync_and_reset(&cmd)) {
        nob_log(NOB_ERROR, "Nao foi possivel fazer o download do arquivo '%s'", x64exe);
        nob_return_defer(false);
    }

    if (!skipInstall) {
        nob_cmd_append(&cmd, x86exe, "/s");
        if (!nob_cmd_run_sync_and_reset(&cmd)) {
            nob_log(NOB_ERROR, "Nao foi possivel instalar '%s'", x86exe);
            nob_return_defer(false);
        }

        nob_cmd_append(&cmd, x64exe, "/s");
        if (!nob_cmd_run_sync_and_reset(&cmd)) {
            nob_log(NOB_ERROR, "Nao foi possivel instalar '%s'", x64exe);
            nob_return_defer(false);
        }
    }

defer:
    nob_sb_free(sb);
    return result;
}

const char *nob_sv_to_cstr(Nob_String_View sv) {
    char *result = malloc(sv.count + 1);
    NOB_ASSERT(result != NULL && "Buy More RAM");
    memcpy(result, sv.data, sv.count);
    result[sv.count] = '\0';
    return result;
}

bool parse_config_file(void) {
    bool result = true;

    // Parse config.csv file
    assert(tmpPath.capacity > MAX_PATH);

    SetLastError(ERROR_SUCCESS);
    DWORD rst = GetModuleFileNameA(NULL, tmpPath.items, tmpPath.capacity);
    if (!rst) { // Get .exe file path
        nob_log(NOB_ERROR, "GetModuleFileNameA failed: %s", nob_win32_error_message(GetLastError()));
        nob_return_defer(false);
    } else if (GetLastError()) {
        nob_log(NOB_ERROR, "GetModuleFileNameA failed: %s", nob_win32_error_message(GetLastError()));
        nob_return_defer(false);
    }
    tmpPath.count = rst - strlen(nob_path_name(tmpPath.items));
    nob_sb_append_cstr(&tmpPath, CFG_FILE);
    nob_sb_append_null(&tmpPath);
    printf("Config FIle Path: '%.*s'\n", (int)tmpPath.count, tmpPath.items);

    Nob_String_Builder sb = { 0 };
    if (nob_file_exists(tmpPath.items) < 1) {
        const char *cfgHeader = "# Linhas que começam com '#' sao ignoradas\n"
                                "# Para links extras, adicione nesse arquivo\n"
                                "# Nome Do Executavel, Link de Download, Argumentos da linha de comando (opcional)\n"
                                "# vcredist_2015_x64.exe, https://aka.ms/vs/17/release/vc_redist.x64.exe, /install /quiet /norestart\n";
        if (!nob_write_entire_file(tmpPath.items, cfgHeader, strlen(cfgHeader))) {
            nob_return_defer(false);
        }
        nob_return_defer(true);
    } else {
        if (!nob_read_entire_file(tmpPath.items, &sb)) {
            nob_return_defer(false);
        }
        Nob_String_View sv = nob_sv_trim(nob_sb_to_sv(sb));
        Nob_String_View sv2;
        Nob_String_View sv3;
        Resource r;

        // for (int i = 1; ; ++i) {
        while (sv.count) {
            memset(&r, 0, sizeof(r));

            sv2 = nob_sv_trim(nob_sv_chop_by_delim(&sv, '\n'));

            // printf("Line %02d: '" SV_Fmt "' -> %s\n", i, SV_Arg(sv2), (*sv2.data == '#') ? "Skipping Line..." : "Processing Line");

            if (*sv2.data == '#')
                continue;

            Nob_String_View fileName;
            Nob_String_View url;
            Nob_String_View args;

            sv3 = nob_sv_trim(nob_sv_chop_by_delim(&sv2, ','));
            // printf("'" SV_Fmt "'\n", SV_Arg(sv3));
            if (sv3.count == 0)
                continue;
            fileName = sv3;

            sv3 = nob_sv_trim(nob_sv_chop_by_delim(&sv2, ','));
            // printf("'" SV_Fmt "'\n", SV_Arg(sv3));
            if (sv3.count == 0)
                continue;
            url = sv3;

            sv3 = nob_sv_trim(nob_sv_chop_by_delim(&sv2, ','));
            // printf("'" SV_Fmt "'\n", SV_Arg(sv3));
            args = sv3;

            r.fileName = nob_sv_to_cstr(fileName);
            r.url = nob_sv_to_cstr(url);
            r.args = args.count ? nob_sv_to_cstr(args) : NULL;

            nob_da_append(&extra_resources, r);
            // puts("----------------------------------");
        }
    }

defer:
    nob_sb_free(sb);
    return result;
}
