//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		search text overlay
//
// $NoKeywords: $osufind
//===============================================================================//

#include "OsuUISearchOverlay.h"

#include "Engine.h"
#include "ResourceManager.h"

#include "Osu.h"

OsuUISearchOverlay::OsuUISearchOverlay(Osu *osu, float xPos, float yPos, float xSize, float ySize, UString name) : CBaseUIElement(xPos, yPos, xSize, ySize, name)
{
	m_osu = osu;

	m_iOffsetRight = 0;
	m_bDrawNumResults = true;

	m_iNumFoundResults = -1;
}

void OsuUISearchOverlay::draw(Graphics *g)
{
	/*
	g->setColor(0xaaaaaaaa);
	g->fillRect(m_vPos.x, m_vPos.y, m_vSize.x, m_vSize.y);
	*/

	// draw search text and background
	const float searchTextScale = 1.0f;
	McFont *searchTextFont = engine->getResourceManager()->getFont("FONT_DEFAULT");

	UString searchText1 = "Search: ";
	UString searchText2 = "Type to search!";
	UString noMatchesFoundText1 = "No matches found. Hit ESC to reset.";
	UString noMatchesFoundText2 = "Hit ESC to reset.";

	UString combinedSearchText = searchText1;
	combinedSearchText.append(searchText2);

	UString offsetText = (m_iNumFoundResults < 0 ? combinedSearchText : noMatchesFoundText1);

	bool hasSearchSubTextVisible = m_sSearchString.length() > 0 && m_bDrawNumResults;

	const int textWidth = searchTextFont->getStringWidth(offsetText)*searchTextScale + (searchTextFont->getHeight()*searchTextScale) + m_iOffsetRight;
	g->setColor(COLOR(m_sSearchString.length() > 0 ? 100 : 30, 0, 0, 0));
	g->fillRect(m_vPos.x + m_vSize.x - textWidth, m_vPos.y, textWidth, (searchTextFont->getHeight()*searchTextScale)*(hasSearchSubTextVisible ? 4.0f : 3.0f));
	/*
	g->setColor(0xffffff00);
	g->drawRect(m_vPos.x + m_vSize.x - textWidth, m_vPos.y, textWidth, (searchTextFont->getHeight()*searchTextScale)*(hasSearchSubTextVisible ? 4.0f : 3.0f));
	*/
	g->setColor(0xffffffff);
	g->pushTransform();
	{
		g->translate(0, (int)(searchTextFont->getHeight() / 2.0f));
		g->scale(searchTextScale, searchTextScale);
		g->translate((int)(m_vPos.x + m_vSize.x - searchTextFont->getStringWidth(offsetText)*searchTextScale - (searchTextFont->getHeight()*searchTextScale)*0.5f - m_iOffsetRight), (int)(m_vPos.y + (searchTextFont->getHeight()*searchTextScale)*1.5f));

		// draw search text and text
		g->pushTransform();
		{
			g->translate(1, 1);
			g->setColor(0xff000000);
			g->drawString(searchTextFont, searchText1);
			g->translate(-1, -1);
			g->setColor(0xff00ff00);
			g->drawString(searchTextFont, searchText1);

			g->translate((int)(searchTextFont->getStringWidth(searchText1)*searchTextScale), 0);
			g->translate(1, 1);
			g->setColor(0xff000000);
			if (m_sSearchString.length() < 1)
				g->drawString(searchTextFont, searchText2);
			else
				g->drawString(searchTextFont, m_sSearchString);

			g->translate(-1, -1);
			g->setColor(0xffffffff);
			if (m_sSearchString.length() < 1)
				g->drawString(searchTextFont, searchText2);
			else
				g->drawString(searchTextFont, m_sSearchString);
		}
		g->popTransform();

		// draw number of matches
		if (hasSearchSubTextVisible)
		{
			g->translate(0, (int)((searchTextFont->getHeight()*searchTextScale)*1.5f));
			g->translate(1, 1);
			g->setColor(0xff000000);
			g->drawString(searchTextFont, m_iNumFoundResults > -1 ? (m_iNumFoundResults > 0 ? UString::format("%i matches found!", m_iNumFoundResults) : noMatchesFoundText1) : noMatchesFoundText2);
			g->translate(-1, -1);
			g->setColor(0xffffffff);
			g->drawString(searchTextFont, m_iNumFoundResults > -1 ? (m_iNumFoundResults > 0 ? UString::format("%i matches found!", m_iNumFoundResults) : noMatchesFoundText1) : noMatchesFoundText2);
		}
	}
	g->popTransform();
}
