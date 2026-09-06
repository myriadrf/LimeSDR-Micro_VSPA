#ifndef LIME_VSPA_IQSTREAM_H
#define LIME_VSPA_IQSTREAM_H

#define VSPA_DEFAULT_TIMEOUT 2000

typedef struct VSPA_State {
    uint32_t errno; // general error indicator is something wrong happened
    uint32_t go_count;
} vspa_state_t;

struct PipeStats {
    uint32_t afe_enq;
    uint32_t afe_compl;
    uint32_t afe_err;
    uint32_t afe_udr;
    uint32_t afe_ovr;
    uint32_t dfe_enq;
    uint32_t dfe_compl;
    uint32_t dfe_err;
    uint32_t dfe_udr;
    uint32_t dfe_ovr;
};

extern vspa_state_t state;

static inline void error_trap(void) {
    while (state.errno) {
        // wait for host to handle and reset error code
    }
}

static bool timeout_happened = false;
#define WAIT_FOR(cond, timeout_cycles)     \
    do {                                   \
        timeout_happened = false;          \
        uint32_t timeout = timeout_cycles; \
        do {                               \
        } while (!(cond) && --timeout);    \
        timeout_happened = timeout == 0;   \
    } while (0)

#endif // LIME_VSPA_IQSTREAM_H