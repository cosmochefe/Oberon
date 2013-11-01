//
//  backend.h
//  Oberon
//
//  Created by Alvaro Costa Neto on 10/20/13.
//  Copyright (c) 2013 Alvaro Costa Neto. All rights reserved.
//

#ifndef Oberon_backend_h
#define Oberon_backend_h

#include <stdio.h>
#include <limits.h>

//typedef enum _op {
//	op_nop,
//	op_add,
//	op_sub,
//	op_neg,
//	op_mul,
//	op_div,
//	op_mod,
//	op_and,
//	op_or,
//	op_not
//} op_t;

// Estas definições determinam o tamanho em bytes dos tipos padrões de dados e endereços
#define MAX_VALUE SCHAR_MAX
#define MIN_VALUE SCHAR_MIN
typedef signed char value_t;
#define MAX_ADDRESS USHRT_MAX
#define MIN_ADDRESS 0
typedef unsigned short address_t;

#endif
