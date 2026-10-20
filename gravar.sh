#!/usr/bin/env bash
# Compila e grava uma variante na placa ligada por USB.
#
# Uso (Git Bash, na raiz do clone):
#   ./gravar.sh <Variante> <PORTA> [lab|estacao] [--limpar]
# Exemplos:
#   ./gravar.sh Monetel_ID01 COM5 lab                # bancada (padrão)
#   ./gravar.sh Wolpac-ID04  COM7 estacao            # configuração de produção
#   ./gravar.sh Garen_ID03   COM7 estacao --limpar   # idem, apagando o backlog do cartão no boot
#   ./gravar.sh --portas                             # lista as placas conectadas
#
# --limpar APAGA contagens: zera LOGREG.csv e POSITION.txt no primeiro boot, para a placa sair para
# o campo sem as linhas de bancada. Só use quando as pendências do cartão forem descartáveis; a
# placa informa no Telegram quantas descartou. Nunca entra numa release (o gerar_release.sh aborta).
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
case "$AMBIENTE" in
  lab)     amb=1 ;;
  estacao) amb=0 ;;
  *) echo "ERRO: ambiente deve ser 'lab' ou 'estacao'"; exit 1 ;;
esac
flags="-DAMBIENTE_LAB=$amb"

# --limpar: esta gravação apaga LOGREG.csv e POSITION.txt do cartão no primeiro boot, para a placa
# ir a campo sem as linhas de bancada. O valor é o INSTANTE da gravação, não um "1": a placa só
# limpa se ele for diferente do que está na NVS, então um reboot não apaga de novo e uma gravação
# nova com --limpar apaga de novo. Ver limparBacklogSolicitado() em ambiente.ino.
if [ "${4:-}" = "--limpar" ]; then
  flags="$flags -DLIMPAR_BACKLOG=$(date +%s)"
  echo "!! --limpar: esta gravacao APAGA LOGREG.csv e POSITION.txt do cartao no primeiro boot."
  echo "!! As contagens pendentes nao enviadas serao descartadas (a placa avisa quantas no Telegram)."
elif [ -n "${4:-}" ]; then
  echo "ERRO: quarto argumento desconhecido '$4' (esperado --limpar ou nada)"; exit 1
fi

declare -a extra=(--build-property "compiler.cpp.extra_flags=$flags")

echo "== $VARIANTE -> $PORTA  (ambiente: $AMBIENTE, versão $(sed -n 's/^#define FW_VERSION "\([^"]*\)".*/\1/p' "$RAIZ/CBAV24/$VARIANTE/$VARIANTE.ino"))"
"$CLI" --config-file "$CFG" compile --fqbn "$FQBN" \
  "${extra[@]}" \
  --build-path "$RAIZ/build/cli/gravar_$VARIANTE" \
  --upload --port "$PORTA" \
  "$RAIZ/CBAV24/$VARIANTE"
echo "== gravado. Abra o monitor serial a 115200 para acompanhar o boot."
