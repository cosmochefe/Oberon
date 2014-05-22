//
//  parser.c
//  Strawberry
//
//  Criado por Alvaro Costa Neto em 12/10/2013.
//  Copyright (c) 2013 Alvaro Costa Neto. Todos os direitos reservados.
//

#include <stdbool.h>
#include <string.h>

#include "backend.h"
#include "errors.h"
#include "scanner.h"
#include "symbol_table.h"
#include "parser.h"

bool should_log;

// Funções de geração de código
void write_index_offset(item_t *item, item_t *index_item);
void write_field_offset(item_t *item, address_t offset);
void write_unary_op(symbol_t symbol, item_t *item);
void write_binary_op(symbol_t symbol, item_t *item, item_t *rhs_item);
void write_comparison(symbol_t symbol, item_t *item, item_t *rhs_item);
void write_branch(item_t *item, bool forward);
void write_inverse_branch(item_t *item, bool forward);
void write_label(item_t *item, const char *label);
void write_store(item_t *dst_item, item_t *src_item);
void fixup_links(item_t *item);

bool is_first(const char *non_terminal, symbol_t symbol)
{
  if (strcmp(non_terminal, "selector") == 0)
    return symbol == symbol_period ||
           symbol == symbol_open_bracket ||
           symbol == symbol_null;
  else if (strcmp(non_terminal, "factor") == 0)
    return symbol == symbol_open_paren ||
           symbol == symbol_not ||
           symbol == symbol_number ||
           symbol == symbol_id;
  else if (strcmp(non_terminal, "term") == 0)
    return symbol == symbol_open_paren ||
           symbol == symbol_not ||
           symbol == symbol_number ||
           symbol == symbol_id;
  else if (strcmp(non_terminal, "simple_expr") == 0)
    return symbol == symbol_plus ||
           symbol == symbol_minus ||
           symbol == symbol_open_paren ||
           symbol == symbol_not ||
           symbol == symbol_number ||
           symbol == symbol_id;
  else if (strcmp(non_terminal, "expr") == 0)
    return symbol == symbol_plus ||
           symbol == symbol_minus ||
           symbol == symbol_open_paren ||
           symbol == symbol_not ||
           symbol == symbol_number ||
           symbol == symbol_id;
  else if (strcmp(non_terminal, "assignment") == 0)
    return symbol == symbol_becomes;
  else if (strcmp(non_terminal, "actual_params") == 0)
    return symbol == symbol_open_paren;
  else if (strcmp(non_terminal, "proc_call") == 0)
    return symbol == symbol_open_paren ||
           symbol == symbol_null;
  else if (strcmp(non_terminal, "if_stmt") == 0)
    return symbol == symbol_if;
  else if (strcmp(non_terminal, "while_stmt") == 0)
    return symbol == symbol_while;
  else if (strcmp(non_terminal, "repeat_stmt") == 0)
    return symbol == symbol_repeat;
  else if (strcmp(non_terminal, "stmt") == 0)
    return symbol == symbol_id ||
           symbol == symbol_if ||
           symbol == symbol_while ||
           symbol == symbol_repeat ||
           symbol == symbol_null;
  else if (strcmp(non_terminal, "stmt_sequence") == 0)
    return symbol == symbol_id ||
           symbol == symbol_if ||
           symbol == symbol_while ||
           symbol == symbol_repeat ||
           symbol == symbol_null;
  else if (strcmp(non_terminal, "id_list") == 0)
    return symbol == symbol_id;
  else if (strcmp(non_terminal, "array_type") == 0)
    return symbol == symbol_array;
  else if (strcmp(non_terminal, "field_list") == 0)
    return symbol == symbol_id ||
           symbol == symbol_null;
  else if (strcmp(non_terminal, "record_type") == 0)
    return symbol == symbol_record;
  else if (strcmp(non_terminal, "type") == 0)
    return symbol == symbol_id ||
           symbol == symbol_array ||
           symbol == symbol_record;
  else if (strcmp(non_terminal, "formal_params_section") == 0)
    return symbol == symbol_id ||
           symbol == symbol_var;
  else if (strcmp(non_terminal, "formal_params") == 0)
    return symbol == symbol_open_paren;
  else if (strcmp(non_terminal, "proc_head") == 0)
    return symbol == symbol_proc;
  else if (strcmp(non_terminal, "proc_body") == 0)
    return symbol == symbol_end ||
           symbol == symbol_const ||
           symbol == symbol_type ||
           symbol == symbol_var ||
           symbol == symbol_proc ||
           symbol == symbol_begin;
  else if (strcmp(non_terminal, "proc_decl") == 0)
    return symbol == symbol_proc;
  else if (strcmp(non_terminal, "const_decl") == 0)
    return symbol == symbol_const;
  else if (strcmp(non_terminal, "type_decl") == 0)
    return symbol == symbol_type;
  else if (strcmp(non_terminal, "var_decl") == 0)
    return symbol == symbol_var;
  else if (strcmp(non_terminal, "declarations") == 0)
    return symbol == symbol_const ||
           symbol == symbol_type ||
           symbol == symbol_var ||
           symbol == symbol_proc ||
           symbol == symbol_begin;
  else if (strcmp(non_terminal, "module") == 0)
    return symbol == symbol_module;
  return false;
}

bool is_follow(const char *non_terminal, symbol_t symbol)
{
  if (strcmp(non_terminal, "selector") == 0)
    return symbol == symbol_times ||
           symbol == symbol_div ||
           symbol == symbol_mod ||
           symbol == symbol_and ||
           symbol == symbol_plus ||
           symbol == symbol_minus ||
           symbol == symbol_or ||
           symbol == symbol_equal ||
           symbol == symbol_not_equal ||
           symbol == symbol_less ||
           symbol == symbol_less_equal ||
           symbol == symbol_greater ||
           symbol == symbol_greater_equal ||
           symbol == symbol_comma ||
           symbol == symbol_close_paren ||
           symbol == symbol_close_bracket ||
           symbol == symbol_becomes ||
           symbol == symbol_of ||
           symbol == symbol_then ||
           symbol == symbol_do ||
           symbol == symbol_semicolon ||
           symbol == symbol_end ||
           symbol == symbol_else ||
           symbol == symbol_elsif ||
           symbol == symbol_until;
  else if (strcmp(non_terminal, "factor") == 0)
    return symbol == symbol_times ||
           symbol == symbol_div ||
           symbol == symbol_mod ||
           symbol == symbol_and ||
           symbol == symbol_plus ||
           symbol == symbol_minus ||
           symbol == symbol_or ||
           symbol == symbol_equal ||
           symbol == symbol_not_equal ||
           symbol == symbol_less ||
           symbol == symbol_less_equal ||
           symbol == symbol_greater ||
           symbol == symbol_greater_equal ||
           symbol == symbol_comma ||
           symbol == symbol_close_paren ||
           symbol == symbol_close_bracket ||
           symbol == symbol_becomes ||
           symbol == symbol_of ||
           symbol == symbol_then ||
           symbol == symbol_do ||
           symbol == symbol_semicolon ||
           symbol == symbol_end ||
           symbol == symbol_else ||
           symbol == symbol_elsif ||
           symbol == symbol_until;
  else if (strcmp(non_terminal, "term") == 0)
    return symbol == symbol_plus ||
           symbol == symbol_minus ||
           symbol == symbol_or ||
           symbol == symbol_equal ||
           symbol == symbol_not_equal ||
           symbol == symbol_less ||
           symbol == symbol_less_equal ||
           symbol == symbol_greater ||
           symbol == symbol_greater_equal ||
           symbol == symbol_comma ||
           symbol == symbol_close_paren ||
           symbol == symbol_close_bracket ||
           symbol == symbol_becomes ||
           symbol == symbol_of ||
           symbol == symbol_then ||
           symbol == symbol_do ||
           symbol == symbol_semicolon ||
           symbol == symbol_end ||
           symbol == symbol_else ||
           symbol == symbol_elsif ||
           symbol == symbol_until;
  else if (strcmp(non_terminal, "simple_expr") == 0)
    return symbol == symbol_plus ||
           symbol == symbol_minus ||
           symbol == symbol_or ||
           symbol == symbol_equal ||
           symbol == symbol_not_equal ||
           symbol == symbol_less ||
           symbol == symbol_less_equal ||
           symbol == symbol_greater ||
           symbol == symbol_greater_equal ||
           symbol == symbol_comma ||
           symbol == symbol_close_paren ||
           symbol == symbol_close_bracket ||
           symbol == symbol_becomes ||
           symbol == symbol_of ||
           symbol == symbol_then ||
           symbol == symbol_do ||
           symbol == symbol_semicolon ||
           symbol == symbol_end ||
           symbol == symbol_else ||
           symbol == symbol_elsif ||
           symbol == symbol_until;
  else if (strcmp(non_terminal, "expr") == 0)
    return symbol == symbol_comma ||
           symbol == symbol_close_paren ||
           symbol == symbol_close_bracket ||
           symbol == symbol_becomes ||
           symbol == symbol_of ||
           symbol == symbol_then ||
           symbol == symbol_do ||
           symbol == symbol_semicolon ||
           symbol == symbol_end ||
           symbol == symbol_else ||
           symbol == symbol_elsif ||
           symbol == symbol_until;
  else if (strcmp(non_terminal, "assignment") == 0)
    return symbol == symbol_semicolon ||
           symbol == symbol_end ||
           symbol == symbol_else ||
           symbol == symbol_elsif ||
           symbol == symbol_until;
  else if (strcmp(non_terminal, "actual_params") == 0)
    return symbol == symbol_null;
  else if (strcmp(non_terminal, "proc_call") == 0)
    return symbol == symbol_semicolon ||
           symbol == symbol_end ||
           symbol == symbol_else ||
           symbol == symbol_elsif ||
           symbol == symbol_until;
  else if (strcmp(non_terminal, "if_stmt") == 0)
    return symbol == symbol_null;
  else if (strcmp(non_terminal, "while_stmt") == 0)
    return symbol == symbol_null;
  else if (strcmp(non_terminal, "repeat_stmt") == 0)
    return symbol == symbol_null;
  else if (strcmp(non_terminal, "stmt") == 0)
    return symbol == symbol_semicolon ||
           symbol == symbol_end ||
           symbol == symbol_else ||
           symbol == symbol_elsif ||
           symbol == symbol_until;
  else if (strcmp(non_terminal, "stmt_sequence") == 0)
    return symbol == symbol_end ||
           symbol == symbol_else ||
           symbol == symbol_elsif ||
           symbol == symbol_until;
  else if (strcmp(non_terminal, "id_list") == 0)
    return symbol == symbol_null;
  else if (strcmp(non_terminal, "array_type") == 0)
    return symbol == symbol_null;
  else if (strcmp(non_terminal, "field_list") == 0)
    return symbol == symbol_semicolon ||
           symbol == symbol_end;
  else if (strcmp(non_terminal, "record_type") == 0)
    return symbol == symbol_null;
  else if (strcmp(non_terminal, "type") == 0)
    return symbol == symbol_close_paren ||
           symbol == symbol_semicolon;
  else if (strcmp(non_terminal, "formal_params_section") == 0)
    return symbol == symbol_close_paren ||
           symbol == symbol_semicolon;
  else if (strcmp(non_terminal, "formal_params") == 0)
    return symbol == symbol_semicolon;
  else if (strcmp(non_terminal, "proc_head") == 0)
    return symbol == symbol_semicolon;
  else if (strcmp(non_terminal, "proc_body") == 0)
    return symbol == symbol_semicolon;
  else if (strcmp(non_terminal, "proc_decl") == 0)
    return symbol == symbol_semicolon;
  else if (strcmp(non_terminal, "const_decl") == 0)
    return symbol == symbol_null;
  else if (strcmp(non_terminal, "type_decl") == 0)
    return symbol == symbol_null;
  else if (strcmp(non_terminal, "var_decl") == 0)
    return symbol == symbol_null;
  else if (strcmp(non_terminal, "declarations") == 0)
    return symbol == symbol_end ||
           symbol == symbol_begin;
  else if (strcmp(non_terminal, "module") == 0)
    return symbol == symbol_eof;
  return false;
}

bool scan()
{
  if (current_token.lexem.symbol == symbol_eof)
    return false;
  do {
    read_token();
  } while (current_token.lexem.symbol == symbol_null);
  return current_token.lexem.symbol == symbol_eof;
}

// Se “symbol_null” for passado como parâmetro, qualquer símbolo será reconhecido
bool verify(symbol_t symbol, bool mark_error, bool next)
{
  if (current_token.lexem.symbol == symbol || symbol == symbol_null) {
    if (should_log)
      mark(error_log, "\"%s\" found.", current_token.lexem.id);
    if (next)
      scan();
    return true;
  }
  if (mark_error)
    mark_missing(symbol);
  return false;
}

static inline bool consume(symbol_t symbol)
{
  return verify(symbol, true, true);
}

// Esta função faz o mesmo que “consume”, mas não avança para a próxima ficha léxica
static inline bool assert(symbol_t symbol)
{
  return verify(symbol, true, false);
}

// As versões com “try_” não mostram mensagens de erro caso não encontrem o símbolo passado como parâmetro
static inline bool try_consume(symbol_t symbol)
{
  return verify(symbol, false, true);
}

static inline bool try_assert(symbol_t symbol)
{
  return verify(symbol, false, false);
}

void expr(item_t *item);

// selector = {"." id | "[" expr "]"}
void selector(item_t *item, token_t entry_token)
{
  position_t position = entry_token.position;
  while (is_first("selector", current_token.lexem.symbol)) {
    if (try_consume(symbol_period)) {
      assert(symbol_id);
      // TODO: Remover as verificações para “item” quando possível
      if (!item || !item->type || item->type->form != form_record)
        mark_at(error_parser, position, "Invalid record.");
      else {
        entry_t *field = find_entry(current_token.lexem.id, item->type->fields);
        if (!field) {
          mark(error_parser, "\"%s\" is not a valid field.", current_token.lexem.id);
          // TODO: Mudar o endereçamento ao invés de anular o tipo?
          item->type = NULL;
        }
        else {
          // Lembrando: o endereço (“address”) do campo corresponde ao seu deslocamento com base no endereço do registro
          // em si e não ao seu endereço global atual
          if (item->addressing == addressing_indirect)
            write_field_offset(item, field->address);
          else
            item->address = item->address + field->address;
          item->type = field->type;
        }
      }
      position = current_token.position;
      scan();
    }
    else if (try_assert(symbol_open_bracket)) {
      position_t open_pos = current_token.position;
      scan();
      if (!item || !item->type || item->type->form != form_array)
        mark_at(error_parser, position, "Invalid array.");
      else {
        position_t index_pos = current_token.position;
        item_t index_item;
        expr(&index_item);
        if (item->addressing != addressing_indirect && index_item.addressing == addressing_immediate) {
          if (index_item.value < 0 || index_item.value > item->type->length - 1) {
            mark_at(error_parser, index_pos, "Index is out of bounds.");
            item->type = NULL;
          }
          else {
            item->address = item->address + (index_item.value * item->type->base->size);
            item->type = item->type->base;
          }
        }
        else {
          write_index_offset(item, &index_item);
          item->type = item->type->base;
        }
      }
      position = current_token.position;
      if (try_assert(symbol_close_bracket))
        scan();
      else
        mark(error_parser, "Missing \"]\" for (%d, %d).", open_pos.line, open_pos.column);
    }
  }
}

// factor = id selector | number | "(" expr ")" | "~" factor
void factor(item_t *item)
{
  if (try_assert(symbol_id)) {
    // TODO: Remover as verificações para “item” quando possível
    token_t entry_token = current_token;
    if (item)
      item->addressing = addressing_unknown;
    entry_t *entry = find_entry(current_token.lexem.id, symbol_table);
    if (!entry)
      mark(error_parser, "\"%s\" hasn't been declared yet.", current_token.lexem.id);
    else {
      // Como a tabela de símbolos armazena tanto variáveis e constantes, quanto tipos e procedimentos, é possível que o
      // identificador encontrado não seja um fator válido (variável ou constante)
      if (entry->class != class_var && entry->class != class_const)
        mark(error_parser, "\"%s\" is not a valid factor.", current_token.lexem.id);
      else {
        if (item) {
          item->addressing = (addressing_t)entry->class;
          item->address = entry->address;
          item->type = entry->type;
          item->value = entry->value;
        }
      }
    }
    scan();
    selector(item, entry_token);
  }
  else if (try_assert(symbol_number)) {
    if (item) {
      item->addressing = addressing_immediate;
      item->value = current_token.value;
    }
    scan();
  }
  else if (try_assert(symbol_open_paren)) {
    position_t p = current_token.position;
    scan();
    expr(item);
    if (try_assert(symbol_close_paren))
      scan();
    else
      mark(error_parser, "Missing \")\" for (%d, %d).", p.line, p.column);
  }
  else if (try_consume(symbol_not)) {
    factor(item);
    write_unary_op(symbol_not, item);
  }
  else {
    mark(error_parser, "Missing factor.");
    // Sincroniza
    while (!is_follow("factor", current_token.lexem.symbol) && scan());
  }
}

// term = factor {("*" | "div" | "mod" | "&") factor}
void term(item_t *item)
{
  factor(item);
  while (current_token.lexem.symbol >= symbol_times && current_token.lexem.symbol <= symbol_and) {
    symbol_t symbol = current_token.lexem.symbol;
    consume(symbol);
    // TODO: Gerar a instrução de salto para o caminho falso para o operador lógico “&”
    item_t rhs_item;
    factor(&rhs_item);
    write_binary_op(symbol, item, &rhs_item);
  }
}

// simple_expr = ["+" | "-"] term {("+" | "-" | "OR") term}
void simple_expr(item_t *item)
{
  if (try_consume(symbol_plus)) {
    term(item);
  } else if (try_consume(symbol_minus)) {
    term(item);
    write_unary_op(symbol_minus, item);
  } else {
    term(item);
  }
  while (current_token.lexem.symbol >= symbol_plus && current_token.lexem.symbol <= symbol_or) {
    symbol_t symbol = current_token.lexem.symbol;
    consume(symbol);
    // TODO: Gerar a instrução de salto para o caminho verdadeiro para o operador lógico “or”
    item_t rhs_item;
    term(&rhs_item);
    write_binary_op(symbol, item, &rhs_item);
  }
}

// expr = simple_expr [("=" | "#" | "<" | "<=" | ">" | ">=") simple_expr]
void expr(item_t *item)
{
  simple_expr(item);
  if (current_token.lexem.symbol >= symbol_equal && current_token.lexem.symbol <= symbol_greater_equal) {
    symbol_t symbol = current_token.lexem.symbol;
    consume(symbol);
    item_t rhs_item;
    simple_expr(&rhs_item);
    write_comparison(symbol, item, &rhs_item);
    item->type = boolean_type->type;
  }
}

// actual_params = "(" [expr {"," expr}] ")"
void actual_params()
{
  try_assert(symbol_open_paren);
  position_t open_pos = current_token.position;
  scan();
  if (is_first("expr", current_token.lexem.symbol)) {
    item_t item;
    expr(&item);
    while (try_consume(symbol_comma))
      expr(&item);
  }
  if (try_assert(symbol_close_paren))
    scan();
  else
    mark(error_parser, "Missing \")\" for (%d, %d).", open_pos.line, open_pos.column);
}

// proc_call = [actual_params]
void proc_call(entry_t *entry)
{
  if (is_first("actual_params", current_token.lexem.symbol))
    actual_params();
}

void stmt_sequence();

// if_stmt = "if" expr "then" stmt_sequence {"elsif" expr "then" stmt_sequence} ["else" stmt_sequence] "end"
void if_stmt()
{
  item_t expr_item, end_item;
  try_consume(symbol_if);
  expr(&expr_item);
  expr_item.links = NULL;
  write_inverse_branch(&expr_item, true);
  consume(symbol_then);
  stmt_sequence();
  // Este item serve como base para o salto para o final da estrutura condicional
  end_item.addressing = addressing_condition;
  end_item.condition = symbol_null;
  end_item.links = NULL;
  while (try_consume(symbol_elsif)) {
    write_branch(&end_item, true);
    write_label(&expr_item, NULL);
    fixup_links(&expr_item);
    expr(&expr_item);
    write_inverse_branch(&expr_item, true);
    consume(symbol_then);
    stmt_sequence();
  }
  if (try_consume(symbol_else)) {
    write_branch(&end_item, true);
    write_label(&expr_item, NULL);
    fixup_links(&expr_item);
    stmt_sequence();
  }
	write_label(&expr_item, NULL);
	fixup_links(&expr_item);
	if (end_item.links) {
		strcpy(end_item.label, expr_item.label);
		fixup_links(&end_item);
	}
  consume(symbol_end);
}

// while_stmt = "while" expr "do" stmt_sequence "end"
void while_stmt()
{
	item_t expr_item, back_item;
  try_consume(symbol_while);
  expr_item.links = NULL;
  expr(&expr_item);
  write_inverse_branch(&expr_item, true);
  consume(symbol_do);
  back_item.addressing = addressing_condition;
  back_item.condition = symbol_null;
  back_item.links = NULL;
  write_label(&back_item, NULL);
  stmt_sequence();
  write_branch(&back_item, false);
  write_label(&expr_item, NULL);
  fixup_links(&expr_item);
  consume(symbol_end);
}

// repeat_stmt = "repeat" stmt_sequence "until" expr
void repeat_stmt()
{
  try_consume(symbol_repeat);
  item_t expr_item;
  expr_item.links = NULL;
  write_label(&expr_item, NULL);
  stmt_sequence();
  consume(symbol_until);
  expr(&expr_item);
  write_inverse_branch(&expr_item, false);
}

// assignment = ":=" expr
void assignment(item_t *item)
{
  try_consume(symbol_becomes);
  item_t expr_item;
  expr(&expr_item);
  write_store(item, &expr_item);
}

// stmt = [id selector (assignment | proc_call) | if_stmt | while_stmt | repeat_stmt]
void stmt()
{
  if (try_assert(symbol_id)) {
    item_t item;
    item.addressing = addressing_unknown;
    token_t entry_token = current_token;
    entry_t *entry = find_entry(current_token.lexem.id, symbol_table);
    if (!entry)
      mark(error_parser, "\"%s\" hasn't been declared yet.", current_token.lexem.id);
    else {
      if (entry->class != class_var)
        mark(error_parser, "\"%s\" is not a variable.", entry->id);
      else {
        item.addressing = addressing_register;
        item.address = entry->address;
        item.type = entry->type;
      }
    }
    scan();
    selector(&item, entry_token);
    if (is_first("assignment", current_token.lexem.symbol))
      assignment(&item);
    else if (is_first("proc_call", current_token.lexem.symbol) || is_follow("proc_call", current_token.lexem.symbol))
      proc_call(entry);
    else {
      mark(error_parser, "Invalid statement.");
      // Sincroniza
      while (!is_follow("stmt", current_token.lexem.symbol) && scan());
    }
  }
  else if (is_first("if_stmt", current_token.lexem.symbol))
    if_stmt();
  else if (is_first("while_stmt", current_token.lexem.symbol))
    while_stmt();
  else if (is_first("repeat_stmt", current_token.lexem.symbol))
    repeat_stmt();
  if (!is_follow("stmt", current_token.lexem.symbol)) {
    mark(error_parser, "Missing \";\" or \"end\".");
    // Sincroniza
    while (!is_follow("stmt", current_token.lexem.symbol) && scan());
  }
}

// stmt_sequence = stmt {";" stmt}
void stmt_sequence()
{
  stmt();
  while (try_consume(symbol_semicolon))
    stmt();
}

// id_list = id {"," id}
entry_t *id_list()
{
  try_assert(symbol_id);
  entry_t *new_entries = create_entry(current_token.lexem.id, current_token.position, class_var);
  scan();
  while (try_consume(symbol_comma)) {
    if (assert(symbol_id)) {
      add_entry(create_entry(current_token.lexem.id, current_token.position, class_var), &new_entries);
      scan();
    }
  }
  return new_entries;
}

type_t *type();

// array_type = "array" expr "of" type
type_t *array_type()
{
  type_t *new_type = NULL;
  try_consume(symbol_array);
  new_type = create_type(form_array, 0, 0, NULL, NULL);
  //  expr();
  value_t length = 0;
  if (assert(symbol_number)) {
    length = current_token.value;
    scan();
  }
  consume(symbol_of);
  type_t *base_type = type();
  unsigned int size = 0;
  if (base_type)
    size = length * base_type->size;
  if (new_type) {
    new_type->length = length;
    new_type->size = size;
    new_type->base = base_type;
  }
  return new_type;
}

// field_list = [id_list ":" type]
entry_t *field_list()
{
  if (is_first("id_list", current_token.lexem.symbol)) {
    entry_t *new_fields = id_list();
    consume(symbol_colon);
    type_t *base_type = type();
    entry_t *e = new_fields;
    while (e) {
      e->type = base_type;
      e = e->next;
    }
    return new_fields;
  }
  return NULL;
}

// record_type = "record" field_list {";" field_list} "end"
type_t *record_type()
{
  type_t *new_type = NULL;
  try_consume(symbol_record);
  new_type = create_type(form_record, 0, 0, NULL, NULL);
  entry_t *fields = field_list();
  while (try_consume(symbol_semicolon)) {
    entry_t *more_fields = field_list();
    if (fields && more_fields)
      add_entry(more_fields, &fields);
  }
  // Efetua o cálculo do tamanho do tipo registro e dos deslocamentos de cada campo
  unsigned int size = 0;
  address_t offset = 0;
  if (fields) {
    entry_t *e = fields;
    while (e && e->type) {
      e->address = offset;
      offset += e->type->size;
      size += e->type->size;
      e = e->next;
    }
  }
  consume(symbol_end);
  if (new_type) {
    new_type->size = size;
    new_type->fields = fields;
  }
  return new_type;
}

// type = id | array_type | record_type
type_t *type()
{
  if (try_assert(symbol_id)) {
    // Qualquer tipo atômico deve ser baseado em um dos tipos internos da linguagem (neste caso apenas “integer”)
    entry_t *entry = find_entry(current_token.lexem.id, symbol_table);
    if (entry) {
      scan();
      return entry->type;
    } else {
      mark(error_parser, "Unknown type \"%s\".", current_token.lexem.id);
      scan();
      return NULL;
    }
  } else if (is_first("array_type", current_token.lexem.symbol)) {
    type_t *new_type = array_type();
    if (!new_type)
      mark(error_parser, "Invalid array type.");
    return new_type;
  } else if (is_first("record_type", current_token.lexem.symbol)) {
    type_t *new_type = record_type();
    if (!new_type)
      mark(error_parser, "Invalid record type.");
    return new_type;
  }
  // Sincroniza
  mark(error_parser, "Missing type.");
  while (!is_follow("type", current_token.lexem.symbol) && current_token.lexem.symbol != symbol_eof)
    scan();
  return NULL;
}

// formal_params_section = ["var"] id_list ":" type
entry_t *formal_params_section()
{
  if (try_consume(symbol_var));
  // TODO: Implementar a passagem de parâmetro por referência
  entry_t *new_params = id_list();
  if (!consume(symbol_colon))
    mark_missing(symbol_colon);
  type_t *base_type = type();
  entry_t *e = new_params;
  while (e) {
    e->type = base_type;
    e = e->next;
  }
  return new_params;
}

// formal_params = "(" [formal_params_section {";" formal_params_section}] ")"
entry_t *formal_params()
{
  entry_t *params = NULL;
  try_consume(symbol_open_paren);
  if (is_first("formal_params_section", current_token.lexem.symbol)) {
    params = formal_params_section();
    while (try_consume(symbol_semicolon))
      add_entry(formal_params_section(), &params);
  }
  consume(symbol_close_paren);
  return params;
}

// proc_head = "procedure" id [formal_params]
void proc_head()
{
  try_consume(symbol_proc);
  consume(symbol_id);
  // TODO: Implementar parâmetros para a entrada do procedimento na tabela de símbolos
  if (is_first("formal_params", current_token.lexem.symbol))
    formal_params();
}

void declarations();

// proc_body = declarations ["begin" stmt_sequence] "end" id
void proc_body()
{
  declarations();
  if (try_consume(symbol_begin))
    stmt_sequence();
  consume(symbol_end);
  consume(symbol_id);
}

// proc_decl = proc_head ";" proc_body
void proc_decl()
{
  proc_head();
  consume(symbol_semicolon);
  proc_body();
}

// const_decl = "const" {id "=" expr ";"}
void const_decl()
{
  try_consume(symbol_const);
  while (try_assert(symbol_id)) {
    entry_t *new_entry = create_entry(current_token.lexem.id, current_token.position, class_const);
    scan();
    consume(symbol_equal);
    // expr();
    if (assert(symbol_number)) {
      if (new_entry) {
        new_entry->value = current_token.value;
        add_entry(new_entry, &symbol_table);
      }
      scan();
    }
    consume(symbol_semicolon);
  }
}

// type_decl = "type" {id "=" type ";"}
void type_decl()
{
  try_consume(symbol_type);
  while (try_assert(symbol_id)) {
    entry_t *new_entry = create_entry(current_token.lexem.id, current_token.position, class_type);
    scan();
    // TODO: Melhorar erro para o “igual”
    consume(symbol_equal);
    type_t *base = type();
    if (new_entry && base) {
      new_entry->type = base;
      add_entry(new_entry, &symbol_table);
    }
    consume(symbol_semicolon);
  }
}

// var_decl = "var" {id_list ":" type ";"}
void var_decl()
{
  try_consume(symbol_var);
  while (is_first("id_list", current_token.lexem.symbol)) {
    entry_t *new_entries = id_list();
    consume(symbol_colon);
    type_t *base = type();
    entry_t *e = new_entries;
    while (e && base) {
      e->address = current_address;
      e->type = base;
      current_address += e->type->size;
      e = e->next;
    }
    add_entry(new_entries, &symbol_table);
    consume(symbol_semicolon);
  }
}

// declarations = [const_decl] [type_decl] [var_decl] {proc_decl ";"}
void declarations()
{
  if (is_first("const_decl", current_token.lexem.symbol))
    const_decl();
  if (is_first("type_decl", current_token.lexem.symbol))
    type_decl();
  if (is_first("var_decl", current_token.lexem.symbol))
    var_decl();
  while (is_first("proc_decl", current_token.lexem.symbol)) {
    proc_decl();
    consume(symbol_semicolon);
  }
}

// module = "module" id ";" declarations ["begin" stmt_sequence] "end" id "."
void module()
{
  consume(symbol_module);
  consume(symbol_id);
  consume(symbol_semicolon);
  declarations();
  if (try_consume(symbol_begin))
    stmt_sequence();
  consume(symbol_end);
  consume(symbol_id);
  consume(symbol_period);
}

// Retorna se a inicialização do analisador léxico e da tabela de símbolos obteve sucesso ou não e se o arquivo de
// entrada estava em branco
bool initialize_parser(FILE *file)
{
  should_log = false;
  if (!initialize_table(0, &symbol_table))
    return false;
  initialize_scanner(file);
  read_token();
  return current_token.lexem.symbol != symbol_eof;
}

bool parse()
{
  module();
  clear_table(&symbol_table);
  return true;
}
