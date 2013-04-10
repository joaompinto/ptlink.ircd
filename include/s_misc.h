/*
 * $Id: s_misc.h,v 1.3 2005/08/27 16:23:49 jpinto Exp $ 
 */

#ifndef INCLUDED_s_misc_h
#define INCLUDED_s_misc_h
#ifndef INCLUDED_sys_types_h
#include <sys/types.h>
#define INCLUDED_sys_types_h
#endif

struct Client;
struct ConfItem;

extern void    serv_info (struct Client *, char *);
extern char*   date(time_t);
extern const char* smalldate(time_t);
extern char    *small_file_date(time_t);

#endif


