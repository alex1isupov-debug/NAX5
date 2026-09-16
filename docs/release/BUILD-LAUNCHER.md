# Сборка NAX5 Windows launcher

Пользовательский launcher для Alpha 0.5 — один файл **`NAX5.exe`** внутри **`NAX5-windows.zip`**.  
Это установщик **Inno Setup** (как chiaki-ng): Windows-native stub, без MSYS runtime. Внутри payload — portable tree (`chiaki.exe`, Qt, FFmpeg и зависимости).

Не использовать MSYS `7zCon.sfx` / `7zSD.sfx` из p7zip: сам упаковщик тянет `msys-*.dll` и падает на чистой Windows.

## Что получится

| Артефакт | Назначение |
| --- | --- |
| `artifacts/alpha-0.5/NAX5-windows.zip` | основная ссылка для пользователей (один `NAX5.exe` = Inno installer) |
| `artifacts/alpha-0.5/NAX5-windows-installer.zip` | то же с именем `NAX5-windows-installer.exe` (как chiaki-ng) |
| `artifacts/alpha-0.5/NAX5-windows-portable.zip` | отладочный portable zip (папка `NAX5/`) |
| `artifacts/alpha-0.5/NAX5-Alpha-0.5-Windows-x64.zip` | legacy alias portable zip |

Скрипт сборки: `scripts/release/build-alpha05-user-pack.sh`

## Требования

1. **Windows 10/11 x64**
2. **MSYS2** с профилем **MINGW64** (`C:\msys64\mingw64.exe`)
3. **Inno Setup 6** (`ISCC.exe`)
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

Скрипт ставит toolchain, Qt6, FFmpeg/libplacebo/sdl2-compat и `python-protobuf`.

Inno Setup 6: https://jrsoftware.org/isinfo.php (или `winget install JRSoftware.InnoSetup`).

## Сборка

```bash
# MSYS2 MINGW64
cd /c/astro/nax5-client
bash scripts/release/build-alpha05-user-pack.sh
```

Если `chiaki.exe` уже собран:

```bash
bash scripts/release/package-alpha05-only.sh
```

Скрипт:

1. проверяет, что worktree чистый (полная сборка);
2. при необходимости генерирует `gui/nax5.ico`;
3. собирает `chiaki.exe` (Release) и unit-тесты `nax5-*-unit`;
4. упаковывает portable tree через `scripts/deploy-windows-msys2.sh` (FFmpeg/SDL/libplacebo явно + `objdump` по импортам);
5. собирает Inno installer и `NAX5-windows.zip`;
6. запускает `verify-clean-windows-launch.ps1` (PATH без MSYS2, silent install).

## Проверка результата

```bash
ls -l artifacts/alpha-0.5/NAX5.exe
ls -l artifacts/alpha-0.5/NAX5-windows.zip
unzip -l artifacts/alpha-0.5/NAX5-windows.zip
sha256sum artifacts/alpha-0.5/NAX5-windows.zip
```

На Windows без MSYS2 в PATH:

```powershell
Expand-Archive artifacts\alpha-0.5\NAX5-windows.zip -DestinationPath $env:TEMP\nax5-test
& "$env:TEMP\nax5-test\NAX5.exe"
```

Ожидаемо:

- `NAX5-windows.zip` — один `NAX5.exe` (Inno Setup, FileDescription = `NAX5 Setup`);
- мастер установки, затем `chiaki.exe` из `%LOCALAPPDATA%\Programs\NAX5`;
- portable tree содержит `avutil-*.dll`, `avcodec-*.dll`, `avformat-*.dll`, `swresample-*.dll`;
- `chiaki.exe` с `FileDescription = NAX5 Remote Play Client`;
- установщик **не** импортирует `msys-*.dll`.

## Публикация на GitHub Releases

1. Обновите `RELEASES.md` в workspace `C:\astro`.
2. Создайте тег, например `alpha-0.5-build-2`.
3. Загрузите assets:
   - `NAX5-windows.zip` (обязательно)
   - `NAX5-windows-installer.zip` (то же содержимое с chiaki-ng именем)
   - `NAX5-windows-portable.zip` (опционально)
4. Обновите `nax5-web/src/config/launcher.ts` (`releaseTag`, `fileName`, URL).
5. Задеплойте web, чтобы `/account` отдавал новую ссылку.

## Типичные ошибки

| Сообщение | Что делать |
| --- | --- |
| `Portable bundle is missing avutil-*.dll` | перезапустите `setup-msys2-build-env.sh`, проверьте FFmpeg в `/mingw64/bin` |
| `unresolved import …` | DLL есть в mingw, но не попала в bundle — скрипт должен скопировать её из `/mingw64/bin`; если файла нет, доустановите пакет |
| `Inno Setup 6 ISCC.exe is required` | поставьте Inno Setup 6 и повторите |
| `chiaki.exe exited immediately` | missing DLL на чистом PATH; смотрите `objdump -p chiaki.exe` |
| `Refusing to build a release tag with a dirty worktree` | закоммитьте или stash изменения |
