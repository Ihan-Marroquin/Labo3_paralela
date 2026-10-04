#include <errno.h>
#include <limits.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { TAG_PING = 10, TAG_PONG = 11 };

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
            fprintf(stderr, "Error: ping_pong requiere exactamente 2 procesos.\n");
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    if (argc < 2 || argc > 4) {
        if (rank == 0) {
            fprintf(stderr, "Uso: %s <iteraciones> [cantidad_enteros] [--detalle]\n", argv[0]);
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    const int iterations = parse_positive_int(argv[1], "iteraciones", rank);
    const int count = argc >= 3 && strcmp(argv[2], "--detalle") != 0
                          ? parse_positive_int(argv[2], "cantidad_enteros", rank)
                          : 1;
    const int detailed = strcmp(argv[argc - 1], "--detalle") == 0;

    int *message = malloc((size_t)count * sizeof(*message));
    if (message == NULL) {
        fprintf(stderr, "Rank %d: no se pudo reservar memoria.\n", rank);
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }
    for (int i = 0; i < count; ++i) {
        message[i] = 42;
    }

    MPI_Barrier(MPI_COMM_WORLD);
    const double start = MPI_Wtime();

    for (int iteration = 0; iteration < iterations; ++iteration) {
        if (rank == 0) {
            MPI_Send(message, count, MPI_INT, 1, TAG_PING, MPI_COMM_WORLD);
            MPI_Recv(message, count, MPI_INT, 1, TAG_PONG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (detailed) {
                printf("[rank 0] iteracion %d: ping enviado y pong recibido, valor=%d\n",
                       iteration + 1, message[0]);
                fflush(stdout);
            }
        } else {
            MPI_Recv(message, count, MPI_INT, 0, TAG_PING, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Send(message, count, MPI_INT, 0, TAG_PONG, MPI_COMM_WORLD);
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    const double elapsed = MPI_Wtime() - start;

    if (rank == 0) {
        const long long bytes = (long long)count * (long long)sizeof(*message);
        const double average_ms = elapsed * 1000.0 / (double)iterations;
        printf("PING_PONG iteraciones=%d enteros=%d bytes=%lld tiempo_total_s=%.9f "
               "tiempo_promedio_ms=%.9f valor_final=%d\n",
               iterations, count, bytes, elapsed, average_ms, message[0]);
    }

    free(message);
    MPI_Finalize();
    return EXIT_SUCCESS;
}
