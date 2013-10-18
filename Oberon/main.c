//
//  main.c
//  Oberon
//
//  Created by Alvaro Costa Neto on 10/11/13.
//  Copyright (c) 2013 Alvaro Costa Neto. All rights reserved.
//

#include <stdio.h>

#include "types.h"
#include "errors.h"
#include "scanner.h"
#include "parser.h"

//
// Constantes, variáveis e definições gerais
//

const string_t input_path  = "/Users/Neto/Dropbox/Programming/Projects/Oberon/Oberon/Input.txt";
const string_t output_path = "/Users/Neto/Dropbox/Programming/Projects/Oberon/Oberon/Output.asm";

const boolean_t true = 1;
const boolean_t false = 0;

file_t input_file;
file_t output_file;

//
// Ponto de entrada do programa
//

int main(int argc, const string_t argv[])
{
	input_file = fopen(input_path, "r");
	output_file = fopen(output_path, "w");
	if (!input_file || !output_file) {
		printf("Input or output files could not be opened.\n");
		return 0;
	}
	if (!parser_initialize())
		printf("Empty or damaged input file.\n");
	else
		parser_run();
	fclose(input_file);
	fclose(output_file);
	return 0;
}
