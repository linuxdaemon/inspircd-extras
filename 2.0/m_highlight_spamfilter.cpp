/*
 * InspIRCd -- Internet Relay Chat Daemon
 *
 *   Copyright (C) 2016 Adam <Adam@anope.org>
 *
 * This file is part of InspIRCd.  InspIRCd is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation, version 2.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


/* $ModAuthor: Adam */
/* $ModAuthorMail: Adam@anope.org */
/* $ModDesc: Block highlight spam */
/* $ModDepends: core 2.0 */

#include "inspircd.h"

class ModuleHighlightSpamfilter : public Module
{
	float percent;
	unsigned int min;

	bool IsSeparator(char c)
	{
		return c == ' ' || c == ',' || c == ':';
	}

	bool IsNickChar(char c)
	{
		if (c >= 'A' && c <= '}')
			return true;
		if (c >= '0' && c <= '9')
			return true;
		if (c == '-')
			return true;
		return false;
	}

	bool IsOnChannel(Channel *c, User *u)
	{
		if (c->userlist.size() < u->chans.size())
			return c->HasUser(u);
		else
			return u->chans.count(c) > 0;
	}

 public:
	ModuleHighlightSpamfilter() : percent(0.0f), min(0)
	{
	}

	void init()
	{
		Implementation eventList[] = { I_OnRehash, I_OnUserPreMessage };
		ServerInstance->Modules->Attach(eventList, this, sizeof(eventList) / sizeof(Implementation));

		OnRehash(NULL);
	}

	void OnRehash(User* user)
	{
		ConfigTag* tag = ServerInstance->Config->ConfValue("highlightspamfilter");
		percent = tag->getFloat("percent", 0.60f);
		min = tag->getInt("min", 15);
	}

	ModResult OnUserPreMessage(User* user, void* dest, int target_type, std::string &text, char status, CUList &exempt_list)
	{
		if (!IS_LOCAL(user) || target_type != TYPE_CHANNEL || percent <= 0.0f)
			return MOD_RES_PASSTHRU;

		Channel *channel = static_cast<Channel *>(dest);
		unsigned int hit = 0, online = 0, miss = 0;

		std::string cur;
		for (unsigned int i = 0; i <= text.size(); ++i)
		{
			if (i < text.size())
			{
				if (IsNickChar(text[i]))
				{
					cur.push_back(text[i]);
					continue;
				}

				if (!IsSeparator(text[i]))
					continue;
			}

			if (cur.empty())
				continue;

			User *u = ServerInstance->FindNickOnly(cur);
			cur.clear();

			if (u == NULL || IS_SERVER(u))
			{
				++miss;
				continue;
			}

			++online;

			if (!IsOnChannel(channel, u))
				continue;

			++hit;
		}

		if (hit == 0)
			return MOD_RES_PASSTHRU;

		unsigned int total = online + miss;

		if (total < min || total == 0)
			return MOD_RES_PASSTHRU;

		if ((float) hit / (float) total > percent)
		{
			user->WriteServ("NOTICE %s :Message to %s blocked due to spam.", user->nick.c_str(), channel->name.c_str());
			return MOD_RES_DENY;
		}

		return MOD_RES_PASSTHRU;
	}

	Version GetVersion()
	{
		return Version("Implements highlight spamfilter");
	}
};

MODULE_INIT(ModuleHighlightSpamfilter)
