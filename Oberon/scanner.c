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

token_t scanner_token;
token_t scanner_last_token;

position_t scanner_position;

const position_t position_zero = { .line = 0, .column = 0, .index = 0 };

char scanner_char;
char scanner_last_char;

FILE *input_file;

// Vetor com todas as palavras-chave da linguagem
lexem_t scanner_keywords[] = {
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
const unsigned int scanner_keywords_count = sizeof(scanner_keywords) / sizeof(lexem_t);

// Vetor com todos os operadores da linguagem
lexem_t scanner_operators[] = {
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
const unsigned int scanner_operators_count = sizeof(scanner_keywords) / sizeof(lexem_t);

// Vetor com todos os sinais de pontuação da linguagem
lexem_t scanner_punctuation[] = {
	{ .id = ".", .symbol = symbol_period },
	{ .id = ",", .symbol = symbol_colon },
	{ .id = ":", .symbol = symbol_comma },
	{ .id = ")", .symbol = symbol_close_paren },
	{ .id = "]", .symbol = symbol_close_bracket },
	{ .id = "(", .symbol = symbol_open_paren },
	{ .id = "[", .symbol = symbol_open_bracket },
	{ .id = ";", .symbol = symbol_semicolon }
};
const unsigned int scanner_punctuation_count = sizeof(scanner_keywords) / sizeof(lexem_t);

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
	while (index < scanner_keywords_count && strcasecmp(scanner_keywords[index].id, id) != 0)
		index++;
	if (index < scanner_keywords_count) {
		if (symbol)
			*symbol = scanner_keywords[index].symbol;
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
	for (unsigned int index = 0; index < scanner_keywords_count; index++)
		if (scanner_keywords[index].symbol == symbol)
			return scanner_keywords[index].id;
	for (unsigned int index = 0; index < scanner_operators_count; index++)
		if (scanner_operators[index].symbol == symbol)
			return scanner_operators[index].id;
	for (unsigned int index = 0; index < scanner_punctuation_count; index++)
		if (scanner_punctuation[index].symbol == symbol)
			return scanner_punctuation[index].id;
	return "unknown";
}

// A razão de se criar uma função somente para isto é aproveitá-la se a codificação do arquivo de código-fonte mudar
bool scanner_step()
{
	scanner_last_char = scanner_char;
	if (fread(&scanner_char, sizeof(char), 1, input_file) == sizeof(char)) {
		if (is_newline(scanner_char, scanner_last_char)) {
			scanner_position.line++;
			scanner_position.column = 0;
		} else scanner_position.column++;
		scanner_position.index++;
		return true;
	}
	return false;
}

//
// ATENÇÃO: todas as funções do analisador léxico devem garantir que “scanner_char” termine com o caractere subsequente
// ao lexema reconhecido. Por exemplo, ao analisar “var x: integer”, a função “id“ será a primeira a ser invocada para
// reconhecer “var”. Ao terminar, “scanner_char” deve conter o espaço em branco entre “var” e “x”
//
// As funções “id”, “integer” e “number” fazem parte da EBNF e deveriam ser consideradas parte do analisador sintático.
// No entanto, pela forma com que o compilador está definido, o reconhecimento de lexemas também é estipulado pela EBNF
// fazendo com que a análise léxica seja realizada por um “mini descendente recursivo” ao invés de um autômato finito
//

void id()
{
	unsigned int index = 0;
	scanner_token.position = scanner_position;
	while (index < SCANNER_MAX_ID_LENGTH && (is_letter(scanner_char) || is_digit(scanner_char))) {
		scanner_token.lexem.id[index++] = scanner_char;
		if (!scanner_step())
			break;
	}
	// O tamanho máximo para um identificador é especificado por “id_length”, a variável “scanner_id” possui tamanho
	// “id_length + 1” e por isso o caractere terminador pode ser incluído mesmo que o limite seja alcançado
	scanner_token.lexem.id[index] = '\0';
	if (!is_keyword(scanner_token.lexem.id, &scanner_token.lexem.symbol))
		scanner_token.lexem.symbol = symbol_id;
}

// FAZER: Adicionar verificação se o número é muito longo
void integer()
{
	unsigned int index = 0;
	scanner_token.position = scanner_position;
	scanner_token.value = 0;
	identifier_t id;
	while (index < SCANNER_MAX_ID_LENGTH && is_digit(scanner_char)) {
		id[index] = scanner_char;
		scanner_token.lexem.id[index] = scanner_char;
		index++;
		// Efetua o cálculo do valor, dígito-a-dígito, com base nos caracteres lidos
		scanner_token.value = 10 * scanner_token.value + (scanner_char - '0');
		if (!scanner_step())
			break;
	}
	scanner_token.lexem.id[index] = '\0';
	scanner_token.lexem.symbol = symbol_number;
	// Avalia se há caracteres inválidos após os dígitos do número
	bool invalid_ending = false;
	while (index < SCANNER_MAX_ID_LENGTH && (is_letter(scanner_char) || scanner_char == '_')) {
		id[index++] = scanner_char;
		invalid_ending	= true;
		if (!scanner_step())
			break;
	}
	if (invalid_ending)
		mark(error_warning, scanner_token.position, "\"%s\" is not a number. Assuming \"%s\".", id, scanner_token.lexem.id);
}

// Por definição, somente números positivos inteiros são reconhecidos
void number()
{
	integer();
}

// Ao entrar nesta função, o analisador léxico já encontrou os caracteres "(*" que iniciam o comentário e “scanner_char”
// possui o asterisco como valor
void comment()
{
	char previous_char = scanner_char;
	scanner_token.position = scanner_position;
	while (scanner_step()) {
		// Comentários aninhados
		if (scanner_char == '*' && previous_char == '(')
			comment();
		// Fim do comentário
		if (scanner_char == ')' && previous_char == '*') {
			scanner_step();
			return;
		}
		previous_char = scanner_char;
	}
	mark(error_fatal, scanner_token.position, "Endless comment detected.");
	scanner_token.lexem.symbol = symbol_eof;
}

void scanner_get()
{
	scanner_last_token = scanner_token;
	// Salta os caracteres em branco, incluindo símbolos de quebra de linha
	while (is_blank(scanner_char))
		scanner_step();
	if (feof(input_file)) {
		strcpy(scanner_token.lexem.id, "EOF");
		scanner_token.lexem.symbol = symbol_eof;
		return;
	}
	// Os casos de um identificador ou um número são considerados separadamente para que o código no “switch” não precise
	// incluir uma chamada a “scanner_step” em cada “case”
	if (is_letter(scanner_char)) {
		id();
		return;
	} else if (is_digit(scanner_char)) {
		number();
		return;
	}
	scanner_token.position = scanner_position;
	scanner_token.lexem.id[0] = scanner_char;
	switch (scanner_token.lexem.id[0]) {
		case '&': scanner_token.lexem.symbol = symbol_and;						break;
		case '*': scanner_token.lexem.symbol = symbol_times;					break;
		case '+': scanner_token.lexem.symbol = symbol_plus;						break;
		case '-': scanner_token.lexem.symbol = symbol_minus;					break;
		case '=': scanner_token.lexem.symbol = symbol_equal;					break;
		case '#': scanner_token.lexem.symbol = symbol_not_equal;			break;
		case '<': scanner_token.lexem.symbol = symbol_less;						break;
		case '>': scanner_token.lexem.symbol = symbol_greater;				break;
		case ';': scanner_token.lexem.symbol = symbol_semicolon;			break;
		case ',':	scanner_token.lexem.symbol = symbol_comma;					break;
		case ':': scanner_token.lexem.symbol = symbol_colon;					break;
		case '.': scanner_token.lexem.symbol = symbol_period;					break;
		case '(': scanner_token.lexem.symbol = symbol_open_paren;			break;
		case ')': scanner_token.lexem.symbol = symbol_close_paren;		break;
		case '[': scanner_token.lexem.symbol = symbol_open_bracket;		break;
		case ']': scanner_token.lexem.symbol = symbol_close_bracket;	break;
		case '~': scanner_token.lexem.symbol = symbol_not;						break;
		default:	scanner_token.lexem.symbol = symbol_null;						break;
	}
	scanner_token.lexem.id[1] = '\0';
	scanner_step();
	if (scanner_token.lexem.symbol == symbol_null) {
		mark(error_scanner, scanner_token.position, "\"%s\" is not a valid symbol.", scanner_token.lexem.id);
		return;
	}
	// Os casos abaixo representam os lexemas com mais de um caracter (como “>=”, “:=” etc.)
	if (scanner_token.lexem.symbol == symbol_less && scanner_char == '=') {
		scanner_token.lexem.id[1] = '=';
		scanner_token.lexem.id[2] = '\0';
		scanner_step();
		scanner_token.lexem.symbol = symbol_less_equal;
	} else if (scanner_token.lexem.symbol == symbol_greater && scanner_char == '=') {
		scanner_token.lexem.id[1] = '=';
		scanner_token.lexem.id[2] = '\0';
		scanner_step();
		scanner_token.lexem.symbol = symbol_greater_equal;
	} else if (scanner_token.lexem.symbol == symbol_colon && scanner_char == '=') {
		scanner_token.lexem.id[1] = '=';
		scanner_token.lexem.id[2] = '\0';
		scanner_step();
		scanner_token.lexem.symbol = symbol_becomes;
	} else if (scanner_token.lexem.symbol == symbol_open_paren	&& scanner_char == '*') {
		scanner_step();
		// Ignora os caracteres entre “(*” e “*)” como sendo comentários e entra novamente na função para buscar o próximo
		// lexema válido
		comment();
		scanner_get();
	}
}

void scanner_initialize(FILE *file)
{
	input_file = file;
	strcpy(scanner_token.lexem.id, "");
	scanner_token.position = position_zero;
	scanner_token.lexem.symbol = symbol_null;
	scanner_token.value = 0;
	scanner_position.line = 1;
	scanner_position.column = 0;
	scanner_position.index = 0;
	scanner_char = '\0';
	scanner_step();
}
