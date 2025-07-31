#if !defined(_WIN32) || !_WIN32
#error Windows Only...
#endif

#define NTDDI_VERSION NTDDI_WIN10
#define _WIN32_WINNT _WIN32_WINNT_WIN10
#define WINVER _WIN32_WINNT_WIN10

// clang-format off
#ifndef UNICODE
#  define UNICODE
#endif // UNICODE

#ifndef _UNICODE
#  define _UNICODE
#endif // _UNICODE
// clang-format on

#include <locale.h>
#include <stdint.h>
#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#include <Shlobj.h>

#define FLAGS_CAP 4
#define FLAG_IMPLEMENTATION
#include "flag.h"

#define NOB_IMPLEMENTATION
#include "nob.h"

typedef enum {
    ENTRY_LINK = 0,
    ENTRY_EXE,
    ENTRY_UNDEFINED,
} EntryType;

typedef struct {
    const char *fileName;
    const char *url;
    const char *args;
    EntryType type;
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

Nob_Cmd cmd = { 0 };
Nob_String_Builder tmpPath = { 0 };
Resources extra_resources = { 0 };

bool IsExeInPath(const char *exe);
int64_t get_file_size(const char *path);
bool delete_dir_recusive(const char *path);
bool get_java_link(bool skipInstall);
bool get_extra_resources(bool skipInstall);
bool parse_config_file(void);
bool open_folder_in_explorer(const char *path);

void usage(FILE *stream) {
    fprintf(stream, "Usage: ./RedistDownloader [OPTIONS]\n");
    fprintf(stream, "OPTIONS:\n");
    flag_print_options(stream);
}

#define CFG_FILE "config.csv"

int main(int argc, char **argv) {
    setlocale(LC_CTYPE, ".UTF8");

    int result = 0;

    bool *skipInstall = flag_bool("s", false, "Skip instalation");
    bool *help = flag_bool("help", false, "Print this help to stdout and exit with 0");

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

    nob_da_reserve(&tmpPath, MAX_PATH + 1);

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
        delete_dir_recusive(tmpPath.items);
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
    if (!*skipInstall) {
        nob_sb_append_null(&tmpPath);
        delete_dir_recusive(tmpPath.items);
    } else {
        nob_log(NOB_INFO, "Arquivos baixados salvos em: \"%s\"", tmpPath.items);

        nob_sb_append_cstr(&tmpPath, DL_DIR);
        nob_sb_append_null(&tmpPath);

        if (!open_folder_in_explorer(tmpPath.items)) {
            const char *msg = nob_temp_sprintf("Abra manualmente a pasta \"%s\"", tmpPath.items);
            if (msg != NULL)
                MessageBoxA(NULL, msg, "ERRO: Nao foi possivel abrir pasta", MB_TOPMOST | MB_OK | MB_ICONERROR);
            else
                MessageBoxA(NULL, "Nao foi possivel abrir pasta de arquivos", "Erro", MB_TOPMOST | MB_OK | MB_ICONERROR);
        }
    }

defer:
    if (result == 0) {
        MessageBoxW(NULL, L"Todos os programas foram baixados/instalados com sucesso", L"Sucesso...",
                    MB_TOPMOST | MB_OK | MB_ICONINFORMATION);
    } else {
        MessageBoxW(NULL, L"Nem todos os programas foram instalados com sucesso", L"Erro...", MB_TOPMOST | MB_OK | MB_ICONERROR);
    }

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
    Nob_Cmd cmdFinder = { 0 };
    Nob_Cmd_Redirect cr = { 0 };

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

bool delete_dir_recusive(const char *path) {
    bool result = true;
    Nob_File_Paths fp = { 0 };
    Nob_String_Builder sb = { 0 };

    nob_log(NOB_INFO, "deleting '%s' recursively", path);

    if (nob_file_exists(path) < 1)
        nob_return_defer(true);

    Nob_File_Type type = nob_get_file_type(path);
    switch (type) {
    case NOB_FILE_REGULAR:
        nob_return_defer(nob_delete_file(path));
        break;
    case NOB_FILE_DIRECTORY:
        break;
    default:
        nob_log(NOB_ERROR, "Invalid file type for '%s'", path);
        nob_return_defer(false);
        break;
    }

    if (!nob_read_entire_dir(path, &fp)) {
        nob_return_defer(false);
    }

    nob_sb_append_cstr(&sb, path);

    if (sb.items[sb.count - 1] != '/' && sb.items[sb.count - 1] != '\\')
        nob_sb_append_cstr(&sb, "/");

    const size_t sb_cp = sb.count;

    for (size_t i = 0; i < fp.count; ++i) {
        if (memcmp(fp.items[i], ".", sizeof(".")) == 0)
            continue;
        if (memcmp(fp.items[i], "..", sizeof("..")) == 0)
            continue;

        sb.count = sb_cp;
        nob_sb_append_cstr(&sb, fp.items[i]);
        nob_sb_append_null(&sb);

        type = nob_get_file_type(sb.items);
        switch (type) {
        case NOB_FILE_REGULAR:
            if (!nob_delete_file(sb.items))
                nob_return_defer(false);
            break;
        case NOB_FILE_DIRECTORY:
            if (!delete_dir_recusive(sb.items))
                nob_return_defer(false);
            break;
        default:
            nob_log(NOB_WARNING, "Invalid file type for '%s', skipping...", sb.items);
            break;
        }
    }

    result = nob_delete_dir(path);

defer:
    nob_da_free(fp);
    nob_sb_free(sb);
    return result;
}

#define JDL_FILE "manual.jsp"
#define JDL_URL "www.java.com/pt-br/download/manual.jsp"

bool get_extra_resources(bool skipInstall) {
    bool result = true;
    cmd.count = 0;

    Nob_String_Builder sb = { 0 };

    char *exe;
    for (size_t i = 0; i < extra_resources.count; ++i) {
        if (extra_resources.items[i].type != ENTRY_LINK)
            continue;
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

            switch (extra_resources.items[i].type) {
            case ENTRY_EXE:
                exe = (char *)extra_resources.items[i].fileName;
                break;
            case ENTRY_LINK:
                exe = nob_temp_sprintf(SV_Fmt "%s", (int)tmpPath.count, tmpPath.items, extra_resources.items[i].fileName);
                break;
            default:
                NOB_UNREACHABLE("get_extra_resources");
                break;
            }

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

    nob_cmd_append(&cmd, "curl", "-L", "-o", htmlFile, JDL_URL, "-H", "User-Agent: Wget/1.25.0");
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
    Nob_String_View x86jre = {};
    Nob_String_View x64jre = {};

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
    Nob_String_Builder sb = {};

    // Parse config.csv file
    SetLastError(ERROR_SUCCESS);

    WCHAR wPath[MAX_PATH];
    if (!GetModuleFileNameW(NULL, wPath, MAX_PATH)) {
        nob_log(NOB_ERROR, "GetModuleFileNameW failed: %s", nob_win32_error_message(GetLastError()));
        nob_return_defer(false);
    }

    size_t rst = WideCharToMultiByte(CP_UTF8, 0, wPath, -1, tmpPath.items, tmpPath.capacity, NULL, NULL);
    if (!rst) {
        nob_log(NOB_ERROR, "WideCharToMultiByte failed: %s", nob_win32_error_message(GetLastError()));
        nob_return_defer(false);
    }

    tmpPath.count = nob_path_name(tmpPath.items) - tmpPath.items;
    nob_sb_append_cstr(&tmpPath, CFG_FILE);
    nob_sb_append_null(&tmpPath);
    nob_log(NOB_INFO, "Config File Path: '%.*s'", (int)tmpPath.count, tmpPath.items);

    if (nob_file_exists(tmpPath.items) < 1) {
        const char *cfgHeader
            = "# Linhas que começam com '#' sao ignoradas,,\n"
              "# Para links extras, adicione nesse arquivo,\n"
              "# Tipo(link/exe), Nome Do Executavel, Link de Download, Argumentos da linha de comando (opcional)\n"
              "# link, vcredist_2015_x64.exe, https://aka.ms/vs/17/release/vc_redist.x64.exe, /install /quiet /norestart\n"
              "# exe, C:\\Redists\\2015\\vcredist_2015_x64.exe, , /install /quiet /norestart";
        if (!nob_write_entire_file(tmpPath.items, cfgHeader, strlen(cfgHeader))) {
            nob_return_defer(false);
        }
        nob_return_defer(true);
    } else {
        if (!nob_read_entire_file(tmpPath.items, &sb)) {
            nob_return_defer(false);
        }
        Nob_String_View content = nob_sv_trim(nob_sb_to_sv(sb));
        Nob_String_View line;
        Nob_String_View value;
        Resource r;

        // for (int i = 1; ; ++i) {
        while (content.count) {
            memset(&r, 0, sizeof(r));

            line = nob_sv_trim(nob_sv_chop_by_delim(&content, '\n'));

            // printf("Line %02d: '" SV_Fmt "' -> %s\n", i, SV_Arg(sv2), (*sv2.data == '#') ? "Skipping Line..." : "Processing Line");

            if (*line.data == '#')
                continue;

            value = nob_sv_trim(nob_sv_chop_by_delim(&line, ','));
            // printf("type: '" SV_Fmt "'\n", SV_Arg(value));
            if (value.count == 0)
                continue;

            if (nob_sv_eq(value, SV("link")))
                r.type = ENTRY_LINK;
            else if (nob_sv_eq(value, SV("exe")))
                r.type = ENTRY_EXE;
            else
                r.type = ENTRY_UNDEFINED;

            assert(r.type < ENTRY_UNDEFINED);

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

            value = nob_sv_trim(nob_sv_chop_by_delim(&line, ','));
            // printf("args: '" SV_Fmt "'\n", SV_Arg(value));
            r.args = value.count ? nob_sv_to_cstr(value) : NULL;

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

bool open_folder_in_explorer(const char *path) {
    bool result = true;
    PIDLIST_ABSOLUTE pidl = NULL;

    HRESULT hr = CoInitialize(NULL);
    if (SUCCEEDED(hr)) {
        int wsz = MultiByteToWideChar(CP_UTF8, 0, path, -1, NULL, 0);
        if (!wsz) {
            nob_log(NOB_ERROR, "MultiByteToWideChar failed: %s", nob_win32_error_message(GetLastError()));
            nob_return_defer(false);
        }

        wchar_t *wstr_path = nob_temp_alloc(wsz * sizeof(wchar_t));
        assert(wstr_path != NULL);

        if (!MultiByteToWideChar(CP_UTF8, 0, path, -1, wstr_path, wsz)) {
            nob_log(NOB_ERROR, "MultiByteToWideChar failed: %s", nob_win32_error_message(GetLastError()));
            nob_return_defer(false);
        }

        hr = SHParseDisplayName(wstr_path, 0, &pidl, 0, 0);
        if (hr == S_OK) {
            ITEMIDLIST idNull = { 0 };
            LPCITEMIDLIST pidlNull[1] = { &idNull };
            hr = SHOpenFolderAndSelectItems(pidl, 1, pidlNull, 0);
            if (hr != S_OK) {
                nob_log(NOB_ERROR, "SHOpenFolderAndSelectItems falhou: %s", nob_win32_error_message(HRESULT_FROM_WIN32(hr)));
                nob_return_defer(false);
            }

        } else {
            nob_log(NOB_ERROR, "SHParseDisplayName falhou: %s", nob_win32_error_message(HRESULT_FROM_WIN32(hr)));
            nob_return_defer(false);
        }
    } else {
        nob_log(NOB_ERROR, "Nao foi possivel iniciar COM: %s", nob_win32_error_message(HRESULT_FROM_WIN32(hr)));
        nob_return_defer(false);
    }

defer:
    if (pidl)
        ILFree(pidl);

    CoUninitialize();
    return result;
}
