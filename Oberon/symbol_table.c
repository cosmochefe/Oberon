//
//  symbol_table.c
//  Oberon
//
//  Created by Alvaro Costa Neto on 10/17/13.
//  Copyright (c) 2013 Alvaro Costa Neto. All rights reserved.
//

#include "symbol_table.h"

object_t *symbol_table;

position_t position_zero = { .line = 0, .column = 0, .index = 0 };

boolean_t symbol_table_initialize() {
	table_clear(&symbol_table);
	// Cria os tipos elementares
	type_t *type = type_create(type_form_atomic, 0, NULL);
	if (!type) {
		errors_mark(error_fatal, "Not enough memory. By the way, who are you and what the hell is 42?");
		return false;
	}
	object_t *base_type = table_add_type("integer", position_zero, type, &symbol_table);
	if (!base_type) {
		errors_mark(error_fatal, "Not enough memory. By the way, who are you and what the hell is 42?");
		free(type);
		return false;
	}
	return true;
}

type_t *type_create(type_form_t form, index_t length, type_t *base) {
	type_t *type = (type_t *)malloc(sizeof(type_t));
	if (!type) {
		errors_mark(error_fatal, "Not enough memory. By the way, who are you and what the hell is 42?");
		return NULL;
	}
	type->form = form;
	type->length = length;
	type->size = 0;
	type->fields = NULL;
	type->base = base;
	return type;
}

void table_clear(object_t **reference) {
	object_t *t = *reference;
	while (t) {
		object_t *current = t;
		t = current->next;
		if (current->type && current->type->fields)
			table_clear(&current->type->fields);
		free(current);
	}
	*reference = NULL;
}

void table_log(object_t *table) {
	while (table) {
		printf("Object: %s\n", table->id);
		table = table->next;
	}
}

object_t *table_find(identifier_t id, object_t *table) {
	object_t *current = table;
	while (current) {
		if (strcasecmp(current->id, id) == 0)
			break;
		current = current->next;
	}
	return current;
}

object_t *table_add(identifier_t id, position_t p, class_t c, type_t *t, value_t v, object_t **reference) {
	if (!reference)
		return NULL;
	object_t *table = *reference;
	object_t *new_object = table_find(id, table);
	if (new_object) {
		errors_mark(error_parser, "The identifier \"%s\" has already been declared.", id);
		return NULL;
	}
	new_object = (object_t *)malloc(sizeof(object_t));
	if (!new_object) {
		errors_mark(error_fatal, "Not enough memory. By the way, who are you and what the hell is 42?");
		return NULL;
	}
	strcpy(new_object->id, id);
	new_object->position = p;
	new_object->address = 0;
	new_object->class = c;
	new_object->type = t;
	new_object->value = v;
	new_object->next = NULL;
	if (!table)
		*reference = new_object;
	else {
		while (table && table->next)
			table = table->next;
		table->next = new_object;
	}
	return new_object;
}

object_t *table_add_const(identifier_t id, position_t position, value_t value, object_t **reference) {
	return table_add(id, position, class_const, NULL, value, reference);
}

object_t *table_add_type(identifier_t id, position_t position, type_t *type, object_t **reference) {
	return table_add(id, position, class_type, type, 0, reference);
}

object_t *table_add_var(identifier_t id, position_t position, type_t *type, object_t **reference) {
	return table_add(id, position, class_var, type, 0, reference);
}

object_t *table_add_proc(identifier_t id, position_t position, type_t *type, object_t **reference) {
	return table_add(id, position, class_proc, type, 0, reference);
}
