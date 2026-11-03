#ifndef PROCESS_TABLE_H
#define PROCESS_TABLE_H

#include "../../kernel/include/process.h"
#include <stdint.h>

/* external symbols */

// idle
extern int idle_main(void);

extern uint32_t __idle_spart;
extern uint32_t __idle_epart;

extern uint32_t __idle_estack;

extern uint32_t __idle_sidata;
extern uint32_t __idle_sdata;
extern uint32_t __idle_edata;

extern uint32_t __idle_sbss;
extern uint32_t __idle_ebss;

// process_a
extern int process_a_main(void);

extern uint32_t __process_a_spart;
extern uint32_t __process_a_epart;

extern uint32_t __process_a_estack;

extern uint32_t __process_a_sidata;
extern uint32_t __process_a_sdata;
extern uint32_t __process_a_edata;

extern uint32_t __process_a_sbss;
extern uint32_t __process_a_ebss;

// process_b
extern int process_b_main(void);

extern uint32_t __process_b_spart;
extern uint32_t __process_b_epart;

extern uint32_t __process_b_estack;

extern uint32_t __process_b_sidata;
extern uint32_t __process_b_sdata;
extern uint32_t __process_b_edata;

extern uint32_t __process_b_sbss;
extern uint32_t __process_b_ebss;

/* process descriptors */

#define NUM_PROCESSES 3

// initialize process descriptors
static void init_process_table(process* process_table) {
    // idle
    process_table[0].name = "idle";
    process_table[0].entry = idle_main;
    process_table[0].prio = PRIO_IDLE;

    process_table[0].spart = (uintptr_t)&__idle_spart;
    process_table[0].epart = (uintptr_t)&__idle_epart;
    
    process_table[0].estack = (uintptr_t)&__idle_estack;
    
    process_table[0].sidata = (uintptr_t)&__idle_sidata;
    process_table[0].sdata = (uintptr_t)&__idle_sdata;
    process_table[0].edata = (uintptr_t)&__idle_edata;
    
    process_table[0].sbss = (uintptr_t)&__idle_sbss;
    process_table[0].ebss = (uintptr_t)&__idle_ebss;

    // process_a
    process_table[1].name = "process_a";
    process_table[1].entry = process_a_main;
    process_table[1].prio = PRIO_LOW;

    process_table[1].spart = (uintptr_t)&__process_a_spart;
    process_table[1].epart = (uintptr_t)&__process_a_epart;
    
    process_table[1].estack = (uintptr_t)&__process_a_estack;
    
    process_table[1].sidata = (uintptr_t)&__process_a_sidata;
    process_table[1].sdata = (uintptr_t)&__process_a_sdata;
    process_table[1].edata = (uintptr_t)&__process_a_edata;
    
    process_table[1].sbss = (uintptr_t)&__process_a_sbss;
    process_table[1].ebss = (uintptr_t)&__process_a_ebss;

    // process_b
    process_table[2].name = "process_b";
    process_table[2].entry = process_b_main;
    process_table[2].prio = PRIO_HIGH;

    process_table[2].spart = (uintptr_t)&__process_b_spart;
    process_table[2].epart = (uintptr_t)&__process_b_epart;
    
    process_table[2].estack = (uintptr_t)&__process_b_estack;
    
    process_table[2].sidata = (uintptr_t)&__process_b_sidata;
    process_table[2].sdata = (uintptr_t)&__process_b_sdata;
    process_table[2].edata = (uintptr_t)&__process_b_edata;
    
    process_table[2].sbss = (uintptr_t)&__process_b_sbss;
    process_table[2].ebss = (uintptr_t)&__process_b_ebss;

}

#endif
