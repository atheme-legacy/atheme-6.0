/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Copyright (c) 2005-2007 Atheme Development Group
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains protocol support for charybdis-based ircd.
 *
 * $Id: charybdis.c 8223 2007-05-05 12:58:06Z jilles $
 */

#include "atheme.h"
#include "uplink.h"
#include "pmodule.h"
#include "protocol/charybdis.h"
#include "protocol/ircd-seven.h"

DECLARE_MODULE_V1("protocol/ircd-seven", true, _modinit, NULL, "$Id: charybdis.c 8223 2007-05-05 12:58:06Z jilles $", "Atheme Development Group <http://www.atheme.org>");

/* *INDENT-OFF* */

ircd_t Seven = {
        "ircd-seven",			/* IRCd name */
        "$$",                           /* TLD Prefix, used by Global. */
        true,                           /* Whether or not we use IRCNet/TS6 UID */
        false,                          /* Whether or not we use RCOMMAND */
        false,                          /* Whether or not we support channel owners. */
        false,                          /* Whether or not we support channel protection. */
        false,                          /* Whether or not we support halfops. */
	false,				/* Whether or not we use P10 */
	false,				/* Whether or not we use vHosts. */
	CMODE_EXLIMIT | CMODE_PERM | CMODE_IMMUNE, /* Oper-only cmodes */
        0,                              /* Integer flag for owner channel flag. */
        0,                              /* Integer flag for protect channel flag. */
        0,                              /* Integer flag for halfops. */
        "+",                            /* Mode we set for owner. */
        "+",                            /* Mode we set for protect. */
        "+",                            /* Mode we set for halfops. */
	PROTOCOL_CHARYBDIS,		/* Protocol type */
	CMODE_PERM,                     /* Permanent cmodes */
	CMODE_IMMUNE,                   /* Oper-immune cmode */
	"beIq",                         /* Ban-like cmodes */
	'e',                            /* Except mchar */
	'I',                            /* Invex mchar */
	IRCD_CIDR_BANS | IRCD_HOLDNICK  /* Flags */
};

struct cmode_ seven_mode_list[] = {
  { 'i', CMODE_INVITE },
  { 'm', CMODE_MOD    },
  { 'n', CMODE_NOEXT  },
  { 'p', CMODE_PRIV   },
  { 's', CMODE_SEC    },
  { 't', CMODE_TOPIC  },
  { 'c', CMODE_NOCOLOR},
  { 'r', CMODE_REGONLY},
  { 'z', CMODE_OPMOD  },
  { 'g', CMODE_FINVITE},
  { 'L', CMODE_EXLIMIT},
  { 'P', CMODE_PERM   },
  { 'F', CMODE_FTARGET},
  { 'Q', CMODE_DISFWD },
  { 'M', CMODE_IMMUNE },
  { '\0', 0 }
};

struct cmode_ seven_user_mode_list[] = {
  { 'p', UF_IMMUNE   },
  { 'a', UF_ADMIN    },
  { 'i', UF_INVIS    },
  { 'o', UF_IRCOP    },
  { '\0', 0 }
};

/* *INDENT-ON* */

static bool seven_is_valid_hostslash(const char *host)
{
        const char *p;
        bool dot = false;

        if (*host == '.' || *host == '/' || *host == ':')
                return false;

        for (p = host; *p != '\0'; p++)
        {
                if (*p == '.' || *p == ':' || *p == '/')
                        dot = true;
                else if (!((*p >= '0' && *p <= '9') || (*p >= 'A' && *p <= 'Z') ||
                                        (*p >= 'a' && *p <= 'z') || *p == '-'))
                        return false;
        }
        /* hyperion allows a trailing / but RichiH does not want it, whatever */
        if (dot && p[-1] == '/')
                return false;
        return dot;
}

void _modinit(module_t * m)
{
	MODULE_TRY_REQUEST_DEPENDENCY(m, "protocol/charybdis");

	mode_list = seven_mode_list;
	user_mode_list = seven_user_mode_list;

	is_valid_host = &seven_is_valid_hostslash;

	ircd = &Seven;

	m->mflags = MODTYPE_CORE;

	pmodule_loaded = true;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */