#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include <openssl/rand.h>

#include "randombytes.h"

/*
 * Windows-compatible randombytes() implementation.
 *
 * OpenSSL RAND_bytes() is used because the XMSS reference project
 * already depends on OpenSSL for SHA-2.
 */
void randombytes(unsigned char *output, unsigned long long output_length)
{
    while (output_length > 0) {
        int chunk_length;

        if (output_length > (unsigned long long)INT_MAX) {
            chunk_length = INT_MAX;
        } else {
            chunk_length = (int)output_length;
        }

        if (RAND_bytes(output, chunk_length) != 1) {
            fprintf(stderr, "Error: OpenSSL RAND_bytes() failed.\n");
            abort();
        }

        output += chunk_length;
        output_length -= (unsigned long long)chunk_length;
    }
}