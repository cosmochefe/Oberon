//
//  backend.c
//  Oberon
//
//  Created by Alvaro Costa Neto on 10/20/13.
//  Copyright (c) 2013 Alvaro Costa Neto. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
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

void write_load(item_t *item)
{
	if (item->addressing == addressing_immediate)
		fprintf(output_file, "\tLOAD R%d, %d\n", register_index, item->value);
	else if (item->addressing == addressing_direct)
		fprintf(output_file, "\tLOAD R%d, [%.4X]\n", register_index, item->address);
	else
		return; // TODO: Devo verificar e apontar erro ou deixar como está?
	item->addressing = addressing_register;
	item->index = register_index;
	register_index++;
}

void write_store(item_t *dest, item_t *orig)
{
	// Se o item de origem já estiver em um registrador, a função “write_load” não mudará nada
	write_load(orig);
	fprintf(output_file, "\tSTORE [%.4X], R%d\n", dest->address, orig->index);
}

void write_unary_op(symbol_t symbol, item_t *item)
{
	if (symbol == symbol_minus) {
		// Se o item for uma constante, o próprio compilador pode fazer a conta e continuar o processo
		if (item->addressing == addressing_immediate) {
			item->value = -item->value;
			return;
		}
		write_load(item);
		fprintf(output_file, "\tNEG R%d\n", item->index);
	} else if (symbol == symbol_not) {
		if (item->addressing == addressing_immediate) {
			item->value = ~item->value;
			return;
		}
		write_load(item);
		fprintf(output_file, "\tNOT R%d\n", item->index);
	}
	// TODO: Verificar operadores unários inválidos
}

void write_binary_op(symbol_t symbol, item_t *first, item_t *second)
{
//	register_index--;
//	if (register_index < 1)
//		mark_at(error_fatal, position_zero, "Negative indexes are as rare as white flies, you know?");
//	char mnemonic[] = "NOP";
//	if (op == op_add)
//		strcpy(mnemonic, "ADD");
//	else if (op == op_sub)
//		strcpy(mnemonic, "SUB");
//	else if (op == op_mul)
//		strcpy(mnemonic, "MUL");
//	else if (op == op_div)
//		strcpy(mnemonic, "DIV");
//	else if (op == op_mod)
//		strcpy(mnemonic, "MOD");
//	else if (op == op_and)
//		strcpy(mnemonic, "AND");
//	else if (op == op_or)
//		strcpy(mnemonic, "OR");
//	fprintf(output_file, "\t%s R%d, R%d\n", mnemonic, register_index - 1, register_index);
}