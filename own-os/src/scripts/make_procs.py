import argparse
from pathlib import Path

# setup arguments
parser = argparse.ArgumentParser(description="Generate process linker script and header")
parser.add_argument("-d", help="Directory contaning process code")
parser.add_argument("-k", help="Kernel linker script")
parser.add_argument("-l", help="Process linker script output")
parser.add_argument("-o", help="Process header output")
args = parser.parse_args()

# get all process files
proc_files = sorted(list(Path(args.d).glob("*.c")))

# accumulate process dictionaries with useful metadata
id = 0
processes = []
for proc_file in proc_files:
    name = proc_file.stem

    processes.append({
        # source and name
        "src": proc_file.name,
        "obj": f"{name}.o",
        "name": name,
        "index": id,

        # entry point
        "entry": f"{name}_main",

        # partition bounds
        "spart": f"__{name}_spart",
        "epart": f"__{name}_epart",

        # stack bounds
        "estack": f"__{name}_estack",

        # data bounds
        "sidata": f"__{name}_sidata",
        "sdata": f"__{name}_sdata",
        "edata": f"__{name}_edata",

        # bss bounds
        "sbss": f"__{name}_sbss",
        "ebss": f"__{name}_ebss",
    })
    id += 1
num_processes = len(processes)

# write header output
with Path(args.o).open("w") as f:
    f.write(f"""\
#ifndef PROCESS_TABLE_H
#define PROCESS_TABLE_H

#include "../../kernel/include/process.h"
#include <stdint.h>

/* external symbols */

""")

    for p in processes:
        f.write(f"""\
// {p["name"]}
extern int {p["entry"]}(void);

extern uint32_t {p["spart"]};
extern uint32_t {p["epart"]};

extern uint32_t {p["estack"]};

extern uint32_t {p["sidata"]};
extern uint32_t {p["sdata"]};
extern uint32_t {p["edata"]};

extern uint32_t {p["sbss"]};
extern uint32_t {p["ebss"]};

""")

    f.write(f"""\
/* process descriptors */

""")

    f.write(f"""\
// process descriptor table
static process process_table[{num_processes}];

#define NUM_PROCESSES {num_processes}

// initialize process descriptors
static void init_process_table(void) {{
""");

    for p in processes:
        f.write(f"""\
    // {p["name"]}
    process_table[{p["index"]}].name = "{p["name"]}";
    process_table[{p["index"]}].entry = {p["entry"]};

    process_table[{p["index"]}].spart = &{p["spart"]};
    process_table[{p["index"]}].epart = &{p["epart"]};
    
    process_table[{p["index"]}].estack = &{p["estack"]};
    
    process_table[{p["index"]}].sidata = &{p["sidata"]};
    process_table[{p["index"]}].sdata = &{p["sdata"]};
    process_table[{p["index"]}].edata = &{p["edata"]};
    
    process_table[{p["index"]}].sbss = &{p["sbss"]};
    process_table[{p["index"]}].ebss = &{p["ebss"]};

""")

    f.write(f"""\
}}

#endif
""")

# write linker script output
with Path(args.l).open("w") as f:
    f.write(f"""\
INCLUDE {args.k}

SECTIONS {{
""")

    for p in processes:
        f.write(f"""\
    _process_start = ALIGN(., _part_size);

	.{p["name"]}_stack ALIGN(_process_start, _part_size) (NOLOAD) : {{
        {p["spart"]} = .;
		. = . + _stack_size;
        {p["estack"]} = .;
	}} > RAM

	.{p["name"]}_data : {{
		. = ALIGN(4);
        {p["sdata"]} = .;
		*{p["obj"]}(.data .data.*)
		. = ALIGN(4);
        {p["edata"]} = .;
	}} > RAM AT > FLASH

    {p["sidata"]} = LOADADDR(.{p["name"]}_data);
	
	.{p["name"]}_bss : {{
		. = ALIGN(4);
        {p["sbss"]} = .;
		*{p["obj"]}(.bss .bss.*)
		. = ALIGN(4);
        {p["ebss"]} = .;
	}} > RAM

	.{p["name"]}_pad (NOLOAD) : {{
		. = {p["spart"]} + _part_size;
        {p["epart"]} = .;
	}} > RAM

""")

    f.write(f"""\
}}
""")
