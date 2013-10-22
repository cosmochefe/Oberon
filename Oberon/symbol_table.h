//
//  symbol_table.h
//  Oberon
//
//  Created by Alvaro Costa Neto on 10/17/13.
//  Copyright (c) 2013 Alvaro Costa Neto. All rights reserved.
//

#ifndef Oberon_symbol_table_h
#define Oberon_symbol_table_h

#include <stdbool.h>

#include "backend.h"
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

struct _entry;

typedef struct _type {
	form_t form;
	value_t length;
	unsigned int size;
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

type_t *create_type(form_t form, value_t length, unsigned int size, entry_t *fields, type_t *base);
entry_t *create_entry(identifier_t id, position_t position, class_t class);

bool initialize_table(address_t base_address, entry_t **ref);
void clear_table(entry_t **ref);
void log_table(entry_t *table);
entry_t *find_entry(identifier_t id, entry_t *table);
bool append_entry(entry_t *entry, entry_t **ref);

#endif
