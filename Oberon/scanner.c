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

#include "scanner.h"

//
// Constantes, variáveis e definições gerais
//

// O lexema
token_t scanner_token;

// Tratamento de erros na análise léxica
boolean_t scanner_error;
position_t scanner_error_position;

// O ponteiro para o arquivo com o código-fonte e o caractere atual
file_t scanner_file;
char scanner_char;

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
const index_t scanner_keywords_count = sizeof(scanner_keywords) / sizeof(lexem_t);

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
const index_t scanner_operators_count = sizeof(scanner_keywords) / sizeof(lexem_t);

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
const index_t scanner_punctuation_count = sizeof(scanner_keywords) / sizeof(lexem_t);

//
// Analisador léxico
//

// As funções “is_letter”, “is_digit” e “is_blank” chamam as versões internas da linguagem C. Por enquanto...
boolean_t is_letter(char c) {
	if (isalpha(c))
		return true;
	return false;
}

boolean_t is_digit(char c) {
	if (isdigit(c))
		return true;
	return false;
}

boolean_t is_blank(char c) {
	if (isspace(c))
		return true;
	return false;
}

// Esta função é responsável por verificar se o identificar “id” é uma palavra reservada ou não
// O símbolo equivalente à palavra reservada é armazenado via referência no parâmetro “symbol”
boolean_t is_keyword(id_t id, symbol_t *symbol) {
	index_t index = 0;
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

string_t id_for_symbol(symbol_t symbol) {
	for (index_t index = 0; index < scanner_keywords_count; index++)
		if (scanner_keywords[index].symbol == symbol)
			return scanner_keywords[index].id;
	for (index_t index = 0; index < scanner_operators_count; index++)
		if (scanner_operators[index].symbol == symbol)
			return scanner_operators[index].id;
	for (index_t index = 0; index < scanner_punctuation_count; index++)
		if (scanner_punctuation[index].symbol == symbol)
			return scanner_punctuation[index].id;
	return "Unknown";
}

// A razão de se criar uma função somente para isto é aproveitá-la se a codificação do arquivo de código-fonte mudar
boolean_t scanner_step() {
	if (fread(&scanner_char, sizeof(char), 1, scanner_file) == sizeof(char))
		return true;
	return false;
}

// Esta função aponta que um erro aconteceu usando a mensagem de parâmetro e a posição atual no arquivo
void scanner_mark(const string_t message) {
	position_t position;
	fgetpos(scanner_file, &position);
	// Como a posição do último erro encontrado ficou para trás, este erro é novo
	if (position > scanner_error_position)
		printf("Erro %llu: %s\n", position, message);
	scanner_error_position = position;
	scanner_error = true;
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

void id() {
	index_t index = 0;
	while (index < SCANNER_MAX_ID_LENGTH && (is_letter(scanner_char) || is_digit(scanner_char))) {
		scanner_token.id[index++] = scanner_char;
		if (!scanner_step())
			break;
	}
	// O tamanho máximo para um identificador é especificado por “id_length”, a variável “scanner_id” possui tamanho
	// “id_length + 1” e por isso o caractere terminador pode ser incluído mesmo que o limite seja alcançado
	scanner_token.id[index] = '\0';
	if (!is_keyword(scanner_token.id, &scanner_token.symbol))
		scanner_token.symbol = symbol_id;
}

// FAZER: Adicionar verificação se o número é muito longo
void integer() {
	index_t index = 0;
	scanner_token.value = 0;
	while (index < SCANNER_MAX_ID_LENGTH && is_digit(scanner_char)) {
		scanner_token.id[index++] = scanner_char;
		// Efetua o cálculo do valor, dígito-a-dígito, com base nos caracteres lidos
		scanner_token.value = 10 * scanner_token.value + (scanner_char - '0');
		if (!scanner_step())
			break;
	}
	scanner_token.id[index] = '\0';
	scanner_token.symbol = symbol_number;
}

// Por definição, somente números positivos inteiros são reconhecidos
void number() {
	integer();
}

// Ao entrar nesta função, o analisador léxico já encontrou os caracteres "(*" que iniciam o comentário e “scanner_char”
// possui o asterisco como valor
void comment() {
	char last_char = scanner_char;
	while (scanner_step()) {
		// Comentários aninhados
		if (scanner_char == '*' && last_char == '(')
			comment();
		// Fim do comentário
		if (scanner_char == ')' && last_char == '*') {
			scanner_step();
			return;
		}
		last_char = scanner_char;
	}
	scanner_mark("Comentário sem fim. Bazinga!");
	scanner_token.symbol = symbol_eof;
}

void scanner_get() {
	// Salta os caracteres em branco, incluindo símbolos de quebra de linha
	while (is_blank(scanner_char))
		scanner_step();
	if (feof(scanner_file)) {
		strcpy(scanner_token.id, "EOF");
		scanner_token.symbol = symbol_eof;
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
	scanner_token.id[0] = scanner_char;
	switch (scanner_token.id[0]) {
		case '&': scanner_token.symbol = symbol_and;						break;
		case '*': scanner_token.symbol = symbol_times;					break;
		case '+': scanner_token.symbol = symbol_plus;						break;
		case '-': scanner_token.symbol = symbol_minus;					break;
		case '=': scanner_token.symbol = symbol_equal;					break;
		case '#': scanner_token.symbol = symbol_not_equal;			break;
		case '<': scanner_token.symbol = symbol_less;						break;
		case '>': scanner_token.symbol = symbol_greater;				break;
		case ';': scanner_token.symbol = symbol_semicolon;			break;
		case ',':	scanner_token.symbol = symbol_comma;					break;
		case ':': scanner_token.symbol = symbol_colon;					break;
		case '.': scanner_token.symbol = symbol_period;					break;
		case '(': scanner_token.symbol = symbol_open_paren;			break;
		case ')': scanner_token.symbol = symbol_close_paren;		break;
		case '[': scanner_token.symbol = symbol_open_bracket;		break;
		case ']': scanner_token.symbol = symbol_close_bracket;	break;
		case '~': scanner_token.symbol = symbol_not;						break;
		default:	scanner_token.symbol = symbol_null;						break;
	}
	scanner_token.id[1] = '\0';
	scanner_step();
	// Os casos abaixo representam os lexemas com mais de um caracter (como “>=”, “:=” etc.)
	if (scanner_token.symbol == symbol_less && scanner_char == '=') {
		scanner_token.id[1] = '=';
		scanner_token.id[2] = '\0';
		scanner_step();
		scanner_token.symbol = symbol_less_equal;
	} else if (scanner_token.symbol == symbol_greater && scanner_char == '=') {
		scanner_token.id[1] = '=';
		scanner_token.id[2] = '\0';
		scanner_step();
		scanner_token.symbol = symbol_greater_equal;
	} else if (scanner_token.symbol == symbol_colon && scanner_char == '=') {
		scanner_token.id[1] = '=';
		scanner_token.id[2] = '\0';
		scanner_step();
		scanner_token.symbol = symbol_becomes;
	} else if (scanner_token.symbol == symbol_open_paren	&& scanner_char == '*') {
		scanner_step();
		// Ignora os caracteres entre “(*” e “*)” como sendo comentários e entra novamente na função para buscar o próximo
		// lexema válido
		comment();
		scanner_get();
	}
}

void scanner_initialize(file_t file, position_t position) {
	scanner_file = file;
	scanner_error = false;
	scanner_error_position = position;
	scanner_step();
}