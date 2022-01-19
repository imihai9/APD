// Mihai Ionescu, 335CA
// mpirun -np <numar_procese> ./tema3 <dimensiune_vector> <eroare_comunicatie>
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CNT_COORDS 3
#define INP_FILE_NAME_SIZE 13
#define min(x, y) (x < y) ? x : y

#define ARRAY_SEND_TAG 0
#define ARRAY_LEN_SEND_TAG 1

int num_procs, rank;
int *parents;

int *v, v_len;
int *v_recv, v_recv_len;

// Variables only used by coordinators
char faulty_link; // 0 = (0, 1) link exists
int *coords, *c_cnt_workers, cnt_workers_total;
int *workers; // worker ids

// Used by coordinator 0
int *start_pos;
int *v_final;

// Uses defaulty_link communicator MPI_COMM_WORLD
// verbose = 0 (quiet) OR !=0 (print M(src, dst))
void send_print(void *buf, int count, MPI_Datatype datatype, int dest, int tag, char verbose){
    MPI_Send(buf, count, datatype, dest, tag, MPI_COMM_WORLD);
    if (verbose)
        printf("M(%d,%d)\n", rank, dest); // log
}

void print_topology(int rank, int *parents) {
    printf("%d -> ", rank);

    short is_parent;

    for (int p = 0; ; p++) { // parent
        is_parent = 0;
        for (int w = 0; w < num_procs; w++) { // worker
            if (parents[w] == p) {
                if (!is_parent) {
                    is_parent = 1;
                    printf("%d:%d", p, w);
                } else {
                    printf(",%d", w);
                }
            }
        }

        if (!is_parent) {
            printf("\n");
            return;
        } else {
            printf(" ");
        }
    }
}

// COORDINATOR HELP FUNCTIONS -----------------------------------

// Generates 'parents' array based on information from the other coords
void c_establish_topology(int *coords, int *c_cnt_workers, int *parents) {
    int *parents_recv = (int*)malloc(num_procs * sizeof(int));
    memset(parents_recv, -1, num_procs * sizeof(int));

    for (int i = 0; i < CNT_COORDS; i++) {
        if (rank != coords[i]) {
            // Send own parent array (filled only with own wokers)
            // to the other coordinators
            send_print(parents, num_procs, MPI_INT, coords[i], ARRAY_SEND_TAG, 1);
        }
    }

    for (int i = 0; i < CNT_COORDS - 1; i++) {
      // Receive the corresponding array from the other coordinators
        MPI_Status status;
        MPI_Recv(parents_recv, num_procs, MPI_INT, MPI_ANY_SOURCE, ARRAY_SEND_TAG, MPI_COMM_WORLD, &status);
        for (int j = 0; j < num_procs; j++) {
            // Update local parents array with values received
            if (parents[j] == -1 && parents_recv[j] != -1) {
                parents[j] = parents_recv[j];
                c_cnt_workers[status.MPI_SOURCE]++;
            }
        }
    }

    if (parents_recv)
        free(parents_recv);
}

// (COORD 0) Divides and assigns array chunks to each coordinate
// Saves starting positions in initial array of the chunks of each worker
void c0_assign_arr_to_coords (int *c_cnt_workers, int *coords, int cnt_workers_total, int *start_pos) {
    v = (int*)malloc(v_len * sizeof(int));
    for (int i = 0; i < v_len; i++)
        v[i] = i;

    // Start, end v array ids corresponding to each worker
    int id_start = 0;
    int id_end;
    for (int c_rank = 0; c_rank < CNT_COORDS; c_rank++) {
        id_end = id_start + c_cnt_workers[c_rank] - 1;

        int start = id_start * (double) v_len / cnt_workers_total;
        start_pos[c_rank] = start;
        int end   = min((id_end + 1) * (double) v_len / cnt_workers_total, v_len);
        int chunk_len = end - start;

        if (c_rank > 0) {
            // Send array length first
            send_print(&chunk_len, 1, MPI_INT, coords[c_rank], ARRAY_LEN_SEND_TAG, 1);
            // Send array
            send_print(v + start, end - start, MPI_INT, coords[c_rank], ARRAY_SEND_TAG, 1);
        }
        else {
            // C0 array chunk, of size v_recv_len, stored in v_recv
            v_recv_len = chunk_len;
            v_recv = (int*) malloc(v_recv_len * sizeof(int));
            memcpy(v_recv, v + start, v_recv_len * sizeof(int));
        }

        id_start = id_end + 1;
    }
}

void c_recv_array_chunk(int *c_cnt_workers, int cnt_workers_total) {
    // For each coordiantor - receive chunk of v generated by coord 0
    // Array chunks - size v_recv_len; stored in v_recv
    MPI_Status status;
    MPI_Recv(&v_recv_len, 1, MPI_INT, 0, ARRAY_LEN_SEND_TAG, MPI_COMM_WORLD, &status);
    v_recv = (int*) malloc(v_recv_len * sizeof(int));
    MPI_Recv(v_recv, v_recv_len, MPI_INT, 0, ARRAY_SEND_TAG, MPI_COMM_WORLD, &status);
}

// (ALL COORDS) Divides and assigns subarray chunks to each worker
void c_assign_arr_to_workers(int cnt_workers, int *workers) {
    int id = 0;
    for (int id = 0; id < cnt_workers; id++) {
        int start = id * (double) v_recv_len / cnt_workers;
        int end   = min((id + 1) * (double) v_recv_len / cnt_workers, v_recv_len);
        int chunk_len = end - start;
        send_print(&chunk_len, 1, MPI_INT, workers[id], ARRAY_LEN_SEND_TAG, 1);
        send_print(v_recv + start, chunk_len, MPI_INT, workers[id], ARRAY_SEND_TAG, 1);
    }
}

void c_recv_arr_from_workers(int cnt_workers, int *workers) {
    // Rebuild array from chunks received from workers in 'v'
    MPI_Status status;
    int chunk_len;

    // Receive cnt_workers arrays, in any order
    for (int i = 0; i < cnt_workers; i++) {
        MPI_Recv(&chunk_len, 1, MPI_INT, MPI_ANY_SOURCE, ARRAY_LEN_SEND_TAG, MPI_COMM_WORLD, &status);

        // Find id corresponding to the worker that sent the msg
        int id;
        for (int i_id = 0; i_id < cnt_workers; i_id++) {
            if (workers[i_id] == status.MPI_SOURCE) {
                id = i_id;
                break;
            }
        }

        // Recalculation is less expensive than keeping start positions in an array
        int start = id * (double) v_recv_len / cnt_workers;
        MPI_Recv(v_recv + start, chunk_len, MPI_INT, workers[id], ARRAY_SEND_TAG, MPI_COMM_WORLD, &status);
    }
}

void c_send_arr_to_c0(int *v_recv_new, int len) {
    send_print(&len, 1, MPI_INT, 0, ARRAY_LEN_SEND_TAG, 1);
    send_print(v_recv_new, len, MPI_INT, 0, ARRAY_SEND_TAG, 1);
}

void c0_assemble_final_array(int len) { //TODO: delete param
    MPI_Status status;
    int chunk_len;

    v_final = (int*)malloc(len * sizeof(int));
    memcpy(v_final + 0, v_recv, v_recv_len * sizeof(int));
    // Receive subarrays, in any order

    for (int i = 0; i < CNT_COORDS - 1; i++) {
        MPI_Recv(&chunk_len, 1, MPI_INT, MPI_ANY_SOURCE, ARRAY_LEN_SEND_TAG, MPI_COMM_WORLD, &status);
        int coord_id = status.MPI_SOURCE;
        MPI_Recv(v_final + start_pos[coord_id], chunk_len, MPI_INT, coord_id, ARRAY_SEND_TAG, MPI_COMM_WORLD, &status);
    }
}

void c0_print_final_array(int *v_final) {
    printf("Rezultat: ");
    for (int i = 0; i < v_len; i++) {
        printf("%d ", v_final[i]);
    }
    printf("\n");
}

// MAIN COORDINATOR FUNCTIONS -----------------------------------
void exec_coordinator_part1() {
    coords = malloc(CNT_COORDS * sizeof(int));
    for (int i = 0; i < CNT_COORDS; i++)
        coords[i] = i;

    int num_workers;
    c_cnt_workers = calloc (CNT_COORDS, sizeof(int)); // Number of workers that each coordinator has

    char file_name[INP_FILE_NAME_SIZE];

    // Read own workers from file
    sprintf(file_name, "%s%d%s", "cluster", rank, ".txt");
    
    FILE *in = fopen(file_name, "rt");
    fscanf(in, "%d", &num_workers);
    c_cnt_workers[rank] = num_workers;

    memset(parents, -1, num_procs * sizeof(int));
    workers = (int*)malloc(num_workers * sizeof(int));
    for (int i = 0; i < num_workers; i++) {
        fscanf(in, "%d", &workers[i]);
        parents[workers[i]] = rank;
    }

    c_establish_topology(coords, c_cnt_workers, parents);
    
    cnt_workers_total = 0;
    for (int i = 0; i < CNT_COORDS; i++)
        cnt_workers_total += c_cnt_workers[i];

    // Coordinators now know the full topology.
    print_topology(rank, parents);

    // Send parent array to own workers
    for (int i = 0; i < num_workers; i++)
        send_print(parents, num_procs, MPI_INT, workers[i], ARRAY_SEND_TAG, 1);
}

void exec_coordinator_part2 () {
    if (rank == 0) {
        // Starting position in initial array of each subarray given to coordinators
        start_pos = (int*) malloc(sizeof(int) * CNT_COORDS);
        c0_assign_arr_to_coords(c_cnt_workers, coords, cnt_workers_total, start_pos);
    }
    else
        c_recv_array_chunk(c_cnt_workers, cnt_workers_total);

    c_assign_arr_to_workers(c_cnt_workers[rank], workers);
    c_recv_arr_from_workers(c_cnt_workers[rank], workers);
}

void exec_coordinator_part3 () {
    if (rank != 0)
        c_send_arr_to_c0(v_recv, v_recv_len);
    else
        c0_assemble_final_array(v_len);
}

// WORKER HELP FUNCTIONS ------------------------------

void w_recv_array_chunk() {
    MPI_Status status;
    // Receive array len;   parents[rank] == coordinator
    MPI_Recv(&v_len, 1, MPI_INT, parents[rank], ARRAY_LEN_SEND_TAG, MPI_COMM_WORLD, &status);
    v = (int*)malloc(v_len * sizeof(int));
    // Receive array
    MPI_Recv(v, v_len, MPI_INT, parents[rank], ARRAY_SEND_TAG, MPI_COMM_WORLD, &status);
}

void w_compute() {
    for (int i = 0; i < v_len; i++)
        v[i] *= 2;
}

// Send array result (after computation) to coordinator
void w_send_array_result() {
    send_print(&v_len, 1, MPI_INT, parents[rank], ARRAY_LEN_SEND_TAG, 1);
    send_print(v, v_len, MPI_INT, parents[rank], ARRAY_SEND_TAG, 1);
}

// MAIN WORKER FUNCTIONS ------------------------------

void exec_worker_part1() {
    int coord; // coordinator

    // Receive parent array from coordinator
    MPI_Status status;
    MPI_Recv(parents, num_procs, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);

    coord = parents[rank]; // same as status.MPI_SOURCE;
    print_topology(rank, parents);
}

void exec_worker_part2() {
    w_recv_array_chunk();
    w_compute();
    w_send_array_result();
}

// MAIN -----------------------------------------------

int main (int argc, char *argv[]) {
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs); // Total number of processes.
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);       // The current process ID / Rank.

    parents = (int*)malloc(num_procs * sizeof(int));

    if (rank < CNT_COORDS) {
        // If current process is COORD 0 => assign v_len parameter
        if (rank == 0) {
            v_len = atoi(argv[1]);
            faulty_link = atoi(argv[2]);
        }
        exec_coordinator_part1();
    } else {
        exec_worker_part1();
    }
 
    // Barrier, to make sure each node receives the complete topology
    MPI_Barrier(MPI_COMM_WORLD);

    if (rank < CNT_COORDS)
        exec_coordinator_part2();
    else
        exec_worker_part2();

    // Barrier, to make sure that coordinator 0 is ready to receive
    // subarrays from the other coordinators
    MPI_Barrier(MPI_COMM_WORLD);

    if (rank < CNT_COORDS)
        exec_coordinator_part3();

    // Barrier, to avoid messages generated while printing the
    // result
    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 0)
        c0_print_final_array(v_final);

    MPI_Finalize();
    return 0;
}
