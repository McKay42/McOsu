//================ Copyright (c) 2020, PG, All rights reserved. =================//
//
// Purpose:		handles star calculation async, cache (similar to osubgldr)
//
// $NoKeywords: $osustarldr
//===============================================================================//

#ifndef OSUBACKGROUNDSTARCALCHANDLER_H
#define OSUBACKGROUNDSTARCALCHANDLER_H

#include "cbase.h"

class OsuDatabaseBeatmap;

class OsuBackgroundStarCalcHandler
{
public:
	OsuBackgroundStarCalcHandler();
	~OsuBackgroundStarCalcHandler();

	void calculate(const OsuDatabaseBeatmap *beatmap);

	void interruptAndEvict();

private:
};

#endif
