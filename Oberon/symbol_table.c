//
//  symbol_table.c
//  Oberon
//
//  Created by Alvaro Costa Neto on 10/17/13.
//  Copyright (c) 2013 Alvaro Costa Neto. All rights reserved.
//

#include "symbol_table.h"

entry_t *symbol_table;
address_t current_address;

boolean_t symbol_table_initialize(address_t base_address)
{
	current_address = base_address;
	table_clear(&symbol_table);
	// Os tipos elementares (neste caso, apenas “integer”) são as primeiras entradas da tabela de símbolos
	// Todos os tipos elementares da linguagem devem ser criados e adicionados à tabela nesta função
	type_t *base_type = type_create(form_atomic, 0, sizeof(int8_t), NULL);
	if (!base_type) {
		errors_mark(error_fatal, "Not enough memory. By the way, who are you and what the hell is 42?");
		return false;
	}
	entry_t *type = entry_create("integer", position_zero, class_type);
	if (!type) {
		errors_mark(error_fatal, "Not enough memory. By the way, who are you and what the hell is 42?");
		free(base_type);
		return false;
	}
	type->type = base_type;
	table_append(type, &symbol_table);
	return true;
}

type_t *type_create(form_t form, value_t length, size_t size, type_t *base)
{
	type_t *type = (type_t *)malloc(sizeof(type_t));
	if (!type) {
		errors_mark(error_fatal, "Not enough memory. By the way, who are you and what the hell is 42?");
		return NULL;
	}
	type->form = form;
	type->length = length;
	type->size = size;
	type->fields = NULL;
	type->base = base;
	return type;
}

entry_t *entry_create(identifier_t id, position_t position, class_t class)
{
	entry_t *new_entry = (entry_t *)malloc(sizeof(entry_t));
	if (!new_entry) {
		errors_mark(error_fatal, "Not enough memory. By the way, who are you and what the hell is 42?");
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
		errors_mark(error_log, "Entry \"%s\" found.", table->id);
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

boolean_t table_append(entry_t *entry, entry_t **ref)
{
	if (!ref || !entry)
		return false;
	entry_t *table = *ref;
	if (table_find(entry->id, table)) {
		// FAZER: Melhorar a mensagem de erro adicionando a posição do objecto já declarado
		errors_mark(error_parser, "The identifier \"%s\" has already been declared.", entry->id);
		return false;
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
