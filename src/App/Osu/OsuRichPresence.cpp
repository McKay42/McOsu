//================ Copyright (c) 2018, PG, All rights reserved. =================//
//
// Purpose:		generalized rich presence handler
//
// $NoKeywords: $rpt
//===============================================================================//

#include "OsuRichPresence.h"

#include "Engine.h"
#include "ConVar.h"

#ifdef MCENGINE_FEATURE_STEAMWORKS

#include "SteamworksInterface.h"

#endif

#ifdef MCENGINE_FEATURE_DISCORD

#include "DiscordInterface.h"

#endif

#include "Osu.h"
#include "OsuScore.h"
#include "OsuSongBrowser2.h"
#include "OsuDatabase.h"

#include "OsuBeatmap.h"
#include "OsuBeatmapDifficulty.h"

ConVar osu_rich_presence("osu_rich_presence", true, OsuRichPresence::onRichPresenceChange);
ConVar osu_rich_presence_show_recentplaystats("osu_rich_presence_show_recentplaystats", true);
ConVar osu_rich_presence_discord_show_totalpp("osu_rich_presence_discord_show_totalpp", true);

ConVar *OsuRichPresence::m_name_ref = NULL;

const UString OsuRichPresence::KEY_STEAM_STATUS = "status";
const UString OsuRichPresence::KEY_DISCORD_STATUS = "state";
const UString OsuRichPresence::KEY_DISCORD_DETAILS = "details";

void OsuRichPresence::onMainMenu(Osu *osu)
{
	setStatus(osu, "Main Menu");
}

void OsuRichPresence::onSongBrowser(Osu *osu)
{
	setStatus(osu, "Song Selection");
}

void OsuRichPresence::onPlayStart(Osu *osu)
{
	UString playingInfo /*= "Playing "*/;
	playingInfo.append(osu->getSelectedBeatmap()->getSelectedDifficulty()->artist);
	playingInfo.append(" - ");
	playingInfo.append(osu->getSelectedBeatmap()->getSelectedDifficulty()->title);
	playingInfo.append(" [");
	playingInfo.append(osu->getSelectedBeatmap()->getSelectedDifficulty()->name);
	playingInfo.append("]");

	setStatus(osu, playingInfo);
}

void OsuRichPresence::onPlayEnd(Osu *osu, bool quit)
{
	if (!quit && osu_rich_presence_show_recentplaystats.getBool())
	{
		const bool isUnranked = (osu->getModAuto() || (osu->getModAutopilot() && osu->getModRelax()));

		if (!isUnranked)
		{
			// e.g.: 230pp 900x 95.50% HDHRDT 6*

			// pp
			UString scoreInfo = UString::format("%ipp", (int)(std::round(osu->getScore()->getPPv2())));

			// max combo
			scoreInfo.append(UString::format(" %ix", osu->getScore()->getComboMax()));

			// accuracy
			scoreInfo.append(UString::format(" %.2f%%", osu->getScore()->getAccuracy()*100.0f));

			// mods
			UString mods = osu->getScore()->getModsString();
			if (mods.length() > 0)
			{
				scoreInfo.append(" ");
				scoreInfo.append(mods);
			}

			// stars
			scoreInfo.append(UString::format(" %.2f*", osu->getScore()->getStarsTomTotal()));

			setStatus(osu, scoreInfo);
		}
	}
}

void OsuRichPresence::setStatus(Osu *osu, UString status, bool force)
{
	if (!osu_rich_presence.getBool() && !force) return;

#ifdef MCENGINE_FEATURE_STEAMWORKS

	// steam
	steam->setRichPresence(KEY_STEAM_STATUS, status);

#endif

#ifdef MCENGINE_FEATURE_DISCORD

	// discord
	discord->setRichPresence("largeImageKey", "logo_512", true);
	discord->setRichPresence("smallImageKey", "logo_discord_512_blackfill", true);
	discord->setRichPresence("largeImageText", osu_rich_presence_discord_show_totalpp.getBool() ? "Top = Status / Recent Play; Bottom = Total weighted pp (McOsu scores only!)" : "", true);
	discord->setRichPresence("smallImageText", osu_rich_presence_discord_show_totalpp.getBool() ? "Total weighted pp only work after the database has been loaded!" : "", true);
	discord->setRichPresence(KEY_DISCORD_DETAILS, status);

	if (osu != NULL)
	{
		if (osu_rich_presence_discord_show_totalpp.getBool())
		{
			if (m_name_ref == NULL)
				m_name_ref = convar->getConVarByName("name");

			const int ppRounded = (int)(std::round(osu->getSongBrowser()->getDatabase()->calculatePlayerStats(m_name_ref->getString()).pp));
			if (ppRounded > 0)
				discord->setRichPresence(KEY_DISCORD_STATUS, UString::format("%ipp (Mc)", ppRounded));
		}
	}
	else if (force && status.length() < 1)
		discord->setRichPresence(KEY_DISCORD_STATUS, "");
#endif
}

void OsuRichPresence::onRichPresenceChange(UString oldValue, UString newValue)
{
	if (!osu_rich_presence.getBool())
		onRichPresenceDisable();
	else
		onRichPresenceEnable();
}

void OsuRichPresence::onRichPresenceEnable()
{
	setStatus(NULL, "...");
}

void OsuRichPresence::onRichPresenceDisable()
{
	setStatus(NULL, "", true);
}
