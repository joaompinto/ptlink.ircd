#ifndef CRYPT_H_
#ifdef _PASSWORD_EFMT1  
#else
#define _PASSWORD_EFMT1         '_'
#endif
#include "setup.h"
#ifdef OWN_CRYPT
extern char *crypt_des(char *key, char *setting);
#endif /* OWN_CRYPT */
#endif
