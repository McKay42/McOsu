//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		generic checkbox
//
// $NoKeywords: $osucb
//===============================================================================//

#ifndef OSUUICHECKBOX_H
#define OSUUICHECKBOX_H

#include "CBaseUICheckbox.h"

class Osu;

class OsuUICheckbox : public CBaseUICheckbox
{
public:
	OsuUICheckbox(Osu *osu, float xPos, float yPos, float xSize, float ySize, UString name, UString text);
	virtual ~OsuUICheckbox();

	virtual void update();

	void setTooltipText(UString text);

private:
	Osu *m_osu;
	std::vector<UString> m_tooltipTextLines;
};

#endif
