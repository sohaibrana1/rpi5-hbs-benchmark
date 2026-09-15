#ifndef INSTRUMENTATION_H
#define INSTRUMENTATION_H

#include <stddef.h>
#include <stdint.h>

/*
 * Single-threaded, nested, exclusive component instrumentation for the
 * XMSS/XMSSMT reference code. A parent component is paused while a child
 * component is active, so component elapsed times can be summed without
 * double-counting nested calls.
 */

typedef enum {
    HBS_COMPONENT_MESSAGE_HASH = 0,
    HBS_COMPONENT_WOTS_SIGN,
    HBS_COMPONENT_LEAF_GENERATION,
    HBS_COMPONENT_LTREE,
    HBS_COMPONENT_TREEHASH,
    HBS_COMPONENT_AUTH_PATH,
    HBS_COMPONENT_BDS_UPDATE,
    HBS_COMPONENT_STATE_UPDATE,
    HBS_COMPONENT_SIGNATURE_ASSEMBLY,
    HBS_COMPONENT_OTHER,
    HBS_COMPONENT_COUNT
} hbs_component_id_t;

typedef struct {
    uint64_t elapsed_ns;
    uint64_t calls;
    uint64_t hash_calls;
} hbs_component_stat_t;

typedef struct {
    uint64_t prf_calls;
    uint64_t hash_f_calls;
    uint64_t hash_h_calls;
    uint64_t hash_message_calls;
    uint64_t wots_chain_steps;
    uint64_t tree_nodes;
    uint64_t auth_path_nodes;
    uint64_t bds_updates;
} hbs_counters_t;

typedef struct {
    hbs_component_stat_t component[HBS_COMPONENT_COUNT];
    hbs_counters_t counters;
    uint64_t unscoped_hash_calls;
    int error_code;
} hbs_instrumentation_snapshot_t;

extern hbs_counters_t g_hbs_counters;

/* Call once before the first measured operation. */
int hbs_instrumentation_init(void);

/* Start/stop one measured operation. Timers are inactive outside this pair. */
void hbs_instrumentation_start(void);
int hbs_instrumentation_stop(hbs_instrumentation_snapshot_t *snapshot);

/* Component boundaries used inside the XMSS implementation. */
int hbs_component_begin(hbs_component_id_t component);
int hbs_component_end(hbs_component_id_t component);

/* Attribute primitive hash work to the currently active component. */
void hbs_note_hash_calls(uint64_t count);

/* Snapshot helpers used by the component benchmark runner. */
const char *hbs_component_name(hbs_component_id_t component);
uint64_t hbs_snapshot_elapsed_sum_ns(
    const hbs_instrumentation_snapshot_t *snapshot);
uint64_t hbs_snapshot_specific_call_count(
    const hbs_instrumentation_snapshot_t *snapshot);

/* Backward-compatible counter helpers. */
void counters_reset(void);
void counters_print(void);

#define HBS_COMPONENT_BEGIN(component_) \
    ((void)hbs_component_begin((component_)))

#define HBS_COMPONENT_END(component_) \
    ((void)hbs_component_end((component_)))

#define HBS_NOTE_HASH_CALLS(count_) \
    hbs_note_hash_calls((uint64_t)(count_))

#define HBS_COUNT_PRF() \
    do { \
        g_hbs_counters.prf_calls++; \
        HBS_NOTE_HASH_CALLS(1U); \
    } while (0)

#define HBS_COUNT_HASH_F() \
    do { \
        g_hbs_counters.hash_f_calls++; \
        HBS_NOTE_HASH_CALLS(1U); \
    } while (0)

#define HBS_COUNT_HASH_H() \
    do { \
        g_hbs_counters.hash_h_calls++; \
        HBS_NOTE_HASH_CALLS(1U); \
    } while (0)

#define HBS_COUNT_HASH_MESSAGE() \
    do { \
        g_hbs_counters.hash_message_calls++; \
        HBS_NOTE_HASH_CALLS(1U); \
    } while (0)

#define HBS_COUNT_WOTS_CHAIN_STEP() \
    do { g_hbs_counters.wots_chain_steps++; } while (0)

#define HBS_COUNT_TREE_NODE() \
    do { g_hbs_counters.tree_nodes++; } while (0)

#define HBS_COUNT_AUTH_PATH_NODE() \
    do { g_hbs_counters.auth_path_nodes++; } while (0)

#define HBS_COUNT_BDS_UPDATE() \
    do { g_hbs_counters.bds_updates++; } while (0)

#endif
