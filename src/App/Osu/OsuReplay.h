//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		replay handler & parser
//
// $NoKeywords: $osr
//===============================================================================//

#ifndef OSUREPLAY_H
#define OSUREPLAY_H

#include "cbase.h"

// HACKHACK: linux unity build fix (X.h defines "None" globally ffs, and because this isn't an enum class)
#ifdef None
#pragma push_macro("None")
#undef None
#define MCOSU_OSUREPLAY_NONE_POP_MACRO_PENDING
#endif

class OsuReplay
{
public:
	enum Mods
	{
		None           = 0,
		NoFail         = 1,
		Easy           = 2,
		TouchDevice    = 4,
		Hidden         = 8,
		HardRock       = 16,
		SuddenDeath    = 32,
		DoubleTime     = 64,
		Relax          = 128,
		HalfTime       = 256,
		Nightcore      = 512, // Only set along with DoubleTime. i.e: NC only gives 576
		Flashlight     = 1024,
		Autoplay       = 2048,
		SpunOut        = 4096,
		Relax2         = 8192,	// Autopilot
		Perfect        = 16384, // Only set along with SuddenDeath. i.e: PF only gives 16416
		Key4           = 32768,
		Key5           = 65536,
		Key6           = 131072,
		Key7           = 262144,
		Key8           = 524288,
		FadeIn         = 1048576,
		Random         = 2097152,
		Cinema         = 4194304, // abused for NM here
		Nightmare	   = Cinema,
		Target         = 8388608,
		Key9           = 16777216,
		KeyCoop        = 33554432,
		Key1           = 67108864,
		Key3           = 134217728,
		Key2           = 268435456,
		ScoreV2        = 536870912,
		LastMod        = 1073741824,
		KeyMod = Key1 | Key2 | Key3 | Key4 | Key5 | Key6 | Key7 | Key8 | Key9 | KeyCoop,
		FreeModAllowed = NoFail | Easy | Hidden | HardRock | SuddenDeath | Flashlight | FadeIn | Relax | Relax2 | SpunOut | KeyMod,
		ScoreIncreaseMods = Hidden | HardRock | DoubleTime | Flashlight | FadeIn
	};

	struct BEATMAP_VALUES
	{
		float AR;
		float CS;
		float OD;
		float HP;

		float speedMultiplier;

		float difficultyMultiplier;
		float csDifficultyMultiplier;
	};

public:

	OsuReplay();
	virtual ~OsuReplay();

	static BEATMAP_VALUES getBeatmapValuesForModsLegacy(int modsLegacy, float legacyAR, float legacyCS, float legacyOD, float legacyHP);
};

class OsuReplayFile
{
public:

private:
	unsigned char readByte();
	unsigned short readShort();
	unsigned int readInt();
	unsigned long readLong();
	unsigned int readULEB128();
	UString readString(unsigned int length);
};

#ifdef MCOSU_OSUREPLAY_NONE_POP_MACRO_PENDING
#pragma pop_macro("None")
#endif

#endif
