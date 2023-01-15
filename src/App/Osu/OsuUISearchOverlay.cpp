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

	m_font = engine->getResourceManager()->getFont("FONT_DEFAULT");

	m_iOffsetRight = 0;
	m_bDrawNumResults = true;

	m_iNumFoundResults = -1;

	m_bSearching = false;
}

void OsuUISearchOverlay::draw(Graphics *g)
{
	/*
	g->setColor(0xaaaaaaaa);
	g->fillRect(m_vPos.x, m_vPos.y, m_vSize.x, m_vSize.y);
	*/

	// draw search text and background
	const float searchTextScale = 1.0f;
	McFont *searchTextFont = m_font;

	const UString searchText1 = "Search: ";
	const UString searchText2 = "Type to search!";
	const UString noMatchesFoundText1 = "No matches found. Hit ESC to reset.";
	const UString noMatchesFoundText2 = "Hit ESC to reset.";
	const UString searchingText2 = "Searching, please wait ...";

	UString combinedSearchText = searchText1;
	combinedSearchText.append(searchText2);

	UString offsetText = (m_iNumFoundResults < 0 ? combinedSearchText : noMatchesFoundText1);

	bool hasSearchSubTextVisible = m_sSearchString.length() > 0 && m_bDrawNumResults;

	const float searchStringWidth = searchTextFont->getStringWidth(m_sSearchString);
	const float offsetTextStringWidth = searchTextFont->getStringWidth(offsetText);

	const int offsetTextWidthWithoutOverflow = offsetTextStringWidth*searchTextScale + (searchTextFont->getHeight()*searchTextScale) + m_iOffsetRight;

	// calc global x offset for overflowing line (don't wrap, just move everything to the left)
	int textOverflowXOffset = 0;
	{
		const int actualXEnd = (int)(m_vPos.x + m_vSize.x - offsetTextStringWidth*searchTextScale - (searchTextFont->getHeight()*searchTextScale)*0.5f - m_iOffsetRight) + (int)(searchTextFont->getStringWidth(searchText1)*searchTextScale) + (int)(searchStringWidth*searchTextScale);
		if (actualXEnd > m_osu->getScreenWidth())
			textOverflowXOffset = actualXEnd - m_osu->getScreenWidth();
	}

	// draw background
	{
		const float lineHeight = (searchTextFont->getHeight() * searchTextScale);
		float numLines = 1.0f;
		{
			if (hasSearchSubTextVisible)
				numLines = 4.0f;
			else
				numLines = 3.0f;

			if (m_sHardcodedSearchString.length() > 0)
				numLines += 1.5f;
		}
		const float height = lineHeight * numLines;
		const int offsetTextWidthWithOverflow = offsetTextWidthWithoutOverflow + textOverflowXOffset;

		g->setColor(COLOR(m_sSearchString.length() > 0 ? 100 : 30, 0, 0, 0));
		g->fillRect(m_vPos.x + m_vSize.x - offsetTextWidthWithOverflow, m_vPos.y, offsetTextWidthWithOverflow, height);
	}

	// draw text
	g->setColor(0xffffffff);
	g->pushTransform();
	{
		g->translate(0, (int)(searchTextFont->getHeight() / 2.0f));
		g->scale(searchTextScale, searchTextScale);
		g->translate((int)(m_vPos.x + m_vSize.x - offsetTextStringWidth*searchTextScale - (searchTextFont->getHeight()*searchTextScale)*0.5f - m_iOffsetRight - textOverflowXOffset), (int)(m_vPos.y + (searchTextFont->getHeight()*searchTextScale)*1.5f));

		// draw search text and text
		g->pushTransform();
		{
			g->translate(1, 1);
			g->setColor(0xff000000);
			g->drawString(searchTextFont, searchText1);
			g->translate(-1, -1);
			g->setColor(0xff00ff00);
			g->drawString(searchTextFont, searchText1);

			if (m_sHardcodedSearchString.length() > 0)
			{
				const float searchText1Width = searchTextFont->getStringWidth(searchText1) * searchTextScale;

				g->pushTransform();
				{
					g->translate(searchText1Width, 0);

					g->translate(1, 1);
					g->setColor(0xff000000);
					g->drawString(searchTextFont, m_sHardcodedSearchString);
					g->translate(-1, -1);
					g->setColor(0xff34ab94);
					g->drawString(searchTextFont, m_sHardcodedSearchString);
				}
				g->popTransform();

				g->translate(0, searchTextFont->getHeight() * searchTextScale * 1.5f);
			}

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
			g->translate(0, (int)((searchTextFont->getHeight()*searchTextScale)*1.5f * (m_sHardcodedSearchString.length() > 0 ? 2.0f : 1.0f)));
			g->translate(1, 1);

			if (m_bSearching)
			{
				g->setColor(0xff000000);
				g->drawString(searchTextFont, searchingText2);
				g->translate(-1, -1);
				g->setColor(0xffffffff);
				g->drawString(searchTextFont, searchingText2);
			}
			else
			{
				g->setColor(0xff000000);
				g->drawString(searchTextFont, m_iNumFoundResults > -1 ? (m_iNumFoundResults > 0 ? UString::format(m_iNumFoundResults == 1 ? "%i match found!" : "%i matches found!", m_iNumFoundResults) : noMatchesFoundText1) : noMatchesFoundText2);
				g->translate(-1, -1);
				g->setColor(0xffffffff);
				g->drawString(searchTextFont, m_iNumFoundResults > -1 ? (m_iNumFoundResults > 0 ? UString::format(m_iNumFoundResults == 1 ? "%i match found!" : "%i matches found!", m_iNumFoundResults) : noMatchesFoundText1) : noMatchesFoundText2);
			}
		}
	}
	g->popTransform();
}
