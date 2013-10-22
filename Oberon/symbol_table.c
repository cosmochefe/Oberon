//
//  symbol_table.c
//  Oberon
//
//  Created by Alvaro Costa Neto on 10/17/13.
//  Copyright (c) 2013 Alvaro Costa Neto. All rights reserved.
//

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "backend.h"
#include "errors.h"
#include "scanner.h"
#include "symbol_table.h"

entry_t *symbol_table;
address_t current_address;

bool symbol_table_initialize(address_t base_address)
{
	current_address = base_address;
	table_clear(&symbol_table);
	// Os tipos elementares (neste caso, apenas “integer”) são as primeiras entradas da tabela de símbolos
	// Todos os tipos elementares da linguagem devem ser criados e adicionados à tabela nesta função
	type_t *base_type = type_create(form_atomic, 0, sizeof(value_t), NULL, NULL);
	if (!base_type) {
		mark(error_fatal, position_zero, "Not enough memory. By the way, who are you and what the hell is 42?");
		return false;
	}
	entry_t *type = entry_create("integer", position_zero, class_type);
	if (!type) {
		mark(error_fatal, position_zero, "Not enough memory. By the way, who are you and what the hell is 42?");
		free(base_type);
		return false;
	}
	type->type = base_type;
	table_append(type, &symbol_table);
	return true;
}

type_t *type_create(form_t form, value_t length, unsigned int size, entry_t *fields, type_t *base)
{
	type_t *type = (type_t *)malloc(sizeof(type_t));
	if (!type) {
		mark(error_fatal, scanner_last_token.position, "Not enough memory. By the way, who are you and what the hell is 42?");
		return NULL;
	}
	type->form = form;
	type->length = length;
	type->size = size;
	type->fields = fields;
	type->base = base;
	return type;
}

entry_t *entry_create(identifier_t id, position_t position, class_t class)
{
	entry_t *new_entry = (entry_t *)malloc(sizeof(entry_t));
	if (!new_entry) {
		mark(error_fatal, position, "Not enough memory. By the way, who are you and what the hell is 42?");
		return NULL;
	}
	strcpy(new_entry->id, id);
	new_entry->position = position;
	new_entry->address = 0;
	new_entry->class = class;
	new_entry->type = NULL;
	new_entry->value = 0;
	new_entry->next = NULL;
	return new_entry;
}

void table_clear(entry_t **ref)
{
	// FAZER: Verificar quais tipos de entradas na tabela devem ser removidos primeiro
	// Pode haver um problema ao liberar a memória para um “type” e este ainda ter referências para si
	// em outras entradas. Contagem de referências seria uma solução…
	entry_t *table = *ref;
	while (table) {
		entry_t *current = table;
		table = current->next;
		if (current->type && current->type->fields)
			table_clear(&current->type->fields);
		free(current);
	}
	*ref = NULL;
}

void table_log(entry_t *table)
{
	while (table) {
		mark(error_log, position_zero, "Entry \"%s\" found.", table->id);
		table = table->next;
	}
}

entry_t *table_find(identifier_t id, entry_t *table)
{
	entry_t *current = table;
	while (current) {
		if (strcasecmp(current->id, id) == 0)
			break;
		current = current->next;
	}
	return current;
}

bool table_append(entry_t *entry, entry_t **ref)
{
	if (!ref || !entry)
		return false;
	entry_t *table = *ref;
	entry_t *e = entry;
	while (e) {
		if (table_find(e->id, table))
			mark(error_parser, e->position, "The identifier \"%s\" has already been declared.", e->id);
		e = e->next;
	}
	if (!table)
		*ref = entry;
	else {
		while (table->next)
			table = table->next;
		table->next = entry;
	}
	return true;
}
