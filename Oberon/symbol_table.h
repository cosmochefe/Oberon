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

boolean_t symbol_table_initialize();

type_t *type_create(type_form_t form, index_t length, type_t *base);

void table_clear(object_t **table);
void table_log(object_t *table);
object_t *table_find(identifier_t id, object_t *table);
object_t *table_add_const(identifier_t id, position_t position, value_t value, object_t **table);
object_t *table_add_type(identifier_t id, position_t position, type_t *type, object_t **table);
object_t *table_add_var(identifier_t id, position_t position, type_t *type, object_t **table);
object_t *table_add_proc(identifier_t id, position_t position, type_t *type, object_t **table);

#endif
