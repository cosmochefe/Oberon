//
//  parser.h
//  Oberon
//
//  Created by Alvaro Costa Neto on 10/12/13.
//  Copyright (c) 2013 Alvaro Costa Neto. All rights reserved.
//

#ifndef Oberon_parser_h
#define Oberon_parser_h

#include <stdio.h>
#include <strings.h>
#include <ctype.h>

#include "types.h"
#include "errors.h"
#include "scanner.h"

//
// Pré-definições
//

boolean_t parser_initialize(file_t input_file, file_t output_file);
boolean_t parser_run();

#endif
