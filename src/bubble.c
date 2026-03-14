#include <omp.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "sorting.h"

/*
   bubble sort -- sequential, parallel --
*/

void sequential_bubble_sort(uint64_t *T, const uint64_t size) {
    int sorted;
    do {
        sorted = 1; // we assume sorted in the beginning
        for (uint64_t i = 0; i < size - 1; i++) {
            if (T[i] > T[i + 1]) {
                uint64_t tmp = T[i];
                T[i]     = T[i + 1];
                T[i + 1] = tmp;
                sorted = 0; // we swapped, so its not sorted
            }
        }
    } while (!sorted); // as long as it's not swapped, redo the whole process
}

void parallel_bubble_sort(uint64_t *T, const uint64_t size) {
    int nthreads = omp_get_max_threads(); // see how many threads openmpwill use
    uint64_t chunk_size = size / nthreads; // divide array into equal chunks
    int global_sorted; // shared variable

    do {
        global_sorted = 1;

        // bubble sort each chunk in parallel
        #pragma omp parallel for schedule(static, 1) reduction(&:global_sorted)
        for (int t = 0; t < nthreads; t++) {
            uint64_t start = (uint64_t)t * chunk_size;
            uint64_t end   = (t == nthreads - 1) ? size : start + chunk_size;
            for (uint64_t i = start; i < end - 1; i++) {
                if (T[i] > T[i + 1]) {
                    uint64_t tmp = T[i];
                    T[i] = T[i + 1];
                    T[i + 1] = tmp;
                    global_sorted = 0;
                }
            }
        }

        // fix boundaries between chunks
        #pragma omp parallel for schedule(static, 1) reduction(&:global_sorted)
        for (int t = 0; t < nthreads - 1; t++) {
            uint64_t border = (uint64_t)(t + 1) * chunk_size - 1;
            if (T[border] > T[border + 1]) {
                uint64_t tmp  = T[border];
                T[border]     = T[border + 1];
                T[border + 1] = tmp;
                global_sorted = 0;
            }
        }
        
    } while (!global_sorted);
}

int main(int argc, char **argv) {
    // Init cpu_stats to measure CPU cycles and elapsed time
    struct cpu_stats *stats = cpu_stats_init();

    unsigned int exp;

    /* the program takes one parameter N which is the size of the array to
       be sorted. The array will have size 2^N */
    if (argc != 2) {
        fprintf(stderr, "Usage: bubble.run N \n");
        exit(-1);
    }

    uint64_t N = 1 << (atoi(argv[1]));
    /* the array to be sorted */
    uint64_t *X = (uint64_t *)malloc(N * sizeof(uint64_t));

    printf("--> Sorting an array of size %lu\n", N);
#ifdef RINIT
    printf("--> The array is initialized randomly\n");
#endif

    for (exp = 0; exp < NB_EXPERIMENTS; exp++) {
#ifdef RINIT
        init_array_random(X, N);
#else
        init_array_sequence(X, N);
#endif

        cpu_stats_begin(stats);

        sequential_bubble_sort(X, N);

        experiments[exp] = cpu_stats_end(stats);

        /* verifying that X is properly sorted */
#ifdef RINIT
        if (!is_sorted(X, N)) {
            print_array(X, N);
            fprintf(stderr, "ERROR: the sequential sorting of the array failed\n");
            exit(-1);
        }
#else
        if (!is_sorted_sequence(X, N)) {
            print_array(X, N);
            fprintf(stderr, "ERROR: the sequential sorting of the array failed\n");
            exit(-1);
        }
#endif
    }

    println_cpu_stats_report("bubble serial", average_report(experiments, NB_EXPERIMENTS));

    for (exp = 0; exp < NB_EXPERIMENTS; exp++) {
#ifdef RINIT
        init_array_random(X, N);
#else
        init_array_sequence(X, N);
#endif

        cpu_stats_begin(stats);

        parallel_bubble_sort(X, N);

        experiments[exp] = cpu_stats_end(stats);

        /* verifying that X is properly sorted */
#ifdef RINIT
        if (!is_sorted(X, N)) {
            print_array(X, N);
            fprintf(stderr, "ERROR: the parallel sorting of the array failed\n");
            exit(-1);
        }
#else
        if (!is_sorted_sequence(X, N)) {
            print_array(X, N);
            fprintf(stderr, "ERROR: the parallel sorting of the array failed\n");
            exit(-1);
        }
#endif
    }

    println_cpu_stats_report("bubble parallel", average_report(experiments, NB_EXPERIMENTS));

    /* print_array (X, N) ; */

    /* before terminating, we run one extra test of the algorithm */
    uint64_t *Y = (uint64_t *)malloc(N * sizeof(uint64_t));
    uint64_t *Z = (uint64_t *)malloc(N * sizeof(uint64_t));

#ifdef RINIT
    init_array_random(Y, N);
#else
    init_array_sequence(Y, N);
#endif

    memcpy(Z, Y, N * sizeof(uint64_t));

    sequential_bubble_sort(Y, N);
    parallel_bubble_sort(Z, N);

    if (!are_vector_equals(Y, Z, N)) {
        fprintf(stderr, "ERROR: sorting with the sequential and the parallel algorithm does not give the same result\n");
        exit(-1);
    }

    free(X);
    free(Y);
    free(Z);
}
