#ifndef __struct_ssl__
#define __struct_ssl__

#ifdef HAVE_SSL

#include "client.h"

#ifdef HAVE_OPENSSL
#include <openssl/rsa.h>       /* SSL stuff */
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>



/* Make these what you want for cert & key files */
#define CERTF  "server.cert.pem"
#define KEYF  "server.key.pem"


extern   SSL_CTX* ctx;
extern   SSL_METHOD *meth;
#endif
extern   void init_ssl();
extern   int ssl_handshake(struct Client *);   /* Handshake the accpeted con.*/
extern   int ssl_client_handshake(struct Client *); /* and the initiated con.*/
extern   int ircd_SSL_read(struct Client *acptr, void *buf, int sz);
extern   int ircd_SSL_write(struct Client *acptr, const void *buf, int sz);
extern   int ircd_SSL_accept(struct Client *acptr, int fd);
extern	 int ircd_SSL_shutdown(struct Client *acptr);
#endif
#endif
