//
//  symbol_table.h
//  Oberon
//
//  Created by Alvaro Costa Neto on 10/17/13.
//  Copyright (c) 2013 Alvaro Costa Neto. All rights reserved.
//

#ifndef Oberon_symbol_table_h
#define Oberon_symbol_table_h

#include <stdio.h>
#include <memory.h>
#include <strings.h>
#include <ctype.h>

#include "types.h"
#include "errors.h"
#include "scanner.h"

typedef enum _class {
	class_const,
	class_type,
	class_var,
	class_proc
} class_t;

typedef enum _type_form {
	type_form_atomic,
	type_form_array,
	type_form_record
} type_form_t;

typedef unsigned long address_t;

struct _object;

typedef struct _type {
	type_form_t form;
	index_t length;
	size_t size;
	struct _object *fields;
	struct _type *base;
} type_t;

typedef struct _object {
	identifier_t id;
	position_t position;
	address_t address;
	class_t class;
	value_t value;
	struct _type *type;
	struct _object *next;
} object_t;

extern object_t *symbol_table;

//
// Pré-definições
//

type_t *type_create(type_form_t form, index_t length, type_t *base);
boolean_t type_add_field(token_t token, type_t *type, type_t *record_type);

void symbol_table_clear();
boolean_t symbol_table_initialize();
object_t *symbol_table_find(identifier_t id, object_t *table);
boolean_t symbol_table_add(token_t token, class_t class, type_t *type, value_t value, object_t *table);
void symbol_table_log();

#endif
