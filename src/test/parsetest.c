/*
 * $Id: parsetest.c,v 1.3 2005/08/27 16:23:50 jpinto Exp $
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/time.h>
#include "../../include/irc_string.h"

#define MAXPARA 20
#define HOSTLEN 63

/* Timing stuff (for performance measurements): compile with -DORATIMING
   and put a TMRESET where you want the counter of time spent set to 0,
   a TMPRINT where you want the accumulated results, and TMYES/TMNO pairs
   around the parts you want timed -orabidoo
*/
struct timeval tsdnow, tsdthen;
unsigned long tsdms;
#define TMRESET tsdms=0;
#define TMYES gettimeofday(&tsdthen, NULL);
#define TMNO gettimeofday(&tsdnow, NULL); if (tsdnow.tv_sec!=tsdthen.tv_sec) tsdms+=1000000*(tsdnow.tv_sec-tsdthen.tv_sec); tsdms+=tsdnow.tv_usec; tsdms-=tsdthen.tv_usec;
#define TMPRINT printf("Time spent: %ld ms\n", tsdms);

static  char    *para[MAXPARA+1];

static  char    sender[HOSTLEN+1];

static char buffer[1024];  /* ZZZ must this be so big? must it be here? */

#define NEW_PARSE

char name[] = "irc.db.net";
char input[512];
char parse_input[512];
char test_input[]="privmsg #test_channel :some test message rah rah\r\n";
int show = 0;

main()
{
  time_t start_time;
  time_t end_time;

  char *p;
  int i;

  while(fgets(input,512,stdin))
    {
      p = strchr(input,'\n');
      if(p)
	{*p++ = '\r'; *p++ = '\n'; *p++ = '\0'; }

      if(!strcasecmp(input,"bench"))
	{
	  show = 0;
	  TMRESET;
	  TMYES;
	  for(i=0;i<100000;i++)
	    {
	      strcpy(parse_input,test_input);
	      parse(parse_input,test_input+sizeof(test_input));
	    }
	  TMNO;
	  TMPRINT;

	  TMRESET;

	  TMYES;
	  for(i=0;i<100000;i++)
	    {
	      strcpy(parse_input,test_input);
	      new_parse(parse_input,test_input+sizeof(test_input));
	    }
	  TMNO;
	  TMPRINT;
	}
      else
	{
	  show = 1;
	  printf("old parse\n");
	  strcpy(parse_input,input);
	  parse(parse_input,input+510);
	  printf("new parse\n");
	  strcpy(parse_input,input);
	  new_parse(parse_input,input+510);
	}
    }

}

/*
 * parse a buffer.
 *
 * NOTE: parse() should not be called recusively by any other functions!
 */
int parse(char *buffer, char *bufend)
{
  char  *ch;
  char  *s;
  int   i;
  char* numeric = 0;
  int   paramcount=MAXPARA;
  int   parc;

  s = sender;
  *s = '\0';

  for (ch = buffer; *ch == ' '; ch++)   /* skip spaces */
    /* null statement */ ;

  para[0] = name;

  if (*ch == ':')
    {
      /*
      ** Copy the prefix to 'sender' assuming it terminates
      ** with SPACE (or NULL, which is an error, though).
      */
      for (++ch, i = 0; *ch && *ch != ' '; ++ch )
        if (s < (sender + sizeof(sender)-1))
          *s++ = *ch; /* leave room for NULL */
      *s = '\0';

      while (*ch == ' ')
        ch++;
    }

  if(show)
    printf("old parse sender [%s] i = %d\n", sender, i);

  if (*ch == '\0')
    {
      fprintf(stderr,"empty message\n");
    }

  /*
  ** Extract the command code from the packet.  Point s to the end
  ** of the command code and calculate the length using pointer
  ** arithmetic.  Note: only need length for numerics and *all*
  ** numerics must have parameters and thus a space after the command
  ** code. -avalon
  *
  * ummm???? - Dianora
  */

  if( *(ch + 3) == ' ' && /* ok, lets see if its a possible numeric.. */
      IsDigit(*ch) && IsDigit(*(ch + 1)) && IsDigit(*(ch + 2)) )
    {
      numeric = ch;
      paramcount = MAXPARA;
      s = ch + 3;       /* I know this is ' ' from above if */
      *s++ = '\0';      /* blow away the ' ', and point s to next part */
    }
  else
    {
      s = strchr(ch, ' ');      /* moved from above,now need it here */
      if (s)
        *s++ = '\0';

      paramcount = 3;
      i = bufend - ((s) ? s : ch);

      if (show)
	{
	  printf("old parse s = %lX ch = %lX\n", s , ch );
	  printf("old parse i = bufend - ((s) ? s : ch) = %d \n", i );
	  printf("old parse: paramcount %d i = %d\n", paramcount, i );
	}
    }

  /*
  ** Must the following loop really be so devious? On
  ** surface it splits the message to parameters from
  ** blank spaces. But, if paramcount has been reached,
  ** the rest of the message goes into this last parameter
  ** (about same effect as ":" has...) --msa
  */

  /* Note initially true: s==NULL || *(s-1) == '\0' !! */

  para[0] = sender;

  i = 0;
  if (s)
    {
      if (paramcount > MAXPARA)
        paramcount = MAXPARA;
      for (;;)
        {
          /*
          ** Never "FRANCE " again!! ;-) Clean
          ** out *all* blanks.. --msa
          */
          while (*s == ' ')
            *s++ = '\0';

          if (*s == '\0')
            break;
          if (*s == ':')
            {
              /*
              ** The rest is single parameter--can
              ** include blanks also.
              */
              para[++i] = s + 1;
              break;
            }
          para[++i] = s;
          if (i >= paramcount)
            break;
          for (; *s != ' ' && *s; s++)
            /* null statement */ ;
        }
    }

  para[++i] = NULL;
  /*  do_numeric(numeric, cptr, from, i, para); */

  parc = i;

  if(show)
    {
      printf("parc = %d \n", i);

      for(i=0;i < parc;i++)
	{
	  printf("parv[%d] = [%s]\n", i, para[i]);
	}
    }
}

/*
 * parse a buffer.
 *
 * NOTE: parse() should not be called recusively by any other functions!
 */
int new_parse(char *buffer, char *bufend)
{
  char  *ch;
  char  *s;
  int   i;
  char* numeric = 0;
  int   paramcount=MAXPARA;
  int   parc;

  s = sender;
  *s = '\0';

  ch = buffer;

  while(*ch == ' ')
    ch++;

  para[0] = name;
#ifdef NEW_PARSE
  if (*ch == ':')
    {
      ch++;

      /*
      ** Copy the prefix to 'sender' assuming it terminates
      ** with SPACE (or NULL, which is an error, though).
      */
      for (i = 0; *ch && *ch != ' '; i++ )
	{
	  if (i < (sizeof(sender)-1))
	    *s++ = *ch++; /* leave room for NULL */
	}
      *s = '\0';

      while (*ch == ' ')
        ch++;
    }
#else
  if (*ch == ':')
    {
      /*
      ** Copy the prefix to 'sender' assuming it terminates
      ** with SPACE (or NULL, which is an error, though).
      */
      for (++ch, i = 0; *ch && *ch != ' '; ++ch )
        if (s < (sender + sizeof(sender)-1))
          *s++ = *ch; /* leave room for NULL */
      *s = '\0';

      while (*ch == ' ')
        ch++;
    }
#endif

  if(show)
    printf("new parse sender [%s] i = %d\n", sender, i);

  if (*ch == '\0')
    {
      fprintf(stderr,"empty message\n");
    }

  /*
  ** Extract the command code from the packet.  Point s to the end
  ** of the command code and calculate the length using pointer
  ** arithmetic.  Note: only need length for numerics and *all*
  ** numerics must have parameters and thus a space after the command
  ** code. -avalon
  *
  * ummm???? - Dianora
  */

  if( *(ch + 3) == ' ' && /* ok, lets see if its a possible numeric.. */
      IsDigit(*ch) && IsDigit(*(ch + 1)) && IsDigit(*(ch + 2)) )
    {
      numeric = ch;
      paramcount = MAXPARA;
      s = ch + 3;       /* I know this is ' ' from above if */
      *s++ = '\0';      /* blow away the ' ', and point s to next part */
    }
  else
    {
      s = strchr(ch, ' ');      /* moved from above,now need it here */
      if (s)
        *s++ = '\0';

      paramcount = 3;
      i = bufend - ((s) ? s : ch);

      if (show)
	{
	  printf("new parse s = %lX ch = %lX\n", s , ch );
	  printf("new parse i = bufend - ((s) ? s : ch) = %d \n", i );
	  printf("new parse: paramcount %d i = %d\n", paramcount, i );
	}
    }

  /*
  ** Must the following loop really be so devious?
  *
  *  nope.
  *
  ** On surface it splits the message to parameters from
  ** blank spaces. But, if paramcount has been reached,
  ** the rest of the message goes into this last parameter
  ** (about same effect as ":" has...) --msa
  */

  /* Note initially true: s==NULL || *(s-1) == '\0' !! */

  para[0] = sender;

  i = 1;
  if (s)
    {
      if (paramcount > MAXPARA)
        paramcount = MAXPARA;

#ifdef NEW_PARSE
      for (;;)
        {
	  /* Never BAD CODE again */

	  while(*s == ' ')	/* tabs are not considered space */
	    s++;

          if (*s == ':')
            {
              /*
              ** The rest is single parameter--can
              ** include blanks also.
              */
              para[i++] = s + 1;
              break;
            }
	  else
	    {
	      para[i++] = s;
	      if (i > paramcount)
		{
		  break;
		}
	      else
		{
		  /* scan for end of string, either ' ' or '\0' */
		  while (IsNonEOS(*s))
		    s++;

		  if(*s == '\0')
		    break;
		  else
		    *s++ = '\0';
		}
	    }
        }
    }

  para[i] = NULL;
  parc = i;
#else
      for (;;)
        {
          /*
          ** Never "FRANCE " again!! ;-) Clean
          ** out *all* blanks.. --msa
          */
          while (*s == ' ')
            *s++ = '\0';

          if (*s == '\0')
            break;
          if (*s == ':')
            {
              /*
              ** The rest is single parameter--can
              ** include blanks also.
              */
              para[++i] = s + 1;
              break;
            }
          para[++i] = s;
          if (i >= paramcount)
            break;
          for (; *s != ' ' && *s; s++)
            /* null statement */ ;
        }
    }

  para[++i] = NULL;
  /*  do_numeric(numeric, cptr, from, i, para); */

  parc = i;
#endif

  if(show)
    {
      printf("parc = %d \n", parc);
      printf("sender [%s]\n", sender );

      for(i=0;i < parc;i++)
	{
	  printf("parv[%d] = [%s]\n", i, para[i]);
	}
    }
}


/*
 * CharAttrs table
 *
 * NOTE: RFC 1459 sez: anything but a ^G, comma, or space is allowed
 * for channel names
 */
const unsigned int CharAttrs[] = {
/* 0  */     CNTRL_C,
/* 1  */     CNTRL_C|CHAN_C|NONEOS_C,
/* 2  */     CNTRL_C|CHAN_C|NONEOS_C,
/* 3  */     CNTRL_C|CHAN_C|NONEOS_C,
/* 4  */     CNTRL_C|CHAN_C|NONEOS_C,
/* 5  */     CNTRL_C|CHAN_C|NONEOS_C,
/* 6  */     CNTRL_C|CHAN_C|NONEOS_C,
/* 7 BEL */  CNTRL_C|NONEOS_C,
/* 8  \b */  CNTRL_C|CHAN_C|NONEOS_C,
/* 9  \t */  CNTRL_C|SPACE_C|CHAN_C|NONEOS_C,
/* 10 \n */  CNTRL_C|SPACE_C|CHAN_C|NONEOS_C,
/* 11 \v */  CNTRL_C|SPACE_C|CHAN_C|NONEOS_C,
/* 12 \f */  CNTRL_C|SPACE_C|CHAN_C|NONEOS_C,
/* 13 \r */  CNTRL_C|SPACE_C|CHAN_C|NONEOS_C,
/* 14 */     CNTRL_C|CHAN_C|NONEOS_C,
/* 15 */     CNTRL_C|CHAN_C|NONEOS_C,
/* 16 */     CNTRL_C|CHAN_C|NONEOS_C,
/* 17 */     CNTRL_C|CHAN_C|NONEOS_C,
/* 18 */     CNTRL_C|CHAN_C|NONEOS_C,
/* 19 */     CNTRL_C|CHAN_C|NONEOS_C,
/* 20 */     CNTRL_C|CHAN_C|NONEOS_C,
/* 21 */     CNTRL_C|CHAN_C|NONEOS_C,
/* 22 */     CNTRL_C|CHAN_C|NONEOS_C,
/* 23 */     CNTRL_C|CHAN_C|NONEOS_C,
/* 24 */     CNTRL_C|CHAN_C|NONEOS_C,
/* 25 */     CNTRL_C|CHAN_C|NONEOS_C,
/* 26 */     CNTRL_C|CHAN_C|NONEOS_C,
/* 27 */     CNTRL_C|CHAN_C|NONEOS_C,
/* 28 */     CNTRL_C|CHAN_C|NONEOS_C,
/* 29 */     CNTRL_C|CHAN_C|NONEOS_C,
/* 30 */     CNTRL_C|CHAN_C|NONEOS_C,
/* 31 */     CNTRL_C|CHAN_C|NONEOS_C,
/* SP */     PRINT_C|SPACE_C,
/* ! */      PRINT_C|KWILD_C|CHAN_C|NONEOS_C,
/* " */      PRINT_C|CHAN_C|NONEOS_C,
/* # */      PRINT_C|CHANPFX_C|CHAN_C|NONEOS_C,
/* $ */      PRINT_C|CHAN_C|NONEOS_C|USER_C,
/* % */      PRINT_C|CHAN_C|NONEOS_C,
/* & */      PRINT_C|CHANPFX_C|CHAN_C|NONEOS_C,
/* ' */      PRINT_C|CHAN_C|NONEOS_C,
/* ( */      PRINT_C|CHAN_C|NONEOS_C,
/* ) */      PRINT_C|CHAN_C|NONEOS_C,
/* * */      PRINT_C|KWILD_C|CHAN_C|NONEOS_C,
/* + */      PRINT_C|CHAN_C|NONEOS_C,
/* , */      PRINT_C|NONEOS_C,
/* - */      PRINT_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* . */      PRINT_C|KWILD_C|CHAN_C|NONEOS_C|HOST_C,
#ifdef RFC1035_ANAL
/* / */      PRINT_C|CHAN_C|NONEOS_C,
#else
/* / */      PRINT_C|CHAN_C|NONEOS_C|HOST_C,
#endif
/* 0 */      PRINT_C|DIGIT_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* 1 */      PRINT_C|DIGIT_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* 2 */      PRINT_C|DIGIT_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* 3 */      PRINT_C|DIGIT_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* 4 */      PRINT_C|DIGIT_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* 5 */      PRINT_C|DIGIT_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* 6 */      PRINT_C|DIGIT_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* 7 */      PRINT_C|DIGIT_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* 8 */      PRINT_C|DIGIT_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* 9 */      PRINT_C|DIGIT_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* : */      PRINT_C|CHAN_C|NONEOS_C,
/* ; */      PRINT_C|CHAN_C|NONEOS_C,
/* < */      PRINT_C|CHAN_C|NONEOS_C,
/* = */      PRINT_C|CHAN_C|NONEOS_C,
/* > */      PRINT_C|CHAN_C|NONEOS_C,
/* ? */      PRINT_C|KWILD_C|CHAN_C|NONEOS_C,
/* @ */      PRINT_C|KWILD_C|CHAN_C|NONEOS_C,
/* A */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* B */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* C */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* D */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* E */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* F */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* G */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* H */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* I */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* J */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* K */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* L */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* M */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* N */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* O */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* P */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* Q */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* R */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* S */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* T */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* U */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* V */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* W */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* X */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* Y */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* Z */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* [ */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C,
/* \ */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C,
/* ] */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C,
/* ^ */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C,
#ifdef RFC1035_ANAL
/* _ */      PRINT_C|NICK_C|CHAN_C|NONEOS_C|USER_C,
#else
/* _ */      PRINT_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
#endif
/* ` */      PRINT_C|NICK_C|CHAN_C|NONEOS_C|USER_C,
/* a */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* b */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* c */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* d */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* e */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* f */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* g */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* h */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* i */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* j */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* k */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* l */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* m */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* n */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* o */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* p */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* q */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* r */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* s */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* t */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* u */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* v */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* w */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* x */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* y */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* z */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* { */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C,
/* | */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C,
/* } */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C,
/* ~ */      PRINT_C|ALPHA_C|CHAN_C|NONEOS_C|USER_C,
/* del  */   CHAN_C|NONEOS_C,
/* 0x80 */   CHAN_C|NONEOS_C,
/* 0x81 */   CHAN_C|NONEOS_C,
/* 0x82 */   CHAN_C|NONEOS_C,
/* 0x83 */   CHAN_C|NONEOS_C,
/* 0x84 */   CHAN_C|NONEOS_C,
/* 0x85 */   CHAN_C|NONEOS_C,
/* 0x86 */   CHAN_C|NONEOS_C,
/* 0x87 */   CHAN_C|NONEOS_C,
/* 0x88 */   CHAN_C|NONEOS_C,
/* 0x89 */   CHAN_C|NONEOS_C,
/* 0x8A */   CHAN_C|NONEOS_C,
/* 0x8B */   CHAN_C|NONEOS_C,
/* 0x8C */   CHAN_C|NONEOS_C,
/* 0x8D */   CHAN_C|NONEOS_C,
/* 0x8E */   CHAN_C|NONEOS_C,
/* 0x8F */   CHAN_C|NONEOS_C,
/* 0x90 */   CHAN_C|NONEOS_C,
/* 0x91 */   CHAN_C|NONEOS_C,
/* 0x92 */   CHAN_C|NONEOS_C,
/* 0x93 */   CHAN_C|NONEOS_C,
/* 0x94 */   CHAN_C|NONEOS_C,
/* 0x95 */   CHAN_C|NONEOS_C,
/* 0x96 */   CHAN_C|NONEOS_C,
/* 0x97 */   CHAN_C|NONEOS_C,
/* 0x98 */   CHAN_C|NONEOS_C,
/* 0x99 */   CHAN_C|NONEOS_C,
/* 0x9A */   CHAN_C|NONEOS_C,
/* 0x9B */   CHAN_C|NONEOS_C,
/* 0x9C */   CHAN_C|NONEOS_C,
/* 0x9D */   CHAN_C|NONEOS_C,
/* 0x9E */   CHAN_C|NONEOS_C,
/* 0x9F */   CHAN_C|NONEOS_C,
/* 0xA0 */   CHAN_C|NONEOS_C,
/* 0xA1 */   CHAN_C|NONEOS_C,
/* 0xA2 */   CHAN_C|NONEOS_C,
/* 0xA3 */   CHAN_C|NONEOS_C,
/* 0xA4 */   CHAN_C|NONEOS_C,
/* 0xA5 */   CHAN_C|NONEOS_C,
/* 0xA6 */   CHAN_C|NONEOS_C,
/* 0xA7 */   CHAN_C|NONEOS_C,
/* 0xA8 */   CHAN_C|NONEOS_C,
/* 0xA9 */   CHAN_C|NONEOS_C,
/* 0xAA */   CHAN_C|NONEOS_C,
/* 0xAB */   CHAN_C|NONEOS_C,
/* 0xAC */   CHAN_C|NONEOS_C,
/* 0xAD */   CHAN_C|NONEOS_C,
/* 0xAE */   CHAN_C|NONEOS_C,
/* 0xAF */   CHAN_C|NONEOS_C,
/* 0xB0 */   CHAN_C|NONEOS_C,
/* 0xB1 */   CHAN_C|NONEOS_C,
/* 0xB2 */   CHAN_C|NONEOS_C,
/* 0xB3 */   CHAN_C|NONEOS_C,
/* 0xB4 */   CHAN_C|NONEOS_C,
/* 0xB5 */   CHAN_C|NONEOS_C,
/* 0xB6 */   CHAN_C|NONEOS_C,
/* 0xB7 */   CHAN_C|NONEOS_C,
/* 0xB8 */   CHAN_C|NONEOS_C,
/* 0xB9 */   CHAN_C|NONEOS_C,
/* 0xBA */   CHAN_C|NONEOS_C,
/* 0xBB */   CHAN_C|NONEOS_C,
/* 0xBC */   CHAN_C|NONEOS_C,
/* 0xBD */   CHAN_C|NONEOS_C,
/* 0xBE */   CHAN_C|NONEOS_C,
/* 0xBF */   CHAN_C|NONEOS_C,
/* 0xC0 */   CHAN_C|NONEOS_C,
/* 0xC1 */   CHAN_C|NONEOS_C,
/* 0xC2 */   CHAN_C|NONEOS_C,
/* 0xC3 */   CHAN_C|NONEOS_C,
/* 0xC4 */   CHAN_C|NONEOS_C,
/* 0xC5 */   CHAN_C|NONEOS_C,
/* 0xC6 */   CHAN_C|NONEOS_C,
/* 0xC7 */   CHAN_C|NONEOS_C,
/* 0xC8 */   CHAN_C|NONEOS_C,
/* 0xC9 */   CHAN_C|NONEOS_C,
/* 0xCA */   CHAN_C|NONEOS_C,
/* 0xCB */   CHAN_C|NONEOS_C,
/* 0xCC */   CHAN_C|NONEOS_C,
/* 0xCD */   CHAN_C|NONEOS_C,
/* 0xCE */   CHAN_C|NONEOS_C,
/* 0xCF */   CHAN_C|NONEOS_C,
/* 0xD0 */   CHAN_C|NONEOS_C,
/* 0xD1 */   CHAN_C|NONEOS_C,
/* 0xD2 */   CHAN_C|NONEOS_C,
/* 0xD3 */   CHAN_C|NONEOS_C,
/* 0xD4 */   CHAN_C|NONEOS_C,
/* 0xD5 */   CHAN_C|NONEOS_C,
/* 0xD6 */   CHAN_C|NONEOS_C,
/* 0xD7 */   CHAN_C|NONEOS_C,
/* 0xD8 */   CHAN_C|NONEOS_C,
/* 0xD9 */   CHAN_C|NONEOS_C,
/* 0xDA */   CHAN_C|NONEOS_C,
/* 0xDB */   CHAN_C|NONEOS_C,
/* 0xDC */   CHAN_C|NONEOS_C,
/* 0xDD */   CHAN_C|NONEOS_C,
/* 0xDE */   CHAN_C|NONEOS_C,
/* 0xDF */   CHAN_C|NONEOS_C,
/* 0xE0 */   CHAN_C|NONEOS_C,
/* 0xE1 */   CHAN_C|NONEOS_C,
/* 0xE2 */   CHAN_C|NONEOS_C,
/* 0xE3 */   CHAN_C|NONEOS_C,
/* 0xE4 */   CHAN_C|NONEOS_C,
/* 0xE5 */   CHAN_C|NONEOS_C,
/* 0xE6 */   CHAN_C|NONEOS_C,
/* 0xE7 */   CHAN_C|NONEOS_C,
/* 0xE8 */   CHAN_C|NONEOS_C,
/* 0xE9 */   CHAN_C|NONEOS_C,
/* 0xEA */   CHAN_C|NONEOS_C,
/* 0xEB */   CHAN_C|NONEOS_C,
/* 0xEC */   CHAN_C|NONEOS_C,
/* 0xED */   CHAN_C|NONEOS_C,
/* 0xEE */   CHAN_C|NONEOS_C,
/* 0xEF */   CHAN_C|NONEOS_C,
/* 0xF0 */   CHAN_C|NONEOS_C,
/* 0xF1 */   CHAN_C|NONEOS_C,
/* 0xF2 */   CHAN_C|NONEOS_C,
/* 0xF3 */   CHAN_C|NONEOS_C,
/* 0xF4 */   CHAN_C|NONEOS_C,
/* 0xF5 */   CHAN_C|NONEOS_C,
/* 0xF6 */   CHAN_C|NONEOS_C,
/* 0xF7 */   CHAN_C|NONEOS_C,
/* 0xF8 */   CHAN_C|NONEOS_C,
/* 0xF9 */   CHAN_C|NONEOS_C,
/* 0xFA */   CHAN_C|NONEOS_C,
/* 0xFB */   CHAN_C|NONEOS_C,
/* 0xFC */   CHAN_C|NONEOS_C,
/* 0xFD */   CHAN_C|NONEOS_C,
/* 0xFE */   CHAN_C|NONEOS_C,
/* 0xFF */   CHAN_C|NONEOS_C
};

