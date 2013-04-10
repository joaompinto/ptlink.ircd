/*
 * $Id: acconfig.h,v 1.3 2005/08/27 16:23:49 jpinto Exp $
 */

/* Define only one of POSIX, BSD, or SYSV signals.  */
/* Define if your system has reliable POSIX signals.  */
#undef POSIX_SIGNALS

/* Define if your system has reliable BSD signals.  */
#undef BSD_RELIABLE_SIGNALS

/* Define if your system has unreliable SYSV signals.  */
#undef SYSV_UNRELIABLE_SIGNALS

/* Define MALLOCH to <malloc.h> if needed.  */
#if !defined(STDC_HEADERS)
#undef MALLOCH
#endif

/* Define if you have the poll() system call.  */
#undef USE_POLL

/* Chose only one of NBLOCK_POSIX, NBLOCK_BSD, and NBLOCK_SYSV */
/* Define if you have posix non-blocking sockets (O_NONBLOCK) */
#undef NBLOCK_POSIX

/* Define if you have BSD non-blocking sockets (O_NDELAY) */
#undef NBLOCK_BSD

/* Define if you have SYSV non-blocking sockets (FIONBIO) */
#undef NBLOCK_SYSV

/* Define if you have the mmap() function.  */
#undef USE_MMAP

/* Define if you are running DYNIXPTX.  */
#undef OS_DYNIXPTX

/* Define if you are running HPUX.  */
#undef OS_HPUX

/* Define if you are running SOLARIS2.  */
#undef OS_SOLARIS2

/* Define if you want to use adns lib */
#undef USE_ADNS

/* Define if you want to use fast dns lookup */
#undef USE_FASTDNS

/* Define if you have SSL */
#define HAVE_SSL

/* Define if you want to allow hebrew chars on nicks */
#undef HEBREW_SUPPORT

/* Define if you want to allow korean chars on nicks */
#undef KOREAN

/* Define if you need built-in crypt() */
#undef OWN_CRYPT

/* Define if you have the setenv() system call.  */
#undef USE_SETENV

/* Define if you want IPV6 support (experimental)*/
#undef IPV6

/* Define if you have chroot() */
#undef HAVE_CHROOT

/* Define if you want to setup the ircd chroot directory */
#undef CHROOT_DIR

/* Define if you want an hardcoed path to the pidfile */
#undef PIDFILE

/* Define if you want the ircd to use a specific UID */
#undef IRC_UID

/* Define if you want the ircd to use a specific GID */
#undef IRC_GID
