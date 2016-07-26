//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		replay handler & parser
//
// $NoKeywords: $osr
//===============================================================================//

#ifndef OSUREPLAY_H
#define OSUREPLAY_H

#include "cbase.h"

class OsuReplay
{
public:
	enum Mods
	{
	    None           = 0,
	    NoFail         = 1,
	    Easy           = 2,
	    //NoVideo      = 4,
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
	    Relax2         = 8192,  // Autopilot?
	    Perfect        = 16384,
	    Key4           = 32768,
	    Key5           = 65536,
	    Key6           = 131072,
	    Key7           = 262144,
	    Key8           = 524288,
	    keyMod         = Key4 | Key5 | Key6 | Key7 | Key8,
	    FadeIn         = 1048576,
	    Random         = 2097152,
	    LastMod        = 4194304,
	    FreeModAllowed = NoFail | Easy | Hidden | HardRock | SuddenDeath | Flashlight | FadeIn | Relax | Relax2 | SpunOut | keyMod,
	    Key9           = 16777216,
	    Key10          = 33554432,
	    Key1           = 67108864,
	    Key3           = 134217728,
	    Key2           = 268435456
	};

	OsuReplay();
	virtual ~OsuReplay();

private:
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

#endif
