/*********************************************************
 * help.h - /helpsys related routines                    *
 *********************************************************
 * (C) PTlink IRC Software 1999-2005  Lamego@PTlink.net  *
 *********************************************************/
#ifndef include_help_h
#define include_help_h

typedef struct helpitem_ HelpItem;
struct helpitem_ {
    char* topic;
    char** text;
    HelpItem *next;
};

int help_load(char *fn, HelpItem **hi);
char** help_find(char* topic, HelpItem *hi);
void help_free(HelpItem **hi);
int help_to_html(char *destdir, HelpItem **hi);
extern HelpItem	*user_help, *oper_help, *admin_help;	/* help systems */

#endif
