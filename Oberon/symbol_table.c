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
entry_t *integer_type;
entry_t *boolean_type;

entry_t *create_elementary_type(identifier_t id)
{
  entry_t *entry = create_entry(id, position_zero, class_type);
  if (!entry) {
    mark_not_enough_memory();
    return NULL;
  }
  type_t *type = create_type(form_atomic, 0, sizeof(value_t), NULL, NULL);
  if (!type) {
    mark_not_enough_memory();
    free(entry);
    return NULL;
  }
  entry->type = type;
	return entry;
}

bool initialize_table(address_t base_address, entry_t **ref)
{
  current_address = base_address;
  clear_table(ref);
  // Os tipos elementares (“integer” e “boolean”) são as primeiras entradas da tabela de símbolos
  // Todos os tipos elementares da linguagem devem ser criados e adicionados à tabela nesta função
	integer_type = create_elementary_type("INTEGER");
  boolean_type = create_elementary_type("BOOLEAN");
	if (!integer_type || !boolean_type)
		return false;
  add_entry(integer_type, ref);
  add_entry(boolean_type, ref);
  return true;
}

type_t *create_type(form_t form, value_t length, unsigned int size, entry_t *fields, type_t *base)
{
  type_t *type = (type_t *)malloc(sizeof(type_t));
  if (!type) {
    mark_not_enough_memory();
    return NULL;
  }
  type->form = form;
  type->length = length;
  type->size = size;
  type->fields = fields;
  type->base = base;
  return type;
}

link_t *create_link(fpos_t position)
{
  link_t *link = (link_t *)malloc(sizeof(link_t));
  if (!link) {
    mark_not_enough_memory();
    return NULL;
  }
  link->position = position;
  link->next = NULL;
  return link;
}

entry_t *create_entry(identifier_t id, position_t position, class_t class)
{
  entry_t *new_entry = (entry_t *)malloc(sizeof(entry_t));
  if (!new_entry) {
    mark_not_enough_memory();
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

void clear_links(link_t **ref)
{
  link_t *links = *ref;
  while (links) {
    link_t *current = links;
    links = current->next;
    free(current);
  }
  *ref = NULL;
}

void clear_table(entry_t **ref)
{
  // TODO: Verificar quais tipos de entradas na tabela devem ser removidos primeiro
  // Pode haver um problema ao liberar a memória para um “type” e este ainda ter referências para si
  // em outras entradas. Contagem de referências seria uma solução…
  entry_t *table = *ref;
  while (table) {
    entry_t *current = table;
    table = current->next;
    if (current->type && current->type->fields)
      clear_table(&current->type->fields);
    free(current);
  }
  *ref = NULL;
}

void log_table(entry_t *table)
{
  while (table) {
    mark_at(error_log, position_zero, "Entry \"%s\" found.", table->id);
    table = table->next;
  }
}

entry_t *find_entry(identifier_t id, entry_t *table)
{
  entry_t *current = table;
  while (current) {
    if (strcmp(current->id, id) == 0)
      break;
    current = current->next;
  }
  return current;
}

bool add_link(link_t *link, link_t **ref)
{
  if (!ref || !link)
    return false;
  link_t *links = *ref;
  if (links)
    link->next = links;
  *ref = link;
  return true;
}

bool add_entry(entry_t *entry, entry_t **ref)
{
  if (!ref || !entry)
    return false;
  entry_t *table = *ref;
  entry_t *e = entry;
  while (e) {
    if (find_entry(e->id, table))
      mark_at(error_parser, e->position, "The identifier \"%s\" has already been declared.", e->id);
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
