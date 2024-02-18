//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		difficulty & plafield behaviour
//
// $NoKeywords: $osugrm
//===============================================================================//

#include "OsuGameRulesMania.h"

ConVar OsuGameRulesMania::osu_mania_hitwindow_od_add_multiplier("osu_mania_hitwindow_od_add_multiplier", 3, FCVAR_NONE, "finalhitwindow = hitwindow + (od_add_multiplier * invertedOD)");
ConVar OsuGameRulesMania::osu_mania_hitwindow_300r("osu_mania_hitwindow_300r", 16, FCVAR_NONE);
ConVar OsuGameRulesMania::osu_mania_hitwindow_300("osu_mania_hitwindow_300", 34, FCVAR_NONE);
ConVar OsuGameRulesMania::osu_mania_hitwindow_200("osu_mania_hitwindow_200", 67, FCVAR_NONE);
ConVar OsuGameRulesMania::osu_mania_hitwindow_100("osu_mania_hitwindow_100", 97, FCVAR_NONE);
ConVar OsuGameRulesMania::osu_mania_hitwindow_50("osu_mania_hitwindow_50", 121, FCVAR_NONE);
ConVar OsuGameRulesMania::osu_mania_hitwindow_miss("osu_mania_hitwindow_miss", 158, FCVAR_NONE);
