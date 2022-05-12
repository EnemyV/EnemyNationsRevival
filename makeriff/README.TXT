Known things that this parser will not correctly handle :

Floats longer than 256 characters will crash the parser.
Floats are not range checked.
Filenames longer than 256 characters will be truncated ( and will probably fail to open ).
Filenames with invalid characters will be caught by failure to
open the file, not by detecting illegal characters.
Filenames with spaces will not work.
Fixed point values are not range checked.
Strings longer than 64K - 1 will crash the parser.
#define'd variables will be truncated after 80 characters.
Floats, ints, and fixed point numbers cannot start with '+'.