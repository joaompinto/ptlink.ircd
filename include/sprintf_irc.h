/*
 * $Id: sprintf_irc.h,v 1.3 2005/08/27 16:23:49 jpinto Exp $ 
 */

#ifndef SPRINTF_IRC
#define SPRINTF_IRC

#include <stdarg.h>

/*=============================================================================
 * Proto types
 */

extern int vsprintf_irc(register char *str, register const char *format,
    register va_list);
extern int ircsprintf(register char *str, register const char *format, ...);

#endif /* SPRINTF_IRC */
