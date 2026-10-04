# Laboratorio 3 de Computación Paralela y Distribuida

Soluciones en C con Open MPI para comunicación punto a punto bloqueante y no bloqueante.

## Contenido

- `src/ping_pong.c`: intercambio de mensajes entre dos procesos y medición con `MPI_Wtime`.
- `src/token_ring.c`: anillo con `MPI_Send`/`MPI_Recv` y una variante con `MPI_Sendrecv`.
- `src/recepcion_anticipada.c`: recepción no bloqueante con `MPI_Irecv` y consulta mediante `MPI_Test`.
- `src/pipeline_chunks.c`: productor-consumidor por chunks con dos buffers y mensaje de parada enviado con `MPI_Isend`.
- `scripts/ejecutar_experimentos.sh`: pruebas funcionales y mediciones repetidas.
- `scripts/graficar_resultados.py`: tablas resumen y gráficas del informe.
- `resultados/`: salidas, mediciones y gráficas obtenidas en la ejecución.
- `informe/Informe_Laboratorio_3.pdf`: informe final.

## Requisitos

En Ubuntu o WSL:

```bash
sudo apt update
sudo apt install openmpi-bin libopenmpi-dev make python3-matplotlib
```

## Compilación

```bash
make
```

Si `make` no está instalado, también se puede compilar cada fuente directamente:

```bash
mkdir -p build
for source in src/*.c; do
  program="$(basename "${source%.c}")"
  mpicc -O2 -Wall -Wextra -Wpedantic -std=c11 "$source" -o "build/$program"
done
```

Los ejecutables se generan dentro de `build/` y no se incluyen en el repositorio.

## Ejecuciones funcionales

```bash
mpirun -np 2 ./build/ping_pong 5 16 --detalle
mpirun --oversubscribe -np 5 ./build/token_ring blocking 2 --detalle
mpirun --oversubscribe -np 5 ./build/token_ring sendrecv 2 --detalle
mpirun -np 2 ./build/recepcion_anticipada 4096 50000
mpirun -np 2 ./build/pipeline_chunks 4096 1024 --detalle
```

## Experimentos y gráficas

```bash
bash scripts/ejecutar_experimentos.sh
python3 scripts/graficar_resultados.py
```

Cada programa valida la cantidad de procesos y sus argumentos antes de iniciar la medición.
