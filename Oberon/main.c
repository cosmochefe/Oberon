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
#include "scanner.h"
#include "parser.h"

//
// Constantes, variáveis e definições gerais
//

//
// Ponto de entrada do programa
//

int main(int argc, const string_t argv[]) {
	file_t source_code_file = fopen("/Users/Neto/Dropbox/Programming/Projects/Oberon/Oberon/Test.oberon", "r");
	if (!source_code_file) {
		printf("Arquivo não encontrado.\n");
		return 0;
	}
	if (!parser_initialize(source_code_file)) {
		printf("Arquivo vazio ou danificado.\n");
		fclose(source_code_file);
		return 0;
	}
	if (!parser_run())
		printf("Ocorreram erros durante a compilação. O arquivo final não foi gerado.");
	fclose(source_code_file);
	return 0;
}
