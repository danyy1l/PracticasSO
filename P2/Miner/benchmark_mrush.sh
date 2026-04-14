#!/usr/bin/env bash
set -u
export LC_ALL=C

# =========================
# Configuración
# =========================
SECS="${SECS:-10}"                 # duración de cada minero
REPS="${REPS:-3}"                  # repeticiones por configuración
OUTDIR="${OUTDIR:-bench_results}"  # carpeta de salida

MINERS_LIST=(2 3 4)
THREADS_LIST=(1 2 4 8)

EXEC="./mrush"

# =========================
# Preparación
# =========================
if [[ ! -f "Makefile" ]]; then
  echo "Error: este script debe ejecutarse dentro de la carpeta del proyecto Miner."
  exit 1
fi

if [[ ! -x "$EXEC" ]]; then
  echo "No se encontró $EXEC. Intentando compilar con make..."
  make || { echo "Error: no se pudo compilar."; exit 1; }
fi

mkdir -p "$OUTDIR/raw_logs"
RESULTS="$OUTDIR/results.csv"

echo "run_id,num_miners,num_threads,secs,elapsed_s,accepted,rejected,total_blocks,total_wallets,blocks_per_sec" > "$RESULTS"

cleanup_files() {
  rm -f ./*.log
  rm -f PIDs.pid Target.tgt Voting.vot
}

run_one_test() {
  local miners="$1"
  local threads="$2"
  local rep="$3"
  local run_id="m${miners}_t${threads}_r${rep}"

  echo "=================================================="
  echo "Ejecutando: $run_id"
  echo "=================================================="

  cleanup_files

  local start end elapsed
  start=$(date +%s.%N)

  pids=()
  for ((i=1; i<=miners; i++)); do
    "$EXEC" "$SECS" "$threads" \
      > "$OUTDIR/raw_logs/${run_id}_miner${i}.out" \
      2> "$OUTDIR/raw_logs/${run_id}_miner${i}.err" &
    pids+=($!)
    sleep 0.2
  done

  for pid in "${pids[@]}"; do
    wait "$pid"
  done

  end=$(date +%s.%N)
  elapsed=$(awk "BEGIN {print $end - $start}")

  mkdir -p "$OUTDIR/raw_logs/$run_id"
  cp -f ./*.log "$OUTDIR/raw_logs/$run_id/" 2>/dev/null || true

  local accepted rejected total_blocks total_wallets bps
  accepted=$(grep -h -c "validated" ./*.log 2>/dev/null | awk '{s+=$1} END{print s+0}')
  rejected=$(grep -h -c "rejected" ./*.log 2>/dev/null | awk '{s+=$1} END{print s+0}')
  total_blocks=$((accepted + rejected))

  total_wallets=$(grep -h "^Wallets :" ./*.log 2>/dev/null | awk -F: '{sum += $3} END {print sum+0}')

  if awk "BEGIN {exit !($elapsed > 0)}"; then
    bps=$(awk "BEGIN {print $total_blocks / $elapsed}")
  else
    bps="0"
  fi

  echo "$run_id,$miners,$threads,$SECS,$elapsed,$accepted,$rejected,$total_blocks,$total_wallets,$bps" >> "$RESULTS"
}

# =========================
# Ejecución de benchmarks
# =========================
for miners in "${MINERS_LIST[@]}"; do
  for threads in "${THREADS_LIST[@]}"; do
    for ((rep=1; rep<=REPS; rep++)); do
      run_one_test "$miners" "$threads" "$rep"
    done
  done
done

echo
echo "Benchmarks terminados."
echo "Resultados guardados en: $RESULTS"
echo "Logs detallados en: $OUTDIR/raw_logs"