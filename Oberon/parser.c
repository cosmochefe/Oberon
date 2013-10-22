//
//  parser.c
//  Oberon
//
//  Created by Alvaro Costa Neto on 10/12/13.
//  Copyright (c) 2013 Alvaro Costa Neto. All rights reserved.
//

//
// Regras para produção de código para o compilador (não-terminal K):
//
//	- "x": if (s == 'x') next(); else error();
//	- (exp): evaluate(exp);
//	- [exp]: if (set_includes(s, first_set(exp)) evaluate(exp);
//	- {exp}: while (set_includes(s, first_set(exp)) evaluate(exp);
//	- f0 f1 … fn: evaluate(f0); evaluate(f1); … evaluate(fn);
//	- t0 | t1 | … | tn: if (set_includes(s, first_set(t0)) evaluate(t0);
//											else if (set_includes(s, first_set(t1)) evaluate(t1);
//											…
//											else if (set_includes(s, first_set(tn)) evaluate(tn);
//
// Condições que devem ser respeitadas para manter o determinismo da gramática (não-terminal K):
//
//	- t0 | t1: set_union(first_set(t0), first_set(t1)) == EMPTY_SET;
//	- f0 f1: if (set_includes(EMPTY, first_set(f0)) set_union(first_set(t0), first_set(t1)) == EMPTY_SET;
//	- [exp] ou {exp}: set_union(first_set(exp), follow_set(K)) == EMPTY_SET;
//
// EBNF da linguagem Oberon-0 (para “letter” e “digit”, usar as funções “is_letter” e “is_digit”):
//
//	- id = letter {letter | digit}
//	- integer = digit {digit}
//	- number = integer
//	- selector = {"." id | "[" expr "]"}
//	- factor = id selector | number | "(" expr ")" | "~" factor
//	- term = factor {("*" | "div" | "mod" | "&") factor}
//	- simple_expr = ["+" | "-"] term {("+" | "-" | "OR") term}
//	- expr = simple_expr [("=" | "#" | "<" | "<=" | ">" | ">=") simple_expr]
//	- assignment = ident selector ":=" expr
//	- actual_params = "(" [expr {"," expr}] ")"
//	- proc_call = ident selector [actual_params]
//	- if_stmt = "if" expr "then" stmt_sequence {"elsif" expr "then" stmt_sequence} ["else" stmt_sequence] "end"
//	- while_stmt = "while" expr "do" stmt_sequence "end"
//	- repeat_stmt = "repeat" stmt_sequence "until" expr
//	- stmt = [assignment | proc_call | if_stmt | while_stmt | repeat_stmt]
//	- stmt_sequence = stmt {";" stmt}
//	- id_list = id {"," id}
//	- array_type = "array" expr "of" type
//	- field_list = [id_list ":" type]
//	- record_type = "record" field_list {";" field_list} "end"
//	- type = id | array_type | record_type
//	- formal_params_section = ["var"] id_list ":" type
//	- formal_params = "(" [formal_params_section {";" formal_params_section}] ")"
//	- proc_head = "procedure" id [formal_params]
//	- proc_body = declarations ["begin" stmt_sequence] "end" id
//	- proc_decl = proc_head ";" proc_body
//	- declarations = ["const" {id "=" expr ";"}]["type" {id "=" type ";"}]["var" {id_list ":" type ";"}]{proc_decl ";"}
//	- module = "module" id ";" declarations ["begin" stmt_sequence] "end" id "."
//

#include <stdbool.h>

#include "backend.h"
#include "errors.h"
#include "scanner.h"
#include "symbol_table.h"
#include "parser.h"

// Registro de ocorrências
bool should_log;

bool scan()
{
	if (current_token.lexem.symbol == symbol_eof)
		return false;
	do {
		read_token();
	} while (current_token.lexem.symbol == symbol_null);
	return current_token.lexem.symbol == symbol_eof;
}

// Se “symbol_null” for passado como parâmetro, qualquer símbolo será reconhecido
bool assert(symbol_t symbol)
{
	if (current_token.lexem.symbol == symbol || symbol == symbol_null) {
		if (should_log)
			mark(error_log, current_token.position, "\"%s\" found.", current_token.lexem.id);
		scan();
		return true;
	}
	if (symbol == symbol_id)
		mark(error_parser, current_token.position, "Missing identifier.");
	else if (symbol == symbol_number)
		mark(error_parser, current_token.position, "Missing number.");
	else
		mark(error_parser, current_token.position, "Missing \"%s\".", id_for_symbol(symbol));
	return false;
}

void expr();

// selector = {"." id | "[" expr "]"}
void selector(entry_t **entry)
{
	while (is_first("selector", current_token.lexem.symbol)) {
		if (current_token.lexem.symbol == symbol_period) {
			assert(symbol_period);
			// FAZER: Verificar os campos de entry, caso ele seja um registro. Caso contrário, erro!
			assert(symbol_id);
		} else {
			// FAZER: Verificar se entry é um vetor
			assert(symbol_open_bracket);
			expr();
			assert(symbol_close_bracket);
		}
	}
}

// factor = id selector | number | "(" expr ")" | "~" factor
void factor()
{
	switch (current_token.lexem.symbol) {
		case symbol_id:
			assert(symbol_id);
			entry_t *entry = find_entry(last_token.lexem.id, symbol_table);
			selector(&entry);
			if (!entry)
				mark(error_parser, last_token.position, "\"%s\" hasn't been declared yet.", last_token.lexem.id);
			else
				load(entry->address);
			break;
		case symbol_number:
			assert(symbol_number);
			load_immediate(last_token.value);
			break;
		case symbol_open_paren:
			assert(symbol_open_paren);
			expr();
			assert(symbol_close_paren);
			break;
		case symbol_not:
			assert(symbol_not);
			factor();
			break;
		default:
			// Sincroniza
			mark(error_parser, current_token.position, "Missing factor.");
			while (!is_follow("factor", current_token.lexem.symbol) && current_token.lexem.symbol != symbol_eof)
				scan();
			break;
	}
}

// term = factor {("*" | "div" | "mod" | "&") factor}
void term()
{
	factor();
	while (current_token.lexem.symbol >= symbol_times && current_token.lexem.symbol <= symbol_and) {
		switch (current_token.lexem.symbol) {
			case symbol_times:
				break;
			case symbol_div:
				break;
			case symbol_mod:
				break;
			case symbol_and:
				break;
			default:
				break;
		}
		assert(current_token.lexem.symbol); 
		factor();
	}
}

// simple_expr = ["+" | "-"] term {("+" | "-" | "OR") term}
void simple_expr()
{
	if (current_token.lexem.symbol == symbol_plus)
		assert(symbol_plus);
	else if (current_token.lexem.symbol == symbol_minus)
		assert(symbol_minus);
	term();
	while (current_token.lexem.symbol >= symbol_plus && current_token.lexem.symbol <= symbol_or) {
		switch (current_token.lexem.symbol) {
			case symbol_plus:
				break;
			case symbol_minus:
				break;
			case symbol_or:
				break;
			default:
				break;
		}
		assert(current_token.lexem.symbol); 
		term();
	}
}

// expr = simple_expr [("=" | "#" | "<" | "<=" | ">" | ">=") simple_expr]
void expr()
{
	simple_expr();
	if (current_token.lexem.symbol >= symbol_equal && current_token.lexem.symbol <= symbol_greater) {
		switch (current_token.lexem.symbol) {
			case symbol_equal:
				break;
			case symbol_not_equal:
				break;
			case symbol_less:
				break;
			case symbol_less_equal:
				break;
			case symbol_greater:
				break;
			case symbol_greater_equal:
				break;
			default:
				break;
		}
		assert(current_token.lexem.symbol);
		simple_expr();
	}
}

// assignment = ":=" expr
void assignment(entry_t *entry)
{
	assert(symbol_becomes);
	expr();
	// FAZER: Adicionar o armazenamento do resultado
}

// actual_params = "(" [expr {"," expr}] ")"
void actual_params()
{
	assert(symbol_open_paren);
	if (is_first("expr", current_token.lexem.symbol)) {
		expr();
		while (current_token.lexem.symbol == symbol_comma) {
			assert(symbol_comma);
			expr();
		}
	}
	assert(symbol_close_paren);
}

// proc_call = [actual_params]
void proc_call()
{
	if (is_first("actual_params", current_token.lexem.symbol))
		actual_params();
}

void stmt_sequence();

// if_stmt = "if" expr "then" stmt_sequence {"elsif" expr "then" stmt_sequence} ["else" stmt_sequence] "end"
void if_stmt()
{
	assert(symbol_if);
	expr();
	assert(symbol_then);
	stmt_sequence();
	while (current_token.lexem.symbol == symbol_elsif) {
		assert(symbol_elsif);
		expr();
		assert(symbol_then);
		stmt_sequence();
	}
	if (current_token.lexem.symbol == symbol_else) {
		assert(symbol_else);
		stmt_sequence();
	}
	assert(symbol_end);
}

// while_stmt = "while" expr "do" stmt_sequence "end"
void while_stmt()
{
	assert(symbol_while);
	expr();
	assert(symbol_do);
	stmt_sequence();
	assert(symbol_end);
}

// repeat_stmt = "repeat" stmt_sequence "until" expr
void repeat_stmt()
{
	assert(symbol_repeat);
	stmt_sequence();
	assert(symbol_until);
	expr();
}

// stmt = [id selector (assignment | proc_call) | if_stmt | while_stmt | repeat_stmt]
void stmt()
{
	if (current_token.lexem.symbol == symbol_id) {
		assert(symbol_id);
		// FAZER: Adicionar geração de código e obtenção da entrada na tabela de símbolos
		selector(NULL);
		if (is_first("assignment", current_token.lexem.symbol))
			assignment(NULL);
		else if (is_first("proc_call", current_token.lexem.symbol) || is_follow("proc_call", current_token.lexem.symbol))
			proc_call();
		else {
			// Sincroniza
			mark(error_parser, current_token.position, "Invalid statement.");
			while (!is_follow("stmt", current_token.lexem.symbol) && current_token.lexem.symbol != symbol_eof)
				scan();
		}
	}	else if (is_first("if_stmt", current_token.lexem.symbol))
		if_stmt();
	else if (is_first("while_stmt", current_token.lexem.symbol))
		while_stmt();
	else if (is_first("repeat_stmt", current_token.lexem.symbol))
		repeat_stmt();
	// Sincroniza
	if (!is_follow("stmt", current_token.lexem.symbol)) {
		mark(error_parser, current_token.position, "Missing \";\" or \"end\".");
		while (!is_follow("stmt", current_token.lexem.symbol) && current_token.lexem.symbol != symbol_eof)
			scan();
	}
}

// stmt_sequence = stmt {";" stmt}
void stmt_sequence()
{
	stmt();
	while (current_token.lexem.symbol == symbol_semicolon) {
		assert(symbol_semicolon);
		stmt();
	}
}

// id_list = id {"," id}
entry_t *id_list()
{
	assert(symbol_id);
	entry_t *new_entries = create_entry(last_token.lexem.id, last_token.position, class_var);
	while (current_token.lexem.symbol == symbol_comma) {
		assert(symbol_comma);
		if (assert(symbol_id))
			append_entry(create_entry(last_token.lexem.id, last_token.position, class_var), &new_entries);
	}
	return new_entries;
}

type_t *type();

// array_type = "array" expr "of" type
type_t *array_type()
{
	assert(symbol_array);
	//	expr();
	value_t length = 0;
	if (assert(symbol_number))
		length = last_token.value;
	assert(symbol_of);
	type_t *base = type();
	unsigned int size = 0;
	if (base)
		size = length * base->size;
	return create_type(form_array, length, size, NULL, base);
}

// field_list = [id_list ":" type]
entry_t *field_list()
{
	if (is_first("id_list", current_token.lexem.symbol)) {
		entry_t *new_fields = id_list();
		assert(symbol_colon);
		type_t *base_type = type();
		entry_t *e = new_fields;
		while (e) {
			e->type = base_type;
			e = e->next;
		}
		return new_fields;
	}
	return NULL;
}

// record_type = "record" field_list {";" field_list} "end"
type_t *record_type()
{
	assert(symbol_record);
	entry_t *fields = field_list();
	while (current_token.lexem.symbol == symbol_semicolon) {
		assert(symbol_semicolon);
		entry_t *more_fields = field_list();
		if (fields && more_fields)
			append_entry(more_fields, &fields);
	}
	// Efetua o cálculo do tamanho do tipo registro e dos deslocamentos de cada campo
	unsigned int size = 0;
	address_t offset = 0;
	if (fields) {
		entry_t *e = fields;
		while (e && e->type) {
			e->address = offset;
			offset += e->type->size;
			size += e->type->size;
			e = e->next;
		}
	}
	assert(symbol_end);
	return create_type(form_record, 0, size, fields, NULL);
}

// type = id | array_type | record_type
type_t *type()
{
	if (current_token.lexem.symbol == symbol_id) {
		// Qualquer tipo atômico deve ser baseado em um dos tipos internos da linguagem (neste caso apenas “integer”)
		assert(symbol_id);
		entry_t *entry = find_entry(last_token.lexem.id, symbol_table);
		if (entry)
			return entry->type;
		else
			mark(error_parser, last_token.position, "Unknown type \"%s\".", last_token.lexem.id);
	} else if (is_first("array_type", current_token.lexem.symbol)) {
		type_t *new_type = array_type();
		if (!new_type)
			mark(error_parser, current_token.position, "Invalid array type.");
		return new_type;
	} else if (is_first("record_type", current_token.lexem.symbol)) {
		type_t *new_type = record_type();
		if (!new_type)
			mark(error_parser, current_token.position, "Invalid record type.");
		return new_type;
	}
	// Sincroniza
	mark(error_parser, current_token.position, "Missing type.");
	while (!is_follow("type", current_token.lexem.symbol) && current_token.lexem.symbol != symbol_eof)
		scan();
	return NULL;
}

// formal_params_section = ["var"] id_list ":" type
entry_t *formal_params_section()
{
	if (current_token.lexem.symbol == symbol_var)
		assert(symbol_var);
	entry_t *new_params = id_list();
	assert(symbol_colon);
	type_t *base_type = type();
	entry_t *e = new_params;
	while (e) {
		e->type = base_type;
		e = e->next;
	}
	return new_params;
}

// formal_params = "(" [formal_params_section {";" formal_params_section}] ")"
entry_t *formal_params()
{
	entry_t *params = NULL;
	assert(symbol_open_paren);
	if (is_first("formal_params_section", current_token.lexem.symbol)) {
		params = formal_params_section();
		while (current_token.lexem.symbol == symbol_semicolon) {
			assert(symbol_semicolon);
			append_entry(formal_params_section(), &params);
		}
	}
	assert(symbol_close_paren);
	return params;
}

// proc_head = "procedure" id [formal_params]
void proc_head()
{
	assert(symbol_proc);
	assert(symbol_id);
	// FAZER: Implementar parâmetros para a entrada do procedimento na tabela de símbolos
	if (is_first("formal_params", current_token.lexem.symbol))
		formal_params();
}

void declarations();

// proc_body = declarations ["begin" stmt_sequence] "end" id
void proc_body()
{
	declarations();
	if (current_token.lexem.symbol == symbol_begin) {
		assert(symbol_begin);
		stmt_sequence();
	}
	assert(symbol_end);
	assert(symbol_id);
}

// proc_decl = proc_head ";" proc_body
void proc_decl()
{
	proc_head();
	assert(symbol_semicolon);
	proc_body();
}

//
// A sintaxe original abaixo foi levemente modificada para facilitar a implementação:
//
// Original:
// declarations = ["const" {id "=" expr ";"}]["type" {id "=" type ";"}]["var" {id_list ":" type ";"}] {proc_decl ";"}
//
// Modificado:
// const_decl = "const" {id "=" expr ";"}
// type_decl = "type" {id "=" type ";"}
// var_decl = "var" {id_list ":" type ";"}
// declarations = [const_decl] [type_decl] [var_decl] {proc_decl ";"}
//
// Na prática, continua a mesma linguagem, porém melhor estruturada em sua implementação
//

// const_decl = "const" {id "=" expr ";"}
void const_decl()
{
	assert(symbol_const);
	while (current_token.lexem.symbol == symbol_id) {
		assert(symbol_id);
		entry_t *new_entry = create_entry(last_token.lexem.id, last_token.position, class_const);
		assert(symbol_equal);
		// expr();
		assert(symbol_number);
		if (new_entry) {
			new_entry->value = last_token.value;
			append_entry(new_entry, &symbol_table);
		}
		assert(symbol_semicolon);
	}
}

// type_decl = "type" {id "=" type ";"}
void type_decl()
{
	assert(symbol_type);
	while (current_token.lexem.symbol == symbol_id) {
		assert(symbol_id);
		entry_t *new_entry = create_entry(last_token.lexem.id, last_token.position, class_type);
		// FAZER: Melhorar erro para o “igual”
		assert(symbol_equal);
		type_t *base = type();
		if (new_entry && base) {
			new_entry->type = base;
			append_entry(new_entry, &symbol_table);
		}
		assert(symbol_semicolon);
	}
}

// var_decl = "var" {id_list ":" type ";"}
void var_decl()
{
	assert(symbol_var);
	while (is_first("id_list", current_token.lexem.symbol)) {
		entry_t *new_entries = id_list();
		assert(symbol_colon);
		type_t *base = type();
		entry_t *e = new_entries;
		while (e && base) {
			e->address = current_address;
			e->type = base;
			current_address += e->type->size;
			e = e->next;
		}
		append_entry(new_entries, &symbol_table);
		assert(symbol_semicolon);
	}
}

// declarations = [const_decl] [type_decl] [var_decl] {proc_decl ";"}
void declarations()
{
	if (is_first("const_decl", current_token.lexem.symbol))
			const_decl();
	if (is_first("type_decl", current_token.lexem.symbol))
			type_decl();
	if (is_first("var_decl", current_token.lexem.symbol))
			var_decl();
	while (is_first("proc_decl", current_token.lexem.symbol)) {
		proc_decl();
		assert(symbol_semicolon);
	}
}

// module = "module" id ";" declarations ["begin" stmt_sequence] "end" id "."
void module()
{
	assert(symbol_module);
	assert(symbol_id);
	assert(symbol_semicolon);
	declarations();
	if (current_token.lexem.symbol == symbol_begin) {
		assert(symbol_begin);
		stmt_sequence();
	}
	assert(symbol_end);
	assert(symbol_id);
	assert(symbol_period);
}

// Retorna se a inicialização do analisador léxico e da tabela de símbolos obteve sucesso ou não e se o arquivo de 
// entrada estava em branco
bool initialize_parser(FILE *file)
{
	should_log = false;
	if (!initialize_table(0, &symbol_table))
		return false;
	initialize_scanner(file);
	read_token();
	return current_token.lexem.symbol != symbol_eof;
}

bool parse()
{
	module();
	clear_table(&symbol_table);
	return true;
}
