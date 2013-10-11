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
// EBNF da linguagem Oberon-0 (para “letter” e “digit”, usar as funções “is_alpha” e “is_digit”):
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
//	- proc_heading = "procedure" id [formal_params]
//	- proc_body = declarations ["begin" stmt_sequence] "end" id
//	- proc_declaration = proc_heading ";" proc_body
//	- declarations = ["const" {id "=" exp ";"}] ["type" {id "=" type ";"}] ["var" {id_list ":" type ";"}] {proc_declaration ";"}
//	- module = "module" id ";" declarations ["begin" stmt_sequence "end" id "."
//

#include <stdio.h>
#include <memory.h>
#include <ctype.h>

int main(int argc, const char *argv[]) {
	FILE *source_code = fopen("Test.oberon", "r");
	if (!source_code)
		return 0;
	fclose(source_code);
	return 0;
}
