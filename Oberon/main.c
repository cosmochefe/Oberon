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

//
// Constantes e definições gerais
//
enum boolean_ {
	false = 0,
	true = 1
};
typedef enum boolean_ boolean_t;

//
// Constantes e definições do analisador léxico
//

// Símbolos
enum symbol_ {
	symbol_null = 0,
	symbol_times = 1,
	symbol_div = 3,
	symbol_mod = 4,
	symbol_and = 5,
	symbol_plus = 6,
	symbol_minus = 7,
	symbol_or = 8,
	symbol_equal = 9,
	symbol_not_equal = 10,
	symbol_less = 11,
	symbol_less_equal = 13,
	symbol_greater = 14,
	symbol_greater_equal = 12,
	symbol_period = 18,
	symbol_comma = 19,
	symbol_colon = 20,
	symbol_close_paren = 22,
	symbol_close_bracket = 23,
	symbol_of = 25,
	symbol_then = 26,
	symbol_do = 27,
	symbol_open_paren = 29,
	symbol_open_bracket = 30,
	symbol_not = 32,
	symbol_becomes = 33,
	symbol_number = 34,
	symbol_id = 37,
	symbol_semicolon = 38,
	symbol_end = 40,
	symbol_else = 41,
	symbol_elsif = 42,
	symbol_until = 43,
	symbol_if = 44,
	symbol_while = 46,
	symbol_repeat = 47,
	symbol_array = 54,
	symbol_record = 55,
	symbol_const = 57,
	symbol_type = 58,
	symbol_var = 59,
	symbol_proc = 60,
	symbol_begin = 61,
	symbol_module = 63,
	symbol_eof = 64
};
typedef enum symbol_ symbol_t;
// Identificadores
const unsigned int id_length = 16;
typedef char id_t[id_length + 1];
// Valores
typedef long int value_t;
// Palavras-chave
struct keyword_ {
	char id[16];
	symbol_t symbol;
};
typedef struct keyword_ keyword_t;
// Entrada do texto
typedef FILE * file_t;

// Propriedades do lexema
id_t scanner_id;
value_t scanner_value;

// Tratamento de erros na análise léxica
boolean_t scanner_error;
unsigned long scanner_error_position;

// O ponteiro para o arquivo com o código-fonte e o caracter atual
file_t scanner_file;
char scanner_char;

// Vetor com todas as palavras-chave da linguagem
keyword_t scanner_keywords[] = {
	{ .id = "do", .symbol = symbol_do },
	{ .id = "if", .symbol = symbol_if },
	{ .id = "of", .symbol = symbol_of },
	{ .id = "or", .symbol = symbol_or },
	{ .id = "end", .symbol = symbol_end },
	{ .id = "mod", .symbol = symbol_mod },
	{ .id = "var", .symbol = symbol_var },
	{ .id = "else", .symbol = symbol_else },
	{ .id = "then", .symbol = symbol_then },
	{ .id = "type", .symbol = symbol_type },
	{ .id = "array", .symbol = symbol_array },
	{ .id = "begin", .symbol = symbol_begin },
	{ .id = "const", .symbol = symbol_const },
	{ .id = "elsif", .symbol = symbol_elsif },
	{ .id = "until", .symbol = symbol_until },
	{ .id = "while", .symbol = symbol_while },
	{ .id = "record", .symbol = symbol_record },
	{ .id = "repeat", .symbol = symbol_repeat },
	{ .id = "procedure", .symbol = symbol_proc },
	{ .id = "div", .symbol = symbol_div },
	{ .id = "module", .symbol = symbol_module }
};


//
// Analisador léxico
//

boolean_t is_keyword(id_t id, symbol_t *symbol) {
	if (strcasecmp("do", id) == 0) {
		*symbol = symbol_do;
		return true;
	}
	return false;
}

void scanner_mark(const char *message) {

}

void scanner_get(symbol_t *symbol) {

}

void scanner_initialize(file_t file, unsigned long position) {

}

//
// Ponto de entrada do programa
//

int main(int argc, const char *argv[]) {
	file_t source_code = fopen("/Users/Neto/Dropbox/Programming/Projects/Oberon/Oberon/Test.oberon", "r");
	if (!source_code) {
		printf("Arquivo não encontrado.");
		return 0;
	}
	fclose(source_code);
	return 0;
}
