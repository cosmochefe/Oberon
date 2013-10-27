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
//	- assignment = id selector ":=" expr
//	- actual_params = "(" [expr {"," expr}] ")"
//	- proc_call = id selector [actual_params]
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

bool verify(symbol_t symbol, bool mark_error, bool next)
{
	if (current_token.lexem.symbol == symbol || symbol == symbol_null) {
		if (should_log)
			mark(error_log, "\"%s\" found.", current_token.lexem.id);
		if (next)
			scan();
		return true;
	}
	if (mark_error)
		mark_missing(symbol);
	return false;
}

// As versões com “try_” não mostram mensagens de erro caso não encontrem o símbolo passado como parâmetro

// Esta função faz o mesmo que “consume”, mas não avança para a próxima ficha léxica
static inline bool assert(symbol_t symbol)
{
	return verify(symbol, true, false);
}
static inline bool try_assert(symbol_t symbol)
{
	return verify(symbol, false, false);
}

// Se “symbol_null” for passado como parâmetro, qualquer símbolo será reconhecido
static inline bool consume(symbol_t symbol)
{
	return verify(symbol, true, true);
}
static inline bool try_consume(symbol_t symbol)
{
	return verify(symbol, false, true);
}

void expr();

// selector = {"." id | "[" expr "]"}
address_t selector(entry_t *entry)
{
	if (!entry)
		return 0;
	while (is_first("selector", current_token.lexem.symbol)) {
		if (try_consume(symbol_period)) {
			// TODO: Verificar os campos de entry, caso ele seja um registro. Caso contrário, erro!
			consume(symbol_id);
		} else if (try_consume(symbol_open_bracket)) {
			// TODO: Verificar se entry é um vetor
			expr();
			if (!consume(symbol_close_bracket));
				// TODO: Apontar qual símbolo de abertura está sobressalente
		} else {
			// Sincroniza
			mark(error_parser, "Invalid selector.");
			while (!is_follow("selector", current_token.lexem.symbol) && current_token.lexem.symbol != symbol_eof)
				scan();
		}
	}
	return entry->address;
}

// factor = id selector | number | "(" expr ")" | "~" factor
void factor()
{
	if (try_assert(symbol_id)) {
		entry_t *entry = find_entry(current_token.lexem.id, symbol_table);
		if (!entry) {
			mark(error_parser, "\"%s\" hasn't been declared yet.", current_token.lexem.id);
			scan();
		} else {
			scan();
			write_load(selector(entry));
		}
	} else if (try_assert(symbol_number)) {
		write_load_immediate(current_token.value);
		scan();
	} else if (try_consume(symbol_open_paren)) {
		expr();
		if (!consume(symbol_close_paren));
			// TODO: Apontar qual símbolo de abertura está sobressalente
	} else if (try_consume(symbol_not)) {
		factor();
		// TODO: Adicionar operação lógica inversora
	} else {
		// Sincroniza
		mark(error_parser, "Missing factor.");
		while (!is_follow("factor", current_token.lexem.symbol) && current_token.lexem.symbol != symbol_eof)
			scan();
	}
}

// term = factor {("*" | "div" | "mod" | "&") factor}
void term()
{
	factor();
	while (true) {
		if (try_consume(symbol_times)) {
		} else if (try_consume(symbol_div)) {
		} else if (try_consume(symbol_mod)) {
		} else if (try_consume(symbol_and)) {
		} else
			break;
		factor();
	}
}

// simple_expr = ["+" | "-"] term {("+" | "-" | "OR") term}
void simple_expr()
{
	if (try_consume(symbol_plus));
	else if (try_consume(symbol_minus));
	term();
	while (true) {
		if (try_consume(symbol_plus)) {
		} else if (try_consume(symbol_minus)) {
		} else if (try_consume(symbol_or)) {
		} else
			break;
		term();
	}
}

// expr = simple_expr [("=" | "#" | "<" | "<=" | ">" | ">=") simple_expr]
void expr()
{
	simple_expr();
	if (try_consume(symbol_equal)) {
	} else if (try_consume(symbol_not_equal)) {
	} else if (try_consume(symbol_less)) {
	} else if (try_consume(symbol_less_equal)) {
	} else if (try_consume(symbol_greater)) {
	} else if (try_consume(symbol_greater_equal)) {
	} else
		return;
	simple_expr();
}

// assignment = ":=" expr
void assignment(entry_t *entry)
{
	try_consume(symbol_becomes);
	expr();
	// TODO: Adicionar o armazenamento do resultado
}

// actual_params = "(" [expr {"," expr}] ")"
void actual_params()
{
	try_consume(symbol_open_paren);
	if (is_first("expr", current_token.lexem.symbol)) {
		expr();
		while (try_consume(symbol_comma))
			expr();
	}
	if (!consume(symbol_close_paren));
		// TODO: Apontar qual símbolo de abertura está sobressalente
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
	try_consume(symbol_if);
	expr();
	consume(symbol_then);
	stmt_sequence();
	while (try_consume(symbol_elsif)) {
		expr();
		consume(symbol_then);
		stmt_sequence();
	}
	if (try_consume(symbol_else))
		stmt_sequence();
	consume(symbol_end);
}

// while_stmt = "while" expr "do" stmt_sequence "end"
void while_stmt()
{
	try_consume(symbol_while);
	expr();
	consume(symbol_do);
	stmt_sequence();
	consume(symbol_end);
}

// repeat_stmt = "repeat" stmt_sequence "until" expr
void repeat_stmt()
{
	try_consume(symbol_repeat);
	stmt_sequence();
	consume(symbol_until);
	expr();
}

// stmt = [id selector (assignment | proc_call) | if_stmt | while_stmt | repeat_stmt]
void stmt()
{
	if (try_consume(symbol_id)) {
		// TODO: Adicionar geração de código e obtenção da entrada na tabela de símbolos
		selector(NULL);
		if (is_first("assignment", current_token.lexem.symbol))
			assignment(NULL);
		else if (is_first("proc_call", current_token.lexem.symbol) || is_follow("proc_call", current_token.lexem.symbol))
			proc_call();
		else {
			// Sincroniza
			mark(error_parser, "Invalid statement.");
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
		mark(error_parser, "Missing \";\" or \"end\".");
		while (!is_follow("stmt", current_token.lexem.symbol) && current_token.lexem.symbol != symbol_eof)
			scan();
	}
}

// stmt_sequence = stmt {";" stmt}
void stmt_sequence()
{
	stmt();
	while (try_consume(symbol_semicolon))
		stmt();
}

// id_list = id {"," id}
entry_t *id_list()
{
	try_assert(symbol_id);
	entry_t *new_entries = create_entry(current_token.lexem.id, current_token.position, class_var);
	scan();
	while (try_consume(symbol_comma)) {
		if (assert(symbol_id)) {
			append_entry(create_entry(current_token.lexem.id, current_token.position, class_var), &new_entries);
			scan();
		}
	}
	return new_entries;
}

type_t *type();

// array_type = "array" expr "of" type
type_t *array_type()
{
	type_t *new_type = NULL;
	try_consume(symbol_array);
	new_type = create_type(form_array, 0, 0, NULL, NULL);
	//	expr();
	value_t length = 0;
	if (assert(symbol_number)) {
		length = current_token.value;
		scan();
	}
	consume(symbol_of);
	type_t *base_type = type();
	unsigned int size = 0;
	if (base_type)
		size = length * base_type->size;
	if (new_type) {
		new_type->length = length;
		new_type->size = size;
		new_type->base = base_type;
	}
	return new_type;
}

// field_list = [id_list ":" type]
entry_t *field_list()
{
	if (is_first("id_list", current_token.lexem.symbol)) {
		entry_t *new_fields = id_list();
		consume(symbol_colon);
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
	type_t *new_type = NULL;
	try_consume(symbol_record);
	new_type = create_type(form_record, 0, 0, NULL, NULL);
	entry_t *fields = field_list();
	while (try_consume(symbol_semicolon)) {
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
	consume(symbol_end);
	if (new_type) {
		new_type->size = size;
		new_type->fields = fields;
	}
	return new_type;
}

// type = id | array_type | record_type
type_t *type()
{
	if (try_assert(symbol_id)) {
		// Qualquer tipo atômico deve ser baseado em um dos tipos internos da linguagem (neste caso apenas “integer”)
		entry_t *entry = find_entry(current_token.lexem.id, symbol_table);
		if (entry) {
			scan();
			return entry->type;
		} else {
			mark(error_parser, "Unknown type \"%s\".", current_token.lexem.id);
			scan();
			return NULL;
		}
	} else if (is_first("array_type", current_token.lexem.symbol)) {
		type_t *new_type = array_type();
		if (!new_type)
			mark(error_parser, "Invalid array type.");
		return new_type;
	} else if (is_first("record_type", current_token.lexem.symbol)) {
		type_t *new_type = record_type();
		if (!new_type)
			mark(error_parser, "Invalid record type.");
		return new_type;
	}
	// Sincroniza
	mark(error_parser, "Missing type.");
	while (!is_follow("type", current_token.lexem.symbol) && current_token.lexem.symbol != symbol_eof)
		scan();
	return NULL;
}

// formal_params_section = ["var"] id_list ":" type
entry_t *formal_params_section()
{
	if (try_consume(symbol_var));
		// TODO: Implementar a passagem de parâmetro por referência
	entry_t *new_params = id_list();
	if (!consume(symbol_colon))
		mark_missing(symbol_colon);
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
	try_consume(symbol_open_paren);
	if (is_first("formal_params_section", current_token.lexem.symbol)) {
		params = formal_params_section();
		while (try_consume(symbol_semicolon))
			append_entry(formal_params_section(), &params);
	}
	consume(symbol_close_paren);
	return params;
}

// proc_head = "procedure" id [formal_params]
void proc_head()
{
	try_consume(symbol_proc);
	consume(symbol_id);
	// TODO: Implementar parâmetros para a entrada do procedimento na tabela de símbolos
	if (is_first("formal_params", current_token.lexem.symbol))
		formal_params();
}

void declarations();

// proc_body = declarations ["begin" stmt_sequence] "end" id
void proc_body()
{
	declarations();
	if (try_consume(symbol_begin))
		stmt_sequence();
	consume(symbol_end);
	consume(symbol_id);
}

// proc_decl = proc_head ";" proc_body
void proc_decl()
{
	proc_head();
	consume(symbol_semicolon);
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
	try_consume(symbol_const);
	while (try_assert(symbol_id)) {
		entry_t *new_entry = create_entry(current_token.lexem.id, current_token.position, class_const);
		scan();
		consume(symbol_equal);
		// expr();
		if (assert(symbol_number)) {
			if (new_entry) {
				new_entry->value = current_token.value;
				append_entry(new_entry, &symbol_table);
			}
			scan();
		}
		consume(symbol_semicolon);
	}
}

// type_decl = "type" {id "=" type ";"}
void type_decl()
{
	try_consume(symbol_type);
	while (try_assert(symbol_id)) {
		entry_t *new_entry = create_entry(current_token.lexem.id, current_token.position, class_type);
		scan();
		// TODO: Melhorar erro para o “igual”
		consume(symbol_equal);
		type_t *base = type();
		if (new_entry && base) {
			new_entry->type = base;
			append_entry(new_entry, &symbol_table);
		}
		consume(symbol_semicolon);
	}
}

// var_decl = "var" {id_list ":" type ";"}
void var_decl()
{
	try_consume(symbol_var);
	while (is_first("id_list", current_token.lexem.symbol)) {
		entry_t *new_entries = id_list();
		consume(symbol_colon);
		type_t *base = type();
		entry_t *e = new_entries;
		while (e && base) {
			e->address = current_address;
			e->type = base;
			current_address += e->type->size;
			e = e->next;
		}
		append_entry(new_entries, &symbol_table);
		consume(symbol_semicolon);
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
		consume(symbol_semicolon);
	}
}

// module = "module" id ";" declarations ["begin" stmt_sequence] "end" id "."
void module()
{
	consume(symbol_module);
	consume(symbol_id);
	consume(symbol_semicolon);
	declarations();
	if (try_consume(symbol_begin))
		stmt_sequence();
	consume(symbol_end);
	consume(symbol_id);
	consume(symbol_period);
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
