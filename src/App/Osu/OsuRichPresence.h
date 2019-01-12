//================ Copyright (c) 2018, PG, All rights reserved. =================//
//
// Purpose:		generalized rich presence handler
//
// $NoKeywords: $rpt
//===============================================================================//

#ifndef OSURICHPRESENCE_H
#define OSURICHPRESENCE_H

#include "cbase.h"

class ConVar;

class Osu;

class OsuRichPresence
{
public:
	static void onMainMenu(Osu *osu);
	static void onSongBrowser(Osu *osu);
	static void onPlayStart(Osu *osu);
	static void onPlayEnd(Osu *osu, bool quit);

	static void onRichPresenceChange(UString oldValue, UString newValue);

private:
	static const UString KEY_STEAM_STATUS;
	static const UString KEY_DISCORD_STATUS;
	static const UString KEY_DISCORD_DETAILS;

	static ConVar *m_name_ref;

	static void setStatus(Osu *osu, UString status, bool force = false);

	static void onRichPresenceEnable();
	static void onRichPresenceDisable();
};

#endif
