//
//  main.c
//  Oberon
//
//  Created by Alvaro Costa Neto on 10/11/13.
//  Copyright (c) 2013 Alvaro Costa Neto. All rights reserved.
//

#include <stdio.h>

#include "backend.h"
#include "parser.h"

//
// Constantes, variáveis e definições gerais
//

const char *input_path  = "/Users/Neto/Dropbox/Programming/Projects/Oberon/Oberon/Input.txt";
const char *output_path = "/Users/Neto/Dropbox/Programming/Projects/Oberon/Oberon/Output.txt";

//
// Ponto de entrada do programa
//

int main(int argc, const char *argv[])
{
	FILE *input_file = fopen(input_path, "r");
	FILE *output_file = fopen(output_path, "w");
	if (!input_file || !output_file) {
		printf("Input or output files could not be opened.\n");
		return 0;
	}
	if (!initialize_parser(input_file))
		printf("Empty or damaged input file.\n");
	else {
		initialize_backend(output_file);
		parse();
	}
	fclose(input_file);
	fclose(output_file);
	return 0;
}
