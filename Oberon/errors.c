//
//  errors.c
//  Oberon
//
//  Created by Alvaro Costa Neto on 10/15/13.
//  Copyright (c) 2013 Alvaro Costa Neto. All rights reserved.
//

#include "errors.h"

unsigned int errors_count = 0;

// Se esta variável for verdadeira, as funções de análise e geração de código devem simplesmente sair
boolean_t errors_just_quit;

// Esta função aponta que um erro aconteceu usando a mensagem de parâmetro e a posição atual no arquivo
void errors_mark(const error_t error, const string_t message, ...) {
	if (error > error_warning)
		errors_count++;
	if (errors_count > ERRORS_BAD_CODE_TOLERANCE) {
		printf("That's it. I quit!\n");
		errors_just_quit = true;
		return;
	}
	switch (error) {
		case error_info:
			printf("Info at "); break;
		case error_tip:
			printf("Tip at "); break;
		case error_warning:
			printf("Warning at "); break;
		case error_scanner:
			printf("Error at "); break;
		case error_parser:
			printf("Error at "); break;
		case error_fatal:
			printf("What are you freaking doing at "); break;
		default:
			printf("Whaaaaaat?! at "); break;
	}
	printf("(%d, %d): ", scanner_token.position.line, scanner_token.position.column);
	va_list args;
	va_start(args, message);
	vprintf(message, args);
	va_end(args);
	printf("\n");
}
