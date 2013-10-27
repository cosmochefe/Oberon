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

// Estas definições determinam o tamanho em bytes dos tipos padrões de dados e endereços
typedef unsigned char value_t;
typedef unsigned short address_t;

void initialize_backend(FILE *file);

void write_load(address_t address);
void write_load_immediate(value_t value);
void write_store(address_t address);

#endif
