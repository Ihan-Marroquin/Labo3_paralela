#include <errno.h>
#include <limits.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

enum { TAG_DATA = 30 };

static int parse_positive_int(const char *text, const char *name, int rank) {
    char *end = NULL;
    errno = 0;
    long value = strtol(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || value <= 0 || value > INT_MAX) {
        if (rank == 0) {
            fprintf(stderr, "Error: %s debe ser un entero positivo.\n", name);
        }
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }
    return (int)value;
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    int rank = 0;
    int size = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size != 2) {
        if (rank == 0) {
            fprintf(stderr, "Error: recepcion_anticipada requiere exactamente 2 procesos.\n");
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    if (argc < 2 || argc > 3) {
        if (rank == 0) {
            fprintf(stderr, "Uso: %s <cantidad_enteros> [operaciones_por_prueba]\n", argv[0]);
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    const int count = parse_positive_int(argv[1], "cantidad_enteros", rank);
    const int work_block = argc == 3
                               ? parse_positive_int(argv[2], "operaciones_por_prueba", rank)
                               : 50000;

    int *message = malloc((size_t)count * sizeof(*message));
    if (message == NULL) {
        fprintf(stderr, "Rank %d: no se pudo reservar memoria.\n", rank);
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }
    for (int i = 0; i < count; ++i) {
        message[i] = rank == 0 ? 7 : 0;
    }

    MPI_Barrier(MPI_COMM_WORLD);
    const double start = MPI_Wtime();

    if (rank == 0) {
        MPI_Request request;
        MPI_Isend(message, count, MPI_INT, 1, TAG_DATA, MPI_COMM_WORLD, &request);
        MPI_Wait(&request, MPI_STATUS_IGNORE);
    } else {
        MPI_Request request;
        int complete = 0;
        long long operations = 0;
        int tests = 0;
        volatile double work = 0.0;

        MPI_Irecv(message, count, MPI_INT, 0, TAG_DATA, MPI_COMM_WORLD, &request);
        while (!complete) {
            for (int i = 0; i < work_block; ++i) {
                const double value = (double)((operations + i) % 97 + 1);
                work += value * 0.000001;
            }
            operations += work_block;
            ++tests;
            MPI_Test(&request, &complete, MPI_STATUS_IGNORE);
        }

        long long checksum = 0;
        for (int i = 0; i < count; ++i) {
            checksum += message[i];
        }
        const double elapsed = MPI_Wtime() - start;
        const long long bytes = (long long)count * (long long)sizeof(*message);
        printf("RECEPCION_ANTICIPADA enteros=%d bytes=%lld tiempo_s=%.9f "
               "pruebas_mpi_test=%d operaciones=%lld valor=%d checksum=%lld trabajo=%.6f\n",
               count, bytes, elapsed, tests, operations, message[0], checksum, work);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    free(message);
    MPI_Finalize();
    return EXIT_SUCCESS;
}
