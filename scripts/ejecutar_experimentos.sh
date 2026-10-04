#!/usr/bin/env bash
set -euo pipefail

repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_dir"

if command -v make >/dev/null 2>&1; then
  make
else
  mkdir -p build
  for source in src/*.c; do
    program="$(basename "${source%.c}")"
    mpicc -O2 -Wall -Wextra -Wpedantic -std=c11 "$source" -o "build/$program"
  done
fi
mkdir -p resultados

run_mpi() {
  mpirun --oversubscribe "$@"
}

{
  echo 'ihan@VICTUS:~/Labo3_paralela$ mpirun -np 2 ./build/ping_pong 5 16 --detalle'
  run_mpi -np 2 ./build/ping_pong 5 16 --detalle
} > resultados/ping_pong_funcional.log

{
  echo 'ihan@VICTUS:~/Labo3_paralela$ mpirun -np 5 ./build/token_ring blocking 2 --detalle'
  run_mpi -np 5 ./build/token_ring blocking 2 --detalle
  echo
  echo 'ihan@VICTUS:~/Labo3_paralela$ mpirun -np 5 ./build/token_ring sendrecv 2 --detalle'
  run_mpi -np 5 ./build/token_ring sendrecv 2 --detalle
} > resultados/token_ring_funcional.log

{
  echo 'ihan@VICTUS:~/Labo3_paralela$ mpirun -np 2 ./build/recepcion_anticipada 4096 50000'
  run_mpi -np 2 ./build/recepcion_anticipada 4096 50000
} > resultados/recepcion_anticipada_funcional.log

{
  echo 'ihan@VICTUS:~/Labo3_paralela$ mpirun -np 2 ./build/pipeline_chunks 4096 1024 --detalle'
  run_mpi -np 2 ./build/pipeline_chunks 4096 1024 --detalle
} > resultados/pipeline_chunks_funcional.log

echo 'repeticion,enteros,bytes,tiempo_total_s,tiempo_promedio_ms' > resultados/ping_pong_raw.csv
for enteros in 1 16 256 4096 65536; do
  for repeticion in 1 2 3 4 5 6 7; do
    salida="$(run_mpi -np 2 ./build/ping_pong 500 "$enteros")"
    bytes="$(awk -F'bytes=' '{print $2}' <<<"$salida" | awk '{print $1}')"
    total="$(awk -F'tiempo_total_s=' '{print $2}' <<<"$salida" | awk '{print $1}')"
    promedio="$(awk -F'tiempo_promedio_ms=' '{print $2}' <<<"$salida" | awk '{print $1}')"
    echo "$repeticion,$enteros,$bytes,$total,$promedio" >> resultados/ping_pong_raw.csv
  done
done

echo 'repeticion,enteros,bytes,tiempo_s,pruebas_mpi_test,operaciones' > resultados/recepcion_anticipada_raw.csv
for enteros in 1 1024 16384 262144 1048576; do
  for repeticion in 1 2 3 4 5 6 7; do
    salida="$(run_mpi -np 2 ./build/recepcion_anticipada "$enteros" 50000)"
    bytes="$(awk -F'bytes=' '{print $2}' <<<"$salida" | awk '{print $1}')"
    tiempo="$(awk -F'tiempo_s=' '{print $2}' <<<"$salida" | awk '{print $1}')"
    pruebas="$(awk -F'pruebas_mpi_test=' '{print $2}' <<<"$salida" | awk '{print $1}')"
    operaciones="$(awk -F'operaciones=' '{print $2}' <<<"$salida" | awk '{print $1}')"
    echo "$repeticion,$enteros,$bytes,$tiempo,$pruebas,$operaciones" >> resultados/recepcion_anticipada_raw.csv
  done
done

echo 'repeticion,total_enteros,chunk_enteros,chunk_bytes,chunks,tiempo_total_s' > resultados/pipeline_chunks_raw.csv
total_enteros=4194304
for chunk in 256 1024 4096 16384 65536 262144; do
  for repeticion in 1 2 3 4 5 6 7; do
    salida="$(run_mpi -np 2 ./build/pipeline_chunks "$total_enteros" "$chunk")"
    chunks="$(awk -F'chunks=' '{print $2}' <<<"$salida" | awk '{print $1}')"
    tiempo="$(awk -F'tiempo_total_s=' '{print $2}' <<<"$salida" | awk '{print $1}')"
    echo "$repeticion,$total_enteros,$chunk,$((chunk * 4)),$chunks,$tiempo" >> resultados/pipeline_chunks_raw.csv
  done
done

printf 'Experimentos completados. Resultados guardados en %s/resultados\n' "$repo_dir"
