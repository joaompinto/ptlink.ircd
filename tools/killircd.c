/*****************************************************************
 * PTlink IRCd is (C) CopyRight PTlink Coders Team 1999-2004     *
 *                 http://software.pt-link.net/                  *
 * This program is distributed under GNU Public License          *
 * Please read the file COPYING for copyright information.       *
 *****************************************************************

  File: killircd.c
  Desc: kill ircd tool
  Author: Lamego@PTlink.net

* $Id: killircd.c,v 1.4 2005/09/09 12:39:07 waxweazle Exp $           *
*/

#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#ifndef INCLUDED_sys_types_h
#include <sys/types.h>       /* pid_t */
#define INCLUDED_sys_types_h
#endif
#include "path.h"
#include "setup.h"

/*
 * check_pidfile_kill
 *
 * inputs       - nothing
 * output       - nothing
 * side effects - reads pid from pidfile and checks if ircd is in process
 *                list. if it is kill it
 * -kre
 */
void check_pidfile_kill(void)
{
  int fd;
  char buff[20];
  pid_t pidfromfile;
  char ppath[256];

#ifndef PIDFILE  
  snprintf(ppath, 256, "%s/ircd.pid", VARPATH);
#else
  strncpy(ppath, PIDFILE, 256);
#endif  
  if ((fd = open(ppath, O_RDONLY)) >= 0 )
  {
    if (read(fd, buff, sizeof(buff)) == -1)
    {
      /* printf("NOTICE: problem reading from %s (%s)\n", ppath,
          strerror(errno)); */
    }
    else
    {
      pidfromfile = atoi(buff);
      if (pidfromfile != (int)getpid() && !kill(pidfromfile, SIGTERM))
      {
        printf("Killed process with pid=%i\n", pidfromfile);
        exit(-1);
      }
    }
    close(fd);
  }
  else if(errno != ENOENT)
  {
    printf("WARNING: problem opening %s: %s\n", ppath, strerror(errno));
  }

}

int main(int parc, char **argv)
{
  check_pidfile_kill();
  return 0;  
}
