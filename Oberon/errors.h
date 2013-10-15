//
//  errors.h
//  Oberon
//
//  Created by Alvaro Costa Neto on 10/15/13.
//  Copyright (c) 2013 Alvaro Costa Neto. All rights reserved.
//

#ifndef Oberon_errors_h
#define Oberon_errors_h

#include "types.h"
#include "scanner.h"

typedef enum _error_level {
	error_unknown,
	error_info,
	error_tip,
	error_warning,
	error_scanner,
	error_parser,
	error_fatal
} error_t;

//
// Pré-definições
//

void errors_mark(const error_t error, const string_t message);

#endif
