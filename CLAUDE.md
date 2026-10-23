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

Serial upload: `./gravar.sh <Variante> <PORTA> [lab|estacao]` (`./gravar.sh --portas` lists boards) compiles and flashes over USB, applying the environment seed without touching the source. Network (ArduinoOTA) upload: `upload --protocol network -p <device-ip>`; the device advertises itself under the `nomeota` hostname, and the IDE network port works in every mode.

**Bench gotcha:** with `teste = 0` the model comes from the jumpers on `PIN_MOD1`/`PIN_MOD2` (GPIO32/33) — exactly one must read HIGH, or `setup()` prints "Erro ao habilitar modelo" and reboots every 5 s forever. On a bare bench board set `teste = 1`, which uses the `modelo` already in each variant's editable block (Monetel `2`, the others `1`).

The sketch uses ~90% of the 1.25 MB app slot (1 179 836 bytes on version 13, leaving ~128 KB). Do not switch to a larger single-app partition scheme — the GitHub OTA path needs the two OTA slots of the default scheme.

### Releases (OTA channels)

Two GitHub release channels, selected by `AMBIENTE_LAB` at compile time:

| channel | boards | GitHub release | URL base in firmware |
|---|---|---|---|
| **estação** | `AMBIENTE_LAB 0` | new release per version, tag = version (e.g. `3`), marked **latest** | `releases/latest/download/` |
| **lab** | `AMBIENTE_LAB 1` | fixed tag `lab`, marked **pre-release** (never becomes `latest`); replace its assets to push a bench test | `releases/download/lab/` |

State as of 2026-10-20: the published version is `7`, promoted to **latest** on 2026-10-20 (`gh release edit 7 --latest`) — `lab` (pre-release) carries the `7` lab binaries, release `7` the station ones, and `7` is the first release with the millisecond timestamp. Right after the promotion GitHub's edge cache was still answering the old `versao.txt` on the plain `releases/latest/download/` URL; it had caught up the same day, and `latest`, tag `7` and tag `lab` all answer `7` (checked 2026-10-20). A stale read there is harmless anyway, because the firmware refuses downgrades. Earlier releases were superseded before any board consumed them.

**Channel state (2026-10-23): both channels carry `12`** (release `12` is **latest**, tag `lab` is the pre-release); the source is at `14`, two changes ahead — the adoption warning in the boot message (see *Legacy backlog*) and the cap on OTA download attempts (see the Garen incident). `12` carries the three fixes from the Garen incident below — the flash counter joining the environment guard, `pendentes no SD` no longer lying when the mount failed, and backwards-version detection. Earlier, version `10` had reached the bench Garen over the air: the board was on `9`, read `10` on the first scheduled check after the upload, downloaded, rebooted, logged `Primeiro boot apos atualizacao: imagem confirmada` and announced `ATUALIZADO: 9 -> 10` on Telegram. The ~3 h of `Versao na nuvem: 7` before that upload, each refused as a downgrade, came free with it. What `10` carries over `7`: environment detection, the `tempo()` reconnect fix, the cross-environment backlog guard, and `TEMPO_REDE_S` at 20 s.

**The cross-environment backlog guard was published without a hardware test, deliberately (2026-10-21).** Exercising it needs a turnstile to generate real failed passages, and the only turnstile is at the station — so the test cannot happen before the deployment it was meant to protect. The bench procedure is written up under *Shelved: bench test of the backlog guard* below; run it the first time a board comes back with a turnstile available.

What makes that acceptable is that **the guard is inert on a board that does not change environment**. At the station the origin marker is written as `ESTACAO` and the board stays in `ESTACAO`, so `backlogDeOutroAmbiente()` is false and `atrasado()` behaves exactly as it did before the guard existed. The guard only has an opinion when a board crosses environments, which from here on is a bench event, under observation. Two conditions keep it that way:

- **Every card leaves the bench clean** — `LOGREG.csv` and `POSITION.txt` deleted together. A card carrying bench backlog arrives at the station marked `LABORATORIO`, and then the untested path runs unobserved on day one.
- **A board that comes back to the bench gets its serial read**, because that is the first time the guard actually decides something: the boot message must carry the `ATENCAO:` line and `pendentes no SD` must stay `sim`.

If the guard were wrong, the failure is asymmetric and that is why this is tolerable: a false block **holds** counts on the card (visible in the boot message, recoverable by returning the board), while a false pass would leak a handful of bench turns into the production BI. Neither loses station counts.

**Upload the `.bin`s first and `versao.txt` last.** The other order makes the channel announce a version it does not yet serve, and a board checking inside that window downloads the old binary, finds its own `FW_VERSION` different from what `versao.txt` promised, and writes that version to NVS `ota/rejeitada` — after which it would never accept the real one.

`11` added two things to `10`: `modelo 1`'s serial label now reads `Modelo 1 - Foca/Garen/Wolpac` instead of just `Foca` (the Wolpac folder already had the full wording — folder drift, found in a 2026-10-21 bench log), and an origin marker found with no backlog behind it is dropped at boot. Only the Garen ran `10` on hardware; the other three went from `7` (Foca from `5`) straight to `11` over the air, on the lab channel, each confirming the image and announcing `ATUALIZADO` — the OTA path is now exercised on all four boards.

Validated on the bench 2026-09-23 (Wolpac): counting, the full OTA cycle (`4` → `5`, download, reboot, `Primeiro boot apos atualizacao: imagem confirmada`, Telegram `ATUALIZADO`), and the stuck-state watchdog restoring the scheduled OTA check. To force an OTA test, temporarily lower that variant's `FW_VERSION` below the channel's, flash over USB, and restore it afterwards — all four must share one version or `gerar_release.sh` refuses.

**Fleet state (2026-10-21):** the Garen runs `11`; the other three were last flashed with `7` and are on the bench. A bench board on the lab build of `7` reads the `lab` channel, so it pulls `11` by itself once powered on that network — no USB needed, and it exercises the OTA path on three more boards. The exception to watch is the Wolpac: releases `5`–`7` shipped a **station** build on the lab channel, so that board may be looking for `POC_MANUTENCAO` and never reach the lab channel at all. If it does not show up, USB is the only way in.

An old-firmware board is now also safe to migrate over the air, which it was not before: `latest` carries `11`, the old code has no version check and flashes whatever it finds at 03:00, and what it finds is the auto-detecting build — which then settles on the right environment wherever it is. The one gap is the asset name: the pre-`8` firmware asks for the Wolpac binary with an **underscore** (`Wolpac_ID04.ino.bin`) while the release ships a **hyphen**, so that request 404s and the board simply does not update. The legacy `V1.0.0` release (lab `Garen_ID03.ino.bin`, `versao.txt` = `3`) is obsolete as a safety net and can be ignored.

**Since version 8 a board no longer has to be reflashed when it changes place** — it detects the environment at boot (see *Environment detection*). The third argument of `gravar.sh` now only seeds the NVS, which detection would correct anyway. Still check the Telegram boot message: it says which environment the board settled on.

**Since version 8 there is one binary per variant, identical in both channels.** What separates bench from station is the **version** published in each channel, not the content of the `.bin`, so publishing only to `lab` is still how a build is tested without reaching the station. `conferir_binario` in `gerar_release.sh` changed accordingly: it now asserts each binary carries **both** networks and **both** OTA URLs, i.e. that it really is the auto-detecting build, and aborts otherwise.

The old check did the opposite — it required the station binary *not* to contain the test host — and it existed because that went wrong: the Wolpac sat with `AMBIENTE_LAB 0` committed from 2026-09-23 (flashed for the station, the edited define swept in by a `git add -A`), the lab channel built from the source default, and releases `5`, `6` and `7` shipped a **station** Wolpac on the lab channel. A bench board that pulled it went looking for `POC_MANUTENCAO` and disappeared, with no network left to receive a fix over the air. That failure mode is now structurally gone: there is no such thing as a binary belonging to the wrong environment.

### Environment detection (since version 8 — validated on hardware 2026-10-20/21)

`AMBIENTE_LAB` is no longer what decides where a board lives. `conectarAmbiente()` (`ambiente.ino`), called from `setupOTA()`, finds out at boot by **trying to connect**, and whichever network answers sets the Wi-Fi credentials, the web-service host, the OTA channel and the two schedules.

Connecting is the test rather than a scan because **`POC_MANUTENCAO` is a hidden network** — `WiFi.scanNetworks()` returns it with an empty SSID and it cannot be recognised by name. Associating is also the stronger evidence: it proves the password is right and the AP is accepting this board, which a name in the air does not.

- **Order**: the environment remembered in NVS first (the normal case, so steady-state boots are fast), then the other one (the board that moved). Each attempt is capped at `TEMPO_REDE_S`.
- **Neither network answers**: the board **keeps** the remembered environment and starts offline, counting to the SD as always. Deliberately no guessing here — a station board whose AP is down that "fell back to bench" would start pointing at the test server and the `lab` OTA channel, and would only come back with a site visit. No network means no new information, so nothing changes.
- **Jumper OTA** (GPIO0–GPIO4 shorted): the programming network wins and the environment is **not** redefined. The board is there to be flashed, not to start living there.
- **No re-probing during operation**, on purpose: detection costs seconds and sensing is blocking, so re-probing from the idle loop would blind the turnstile. It is also unnecessary — moving a board means unplugging it, which means a boot.
- The environment lands in NVS namespace `"ambiente"`, key `"lab"`, and the Telegram boot message states it, whether it was confirmed by a connection or only kept from NVS, and whether it **changed** this boot. That message is the operator's confirmation.

What this removes: the "reflash it before it goes back" step, and with it the two incidents it caused (the `AMBIENTE_LAB 0` swept into a commit and published to the `lab` channel; the Wolpac that sat mute on the bench). A binary is no longer right or wrong for a place.

Validated on the bench with the Garen on 2026-10-20 and 2026-10-21, all three paths: seeded `estacao` in the lab it spent ~12 s on `POC_MANUTENCAO`, connected to `WIFI-ARHD`, and wrote `bancada` to NVS; the next boot went straight there with no `(MUDOU)`; and with `WIFI_LAB_SSID` temporarily pointed at a nonexistent name, neither network answered and it **kept** `LABORATORIO` instead of flipping. Pointing a gitignored `segredos.h` SSID at a bogus name is the cheap way to exercise these paths without touching the AP.

**`TEMPO_REDE_S` was raised from 12 s to 20 s on 2026-10-21**, because 12 was not enough. The Garen read `WIFI-ARHD` as absent across the full 12 s and fell into "neither network answered", yet was associated to it seconds later: the boot message said `ambiente: LABORATORIO (mantido da NVS; nenhuma rede conhecida respondeu)` while the board was sitting at `192.168.0.20`. Boots that connected in ~2 s had RSSI -65/-66 dBm; that one was -71.

A present network misread as absent matters more here than anywhere else, because this is the **decision** about which environment the board is in. A freshly flashed board arriving at the station would keep the bench seed and point at the test server until some later boot managed to associate inside the window. It self-heals across reboots, but wide margin is cheap — it costs boot time, and only when a network really is missing.

`tempo()`'s own 10 s attempt was left alone: `sincronizarRelogio()` retries it eight times, and an association that completes after one attempt gives up is picked up by the next one, which is exactly what that log shows (attempt 1 timed out, attempt 2 found the board already connected).

### The backlog crosses environments (version 9)

A `LOGREG.csv` line is only `sentido;timestamp;` — it does not record which server it was for, and `atrasado()` delivers it to whatever `host` the current environment set. So a board that banked real passages at the station and then came to the bench would push them to the **test** server (the BI never sees them), and a board carrying bench turns to the station would push those into **production** as if they were passengers. Silently, both ways: the lines are valid, the server answers `000`, and they leave the file.

So the environment the backlog was written in lives in NVS (`ambiente/backlog`), and `atrasado()` refuses to send when it does not match the current one:

- **Blocked, not discarded.** Take the board back to the environment the lines came from and the marker matches again, so they drain normally. Nothing is lost, it just waits. The Telegram boot message reports the held count and what to do.
- **The NVS counter is covered too (since version 12).** `counterF` — the passages lost when the server *and* the card failed — used to have no origin marker at all, because only `card.ino` wrote one. So a board whose card had died banked its counts in flash unprotected, and `atrasado()` would hand them to whatever server the current environment pointed at. Both `counterF++` sites in `loop.ino` now call `marcarAmbienteBacklog()`, `adotarAmbienteBacklog()` reads the counter straight from NVS (`pendentesNaFlash()`, because the global `counterR` is only filled in `loop()`), and the marker is released at the **end** of `atrasado()`, when `flagSD != 1 && flagR == 0` — both stores drained — instead of falling with the `LOGREG.csv` alone.
- **Legacy backlog** (written before this guard, so unmarked) adopts the current environment on the first boot, in `adotarAmbienteBacklog()` — the assumption being that this is where it was accumulated. With the card mute (`flagF != 0`) it neither adopts nor clears on the `LOGREG.csv` side: nothing can be concluded about a file that did not answer.
- **That adoption is a guess, and version 12 showed where it misses (2026-10-23).** On its first `12` boot the bench Garen printed `[SD] Backlog sem origem marcada: assumindo LABORATORIO.` — and those were the 151 **station** counts from its own incident, now marked as bench. The assumption fails in exactly the case the guard exists for, and for a reason that is structural rather than unlucky: a dead card is the kind of fault that gets a board carried to the bench, so by the time the guard first sees that data the board has already moved. Worse, the marker then points the wrong way round — at the bench the counts drain to the **test** server, and taking the board back to the station **blocks** them, which is the one place they belonged.
- **So since version 13 the adoption is announced.** `origemBacklogAdotada` reaches `enviarMensagemBoot()`, which states what was assumed and how many counts it covers, so the operator sees the guess on their phone and can act before the first passage drains the backlog. There is no better fix available: the origin of unmarked data does not exist anywhere to be read, and refusing to adopt would strand every legacy board as permanently blocked. Making the guess visible is the whole remedy, which is why it is worth a release of its own.
- **Mixed origins**: if a blocked board banks *new* failures locally, the file holds lines from two environments and per-file routing cannot sort them. It is marked `MISTO`, blocked permanently, and the decision becomes explicitly human instead of the firmware picking the wrong server for half the lines.
- **Orphan marker**: the marker lives in NVS and the card is cleared by hand, so deleting `LOGREG.csv` leaves a marker with no file behind it. The next failed passage would then read as a *second* origin and the file would be marked `MISTO` — permanently blocked, with a single origin. `adotarAmbienteBacklog()` therefore drops a marker it finds with no backlog (`flagF == 0 && flagSD != 1`) at boot. It checks `flagF` first because `fail()` does not touch `flagSD`, so after a failed mount `flagSD` still holds the previous read's value and means nothing.

**Clearing a card is done by hand, on purpose.** Pull the microSD and delete `LOGREG.csv` **and** `POSITION.txt` together — never one without the other, which is what produced the `sentido 62`. A firmware route for this (a `--limpar` flag on `gravar.sh` arming a one-shot wipe) was built and then dropped on 2026-10-21: it needed a Git Bash flash rather than the IDE, with the serial monitor closed and usually a manual BOOT+EN, which is more friction and more ways to go wrong than opening the board. If it is ever revived, the hard requirement is that an armed binary must be mechanically impossible to publish — a board pulling such a build over OTA would wipe its own pending counts with nobody asking.

### The Garen at the station, 2026-10-23 — three symptoms, one board

The Garen went to the station and came back the same week in a loop: `Reiniciou - motivo: watchdog de tarefa (travou mais de 120 s)` every few minutes since ~05:00, `SD: FALHOU`, and counts piling into flash (144 at 09:29, 151 by the time it reached the bench). The other three boards were fine, so this is board-specific, not a firmware regression. Wi-Fi at the station was **−82 dBm**, which is its own field problem: weak signal → sends fail → the card should catch them → the card was dead → only the flash total survives, with no timestamps.

Two things the incident taught, both now fixed:

- **The flash counter had no environment marker** (above). Bringing that board to the bench would have posted 147 station passages into the test database on the first successful send, and `atrasado()` fires after *any* acknowledged event — a floating-sensor `sentido 3` is enough. Nothing in the guard would have stopped it.
- **`pendentes no SD: nao` lied.** With the mount failing, `flagSD` keeps the previous read's value, so the boot message asserted the card was empty while nothing had been read from it. It now prints `desconhecido` whenever `flagF != 0`, and the held-backlog `ATENCAO:` line reports the flash count beside the card count.

**It went 11 → 10, and it was not a rollback.** At the station the board reported `Versao: 11`; at the bench it came up as **10** and downloaded 11 again. Both `Versao:` and `[OTA] Versao da placa:` print the compiled `FW_VERSION`, so the running image really did change. The flash was dumped over USB (`esptool read-flash 0xe000 0x282000`, otadata + both app slots) and it settled the question:

| evidence | reading |
|---|---|
| app0 @ `0x10000` | version `10` (serial label `Foca`) |
| app1 @ `0x150000` | version `11` (label `Foca/Garen/Wolpac`), **byte-identical** to the published `Garen_ID03.ino.bin` — sha256 `ebb7b8195da0…` |
| otadata, both copies | `ota_seq = 2`, state **`VALID`**, CRC ok → active slot = app1 |

No `INVALID`, no `ABORTED`: the bootloader never rejected anything, and version `11` was sitting intact in app1 the whole time. What changed was the **boot pointer**. Two signs: the two otadata copies are *identical*, where normal OTA alternates sectors and leaves them one apart (identical copies are the signature of a whole-partition write — an Arduino USB upload writes an 8 KB `boot_app0.bin` over both); and `ota_seq = 2` is far too low for a board that has been through `9 → 10 → 11`. The operator confirmed no IDE upload happened after the lab, so the partition lost its contents on its own. With otadata gone the bootloader falls back to the first slot, which still held `10`.

This also killed the re-flash loop's mystery: running `10`, the board sees cloud `11` and downloads it again, writing a full OTA slot per cycle.

**`verificarEstadoOTA()` could not have caught it**, and neither could a correct rollback check: it detects a rollback only while NVS `ota/tentada` is set (deleted on the boot right after an update), and `esp_ota_get_last_invalid_partition()` returns `NULL` here because no partition was ever invalidated. Worse, the line that stores `ota/versao` silently overwrote `11` with `10`, erasing the evidence, and the boot message announced it as `ATUALIZADO: 10 -> 11`.

**Fixed in version 12 by not relying on the ESP-IDF bookkeeping at all**: `verificarEstadoOTA()` now compares the stored `ota/versao` against the running `FW_VERSION` with `compararVersao()`, and a *backwards* result sets `houveRetrocesso`, bumps NVS `ota/retrocessos` and leads the boot message with `RETROCESSO:` instead of `ATUALIZADO:`. It **only reports** — deliberately. The new version has no defect (it is intact in the other slot), so marking it rejected would punish it for a pointer problem and strand the board on the old image. The counter is what turns a one-off into a pattern.

**Practical consequence:** whatever version sits in app0 is where a pointer loss lands. A USB upload writes app0 *and* resets the pointer to it, so flashing a board over USB with the current build is how you stop a fallback from being a downgrade. Leaving a stale image in app0 means every pointer loss repeats this episode.

**Root cause of the SD failure: a cold solder joint on the card reader**, resoldered on the bench 2026-10-23, after which the same board mounted a card on the first try (`SD Ok!`, `pendentes no SD: nao`) cold and freshly powered. Two different cards, one of them new and FAT32, had both failed before that — which is what moved the suspicion from the cards to the board. The original card was never retested on a working reader, so it is an unknown rather than a known bad one.

**The 120 s hangs did not reproduce on the bench, with or without a card.** The board ran 2 h 56 min with the faulty card still installed and never once tripped the task watchdog, heap flat at 201 KB; the no-card run after it was quiet too, but says little, because the control condition already passes. So the hangs belong to the station, and there are two candidates:

- **An intermittent (not open) joint**, stalling SD operations inside the library where nothing feeds the watchdog. Fits that the fault was mechanical, that the station has a vibrating turnstile while the bench does not, and that the card failed *fast* at the bench once the joint had opened completely.
- **The OTA retry loop.** On the station the board was running `10` with `11` published, and `OTA_CHECAR_NO_BOOT 1` re-runs the check on *every* boot, so each watchdog reboot started another 1.18 MB download over a −82 dBm link: reboot → check → download → hang → reboot, about 2.5 min a turn, which matches the observed cadence. The blind spot is real either way — between the `esp_task_wdt_reset()` before `httpUpdate.update()` and the first `onProgress` callback, nothing feeds the watchdog while DNS, TCP, the TLS handshake and the response headers happen, and `client.setTimeout(30000)` bounds each read at 30 s, so four slow reads reach 120 s.

The first is the more economical explanation, and it needs no coincidence with the OTA. Neither can be tested on this board any more, because the defect is gone: **if the hangs do not come back at the station, it was the joint.** Worth having regardless, and **implemented in version 14**: a cap on download attempts, so a board that cannot complete an update fails it and goes back to counting instead of rebooting all day. The damage at the station was not the failed update, it was a full day of counting nobody.

- `OTA_MAX_TENTATIVAS` (3) download attempts per cloud version. The count lives in NVS (`ota/tentvers` + `ota/tentativas`) and is written **before** `httpUpdate.update()`, because the attempt that kills the board is precisely the one that would otherwise leave no trace: `verificarEstadoOTA()` clears `ota/tentada` on every boot, so a watchdog death inside the download is currently invisible. The existing `tentada` key could not be reused for the same reason — a count that resets every reboot brakes nothing.
- **The scheduled check opens a fresh window** (`novaJanela`), the boot check does not. That is the whole mechanism: the reboot loop is made of *boot* checks, so that is what needs the brake, while the scheduled check is the deliberate opportunity. At a station that means at most 3 attempts a day, since the schedule there is 03:00 only; the board then counts for the rest of the day and tries again on its own tomorrow. On a bench, where the schedule runs every 5 minutes, behaviour is essentially unchanged — which is what you want while someone is watching.
- Giving up is announced on Telegram once, naming the attempt count and suggesting USB. The "baixando" message now carries `tentativa N/3`, so the burst is legible in the group rather than looking like a flood.
- **What this does not do:** it counts *download* attempts, so a hang inside `lerVersaoNuvem()` would not be caught. That path is far less exposed (one small GET, already wrapped in two retries that return rather than hang) and it is not where the evidence points, since the station board clearly managed to read `11` from `versao.txt` often enough to keep retrying the download.

One thing deliberately left alone: the TLS and read timeouts. `setHandshakeTimeout(25)` already bounds each handshake, so two handshakes plus headers come to well under the watchdog's 120 s — which is itself a reason to prefer the solder explanation for the hang over anything in the network path. Those values are also measured rather than guessed (8 s gave `HTTP -11` on the bench), and the comment that records that should not be overwritten casually.

**Procedure learned — a board that comes back with a dead card must have its flash counter cleared before it returns.** The counts banked in `counterF` adopt the current environment the first time the guard sees them (see *Legacy backlog*), which on a bench is the wrong one. Leaving that marker is not merely cosmetic: back at the station, `marcarAmbienteBacklog()` sees origin `LABORATORIO` against a board in `ESTACAO`, concludes the set has two origins and marks it **`MISTO`** — permanent block — so the next station passage that fails to send strands *real, timestamped* counts together with the stale bench-marked ones. There is no manual route for `counterF`, so clearing it means erasing the NVS region over USB:

```bash
esptool --port <COM> --chip esp32 --before default-reset erase-region 0x9000 0x5000
```

Done on the Garen 2026-10-23 (the 151 counts were already written off). It also drops `ambiente/lab` (detection rebuilds it), `ota/versao` (the next boot prints the plain reset reason, since the `ATUALIZADO` line requires a non-empty stored version), `ota/retrocessos` and the `resumo` baselines (the next summary shows totals, with the note it already carries for that). `checkNVS()` recreates the structure on the next boot and restarts once by itself. The carrier board holds GPIO2 high, which blocks automatic entry into the serial bootloader, so **BOOT has to be held by hand** while the command runs — the same thing the otadata read needed.

### Shelved: bench test of the backlog guard

Written 2026-10-21 and **not yet run** — it needs a turnstile to produce real failed passages, and the only turnstile is at the station. Run it the first time a board is on a bench with one. It touches `segredos.h` only, never tracked source.

The lever: the environment picks the Wi-Fi **and** the host. A board that adopts `ESTACAO` while sitting in the lab points at the production server, which is unreachable from there — so every passage fails and lands on the card marked `ESTACAO`.

1. **Create `ESTACAO` backlog.** In that variant's `segredos.h`, put the real bench credentials in `WIFI_ESTACAO_SSID`/`WIFI_ESTACAO_SENHA` and a nonexistent name in `WIFI_LAB_SSID`. Flash over USB. The board spends `TEMPO_REDE_S` failing on the bogus name, then associates as `ESTACAO` and says so (`Ambiente mudou para ESTACAO e foi gravado na NVS`). The clock will not sync — the time source is that same unreachable host — so the lines carry the literal `E2`, which `backlogValido()` accepts and which is what a board that never reached its server really records. Turn the turnstile 3–4 times; each turn should print `LogReg Gravado!`.
2. **Return to the bench environment.** Restore `segredos.h`, flash again, do **not** clear the card. Expect `pendentes no SD: sim`, the boot message's `ATENCAO:` line naming `ESTACAO`, and — after one *new* turn, because `atrasado()` only runs after a successful send — `[SD] Backlog gravado em ESTACAO e a placa esta em LABORATORIO: nao vou enviar, iria para o servidor errado.` The real assertion is negative: the new turn reaches the test database and the held lines do not, and `pendentes no SD` stays `sim` across reboots. A passage that *fails* in this phase marks the file `MISTO`, which is correct and blocks permanently.
3. **Unblock.** Swap `segredos.h` back as in step 1 and flash: the marker matches again, the `ATENCAO:` line and the block message are gone. Delivery itself cannot be shown on the bench (the production host is unreachable), so what this proves is that returning to the origin environment releases the hold — the difference between holding and discarding.
4. **Discard and reset.** Delete `LOGREG.csv` **and** `POSITION.txt`, boot once, and expect `[SD] Nao ha backlog, mas havia marca de origem: marca removida.` Restore `segredos.h`.

Never build a release while `segredos.h` is swapped: the binary would carry the bench network in the station slot. `conferir_binario` would abort on the missing `POC_MANUTENCAO`, but do not lean on that.

Each release carries the four `<Variant>.ino.bin` plus `versao.txt` (content = `FW_VERSION` of those binaries). `./gerar_release.sh [estacao|lab|ambos]` (Git Bash, repo root) builds **one** binary per variant and copies it into `release/estacao/` and `release/lab/`, which is why a `sha256sum` of the two folders matches pair by pair. Since version 8 the two channels ship the same `.bin` — the board finds its own environment at boot, so what separates bench from station is the *version published in each channel*, not the file. `conferir_binario` asserts each binary carries both networks and both OTA base URLs, which is what makes the old "station binary on the lab channel" failure structurally impossible. Upload the folder's contents as the release assets; `release/` and `build/` are gitignored. Bump `FW_VERSION` in all four sketches before building, and keep it identical across them (the script refuses otherwise). `Sketch > Export Compiled Binary` in the IDE also works (writes `<Variant>/build/esp32.esp32.esp32/<Variant>.ino.bin`), but only for whatever `AMBIENTE_LAB` the source currently says.

## Four variants, one codebase

`Monetel_ID01`, `Foca_ID02`, `Garen_ID03`, `Wolpac-ID04` are four copies of the same sketch, one per turnstile manufacturer/device. The only intended differences are the **"VARIÁVEIS EDITÁVEIS"** block at the top of each `<Variant>.ino`:

| var | meaning |
|---|---|
| `FW_VERSION` | this build's version (plain integers: `"14"` as of 2026-10-23). Must equal the `versao.txt` of the GitHub release that ships this binary; keep it the same across the four variants |
| `OTA_ASSET` | this variant's binary name inside the release — the name the IDE exports (`<sketch>.ino.bin`, so `Wolpac-ID04.ino.bin` with a hyphen) |
| `AMBIENTE_LAB` | **only the seed** for the environment the board assumes on its very first boot with empty NVS, and the one it keeps when neither network answers. The environment itself is detected at runtime — see *Environment detection* below. Still guarded by `#ifndef`, so `gravar.sh`/`gerar_release.sh` can set the seed on the command line without editing the file |
| `TEMPO_REDE_S` | seconds allowed for each connection attempt (`20`). Worst-case boot spends twice this, when the board has moved and the remembered network is tried first. It was `12` and that was not enough — see the bench note below |
| `numid` | device id sent in every message (1–4) |
| `tipoEntr` | sensor electrical type: `1` analog voltage divider (Garen, Wolpac), `2` digital/Zener (Foca). Ignored for Monetel |
| `modelo` | turnstile mechanics: `1` = 4 quarter-turns per passage (Foca/Garen/Wolpac), `2` = 2 half-turns (Ascom/Monetel). Only honored when `teste = 1`; otherwise auto-detected from hardware jumpers at boot |
| `nomeota` | ArduinoOTA hostname and the prefix of every Telegram message |
| `TG_TOKEN`/`TG_CHAT` | Telegram bot + group (same for all four), from `TELEGRAM_TOKEN`/`TELEGRAM_CHAT` in `segredos.h` |
| `ssid`/`password`, `ssidOTA`/`passwordOTA` | from `WIFI_LAB_*` / `WIFI_ESTACAO_*` / `WIFI_OTA_*` in `segredos.h`. `ssidOTA` is the network joined when the OTA jumper (GPIO0–GPIO4) is shorted at boot — the emergency path to reflash over Wi-Fi when USB is broken |

| `ADC_ACIONADO` / `ADC_REPOUSO` | sensor thresholds for `tipoEntr = 1`, as raw 12-bit ADC counts: a sensor reads **below** `ADC_ACIONADO` when triggered and **above** `ADC_REPOUSO` at rest, with a deliberate dead band between. Wolpac is `600`/`730` (validated counting on the bench 2026-09-22: its board drops the pin voltage to ~0.70 V at rest ≈ 869 counts and ~0.37 V triggered ≈ 459 — note that `150`/`200` was tried on the same board and did **not** work); the rest are `300`/`450`, never recalibrated. If both sensors read on the same side, the firmware sees the mechanically impossible "both triggered" combination, jumps to `cont = 2` and spins there forever — no counting, and `tarefasOciosas()` never runs, so OTA, summaries and queued Telegram messages all stop silently. That is the first thing to check when a board goes quiet |
| `DEBUG_ADC` | `1` turns `loop()` into calibration mode: the board counts nothing and prints all six sensor pins (analog and digital) to serial twice a second. Measure the real numbers here instead of converting from volts — the ADC is non-linear and the on-board divider varies by variant. The block is a `while (true)` at the top of `loop()`, so **everything** else stops too: no counting, no sending, no GitHub OTA, and no IDE-over-Wi-Fi upload (`ArduinoOTA.handle()` lives in the idle loop, which is never reached) — USB is the only way back. Because it is a bench tweak that lives in the source uncommitted, `gerar_release.sh` reads the four sketches and **refuses to build** while any of them has `DEBUG_ADC` other than `0`; `conferir_binario` cannot see it in the `.bin`, so the check has to be on the source |

**When you fix a bug in a shared tab, apply it to all four folders** — there is no shared source; `diff` the folders to check drift. All four `loop.ino` are now byte-identical (the Wolpac thresholds moved into the constants above), so a `diff` there should come back empty. The reference copy is `Garen_ID03`, and the port to the other three was done by generating each main sketch from Garen's with only the variant lines substituted (2026-09-21; pre-port originals in `backup/`).

Never hardcode a threshold back into `loop.ino`: the comparison appears ~70 times, and editing one of them (which is what happened on the bench on 2026-09-22) leaves the other 69 on the old value and produces exactly the stuck-board symptom above.

### Features (all four variants, since 2026-09-21)

- **OTA** (`ota_update.ino`): downloads `<OTA_URL_BASE><OTA_ASSET>` (channel per `AMBIENTE_LAB`, see Releases) only when `versao.txt` from the same release is numerically **greater** than `FW_VERSION` — never a downgrade. Checked at boot (`OTA_CHECAR_NO_BOOT`) and on the schedule. `onProgress` feeds the WDT during the download. Telegram gets "baixando"/"FALHA" once per cloud version; success is announced by the next boot message. **Download attempts are capped at `OTA_MAX_TENTATIVAS` (3) per cloud version since version 14** — see the Garen incident for why.
- **Rollback**: `rollback_hook.cpp` (`verifyRollbackLater()` → true) + `verificarEstadoOTA()` early in `setup()` confirm the image only after NVS/WDT init. A version that crashes before that rolls back and is stored in NVS `ota/rejeitada`; a downloaded binary whose own `FW_VERSION` differs from what `versao.txt` promised is also blocked (otherwise the board would re-flash it forever).
- **Clock**: no NTP. `configurarFuso()` only sets TZ (`<-03>3`); the web service is the single time source (verified: it sends UTC epoch). `tempo()` returns `bool`; `sincronizarRelogio()` keeps counting offline if the RTC is already valid (it survives `ESP.restart()`/WDT), and only reboots when the clock was never set. **Every epoch is validated (> jan/2023) before `rtc.setTime()`, in `servidor()` as well as `tempo()`** — `servidor()` re-syncs the clock from the response to *every* count, and used to do it unvalidated, so one truncated or unexpected response would set the clock to 1970 and every later passage would carry the literal `E2` timestamp. Combined with the server's dedup rule (below) that silently collapses a whole day of counts into one row.
- **Telegram** (`telegram.ino`): `enviarTelegram()` (JSON POST, 10 s handshake timeout) sends now; `agendarTelegram()` queues for the idle loop (`enviarTelegramPendente()`, ≤1 try/min). Boot message: reset reason / `ATUALIZADO x -> y` / `ROLLBACK`, environment, `JUMPER OTA` flag, Wi-Fi, clock, SD state. `resumo.ino`: periodic summary (deltas vs. NVS `resumo/E,S,W`, pending lines in `LOGREG.csv`), queued if Wi-Fi is down. `falhou()` queues one SD-failure alert per boot.
- **`tarefasOciosas()`** (`resumo.ino`) is the single place blocking work runs — IDE OTA, GitHub OTA, summary, queued Telegram — called from both `cont == 0` idle loops only, so it runs **only while the turnstile is at rest**. Everything the board does besides counting therefore depends on reaching `cont == 0`.
- **Stuck-state watchdog** (`estadoPreso()` in `resumo.ino`, called at the top of every `while (cont == N)` with `N != 0`): a real passage takes 1–2 s, so if `cont` stays non-zero for `TEMPO_MAX_ESTADO_MS` (30 s) the board logs it, queues one Telegram alert, resets `cont`/`direcao` and returns from `loop()` to get back to the idle loop. Without it a jammed sensor, a turnstile left half-turned, or a bench board with the sensor inputs floating parks the firmware in that state's `while` forever — it keeps feeding the watchdog, so it never reboots, and OTA, summaries and Telegram all stop silently. This is how the bench board behaved on 2026-09-23 when it was plugged into a PC with the sensors disconnected.
- **`setupOTA()`** gives up after 15 s and prints which of three modes it is in (jumper ⇒ "modo físico OTA" on `ssidOTA`; else lab or station per `AMBIENTE_LAB`); the IDE network port is enabled in every mode.
- **`modelo 2` counts both directions**: the rest state's two transitions used to be gated on `direcao` (`0 1` required `direcao == 1 || 0`, `1 0` required `direcao == 2 || 0`) while `direcao` was never reset to 0 in that branch — so after the first passage the board counted only that direction and dropped the opposite one silently. Confirmed on the bench 2026-09-24 (entry counted, exit not) and removed; `direcao` is now only what `modelo 1` uses to disambiguate a glitch.
- **No error floods** (`ultimoEventoFoiErro`): the "both sensors triggered" combination is only counted and sent **once** until a valid passage happens, and the first one queues a Telegram alert. In `modelo 2` the error block is an `if`, not a `while`, so a stuck or disconnected sensor used to produce one `sentido 3` record *per loop iteration* — the bench Monetel emitted one or two per second into the test database on 2026-09-25. `modelo 1` has the same shape.
- **Backlog does not livelock** (`ultimoHttpCode`): `nextposition` only advances on `resposta == "000"`, so a line the server rejects used to be retried forever and the whole backlog stopped draining. A `4xx` now discards that line and moves on ("the server refused this record"), while negative codes (no connection, timeout) still keep it for a later attempt. `servidor()` logs the failing code via `descricaoErroHttp()` **and the URL it tried** — without that there is no way to tell what the server refused.
- **Backlog validation** (`backlogValido()` in `atrasado.ino`): a line from `LOGREG.csv` is only sent if `sentido` is 1/2/3 and the timestamp is numeric (or the literal `E2`), and `nextposition` is reset to 0 when it points past the end of the file. `POSITION.txt` can fall out of sync with `LOGREG.csv` (a reboot between deleting the file and resetting the offset), which makes `seek()` land mid-line so a fragment of a timestamp becomes the `sentido` — that is how a `sentido 62` reached the test database on 2026-09-24. Invalid lines are now logged and skipped instead of being sent.
- **Precision fixes**: `leId()` opens `LOGREG.csv` read-only and sets `flagSD` from `size()` (before: `FILE_WRITE` put the cursor at EOF ⇒ `available()==0` ⇒ `flagSD` always 1 ⇒ `atrasado()` ran on every passage); `loop()` no longer mounts the SD to delete an empty `LOGREG.csv`; both LOGREG line formats unified to `sentido;timestamp;`; the `;` missing in the delayed packet in `sendMessage.ino` restored (it used to be compensated by accident via the trailing `;` kept in `timestamp2`); `servidor()`'s critical-timeout branch now sets `flagSERV=1`/`resposta="222"` instead of inheriting the previous passage's state.

## Sketch structure (multi-tab)

Arduino concatenates every `.ino` in a folder into one translation unit (main sketch first, then tabs alphabetically), so each tab is a single global function with no headers and no prototypes. Globals used across tabs must live in `<Variant>.ino` — a global defined in a tab is invisible to tabs that sort before it. `ota_update.ino`, `telegram.ino` have their own `#include`s. Anything needing C linkage (a core weak hook) must go in a `.cpp` file, because the Arduino preprocessor emits C++ prototypes for every function in a `.ino` (see `rollback_hook.cpp`).

| tab | role |
|---|---|
| `<Variant>.ino` | globals, pin map, `setup()`, `setupOTA()` |
| `loop.ino` | one turnstile event per iteration: reconcile previous send, then block in a sensor-polling state machine |
| `sendMessage.ino` → `servidor.ino` | build the packet and HTTP GET it to `/enviarContagem/<numid>;<timestampMs>;<atraso>;<sdErr>;<sentido>;`, then re-sync the clock from the response |
| `tempo.ino` | `tempo()`: one bounded GET `/recuperaTimestamp`, sets the `ESP32Time` RTC; `sincronizarRelogio()`: boot policy around it |
| `card.ino`, `cardID.ino`, `leId.ino` | SD file I/O (see below) |
| `atrasado.ino` | replay the SD backlog and NVS-lost count to the server |
| `ota_update.ino` | TZ, GitHub version check + `HTTPUpdate`, rollback bookkeeping (`verificarEstadoOTA`) |
| `telegram.ino` | `enviarTelegram`/`agendarTelegram`, reset reason, boot message |
| `resumo.ino` | periodic summary, `contarPendentesSD()`, `tarefasOciosas()` |
| `ambiente.ino` | `conectarAmbiente()`: detects the environment at boot by trying to connect, and `aplicarAmbiente()`, which points `ssid`/`password`/`host`/`url_*` at it. Also the cross-environment backlog guard |
| `rollback_hook.cpp` | `verifyRollbackLater()` → true (C linkage) |
| `checkNVS.ino`, `fail.ino`, `falhou.ino` | NVS integrity check at boot; SD-failure flag reset / alert |

## Runtime flow

**`setup()`**: brown-out threshold → `checkNVS()` → OTA jumper probe (drive GPIO0 high, read GPIO4; shorted ⇒ `otaativ = 1` ⇒ join `ssidOTA` instead of `ssid`) → 120 s task watchdog with panic → `verificarEstadoOTA()` (confirm image / detect rollback; deliberately before Wi-Fi) → model jumper probe (drive `PIN_MOD0`, read `PIN_MOD1`/`PIN_MOD2`; neither/both ⇒ restart) → `configurarFuso()` → `setupOTA()` (`conectarAmbiente()`, up to 2 × `TEMPO_REDE_S`, + `ArduinoOTA.begin()`) → `sincronizarRelogio()` → boot OTA check → `cardID()`/`leId()` to create/read the SD counter files → `enviarMensagemBoot()`.

**`loop()`** — each pass handles exactly one passage:
1. *Reconcile* the previous send using `flagSERV`/`resposta`: on failure append `sentido;timestamp;` to `LOGREG.csv` (or, if the SD also failed, bump NVS `counterF`); on success call `atrasado()` if there is a backlog.
2. *Sense* in a `goto`-label state machine (`cont0a…cont4b` for `modelo 1`, `cont0c…cont3c` for `modelo 2`). `cont` counts quarter-turns; `+4`/`+2` = entry (`sentido 1`), `-4`/`-2` = exit (`sentido 2`), an impossible sensor combination pushes `cont` out of range = error (`sentido 3`). `direcao` remembers the last direction to disambiguate glitches. `ArduinoOTA.handle()` and `verificarHorarioOTA()` only run in the idle `cont == 0` loop.
3. On a completed passage: `timestamp = timestampMs()` (or the literal `"E2"` if the clock is still pre-2023, i.e. never synced), set `flagE/flagS/flagW`, `sendMessage()`.

Sensing is fully blocking — nothing is counted while `servidor()` is talking to the server. That is why `servidor()` uses deliberately tight timeouts (350 ms request / 700 ms connect / 800 ms total): a slow server must not eat the next passage. Keep any change here fast.

**Watchdog**: every function that can block calls `esp_task_wdt_reset()` liberally. Any new loop or long network call must do the same or the board panics and reboots after 120 s.

## Persistence conventions

Every SD access is wrapped in `digitalWrite(SD_CS, Select)` → `SD.begin(...)` → work → `SD.end()` → `DeSelect`; the SD bus is never left mounted. SD files, all in the root:

- `LOGREG.csv` — backlog of unsent passages (`sentido;timestamp;` per line)
- `ENTRADAS.txt`, `SAIDAS.txt`, `ERROS.txt` — running totals (single integer)
- `POSITION.txt` — byte offset in `LOGREG.csv` up to which `atrasado()` has already replayed

Those five are the **only** files the firmware opens — `grep "SD.open(\|SD.remove("` over the tabs returns nothing else. Two stale things a card and the comments will show you anyway:

- **`ID.txt` on a card is dead data.** Cards in service carry one (content = the zero-padded `numid`, e.g. `003` on the Garen), left by pre-rework firmware. Nothing in the current code reads, writes or deletes it. Safe to leave or remove; it is not part of any protocol.
- The header comments in `cardID.ino` and `leId.ino`, and the `debug` legend in `<Variant>.ino`, still describe a **`MSGID.txt`** holding "the id of the last message sent". That file is never created or read either — the comments outlived the feature. Do not implement against them.

`cardID()`/`leId()` are driven by a flag protocol rather than arguments: `flagE/flagS/flagW/flagP` (`0` skip, `1` increment/write, `2` create-and-read) and `escreve` (`0` write, `1` delete). A counter update is always "delete file, then rewrite" (`escreve = 1; cardID(); escreve = 0; cardID();`). `flagF = 1` means the last SD access failed; `fail()` resets all flags to `2` so the next successful mount recreates everything.

NVS namespace `"my-app"`, key `counterF`: number of passages lost because both the server and the SD failed. It is sent later by `atrasado()` with `debug = 0x06` and cleared on acknowledgment. Namespaces `"ota"` (`tentada`, `rejeitada`, `versao`, `retrocessos`: how many times this board has come up on a version *older* than the one it stored; `tentvers`/`tentativas`: the cloud version whose downloads are being counted and how many have been started for it), `"resumo"` (`E`, `S`, `W`) and `"ambiente"` (`lab`: the last confirmed environment; `backlog`: the environment the pending counts were written in — card **or** flash — or `2` for mixed) are accessed through the second `Preferences` instance `prefsCBA` because `preferences` stays open on `"my-app"` for the whole loop iteration. `pendentesNaFlash()` in `ambiente.ino` is the exception that opens `"my-app"` on `prefsCBA`: it runs in `setup()`, where `preferences` is not open yet.

`resposta` is a three-digit string state: `"999"` idle, `"000"` server acknowledged, `"111"` awaiting ack / write to SD, `"222"` send failed. `debug` byte values are documented inline in `<Variant>.ino`.

## Timestamp format: milliseconds (since 2026-10-19)

The passage timestamp sent to the web service is **13 digits, in milliseconds** (`timestampMs()` in `tempo.ino`), changed from the 10-digit seconds the project started with; first shipped in release `7`, validated on the bench with the Garen on 2026-10-19. The board's own clock still runs in **seconds** — only the outgoing count carries milliseconds.

- `timestampMs()` reads the clock **once** with `gettimeofday()`. `rtc.getEpoch()` and `rtc.getMillis()` are two separate reads, and a second ticking over between them would produce a value one second off.
- The sub-second digits are real elapsed time, but the clock is only ever *set* with second precision (the server's sync endpoint sends seconds), so the fraction is not aligned with the server's. It is precise for ordering and uniqueness, not for absolute sub-second accuracy.
- That uniqueness is the point: the server deduplicates on `numid` + timestamp, so with seconds two passages in the same second on one board collapsed into a single row. Milliseconds remove that.
- **`LOGREG.csv` can hold both formats.** `backlogValido()` accepts exactly 10 digits (legacy seconds) or 13 (milliseconds) — any other length is corruption — and `atrasado()` multiplies a 10-digit value by 1000 before sending, so the server never receives the two formats mixed in one column.
- `epochSegundos()` (`tempo.ino`) parses the server's clock reply with `strtoll` and accepts **either** 10 or 13 digits, normalising to seconds. Both `tempo()` and `servidor()` use it. The previous `toInt()` is 32-bit and would overflow on a 13-digit value, so if the web service ever switches its sync reply to milliseconds too, the board sets its clock from garbage instead.

## Server contract (confirmed with the web service owner, 2026-09-24)

- **The server deduplicates on `numid` + measurement timestamp.** Replaying the SD backlog is therefore safe and cannot double-count in the BI — that was the main open precision risk. The flip side: two records that share a board id and a timestamp collapse into one, so anything that makes the clock repeat or freeze destroys counts silently. That is why every `rtc.setTime()` is now guarded, and why an `E2` timestamp is far worse than it looks.
- **`sentido` is always 1, 2 or 3.** Anything else in the database came from the firmware, not from the turnstile; `backlogValido()` now blocks that at the source.

## Hardware gotchas

- `pinoLedInterno` is **GPIO1 = UART TX**. The code deliberately does `Serial.end()` before driving the LED and `Serial.begin()` after (first three passages only, via `ledaux`). Don't "clean up" those calls; serial output is lost by design during the blink.
- Pin map (`PIN_BTN1/2` = 15/2 for Monetel, `PIN_BTN3/4` = 16/17 digital, `PIN_BTN5/6` = 36/39 analog, `PIN_MOD0/1/2` = 25/32/33, SD on VSPI 18/19/23/5) is in `<Variant>.ino`; the SD `#define` comments record which GPIOs were found not to work.
- **The carrier board has three separate sensor input paths, and `tipoEntr` picks which one the firmware watches.** Per the project's wiring doc (*"configurações e bibliotecas V15"*, kept by the colleague who built the hardware — note it also contains BI/database credentials, so do not attach it to this public repo):

  | board input | pins | doc says it is used by | `tipoEntr` |
  |---|---|---|---|
  | Direto 1 / 2 | GPIO15, GPIO2 | Monetel/Ascom | n/a (`modelo 2` reads these) |
  | Zener 1 / 2 | GPIO16, GPIO17 | Wolpac, Garen, and *some* Focas | `2` |
  | Analógico 1 / 2 | GPIO36, GPIO39 | Foca, possibly others | `1` |

  The doc and the sketch comments disagree (the sketch calls `1` "divisor de tensão (garen, wolpac)" and `2` "Zener (Foca)"). **Wiring varies per unit — do not infer it, measure it**: flash with `DEBUG_ADC = 1` and turn the turnstile; whichever pair moves is the one that unit is wired to. A board watching the wrong pair reads both sensors as permanently triggered and stops counting. Measured and validated end to end (turn → count → web service → test database) on the bench:

  | variant | pair that moves | setting | pattern | validated |
  |---|---|---|---|---|
  | Wolpac | analog GPIO36/39 | `tipoEntr 1`, 600/730 | `11 → 01 → 00 → 10 → 11` (4 steps) | 2026-09-23 |
  | Foca | Zener GPIO16/17 | `tipoEntr 2` | same 4-step quadrature | 2026-09-24 |
  | Monetel | Direto GPIO15/2 | `modelo 2` | rest `11`, one pin pulses and returns (2 steps): GPIO15 = entry, GPIO2 = exit | 2026-09-25 |
  | Garen | analog GPIO36/39 | `tipoEntr 1`, 300/450 | same 4-step quadrature | 2026-09-25 |

  All four are validated on the current build, covering all three input paths on the carrier board.
- The pin silkscreened `GND` next to `V5` (G11) is **not a real ground** — it is CMD/CSC on the official pinout. Measuring 5 V between `V5` and it will not work.
- GitHub OTA uses `client.setInsecure()` (no cert validation) and follows redirects, because GitHub release assets 302 to a CDN.
