//
//  main.c
//  Oberon
//
//  Created by Alvaro Costa Neto on 10/11/13.
//  Copyright (c) 2013 Alvaro Costa Neto. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "backend.h"
#include "scanner.h"
#include "parser.h"

#define OUTPUT_EXTENSION ".asm"

void initialize_backend(FILE *file);

int main(int argc, const char *argv[])
{
	FILE *input_file = fopen("/Users/Alvaro/Dropbox/Programming/Projects/Oberon/Oberon/Input.txt", "r");
	if (!input_file) {
		printf("Input file could not be opened.\n");
		return EXIT_FAILURE;
	}
	FILE *output_file = fopen("/Users/Alvaro/Dropbox/Programming/Projects/Oberon/Oberon/Output.txt", "w+");
	if (!output_file) {
		printf("Output file could not be created.\n");
		return EXIT_FAILURE;
	}
	if (!initialize_parser(input_file))
		printf("Empty or damaged input file.\n");
	else {
		initialize_backend(output_file);
		parse();
	}
	fclose(input_file);
	fclose(output_file);
	return EXIT_SUCCESS;
}
