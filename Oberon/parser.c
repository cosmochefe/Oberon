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
//	- simple_expr = ["+" | "-"] term {"+" | "-" | "OR") term}
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

boolean_t parser_assert_symbol(symbol_t symbol, string_t id) {
	char message[255];
	if (parser_symbol == symbol) {
		sprintf(message, "\"%s\" encontrado.", id);
		parser_log(message);
		parser_next();
		return true;
	}
	sprintf(message, "Faltando \"%s\".", id);
	scanner_mark(message);
	return false;
}

//// selector = {"." id | "[" expr "]"}
//boolean_t selector() {
//	
//}
//
//// factor = id selector | number | "(" expr ")" | "~" factor
//boolean_t factor() {
//
//}
//
//// term = factor {("*" | "div" | "mod" | "&") factor}
//boolean_t term() {
//
//}
//
//// simple_expr = ["+" | "-"] term {"+" | "-" | "OR") term}
//boolean_t simple_expr() {
//
//}
//
//// expr = simple_expr [("=" | "#" | "<" | "<=" | ">" | ">=") simple_expr]
//boolean_t expr() {
//
//}
//
//// assignment = ident selector ":=" expr
//boolean_t assignment() {
//
//}
//
//// actual_params = "(" [expr {"," expr}] ")"
//boolean_t actual_params() {
//
//}
//
//// proc_call = ident selector [actual_params]
//boolean_t proc_call() {
//
//}
//
//// if_stmt = "if" expr "then" stmt_sequence {"elsif" expr "then" stmt_sequence} ["else" stmt_sequence] "end"
//boolean_t if_stmt() {
//
//}
//
//// while_stmt = "while" expr "do" stmt_sequence "end"
//boolean_t while_stmt() {
//
//}
//
//// repeat_stmt = "repeat" stmt_sequence "until" expr
//boolean_t repeat_stmt() {
//
//}
//
//// stmt = [assignment | proc_call | if_stmt | while_stmt | repeat_stmt]
//boolean_t stmt() {
//
//}
//
//// stmt_sequence = stmt {";" stmt}
//boolean_t stmt_sequence() {
//
//}
//
//// id_list = id {"," id}
//boolean_t id_list() {
//
//}
//
//// array_type = "array" expr "of" type
//boolean_t array_type() {
//
//}
//
//// field_list = [id_list ":" type]
//boolean_t field_list() {
//
//}
//
//// record_type = "record" field_list {";" field_list} "end"
//boolean_t record_type() {
//
//}
//
//// type = id | array_type | record_type
//boolean_t type() {
//
//}
//
//// formal_params_section = ["var"] id_list ":" type
//boolean_t formal_params_section() {
//
//}
//
//// formal_params = "(" [formal_params_section {";" formal_params_section}] ")"
//boolean_t formal_params() {
//
//}
//
//// proc_head = "procedure" id [formal_params]
//boolean_t proc_head() {
//
//}
//
//// proc_body = declarations ["begin" stmt_sequence] "end" id
//boolean_t proc_body() {
//
//}
//
//// proc_decl = proc_head ";" proc_body
//boolean_t proc_decl() {
//
//}
//
//// declarations = ["const" {id "=" expr ";"}]["type" {id "=" type ";"}]["var" {id_list ":" type ";"}]{proc_decl ";"}
//boolean_t declarations() {
//
//}

// module = "module" id ";" declarations ["begin" stmt_sequence] "end" id "."
boolean_t module() {
	if (!parser_assert_symbol(symbol_module, "module"))
		return false;
	return true;
}

// Retorna se pôde ser lido ou não
boolean_t parser_initialize(file_t file) {
	scanner_initialize(file, 0);
	scanner_get(&parser_symbol);
	if (parser_symbol == symbol_eof)
		return false;
	return true;
}

boolean_t parser_run() {
	return module();
}
