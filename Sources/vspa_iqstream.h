#ifndef LIME_VSPA_IQSTREAM_H
#define LIME_VSPA_IQSTREAM_H

#define VSPA_DEFAULT_TIMEOUT 2000

typedef struct VSPA_State {
    uint32_t errno; // general error indicator is something wrong happened
    uint32_t go_count;
} vspa_state_t;

extern vspa_state_t state;

static inline void error_trap(void) {
    while (state.errno) {
        // wait for host to handle and reset error code
    }
}

static bool timeout_happened = false;
#define WAIT_TIMEOUT(cond, timeout_cycles) \
    do {                                   \
        timeout_happened = false;          \
        uint32_t timeout = timeout_cycles; \
        do {                               \
        } while (!(cond) && --timeout);    \
        timeout_happened = timeout == 0;   \
    } while (0)

#endif // LIME_VSPA_IQSTREAM_H