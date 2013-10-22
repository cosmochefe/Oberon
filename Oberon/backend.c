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

void load(address_t address)
{
	fprintf(output_file, "\tLOAD R%d, [%.4X]\n", register_index, address);
	register_index++;
	if (register_index > REGISTER_INDEX_MAX) {
		mark_at(error_fatal, last_token.position, "Waaaaay too many registers needed. Sorry, no can do!");
		exit(EXIT_FAILURE);
	}
}

void load_immediate(value_t value)
{
	fprintf(output_file, "\tLOAD R%d, %d\n", register_index, value);
	register_index++;
	if (register_index > REGISTER_INDEX_MAX) {
		mark_at(error_fatal, last_token.position, "Waaaaay too many registers needed. Sorry, no can do!");
		exit(EXIT_FAILURE);
	}
}

void store(address_t address)
{
	
}

void backend_sum()
{
	
}

void backend_negate()
{
	
}

void backend_subract()
{
	
}

void backend_multiply()
{
	
}

void backend_divide()
{
	
}

void backend_logic_xor()
{
	
}

void backend_logic_or()
{
	
}

void backend_logic_and()
{
	
}

void backend_logic_not()
{
	
}
