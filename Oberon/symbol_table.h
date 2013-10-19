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

typedef size_t address_t;

struct _entry;

typedef struct _type {
	form_t form;
	value_t length;
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

// FAZER: Adicionar árvore de tabelas para gerenciar escopo

extern entry_t *symbol_table;
extern address_t current_address;

//
// Pré-definições
//

boolean_t symbol_table_initialize(address_t base_address);

type_t *type_create(form_t form, value_t length, size_t size, type_t *base);
entry_t *entry_create(identifier_t id, position_t position, class_t class);

void table_clear(entry_t **ref);
void table_log(entry_t *table);
entry_t *table_find(identifier_t id, entry_t *table);
boolean_t table_append(entry_t *entry, entry_t **ref);

#endif
