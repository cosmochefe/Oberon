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

//
// Constantes, variáveis e definições gerais
//

// Símbolo atual
symbol_t parser_symbol;

// Registro de ocorrências
boolean_t parser_should_log = true;

// Arquivos de entrada e saída
file_t parser_input_file, parser_output_file;

//
// Analisador sintático
//

// FAZER: Adicionar recuperação de erros

void parser_next() {
	scanner_get(&parser_symbol);
}

void parser_log(const string_t message) {
	if (parser_should_log)
		printf("Log: %s\n", message);
}

// Se “symbol_null” for passado como parâmetro, qualquer símbolo será reconhecido
// FAZER: Implementar algo mais robusto!
boolean_t parser_assert(symbol_t symbol) {
	char message[256];
	if (scanner_token.symbol == symbol || symbol == symbol_null) {
		sprintf(message, "\"%s\" encontrado.", scanner_token.id);
		parser_log(message);
		parser_next();
		return true;
	}
	if (symbol == symbol_id)
		sprintf(message, "Faltando um identificador.");
	else if (symbol == symbol_number)
		sprintf(message, "Faltando um número.");
	else
		sprintf(message, "Faltando \"%s\".", id_for_symbol(symbol));
	scanner_mark(message);
	return false;
}

boolean_t expr();

// selector = {"." id | "[" expr "]"}
boolean_t selector() {
	boolean_t result = true;
	while (scanner_token.symbol == symbol_period || scanner_token.symbol == symbol_open_bracket) {
		if (scanner_token.symbol == symbol_period) {
			result = parser_assert(symbol_period);
			result = parser_assert(symbol_id);
		} else {
			result = parser_assert(symbol_open_bracket);
			result = expr();
			result = parser_assert(symbol_close_bracket);
		}
	}
	return result;
}

// factor = id selector | number | "(" expr ")" | "~" factor
boolean_t factor() {
	boolean_t result;
	switch (scanner_token.symbol) {
		case symbol_id:
			result = parser_assert(symbol_id);
			result = selector();
			break;
		case symbol_number:
			result = parser_assert(symbol_number);
			break;
		case symbol_open_paren:
			result = parser_assert(symbol_open_paren);
			result = expr();
			result = parser_assert(symbol_close_paren);
			break;
		case symbol_not:
			result = parser_assert(symbol_not);
			result = factor();
			break;
		default:
			scanner_mark("Faltando fator.");
			result = false;
			break;
	}
	return result;
}

// term = factor {("*" | "div" | "mod" | "&") factor}
boolean_t term() {
	boolean_t result;
	result = factor();
	while (scanner_token.symbol >= symbol_times && scanner_token.symbol <= symbol_and) {
		result = parser_assert(symbol_null);
		result = factor();
	}
	return result;
}

// simple_expr = ["+" | "-"] term {("+" | "-" | "OR") term}
boolean_t simple_expr() {
	boolean_t result;
	if (scanner_token.symbol == symbol_plus)
		result = parser_assert(symbol_plus);
	else if (scanner_token.symbol == symbol_minus)
		result = parser_assert(symbol_minus);
	result = term();
	while (scanner_token.symbol >= symbol_plus && scanner_token.symbol <= symbol_or) {
		result = parser_assert(symbol_null);
		result = term();
	}
	return result;
}

// expr = simple_expr [("=" | "#" | "<" | "<=" | ">" | ">=") simple_expr]
boolean_t expr() {
	boolean_t result;
	result = simple_expr();
	// FAZER: Imlementar categorias para verificação
	if (scanner_token.symbol >= symbol_equal && scanner_token.symbol <= symbol_greater) {
		result = parser_assert(symbol_null);
		result = simple_expr();
	}
	return result;
}

// assignment = ident selector ":=" expr
boolean_t assignment() {
	boolean_t result;
	return result;
}

// actual_params = "(" [expr {"," expr}] ")"
boolean_t actual_params() {
	boolean_t result;
	return result;
}

// proc_call = ident selector [actual_params]
boolean_t proc_call() {
	boolean_t result;
	return result;
}

// if_stmt = "if" expr "then" stmt_sequence {"elsif" expr "then" stmt_sequence} ["else" stmt_sequence] "end"
boolean_t if_stmt() {
	boolean_t result;
	return result;
}

// while_stmt = "while" expr "do" stmt_sequence "end"
boolean_t while_stmt() {
	boolean_t result;
	return result;
}

// repeat_stmt = "repeat" stmt_sequence "until" expr
boolean_t repeat_stmt() {
	boolean_t result;
	return result;
}

// stmt = [assignment | proc_call | if_stmt | while_stmt | repeat_stmt]
boolean_t stmt() {
	boolean_t result;
	return result;
}

// stmt_sequence = stmt {";" stmt}
boolean_t stmt_sequence() {
	boolean_t result;
	return result;
}

// id_list = id {"," id}
boolean_t id_list() {
	boolean_t result;
	return result;
}

// array_type = "array" expr "of" type
boolean_t array_type() {
	boolean_t result;
	return result;
}

// field_list = [id_list ":" type]
boolean_t field_list() {
	boolean_t result;
	return result;
}

// record_type = "record" field_list {";" field_list} "end"
boolean_t record_type() {
	boolean_t result;
	return result;
}

// type = id | array_type | record_type
boolean_t type() {
	boolean_t result;
	return result;
}

// formal_params_section = ["var"] id_list ":" type
boolean_t formal_params_section() {
	boolean_t result;
	return result;
}

// formal_params = "(" [formal_params_section {";" formal_params_section}] ")"
boolean_t formal_params() {
	boolean_t result;
	return result;
}

// proc_head = "procedure" id [formal_params]
boolean_t proc_head() {
	boolean_t result;
	return result;
}

// proc_body = declarations ["begin" stmt_sequence] "end" id
boolean_t proc_body() {
	boolean_t result;
	return result;
}

// proc_decl = proc_head ";" proc_body
boolean_t proc_decl() {
	boolean_t result;
//	result = proc_head();
//	result = parser_assert(symbol_semicolon);
//	result = proc_body();
	return result;
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
boolean_t const_decl() {
	boolean_t result;
	result = parser_assert(symbol_const);
	while (scanner_token.symbol == symbol_id) {
		result = parser_assert(symbol_id);
		result = parser_assert(symbol_equal);
		result = expr();
		result = parser_assert(symbol_semicolon);
	}
	return result;
}

// type_decl = "type" {id "=" type ";"}
boolean_t type_decl() {
	boolean_t result;
	result = parser_assert(symbol_type);
	return result;
}

// var_decl = "var" {id_list ":" type ";"}
boolean_t var_decl() {
	boolean_t result;
	result = parser_assert(symbol_var);
	return result;
}

// declarations = [const_decl] [type_decl] [var_decl] {proc_decl ";"}
boolean_t declarations() {
	boolean_t result = true;
	if (scanner_token.symbol == symbol_const)
			result = const_decl();
//	if (scanner_token.symbol == symbol_type)
//			result = type_decl();
//	if (scanner_token.symbol == symbol_var)
//			result = var_decl();
//	while (scanner_token.symbol == symbol_proc) {
//		result = proc_decl();
//		result = parser_assert(symbol_semicolon);
//	}
	return result;
}

// module = "module" id ";" declarations ["begin" stmt_sequence] "end" id "."
boolean_t module() {
	boolean_t result;
	result = parser_assert(symbol_module);
	result = parser_assert(symbol_id);
	result = parser_assert(symbol_semicolon);
	result = declarations();
	if (scanner_token.symbol == symbol_begin) {
		result = parser_assert(symbol_begin);
//		result = stmt_sequence();
	}
	result = parser_assert(symbol_end);
	result = parser_assert(symbol_id);
	result = parser_assert(symbol_period);
	return result;
}

// Retorna se a inicialização do analisador léxico obteve sucesso ou não e se o arquivo estava em branco
boolean_t parser_initialize(file_t input_file, file_t output_file) {
	parser_input_file = input_file;
	parser_output_file = output_file;
	scanner_initialize(parser_input_file, 0);
	scanner_get();
	if (scanner_token.symbol == symbol_eof)
		return false;
	return true;
}

boolean_t parser_run() {
	return module();
}