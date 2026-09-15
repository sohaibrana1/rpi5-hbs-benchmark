#include <stdio.h>
#include <string.h>
#include <time.h>

#if defined(_WIN32)
#include <windows.h>
#endif

#include "instrumentation.h"

#define HBS_COMPONENT_STACK_MAX 64U

enum {
    HBS_INSTRUMENTATION_OK = 0,
    HBS_INSTRUMENTATION_ERR_CLOCK = 1,
    HBS_INSTRUMENTATION_ERR_COMPONENT = 2,
    HBS_INSTRUMENTATION_ERR_STACK_OVERFLOW = 3,
    HBS_INSTRUMENTATION_ERR_STACK_MISMATCH = 4,
    HBS_INSTRUMENTATION_ERR_UNCLOSED_SCOPE = 5,
    HBS_INSTRUMENTATION_ERR_NULL_SNAPSHOT = 6
};

typedef struct {
    hbs_component_id_t component;
    uint64_t resumed_at_ns;
} hbs_component_frame_t;

hbs_counters_t g_hbs_counters;

static hbs_component_stat_t g_component_stats[HBS_COMPONENT_COUNT];
static hbs_component_frame_t g_component_stack[HBS_COMPONENT_STACK_MAX];
static size_t g_component_depth;
static uint64_t g_unscoped_hash_calls;
static int g_collecting;
static int g_initialized;
static int g_error_code;

#if defined(_WIN32)
static LARGE_INTEGER g_qpc_frequency;
#endif

static int instrumentation_now_ns(uint64_t *value)
{
#if defined(_WIN32)
    LARGE_INTEGER counter;
    long double nanoseconds;

    if (!QueryPerformanceCounter(&counter)) {
        return -1;
    }

    nanoseconds =
        (long double)counter.QuadPart * 1000000000.0L /
        (long double)g_qpc_frequency.QuadPart;

    *value = (uint64_t)nanoseconds;
    return 0;
#else
    struct timespec current;

    if (timespec_get(&current, TIME_UTC) != TIME_UTC) {
        return -1;
    }

    *value =
        (uint64_t)current.tv_sec * UINT64_C(1000000000) +
        (uint64_t)current.tv_nsec;

    return 0;
#endif
}

static int component_is_valid(hbs_component_id_t component)
{
    return component >= HBS_COMPONENT_MESSAGE_HASH &&
           component < HBS_COMPONENT_COUNT;
}

static void set_error_once(int error_code)
{
    if (g_error_code == HBS_INSTRUMENTATION_OK) {
        g_error_code = error_code;
    }
}

int hbs_instrumentation_init(void)
{
#if defined(_WIN32)
    if (!QueryPerformanceFrequency(&g_qpc_frequency) ||
        g_qpc_frequency.QuadPart <= 0) {

        g_initialized = 0;
        return -1;
    }
#endif

    g_initialized = 1;
    g_collecting = 0;
    g_component_depth = 0U;
    g_error_code = HBS_INSTRUMENTATION_OK;
    return 0;
}

void counters_reset(void)
{
    memset(&g_hbs_counters, 0, sizeof(g_hbs_counters));
}

void hbs_instrumentation_start(void)
{
    const int initialization_failed =
        !g_initialized && hbs_instrumentation_init() != 0;

    memset(g_component_stats, 0, sizeof(g_component_stats));
    memset(g_component_stack, 0, sizeof(g_component_stack));
    counters_reset();

    g_component_depth = 0U;
    g_unscoped_hash_calls = 0U;
    g_error_code = HBS_INSTRUMENTATION_OK;
    g_collecting = 1;

    if (initialization_failed) {
        set_error_once(HBS_INSTRUMENTATION_ERR_CLOCK);
    }
}

int hbs_component_begin(hbs_component_id_t component)
{
    uint64_t now;

    if (!g_collecting) {
        return 0;
    }

    if (!component_is_valid(component)) {
        set_error_once(HBS_INSTRUMENTATION_ERR_COMPONENT);
        return -1;
    }

    if (g_component_depth >= HBS_COMPONENT_STACK_MAX) {
        set_error_once(HBS_INSTRUMENTATION_ERR_STACK_OVERFLOW);
        return -1;
    }

    if (instrumentation_now_ns(&now) != 0) {
        set_error_once(HBS_INSTRUMENTATION_ERR_CLOCK);
        return -1;
    }

    if (g_component_depth > 0U) {
        hbs_component_frame_t *parent =
            &g_component_stack[g_component_depth - 1U];

        g_component_stats[parent->component].elapsed_ns +=
            now - parent->resumed_at_ns;
    }

    g_component_stats[component].calls++;
    g_component_stack[g_component_depth].component = component;
    g_component_stack[g_component_depth].resumed_at_ns = now;
    g_component_depth++;

    return 0;
}

int hbs_component_end(hbs_component_id_t component)
{
    hbs_component_frame_t *current;
    uint64_t now;

    if (!g_collecting) {
        return 0;
    }

    if (!component_is_valid(component) ||
        g_component_depth == 0U ||
        g_component_stack[g_component_depth - 1U].component != component) {

        set_error_once(HBS_INSTRUMENTATION_ERR_STACK_MISMATCH);
        return -1;
    }

    if (instrumentation_now_ns(&now) != 0) {
        set_error_once(HBS_INSTRUMENTATION_ERR_CLOCK);
        return -1;
    }

    current = &g_component_stack[g_component_depth - 1U];
    g_component_stats[current->component].elapsed_ns +=
        now - current->resumed_at_ns;

    g_component_depth--;

    if (g_component_depth > 0U) {
        g_component_stack[g_component_depth - 1U].resumed_at_ns = now;
    }

    return 0;
}

void hbs_note_hash_calls(uint64_t count)
{
    if (!g_collecting || count == 0U) {
        return;
    }

    if (g_component_depth == 0U) {
        g_unscoped_hash_calls += count;
        return;
    }

    g_component_stats[
        g_component_stack[g_component_depth - 1U].component
    ].hash_calls += count;
}

int hbs_instrumentation_stop(hbs_instrumentation_snapshot_t *snapshot)
{
    if (snapshot == NULL) {
        set_error_once(HBS_INSTRUMENTATION_ERR_NULL_SNAPSHOT);
        g_collecting = 0;
        g_component_depth = 0U;
        return -1;
    }

    if (g_component_depth != 0U) {
        set_error_once(HBS_INSTRUMENTATION_ERR_UNCLOSED_SCOPE);
    }

    memset(snapshot, 0, sizeof(*snapshot));
    memcpy(snapshot->component,
           g_component_stats,
           sizeof(g_component_stats));
    snapshot->counters = g_hbs_counters;
    snapshot->unscoped_hash_calls = g_unscoped_hash_calls;
    snapshot->error_code = g_error_code;

    g_collecting = 0;
    g_component_depth = 0U;

    return snapshot->error_code == HBS_INSTRUMENTATION_OK ? 0 : -1;
}

const char *hbs_component_name(hbs_component_id_t component)
{
    static const char *const names[HBS_COMPONENT_COUNT] = {
        "MESSAGE_HASH",
        "WOTS_SIGN",
        "LEAF_GENERATION",
        "LTREE",
        "TREEHASH",
        "AUTH_PATH",
        "BDS_UPDATE",
        "STATE_UPDATE",
        "SIGNATURE_ASSEMBLY",
        "OTHER"
    };

    if (!component_is_valid(component)) {
        return "INVALID_COMPONENT";
    }

    return names[component];
}

uint64_t hbs_snapshot_elapsed_sum_ns(
    const hbs_instrumentation_snapshot_t *snapshot)
{
    uint64_t total = 0U;
    size_t i;

    if (snapshot == NULL) {
        return 0U;
    }

    for (i = 0U; i < (size_t)HBS_COMPONENT_COUNT; i++) {
        total += snapshot->component[i].elapsed_ns;
    }

    return total;
}

uint64_t hbs_snapshot_specific_call_count(
    const hbs_instrumentation_snapshot_t *snapshot)
{
    uint64_t total = 0U;
    size_t i;

    if (snapshot == NULL) {
        return 0U;
    }

    for (i = 0U; i < (size_t)HBS_COMPONENT_OTHER; i++) {
        total += snapshot->component[i].calls;
    }

    return total;
}

void counters_print(void)
{
    printf("PRF calls: %llu\n",
           (unsigned long long)g_hbs_counters.prf_calls);
    printf("F calls: %llu\n",
           (unsigned long long)g_hbs_counters.hash_f_calls);
    printf("H calls: %llu\n",
           (unsigned long long)g_hbs_counters.hash_h_calls);
    printf("Message-hash calls: %llu\n",
           (unsigned long long)g_hbs_counters.hash_message_calls);
    printf("WOTS chain steps: %llu\n",
           (unsigned long long)g_hbs_counters.wots_chain_steps);
    printf("Tree nodes: %llu\n",
           (unsigned long long)g_hbs_counters.tree_nodes);
    printf("Authentication-path nodes: %llu\n",
           (unsigned long long)g_hbs_counters.auth_path_nodes);
    printf("BDS updates: %llu\n",
           (unsigned long long)g_hbs_counters.bds_updates);
}
