#include <stdio.h>
#include <stdlib.h>

#define VCD_FILE "trace.vcd"
#define VCD_TIME_SCALE "1ns"

typedef struct {
    FILE *file;
    uint64_t time;
} VCD_Trace;

void vcd_init(VCD_Trace *trace) {
    trace->file = fopen(VCD_FILE, "w");
    if (!trace->file) return;
    fprintf(trace->file, "$date\n  %s\n$end\n", __DATE__);
    fprintf(trace->file, "$version\n  STM32 Simulator\n$end\n");
    fprintf(trace->file, "$timescale %s $end\n", VCD_TIME_SCALE);
    fprintf(trace->file, "$scope module simulator $end\n");
    fprintf(trace->file, "$var wire 32 CNT cnt $end\n");
    fprintf(trace->file, "$var wire 1 UIF uif $end\n");
    fprintf(trace->file, "$var wire 1 UIE uie $end\n");
    fprintf(trace->file, "$var wire 1 PENDING pending $end\n");
    fprintf(trace->file, "$var wire 1 IN_ISR in_isr $end\n");
    fprintf(trace->file, "$upscope $end\n");
    fprintf(trace->file, "$enddefinitions $end\n");
    trace->time = 0;
}

void vcd_dump(VCD_Trace *trace, Simulator *sim) {
    if (!trace->file) return;
    fprintf(trace->file, "#%llu\n", trace->time);
    fprintf(trace->file, "b%d CNT\n", sim->tim6.regs.cnt);
    fprintf(trace->file, "b%d UIF\n", (sim->tim6.regs.sr & TIM6_SR_UIF) ? 1 : 0);
    fprintf(trace->file, "b%d UIE\n", (sim->tim6.regs.dier & TIM6_DIER_UIE) ? 1 : 0);
    fprintf(trace->file, "b%d PENDING\n", nvic_is_pending(&sim->nvic, NVIC_IRQ_TIM6));
    fprintf(trace->file, "b%d IN_ISR\n", sim->cpu_in_isr);
    trace->time++;
}

void vcd_close(VCD_Trace *trace) {
    if (trace->file) fclose(trace->file);
}
