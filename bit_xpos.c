#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <openssl/md5.h>
#include <xmmintrin.h> // for _mm_prefetch

#define ARRAY_SZ 1048576 * 8
#define TRANSPOSE_SZ 64
typedef uint64_t my_size; 

void print_md5(unsigned char *digest) {
    char mdString[33];
    for (int i = 0; i < 16; ++i) {
        sprintf(&mdString[i * 2], "%02x", (unsigned int)digest[i]);
    }
    printf("MD5 digest: %s\n", mdString);
}

// IDK if I did this right, but it seems to work
void compute_md5(my_size transpose_array[TRANSPOSE_SZ][ARRAY_SZ/TRANSPOSE_SZ]) {
    unsigned char result[MD5_DIGEST_LENGTH];
    
    // Assuming the transpose_array is a contiguous block of memory, which it should be in C
    MD5((unsigned char*)transpose_array, sizeof(uint64_t) * TRANSPOSE_SZ * (ARRAY_SZ / TRANSPOSE_SZ), result);
    
    print_md5(result);
}

my_size array[ARRAY_SZ];
my_size transpose_array[TRANSPOSE_SZ][ARRAY_SZ/TRANSPOSE_SZ];

inline int get_bit(my_size *num_ptr, int i) {
    // if (num_ptr == NULL || i < 0 || i >= TRANSPOSE_SZ) {
    //     return -1; // Invalid pointer or index
    // }
    return (*num_ptr >> i) & 1;
}


// IF WE ZERO THE TRANS ARRAY, we only need to set the bit when 1
// Function to set the i-th bit of a 64-bit integer to a given value (bit_val)
inline void store_bit(my_size *num_ptr, int i, my_size bit_val) {
    // Clear the i-th bit
    // XXX this assumes you have zeroed the array!
    // *num_ptr &= ~(1ULL << i);
    
    // Set the i-th bit to bit_val
    *num_ptr |= (bit_val << i);
}

// This has an access pattern that should be better for the last level cache
// We measure it to be 
void transpose_array_new(my_size * array){
    for (int i = 0; i < ARRAY_SZ; i++) {
        for (int j = 0; j < TRANSPOSE_SZ; j++) {
            if(get_bit(&array[i], j)){
                store_bit(&transpose_array[j][i/TRANSPOSE_SZ], i%TRANSPOSE_SZ, 1);
            }
        }
    }
}

// void transpose_array_old(my_size * array){
//     // outer loop over bits, inner loop over array elements
//     for (int i = 0; i < TRANSPOSE_SZ; i++) {
//         for (int j = 0; j < ARRAY_SZ; j++) {
//             if(get_bit(&array[j], i)){
//                 store_bit(&transpose_array[i][j/TRANSPOSE_SZ], j%TRANSPOSE_SZ, 1);
//             }
//         }
//     }
// }

// This was a shot in the dark, didn't really expect it to work
// void transpose_array_new(my_size * array){
//     for (int i = 0; i < ARRAY_SZ/2; i++) {
//         for (int j = 0; j < TRANSPOSE_SZ; j++) {
//             store_bit(&transpose_array[j][i/TRANSPOSE_SZ], i%TRANSPOSE_SZ, get_bit(&array[i], j));
//         }
//     }
//     for (int i = ARRAY_SZ/2; i < ARRAY_SZ; i++) {
//         for (int j = 0; j < TRANSPOSE_SZ; j++) {
//             store_bit(&transpose_array[j][i/TRANSPOSE_SZ], i%TRANSPOSE_SZ, get_bit(&array[i], j));
//         }
//     }
// }



// The idea here is to cut in half the number of hot cache lines in the transpose array
// Trade more misses in the input array for reduced cache contension
// XXX This is much slower
// void transpose_array_new(my_size * array){
//     // Fill the first 32 rows of the transpose array
//     for (int i = 0; i < ARRAY_SZ; i++) {
//         for (int j = 0; j < TRANSPOSE_SZ/2; j++) {
//             store_bit(&transpose_array[j][i/TRANSPOSE_SZ], i%TRANSPOSE_SZ, get_bit(&array[i], j));
//         }
//     }

//     // Fill the last 32 rows of the transpose array
//     for (int i = 0; i < ARRAY_SZ; i++) {
//         for (int j = TRANSPOSE_SZ/2; j < TRANSPOSE_SZ; j++) {
//             store_bit(&transpose_array[j][i/TRANSPOSE_SZ], i%TRANSPOSE_SZ, get_bit(&array[i], j));
//         }
//     }
// }

// BAD
// #define BLOCK_SIZE 16  // Choose a suitable block size
// void transpose_array_new(uint64_t * array) {
//         for (int jj = 0; jj < TRANSPOSE_SZ; jj += BLOCK_SIZE) {
//             // Inner loops
//             for (int i = 0; i < ARRAY_SZ; ++i) {
//                 for (int j = jj; j < jj + BLOCK_SIZE && j < TRANSPOSE_SZ; j++) {
//                     store_bit(&transpose_array[j][i / TRANSPOSE_SZ], i % TRANSPOSE_SZ, get_bit(&array[i], j));
//                 }
//             }
//         }
// }

// #define BLOCK_SIZE 4  // Choose a suitable block size
// void transpose_array_new(uint64_t * array) {
//     for (int ii = 0; ii < ARRAY_SZ; ii += BLOCK_SIZE) {
//         for (int jj = 0; jj < TRANSPOSE_SZ; jj += BLOCK_SIZE) {
//             // Inner loops
//             for (int i = ii; i < ii + BLOCK_SIZE && i < ARRAY_SZ; ++i) {
//                 for (int j = jj; j < jj + BLOCK_SIZE && j < TRANSPOSE_SZ; j++) {
//                     store_bit(&transpose_array[j][i / TRANSPOSE_SZ], i % TRANSPOSE_SZ, get_bit(&array[i], j));
//                 }
//             }
//         }
//     }
// }
// missing in l1 d cache
// void transpose_array_new(my_size * array){
//     for (int i = 0; i < ARRAY_SZ; i++) {
//         for (int j = 0; j < TRANSPOSE_SZ; j+=4) {
//             store_bit(&transpose_array[j][i/TRANSPOSE_SZ], i%TRANSPOSE_SZ, get_bit(&array[i], j));
//             store_bit(&transpose_array[j+1][i/TRANSPOSE_SZ], i%TRANSPOSE_SZ, get_bit(&array[i], j+1));
//             store_bit(&transpose_array[j+2][i/TRANSPOSE_SZ], i%TRANSPOSE_SZ, get_bit(&array[i], j+2));
//             store_bit(&transpose_array[j+3][i/TRANSPOSE_SZ], i%TRANSPOSE_SZ, get_bit(&array[i], j+3));
//         }
//     }
// }

// #define TILE_SZ 16  // or some other value

// void transpose_array_old(my_size * array){
//     for (int ii = 0; ii < TRANSPOSE_SZ; ii += TILE_SZ) {
//         for (int j = 0; j < ARRAY_SZ; j++) {
//             for (int i = ii; i < ii + TILE_SZ && i < TRANSPOSE_SZ; i++) {
//                 store_bit(&transpose_array[i][j/TRANSPOSE_SZ], j%TRANSPOSE_SZ, get_bit(&array[j], i));
//             }
//         }
//     }
// }

// void transpose_array_old(my_size *array) {
//     for (int i = 0; i < TRANSPOSE_SZ; ++i) {
//         for (int j = 0; j < ARRAY_SZ; ++j) {
//             store_bit(&transpose_array[i][j / TRANSPOSE_SZ], j % TRANSPOSE_SZ, get_bit(&array[j], i));
//         }
//     }
// }

// void transpose_array_old(my_size *array) {
//     int block_sz = 2; // Pick a suitable block size
//     for (int bi = 0; bi < TRANSPOSE_SZ; bi += block_sz) {
//         for (int bj = 0; bj < ARRAY_SZ; bj += block_sz) {
//             for (int i = bi; i < bi + block_sz; ++i) {
//                 for (int j = bj; j < bj + block_sz; ++j) {
//                     store_bit(&transpose_array[i][j / TRANSPOSE_SZ], j % TRANSPOSE_SZ, get_bit(&array[j], i));
//                 }
//             }
//         }
//     }
// }
// void transpose_array_old(my_size *array) {
//     int block_sz_i = 8; // Pick a suitable block size for i
//     for (int bi = 0; bi < TRANSPOSE_SZ; bi += block_sz_i) {
//         for (int i = bi; i < bi + block_sz_i && i < TRANSPOSE_SZ; ++i) {
//             for (int j = 0; j < ARRAY_SZ; ++j) {
//                 store_bit(&transpose_array[i][j / TRANSPOSE_SZ], j % TRANSPOSE_SZ, get_bit(&array[j], i));
//             }
//         }
//     }
// }

#define PREFETCH_DISTANCE (64)
// by far the best
void transpose_array_old(my_size *array) {
    int block_sz_i = 4; // Pick a suitable block size
    int block_sz_j = 2; // Pick a suitable block size

    for (int bi = 0; bi < TRANSPOSE_SZ; bi += block_sz_i) {
        for (int bj = 0; bj < ARRAY_SZ; bj += block_sz_j) {
            if (bi + PREFETCH_DISTANCE / TRANSPOSE_SZ < TRANSPOSE_SZ) {
                _mm_prefetch((const char*)&transpose_array[bi][(bj / TRANSPOSE_SZ) + (PREFETCH_DISTANCE / TRANSPOSE_SZ)], _MM_HINT_T0);
            }
            for (int i = bi; i < bi + block_sz_i && i < TRANSPOSE_SZ; ++i) {
                for (int j = bj; j < bj + block_sz_j && j < ARRAY_SZ; j+=1) {
                    if(get_bit(&array[j], i)){
                        store_bit(&transpose_array[i][j / TRANSPOSE_SZ], j % TRANSPOSE_SZ, 1);
                    }
                }
            }
        }
    }
}
// void transpose_array_old(my_size *array) {
//     int block_sz_i = 2; // Pick a suitable block size
//     for (int bi = 0; bi < TRANSPOSE_SZ; bi += block_sz_i) {
//             for (int i = bi; i < bi + block_sz_i && i < TRANSPOSE_SZ; ++i) {
//                 for (int j = 0; j < ARRAY_SZ; ++j) {
//                     store_bit(&transpose_array[i][j / TRANSPOSE_SZ], j % TRANSPOSE_SZ, get_bit(&array[j], i));
//                 }
//             }
//         }
// }




// missing in l3
// void transpose_array_old(my_size * array){
//     // outer loop over bits, inner loop over array elements
//     for (int i = 0; i < TRANSPOSE_SZ; i++) {
//         for (int j = 0; j < ARRAY_SZ; j+=4) {
//             store_bit(&transpose_array[i][j/TRANSPOSE_SZ], j%TRANSPOSE_SZ, get_bit(&array[j], i));
//             store_bit(&transpose_array[i][(j+1)/TRANSPOSE_SZ], (j+1)%TRANSPOSE_SZ, get_bit(&array[j+1], i));
//             store_bit(&transpose_array[i][(j+2)/TRANSPOSE_SZ], (j+2)%TRANSPOSE_SZ, get_bit(&array[j+2], i));
//             store_bit(&transpose_array[i][(j+3)/TRANSPOSE_SZ], (j+3)%TRANSPOSE_SZ, get_bit(&array[j+3], i));
//         }
//     }
// }

int main(int argc, char **argv){
    // Check that we got a single argument
    if (argc != 2) {
        printf("Usage: %s <selector: 0 or 1>\n", argv[0]);
        return -1;
    }
    int selector = atoi(argv[1]);
    if(selector == 0) {
        printf("Using transpose_array_old\n");
    } else if(selector == 1) {
        printf("Using transpose_array_new\n");
    } else {
        printf("Invalid transpose selector: %d\n", selector);
        return -1;
    }

    // srand(time(NULL)); // Initialize random seed using current time

    // Fill array with random values
    // for (int i = 0; i < ARRAY_SZ; ++i) {
    //     my_size rand_value = (((my_size)rand()) << 32) | rand();
    //     array[i] = rand_value;
    // }

    // Fill array with incrementing values (so we can md5sum at the end)
    for (int i = 0; i < ARRAY_SZ; ++i) {
        array[i] = i;
    }

    // zero out the transpose array
    for (int i = 0; i < TRANSPOSE_SZ; i++) {
        for (int j = 0; j < ARRAY_SZ/TRANSPOSE_SZ; j++) {
            transpose_array[i][j] = 0;
        }
    }

    printf("Done rand\n");

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    if(selector == 0){
        transpose_array_old(array);
    } else {
        transpose_array_new(array);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    // print elapsed time in seconds
    double elapsed_ns = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
    printf("Elapsed time: %f seconds\n", elapsed_ns / 1e9);

    // change the transpose array
    compute_md5(transpose_array);


}