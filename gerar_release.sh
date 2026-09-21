#!/usr/bin/env bash
# Gera os binários das 4 variantes para os dois canais de OTA e os reúne em release/:
#   release/estacao/  -> 4 .bin compilados com AMBIENTE_LAB=0 + versao.txt  (assets da release "latest")
#   release/lab/      -> 4 .bin compilados com AMBIENTE_LAB=1 + versao.txt  (assets da release de tag "lab", pre-release)
# Uso (Git Bash, na raiz do clone):  ./gerar_release.sh [estacao|lab|ambos]   (padrão: ambos)
# Requer o arduino-cli embutido na IDE 2 e o segredos.h em cada pasta de sketch.
set -euo pipefail

CLI="$HOME/AppData/Local/Programs/Arduino IDE/resources/app/lib/backend/resources/arduino-cli.exe"
CFG="$HOME/.arduinoIDE/arduino-cli.yaml"
FQBN="esp32:esp32:esp32"
RAIZ="$(cd "$(dirname "$0")" && pwd)"
VARIANTES=(Monetel_ID01 Foca_ID02 Garen_ID03 Wolpac-ID04)
CANAL="${1:-ambos}"

versao_do_fonte() {  # lê FW_VERSION do sketch principal
  sed -n 's/^#define FW_VERSION "\([^"]*\)".*/\1/p' "$RAIZ/CBAV24/$1/$1.ino"
}

gerar() {  # $1 = estacao|lab
  local canal="$1" out="$RAIZ/release/$1"
  local -a extra=()
  # lab usa o padrão do fonte (AMBIENTE_LAB 1); estação sobrepõe com -D sem editar o arquivo
  [ "$canal" = "estacao" ] && extra=(--build-property "compiler.cpp.extra_flags=-DAMBIENTE_LAB=0")
  mkdir -p "$out"
  local versao=""
  for v in "${VARIANTES[@]}"; do
    local fv; fv="$(versao_do_fonte "$v")"
    if [ -n "$versao" ] && [ "$fv" != "$versao" ]; then
      echo "ERRO: FW_VERSION difere entre as variantes ($versao vs $fv em $v)"; exit 1
    fi
    versao="$fv"
    echo "== $canal / $v (FW_VERSION $fv)"
    "$CLI" --config-file "$CFG" compile --fqbn "$FQBN" \
      "${extra[@]}" \
      --build-path "$RAIZ/build/cli/${canal}_$v" \
      --output-dir "$RAIZ/build/cli/${canal}_${v}_out" \
      "$RAIZ/CBAV24/$v" 2>&1 | grep -E "Sketch uses|error" || true
    cp "$RAIZ/build/cli/${canal}_${v}_out/$v.ino.bin" "$out/$v.ino.bin"
  done
  printf '%s\n' "$versao" > "$out/versao.txt"
  echo "-> $out: $(ls "$out" | tr '\n' ' ')"
}

case "$CANAL" in
  estacao|lab) gerar "$CANAL" ;;
  ambos) gerar estacao; gerar lab ;;
  *) echo "uso: $0 [estacao|lab|ambos]"; exit 1 ;;
esac
