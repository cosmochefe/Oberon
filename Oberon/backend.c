//
//  backend.c
//  Oberon
//
//  Created by Alvaro Costa Neto on 10/20/13.
//  Copyright (c) 2013 Alvaro Costa Neto. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

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
	if (register_index > REGISTER_INDEX_MAX)
		mark_at(error_fatal, position_zero, "Waaaaay too many registers needed. Sorry, no can do!");
}

void write_load_immediate(value_t value)
{
	fprintf(output_file, "\tLOAD R%d, %d\n", register_index, value);
	register_index++;
	if (register_index > REGISTER_INDEX_MAX)
		mark_at(error_fatal, position_zero, "Waaaaay too many registers needed. Sorry, no can do!");
}

void write_store(address_t address)
{
	register_index--;
	if (register_index < 0)
		mark_at(error_fatal, position_zero, "Negative indexes are as rare as white flies, you know?");
	fprintf(output_file, "\tSTORE [%.4X], R%d\n", address, register_index);
}

void write_binary_op(symbol_t symbol)
{
	register_index--;
	if (register_index < 1)
		mark_at(error_fatal, position_zero, "Negative indexes are as rare as white flies, you know?");
	char mnemonic[] = "NOP";
	if (symbol == symbol_plus)
		strcpy(mnemonic, "ADD");
	else if (symbol == symbol_minus)
		strcpy(mnemonic, "SUB");
	else if (symbol == symbol_times)
		strcpy(mnemonic, "MUL");
	else if (symbol == symbol_div)
		strcpy(mnemonic, "DIV");
	else if (symbol == symbol_mod)
		strcpy(mnemonic, "MOD");
	else if (symbol == symbol_or)
		strcpy(mnemonic, "OR");
	else if (symbol == symbol_and)
		strcpy(mnemonic, "AND");
	fprintf(output_file, "\t%s R%d, R%d\n", mnemonic, register_index - 1, register_index);
}

void write_negate()
{
	
}
