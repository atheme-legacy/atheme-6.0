/*
 * Copyright (c) 2009 Celestin, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains a BService SET which can change
 * botserv settings on channel or bot.
 *
 * $Id: set.c 7969 2009-04-09 19:19:38Z celestin $
 */

#include "atheme.h"
#include "botserv.h"

DECLARE_MODULE_V1
(
	"botserv/set", FALSE, _modinit, _moddeinit,
	"$Id: set.c 7969 2009-04-09 19:19:38Z celestin $",
	"Rizon Development Group <http://dev.rizon.net>"
);

static void bs_help_set(sourceinfo_t *si); 
static void bs_cmd_set(sourceinfo_t *si, int parc, char *parv[]);

static void bs_cmd_set_dontkickops(sourceinfo_t *si, int parc, char *parv[]);
static void bs_cmd_set_dontkickvoices(sourceinfo_t *si, int parc, char *parv[]);
static void bs_cmd_set_greet(sourceinfo_t *si, int parc, char *parv[]);
static void bs_cmd_set_fantasy(sourceinfo_t *si, int parc, char *parv[]);
static void bs_cmd_set_nobot(sourceinfo_t *si, int parc, char *parv[]); 
static void bs_cmd_set_private(sourceinfo_t *si, int parc, char *parv[]); 

command_t bs_set = { "SET", N_("Configures bot options."), AC_NONE, 3, bs_cmd_set };

command_t bs_set_dontkickops    = { "DONTKICKOPS",    N_("To protect ops against bot kicks."),                       AC_NONE, 2, bs_cmd_set_dontkickops };
command_t bs_set_dontkickvoices = { "DONTKICKVOICES", N_("To protect voices against bot kicks."),                    AC_NONE, 2, bs_cmd_set_dontkickvoices };
command_t bs_set_greet          = { "GREET",          N_("Enable greet messages."),                                  AC_NONE, 2, bs_cmd_set_greet };
command_t bs_set_fantasy        = { "FANTASY",        N_("Enable fantaisist commands."),                             AC_NONE, 2, bs_cmd_set_fantasy };
command_t bs_set_nobot          = { "NOBOT",          N_("Prevent a bot from being assigned to a channel."),         PRIV_CHAN_ADMIN, 2, bs_cmd_set_nobot };
command_t bs_set_private        = { "PRIVATE",        N_("Prevent a bot from being assigned by non IRC operators."), PRIV_CHAN_ADMIN, 2, bs_cmd_set_private }; 

command_t *bs_set_commands[] = {
	&bs_set_dontkickops,
	&bs_set_dontkickvoices,
	&bs_set_greet,
	&bs_set_fantasy,
	&bs_set_nobot,
	&bs_set_private,
	NULL
};

fn_botserv_bot_find *botserv_bot_find;
list_t *bs_bots;

list_t *bs_cmdtree;
list_t *bs_helptree;
list_t bs_set_cmdtree; 

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(bs_cmdtree, "botserv/main", "bs_cmdtree");
	MODULE_USE_SYMBOL(bs_helptree, "botserv/main", "bs_helptree");
	MODULE_USE_SYMBOL(bs_bots, "botserv/main", "bs_bots");
	MODULE_USE_SYMBOL(botserv_bot_find, "botserv/main", "botserv_bot_find");
	
	command_add(&bs_set, bs_cmdtree);
	command_add_many(bs_set_commands, &bs_set_cmdtree);
 

	help_addentry(bs_helptree, "SET", NULL, bs_help_set);
	help_addentry(bs_helptree, "SET DONTKICKOPS", "help/botserv/set_dontkickops", NULL);
	help_addentry(bs_helptree, "SET DONTKICKVOICES", "help/botserv/set_dontkickvoices", NULL);
	help_addentry(bs_helptree, "SET GREET", "help/botserv/set_greet", NULL);
	help_addentry(bs_helptree, "SET FANTASY", "help/botserv/set_fantasy", NULL);
	help_addentry(bs_helptree, "SET NOBOT", "help/botserv/set_nobot", NULL);
	help_addentry(bs_helptree, "SET PRIVATE", "help/botserv/set_private", NULL);
}

void _moddeinit()
{
	command_delete(&bs_set, bs_cmdtree);

	help_delentry(bs_helptree, "SET");
	help_delentry(bs_helptree, "SET DONTKICKOPS");
	help_delentry(bs_helptree, "SET DONTKICKVOICES");
	help_delentry(bs_helptree, "SET GREET");
	help_delentry(bs_helptree, "SET FANTASY");
	help_delentry(bs_helptree, "SET NOBOT");
	help_delentry(bs_helptree, "SET PRIVATE");
}

/* ******************************************************************** */ 

static void botserv_save_database(void *unused)
{
	FILE *f;
	node_t *n;
	int errno1;

	if (!(f = fopen(DATADIR "/botserv.db.new", "w")))
	{
		errno1 = errno;
		slog(LG_ERROR, "botserv_save_database(): cannot create botserv.db.new: %s", strerror(errno1));
		wallops(_("\2DATABASE ERROR\2: botserv_save_database(): cannot create botserv.db.new: %s"), strerror(errno1));
		snoop(_("\2DATABASE ERROR\2: botserv_save_database(): cannot create botserv.db.new: %s"), strerror(errno1));
		return;
	}

	/* write database version */
	fprintf(f, "DBV 1\n");

	/* iterate through and write all the metadata */
	LIST_FOREACH(n, bs_bots->head)
	{
		botserv_bot_t *bot = (botserv_bot_t *) n->data;

		fprintf(f, "BOT %s %s %s %d %ld %s\n", bot->nick, bot->user, bot->host, bot->private, (long)bot->registered, bot->real);
	}

	fprintf(f, "COUNT %d\n", bs_bots->count);

	fclose(f);

	/* use an atomic rename */
	unlink(DATADIR "/botserv.db");

	if ((rename(DATADIR "/botserv.db.new", DATADIR "/botserv.db")) < 0)
	{
		errno1 = errno;
		slog(LG_ERROR, "botserv_save_database(): cannot rename botserv.db.new to botserv.db: %s", strerror(errno1));
		wallops(_("\2DATABASE ERROR\2: botserv_save_database(): cannot rename botserv.db.new to botserv.db: %s"), strerror(errno1));
		snoop(_("\2DATABASE ERROR\2: botserv_save_database(): cannot rename botserv.db.new to botserv.db: %s"), strerror(errno1));
		return;
	}
}

/* ******************************************************************** */

static void bs_help_set(sourceinfo_t *si)
{
	command_success_nodata(si, _("Help for \2SET\2:"));
	command_success_nodata(si, " ");
	command_success_nodata(si, _("Configures different botserv bot options."));
	command_success_nodata(si, " ");
	command_help(si, &bs_set_cmdtree);
	command_success_nodata(si, " ");
	command_success_nodata(si, _("For more specific help use \2/msg %s HELP SET \37command\37\2."), si->service->disp); 
}

static void bs_cmd_set(sourceinfo_t *si, int parc, char *parv[])
{
	char *dest;
	char *cmd;
	command_t *c;

	if (parc < 3)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET");
		command_fail(si, fault_needmoreparams, _("Syntax: SET <destination> <setting> <parameters>"));
		return;
	}

	dest = parv[0];
	cmd = parv[1];
	c = command_find(&bs_set_cmdtree, cmd);
	if (c == NULL)
	{
		command_fail(si, fault_badparams, _("Invalid command. Use \2/%s%s help\2 for a command listing."), (ircd->uses_rcommand == FALSE) ? "msg " : "", si->service->disp);
		return;
	}

	parv[1] = dest;
	command_exec(si->service, si, c, parc - 1, parv + 1); 
}

static void bs_cmd_set_dontkickops(sourceinfo_t *si, int parc, char *parv[])
{
	char *channel = parv[0];
	char *option = parv[1];
	mychan_t *mc;
 
	if (parc < 2 || !channel || !option)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET DONTKICKOPS");
		command_fail(si, fault_needmoreparams, _("Syntax: SET <#channel> DONTKICKOPS {ON|OFF}"));
		return;
	}

	mc = mychan_find(channel);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), channel);
		return;
	}

	if (!si->smu)
	{
		command_fail(si, fault_noprivs, _("You are not logged in."));
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_SET) && !has_priv(si, PRIV_CHAN_ADMIN))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
		return;
	}

	if(!irccasecmp(option, "ON")) 
	{
		metadata_add(mc, "private:botserv:bot-dontkick-ops", "ON"); 
		command_success_nodata(si, _("Bot \2won't kick ops\2 on channel %s."), mc->name); 
	}
	else if(!irccasecmp(option, "OFF")) 
	{
		metadata_delete(mc, "private:botserv:bot-dontkick-ops"); 
		command_success_nodata(si, _("Bot \2will kick ops\2 on channel %s."), mc->name); 
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET DONTKICKOPS");
		command_fail(si, fault_badparams, _("Syntax: SET <#channel> DONTKICKOPS {ON|OFF}"));
	}
}

static void bs_cmd_set_dontkickvoices(sourceinfo_t *si, int parc, char *parv[])
{
	char *channel = parv[0];
	char *option = parv[1];
	mychan_t *mc;
 
	if (parc < 2 || !channel || !option)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET DONTKICKVOICES");
		command_fail(si, fault_needmoreparams, _("Syntax: SET <#channel> DONTKICKVOICES {ON|OFF}"));
		return;
	}

	mc = mychan_find(channel);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), channel);
		return;
	}

	if (!si->smu)
	{
		command_fail(si, fault_noprivs, _("You are not logged in."));
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_SET) && !has_priv(si, PRIV_CHAN_ADMIN))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
		return;
	}

	if(!irccasecmp(option, "ON")) 
	{
		metadata_add(mc, "private:botserv:bot-dontkick-voices", "ON"); 
		command_success_nodata(si, _("Bot \2won't kick voices\2 on channel %s."), mc->name); 
	}
	else if(!irccasecmp(option, "OFF")) 
	{
		metadata_delete(mc, "private:botserv:bot-dontkick-voices"); 
		command_success_nodata(si, _("Bot \2will kick voices\2 on channel %s."), mc->name); 
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET DONTKICKVOICES");
		command_fail(si, fault_badparams, _("Syntax: SET <#channel> DONTKICKVOICES {ON|OFF}"));
	}
}

static void bs_cmd_set_greet(sourceinfo_t *si, int parc, char *parv[])
{
	char *channel = parv[0];
	char *option = parv[1];
	mychan_t *mc;
 
	if (parc < 2 || !channel || !option)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET GREET");
		command_fail(si, fault_needmoreparams, _("Syntax: SET <#channel> GREET {ON|OFF}"));
		return;
	}

	mc = mychan_find(channel);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), channel);
		return;
	}

	if (!si->smu)
	{
		command_fail(si, fault_noprivs, _("You are not logged in."));
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_SET) && !has_priv(si, PRIV_CHAN_ADMIN))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
		return;
	}

	if(!irccasecmp(option, "ON")) 
	{
		metadata_add(mc, "private:botserv:bot-handle-greet", "ON"); 
		command_success_nodata(si, _("Greet mode is now \2ON\2 on channel %s."), mc->name); 
	}
	else if(!irccasecmp(option, "OFF")) 
	{
		metadata_delete(mc, "private:botserv:bot-handle-greet"); 
		command_success_nodata(si, _("Greet mode is now \2OFF\2 on channel %s."), mc->name); 
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET GREET");
		command_fail(si, fault_badparams, _("Syntax: SET <#channel> GREET {ON|OFF}"));
	}
}

static void bs_cmd_set_fantasy(sourceinfo_t *si, int parc, char *parv[])
{
	char *channel = parv[0];
	char *option = parv[1];
	mychan_t *mc;
 
	if (parc < 2 || !channel || !option)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET FANTASY");
		command_fail(si, fault_needmoreparams, _("Syntax: SET <#channel> FANTASY {ON|OFF}"));
		return;
	}

	mc = mychan_find(channel);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), channel);
		return;
	}

	if (!si->smu)
	{
		command_fail(si, fault_noprivs, _("You are not logged in."));
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_SET) && !has_priv(si, PRIV_CHAN_ADMIN))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
		return;
	}

	if(!irccasecmp(option, "ON")) 
	{
		metadata_add(mc, "private:botserv:bot-handle-fantasy", "ON"); 
		command_success_nodata(si, _("Fantasy mode is now \2ON\2 on channel %s."), mc->name); 
	}
	else if(!irccasecmp(option, "OFF")) 
	{
		metadata_delete(mc, "private:botserv:bot-handle-fantasy"); 
		command_success_nodata(si, _("Fantasy mode is now \2OFF\2 on channel %s."), mc->name); 
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET FANTASY");
		command_fail(si, fault_badparams, _("Syntax: SET <#channel> FANTASY {ON|OFF}"));
	}
}

static void bs_cmd_set_nobot(sourceinfo_t *si, int parc, char *parv[])
{
	char *channel = parv[0];
	char *option = parv[1];
	mychan_t *mc;
	metadata_t *md;
 
	if (parc < 2 || !channel || !option)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET NOBOT");
		command_fail(si, fault_needmoreparams, _("Syntax: SET <#channel> NOBOT {ON|OFF}"));
		return;
	}

	mc = mychan_find(channel);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), channel);
		return;
	}

	if (!si->smu)
	{
		command_fail(si, fault_noprivs, _("You are not logged in."));
		return;
	}

	if (!has_priv(si, PRIV_CHAN_ADMIN))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
		return;
	}

	if(!irccasecmp(option, "ON")) 
	{
		metadata_add(mc, "private:botserv:no-bot", "ON"); 
		if ((md = metadata_find(mc, "private:botserv:bot-assigned")) != NULL)
		{
			part(mc->name, md->value);
			metadata_delete(mc, "private:botserv:bot-assigned");
			metadata_delete(mc, "private:botserv:bot-handle-fantasy");
			if ((mc->flags & MC_GUARD) && (mc->chan != NULL && mc->chan->members.count != 0))
			{
				join(mc->name, chansvs.nick);
			} 
		}
		command_success_nodata(si, _("No Bot mode is now \2ON\2 on channel %s."), mc->name); 
	}
	else if(!irccasecmp(option, "OFF")) 
	{
		metadata_delete(mc, "private:botserv:no-bot"); 
		command_success_nodata(si, _("No Bot mode is now \2OFF\2 on channel %s."), mc->name); 
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET NOBOT");
		command_fail(si, fault_badparams, _("Syntax: SET <#channel> NOBOT {ON|OFF}"));
	}
}

static void bs_cmd_set_private(sourceinfo_t *si, int parc, char *parv[])
{
	char *botserv = parv[0];
	char *option = parv[1];
	botserv_bot_t *bot; 
 
	if (parc < 2 || !botserv || !option)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET PRIVATE");
		command_fail(si, fault_needmoreparams, _("Syntax: SET <botnick> PRIVATE {ON|OFF}"));
		return;
	}

	bot = botserv_bot_find(botserv);
	if (!bot)
	{
		command_fail(si, fault_nosuch_target, _("\2%s\2 is not a bot."), botserv);
		return;
	}

	if (!si->smu)
	{
		command_fail(si, fault_noprivs, _("You are not logged in."));
		return;
	}

	if (!has_priv(si, PRIV_CHAN_ADMIN))
	{
		command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
		return;
	}

	if(!irccasecmp(option, "ON")) 
	{
		bot->private = 1; 
		botserv_save_database(NULL);
		command_success_nodata(si, _("Private mode of bot %s is now \2ON\2."), bot->nick); 
	}
	else if(!irccasecmp(option, "OFF")) 
	{
		bot->private = 0; 
		botserv_save_database(NULL);
		command_success_nodata(si, _("Private mode of bot %s is now \2OFF\2."), bot->nick); 
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET PRIVATE");
		command_fail(si, fault_badparams, _("Syntax: SET <botnick> PRIVATE {ON|OFF}"));
	}
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */ 
 
 