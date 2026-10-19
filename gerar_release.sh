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

# Confere no binário pronto que ele saiu com a rede e o canal de OTA do ambiente certo.
# É a última barreira antes de publicar: um binário do ambiente errado só se revela quando a
# placa já está no lugar errado, sem rede para receber correção.
conferir_binario() {  # $1 = arquivo, $2 = canal, $3 = variante
  local arq="$1" canal="$2" v="$3" deve falta
  if [ "$canal" = "estacao" ]; then
    deve="POC_MANUTENCAO"; falta="10.66.24.196"
  else
    deve="10.66.24.196"; falta="POC_MANUTENCAO"
  fi
  if ! grep -qa "$deve" "$arq"; then
    echo "ERRO: $v ($canal) nao contem '$deve' - binario do ambiente errado, abortando."; exit 1
  fi
  if grep -qa "$falta" "$arq"; then
    echo "ERRO: $v ($canal) contem '$falta' - binario do ambiente errado, abortando."; exit 1
  fi
}

gerar() {  # $1 = estacao|lab
  local canal="$1" out="$RAIZ/release/$1"
  # O ambiente é SEMPRE imposto por -D, nos dois canais. Antes o canal lab usava o valor do
  # #define no fonte, e em 19/10/2026 o Wolpac foi publicado no canal lab com um binário de
  # ESTAÇÃO, porque aquela variante estava com AMBIENTE_LAB 0 commitado. Uma placa de bancada
  # que baixasse isso iria procurar a rede da estação e sumir. Com o -D explícito, o valor do
  # fonte não influencia mais a release.
  local amb
  case "$canal" in
    estacao) amb=0 ;;
    lab)     amb=1 ;;
    *) echo "ERRO: canal desconhecido '$canal'"; exit 1 ;;
  esac
  mkdir -p "$out"
  local versao=""
  for v in "${VARIANTES[@]}"; do
    local fv; fv="$(versao_do_fonte "$v")"
    if [ -n "$versao" ] && [ "$fv" != "$versao" ]; then
      echo "ERRO: FW_VERSION difere entre as variantes ($versao vs $fv em $v)"; exit 1
    fi
    versao="$fv"
    echo "== $canal / $v (FW_VERSION $fv, AMBIENTE_LAB=$amb)"
    "$CLI" --config-file "$CFG" compile --fqbn "$FQBN" \
      --build-property "compiler.cpp.extra_flags=-DAMBIENTE_LAB=$amb" \
      --build-path "$RAIZ/build/cli/${canal}_$v" \
      --output-dir "$RAIZ/build/cli/${canal}_${v}_out" \
      "$RAIZ/CBAV24/$v" 2>&1 | grep -E "Sketch uses|error" || true
    cp "$RAIZ/build/cli/${canal}_${v}_out/$v.ino.bin" "$out/$v.ino.bin"
    conferir_binario "$out/$v.ino.bin" "$canal" "$v"
  done
  printf '%s\n' "$versao" > "$out/versao.txt"
  echo "-> $out: $(ls "$out" | tr '\n' ' ')"
}

case "$CANAL" in
  estacao|lab) gerar "$CANAL" ;;
  ambos) gerar estacao; gerar lab ;;
  *) echo "uso: $0 [estacao|lab|ambos]"; exit 1 ;;
esac
