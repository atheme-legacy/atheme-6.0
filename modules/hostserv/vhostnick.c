/*
 * Copyright (c) 2005 William Pitcock <nenolod -at- nenolod.net>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Allows setting a vhost on a nick
 *
 * $Id: vhost.c 8195 2007-04-25 16:27:08Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"hostserv/vhostnick", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

list_t *hs_cmdtree, *hs_helptree;

static void hs_cmd_vhostnick(sourceinfo_t *si, int parc, char *parv[]);

command_t hs_vhostnick = { "VHOSTNICK", N_("Manages per-nick virtual hosts."), PRIV_USER_VHOST, 2, hs_cmd_vhostnick };

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(hs_cmdtree, "hostserv/main", "hs_cmdtree");
	MODULE_USE_SYMBOL(hs_helptree, "hostserv/main", "hs_helptree");

	command_add(&hs_vhostnick, hs_cmdtree);
	help_addentry(hs_helptree, "VHOSTNICK", "help/hostserv/vhostnick", NULL);
}

void _moddeinit(void)
{
	command_delete(&hs_vhostnick, hs_cmdtree);
	help_delentry(hs_helptree, "VHOSTNICK");
}


static void do_sethost(user_t *u, char *host)
{
	if (!strcmp(u->vhost, host ? host : u->host))
		return;
	strlcpy(u->vhost, host ? host : u->host, HOSTLEN);
	sethost_sts(hostsvs.me->me, u, u->vhost);
}

/* VHOSTNICK <nick> [host] */
static void hs_cmd_vhostnick(sourceinfo_t *si, int parc, char *parv[])
{
	char *target = parv[0];
	char *host = parv[1];
	myuser_t *mu;
	user_t *u;
	metadata_t *md;
	char buf[BUFSIZE];
	node_t *n;
	int found = 0;

	if (!target)
	{
		command_fail(si, fault_needmoreparams, STR_INVALID_PARAMS, "VHOSTNICK");
		command_fail(si, fault_needmoreparams, _("Syntax: VHOSTNICK <nick> [vhost]"));
		return;
	}

	/* find the user... */
	if (!(mu = myuser_find_ext(target)))
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."), target);
		return;
	}

	LIST_FOREACH(n, mu->nicks.head)
	{
		if (!irccasecmp(((mynick_t *)(n->data))->nick, target))
		{
			snprintf(buf, BUFSIZE, "%s:%s", "private:usercloak", ((mynick_t *)(n->data))->nick);
			found++;
		}
	}

	if (!found)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not a valid target."), target);
		return;
	}

	/* deletion action */
	if (!host)
	{
		metadata_delete(mu, buf);
		command_success_nodata(si, _("Deleted vhost for \2%s\2."), target);
		logcommand(si, CMDLOG_ADMIN, "VHOSTNICK:REMOVE: \2%s\2", target);
		u = user_find_named(target);
		if (u != NULL)
		{
			/* Revert to account's vhost */
			md = metadata_find(mu, "private:usercloak");
			do_sethost(u, md ? md->value : NULL);
		}
		return;
	}

	if (!check_vhost_validity(si, host))
		return;

	metadata_add(mu, buf, host);
	command_success_nodata(si, _("Assigned vhost \2%s\2 to \2%s\2."),
			host, target);
	logcommand(si, CMDLOG_ADMIN, "VHOSTNICK:ASSIGN: \2%s\2 to \2%s\2",
			host, target);
	u = user_find_named(target);
	if (u != NULL)
		do_sethost(u, host);
	return;
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */
