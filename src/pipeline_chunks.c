#include <errno.h>
#include <limits.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { TAG_META = 40, TAG_DATA = 41, TAG_STOP = 42, BUFFER_COUNT = 2 };

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
            fprintf(stderr, "Error: pipeline_chunks requiere exactamente 2 procesos.\n");
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    if (argc < 3 || argc > 4) {
        if (rank == 0) {
            fprintf(stderr, "Uso: %s <total_enteros> <tamano_chunk> [--detalle]\n", argv[0]);
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    int total = parse_positive_int(argv[1], "total_enteros", rank);
    int chunk_size = parse_positive_int(argv[2], "tamano_chunk", rank);
    const int detailed = argc == 4 && strcmp(argv[3], "--detalle") == 0;

    if (chunk_size > total || total % chunk_size != 0) {
        if (rank == 0) {
            fprintf(stderr, "Error: tamano_chunk debe dividir exactamente a total_enteros.\n");
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    int metadata[2] = {total, chunk_size};
    if (rank == 0) {
        MPI_Send(metadata, 2, MPI_INT, 1, TAG_META, MPI_COMM_WORLD);
    } else {
        MPI_Recv(metadata, 2, MPI_INT, 0, TAG_META, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        total = metadata[0];
        chunk_size = metadata[1];
    }

    const int chunks = total / chunk_size;
    int *data = NULL;
    int *buffers[BUFFER_COUNT] = {NULL, NULL};

    if (rank == 0) {
        data = malloc((size_t)total * sizeof(*data));
        if (data != NULL) {
            for (int i = 0; i < total; ++i) {
                data[i] = i % 100;
            }
        }
    } else {
        for (int i = 0; i < BUFFER_COUNT; ++i) {
            buffers[i] = malloc((size_t)chunk_size * sizeof(**buffers));
        }
    }

    if ((rank == 0 && data == NULL) ||
        (rank == 1 && (buffers[0] == NULL || buffers[1] == NULL))) {
        fprintf(stderr, "Rank %d: no se pudo reservar memoria.\n", rank);
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    const double start = MPI_Wtime();

    if (rank == 0) {
        MPI_Request sends[BUFFER_COUNT] = {MPI_REQUEST_NULL, MPI_REQUEST_NULL};
        for (int chunk = 0; chunk < chunks; ++chunk) {
            const int slot = chunk % BUFFER_COUNT;
            if (sends[slot] != MPI_REQUEST_NULL) {
                MPI_Wait(&sends[slot], MPI_STATUS_IGNORE);
            }
            MPI_Isend(data + (size_t)chunk * chunk_size, chunk_size, MPI_INT,
                      1, TAG_DATA, MPI_COMM_WORLD, &sends[slot]);
        }
        MPI_Waitall(BUFFER_COUNT, sends, MPI_STATUSES_IGNORE);

        MPI_Request stop_request;
        MPI_Isend(NULL, 0, MPI_INT, 1, TAG_STOP, MPI_COMM_WORLD, &stop_request);
        MPI_Wait(&stop_request, MPI_STATUS_IGNORE);
    } else {
        MPI_Request receives[BUFFER_COUNT] = {MPI_REQUEST_NULL, MPI_REQUEST_NULL};
        const int initial = chunks < BUFFER_COUNT ? chunks : BUFFER_COUNT;
        for (int slot = 0; slot < initial; ++slot) {
            MPI_Irecv(buffers[slot], chunk_size, MPI_INT, 0, TAG_DATA,
                      MPI_COMM_WORLD, &receives[slot]);
        }

        long long checksum = 0;
        int posted = initial;
        int processed = 0;
        while (processed < chunks) {
            int slot = MPI_UNDEFINED;
            MPI_Waitany(BUFFER_COUNT, receives, &slot, MPI_STATUS_IGNORE);
            if (slot == MPI_UNDEFINED) {
                fprintf(stderr, "Error: no quedan recepciones activas.\n");
                MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
            }

            long long partial = 0;
            for (int i = 0; i < chunk_size; ++i) {
                partial += buffers[slot][i];
            }
            checksum += partial;
            ++processed;

            if (detailed) {
                printf("[rank 1] chunk %d/%d procesado, suma_parcial=%lld\n",
                       processed, chunks, partial);
                fflush(stdout);
            }

            if (posted < chunks) {
                MPI_Irecv(buffers[slot], chunk_size, MPI_INT, 0, TAG_DATA,
                          MPI_COMM_WORLD, &receives[slot]);
                ++posted;
            } else {
                receives[slot] = MPI_REQUEST_NULL;
            }
        }

        MPI_Request stop_request;
        int stop_received = 0;
        MPI_Irecv(NULL, 0, MPI_INT, 0, TAG_STOP, MPI_COMM_WORLD, &stop_request);
        while (!stop_received) {
            MPI_Test(&stop_request, &stop_received, MPI_STATUS_IGNORE);
        }

        const double elapsed = MPI_Wtime() - start;
        const long long bytes = (long long)total * (long long)sizeof(*data);
        printf("PIPELINE_CHUNKS total_enteros=%d chunk_enteros=%d chunks=%d bytes=%lld "
               "tiempo_total_s=%.9f checksum=%lld stop_recibido=%s\n",
               total, chunk_size, chunks, bytes, elapsed, checksum,
               stop_received ? "si" : "no");
    }

    MPI_Barrier(MPI_COMM_WORLD);
    free(data);
    free(buffers[0]);
    free(buffers[1]);
    MPI_Finalize();
    return EXIT_SUCCESS;
}
