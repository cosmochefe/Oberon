//
//  symbol_table.c
//  Oberon
//
//  Created by Alvaro Costa Neto on 10/17/13.
//  Copyright (c) 2013 Alvaro Costa Neto. All rights reserved.
//

#include "symbol_table.h"

object_t *symbol_table;
object_t *symbol_table_last;

position_t position_zero = { .line = 0, .column = 0, .index = 0 };

type_t *type_create(type_form_t form, index_t length, type_t *base) {
	type_t *type = (type_t *)malloc(sizeof(type_t));
	if (!type) {
		errors_mark(error_fatal, "Not enough memory. By the way, who are you and what the hell is 42?");
		return NULL;
	}
	type->form = form;
	type->length = length;
	type->fields = NULL;
	type->base = base;
	return type;
}

boolean_t type_add_field(token_t token, type_t *type, type_t *record_type) {
	object_t *field = (object_t *)malloc(sizeof(object_t));
	if (!field) {
		errors_mark(error_fatal, "Not enough memory. By the way, who are you and what the hell is 42?");
		return false;
	}
	strcpy(field->id, token.id);
	field->position = token.position;
	field->class = class_var;
	field->type = type;
	field->value = 0;
	field->next = NULL;
	object_t *current = record_type->fields;
	while (current && current->next)
		current = current->next;
	current->next = field;
	return true;
}

void symbol_table_clear() {
	while (symbol_table) {
		object_t *current = symbol_table;
		symbol_table = current->next;
		free(current);
	}
	symbol_table = NULL;
	symbol_table_last = NULL;
}

boolean_t symbol_table_initialize() {
	symbol_table_clear();
	// Cria os tipos elementares
	object_t *base_type = (object_t *)malloc(sizeof(object_t));
	type_t *type = (type_t *)malloc(sizeof(type_t));
	if (!base_type || !type) {
		errors_mark(error_fatal, "Not enough memory. By the way, who are you and what the hell is 42?");
		return false;
	}
	type->form = type_form_atomic;
	type->length = 0;
	type->size = sizeof(int8_t);
	type->fields = NULL;
	type->base = NULL;
	strcpy(base_type->id, "integer");
	base_type->position = position_zero;
	base_type->address = 0;
	base_type->class = class_type;
	base_type->type = type;
	base_type->value = 0;
	base_type->next = NULL;
	symbol_table = base_type;
	return true;
}

object_t *symbol_table_find(identifier_t id, object_t *table) {
	object_t *current = table;
	while (current) {
		if (strcasecmp(current->id, id) == 0)
			break;
		current = current->next;
	}
	return current;
}

boolean_t symbol_table_add(token_t token, class_t class, type_t *type, value_t value, object_t *table) {
	object_t *new_object = symbol_table_find(token.id, table);
	if (new_object) {
		errors_mark(error_parser, "The identifier \"%s\" has already been declared.", token.id);
		return false;
	}
	new_object = (object_t *)malloc(sizeof(object_t));
	if (!new_object) {
		errors_mark(error_fatal, "Not enough memory. By the way, who are you and what the hell is 42?");
		return false;
	}
	strcpy(new_object->id, token.id);
	new_object->position = token.position;
	new_object->class = class;
	new_object->type = type;
	new_object->value = value;
	new_object->next = NULL;
	while (table && table->next)
		table = table->next;
	table->next = new_object;
	return true;
}

void symbol_table_log() {
	object_t *current = symbol_table;
	while (current) {
		printf("Object: %s\n", current->id);
		current = current->next;
	}
}