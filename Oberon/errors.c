//
//  errors.c
//  Oberon
//
//  Created by Alvaro Costa Neto on 10/15/13.
//  Copyright (c) 2013 Alvaro Costa Neto. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "scanner.h"
#include "errors.h"

unsigned int errors_count = 0;

// Esta função aponta que um erro aconteceu usando a mensagem de parâmetro e a posição atual no arquivo
void errors_mark(const error_t error, const position_t position, const char *message, ...)
{
	if (error > error_warning)
		errors_count++;
	if (errors_count > ERRORS_BAD_CODE_TOLERANCE) {
		printf("%d errors? That's it. I quit!\n", ERRORS_BAD_CODE_TOLERANCE);
		exit(EXIT_FAILURE);
	}
	switch (error) {
		case error_log:
			printf("Log at "); break;
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
	printf("(%d, %d): ", position.line, position.column);
	va_list args;
	va_start(args, message);
	vprintf(message, args);
	va_end(args);
	printf("\n");
}
