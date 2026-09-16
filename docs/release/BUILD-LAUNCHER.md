# Сборка NAX5 Windows launcher

Пользовательский launcher для Alpha 0.5 — один файл **`NAX5-windows-installer.exe`**.  
Внутри инсталлятора portable-сборка с `chiaki.exe`, Qt runtime и зависимостями.

## Что получится

| Артефакт | Назначение |
| --- | --- |
| `artifacts/alpha-0.5/NAX5-windows-installer.exe` | основная ссылка для пользователей |
| `artifacts/alpha-0.5/NAX5-Alpha-0.5-Windows-x64.zip` | portable fallback для GitHub Release |

Скрипт сборки: `scripts/release/build-alpha05-user-pack.sh`  
Inno Setup script: `scripts/nax5-windows-user.iss`

## Требования

1. **Windows 10/11 x64**
2. **MSYS2** с профилем **MINGW64** (`C:\msys64\mingw64.exe`)
3. **Inno Setup 6** (`ISCC.exe`)
4. **Git** (для provenance в `BUILD-INFO.txt`)
5. Чистый worktree (`git status` без незакоммиченных файлов)

### Установка MSYS2 и Inno Setup

```powershell
winget install MSYS2.MSYS2
winget install JRSoftware.InnoSetup
```

После установки MSYS2 один раз откройте **MSYS2 MINGW64** и выполните:

```bash
bash scripts/release/setup-msys2-build-env.sh
```

Скрипт ставит toolchain, Qt6, FFmpeg/libplacebo/sdl2-compat и `python-protobuf`.

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
4. упаковывает portable tree через `scripts/deploy-windows-msys2.sh`;
5. создаёт ZIP;
6. компилирует `NAX5-windows-installer.exe` через Inno Setup.

## Проверка результата

```bash
ls -l artifacts/alpha-0.5/NAX5-windows-installer.exe
ls -l artifacts/alpha-0.5/NAX5-Alpha-0.5-Windows-x64.zip
unzip -tq artifacts/alpha-0.5/NAX5-Alpha-0.5-Windows-x64.zip
sha256sum artifacts/alpha-0.5/NAX5-windows-installer.exe
```

На Windows:

```powershell
(Get-Item artifacts\alpha-0.5\NAX5-windows-installer.exe).Length
Get-FileHash artifacts\alpha-0.5\NAX5-windows-installer.exe -Algorithm SHA256
```

Ожидаемо:

- installer ~60–80 MB (как `chiaki-ng-windows-installer.exe`);
- portable ZIP ~100 MB;
- `chiaki.exe` в portable tree с `FileDescription = NAX5 Remote Play Client`;
- иконка/логотип NAX5 в UI и в `.exe`.

## Публикация на GitHub Releases

1. Обновите `RELEASES.md` в workspace `C:\astro`.
2. Создайте тег, например `alpha-0.5-build-2`.
3. Загрузите assets:
   - `NAX5-windows-installer.exe` (обязательно)
   - `NAX5-Alpha-0.5-Windows-x64.zip` (опционально)
4. Обновите `nax5-web/src/config/launcher.ts` (`releaseTag`, `fileName`, URL).
5. Задеплойте web, чтобы `/account` отдавал новую ссылку.

## CI (без локального MSYS2)

Workflow: `.github/workflows/build-nax5-alpha05-release.yml`

```powershell
gh workflow run build-nax5-alpha05-release.yml --repo alex1isupov-debug/NAX5
gh run list --workflow build-nax5-alpha05-release.yml --limit 1
```

Артефакты появятся в GitHub Actions run.

## Частые ошибки

| Ошибка | Решение |
| --- | --- |
| `Refusing to build a release tag with a dirty worktree` | закоммитьте или stash изменения |
| `Missing mingw-w64-x86_64-python-protobuf` | `bash scripts/release/setup-msys2-build-env.sh` |
| `Inno Setup 6 (ISCC.exe) is required` | установите Inno Setup 6, перезапустите MSYS2 |
| `ldd timed out` | повторите сборку; при повторе проверьте DLL в `build-alpha05/gui` |
| SmartScreen при установке | ожидаемо для unsigned alpha |

## Локальная разработка (не user pack)

- product: `scripts/nax5/start-nax5-local.cmd`
- operator: `scripts/nax5/start-nax5-operator-local.cmd`

Эти `.cmd` **не** попадают в user pack и installer.
