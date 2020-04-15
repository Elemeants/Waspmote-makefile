// ArduinoJson - arduinojson.org
// Copyright Benoit Blanchon 2014-2019
// MIT License
#ifndef CTYPE_HPP
#define CTYPE_HPP

#include <stdint.h>

namespace ARDUINOJSON_NAMESPACE
{

inline bool issign( char c) {
  return '-' == c || c == '+';
}

inline bool _isdigit( char c ) {
  return '0' <= c && c <= '9';
}

}  // namespace ARDUINOJSON_NAMESPACE

#endif