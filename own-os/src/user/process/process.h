#ifndef PROCESS_TABLE_H
#define PROCESS_TABLE_H

#include "../../kernel/include/process.h"
#include <stdint.h>

/* external symbols */

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

// process descriptor table
static process process_table[2];

#define NUM_PROCESSES 2

// initialize process descriptors
static void init_process_table(void) {
    // process_a
    process_table[0].name = "process_a";
    process_table[0].entry = process_a_main;

    process_table[0].spart = &__process_a_spart;
    process_table[0].epart = &__process_a_epart;
    
    process_table[0].estack = &__process_a_estack;
    
    process_table[0].sidata = &__process_a_sidata;
    process_table[0].sdata = &__process_a_sdata;
    process_table[0].edata = &__process_a_edata;
    
    process_table[0].sbss = &__process_a_sbss;
    process_table[0].ebss = &__process_a_ebss;

    // process_b
    process_table[1].name = "process_b";
    process_table[1].entry = process_b_main;

    process_table[1].spart = &__process_b_spart;
    process_table[1].epart = &__process_b_epart;
    
    process_table[1].estack = &__process_b_estack;
    
    process_table[1].sidata = &__process_b_sidata;
    process_table[1].sdata = &__process_b_sdata;
    process_table[1].edata = &__process_b_edata;
    
    process_table[1].sbss = &__process_b_sbss;
    process_table[1].ebss = &__process_b_ebss;

}

#endif
