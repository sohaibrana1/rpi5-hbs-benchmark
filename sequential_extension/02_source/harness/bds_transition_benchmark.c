#define _GNU_SOURCE

#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../xmss.h"
#include "../params.h"
#include "../utils.h"

#define MESSAGE_BYTES 32U
#define WARMUP_SIGNATURES 100U

typedef enum {
    FAMILY_XMSS = 0,
    FAMILY_XMSSMT = 1
} family_t;

static uint64_t diff_ns(const struct timespec *start,
                        const struct timespec *stop)
{
    uint64_t sec;
    int64_t nsec;

    sec = (uint64_t)(stop->tv_sec - start->tv_sec);
    nsec = (int64_t)stop->tv_nsec - (int64_t)start->tv_nsec;

    if (nsec < 0) {
        sec--;
        nsec += 1000000000LL;
    }

    return sec * 1000000000ULL + (uint64_t)nsec;
}

static int parse_positive_u64(const char *s, uint64_t *value)
{
    char *end = NULL;
    unsigned long long tmp;

    if (s == NULL || *s == '\0')
        return -1;

    errno = 0;
    tmp = strtoull(s, &end, 10);

    if (errno != 0 || end == s || *end != '\0' || tmp == 0)
        return -1;

    *value = (uint64_t)tmp;
    return 0;
}

static int determine_family(const char *variant, family_t *family)
{
    if (strncmp(variant, "XMSSMT-", 7) == 0) {
        *family = FAMILY_XMSSMT;
        return 0;
    }

    if (strncmp(variant, "XMSS-", 5) == 0) {
        *family = FAMILY_XMSS;
        return 0;
    }

    return -1;
}

static int get_oid_and_params(const char *variant,
                              family_t family,
                              uint32_t *oid,
                              xmss_params *params)
{
    int rc;

    if (family == FAMILY_XMSSMT) {
        rc = xmssmt_str_to_oid(oid, variant);
        if (rc != 0)
            return rc;

        return xmssmt_parse_oid(params, *oid);
    }

    rc = xmss_str_to_oid(oid, variant);
    if (rc != 0)
        return rc;

    return xmss_parse_oid(params, *oid);
}

static int generate_keypair(family_t family,
                            unsigned char *pk,
                            unsigned char *sk,
                            uint32_t oid)
{
    if (family == FAMILY_XMSSMT)
        return xmssmt_keypair(pk, sk, oid);

    return xmss_keypair(pk, sk, oid);
}

static int sign_message(family_t family,
                        unsigned char *sk,
                        unsigned char *sm,
                        unsigned long long *smlen,
                        const unsigned char *message,
                        unsigned long long message_len)
{
    if (family == FAMILY_XMSSMT) {
        return xmssmt_sign(
            sk, sm, smlen, message, message_len
        );
    }

    return xmss_sign(
        sk, sm, smlen, message, message_len
    );
}

static int verify_message(family_t family,
                          unsigned char *mout,
                          unsigned long long *mout_len,
                          const unsigned char *sm,
                          unsigned long long smlen,
                          const unsigned char *pk)
{
    if (family == FAMILY_XMSSMT) {
        return xmssmt_sign_open(
            mout, mout_len, sm, smlen, pk
        );
    }

    return xmss_sign_open(
        mout, mout_len, sm, smlen, pk
    );
}

static uint64_t secret_key_index(const unsigned char *sk,
                                 const xmss_params *params)
{
    /*
     * Wrapper-level secret keys start with the 4-byte OID.
     * The serialized XMSS/XMSSMT index immediately follows it.
     */
    return bytes_to_ull(
        sk + XMSS_OID_LEN,
        params->index_bytes
    );
}

static int validate_supported_study_variant(const char *variant)
{
    static const char *allowed[] = {
        "XMSS-SHA2_10_256",
        "XMSS-SHA2_16_256",
        "XMSSMT-SHA2_20/2_256",
        "XMSSMT-SHA2_20/4_256"
    };

    size_t i;

    for (i = 0; i < sizeof(allowed) / sizeof(allowed[0]); i++) {
        if (strcmp(variant, allowed[i]) == 0)
            return 0;
    }

    return -1;
}

static uint64_t usable_signature_count(const xmss_params *params)
{
    /*
     * The frozen FAST core rejects signing when
     *
     *   idx >= (2^full_height - 1)
     *
     * and therefore intentionally leaves the final theoretical index
     * unused.  For H < 64, the number of signatures accepted by this
     * implementation is consequently 2^H - 1, corresponding to
     * measured indices 0 .. 2^H - 2.
     */
    if (params->full_height >= 64)
        return UINT64_MAX - 1ULL;

    return (1ULL << params->full_height) - 1ULL;
}

int main(int argc, char **argv)
{
    const char *variant;
    const char *key_rep;
    const char *output_csv;

    uint64_t requested_signatures;
    uint64_t capacity;
    uint64_t i;

    family_t family;
    xmss_params params;
    uint32_t oid;

    unsigned char *warm_pk = NULL;
    unsigned char *warm_sk = NULL;

    unsigned char *pk = NULL;
    unsigned char *sk = NULL;

    unsigned char *sm = NULL;
    unsigned char *mout = NULL;

    unsigned char message[MESSAGE_BYTES];

    unsigned long long smlen = 0;
    unsigned long long mout_len = 0;

    FILE *csv = NULL;

    int rc = 1;

    struct timespec t0;
    struct timespec t1;
    struct timespec v0;
    struct timespec v1;

    uint64_t index_before;
    uint64_t index_after;
    uint64_t sign_ns;
    uint64_t verify_ns;

    int sign_rc;
    int verify_rc;

    if (argc != 5) {
        fprintf(
            stderr,
            "Usage: %s PARAMETER_SET SIGNATURE_COUNT KEY_REP OUTPUT.csv\n",
            argv[0]
        );
        return 2;
    }

    variant = argv[1];
    key_rep = argv[3];
    output_csv = argv[4];

    if (validate_supported_study_variant(variant) != 0) {
        fprintf(stderr, "ERROR: unsupported study variant: %s\n", variant);
        return 2;
    }

    if (parse_positive_u64(argv[2], &requested_signatures) != 0) {
        fprintf(stderr, "ERROR: invalid signature count: %s\n", argv[2]);
        return 2;
    }

    if (determine_family(variant, &family) != 0) {
        fprintf(stderr, "ERROR: could not determine family.\n");
        return 2;
    }

    if (get_oid_and_params(
            variant, family, &oid, &params
        ) != 0) {
        fprintf(stderr, "ERROR: parameter parsing failed.\n");
        return 2;
    }

    /*
     * This extension is explicitly restricted to the standardized
     * w=16 study configurations.
     */
    if (params.wots_w != 16U) {
        fprintf(
            stderr,
            "ERROR: expected w=16 but parsed w=%u\n",
            params.wots_w
        );
        return 2;
    }

    capacity = usable_signature_count(&params);

    if (requested_signatures > capacity) {
        fprintf(
            stderr,
            "ERROR: requested %" PRIu64
            " signatures exceeds implementation-usable count %" PRIu64 "\n",
            requested_signatures,
            capacity
        );
        return 2;
    }

    /*
     * Fixed 32-byte message. Message generation is deliberately outside
     * all timing regions.
     */
    for (i = 0; i < MESSAGE_BYTES; i++)
        message[i] = (unsigned char)(0xA5U ^ (unsigned char)i);

    warm_pk = malloc(XMSS_OID_LEN + params.pk_bytes);
    warm_sk = malloc(XMSS_OID_LEN + params.sk_bytes);

    pk = malloc(XMSS_OID_LEN + params.pk_bytes);
    sk = malloc(XMSS_OID_LEN + params.sk_bytes);

    sm = malloc(params.sig_bytes + MESSAGE_BYTES);
    /*
     * The frozen XMSS verification implementation uses the output buffer
     * as workspace while processing [signature || message].  Match the
     * allocation convention used by the original test/xmss.c runner.
     */
    mout = malloc(params.sig_bytes + MESSAGE_BYTES);

    if (warm_pk == NULL || warm_sk == NULL ||
        pk == NULL || sk == NULL ||
        sm == NULL || mout == NULL) {
        fprintf(stderr, "ERROR: allocation failure.\n");
        goto cleanup;
    }

    /*
     * ------------------------------------------------------------
     * WARM-UP KEY
     * ------------------------------------------------------------
     * The warm-up key is independent of the measured key.
     */
    if (generate_keypair(
            family, warm_pk, warm_sk, oid
        ) != 0) {
        fprintf(stderr, "ERROR: warm-up key generation failed.\n");
        goto cleanup;
    }

    for (i = 0; i < WARMUP_SIGNATURES; i++) {
        sign_rc = sign_message(
            family,
            warm_sk,
            sm,
            &smlen,
            message,
            MESSAGE_BYTES
        );

        if (sign_rc != 0) {
            fprintf(
                stderr,
                "ERROR: warm-up signing failed at index %" PRIu64 "\n",
                i
            );
            goto cleanup;
        }

        verify_rc = verify_message(
            family,
            mout,
            &mout_len,
            sm,
            smlen,
            warm_pk
        );

        if (verify_rc != 0 ||
            mout_len != MESSAGE_BYTES ||
            memcmp(message, mout, MESSAGE_BYTES) != 0) {
            fprintf(
                stderr,
                "ERROR: warm-up verification failed at index %" PRIu64 "\n",
                i
            );
            goto cleanup;
        }
    }

    /*
     * ------------------------------------------------------------
     * MEASUREMENT KEY
     * ------------------------------------------------------------
     * New key. Its index MUST begin at zero.
     */
    if (generate_keypair(
            family, pk, sk, oid
        ) != 0) {
        fprintf(stderr, "ERROR: measurement key generation failed.\n");
        goto cleanup;
    }

    if (secret_key_index(sk, &params) != 0) {
        fprintf(
            stderr,
            "ERROR: measurement key did not begin at index 0.\n"
        );
        goto cleanup;
    }

    /*
     * Refuse to overwrite an existing result file.
     */
    csv = fopen(output_csv, "wx");

    if (csv == NULL) {
        fprintf(
            stderr,
            "ERROR: cannot create output CSV '%s': %s\n",
            output_csv,
            strerror(errno)
        );
        goto cleanup;
    }

    fprintf(
        csv,
        "parameter_set,key_rep,signature_index,"
        "sk_index_before,sk_index_after,"
        "sign_ns,verify_ns,verification_pass,"
        "signature_bytes,secret_key_bytes,"
        "tree_height,full_height,layers,wots_w\n"
    );

    for (i = 0; i < requested_signatures; i++) {

        index_before = secret_key_index(sk, &params);

        /*
         * Signing timer contains ONLY the signing call.
         */
        if (clock_gettime(CLOCK_MONOTONIC_RAW, &t0) != 0) {
            perror("clock_gettime sign start");
            goto cleanup;
        }

        sign_rc = sign_message(
            family,
            sk,
            sm,
            &smlen,
            message,
            MESSAGE_BYTES
        );

        if (clock_gettime(CLOCK_MONOTONIC_RAW, &t1) != 0) {
            perror("clock_gettime sign stop");
            goto cleanup;
        }

        sign_ns = diff_ns(&t0, &t1);
        index_after = secret_key_index(sk, &params);

        if (sign_rc != 0) {
            fprintf(
                stderr,
                "ERROR: signing failed at measured row %" PRIu64
                " / index %" PRIu64 "\n",
                i,
                index_before
            );
            goto cleanup;
        }

        if (index_before != i) {
            fprintf(
                stderr,
                "ERROR: non-contiguous index before sign."
                " expected=%" PRIu64
                " actual=%" PRIu64 "\n",
                i,
                index_before
            );
            goto cleanup;
        }

        if (index_after != index_before + 1U) {
            fprintf(
                stderr,
                "ERROR: unexpected post-sign index."
                " before=%" PRIu64
                " after=%" PRIu64 "\n",
                index_before,
                index_after
            );
            goto cleanup;
        }

        if (smlen !=
            (unsigned long long)params.sig_bytes + MESSAGE_BYTES) {
            fprintf(
                stderr,
                "ERROR: unexpected signed-message length at index %" PRIu64 "\n",
                index_before
            );
            goto cleanup;
        }

        /*
         * Verification is deliberately outside the signing timer.
         */
        if (clock_gettime(CLOCK_MONOTONIC_RAW, &v0) != 0) {
            perror("clock_gettime verify start");
            goto cleanup;
        }

        verify_rc = verify_message(
            family,
            mout,
            &mout_len,
            sm,
            smlen,
            pk
        );

        if (clock_gettime(CLOCK_MONOTONIC_RAW, &v1) != 0) {
            perror("clock_gettime verify stop");
            goto cleanup;
        }

        verify_ns = diff_ns(&v0, &v1);

        if (verify_rc != 0 ||
            mout_len != MESSAGE_BYTES ||
            memcmp(message, mout, MESSAGE_BYTES) != 0) {
            fprintf(
                stderr,
                "ERROR: verification failed at index %" PRIu64 "\n",
                index_before
            );
            goto cleanup;
        }

        fprintf(
            csv,
            "%s,%s,%" PRIu64 ",%" PRIu64 ",%" PRIu64
            ",%" PRIu64 ",%" PRIu64
            ",PASS,%u,%llu,%u,%u,%u,%u\n",
            variant,
            key_rep,
            i,
            index_before,
            index_after,
            sign_ns,
            verify_ns,
            params.sig_bytes,
            (unsigned long long)XMSS_OID_LEN + params.sk_bytes,
            params.tree_height,
            params.full_height,
            params.d,
            params.wots_w
        );

        /*
         * Flush each measured row so a partial run remains auditable.
         */
        if (fflush(csv) != 0) {
            perror("fflush");
            goto cleanup;
        }
    }

    if (secret_key_index(sk, &params) != requested_signatures) {
        fprintf(
            stderr,
            "ERROR: final SK index mismatch."
            " expected=%" PRIu64
            " actual=%" PRIu64 "\n",
            requested_signatures,
            secret_key_index(sk, &params)
        );
        goto cleanup;
    }

    printf("PASS\n");
    printf("parameter_set=%s\n", variant);
    printf("key_rep=%s\n", key_rep);
    printf("measured_signatures=%" PRIu64 "\n", requested_signatures);
    printf("warmup_signatures=%u\n", WARMUP_SIGNATURES);
    printf("initial_measured_index=0\n");
    printf("final_measured_index=%" PRIu64 "\n",
           secret_key_index(sk, &params));
    printf("verification_passes=%" PRIu64 "\n",
           requested_signatures);
    printf("wots_w=%u\n", params.wots_w);
    printf("output_csv=%s\n", output_csv);

    rc = 0;

cleanup:

    if (csv != NULL)
        fclose(csv);

    free(warm_pk);
    free(warm_sk);
    free(pk);
    free(sk);
    free(sm);
    free(mout);

    return rc;
}
