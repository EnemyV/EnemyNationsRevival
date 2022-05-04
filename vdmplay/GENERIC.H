/*_ generic.hpp   Tue Jul  5 1988   Modified by: Walter Bright */

#ifndef __GENERIC_H
#define __GENERIC_H	1

#include "rw/generic.h"


typedef int (*GPT)	(int,char *);
extern  int genericerror(int,char *);

#define callerror(generic,type,a,b)	(*errorhandler(generic,type))(a,b)

#define declare2(a,type1,type2)		a##declare2(type1,type2)
#define implement2(a,type1,type2)	a##implement2(type1,type2)

#endif /* __GENERIC_H */
