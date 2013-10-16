//
//  main.c
//  Oberon
//
//  Created by Alvaro Costa Neto on 10/11/13.
//  Copyright (c) 2013 Alvaro Costa Neto. All rights reserved.
//

#include <stdio.h>
#include <memory.h>
#include <ctype.h>
#include <strings.h>

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

//
// Ponto de entrada do programa
//

int main(int argc, const string_t argv[]) {
	file_t source_code_file = fopen(input_path, "r");
	file_t assembly_file = fopen(output_path, "w");
	if (!source_code_file || !assembly_file) {
		printf("Input or output files could not be opened.\n");
		return 0;
	}
	if (!parser_initialize(source_code_file, assembly_file)) {
		printf("Empty or damaged input file.\n");
		fclose(source_code_file);
		return 0;
	}
	parser_run();
	if (errors_count > 0)
		printf("There were errors during compilation. The output file is invalid.\n");
	fclose(assembly_file);
	fclose(source_code_file);
	return 0;
}
