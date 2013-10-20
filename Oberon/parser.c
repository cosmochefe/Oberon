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

#include "errors.h"
#include "scanner.h"
#include "symbol_table.h"
#include "parser.h"

// Registro de ocorrências
bool parser_should_log;

bool parser_next()
{
	if (scanner_token.lexem.symbol == symbol_eof)
		return false;
	do {
		scanner_get();
	} while (scanner_token.lexem.symbol == symbol_null);
	return scanner_token.lexem.symbol == symbol_eof;
}

// Se “symbol_null” for passado como parâmetro, qualquer símbolo será reconhecido
bool parser_assert(symbol_t symbol)
{
	if (scanner_token.lexem.symbol == symbol || symbol == symbol_null) {
		if (parser_should_log)
			errors_mark(error_log, "\"%s\" found.", scanner_token.lexem.id);
		parser_next();
		return true;
	}
	if (symbol == symbol_id)
		errors_mark(error_parser, "Missing identifier.");
	else if (symbol == symbol_number)
		errors_mark(error_parser, "Missing number.");
	else
		errors_mark(error_parser, "Missing \"%s\".", id_for_symbol(symbol));
	return false;
}

void expr();

// selector = {"." id | "[" expr "]"}
void selector()
{
	while (is_first("selector", scanner_token.lexem.symbol)) {
		if (scanner_token.lexem.symbol == symbol_period) {
			parser_assert(symbol_period);
			parser_assert(symbol_id);
		} else {
			parser_assert(symbol_open_bracket);
			expr();
			parser_assert(symbol_close_bracket);
		}
	}
}

// factor = id selector | number | "(" expr ")" | "~" factor
void factor()
{
	switch (scanner_token.lexem.symbol) {
		case symbol_id:
			parser_assert(symbol_id);
			selector();
			break;
		case symbol_number:
			parser_assert(symbol_number);
			break;
		case symbol_open_paren:
			parser_assert(symbol_open_paren);
			expr();
			parser_assert(symbol_close_paren);
			break;
		case symbol_not:
			parser_assert(symbol_not);
			factor();
			break;
		default:
			// Sincroniza
			errors_mark(error_parser, "Missing factor.");
			while (!is_follow("factor", scanner_token.lexem.symbol) && scanner_token.lexem.symbol != symbol_eof)
				parser_next();
			break;
	}
}

// term = factor {("*" | "div" | "mod" | "&") factor}
void term()
{
	factor();
	while (scanner_token.lexem.symbol >= symbol_times && scanner_token.lexem.symbol <= symbol_and) {
		switch (scanner_token.lexem.symbol) {
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
		parser_assert(scanner_token.lexem.symbol); 
		factor();
	}
}

// simple_expr = ["+" | "-"] term {("+" | "-" | "OR") term}
void simple_expr()
{
	if (scanner_token.lexem.symbol == symbol_plus)
		parser_assert(symbol_plus);
	else if (scanner_token.lexem.symbol == symbol_minus)
		parser_assert(symbol_minus);
	term();
	while (scanner_token.lexem.symbol >= symbol_plus && scanner_token.lexem.symbol <= symbol_or) {
		switch (scanner_token.lexem.symbol) {
			case symbol_plus:
				break;
			case symbol_minus:
				break;
			case symbol_or:
				break;
			default:
				break;
		}
		parser_assert(scanner_token.lexem.symbol); 
		term();
	}
}

// expr = simple_expr [("=" | "#" | "<" | "<=" | ">" | ">=") simple_expr]
void expr()
{
	simple_expr();
	if (scanner_token.lexem.symbol >= symbol_equal && scanner_token.lexem.symbol <= symbol_greater) {
		switch (scanner_token.lexem.symbol) {
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
		parser_assert(scanner_token.lexem.symbol);
		simple_expr();
	}
}

// assignment = ":=" expr
void assignment()
{
	parser_assert(symbol_becomes);
	expr();
}

// actual_params = "(" [expr {"," expr}] ")"
void actual_params()
{
	parser_assert(symbol_open_paren);
	if (is_first("expr", scanner_token.lexem.symbol)) {
		expr();
		while (scanner_token.lexem.symbol == symbol_comma) {
			parser_assert(symbol_comma);
			expr();
		}
	}
	parser_assert(symbol_close_paren);
}

// proc_call = [actual_params]
void proc_call()
{
	if (is_first("actual_params", scanner_token.lexem.symbol))
		actual_params();
}

void stmt_sequence();

// if_stmt = "if" expr "then" stmt_sequence {"elsif" expr "then" stmt_sequence} ["else" stmt_sequence] "end"
void if_stmt()
{
	parser_assert(symbol_if);
	expr();
	parser_assert(symbol_then);
	stmt_sequence();
	while (scanner_token.lexem.symbol == symbol_elsif) {
		parser_assert(symbol_elsif);
		expr();
		parser_assert(symbol_then);
		stmt_sequence();
	}
	if (scanner_token.lexem.symbol == symbol_else) {
		parser_assert(symbol_else);
		stmt_sequence();
	}
	parser_assert(symbol_end);
}

// while_stmt = "while" expr "do" stmt_sequence "end"
void while_stmt()
{
	parser_assert(symbol_while);
	expr();
	parser_assert(symbol_do);
	stmt_sequence();
	parser_assert(symbol_end);
}

// repeat_stmt = "repeat" stmt_sequence "until" expr
void repeat_stmt()
{
	parser_assert(symbol_repeat);
	stmt_sequence();
	parser_assert(symbol_until);
	expr();
}

// stmt = [id (assignment | proc_call) | if_stmt | while_stmt | repeat_stmt]
void stmt()
{
	if (scanner_token.lexem.symbol == symbol_id) {
		parser_assert(symbol_id);
		selector();
		if (is_first("assignment", scanner_token.lexem.symbol))
			assignment();
		else if (is_first("proc_call", scanner_token.lexem.symbol) || is_follow("proc_call", scanner_token.lexem.symbol))
			proc_call();
		else {
			// Sincroniza
			errors_mark(error_parser, "Invalid statement.");
			while (!is_follow("stmt", scanner_token.lexem.symbol) && scanner_token.lexem.symbol != symbol_eof)
				parser_next();
		}
	}	else if (is_first("if_stmt", scanner_token.lexem.symbol))
		if_stmt();
	else if (is_first("while_stmt", scanner_token.lexem.symbol))
		while_stmt();
	else if (is_first("repeat_stmt", scanner_token.lexem.symbol))
		repeat_stmt();
	// Sincroniza
	if (!is_follow("stmt", scanner_token.lexem.symbol)) {
		errors_mark(error_parser, "Missing \";\" or \"end\".");
		while (!is_follow("stmt", scanner_token.lexem.symbol) && scanner_token.lexem.symbol != symbol_eof)
			parser_next();
	}
}

// stmt_sequence = stmt {";" stmt}
void stmt_sequence()
{
	stmt();
	while (scanner_token.lexem.symbol == symbol_semicolon) {
		parser_assert(symbol_semicolon);
		stmt();
	}
}

// id_list = id {"," id}
entry_t *id_list()
{
	parser_assert(symbol_id);
	entry_t *new_entries = entry_create(scanner_last_token.lexem.id, scanner_last_token.position, class_var);
	while (scanner_token.lexem.symbol == symbol_comma) {
		parser_assert(symbol_comma);
		if (parser_assert(symbol_id))
			table_append(entry_create(scanner_last_token.lexem.id, scanner_last_token.position, class_var), &new_entries);
	}
	return new_entries;
}

type_t *type();

// array_type = "array" expr "of" type
type_t *array_type()
{
	parser_assert(symbol_array);
	//	expr();
	value_t length = 0;
	if (parser_assert(symbol_number))
		length = scanner_last_token.value;
	parser_assert(symbol_of);
	type_t *base = type();
	unsigned int size = 0;
	if (base)
		size = length * base->size;
	return type_create(form_array, length, size, NULL, base);
}

// field_list = [id_list ":" type]
entry_t *field_list()
{
	if (is_first("id_list", scanner_token.lexem.symbol)) {
		entry_t *new_fields = id_list();
		parser_assert(symbol_colon);
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
	parser_assert(symbol_record);
	entry_t *fields = field_list();
	while (scanner_token.lexem.symbol == symbol_semicolon) {
		parser_assert(symbol_semicolon);
		entry_t *more_fields = field_list();
		if (fields && more_fields)
			table_append(more_fields, &fields);
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
	parser_assert(symbol_end);
	return type_create(form_record, 0, size, fields, NULL);
}

// type = id | array_type | record_type
type_t *type()
{
	if (scanner_token.lexem.symbol == symbol_id) {
		// Qualquer tipo atômico deve ser baseado em um dos tipos internos da linguagem (neste caso apenas “integer”)
		parser_assert(symbol_id);
		entry_t *entry = table_find(scanner_last_token.lexem.id, symbol_table);
		if (entry)
			return entry->type;
		else
			errors_mark(error_parser, "Unknown type \"%s\".", scanner_last_token.lexem.id);
	} else if (is_first("array_type", scanner_token.lexem.symbol)) {
		type_t *new_type = array_type();
		if (!new_type)
			errors_mark(error_parser, "Invalid array type.");
		return new_type;
	} else if (is_first("record_type", scanner_token.lexem.symbol)) {
		type_t *new_type = record_type();
		if (!new_type)
			errors_mark(error_parser, "Invalid record type.");
		return new_type;
	}
	// Sincroniza
	errors_mark(error_parser, "Missing type.");
	while (!is_follow("type", scanner_token.lexem.symbol) && scanner_token.lexem.symbol != symbol_eof)
		parser_next();
	return NULL;
}

// formal_params_section = ["var"] id_list ":" type
entry_t *formal_params_section()
{
	if (scanner_token.lexem.symbol == symbol_var)
		parser_assert(symbol_var);
	entry_t *new_params = id_list();
	parser_assert(symbol_colon);
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
	parser_assert(symbol_open_paren);
	if (is_first("formal_params_section", scanner_token.lexem.symbol)) {
		params = formal_params_section();
		while (scanner_token.lexem.symbol == symbol_semicolon) {
			parser_assert(symbol_semicolon);
			table_append(formal_params_section(), &params);
		}
	}
	parser_assert(symbol_close_paren);
	return params;
}

// proc_head = "procedure" id [formal_params]
void proc_head()
{
	parser_assert(symbol_proc);
	parser_assert(symbol_id);
	// FAZER: Implementar parâmetros para a entrada do procedimento na tabela de símbolos
	if (is_first("formal_params", scanner_token.lexem.symbol))
		formal_params();
}

void declarations();

// proc_body = declarations ["begin" stmt_sequence] "end" id
void proc_body()
{
	declarations();
	if (scanner_token.lexem.symbol == symbol_begin) {
		parser_assert(symbol_begin);
		stmt_sequence();
	}
	parser_assert(symbol_end);
	parser_assert(symbol_id);
}

// proc_decl = proc_head ";" proc_body
void proc_decl()
{
	proc_head();
	parser_assert(symbol_semicolon);
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
	parser_assert(symbol_const);
	while (scanner_token.lexem.symbol == symbol_id) {
		parser_assert(symbol_id);
		entry_t *new_entry = entry_create(scanner_last_token.lexem.id, scanner_last_token.position, class_const);
		parser_assert(symbol_equal);
		// expr();
		parser_assert(symbol_number);
		if (new_entry) {
			new_entry->value = scanner_last_token.value;
			table_append(new_entry, &symbol_table);
		}
		parser_assert(symbol_semicolon);
	}
}

// type_decl = "type" {id "=" type ";"}
void type_decl()
{
	parser_assert(symbol_type);
	while (scanner_token.lexem.symbol == symbol_id) {
		parser_assert(symbol_id);
		entry_t *new_entry = entry_create(scanner_last_token.lexem.id, scanner_last_token.position, class_type);
		// FAZER: Melhorar erro para o “igual”
		parser_assert(symbol_equal);
		type_t *base = type();
		if (new_entry && base) {
			new_entry->type = base;
			table_append(new_entry, &symbol_table);
		}
		parser_assert(symbol_semicolon);
	}
}

// var_decl = "var" {id_list ":" type ";"}
void var_decl()
{
	parser_assert(symbol_var);
	while (is_first("id_list", scanner_token.lexem.symbol)) {
		entry_t *new_entries = id_list();
		parser_assert(symbol_colon);
		type_t *base = type();
		entry_t *e = new_entries;
		while (e && base) {
			e->address = current_address;
			e->type = base;
			current_address += e->type->size;
			e = e->next;
		}
		if (new_entries)
			table_append(new_entries, &symbol_table);
		parser_assert(symbol_semicolon);
	}
}

// declarations = [const_decl] [type_decl] [var_decl] {proc_decl ";"}
void declarations()
{
	if (is_first("const_decl", scanner_token.lexem.symbol))
			const_decl();
	if (is_first("type_decl", scanner_token.lexem.symbol))
			type_decl();
	if (is_first("var_decl", scanner_token.lexem.symbol))
			var_decl();
	while (is_first("proc_decl", scanner_token.lexem.symbol)) {
		proc_decl();
		parser_assert(symbol_semicolon);
	}
}

// module = "module" id ";" declarations ["begin" stmt_sequence] "end" id "."
void module()
{
	parser_assert(symbol_module);
	parser_assert(symbol_id);
	parser_assert(symbol_semicolon);
	declarations();
	if (scanner_token.lexem.symbol == symbol_begin) {
		parser_assert(symbol_begin);
		stmt_sequence();
	}
	parser_assert(symbol_end);
	parser_assert(symbol_id);
	parser_assert(symbol_period);
}

// Retorna se a inicialização do analisador léxico e da tabela de símbolos obteve sucesso ou não e se o arquivo de 
// entrada estava em branco
bool parser_initialize(FILE *file)
{
	parser_should_log = false;
	if (!symbol_table_initialize(0))
		return false;
	scanner_initialize(file);
	scanner_get();
	return scanner_token.lexem.symbol != symbol_eof;
}

bool parser_run()
{
	module();
	table_clear(&symbol_table);
	return true;
}
