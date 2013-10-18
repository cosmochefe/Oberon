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

typedef enum _form {
	form_atomic,
	form_array,
	form_record
} form_t;

typedef unsigned long address_t;

struct _entry;

typedef struct _type {
	form_t form;
	index_t length;
	size_t size;
	struct _entry *fields;
	struct _type *base;
} type_t;

typedef struct _entry {
	identifier_t id;
	position_t position;
	address_t address;
	class_t class;
	value_t value;
	struct _type *type;
	struct _entry *next;
} entry_t;

extern entry_t *symbol_table;

//
// Pré-definições
//

boolean_t symbol_table_initialize();

type_t *type_create(form_t form, index_t length, size_t size, type_t *base);

void table_clear(entry_t **ref);
void table_log(entry_t *table);
entry_t *table_find(identifier_t id, entry_t *table);
entry_t *table_add_const(identifier_t id, position_t position, value_t value, entry_t **ref);
entry_t *table_add_type(identifier_t id, position_t position, type_t *type, entry_t **ref);
entry_t *table_add_var(identifier_t id, position_t position, type_t *type, entry_t **ref);
entry_t *table_add_proc(identifier_t id, position_t position, type_t *type, entry_t **ref);

#endif
