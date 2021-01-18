//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		search text overlay
//
// $NoKeywords: $osufind
//===============================================================================//

#ifndef OSUUISEARCHOVERLAY_H
#define OSUUISEARCHOVERLAY_H

#include "CBaseUIElement.h"

class Osu;

class OsuUISearchOverlay : public CBaseUIElement
{
public:
	OsuUISearchOverlay(Osu *osu, float xPos, float yPos, float xSize, float ySize, UString name);

	virtual void draw(Graphics *g);

	void setDrawNumResults(bool drawNumResults) {m_bDrawNumResults = drawNumResults;}
	void setOffsetRight(int offsetRight) {m_iOffsetRight = offsetRight;}

	void setSearchString(UString searchString) {m_sSearchString = searchString;}
	void setNumFoundResults(int numFoundResults) {m_iNumFoundResults = numFoundResults;}

private:
	Osu *m_osu;

	McFont *m_font;

	int m_iOffsetRight;
	bool m_bDrawNumResults;

	UString m_sSearchString;
	int m_iNumFoundResults;
};

#endif
