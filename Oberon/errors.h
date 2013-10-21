//
//  errors.h
//  Oberon
//
//  Created by Alvaro Costa Neto on 10/15/13.
//  Copyright (c) 2013 Alvaro Costa Neto. All rights reserved.
//

#ifndef Oberon_errors_h
#define Oberon_errors_h

#include "scanner.h"

#define ERRORS_BAD_CODE_TOLERANCE 50

typedef enum _error {
	error_log,
	error_info,
	error_tip,
	error_warning,
	error_scanner,
	error_parser,
	error_fatal,
	error_unknown
} error_t;

void errors_mark(const error_t error, const position_t position, const char *message, ...);

#endif
