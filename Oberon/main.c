//
//  main.c
//  Oberon
//
//  Created by Alvaro Costa Neto on 10/11/13.
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
// EBNF da linguagem Oberon-0 (para “letter” e “digit”, usar as funções “is_alpha”, “is_alnum” e “is_digit”):
//	- id = letter {letter | digit}
//	- integer = digit {digit}
//	- selector = {"." id | "[" exp "]"}
//	- number = integer
//	- factor = id selector | number | "(" exp ")" | "~" factor
//	- term = factor {("*" | "div" | "mod" | "&") factor}
//	- simple_exp = ["+" | "-"] term {"+" | "-" | "OR") term}
//	- exp = simple_exp [("=" | "#" | "<" | "<=" | ">" | ">=") simple_exp]
//	- assignment = ident selector ":=" exp
//	- actual_params = "(" [exp {"," exp}] ")"
//	- proc_call = ident selector [actual_params]
//	- if_stmt = "if" exp "then" stmt_sequence {"elsif" exp "then" stmt_sequence} ["else" stmt_sequence] "end"
//	- while_stmt = "while" exp "do" stmt_sequence "end"
//	- repeat_stmt = "repeat" stmt_sequence "until" exp
//	- stmt = [assignment | proc_call | if_stmt | while_stmt | repeat_stmt]
//	- stmt_sequence = stmt {";" stmt}
//	- id_list = id {"," id}
//	- array_type = "array" exp "of" type
//	- field_list = [id_list ":" type]
//	- record_type = "record" field_list {";" field_list} "end"
//	- type = id | array_type | record_type
//	- formal_params_section = ["var"] id_list ":" type
//	- formal_params = "(" [formal_params_section {";" formal_params_section}] ")"
//	- proc_head = "procedure" id [formal_params]
//	- proc_body = declarations ["begin" stmt_sequence] "end" id
//	- proc_decl = proc_head ";" proc_body
//	- declarations = ["const" {id "=" exp ";"}]["type" {id "=" type ";"}]["var" {id_list ":" type ";"}]{proc_decl ";"}
//	- module = "module" id ";" declarations ["begin" stmt_sequence] "end" id "."
//
//	Vocabulário (para a análise léxica):
//
//	- * div mod & + - or
//	- = # < <= > >= . , : ) ]
//	- of then do until ( [ ~ := ;
//	- end else elsif if while repeat
//	- array record const type var procedure begin module
//

#include <stdio.h>
#include <memory.h>
#include <ctype.h>
#include <strings.h>

#include "types.h"
#include "scanner.h"
#include "parser.h"

//
// Constantes e definições gerais
//

//
// Ponto de entrada do programa
//

int main(int argc, const string_t argv[]) {
	file_t source_code_file = fopen("/Users/Neto/Dropbox/Programming/Projects/Oberon/Oberon/Test.oberon", "r");
	if (!source_code_file) {
		printf("Arquivo não encontrado.\n");
		return 0;
	}
	scanner_initialize(source_code_file, 0);
	symbol_t symbol = symbol_null;
	while (symbol != symbol_eof) {
		scanner_get(&symbol);
//		if (symbol == symbol_id)
//			printf("id: %s, ", scanner_id);
//		else if (symbol == symbol_number)
//			printf("number: %ld, ", scanner_value);
//		else
			printf("%d, ", symbol);
	}
	printf("eof\n");
	fclose(source_code_file);
	return 0;
}
