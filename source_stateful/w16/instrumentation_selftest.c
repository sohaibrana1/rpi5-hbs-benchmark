#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "instrumentation.h"

static void busy_work(unsigned int rounds)
{
    volatile uint64_t value = 1U;
    unsigned int i;

    for (i = 0U; i < rounds; i++) {
        value = value * UINT64_C(6364136223846793005) + 1U;
    }

    if (value == 0U) {
        puts("unreachable");
    }
}

int main(void)
{
    hbs_instrumentation_snapshot_t snapshot;

    if (hbs_instrumentation_init() != 0) {
        fprintf(stderr, "self-test: timer initialization failed\n");
        return EXIT_FAILURE;
    }

    hbs_instrumentation_start();
    HBS_COMPONENT_BEGIN(HBS_COMPONENT_OTHER);
    busy_work(10000U);

    HBS_COMPONENT_BEGIN(HBS_COMPONENT_MESSAGE_HASH);
    HBS_COUNT_HASH_MESSAGE();
    HBS_NOTE_HASH_CALLS(2U);
    busy_work(10000U);
    HBS_COMPONENT_END(HBS_COMPONENT_MESSAGE_HASH);

    HBS_COMPONENT_BEGIN(HBS_COMPONENT_WOTS_SIGN);
    HBS_COUNT_HASH_F();
    HBS_NOTE_HASH_CALLS(4U);
    busy_work(10000U);
    HBS_COMPONENT_END(HBS_COMPONENT_WOTS_SIGN);

    busy_work(10000U);
    HBS_COMPONENT_END(HBS_COMPONENT_OTHER);

    if (hbs_instrumentation_stop(&snapshot) != 0 ||
        snapshot.error_code != 0 ||
        snapshot.unscoped_hash_calls != 0U ||
        snapshot.component[HBS_COMPONENT_OTHER].calls != 1U ||
        snapshot.component[HBS_COMPONENT_MESSAGE_HASH].calls != 1U ||
        snapshot.component[HBS_COMPONENT_WOTS_SIGN].calls != 1U ||
        snapshot.component[HBS_COMPONENT_MESSAGE_HASH].hash_calls != 3U ||
        snapshot.component[HBS_COMPONENT_WOTS_SIGN].hash_calls != 5U ||
        hbs_snapshot_specific_call_count(&snapshot) != 2U ||
        hbs_snapshot_elapsed_sum_ns(&snapshot) == 0U) {

        fprintf(stderr, "self-test: valid nested timing case failed\n");
        return EXIT_FAILURE;
    }

    hbs_instrumentation_start();
    HBS_COMPONENT_BEGIN(HBS_COMPONENT_OTHER);
    HBS_COMPONENT_END(HBS_COMPONENT_WOTS_SIGN);

    if (hbs_instrumentation_stop(&snapshot) == 0 ||
        snapshot.error_code == 0) {

        fprintf(stderr, "self-test: mismatched scope was not detected\n");
        return EXIT_FAILURE;
    }

    puts("instrumentation self-test: PASS");
    return EXIT_SUCCESS;
}
