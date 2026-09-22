# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

ESP32 firmware (Arduino framework) for **CBA** turnstile passenger counters deployed at Metrô-DF stations. Each device sits on a station turnstile ("bloqueio"), detects full rotations via two sensors, and reports each passage (entry / exit / count error) to a web service over Wi-Fi, buffering to an SD card (and NVS as last resort) when the network is down.

Git repo: `https://github.com/iBizu/CBA-MetroDF` (public; `main`). Layout is nested (`CBAV24/CBAV24/`); the four sketch folders live in the inner directory. Code comments and serial messages are in Portuguese — keep new ones in Portuguese too.

**Secrets never go in the tree.** Each sketch folder needs a `segredos.h` (gitignored) with the Wi-Fi SSIDs/passwords and the Telegram token/chat; `segredos.h.exemplo` is the committed template — copy and fill it after cloning, or the sketch will not compile. `git grep` for a password before committing; the old Wi-Fi passwords are already in the public history, the Telegram token is not.

## Build / flash

No tests, no linter. Toolchain is Arduino IDE 2 with the `esp32:esp32` core **3.3.x** (ESP-IDF v5 APIs — e.g. `esp_task_wdt_config_t`; the code will not compile on core 2.x). Board: **ESP32 Dev Module**, FQBN `esp32:esp32:esp32`, default partition scheme.

Required user libraries (in `~/Documents/Arduino/libraries`): `esp32-micro-sdcard-master` (provides `mySD.h`) and `ESP32Time`. Everything else (`Preferences`, `WiFi`, `HTTPClient`, `HTTPUpdate`, `WiFiClientSecure`, `ArduinoOTA`, `ESPmDNS`) ships with the core.

`arduino-cli` is not on PATH but is bundled with the IDE. From `C:\Users\mathm\Downloads\CBAV24`:

```bash
CLI="$HOME/AppData/Local/Programs/Arduino IDE/resources/app/lib/backend/resources/arduino-cli.exe"
"$CLI" --config-file "$HOME/.arduinoIDE/arduino-cli.yaml" compile --fqbn esp32:esp32:esp32 CBAV24/Monetel_ID01
```

Serial upload: `./gravar.sh <Variante> <PORTA> [lab|estacao]` (`./gravar.sh --portas` lists boards) compiles and flashes over USB, applying the station override without touching the source. Network (ArduinoOTA) upload: `upload --protocol network -p <device-ip>`; the device advertises itself under the `nomeota` hostname, and the IDE network port works in every mode.

**Bench gotcha:** with `teste = 0` the model comes from the jumpers on `PIN_MOD1`/`PIN_MOD2` (GPIO32/33) — exactly one must read HIGH, or `setup()` prints "Erro ao habilitar modelo" and reboots every 5 s forever. On a bare bench board set `teste = 1`, which uses the `modelo` already in each variant's editable block (Monetel `2`, the others `1`).

The sketch uses ~88% of the 1.25 MB app slot. Do not switch to a larger single-app partition scheme — the GitHub OTA path needs the two OTA slots of the default scheme.

### Releases (OTA channels)

Two GitHub release channels, selected by `AMBIENTE_LAB` at compile time:

| channel | boards | GitHub release | URL base in firmware |
|---|---|---|---|
| **estação** | `AMBIENTE_LAB 0` | new release per version, tag = version (e.g. `3`), marked **latest** | `releases/latest/download/` |
| **lab** | `AMBIENTE_LAB 1` | fixed tag `lab`, marked **pre-release** (never becomes `latest`); replace its assets to push a bench test | `releases/download/lab/` |

State as of 2026-09-22: the current version is `4` — `lab` (pre-release) carries the `4` lab binaries, and release `4` carries the station ones. Neither `3` nor `4` is marked latest; `V1.0.0` still is. Promote with `gh release edit <tag> --latest` only once no board still runs pre-2026-09-22 firmware — a board on the old build reads `releases/latest/download/` and would take the station binary wherever it happens to be sitting. Release `3` was superseded before any board consumed it.

All four boards came back to the lab on 2026-09-22 for testing, so every one of them gets flashed over USB (`./gravar.sh <Variante> <PORTA> lab`) and no OTA migration is needed. An earlier attempt to migrate the three station boards over the air — station binaries uploaded to `V1.0.0` under the legacy names the old firmware requests (`Monetel_ID01.ino.bin`, `Foca_ID02.ino.bin`, and the Wolpac one with an **underscore**) — was undone for that reason; redo it only for a board that is physically at the station and running old firmware, remembering that the old code has no version check and flashes whatever it finds at 03:00. `V1.0.0` still holds the **lab** `Garen_ID03.ino.bin` plus `versao.txt` = `3`, which is the safety net for a bench board still on the reworked pre-channel firmware: it reads `latest` and lands on the lab build.

**Before a board leaves for the station, reflash it with `estacao`** — a board still carrying the lab build looks for `WIFI-ARHD` and never reports. The Telegram boot message states the environment, so check it after flashing.

Each release carries the four `<Variant>.ino.bin` plus `versao.txt` (content = `FW_VERSION` of those binaries). `./gerar_release.sh [estacao|lab|ambos]` (Git Bash, repo root) builds everything into `release/estacao/` and `release/lab/` — station binaries are built with `--build-property "compiler.cpp.extra_flags=-DAMBIENTE_LAB=0"`, so the committed source keeps the lab default (`#ifndef AMBIENTE_LAB`). Upload the folder's contents as the release assets; `release/` and `build/` are gitignored. Bump `FW_VERSION` in all four sketches before building, and keep it identical across them (the script refuses otherwise). `Sketch > Export Compiled Binary` in the IDE also works (writes `<Variant>/build/esp32.esp32.esp32/<Variant>.ino.bin`), but only for whatever `AMBIENTE_LAB` the source currently says.

## Four variants, one codebase

`Monetel_ID01`, `Foca_ID02`, `Garen_ID03`, `Wolpac-ID04` are four copies of the same sketch, one per turnstile manufacturer/device. The only intended differences are the **"VARIÁVEIS EDITÁVEIS"** block at the top of each `<Variant>.ino`:

| var | meaning |
|---|---|
| `FW_VERSION` | this build's version (plain integers: `"3"` as of 2026-09-21). Must equal the `versao.txt` of the GitHub release that ships this binary; keep it the same across the four variants |
| `OTA_ASSET` | this variant's binary name inside the release — the name the IDE exports (`<sketch>.ino.bin`, so `Wolpac-ID04.ino.bin` with a hyphen) |
| `AMBIENTE_LAB` | `1` lab (`WIFI-ARHD`, test host `10.66.24.196`, OTA from the `lab` release every `OTA_INTERVALO_MIN`, Telegram summary every `RESUMO_INTERVALO_MIN`); `0` station (`POC_MANUTENCAO`, `wsserver02-prod…`, OTA from `latest` at `OTA_HORA:OTA_MINUTO` = 03:00, summary at `RESUMO_HORA:RESUMO_MINUTO` = 23:45; the station closes 23:30). Guarded by `#ifndef` so `-DAMBIENTE_LAB=0` on the command line overrides it; from the IDE, edit the define |
| `numid` | device id sent in every message (1–4) |
| `tipoEntr` | sensor electrical type: `1` analog voltage divider (Garen, Wolpac), `2` digital/Zener (Foca). Ignored for Monetel |
| `modelo` | turnstile mechanics: `1` = 4 quarter-turns per passage (Foca/Garen/Wolpac), `2` = 2 half-turns (Ascom/Monetel). Only honored when `teste = 1`; otherwise auto-detected from hardware jumpers at boot |
| `nomeota` | ArduinoOTA hostname and the prefix of every Telegram message |
| `TG_TOKEN`/`TG_CHAT` | Telegram bot + group (same for all four), from `TELEGRAM_TOKEN`/`TELEGRAM_CHAT` in `segredos.h` |
| `ssid`/`password`, `ssidOTA`/`passwordOTA` | from `WIFI_LAB_*` / `WIFI_ESTACAO_*` / `WIFI_OTA_*` in `segredos.h`. `ssidOTA` is the network joined when the OTA jumper (GPIO0–GPIO4) is shorted at boot — the emergency path to reflash over Wi-Fi when USB is broken |

| `ADC_ACIONADO` / `ADC_REPOUSO` | sensor thresholds for `tipoEntr = 1`, as raw 12-bit ADC counts: a sensor reads **below** `ADC_ACIONADO` when triggered and **above** `ADC_REPOUSO` at rest, with a deliberate dead band between. Wolpac is `600`/`730` (validated counting on the bench 2026-09-22: its board drops the pin voltage to ~0.70 V at rest ≈ 869 counts and ~0.37 V triggered ≈ 459 — note that `150`/`200` was tried on the same board and did **not** work); the rest are `300`/`450`, never recalibrated. If both sensors read on the same side, the firmware sees the mechanically impossible "both triggered" combination, jumps to `cont = 2` and spins there forever — no counting, and `tarefasOciosas()` never runs, so OTA, summaries and queued Telegram messages all stop silently. That is the first thing to check when a board goes quiet |
| `DEBUG_ADC` | `1` turns `loop()` into calibration mode: the board counts nothing and prints all six sensor pins (analog and digital) to serial twice a second. Measure the real numbers here instead of converting from volts — the ADC is non-linear and the on-board divider varies by variant |

**When you fix a bug in a shared tab, apply it to all four folders** — there is no shared source; `diff` the folders to check drift. All four `loop.ino` are now byte-identical (the Wolpac thresholds moved into the constants above), so a `diff` there should come back empty. The reference copy is `Garen_ID03`, and the port to the other three was done by generating each main sketch from Garen's with only the variant lines substituted (2026-09-21; pre-port originals in `backup/`).

Never hardcode a threshold back into `loop.ino`: the comparison appears ~70 times, and editing one of them (which is what happened on the bench on 2026-09-22) leaves the other 69 on the old value and produces exactly the stuck-board symptom above.

### Features (all four variants, since 2026-09-21)

- **OTA** (`ota_update.ino`): downloads `<OTA_URL_BASE><OTA_ASSET>` (channel per `AMBIENTE_LAB`, see Releases) only when `versao.txt` from the same release is numerically **greater** than `FW_VERSION` — never a downgrade. Checked at boot (`OTA_CHECAR_NO_BOOT`) and on the schedule. `onProgress` feeds the WDT during the download. Telegram gets "baixando"/"FALHA" once per cloud version; success is announced by the next boot message.
- **Rollback**: `rollback_hook.cpp` (`verifyRollbackLater()` → true) + `verificarEstadoOTA()` early in `setup()` confirm the image only after NVS/WDT init. A version that crashes before that rolls back and is stored in NVS `ota/rejeitada`; a downloaded binary whose own `FW_VERSION` differs from what `versao.txt` promised is also blocked (otherwise the board would re-flash it forever).
- **Clock**: no NTP. `configurarFuso()` only sets TZ (`<-03>3`); the web service is the single time source (verified: it sends UTC epoch). `tempo()` returns `bool`; `sincronizarRelogio()` keeps counting offline if the RTC is already valid (it survives `ESP.restart()`/WDT), and only reboots when the clock was never set.
- **Telegram** (`telegram.ino`): `enviarTelegram()` (JSON POST, 10 s handshake timeout) sends now; `agendarTelegram()` queues for the idle loop (`enviarTelegramPendente()`, ≤1 try/min). Boot message: reset reason / `ATUALIZADO x -> y` / `ROLLBACK`, environment, `JUMPER OTA` flag, Wi-Fi, clock, SD state. `resumo.ino`: periodic summary (deltas vs. NVS `resumo/E,S,W`, pending lines in `LOGREG.csv`), queued if Wi-Fi is down. `falhou()` queues one SD-failure alert per boot.
- **`tarefasOciosas()`** (`resumo.ino`) is the single place blocking work runs — IDE OTA, GitHub OTA, summary, queued Telegram — called from both `cont == 0` idle loops only.
- **`setupOTA()`** gives up after 15 s and prints which of three modes it is in (jumper ⇒ "modo físico OTA" on `ssidOTA`; else lab or station per `AMBIENTE_LAB`); the IDE network port is enabled in every mode.
- **Precision fixes**: `leId()` opens `LOGREG.csv` read-only and sets `flagSD` from `size()` (before: `FILE_WRITE` put the cursor at EOF ⇒ `available()==0` ⇒ `flagSD` always 1 ⇒ `atrasado()` ran on every passage); `loop()` no longer mounts the SD to delete an empty `LOGREG.csv`; both LOGREG line formats unified to `sentido;timestamp;`; the `;` missing in the delayed packet in `sendMessage.ino` restored (it used to be compensated by accident via the trailing `;` kept in `timestamp2`); `servidor()`'s critical-timeout branch now sets `flagSERV=1`/`resposta="222"` instead of inheriting the previous passage's state.

## Sketch structure (multi-tab)

Arduino concatenates every `.ino` in a folder into one translation unit (main sketch first, then tabs alphabetically), so each tab is a single global function with no headers and no prototypes. Globals used across tabs must live in `<Variant>.ino` — a global defined in a tab is invisible to tabs that sort before it. `ota_update.ino`, `telegram.ino` have their own `#include`s. Anything needing C linkage (a core weak hook) must go in a `.cpp` file, because the Arduino preprocessor emits C++ prototypes for every function in a `.ino` (see `rollback_hook.cpp`).

| tab | role |
|---|---|
| `<Variant>.ino` | globals, pin map, `setup()`, `setupOTA()` |
| `loop.ino` | one turnstile event per iteration: reconcile previous send, then block in a sensor-polling state machine |
| `sendMessage.ino` → `servidor.ino` | build the packet and HTTP GET it to `/enviarContagem/<numid>;<timestamp>;<atraso>;<sdErr>;<sentido>;` |
| `tempo.ino` | `tempo()`: one bounded GET `/recuperaTimestamp`, sets the `ESP32Time` RTC; `sincronizarRelogio()`: boot policy around it |
| `card.ino`, `cardID.ino`, `leId.ino` | SD file I/O (see below) |
| `atrasado.ino` | replay the SD backlog and NVS-lost count to the server |
| `ota_update.ino` | TZ, GitHub version check + `HTTPUpdate`, rollback bookkeeping (`verificarEstadoOTA`) |
| `telegram.ino` | `enviarTelegram`/`agendarTelegram`, reset reason, boot message |
| `resumo.ino` | periodic summary, `contarPendentesSD()`, `tarefasOciosas()` |
| `rollback_hook.cpp` | `verifyRollbackLater()` → true (C linkage) |
| `checkNVS.ino`, `fail.ino`, `falhou.ino` | NVS integrity check at boot; SD-failure flag reset / alert |

## Runtime flow

**`setup()`**: brown-out threshold → `checkNVS()` → OTA jumper probe (drive GPIO0 high, read GPIO4; shorted ⇒ `otaativ = 1` ⇒ join `ssidOTA` instead of `ssid`) → 120 s task watchdog with panic → `verificarEstadoOTA()` (confirm image / detect rollback; deliberately before Wi-Fi) → model jumper probe (drive `PIN_MOD0`, read `PIN_MOD1`/`PIN_MOD2`; neither/both ⇒ restart) → `configurarFuso()` → `setupOTA()` (Wi-Fi ≤15 s + `ArduinoOTA.begin()`) → `sincronizarRelogio()` → boot OTA check → `cardID()`/`leId()` to create/read the SD counter files → `enviarMensagemBoot()`.

**`loop()`** — each pass handles exactly one passage:
1. *Reconcile* the previous send using `flagSERV`/`resposta`: on failure append `sentido;timestamp;` to `LOGREG.csv` (or, if the SD also failed, bump NVS `counterF`); on success call `atrasado()` if there is a backlog.
2. *Sense* in a `goto`-label state machine (`cont0a…cont4b` for `modelo 1`, `cont0c…cont3c` for `modelo 2`). `cont` counts quarter-turns; `+4`/`+2` = entry (`sentido 1`), `-4`/`-2` = exit (`sentido 2`), an impossible sensor combination pushes `cont` out of range = error (`sentido 3`). `direcao` remembers the last direction to disambiguate glitches. `ArduinoOTA.handle()` and `verificarHorarioOTA()` only run in the idle `cont == 0` loop.
3. On a completed passage: `timestamp = rtc.getEpoch()` (or the literal `"E2"` if the epoch is still pre-2023, i.e. never synced), set `flagE/flagS/flagW`, `sendMessage()`.

Sensing is fully blocking — nothing is counted while `servidor()` is talking to the server. That is why `servidor()` uses deliberately tight timeouts (350 ms request / 700 ms connect / 800 ms total): a slow server must not eat the next passage. Keep any change here fast.

**Watchdog**: every function that can block calls `esp_task_wdt_reset()` liberally. Any new loop or long network call must do the same or the board panics and reboots after 120 s.

## Persistence conventions

Every SD access is wrapped in `digitalWrite(SD_CS, Select)` → `SD.begin(...)` → work → `SD.end()` → `DeSelect`; the SD bus is never left mounted. SD files, all in the root:

- `LOGREG.csv` — backlog of unsent passages (`sentido;timestamp;` per line)
- `ENTRADAS.txt`, `SAIDAS.txt`, `ERROS.txt` — running totals (single integer)
- `POSITION.txt` — byte offset in `LOGREG.csv` up to which `atrasado()` has already replayed

`cardID()`/`leId()` are driven by a flag protocol rather than arguments: `flagE/flagS/flagW/flagP` (`0` skip, `1` increment/write, `2` create-and-read) and `escreve` (`0` write, `1` delete). A counter update is always "delete file, then rewrite" (`escreve = 1; cardID(); escreve = 0; cardID();`). `flagF = 1` means the last SD access failed; `fail()` resets all flags to `2` so the next successful mount recreates everything.

NVS namespace `"my-app"`, key `counterF`: number of passages lost because both the server and the SD failed. It is sent later by `atrasado()` with `debug = 0x06` and cleared on acknowledgment. Namespaces `"ota"` (`tentada`, `rejeitada`, `versao`) and `"resumo"` (`E`, `S`, `W`) are accessed through the second `Preferences` instance `prefsCBA` because `preferences` stays open on `"my-app"` for the whole loop iteration.

`resposta` is a three-digit string state: `"999"` idle, `"000"` server acknowledged, `"111"` awaiting ack / write to SD, `"222"` send failed. `debug` byte values are documented inline in `<Variant>.ino`.

## Hardware gotchas

- `pinoLedInterno` is **GPIO1 = UART TX**. The code deliberately does `Serial.end()` before driving the LED and `Serial.begin()` after (first three passages only, via `ledaux`). Don't "clean up" those calls; serial output is lost by design during the blink.
- Pin map (`PIN_BTN1/2` = 15/2 for Monetel, `PIN_BTN3/4` = 16/17 digital, `PIN_BTN5/6` = 36/39 analog, `PIN_MOD0/1/2` = 25/32/33, SD on VSPI 18/19/23/5) is in `<Variant>.ino`; the SD `#define` comments record which GPIOs were found not to work.
- GitHub OTA uses `client.setInsecure()` (no cert validation) and follows redirects, because GitHub release assets 302 to a CDN.
