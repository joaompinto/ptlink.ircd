/* m_codepage.c -- change current codepage
 * Copyright (C) 2001-2002, ForestNet IRC Network
 * This is a free software; see file `license' for details.
 */

#include "struct.h"
#include "send.h"
#include "numeric.h"
#include "msg.h"
#include "channel.h"
#include "unicode.h"
#include "client.h"
#include "ircd.h"
#include "m_commands.h"
#include "s_serv.h"

#include <strings.h>

#define cp_utf8 "unicode [utf8]"

/* parv[0] - codepage name, parv[1] - nick to set codepage for */
int m_codepage(aClient *cptr, aClient *sptr, int parc, char *parv[])
{
	int idx;
	aClient *acptr;

	if (check_registered(sptr))
		return 0;

	if (parc < 2) {
		send_syntax(sptr, MSG_CODEPAGE, "CODEPAGE name [nick] "
			"(current: %s, available codepages: %s)", sptr->codepage
			? cps[sptr->codepage - 1]->name : cp_utf8,
			codepage_list());
		return 0;
	}

	if (parc > 2 && MyClient(sptr) && !IsOper(sptr)) {
		sendto_one(sptr, form_str(ERR_NOPRIVILEGES), me.name, parv[0]);
		return 0;
	}

	if (parc > 2 && hunt_server(cptr, sptr, ":%s CODEPAGE %s %s",
		2, parc, parv) != HUNTED_ISME) return 0;

	acptr = (parc > 2) ? find_person(parv[2], 0) : sptr;

	if (!strcasecmp(parv[1], "utf8") || !strcasecmp(parv[1], "none"))
		idx = -1;
	else if ((idx = codepage_find(parv[1])) == -1) {
		sendto_one(sptr, ":%s NOTICE %s :Unknown codepage %s, "
			"available codepages are: %s. Current codepage: `%s'.",
			me.name, sptr->name, parv[1], codepage_list(),
			acptr->codepage ? cps[acptr->codepage-1]->name : cp_utf8);
		return 0;
	}

	if (sptr == acptr)
		sendto_one(acptr, ":%s NOTICE %s :Your codepage has been set "
			"to %s.", me.name, acptr->name,
			idx == -1 ? cp_utf8 : cps[idx]->name);
	else
		sendto_one(acptr, ":%s NOTICE %s :Your codepage has been set "
			"to %s by %s.", me.name, acptr->name,
			idx == -1 ? cp_utf8 : cps[idx]->name, sptr->name);

	if (acptr->codepage && cps[acptr->codepage - 1])
		--(cps[acptr->codepage-1]->refcount);

	acptr->codepage = (idx + 1)  & 0xff;
	if (idx >= 0)
		++(cps[idx]->refcount);

	return 0;
}
