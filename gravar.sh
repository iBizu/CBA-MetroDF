#!/usr/bin/env bash
# Compila e grava uma variante na placa ligada por USB.
#
# Uso (Git Bash, na raiz do clone):
#   ./gravar.sh <Variante> <PORTA> [lab|estacao]
# Exemplos:
#   ./gravar.sh Monetel_ID01 COM5 lab       # bancada (padrão)
#   ./gravar.sh Wolpac-ID04  COM7 estacao   # configuração de produção
#   ./gravar.sh --portas                    # lista as placas conectadas
#
# Desde a versão 8 a placa descobre o ambiente sozinha no boot (ver ambiente.ino), então o terceiro
# argumento só semeia o primeiro boot com NVS vazia: "lab" semeia bancada, "estacao" semeia produção.
# Se a semente estiver errada, a detecção corrige no mesmo boot, gastando uma tentativa a mais.
# O valor é sempre aplicado na compilação, sem editar o arquivo.
set -euo pipefail

CLI="$HOME/AppData/Local/Programs/Arduino IDE/resources/app/lib/backend/resources/arduino-cli.exe"
CFG="$HOME/.arduinoIDE/arduino-cli.yaml"
FQBN="esp32:esp32:esp32"
RAIZ="$(cd "$(dirname "$0")" && pwd)"

if [ "${1:-}" = "--portas" ]; then
  "$CLI" --config-file "$CFG" board list
  exit 0
fi

VARIANTE="${1:-}"
PORTA="${2:-}"
AMBIENTE="${3:-lab}"

if [ -z "$VARIANTE" ] || [ -z "$PORTA" ]; then
  echo "uso: $0 <Variante> <PORTA> [lab|estacao]   |   $0 --portas"
  echo "variantes: Monetel_ID01  Foca_ID02  Garen_ID03  Wolpac-ID04"
  exit 1
fi
if [ ! -d "$RAIZ/CBAV24/$VARIANTE" ]; then
  echo "ERRO: não existe CBAV24/$VARIANTE"; exit 1
fi
if [ ! -f "$RAIZ/CBAV24/$VARIANTE/segredos.h" ]; then
  echo "ERRO: falta CBAV24/$VARIANTE/segredos.h (copie de segredos.h.exemplo e preencha)"; exit 1
fi

# Os dois casos passam o -D explicitamente, nunca "o padrão do fonte". Enquanto a bancada era o
# padrão, "lab" não passava nada; quando o fonte passou a compilar como estação (fa9fbb4), o mesmo
# comando começou a gravar um build de ESTAÇÃO em silêncio, que é justamente o engano que este
# script existe para evitar. O valor do #define no fonte não deve influenciar o que é gravado.
declare -a extra=()
case "$AMBIENTE" in
  lab)     extra=(--build-property "compiler.cpp.extra_flags=-DAMBIENTE_LAB=1") ;;
  estacao) extra=(--build-property "compiler.cpp.extra_flags=-DAMBIENTE_LAB=0") ;;
  *) echo "ERRO: ambiente deve ser 'lab' ou 'estacao'"; exit 1 ;;
esac

echo "== $VARIANTE -> $PORTA  (ambiente: $AMBIENTE, versão $(sed -n 's/^#define FW_VERSION "\([^"]*\)".*/\1/p' "$RAIZ/CBAV24/$VARIANTE/$VARIANTE.ino"))"
"$CLI" --config-file "$CFG" compile --fqbn "$FQBN" \
  "${extra[@]}" \
  --build-path "$RAIZ/build/cli/gravar_$VARIANTE" \
  --upload --port "$PORTA" \
  "$RAIZ/CBAV24/$VARIANTE"
echo "== gravado. Abra o monitor serial a 115200 para acompanhar o boot."
