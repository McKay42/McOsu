//================ Copyright (c) 2022, PG, All rights reserved. =================//
//
// Purpose:		currently only used for showing the pp/star algorithm versions
//
// $NoKeywords: $
//===============================================================================//

#ifndef OSUUIUSERSTATSSCREENLABEL_H
#define OSUUIUSERSTATSSCREENLABEL_H

#include "CBaseUILabel.h"

class Osu;

class OsuUIUserStatsScreenLabel : public CBaseUILabel
{
public:
	OsuUIUserStatsScreenLabel(Osu *osu, float xPos=0, float yPos=0, float xSize=0, float ySize=0, UString name="", UString text="");

	virtual void update();

	void setTooltipText(UString text) {m_tooltipTextLines = text.split("\n");}

private:
	Osu *m_osu;
	std::vector<UString> m_tooltipTextLines;
};

#endif
