#!/usr/bin/env bash
# Gera os binários das 4 variantes e os reúne em release/ para publicar nos canais de OTA:
#   release/estacao/  -> 4 .bin + versao.txt  (assets da release "latest")
#   release/lab/      -> 4 .bin + versao.txt  (assets da release de tag "lab", pre-release)
# Uso (Git Bash, na raiz do clone):  ./gerar_release.sh [estacao|lab|ambos]   (padrão: ambos)
# Requer o arduino-cli embutido na IDE 2 e o segredos.h em cada pasta de sketch.
#
# Desde a versão 8 existe UM binário por variante, igual nos dois canais: a placa descobre no boot
# em qual ambiente está (ver conectarAmbiente() em ambiente.ino). O que separa bancada de estação
# passou a ser a VERSÃO publicada em cada canal, não o conteúdo do .bin — publicar só em "lab"
# continua sendo a forma de testar na bancada sem alcançar a estação.
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

# Confere no binário pronto que ele é mesmo o de detecção automática: tem de carregar as DUAS redes
# e os DOIS canais de OTA, porque é isso que permite à placa se adaptar ao lugar onde for ligada.
# Um binário sem isso veio de um fonte antigo (ou de um -DAMBIENTE_LAB mal colocado) e serve a um
# ambiente só — publicado no canal errado, a placa que o baixasse ficaria procurando a rede errada,
# sem rede para receber correção. Foi o que aconteceu nas versões 5, 6 e 7 do canal lab.
conferir_binario() {  # $1 = arquivo, $2 = variante
  local arq="$1" v="$2" s
  for s in POC_MANUTENCAO 10.66.24.196 "releases/latest/download/" "releases/download/lab/"; do
    if ! grep -qa -- "$s" "$arq"; then
      echo "ERRO: $v nao contem '$s' - nao e o binario de deteccao automatica, abortando."; exit 1
    fi
  done
}

# DEBUG_ADC = 1 transforma o loop() em modo calibracao: a placa NAO conta e NAO envia nada.
# E um ajuste de bancada que vive no fonte sem commit, logo e exatamente o tipo de coisa que
# escapa para uma release. Publicado, deixaria a catraca sem contar, e o conferir_binario nao
# tem como ver isso no .bin - por isso a checagem e no fonte, antes de compilar.
conferir_fonte() {  # $1 = variante
  local dbg
  dbg="$(grep -m1 '^#define DEBUG_ADC' "$RAIZ/CBAV24/$1/$1.ino" | awk '{print $3}')"
  if [ "$dbg" != "0" ]; then
    echo "ERRO: $1 esta com DEBUG_ADC = ${dbg:-ausente} (modo calibracao, nao conta)."
    echo "      Volte para 0 em CBAV24/$1/$1.ino antes de gerar release."
    exit 1
  fi
}

destinos() {  # pastas de saída conforme o canal pedido
  case "$1" in
    estacao) echo "estacao" ;;
    lab)     echo "lab" ;;
    ambos)   echo "estacao lab" ;;
    *) echo "uso: $0 [estacao|lab|ambos]" >&2; exit 1 ;;
  esac
}

SAIDAS="$(destinos "$CANAL")"
for d in $SAIDAS; do mkdir -p "$RAIZ/release/$d"; done

# Confere o fonte das quatro ANTES de compilar qualquer uma: falhar na terceira depois de dois
# builds longos nao ajuda ninguem.
for v in "${VARIANTES[@]}"; do conferir_fonte "$v"; done

versao=""
for v in "${VARIANTES[@]}"; do
  fv="$(versao_do_fonte "$v")"
  if [ -n "$versao" ] && [ "$fv" != "$versao" ]; then
    echo "ERRO: FW_VERSION difere entre as variantes ($versao vs $fv em $v)"; exit 1
  fi
  versao="$fv"
  echo "== $v (FW_VERSION $fv)"
  "$CLI" --config-file "$CFG" compile --fqbn "$FQBN" \
    --build-path "$RAIZ/build/cli/$v" \
    --output-dir "$RAIZ/build/cli/${v}_out" \
    "$RAIZ/CBAV24/$v" 2>&1 | grep -E "Sketch uses|error" || true
  bin="$RAIZ/build/cli/${v}_out/$v.ino.bin"
  conferir_binario "$bin" "$v"
  for d in $SAIDAS; do cp "$bin" "$RAIZ/release/$d/$v.ino.bin"; done
done

for d in $SAIDAS; do
  printf '%s\n' "$versao" > "$RAIZ/release/$d/versao.txt"
  echo "-> release/$d: $(ls "$RAIZ/release/$d" | tr '\n' ' ')"
done
