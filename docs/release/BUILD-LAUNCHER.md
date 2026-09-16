# Сборка NAX5 Windows launcher

Пользовательский launcher для Alpha 0.5 — один файл **`NAX5.exe`** внутри **`NAX5-windows.zip`**.  
Это self-extracting 7-Zip архив: без мастера установки, с полным portable tree (`chiaki.exe`, Qt, FFmpeg и зависимости).

## Что получится

| Артефакт | Назначение |
| --- | --- |
| `artifacts/alpha-0.5/NAX5-windows.zip` | основная ссылка для пользователей (один `NAX5.exe`) |
| `artifacts/alpha-0.5/NAX5-windows-portable.zip` | отладочный portable zip (папка `NAX5/`) |
| `artifacts/alpha-0.5/NAX5-Alpha-0.5-Windows-x64.zip` | legacy alias portable zip |

Скрипт сборки: `scripts/release/build-alpha05-user-pack.sh`

## Требования

1. **Windows 10/11 x64**
2. **MSYS2** с профилем **MINGW64** (`C:\msys64\mingw64.exe`)
3. **p7zip** (`pacman -S p7zip`) для self-extracting `NAX5.exe`
4. **Git** (для provenance в `BUILD-INFO.txt`)
5. Чистый worktree (`git status` без незакоммиченных файлов)

### Установка MSYS2

```powershell
winget install MSYS2.MSYS2
```

После установки MSYS2 один раз откройте **MSYS2 MINGW64** и выполните:

```bash
bash scripts/release/setup-msys2-build-env.sh
```

Скрипт ставит toolchain, Qt6, FFmpeg/libplacebo/sdl2-compat, p7zip и `python-protobuf`.

## Сборка

```bash
# MSYS2 MINGW64
cd /c/astro/nax5-client
bash scripts/release/build-alpha05-user-pack.sh
```

Скрипт:

1. проверяет, что worktree чистый;
2. при необходимости генерирует `gui/nax5.ico`;
3. собирает `chiaki.exe` (Release) и unit-тесты `nax5-*-unit`;
4. упаковывает portable tree через `scripts/deploy-windows-msys2.sh` (включая FFmpeg DLL);
5. создаёт `NAX5.exe` (7z SFX) и `NAX5-windows.zip`.

## Проверка результата

```bash
ls -l artifacts/alpha-0.5/NAX5.exe
ls -l artifacts/alpha-0.5/NAX5-windows.zip
unzip -l artifacts/alpha-0.5/NAX5-windows.zip
unzip -p artifacts/alpha-0.5/NAX5-windows-portable.zip | strings | grep avutil
sha256sum artifacts/alpha-0.5/NAX5-windows.zip
```

На Windows:

```powershell
Expand-Archive artifacts\alpha-0.5\NAX5-windows.zip -DestinationPath $env:TEMP\nax5-test
& "$env:TEMP\nax5-test\NAX5.exe"
```

Ожидаемо:

- `NAX5-windows.zip` ~100 MB;
- внутри zip только `NAX5.exe`;
- double-click на `NAX5.exe` распаковывает и запускает `chiaki.exe` без мастера установки;
- portable tree содержит `avutil-*.dll`, `avcodec-*.dll`, `avformat-*.dll`, `swresample-*.dll`;
- `chiaki.exe` с `FileDescription = NAX5 Remote Play Client`.

## Публикация на GitHub Releases

1. Обновите `RELEASES.md` в workspace `C:\astro`.
2. Создайте тег, например `alpha-0.5-build-2`.
3. Загрузите assets:
   - `NAX5-windows.zip` (обязательно)
   - `NAX5-windows-portable.zip` (опционально)
4. Обновите `nax5-web/src/config/launcher.ts` (`releaseTag`, `fileName`, URL).
5. Задеплойте web, чтобы `/account` отдавал новую ссылку.

## Типичные ошибки

| Сообщение | Что делать |
| --- | --- |
| `Portable bundle is missing avutil-*.dll` | перезапустите `setup-msys2-build-env.sh`, проверьте FFmpeg в `/mingw64/bin` |
| `7zSD.sfx not found` | `pacman -S p7zip` в MSYS2 |
| `Refusing to build a release tag with a dirty worktree` | закоммитьте или stash изменения |
