#include <errno.h>
#include <limits.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { TAG_TOKEN = 20 };

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

static int run_blocking(int rank, int size, int cycles, int detailed) {
    const int source = (rank - 1 + size) % size;
    const int destination = (rank + 1) % size;
    int token = 0;

    for (int cycle = 0; cycle < cycles; ++cycle) {
        if (rank == 0) {
            if (detailed) {
                printf("[rank 0] ciclo %d: inicia token=%d y envia a rank %d\n",
                       cycle + 1, token, destination);
                fflush(stdout);
            }
            MPI_Send(&token, 1, MPI_INT, destination, TAG_TOKEN, MPI_COMM_WORLD);
            MPI_Recv(&token, 1, MPI_INT, source, TAG_TOKEN, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            ++token;
            if (detailed) {
                printf("[rank 0] ciclo %d: recibe de rank %d, token=%d\n",
                       cycle + 1, source, token);
                fflush(stdout);
            }
        } else {
            MPI_Recv(&token, 1, MPI_INT, source, TAG_TOKEN, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            ++token;
            if (detailed) {
                printf("[rank %d] recibe de rank %d y envia a rank %d, token=%d\n",
                       rank, source, destination, token);
                fflush(stdout);
            }
            MPI_Send(&token, 1, MPI_INT, destination, TAG_TOKEN, MPI_COMM_WORLD);
        }
    }
    return rank == 0 ? token : -1;
}

static int run_sendrecv(int rank, int size, int cycles, int detailed) {
    const int source = (rank - 1 + size) % size;
    const int destination = (rank + 1) % size;
    int token = rank == 0 ? 0 : -1;

    for (int cycle = 0; cycle < cycles; ++cycle) {
        for (int hop = 0; hop < size; ++hop) {
            int received = -1;
            MPI_Sendrecv(&token, 1, MPI_INT, destination, TAG_TOKEN,
                         &received, 1, MPI_INT, source, TAG_TOKEN,
                         MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            token = received;
            if (token >= 0) {
                ++token;
                if (detailed) {
                    printf("[rank %d] ciclo %d, salto %d: recibe de rank %d, token=%d\n",
                           rank, cycle + 1, hop + 1, source, token);
                    fflush(stdout);
                }
            }
        }
    }
    return rank == 0 ? token : -1;
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    int rank = 0;
    int size = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size <= 4) {
        if (rank == 0) {
            fprintf(stderr, "Error: token_ring requiere mas de 4 procesos.\n");
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    if (argc < 2 || argc > 4 ||
        (strcmp(argv[1], "blocking") != 0 && strcmp(argv[1], "sendrecv") != 0)) {
        if (rank == 0) {
            fprintf(stderr, "Uso: %s <blocking|sendrecv> [ciclos] [--detalle]\n", argv[0]);
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    const char *method = argv[1];
    const int cycles = argc >= 3 && strcmp(argv[2], "--detalle") != 0
                           ? parse_positive_int(argv[2], "ciclos", rank)
                           : 1;
    const int detailed = strcmp(argv[argc - 1], "--detalle") == 0;

    MPI_Barrier(MPI_COMM_WORLD);
    const double start = MPI_Wtime();
    const int final_token = strcmp(method, "blocking") == 0
                                ? run_blocking(rank, size, cycles, detailed)
                                : run_sendrecv(rank, size, cycles, detailed);
    MPI_Barrier(MPI_COMM_WORLD);
    const double elapsed = MPI_Wtime() - start;

    if (rank == 0) {
        const int expected = size * cycles;
        printf("TOKEN_RING metodo=%s procesos=%d ciclos=%d token_final=%d esperado=%d "
               "tiempo_s=%.9f estado=%s\n",
               method, size, cycles, final_token, expected, elapsed,
               final_token == expected ? "correcto" : "incorrecto");
    }

    MPI_Finalize();
    return final_token == size * cycles || rank != 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
