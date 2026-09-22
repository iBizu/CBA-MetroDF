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
# "lab" grava a configuração de bancada (WIFI-ARHD, servidor de teste, canal de OTA "lab").
# "estacao" grava a de produção (POC_MANUTENCAO, servidor de produção, canal "latest").
# O fonte no git fica sempre no padrão de bancada; "estacao" é aplicado só na compilação.
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

declare -a extra=()
case "$AMBIENTE" in
  lab)     ;;  # padrão do fonte
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
