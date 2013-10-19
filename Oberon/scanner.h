//
//  scanner.h
//  Oberon
//
//  Created by Alvaro Costa Neto on 10/12/13.
//  Copyright (c) 2013 Alvaro Costa Neto. All rights reserved.
//

#ifndef Oberon_scanner_h
#define Oberon_scanner_h

#include <stdio.h>
#include <strings.h>
#include <ctype.h>

#include "types.h"
#include "errors.h"

#define SCANNER_MAX_ID_LENGTH 16

typedef enum _symbol {
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
} symbol_t;

typedef char identifier_t[SCANNER_MAX_ID_LENGTH + 1];

typedef long int value_t;

typedef struct _position {
	unsigned int line;
	unsigned int column;
	fpos_t index;
} position_t;

typedef struct _lexem {
	identifier_t id;
	symbol_t symbol;
} lexem_t;

extern lexem_t scanner_keywords[];
extern const index_t scanner_keywords_count;

extern lexem_t scanner_operators[];
extern const index_t scanner_operators_count;

extern lexem_t scanner_punctuation[];
extern const index_t scanner_punctuation_count;

typedef struct _token {
	identifier_t id;
	symbol_t symbol;
	value_t value;
	position_t position;
} token_t;

extern token_t scanner_token;
extern token_t scanner_last_token;
extern position_t scanner_position;
extern const position_t position_zero;

//
// Pré-definições
//

boolean_t is_letter(char c);
boolean_t is_digit(char c);
boolean_t is_blank(char c);

boolean_t is_keyword(identifier_t id, symbol_t *symbol);

boolean_t is_first(string_t non_terminal, symbol_t symbol);
boolean_t is_follow(string_t non_terminal, symbol_t symbol);

string_t id_for_symbol(symbol_t symbol);

void scanner_initialize();
void scanner_get();

#endif
