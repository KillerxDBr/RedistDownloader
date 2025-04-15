# Redist Downloader

VC++ Redist and Java Runtime Downloader

To build (Win10 & Win11, MinGW only):

```console
    cc -o nob.exe nob.c
    ./nob.exe
```

## Usage
```console
    ./RedistDownloader.exe [-s]
```
`-s Flag: Skip installation and dont delete files. (Optional)`

Files are saved in "%temp%\KxD.VCD.App\downloads"

## TODO List
- Add MSVC build support.
- Change Java page parsing to English.
