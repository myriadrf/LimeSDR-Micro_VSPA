#ifndef LIME_VSPA_IQSTREAM_H
#define LIME_VSPA_IQSTREAM_H

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
    uint32_t host_udr;
    uint32_t afe_drop;
    uint32_t dfe_drop;
};

struct VSPA_Stats {
    uint32_t busy_cycles;
    uint32_t ext_go_count;
    uint32_t go_count;
};

// static bool timeout_happened = false;
// #define WAIT_FOR(cond, timeout_cycles)     \
//     do {                                   \
//         uint32_t timeout = timeout_cycles; \
//         do {                               \
//         } while (!(cond) && --timeout);    \
//     } while (0)

// with the debugger support available, no longer need to do timeouts, to protect against core lock up.
// Debugger can force restart the firmware
#define WAIT_FOR(cond, timeout_cycles) \
    do {                               \
    } while (!(cond))

#endif // LIME_VSPA_IQSTREAM_H