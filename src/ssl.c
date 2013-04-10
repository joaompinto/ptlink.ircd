/*
 *   $Id: 
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

/* Make these what you want for cert & key files */
#define CERTF  "server.cert.pem"
#define KEYF  "server.key.pem"
  
#include "config.h"
#ifdef HAVE_SSL

#ifdef HAVE_GNUTLS
#include "gnutls/gnutls.h"
#include "gnutls/openssl.h"
#endif

#include "ssl.h"

#include "common.h"
#include "ircd.h"
#include "client.h"
#include "s_log.h"
#include "s_bsd.h"

#define SAFE_SSL_READ 1
#define SAFE_SSL_WRITE 2
#define SAFE_SSL_ACCEPT 3
#define SAFE_SSL_CONNECT 4

/* The SSL structures */
SSL_CTX* ctx;
SSL_METHOD* meth;
static int fatal_ssl_error(int ssl_error, int where, struct Client *sptr);

void init_ssl() {
    /* SSL preliminaries. We keep the certificate and key with the context. */

  SSL_load_error_strings();
  SSLeay_add_ssl_algorithms();
  meth = SSLv23_server_method();
  ctx = SSL_CTX_new (meth);

       if (!ctx)
         {
               irclog(L_ERROR, "Error creating SSL CTX");                        
#if 0         
               ERR_print_errors_fp (stderr);
#endif               
               exit (2);               
         }

       if (SSL_CTX_use_certificate_file (ctx, CERTF, SSL_FILETYPE_PEM) <= 0)
         {
            irclog(L_ERROR, "Failed to load SSL certificate %s, SSL disabled", CERTF);
            no_ssl = -1;
            return;
         }
       if (SSL_CTX_use_PrivateKey_file (ctx, KEYF, SSL_FILETYPE_PEM) <= 0)
         {
            irclog(L_ERROR, "Failed to load SSL private key %s, SSL disabled", KEYF);
            no_ssl = -1;            
            return;
         }

#if 0
       if (!SSL_CTX_check_private_key (ctx))
         {
               fprintf (stderr, "Private key does not match the certificate public key\n");
               exit (5);
         }
#endif
}

#define CHK_NULL(x) if ((x)==NULL) {\
        irclog(L_NOTICE, "Lost connection to %s:Error in SSL", \
                     get_client_name(cptr, TRUE)); \
	return 0;\
	}

int ssl_handshake(struct Client *cptr) {

  char *str;
  int err;

      cptr->ssl = (struct SSL*) SSL_new (ctx);
//      cptr->use_ssl=1;


      CHK_NULL (cptr->ssl);
      SSL_set_fd ((SSL *)cptr->ssl, cptr->fd);
      set_non_blocking(cptr->fd);
      err = ircd_SSL_accept (cptr, cptr->fd);
      if ((err)==-1) {
        irclog(L_ERROR,"Lost connection to %s:Error in SSL_accept()", 
                     get_client_name(cptr, TRUE));
	SSL_shutdown((SSL *)cptr->ssl);
	SSL_free((SSL *)cptr->ssl);
	cptr->ssl = NULL;
        return 0;
      }

      /* Get the cipher - opt */
      SetSecure(cptr);
      
      irclog (L_DEBUG, "SSL connection using %s", SSL_get_cipher ((SSL *)cptr->ssl));

      /* Get client's certificate (note: beware of dynamic
       * allocation) - opt */

      cptr->client_cert = (struct X509*)SSL_get_peer_certificate ((SSL *)cptr->ssl);

      if (cptr->client_cert != NULL)
      {
        irclog (L_DEBUG,"Client certificate:");

        str = X509_NAME_oneline (X509_get_subject_name ((X509*)cptr->client_cert), 0, 0);
        CHK_NULL (str);
        irclog (L_DEBUG, "\t subject: %s", str);
        // Bejvavalo
 	//       Free (str);
	free(str);
	
        str = X509_NAME_oneline (X509_get_issuer_name ((X509*)cptr->client_cert), 0, 0);
        CHK_NULL (str);
        irclog (L_DEBUG, "\t issuer: %s", str);
        // Bejvavalo
        // Free (str);
	free(str);
	
        /* We could do all sorts of certificate
         * verification stuff here before
         *        deallocating the certificate. */

        X509_free ((X509*)cptr->client_cert);
      }
      else
        irclog (L_DEBUG, "Client does not have certificate.");

      return 1;

}

int ssl_client_handshake(struct Client *cptr) {

  char *str;
  int err;

  cptr->ssl = (struct SSL*)SSL_new (ctx);                         CHK_NULL(cptr->ssl);    
  SSL_set_fd ((SSL*)cptr->ssl, cptr->fd);
  set_blocking(cptr->fd);
  err = SSL_connect ((SSL*)cptr->ssl);                     
  set_non_blocking(cptr->fd);
  if ((err)==-1) {
     irclog(L_NOTICE,"Could connect to %s:Error in SSL_connect()", 
                  get_client_name(cptr, TRUE));
     return 0;
     }
    
  /* Following two steps are optional and not required for
     data exchange to be successful. */
  
  /* Get the cipher - opt */
  set_blocking(cptr->fd);
  irclog (L_NOTICE,"SSL connection using %s", SSL_get_cipher ((SSL*)cptr->ssl));
  
  
  SetSecure(cptr);      
  /* Get server's certificate (note: beware of dynamic allocation) - opt */

  cptr->client_cert = (struct X509*)SSL_get_peer_certificate ((SSL *)cptr->ssl);
  set_non_blocking(cptr->fd);

  if (cptr->client_cert != NULL)
    {
      irclog (L_NOTICE,"Server certificate:");
  
      str = X509_NAME_oneline (X509_get_subject_name ((X509*)cptr->client_cert),0,0);
      CHK_NULL(str);
      irclog (L_NOTICE, "\t subject: %s", str);
      // Free (str);
      free (str);
  
      str = X509_NAME_oneline (X509_get_issuer_name  ((X509*)cptr->client_cert),0,0);
      CHK_NULL(str);
      irclog (L_NOTICE, "\t issuer: %s", str);
      // Free (str);
      free (str);
      /* We could do all sorts of certificate verification stuff here before
      deallocating the certificate. */

      X509_free ((X509*)cptr->client_cert);

    }
    else
      irclog (L_NOTICE, "Server does not have certificate.");
      
  
  return 1;
}

int ircd_SSL_read(struct Client *acptr, void *buf, int sz)
{
    int len, ssl_err;
    len = SSL_read((SSL *)acptr->ssl, buf, sz);
    if (len <= 0)
    {
       switch(ssl_err = SSL_get_error((SSL *)acptr->ssl, len)) {
           case SSL_ERROR_SYSCALL:
               if (ERRNO == EWOULDBLOCK || ERRNO == EAGAIN ||
                       ERRNO == EINTR) {
           case SSL_ERROR_WANT_READ:
                   SET_ERRNO(EWOULDBLOCK);
		   Debug((DEBUG_ERROR, "ircd_SSL_read: returning EWOULDBLOCK and 0 for %s - %s", acptr->name,
			ssl_err == SSL_ERROR_WANT_READ ? "SSL_ERROR_WANT_READ" : "SSL_ERROR_SYSCALL"		   
		   ));
                   return -1;
               }
           case SSL_ERROR_SSL:
               if(ERRNO == EAGAIN)
                   return -1;
           default:
           	Debug((DEBUG_ERROR, "ircd_SSL_read: returning fatal_ssl_error for %s",
           	 acptr->name));
		return  -1;
       }
    }
    Debug((DEBUG_ERROR, "ircd_SSL_read for %s (%p, %i): success", acptr->name, buf, sz));
    return len;
}
int ircd_SSL_write(struct Client *acptr, const void *buf, int sz)
{
    int len, ssl_err;

    len = SSL_write((SSL *)acptr->ssl, buf, sz);
    if (len <= 0)
    {
       switch(ssl_err = SSL_get_error((SSL *)acptr->ssl, len)) {
           case SSL_ERROR_SYSCALL:
               if (ERRNO == EWOULDBLOCK || ERRNO == EAGAIN ||
                       ERRNO == EINTR)
		{
			SET_ERRNO(EWOULDBLOCK);
			return -1;
		}
		return -1;
          case SSL_ERROR_WANT_WRITE:
                   SET_ERRNO(EWOULDBLOCK);
                   return -1;
           case SSL_ERROR_SSL:
               if(ERRNO == EAGAIN)
                   return -1;
           default:
		Debug((DEBUG_ERROR, "ircd_SSL_write: returning fatal_ssl_error for %s", acptr->name));
		return -1;
       }
    }
    Debug((DEBUG_ERROR, "ircd_SSL_write for %s (%p, %i): success", acptr->name, buf, sz));
    return len;
}

int ircd_SSL_accept(struct Client *acptr, int fd) {

    int ssl_err;

    if((ssl_err = SSL_accept((SSL *)acptr->ssl)) <= 0) {
	switch(ssl_err = SSL_get_error((SSL *)acptr->ssl, ssl_err)) {
	    case SSL_ERROR_SYSCALL:
		if (ERRNO == EINTR || ERRNO == EWOULDBLOCK
			|| ERRNO == EAGAIN)
	    case SSL_ERROR_WANT_READ:
	    case SSL_ERROR_WANT_WRITE:
		    Debug((DEBUG_DEBUG, "ircd_SSL_accept(%s), - %s", get_client_name(acptr, TRUE), ssl_error_str(ssl_err)));
		    /* handshake will be completed later . . */
		    return 1;
	    default:
		return fatal_ssl_error(ssl_err, SAFE_SSL_ACCEPT, acptr);
		
	}
	/* NOTREACHED */
	return -1;
    }
    return 1;
}

int ircd_SSL_shutdown(struct Client *acptr)
  {
    SSL_shutdown((SSL *) acptr->ssl);
    SSL_free((SSL *)acptr->ssl);
    acptr->ssl = NULL;
    return 0;
  }

static int fatal_ssl_error(int ssl_error, int where, struct Client *sptr)
{
    /* don`t alter ERRNO */
    int errtmp = ERRNO;
    char *ssl_errstr, *ssl_func;

    switch(where) {
	case SAFE_SSL_READ:
	    ssl_func = "SSL_read()";
	    break;
	case SAFE_SSL_WRITE:
	    ssl_func = "SSL_write()";
	    break;
	case SAFE_SSL_ACCEPT:
	    ssl_func = "SSL_accept()";
	    break;
	case SAFE_SSL_CONNECT:
	    ssl_func = "SSL_connect()";
	    break;
	default:
	    ssl_func = "undefined SSL func";
    }

    switch(ssl_error) {
    	case SSL_ERROR_NONE:
	    ssl_errstr = "SSL: No error";
	    break;
	case SSL_ERROR_SSL:
	    ssl_errstr = "Internal OpenSSL error or protocol error";
	    break;
	case SSL_ERROR_WANT_READ:
	    ssl_errstr = "OpenSSL functions requested a read()";
	    break;
	case SSL_ERROR_WANT_WRITE:
	    ssl_errstr = "OpenSSL functions requested a write()";
	    break;
#if 0	    
	case SSL_ERROR_WANT_X509_LOOKUP:
	    ssl_errstr = "OpenSSL requested a X509 lookup which didn`t arrive";
	    break;
#endif	    
	case SSL_ERROR_SYSCALL:
	    ssl_errstr = "Underlying syscall error";
	    break;
	case SSL_ERROR_ZERO_RETURN:
	    ssl_errstr = "Underlying socket operation returned zero";
	    break;
#if 0	    
	case SSL_ERROR_WANT_CONNECT:
	    ssl_errstr = "OpenSSL functions wanted a connect()";
	    break;
#endif	    
	default:
	    ssl_errstr = "Unknown OpenSSL error (huh?)";
    }
    /* if we reply() something here, we might just trigger another
     * fatal_ssl_error() call and loop until a stack overflow... 
     * the client won`t get the ERROR : ... string, but this is
     * the only way to do it.
     * IRC protocol wasn`t SSL enabled .. --vejeta
     */
	sptr->flags |= FLAGS_DEADSOCKET;
	if (errtmp)
	{
		SET_ERRNO(errtmp);
		/* sptr->error_str = strdup(strerror(errtmp)); */
	} else {
		SET_ERRNO(EIO);
		/* sptr->error_str = strdup(ssl_errstr); */
	}
    return -1;
}


#endif
