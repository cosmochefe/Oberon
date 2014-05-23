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

#define SYMBOL_TABLE_MAX_LABEL_LENGTH 16

typedef enum _class {
  class_unknown,
  class_var,
  class_const,
  class_type,
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

typedef enum _addressing {
  addressing_unknown,
  addressing_direct,
  addressing_immediate,
  addressing_register,
  addressing_indirect,
  addressing_condition
} addressing_t;

typedef struct _link {
  fpos_t position;
  struct _link *next;
} link_t;

typedef struct _item {
  addressing_t addressing;
  struct _type *type;
  address_t address;   // Para variáveis na memória
  value_t value;       // Para constantes
  unsigned char index; // Para registradores
  symbol_t condition;  // Para condicionais
  char label[SYMBOL_TABLE_MAX_LABEL_LENGTH + 1];
  link_t *links;
  link_t *true_links, *false_links;
} item_t;

// TODO: Criar uma estrutura de dados real e organizar toda essa bagunça!

extern entry_t *symbol_table;
extern address_t current_address;
extern entry_t *integer_type;
extern entry_t *boolean_type;

type_t *create_type(form_t form, value_t length, unsigned int size, entry_t *fields, type_t *base);
link_t *create_link(fpos_t position);
entry_t *create_entry(identifier_t id, position_t position, class_t class);

bool initialize_table(address_t base_address, entry_t **ref);
void clear_table(entry_t **ref);
void clear_links(link_t **ref);
void log_table(entry_t *table);
entry_t *find_entry(identifier_t id, entry_t *table);
bool add_entry(entry_t *entry, entry_t **ref);
bool add_link(link_t *link, link_t **ref);

#endif
