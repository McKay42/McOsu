//================ Copyright (c) 2020, PG, All rights reserved. =================//
//
// Purpose:		handles star calculation async, cache (similar to osubgldr)
//
// $NoKeywords: $osustarldr
//===============================================================================//

#include "OsuBackgroundStarCalcHandler.h"

#include "Engine.h"
#include "ResourceManager.h"

#include "OsuDatabaseBeatmap.h"

OsuBackgroundStarCalcHandler::OsuBackgroundStarCalcHandler()
{

}

OsuBackgroundStarCalcHandler::~OsuBackgroundStarCalcHandler()
{

}

void OsuBackgroundStarCalcHandler::calculate(const OsuDatabaseBeatmap *beatmap)
{
	// TODO: spawn OsuDatabaseBeatmapStarCalculator and run async, keep track of all running ones in here
}

void OsuBackgroundStarCalcHandler::interruptAndEvict()
{
	// TODO: destroy EVERYTHING currently being calculated
	// TODO: since we will keep a reference to OsuDatabaseBeatmaps, clearing the database is only allowed if no async calc is running
}
