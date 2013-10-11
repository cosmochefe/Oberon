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

int main(int argc, const char *argv[]) {
	FILE *source_code = fopen("Test.oberon", "r");
	if (!source_code)
		return 0;
	fclose(source_code);
	return 0;
}
