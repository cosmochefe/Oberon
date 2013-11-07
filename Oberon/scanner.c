//
//  scanner.c
//  Oberon
//
//  Created by Alvaro Costa Neto on 10/12/13.
//  Copyright (c) 2013 Alvaro Costa Neto. All rights reserved.
//

//
//	Vocabulário:
//
//	- * div mod & + - or
//	- = # < <= > >= . , : ) ]
//	- of then do until ( [ ~ := ;
//	- end else elsif if while repeat
//	- array record const type var procedure begin module
//
// Conjuntos first(K), com ø indicando o vazio:
//
//	- selector = . { ø
//	- factor = ( ~ number id
//	- term = ( ~ number id
//	- simple_expr = + - ( ~ number id
//	- expr = + - ( ~ number id
//	- assignment = id
//	- proc_call = id
//	- stmt = id if while repeat ø
//	- stmt_sequence = id if while repeat ø
//	- field_list = id ø
//	- type = id array record
//	- formal_params_section = id var
//	- formal_params = (
//	- proc_head = procedure
//	-	proc_body = end const type var procedure begin
//	- proc_decl = procedure
//	- declarations = const type var procedure ø
//	- module = module
//
// Conjuntos follow(K), com ø indicando o vazio:
//
//	- selector = * div mod & + - or = # < <= > >= , ) ] := of then do ; end else elsif until
//	- factor = * div mod & + - or = # < <= > >= , ) ] of then do ; end else elsif until
//	- term = + - or = # < <= > >= , ) ] of then do ; end else elsif until
//	- simple_expr = = # < <= > >= , ) ] of then do ; end else elsif until
//	- expr = , ) ] of then do ; end else elsif until
//	- assignment = ; end else elsif until
//	- proc_call = ; end else elsif until
//	- stmt = ; end else elsif until
//	- stmt_sequence = end else elsif until
//	- field_list = ; end
//	- type = ) ;
//	- formal_params_section = ) ;
//	- formal_params = ;
//	- proc_head = ;
//	-	proc_body = ;
//	- proc_decl = ;
//	- declarations = end begin
//

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "errors.h"
#include "scanner.h"

FILE *input_file;

// Variávies e constantes globais
token_t current_token, last_token;
const position_t position_zero = { .line = 0, .column = 0, .index = 0 };

char current_char, last_char;
position_t current_position;

// Vetor com todas as palavras-chave da linguagem
lexem_t keywords[] = {
	{ .id = "do",					.symbol = symbol_do },
	{ .id = "if",					.symbol = symbol_if },
	{ .id = "of",					.symbol = symbol_of },
	{ .id = "or",					.symbol = symbol_or },
	{ .id = "end",				.symbol = symbol_end },
	{ .id = "mod",				.symbol = symbol_mod },
	{ .id = "var",				.symbol = symbol_var },
	{ .id = "else",				.symbol = symbol_else },
	{ .id = "then",				.symbol = symbol_then },
	{ .id = "type",				.symbol = symbol_type },
	{ .id = "array",			.symbol = symbol_array },
	{ .id = "begin",			.symbol = symbol_begin },
	{ .id = "const",			.symbol = symbol_const },
	{ .id = "elsif",			.symbol = symbol_elsif },
	{ .id = "until",			.symbol = symbol_until },
	{ .id = "while",			.symbol = symbol_while },
	{ .id = "record",			.symbol = symbol_record },
	{ .id = "repeat",			.symbol = symbol_repeat },
	{ .id = "procedure",	.symbol = symbol_proc },
	{ .id = "div",				.symbol = symbol_div },
	{ .id = "module",			.symbol = symbol_module }
};
const unsigned int keywords_count = sizeof(keywords) / sizeof(lexem_t);

// Vetor com todos os operadores da linguagem
lexem_t operators[] = {
	{ .id = "*",	.symbol = symbol_times },
	{ .id = "&",	.symbol = symbol_and },
	{ .id = "+",	.symbol = symbol_plus },
	{ .id = "-",	.symbol = symbol_minus },
	{ .id = "=",	.symbol = symbol_equal },
	{ .id = "#",	.symbol = symbol_not_equal },
	{ .id = "<",	.symbol = symbol_less },
	{ .id = "<=",	.symbol = symbol_less_equal },
	{ .id = ">",	.symbol = symbol_greater },
	{ .id = ">=",	.symbol = symbol_greater_equal },
	{ .id = "~",	.symbol = symbol_not },
	{ .id = ":=",	.symbol = symbol_becomes }
};
const unsigned int operators_count = sizeof(keywords) / sizeof(lexem_t);

// Vetor com todos os sinais de pontuação da linguagem
lexem_t punctuation[] = {
	{ .id = ".", .symbol = symbol_period },
	{ .id = ",", .symbol = symbol_colon },
	{ .id = ":", .symbol = symbol_comma },
	{ .id = ")", .symbol = symbol_close_paren },
	{ .id = "]", .symbol = symbol_close_bracket },
	{ .id = "(", .symbol = symbol_open_paren },
	{ .id = "[", .symbol = symbol_open_bracket },
	{ .id = ";", .symbol = symbol_semicolon }
};
const unsigned int punctuation_count = sizeof(keywords) / sizeof(lexem_t);

//
// Analisador léxico
//

// As funções “is_letter”, “is_digit” e “is_blank” chamam as versões internas da linguagem C. Por enquanto...
bool is_letter(char c)
{
	return isalpha(c);
}

bool is_digit(char c)
{
	return isdigit(c);
}

bool is_blank(char c)
{
	return isspace(c);
}

bool is_newline(char c, char p)
{
	return (c == '\n' && p != '\r') || c == '\r';
}

// Esta função é responsável por verificar se o identificar “id” é uma palavra reservada ou não
// O símbolo equivalente à palavra reservada é armazenado via referência no parâmetro “symbol”
bool is_keyword(identifier_t id, symbol_t *symbol)
{
	unsigned int index = 0;
	while (index < keywords_count && strcasecmp(keywords[index].id, id) != 0)
		index++;
	if (index < keywords_count) {
		if (symbol)
			*symbol = keywords[index].symbol;
		return true;
	}
	if (symbol)
		*symbol = symbol_null;
	return false;
}

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

char *id_for_symbol(symbol_t symbol)
{
	for (unsigned int index = 0; index < keywords_count; index++)
		if (keywords[index].symbol == symbol)
			return keywords[index].id;
	for (unsigned int index = 0; index < operators_count; index++)
		if (operators[index].symbol == symbol)
			return operators[index].id;
	for (unsigned int index = 0; index < punctuation_count; index++)
		if (punctuation[index].symbol == symbol)
			return punctuation[index].id;
	return "unknown";
}

symbol_t inverse_condition(symbol_t symbol)
{
	switch (symbol) {
		case symbol_equal: return symbol_not_equal; break;
		case symbol_not_equal: return symbol_equal; break;
		case symbol_less: return symbol_greater_equal; break;
		case symbol_less_equal: return symbol_greater; break;
		case symbol_greater: return symbol_less_equal; break;
		case symbol_greater_equal: return symbol_less; break;
		default: break;
	}
	return symbol_null;
}

// A razão de se criar uma função somente para isto é aproveitá-la se a codificação do arquivo de código-fonte mudar
bool read_char()
{
	last_char = current_char;
	if (fread(&current_char, sizeof(char), 1, input_file) == sizeof(char)) {
		if (is_newline(current_char, last_char)) {
			current_position.line++;
			current_position.column = 0;
		} else current_position.column++;
		current_position.index++;
		return true;
	}
	return false;
}

//
// ATENÇÃO: todas as funções do analisador léxico devem garantir que “current_char” termine com o caractere subsequente
// ao lexema reconhecido. Por exemplo, ao analisar “var x: integer”, a função “id“ será a primeira a ser invocada para
// reconhecer “var”. Ao terminar, “current_char” deve conter o espaço em branco entre “var” e “x”
//
// As funções “id”, “integer” e “number” fazem parte da EBNF e deveriam ser consideradas parte do analisador sintático.
// No entanto, pela forma com que o compilador está definido, o reconhecimento de lexemas também é estipulado pela EBNF
// fazendo com que a análise léxica seja realizada por um “mini descendente recursivo” ao invés de um autômato finito
//

void id()
{
	unsigned int index = 0;
	current_token.position = current_position;
	while (index < SCANNER_MAX_ID_LENGTH && (is_letter(current_char) || is_digit(current_char))) {
		current_token.lexem.id[index++] = current_char;
		if (!read_char())
			break;
	}
	// O tamanho máximo para um identificador é especificado por “id_length”, a variável “scanner_id” possui tamanho
	// “id_length + 1” e por isso o caractere terminador pode ser incluído mesmo que o limite seja alcançado
	current_token.lexem.id[index] = '\0';
	if (!is_keyword(current_token.lexem.id, &current_token.lexem.symbol))
		current_token.lexem.symbol = symbol_id;
}

// TODO: Adicionar verificação se o número é muito longo
void integer()
{
	unsigned int index = 0;
	current_token.position = current_position;
	current_token.value = 0;
	identifier_t id;
	while (index < SCANNER_MAX_ID_LENGTH && is_digit(current_char)) {
		id[index] = current_char;
		current_token.lexem.id[index] = current_char;
		index++;
		// Efetua o cálculo do valor, dígito-a-dígito, com base nos caracteres lidos
		current_token.value = 10 * current_token.value + (current_char - '0');
		if (!read_char())
			break;
	}
	current_token.lexem.id[index] = '\0';
	current_token.lexem.symbol = symbol_number;
	// Avalia se há caracteres inválidos após os dígitos do número
	bool invalid_ending = false;
	while (index < SCANNER_MAX_ID_LENGTH && (is_letter(current_char) || current_char == '_')) {
		id[index++] = current_char;
		invalid_ending	= true;
		if (!read_char())
			break;
	}
	if (invalid_ending)
		mark(error_warning, "\"%s\" is not a number. Assuming \"%s\".", id, current_token.lexem.id);
}

// Por definição, somente números positivos inteiros são reconhecidos
void number()
{
	integer();
}

// Ao entrar nesta função, o analisador léxico já encontrou os caracteres "(*" que iniciam o comentário e “current_char”
// possui o asterisco como valor
void comment()
{
	current_token.position = current_position;
	while (read_char()) {
		// Comentários aninhados
		if (current_char == '*' && last_char == '(')
			comment();
		// Fim do comentário
		if (current_char == ')' && last_char == '*') {
			read_char();
			return;
		}
	}
	mark(error_fatal, "Endless comment detected.");
	current_token.lexem.symbol = symbol_eof;
}

void read_token()
{
	last_token = current_token;
	// Salta os caracteres em branco, incluindo símbolos de quebra de linha
	while (is_blank(current_char))
		read_char();
	if (feof(input_file)) {
		strcpy(current_token.lexem.id, "EOF");
		current_token.lexem.symbol = symbol_eof;
		return;
	}
	// Os casos de um identificador ou um número são considerados separadamente para que o código no “switch” não precise
	// incluir uma chamada a “read_char” em cada “case”
	if (is_letter(current_char)) {
		id();
		return;
	} else if (is_digit(current_char)) {
		number();
		return;
	}
	current_token.position = current_position;
	current_token.lexem.id[0] = current_char;
	switch (current_token.lexem.id[0]) {
		case '&': current_token.lexem.symbol = symbol_and;						break;
		case '*': current_token.lexem.symbol = symbol_times;					break;
		case '+': current_token.lexem.symbol = symbol_plus;						break;
		case '-': current_token.lexem.symbol = symbol_minus;					break;
		case '=': current_token.lexem.symbol = symbol_equal;					break;
		case '#': current_token.lexem.symbol = symbol_not_equal;			break;
		case '<': current_token.lexem.symbol = symbol_less;						break;
		case '>': current_token.lexem.symbol = symbol_greater;				break;
		case ';': current_token.lexem.symbol = symbol_semicolon;			break;
		case ',':	current_token.lexem.symbol = symbol_comma;					break;
		case ':': current_token.lexem.symbol = symbol_colon;					break;
		case '.': current_token.lexem.symbol = symbol_period;					break;
		case '(': current_token.lexem.symbol = symbol_open_paren;			break;
		case ')': current_token.lexem.symbol = symbol_close_paren;		break;
		case '[': current_token.lexem.symbol = symbol_open_bracket;		break;
		case ']': current_token.lexem.symbol = symbol_close_bracket;	break;
		case '~': current_token.lexem.symbol = symbol_not;						break;
		default:	current_token.lexem.symbol = symbol_null;						break;
	}
	current_token.lexem.id[1] = '\0';
	read_char();
	if (current_token.lexem.symbol == symbol_null) {
		mark(error_scanner, "\"%s\" is not a valid symbol.", current_token.lexem.id);
		return;
	}
	// Os casos abaixo representam os lexemas com mais de um caracter (como “>=”, “:=” etc.)
	if (current_token.lexem.symbol == symbol_less && current_char == '=') {
		current_token.lexem.id[1] = '=';
		current_token.lexem.id[2] = '\0';
		read_char();
		current_token.lexem.symbol = symbol_less_equal;
	} else if (current_token.lexem.symbol == symbol_greater && current_char == '=') {
		current_token.lexem.id[1] = '=';
		current_token.lexem.id[2] = '\0';
		read_char();
		current_token.lexem.symbol = symbol_greater_equal;
	} else if (current_token.lexem.symbol == symbol_colon && current_char == '=') {
		current_token.lexem.id[1] = '=';
		current_token.lexem.id[2] = '\0';
		read_char();
		current_token.lexem.symbol = symbol_becomes;
	} else if (current_token.lexem.symbol == symbol_open_paren	&& current_char == '*') {
		read_char();
		// Ignora os caracteres entre “(*” e “*)” como sendo comentários e entra novamente na função para buscar o próximo
		// lexema válido
		comment();
		read_token();
	}
}

void initialize_scanner(FILE *file)
{
	input_file = file;
	strcpy(current_token.lexem.id, "");
	current_token.position = position_zero;
	current_token.lexem.symbol = symbol_null;
	current_token.value = 0;
	current_position.line = 1;
	current_position.column = 0;
	current_position.index = 0;
	current_char = '\0';
	read_char();
}
