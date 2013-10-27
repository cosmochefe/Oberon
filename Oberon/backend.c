//
//  backend.c
//  Oberon
//
//  Created by Alvaro Costa Neto on 10/20/13.
//  Copyright (c) 2013 Alvaro Costa Neto. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>

#include "backend.h"
#include "errors.h"
#include "symbol_table.h"

FILE *output_file;

#define REGISTER_INDEX_MAX 31

unsigned char register_index = 0;

void initialize_backend(FILE *file)
{
	output_file = file;
}

void write_load(address_t address)
{
	fprintf(output_file, "\tLOAD R%d, [%.4X]\n", register_index, address);
	register_index++;
	if (register_index > REGISTER_INDEX_MAX) {
		mark_at(error_fatal, position_zero, "Waaaaay too many registers needed. Sorry, no can do!");
		exit(EXIT_FAILURE);
	}
}

void write_load_immediate(value_t value)
{
	fprintf(output_file, "\tLOAD R%d, %d\n", register_index, value);
	register_index++;
	if (register_index > REGISTER_INDEX_MAX) {
		mark_at(error_fatal, position_zero, "Waaaaay too many registers needed. Sorry, no can do!");
		exit(EXIT_FAILURE);
	}
}

void write_store(address_t address)
{
	
}

void write_sum()
{
	
}

void write_negate()
{
	
}

void write_subract()
{
	
}

void write_multiply()
{
	
}

void write_divide()
{
	
}

void write_logic_xor()
{
	
}

void write_logic_or()
{
	
}

void write_logic_and()
{
	
}

void write_logic_not()
{
	
}
