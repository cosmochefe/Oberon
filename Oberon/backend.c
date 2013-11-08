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

#define REGISTER_INDEX_COUNT 32

FILE *output_file = NULL;
unsigned char register_index = 0;
address_t program_counter = 0;

void initialize_backend(FILE *file)
{
	output_file = file;
}

void inc_index(unsigned char amount)
{
	register_index = (register_index + amount) % REGISTER_INDEX_COUNT;
}

void dec_index(unsigned char amount)
{
	register_index = (register_index - amount) % REGISTER_INDEX_COUNT;
}

void output(const char *comments, const char *instruction, ...)
{
	va_list args;
	va_start(args, instruction);
	vfprintf(output_file, instruction, args);
	va_end(args);
	fprintf(output_file, "\t");
	fprintf(output_file, "%s", comments);
	fprintf(output_file, "\n");
	program_counter++;
}

void write_load(item_t *item)
{
	if (!item)
		return;
	if (item->addressing == addressing_immediate)
		output(NULL, "LOAD R%d, %d", register_index, item->value);
	else if (item->addressing == addressing_direct)
		output(NULL, "LOAD R%d, [%.4X]", register_index, item->address);
	else if (item->addressing == addressing_indirect) {
		output(NULL, "LOAD R%d, [R%d]", item->index, item->index);
		item->addressing = addressing_register;
		return;
	}
	else
		return; // TODO: Devo verificar e apontar erro ou deixar como está?
	item->addressing = addressing_register;	
	item->index = register_index;
	inc_index(1);
}

void write_store(item_t *dst_item, item_t *src_item)
{
	if (!dst_item || !src_item)
		return;
	// Se o item de origem já estiver em um registrador, a função “write_load” não mudará nada
	write_load(src_item);
	output(NULL, "STORE [%.4X], R%d", dst_item->address, src_item->index);
	dst_item->addressing = addressing_register;
	dst_item->index = src_item->index;
	// TODO: Se for reaproveitar o destino para as próximas contas, é necessário reduzir o índice de registradores?
	dec_index(1);
}

void write_index_offset(item_t *item, item_t *index_item)
{
  // TODO: Adicionar rotina “trap” para índices fora do limite
	write_load(index_item);
	output(NULL, "MUL R%d, %d", index_item->index, item->type->base->size);
	if (item->addressing == addressing_direct) {
		output(NULL, "ADD R%d, %d", index_item->index, item->address);
		item->index = index_item->index;
		item->addressing = addressing_indirect;
	}
	else if (item->addressing == addressing_indirect) {
		output(NULL, "ADD R%d, R%d", item->index, index_item->index);
		dec_index(1);
	}
}

void write_field_offset(item_t *item, address_t offset)
{
	output(NULL, "ADD R%d, %d", item->index, offset);
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
		output(NULL, "NEG R%d", item->index);
	} else if (symbol == symbol_not) {
		if (item->addressing == addressing_immediate) {
			item->value = ~item->value;
			return;
		}
		write_load(item);
		output(NULL, "NOT R%d", item->index);
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
				output(NULL, "%s R%d, R%d", opcode, item->index, rhs_item->index);
				// É preciso mover o resultado para o registrador de menor índice (neste caso, o do segundo operando) para que
				// o índice de registradores em uso possa ser reduzido, evitando que a quantidade disponível de registradores
				// esgote-se
				output(NULL, "MOV R%d, R%d", rhs_item->index, item->index);
				item->index = rhs_item->index;
				dec_index(1);
				return;
			}
			output(NULL, "%s R%d, %d", opcode, rhs_item->index, item->value);
			item->addressing = addressing_register;
			item->index = rhs_item->index;
		} else if (rhs_item->addressing == addressing_immediate) {
			write_load(item);
			output(NULL, "%s R%d, %d", opcode, item->index, rhs_item->value);
		} else {
			write_load(item);
			write_load(rhs_item);
			output(NULL, "%s R%d, R%d", opcode, item->index, rhs_item->index);
			if (item->index > rhs_item->index) {
				output(NULL, "MOV R%d, R%d", rhs_item->index, item->index);
				item->index = rhs_item->index;
			}
			dec_index(1);
		}
	}
}

void write_comparison(symbol_t symbol, item_t *item, item_t *rhs_item)
{
	write_load(item);
	write_load(rhs_item);
	output(NULL, "CMP R%d, R%d", item->index, rhs_item->index);
	item->addressing = addressing_condition;
	item->condition = symbol;
	// É necessário liberar ambos os registradores após a comparação. Ou não?
	dec_index(2);
}

void write_branch(item_t *item, bool forward)
{
	if (!item) return;
	char condition_mnemonic[] = "CH";
	switch (item->condition) {
		case symbol_equal: strcpy(condition_mnemonic, "EQ"); break;
		case symbol_not_equal: strcpy(condition_mnemonic, "NE"); break;
		case symbol_less: strcpy(condition_mnemonic, "LS"); break;
		case symbol_less_equal: strcpy(condition_mnemonic, "LE"); break;
		case symbol_greater: strcpy(condition_mnemonic, "GR"); break;
		case symbol_greater_equal: strcpy(condition_mnemonic, "GE"); break;
		default: break;
	}
	fgetpos(output_file, &item->link);
	output(NULL, "BR%s", condition_mnemonic);
}

void write_inverse_branch(item_t *item, bool forward)
{
	if (!item) return;
	item->condition = inverse_condition(item->condition);
	write_branch(item, forward);
}

void write_label(item_t *item)
{
	output(NULL, "LBL_%d:", program_counter);
}

void write_fixup(item_t *item)
{
  output(NULL, "LBL_%d:", program_counter);
}