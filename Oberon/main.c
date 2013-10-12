//
//  main.c
//  Oberon
//
//  Created by Alvaro Costa Neto on 10/11/13.
//  Copyright (c) 2013 Alvaro Costa Neto. All rights reserved.
//

//
// Regras para produção de código para o compilador (não-terminal K):
//
//	- "x": if (s == 'x') next(); else error();
//	- (exp): evaluate(exp);
//	- [exp]: if (set_includes(s, first_set(exp)) evaluate(exp);
//	- {exp}: while (set_includes(s, first_set(exp)) evaluate(exp);
//	- f0 f1 … fn: evaluate(f0); evaluate(f1); … evaluate(fn);
//	- t0 | t1 | … | tn: if (set_includes(s, first_set(t0)) evaluate(t0);
//											else if (set_includes(s, first_set(t1)) evaluate(t1);
//											…
//											else if (set_includes(s, first_set(tn)) evaluate(tn);
//
// Condições que devem ser respeitadas para manter o determinismo da gramática (não-terminal K):
//
//	- t0 | t1: set_union(first_set(t0), first_set(t1)) == EMPTY_SET;
//	- f0 f1: if (set_includes(EMPTY, first_set(f0)) set_union(first_set(t0), first_set(t1)) == EMPTY_SET;
//	- [exp] ou {exp}: set_union(first_set(exp), follow_set(K)) == EMPTY_SET;
//
// EBNF da linguagem Oberon-0 (para “letter” e “digit”, usar as funções “is_alpha”, “is_alnum” e “is_digit”):
//	- id = letter {letter | digit}
//	- integer = digit {digit}
//	- selector = {"." id | "[" exp "]"}
//	- number = integer
//	- factor = id selector | number | "(" exp ")" | "~" factor
//	- term = factor {("*" | "div" | "mod" | "&") factor}
//	- simple_exp = ["+" | "-"] term {"+" | "-" | "OR") term}
//	- exp = simple_exp [("=" | "#" | "<" | "<=" | ">" | ">=") simple_exp]
//	- assignment = ident selector ":=" exp
//	- actual_params = "(" [exp {"," exp}] ")"
//	- proc_call = ident selector [actual_params]
//	- if_stmt = "if" exp "then" stmt_sequence {"elsif" exp "then" stmt_sequence} ["else" stmt_sequence] "end"
//	- while_stmt = "while" exp "do" stmt_sequence "end"
//	- repeat_stmt = "repeat" stmt_sequence "until" exp
//	- stmt = [assignment | proc_call | if_stmt | while_stmt | repeat_stmt]
//	- stmt_sequence = stmt {";" stmt}
//	- id_list = id {"," id}
//	- array_type = "array" exp "of" type
//	- field_list = [id_list ":" type]
//	- record_type = "record" field_list {";" field_list} "end"
//	- type = id | array_type | record_type
//	- formal_params_section = ["var"] id_list ":" type
//	- formal_params = "(" [formal_params_section {";" formal_params_section}] ")"
//	- proc_head = "procedure" id [formal_params]
//	- proc_body = declarations ["begin" stmt_sequence] "end" id
//	- proc_decl = proc_head ";" proc_body
//	- declarations = ["const" {id "=" exp ";"}]["type" {id "=" type ";"}]["var" {id_list ":" type ";"}]{proc_decl ";"}
//	- module = "module" id ";" declarations ["begin" stmt_sequence] "end" id "."
//
//	Vocabulário (para a análise léxica):
//
//	- * div mod & + - or
//	- = # < <= > >= . , : ) ]
//	- of then do until ( [ ~ := ;
//	- end else elsif if while repeat
//	- array record const type var procedure begin module
//

#include <stdio.h>
#include <memory.h>
#include <ctype.h>
#include <strings.h>

//
// Constantes e definições gerais
//
enum boolean_ {
	false = 0,
	true = 1
};
typedef enum boolean_ boolean_t;

typedef unsigned int index_t;
typedef FILE * file_t;
typedef char * string_t;

//
// Constantes e definições do analisador léxico
//

// Símbolos
enum symbol_ {
	symbol_null = 0,
	symbol_times = 1,
	symbol_div = 3,
	symbol_mod = 4,
	symbol_and = 5,
	symbol_plus = 6,
	symbol_minus = 7,
	symbol_or = 8,
	symbol_equal = 9,
	symbol_not_equal = 10,
	symbol_less = 11,
	symbol_less_equal = 13,
	symbol_greater = 14,
	symbol_greater_equal = 12,
	symbol_period = 18,
	symbol_comma = 19,
	symbol_colon = 20,
	symbol_close_paren = 22,
	symbol_close_bracket = 23,
	symbol_of = 25,
	symbol_then = 26,
	symbol_do = 27,
	symbol_open_paren = 29,
	symbol_open_bracket = 30,
	symbol_not = 32,
	symbol_becomes = 33,
	symbol_number = 34,
	symbol_id = 37,
	symbol_semicolon = 38,
	symbol_end = 40,
	symbol_else = 41,
	symbol_elsif = 42,
	symbol_until = 43,
	symbol_if = 44,
	symbol_while = 46,
	symbol_repeat = 47,
	symbol_array = 54,
	symbol_record = 55,
	symbol_const = 57,
	symbol_type = 58,
	symbol_var = 59,
	symbol_proc = 60,
	symbol_begin = 61,
	symbol_module = 63,
	symbol_eof = 64
};
typedef enum symbol_ symbol_t;
// Identificadores
const unsigned int id_length = 16;
typedef char id_t[id_length + 1];
// Valores
typedef long int value_t;
// Palavras-chave
struct keyword_ {
	char id[16];
	symbol_t symbol;
};
typedef struct keyword_ keyword_t;

// Propriedades do lexema
id_t scanner_id;
value_t scanner_value;

// Tratamento de erros na análise léxica
boolean_t scanner_error;
fpos_t scanner_error_position;

// O ponteiro para o arquivo com o código-fonte e o caracter atual
file_t scanner_file;
char scanner_char;

// Vetor com todas as palavras-chave da linguagem
keyword_t scanner_keywords[] = {
	{ .id = "do", .symbol = symbol_do },
	{ .id = "if", .symbol = symbol_if },
	{ .id = "of", .symbol = symbol_of },
	{ .id = "or", .symbol = symbol_or },
	{ .id = "end", .symbol = symbol_end },
	{ .id = "mod", .symbol = symbol_mod },
	{ .id = "var", .symbol = symbol_var },
	{ .id = "else", .symbol = symbol_else },
	{ .id = "then", .symbol = symbol_then },
	{ .id = "type", .symbol = symbol_type },
	{ .id = "array", .symbol = symbol_array },
	{ .id = "begin", .symbol = symbol_begin },
	{ .id = "const", .symbol = symbol_const },
	{ .id = "elsif", .symbol = symbol_elsif },
	{ .id = "until", .symbol = symbol_until },
	{ .id = "while", .symbol = symbol_while },
	{ .id = "record", .symbol = symbol_record },
	{ .id = "repeat", .symbol = symbol_repeat },
	{ .id = "procedure", .symbol = symbol_proc },
	{ .id = "div", .symbol = symbol_div },
	{ .id = "module", .symbol = symbol_module }
};
const unsigned long scanner_keywords_count = sizeof(scanner_keywords)	/ sizeof(keyword_t);

//
// Analisador léxico
//

// Esta função é responsável por verificar se o identificar “id” é uma palavra reservada ou não
// O símbolo equivalente à palavra reservada é armazenada por referência no parâmetro “symbol”
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

// A razão de se criar uma função somente para isto é aproveitá-la se a codificação do arquivo de código-fonte mudar
boolean_t scanner_step() {
	if (fread(&scanner_char, sizeof(char), 1, scanner_file) == sizeof(char))
		return true;
	return false;
}

// Esta função aponta que um erro aconteceu usando a mensagem de parâmetro e a posição atual no arquivo
void scanner_mark(const string_t message) {
	fpos_t position;
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
// reconhecer “var”. Ao terminar, “scanner_char” deve conter o espaço em branco que o segue
//
// As funções “id”, “integer” e “number” fazem parte da EBNF e deveriam ser consideradas parte do analisador sintático.
// No entanto, pela forma com que o compilador está definido, o reconhecimento de lexemas também é estipulado pela EBNF
// fazendo com que a análise léxica seja realizada por um “mini descendente recursivo” ao invés de um autômato finito
//

void id(symbol_t *symbol) {
	index_t index = 0;
	while (index < id_length && isalnum(scanner_char)) {
		scanner_id[index++] = scanner_char;
		if (!scanner_step())
			break;
	}
	// O tamanho máximo para um identificador é especificado por “id_length” e a variável “scanner_id” possui tamanho
	// “id_length + 1” e por isso o caractere terminador pode ser incluído mesmo que o limite seja alcançado
	scanner_id[index] = '\0';
	if (!is_keyword(scanner_id, symbol) && symbol)
		*symbol = symbol_id;
}

// FAZER: Adicionar verificação se o número é muito longo
void integer(symbol_t *symbol) {
	scanner_value = 0;
	while (isdigit(scanner_char)) {
		// Efetua o cálculo do valor, dígito-a-dígito, com base nos caracteres lidos
		scanner_value = 10 * scanner_value + (scanner_char - '0');
		if (!scanner_step())
			break;
	}
	if (symbol)
		*symbol = symbol_number;
}

// Por definição, somente números positivos inteiros são reconhecidos
void number(symbol_t *symbol) {
	integer(symbol);
}

// Ao entrar nesta função, o analisador léxico já encontrou os caracteres "(*" que iniciam o comentário e “scanner_char”
// possui o asterisco como valor
void comment(symbol_t *symbol) {
	char last_char = scanner_char;
	while (scanner_step()) {
		// Comentários aninhados
		if (scanner_char == '*' && last_char == '(')
			comment(symbol);
		// Fim do comentário
		if (scanner_char == ')' && last_char == '*') {
			scanner_step();
			return;
		}
		last_char = scanner_char;
	}
	scanner_mark("Comentário sem fim. Blá-blá-blá...");
	*symbol = symbol_eof;
}

void scanner_get(symbol_t *symbol) {
	// Salta os caracteres em branco, incluindo símbolos de quebra de linha
	while (isspace(scanner_char))
		scanner_step();
	if (feof(scanner_file)) {
		*symbol = symbol_eof;
		return;
	}
	// Os casos de um identificador ou um número são considerados separadamente para que o código no “switch” não precise
	// incluir uma chamada a “scanner_step” em cada “case”
	if (isalpha(scanner_char)) {
		id(symbol);
		return;
	} else if (isdigit(scanner_char)) {
		number(symbol);
		return;
	}
	switch (scanner_char) {
		case '&': *symbol = symbol_and;						break;
		case '*': *symbol = symbol_times;					break;
		case '+': *symbol = symbol_plus;					break;
		case '-': *symbol = symbol_minus;					break;
		case '=': *symbol = symbol_equal;					break;
		case '#': *symbol = symbol_not_equal;			break;
		case '<': *symbol = symbol_less;					break;
		case '>': *symbol = symbol_greater;				break;
		case ';': *symbol = symbol_semicolon;			break;
		case ',':	*symbol = symbol_comma;					break;
		case ':': *symbol = symbol_colon;					break;
		case '.': *symbol = symbol_period;				break;
		case '(': *symbol = symbol_open_paren;		break;
		case ')': *symbol = symbol_close_paren;		break;
		case '[': *symbol = symbol_open_bracket;	break;
		case ']': *symbol = symbol_close_bracket;	break;
		case '~': *symbol = symbol_not;						break;
		default:	*symbol = symbol_null;					break;
	}
	scanner_step();
	// Os casos abaixo representam os lexemas com mais de um caracter (como “>=”, “:=” etc.)
	if (*symbol == symbol_less && scanner_char == '=') {
		scanner_step();
		*symbol = symbol_less_equal;
	} else if (*symbol == symbol_greater && scanner_char == '=') {
		scanner_step();
		*symbol = symbol_greater_equal;
	} else if (*symbol == symbol_colon && scanner_char == '=') {
		scanner_step();
		*symbol = symbol_becomes;
	} else if (*symbol == symbol_open_paren	&& scanner_char == '*') {
		scanner_step();
		// Ignora os caracteres entre “(*” e “*)” como sendo comentários e entra novamente na função para buscar o próximo
		// lexema válido
		comment(symbol);
		scanner_get(symbol);
	}
}

void scanner_initialize(file_t file, fpos_t position) {
	scanner_file = file;
	scanner_error = false;
	scanner_error_position = position;
	scanner_step();
}

//
// Ponto de entrada do programa
//

int main(int argc, const string_t argv[]) {
	file_t source_code_file = fopen("/Users/Neto/Dropbox/Programming/Projects/Oberon/Oberon/Test.oberon", "r");
	if (!source_code_file) {
		printf("Arquivo não encontrado.\n");
		return 0;
	}
	scanner_initialize(source_code_file, 0);
	symbol_t symbol = symbol_null;
	while (symbol != symbol_eof) {
		scanner_get(&symbol);
		if (symbol == symbol_id)
			printf("id: %s, ", scanner_id);
		else if (symbol == symbol_number)
			printf("number: %ld, ", scanner_value);
		else
			printf("%d, ", symbol);
	}
	printf("eof\n");
	fclose(source_code_file);
	return 0;
}
