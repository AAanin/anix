#ifndef ANIX_H
#define ANIX_H
#include <stddef.h>
#include <stdint.h>
/* Wire integers are little endian; magic is the four ASCII bytes ANIX.
 * Header 68 bytes, input contracts 8 bytes each, nodes 32 bytes each.
 * All offsets are relative to payload start. Never cast wire bytes to structs. */
#define ANIX_HEADER_SIZE 68u
#define ANIX_NODE_SIZE 32u
#define ANIX_MAX_NODES 4096u
#define ANIX_MAX_INPUTS 256u
typedef enum { ANIX_OK=0, ANIX_ARGUMENT, ANIX_FORMAT, ANIX_CHECKSUM,
 ANIX_BOUNDS, ANIX_CONTRACT, ANIX_PLATFORM } anix_status_t;
/* A decision: kind=1, input index, threshold, lower, upper, yes, no, shadow.
 * An action: kind=0, action ID, six zero words. Edges strictly advance. */
anix_status_t anix_validate(const uint8_t *, size_t);
anix_status_t anix_execute(const uint8_t *, size_t, const uint32_t *, size_t, uint32_t *);
typedef void (*anix_lock_fn)(void *);
typedef struct { const uint8_t *blob; size_t len; } anix_bank_t;
typedef struct { anix_bank_t banks[2]; unsigned active;
 anix_lock_fn enter, leave; void *context; } anix_engine_t;
/* Hooks must serialize swaps AND readers, with compiler/CPU memory barriers.
 * Reader holds the lock through execution. Blob lifetime and immutability
 * are caller obligations. Hooks must be bounded for real-time use. */
anix_status_t anix_engine_init(anix_engine_t *, const uint8_t *, size_t,
 anix_lock_fn, anix_lock_fn, void *);
anix_status_t anix_hot_swap_bank(anix_engine_t *, const uint8_t *, size_t);
anix_status_t anix_engine_execute(anix_engine_t *, const uint32_t *, size_t, uint32_t *);
#endif
