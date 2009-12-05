/***************************************************************************\
|* Function Parser for C++ v4.0.1                                          *|
|*-------------------------------------------------------------------------*|
|* Copyright: Juha Nieminen                                                *|
\***************************************************************************/

#ifndef ONCE_FPARSER_GMPINT_H_
#define ONCE_FPARSER_GMPINT_H_

#include "fparser.h"
#include "mpfr/GmpInt.h"

class FunctionParser_gmpint: public FunctionParserBase<GmpInt> {};

#endif
