#ifndef __PARSER_H
#define __PARSER_H 1

#include "common.h"

//! @brief Verificador de palabra alfanumerica.
//!
//! @param word - String a analizar.
int is_word(char *word);

//! @brief Parser de texto.
//!
//! @param[in] buf - const char *.  No debe contener el '\n'
//! @param[out] toks - char *: arreglo de tokens
//! @param[out] lens - arreglo de enteros, contiene la longitud de los tokens
int text_parser(char *buf, char *toks[3], int lens[3]);


#endif
