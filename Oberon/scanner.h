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

typedef char id_t[SCANNER_MAX_ID_LENGTH + 1];

typedef long int value_t;

typedef struct _keyword {
	id_t id;
	symbol_t symbol;
} keyword_t;

typedef fpos_t position_t;

//extern keyword_t scanner_keywords[];
//extern const index_t scanner_keywords_count;

//
// Pré-definições
//
boolean_t is_letter(char c);
boolean_t is_digit(char c);
boolean_t is_blank(char c);
boolean_t is_keyword(id_t id, symbol_t *symbol);
void scanner_initialize(file_t file, position_t position);
void scanner_get(symbol_t *symbol);

#endif
