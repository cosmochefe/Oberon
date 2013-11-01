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
#include <stdbool.h>
#include <strings.h>

#include "backend.h"
#include "errors.h"
#include "symbol_table.h"

#define REGISTER_INDEX_MAX 31

FILE *output_file = NULL;
unsigned char register_index = 0;

void initialize_backend(FILE *file)
{
	output_file = file;
}

void write_load(item_t *item)
{
	if (!item)
		return;
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
	if (!dest || !orig)
		return;
	// Se o item de origem já estiver em um registrador, a função “write_load” não mudará nada
	write_load(orig);
	fprintf(output_file, "\tSTORE [%.4X], R%d\n", dest->address, orig->index);
	dest->addressing = addressing_register;
	dest->index = orig->index;
	// TODO: Se for reaproveitar o destino para as próximas contas, é necessário reduzir o índice de registradores?
	register_index--;
}

void write_unary_op(symbol_t symbol, item_t *item)
{
	if (!item)
		return;
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

void write_binary_op(symbol_t symbol, item_t *item, item_t *rhs_item)
{
	if (!item || !rhs_item)
		return;
	if (item->addressing == addressing_immediate && rhs_item->addressing == addressing_immediate) {
		if (symbol == symbol_plus)
			item->value = item->value + rhs_item->value;
		else if (symbol == symbol_minus)
			item->value = item->value - rhs_item->value;
		else if (symbol == symbol_times)
			item->value = item->value * rhs_item->value;
		else if (symbol == symbol_div)
			item->value = item->value / rhs_item->value;
		else if (symbol == symbol_mod)
			item->value = item->value % rhs_item->value;
		else if (symbol == symbol_and)
			item->value = item->value & rhs_item->value;
		else if (symbol == symbol_or)
			item->value = item->value | rhs_item->value;
		// TODO: Verificar operadores binários inválidos
		return;
	} else {
		char opcode[] = "NOP";
		bool keep_order = false;
		if (symbol == symbol_plus) {
			strcpy(opcode, "ADD");
		} else if (symbol == symbol_minus) {
			strcpy(opcode, "SUB");
			keep_order = true;
		} else if (symbol == symbol_times) {
			strcpy(opcode, "MUL");
		} else if (symbol == symbol_div) {
			strcpy(opcode, "DIV");
			keep_order = true;
		} else if (symbol == symbol_mod) {
			strcpy(opcode, "MOD");
			keep_order = true;
		} else if (symbol == symbol_and) {
			strcpy(opcode, "AND");
		} else if (symbol == symbol_or) {
			strcpy(opcode, "OR");
		}
		if (item->addressing == addressing_immediate) {
			write_load(rhs_item);
			// Para algumas operações a ordem dos operandos é significante e deve ser respeitada
			if (keep_order) {
				// TODO: Trocar a ordem dos índices para remover a instrução “MOV”
				write_load(item);
				fprintf(output_file, "\t%s R%d, R%d\n", opcode, item->index, rhs_item->index);
				// É necessário mover o resultado para o registrador de menor índice (neste caso, o do segundo operando) para que
				// o índice de registradores em uso possa ser reduzido, evitando que a quantidade disponível de registradores
				// esgote-se
				fprintf(output_file, "\tMOV R%d, R%d\n", rhs_item->index, item->index);
				item->index = rhs_item->index;
				register_index--;
				return;
			}
			fprintf(output_file, "\t%s R%d, %d\n", opcode, rhs_item->index, item->value);
			item->addressing = addressing_register;
			item->index = rhs_item->index;
		} else if (rhs_item->addressing == addressing_immediate) {
			write_load(item);
			fprintf(output_file, "\t%s R%d, %d\n", opcode, item->index, rhs_item->value);
		} else {
			write_load(item);
			write_load(rhs_item);
			fprintf(output_file, "\t%s R%d, R%d\n", opcode, item->index, rhs_item->index);
			register_index--;
		}
	}
}