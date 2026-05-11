#include "pipelines.h"

void stage_setup(stage_t *s, struct MemoryFIFO *inputpool, struct MemoryFIFO *outputpool) {
    s->input.fifo = inputpool;
    s->output.fifo = outputpool;
    s->input.bytes_done = 0;
    s->output.bytes_done = 0;
}
