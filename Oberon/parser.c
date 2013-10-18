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
// Conjuntos first(K), com ø indicando o vazio:
//
//	- selector = . { ø
//	- factor = ( ~ number id
//	- term = ( ~ number id
//	- simple_expr = + - ( ~ number id
//	- expr = + - ( ~ number id
//	- assignment = id
//	- proc_call = id
//	- stmt = id if while repeat ø
//	- stmt_sequence = id if while repeat ø
//	- field_list = id ø
//	- type = id array record
//	- formal_params_section = id var
//	- formal_params = (
//	- proc_head = procedure
//	-	proc_body = end const type var procedure begin
//	- proc_decl = procedure
//	- declarations = const type var procedure ø
//	- module = module
//
// Conjuntos follow(K), com ø indicando o vazio:
//
//	- selector = * div mod & + - or = # < <= > >= , ) ] := of then do ; end else elsif until
//	- factor = * div mod & + - or = # < <= > >= , ) ] of then do ; end else elsif until
//	- term = + - or = # < <= > >= , ) ] of then do ; end else elsif until
//	- simple_expr = = # < <= > >= , ) ] of then do ; end else elsif until
//	- expr = , ) ] of then do ; end else elsif until
//	- assignment = ; end else elsif until
//	- proc_call = ; end else elsif until
//	- stmt = ; end else elsif until
//	- stmt_sequence = end else elsif until
//	- field_list = ; end
//	- type = ) ;
//	- formal_params_section = ) ;
//	- formal_params = ;
//	- proc_head = ;
//	-	proc_body = ;
//	- proc_decl = ;
//	- declarations = end begin
//

#include "parser.h"

// Registro de ocorrências
boolean_t parser_should_log;

boolean_t parser_next()
{
	if (scanner_token.symbol == symbol_eof)
		return false;
	do {
		scanner_get();
	} while (scanner_token.symbol == symbol_null);
	return scanner_token.symbol == symbol_eof;
}

// Se “symbol_null” for passado como parâmetro, qualquer símbolo será reconhecido
boolean_t parser_assert(symbol_t symbol, boolean_t should_mark_error)
{
	if (scanner_token.symbol == symbol || symbol == symbol_null) {
		if (parser_should_log)
			errors_mark(error_log, "\"%s\" found.", scanner_token.id);
		parser_next();
		return true;
	}
	if (should_mark_error) {
		if (symbol == symbol_id)
			errors_mark(error_parser, "Missing identifier.");
		else if (symbol == symbol_number)
			errors_mark(error_parser, "Missing number.");
		else
			errors_mark(error_parser, "Missing \"%s\".", id_for_symbol(symbol));
	}
	return false;
}

void expr();

// selector = {"." id | "[" expr "]"}
void selector()
{
	while (is_first("selector", scanner_token.symbol)) {
		if (scanner_token.symbol == symbol_period) {
			parser_assert(symbol_period, false);
			parser_assert(symbol_id, true);
		} else {
			parser_assert(symbol_open_bracket, false);
			expr();
			parser_assert(symbol_close_bracket, true);
		}
	}
}

// factor = id selector | number | "(" expr ")" | "~" factor
void factor()
{
	switch (scanner_token.symbol) {
		case symbol_id:
			parser_assert(symbol_id, false);
			selector();
			break;
		case symbol_number:
			parser_assert(symbol_number, false);
			break;
		case symbol_open_paren:
			parser_assert(symbol_open_paren, false);
			expr();
			parser_assert(symbol_close_paren, true);
			break;
		case symbol_not:
			parser_assert(symbol_not, false);
			factor();
			break;
		default:
			// Sincroniza
			errors_mark(error_parser, "Missing factor.");
			while (!is_follow("factor", scanner_token.symbol) && scanner_token.symbol != symbol_eof)
				parser_next();
			break;
	}
}

// term = factor {("*" | "div" | "mod" | "&") factor}
void term()
{
	factor();
	while (scanner_token.symbol >= symbol_times && scanner_token.symbol <= symbol_and) {
		switch (scanner_token.symbol) {
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
		parser_assert(scanner_token.symbol, false); 
		factor();
	}
}

// simple_expr = ["+" | "-"] term {("+" | "-" | "OR") term}
void simple_expr()
{
	if (scanner_token.symbol == symbol_plus)
		parser_assert(symbol_plus, false);
	else if (scanner_token.symbol == symbol_minus)
		parser_assert(symbol_minus, false);
	term();
	while (scanner_token.symbol >= symbol_plus && scanner_token.symbol <= symbol_or) {
		switch (scanner_token.symbol) {
			case symbol_plus:
				break;
			case symbol_minus:
				break;
			case symbol_or:
				break;
			default:
				break;
		}
		parser_assert(scanner_token.symbol, false); 
		term();
	}
}

// expr = simple_expr [("=" | "#" | "<" | "<=" | ">" | ">=") simple_expr]
void expr()
{
	simple_expr();
	if (scanner_token.symbol >= symbol_equal && scanner_token.symbol <= symbol_greater) {
		switch (scanner_token.symbol) {
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
		parser_assert(scanner_token.symbol, false);
		simple_expr();
	}
}

// assignment = ":=" expr
void assignment()
{
	parser_assert(symbol_becomes, false);
	expr();
}

// actual_params = "(" [expr {"," expr}] ")"
void actual_params()
{
	parser_assert(symbol_open_paren, false);
	if (is_first("expr", scanner_token.symbol)) {
		expr();
		while (scanner_token.symbol == symbol_comma) {
			parser_assert(symbol_comma, false);
			expr();
		}
	}
	parser_assert(symbol_close_paren, true);
}

// proc_call = [actual_params]
void proc_call()
{
	if (is_first("actual_params", scanner_token.symbol))
		actual_params();
}

void stmt_sequence();

// if_stmt = "if" expr "then" stmt_sequence {"elsif" expr "then" stmt_sequence} ["else" stmt_sequence] "end"
void if_stmt()
{
	parser_assert(symbol_if, false);
	expr();
	parser_assert(symbol_then, true);
	stmt_sequence();
	while (scanner_token.symbol == symbol_elsif) {
		parser_assert(symbol_elsif, false);
		expr();
		parser_assert(symbol_then, true);
		stmt_sequence();
	}
	if (scanner_token.symbol == symbol_else) {
		parser_assert(symbol_else, false);
		stmt_sequence();
	}
	parser_assert(symbol_end, true);
}

// while_stmt = "while" expr "do" stmt_sequence "end"
void while_stmt()
{
	parser_assert(symbol_while, false);
	expr();
	parser_assert(symbol_do, true);
	stmt_sequence();
	parser_assert(symbol_end, true);
}

// repeat_stmt = "repeat" stmt_sequence "until" expr
void repeat_stmt()
{
	parser_assert(symbol_repeat, false);
	stmt_sequence();
	parser_assert(symbol_until, true);
	expr();
}

// stmt = [id (assignment | proc_call) | if_stmt | while_stmt | repeat_stmt]
void stmt()
{
	if (scanner_token.symbol == symbol_id) {
		parser_assert(symbol_id, false);
		selector();
		if (is_first("assignment", scanner_token.symbol))
			assignment();
		else if (is_first("proc_call", scanner_token.symbol) || is_follow("proc_call", scanner_token.symbol))
			proc_call();
		else {
			// Sincroniza
			errors_mark(error_parser, "Invalid statement.");
			while (!is_follow("stmt", scanner_token.symbol) && scanner_token.symbol != symbol_eof)
				parser_next();
		}
	}	else if (is_first("if_stmt", scanner_token.symbol))
		if_stmt();
	else if (is_first("while_stmt", scanner_token.symbol))
		while_stmt();
	else if (is_first("repeat_stmt", scanner_token.symbol))
		repeat_stmt();
	// Sincroniza
	if (!is_follow("stmt", scanner_token.symbol)) {
		errors_mark(error_parser, "Missing \";\" or \"end\".");
		while (!is_follow("stmt", scanner_token.symbol) && scanner_token.symbol != symbol_eof)
			parser_next();
	}
}

// stmt_sequence = stmt {";" stmt}
void stmt_sequence()
{
	stmt();
	while (scanner_token.symbol == symbol_semicolon) {
		parser_assert(symbol_semicolon, false);
		stmt();
	}
}

// id_list = id {"," id}
void id_list()
{
	parser_assert(symbol_id, false);
	while (scanner_token.symbol == symbol_comma) {
		parser_assert(symbol_comma, false);
		parser_assert(symbol_id, true);
	}	
}

void type();

// array_type = "array" expr "of" type
void array_type()
{
	parser_assert(symbol_array, false);
	expr();
	parser_assert(symbol_of, true);
	type();
}

// field_list = [id_list ":" type]
void field_list()
{
	if (is_first("id_list", scanner_token.symbol)) {
		id_list();
		parser_assert(symbol_colon, true);
		type();
	}
}

// record_type = "record" field_list {";" field_list} "end"
void record_type()
{
	parser_assert(symbol_record, false);
	field_list();
	while (scanner_token.symbol == symbol_semicolon) {
		parser_assert(symbol_semicolon, false);
		field_list();
	}
	parser_assert(symbol_end, true);
}

// type = id | array_type | record_type
void type()
{
	if (scanner_token.symbol == symbol_id)
		parser_assert(symbol_id, false);
	else if (is_first("array_type", scanner_token.symbol))
		array_type();
	else if (is_first("record_type", scanner_token.symbol))
		record_type();
	else {
		// Sincroniza
		errors_mark(error_parser, "Missing type.");
		while (!is_follow("type", scanner_token.symbol) && scanner_token.symbol != symbol_eof)
			parser_next();
	}
}

// formal_params_section = ["var"] id_list ":" type
void formal_params_section()
{
	if (scanner_token.symbol == symbol_var)
		parser_assert(symbol_var, false);
	id_list();
	parser_assert(symbol_colon, true);
	type();
}

// formal_params = "(" [formal_params_section {";" formal_params_section}] ")"
void formal_params()
{
	parser_assert(symbol_open_paren, false);
	if (is_first("formal_params_section", scanner_token.symbol)) {
		formal_params_section();
		while (scanner_token.symbol == symbol_semicolon) {
			parser_assert(symbol_semicolon, false);
			formal_params_section();
		}
	}
	parser_assert(symbol_close_paren, true);
}

// proc_head = "procedure" id [formal_params]
void proc_head()
{
	parser_assert(symbol_proc, false);
	parser_assert(symbol_id, true);
	if (is_first("formal_params", scanner_token.symbol))
		formal_params();
}

void declarations();

// proc_body = declarations ["begin" stmt_sequence] "end" id
void proc_body()
{
	declarations();
	if (scanner_token.symbol == symbol_begin) {
		parser_assert(symbol_begin, false);
		stmt_sequence();
	}
	parser_assert(symbol_end, true);
	parser_assert(symbol_id, true);
}

// proc_decl = proc_head ";" proc_body
void proc_decl()
{
	proc_head();
	parser_assert(symbol_semicolon, true);
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
	parser_assert(symbol_const, false);
	while (scanner_token.symbol == symbol_id) {
		parser_assert(symbol_id, false);
		parser_assert(symbol_equal, true);
		expr();
		parser_assert(symbol_semicolon, true);
	}
}

// type_decl = "type" {id "=" type ";"}
void type_decl()
{
	parser_assert(symbol_type, false);
	while (scanner_token.symbol == symbol_id) {
		parser_assert(symbol_id, false);
		parser_assert(symbol_equal, true);  // FAZER: Melhorar erro para o “igual”
		type();
		parser_assert(symbol_semicolon, true);
	}
}

// var_decl = "var" {id_list ":" type ";"}
void var_decl()
{
	parser_assert(symbol_var, false);
	while (is_first("id_list", scanner_token.symbol)) {
		id_list();
		parser_assert(symbol_colon, true);
		type();
		parser_assert(symbol_semicolon, true);
	}
}

// declarations = [const_decl] [type_decl] [var_decl] {proc_decl ";"}
void declarations()
{
	if (is_first("const_decl", scanner_token.symbol))
			const_decl();
	if (is_first("type_decl", scanner_token.symbol))
			type_decl();
	if (is_first("var_decl", scanner_token.symbol))
			var_decl();
	while (is_first("proc_decl", scanner_token.symbol)) {
		proc_decl();
		parser_assert(symbol_semicolon, true);
	}
}

// module = "module" id ";" declarations ["begin" stmt_sequence] "end" id "."
void module()
{
	parser_assert(symbol_module, true);
	parser_assert(symbol_id, true);
	parser_assert(symbol_semicolon, true);
	declarations();
	if (scanner_token.symbol == symbol_begin) {
		parser_assert(symbol_begin, false);
		stmt_sequence();
	}
	parser_assert(symbol_end, true);
	parser_assert(symbol_id, true);
	parser_assert(symbol_period, true);
}

// Retorna se a inicialização do analisador léxico  e da tabela de símbolos obteve sucesso ou não e se o arquivo de 
// entrada estava em branco
boolean_t parser_initialize()
{
	parser_should_log = false;
	if (!symbol_table_initialize())
		return false;
	scanner_initialize();
	scanner_get();
	return scanner_token.symbol != symbol_eof;
}

boolean_t parser_run()
{
	module();
	table_clear(&symbol_table);
	return true;
}
