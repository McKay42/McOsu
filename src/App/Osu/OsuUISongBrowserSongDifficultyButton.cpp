//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		beatmap difficulty button (child of OsuUISongBrowserSongButton)
//
// $NoKeywords: $osusbsdb
//===============================================================================//

#include "OsuUISongBrowserSongDifficultyButton.h"

#include "Engine.h"
#include "ResourceManager.h"
#include "ConVar.h"

#include "Osu.h"
#include "OsuSkin.h"
#include "OsuBeatmap.h"
#include "OsuBeatmapDifficulty.h"
#include "OsuSongBrowser2.h"

OsuUISongBrowserSongDifficultyButton *OsuUISongBrowserSongDifficultyButton::previousButton = NULL;

OsuUISongBrowserSongDifficultyButton::OsuUISongBrowserSongDifficultyButton(Osu *osu, OsuSongBrowser2 *songBrowser, CBaseUIScrollView *view, float xPos, float yPos, float xSize, float ySize, UString name, OsuBeatmap *beatmap, OsuBeatmapDifficulty *diff) : OsuUISongBrowserSongButton(osu, songBrowser, view, xPos, yPos, xSize, ySize, name, NULL)
{
	m_beatmap = beatmap;
	m_diff = diff;

	previousButton = NULL; // reset

	/*
	m_sTitle = "Title";
	m_sArtist = "Artist";
	m_sMapper = "Mapper";
	m_sDiff = "Difficulty";
	*/

	m_sTitle = m_diff->title;
	m_sArtist = m_diff->artist;
	m_sMapper = m_diff->creator;
	m_sDiff = m_diff->name;

	m_fDiffScale = 0.18f;

	// settings
	setHideIfSelected(false);
	setOffsetPercent(0.075f);
	setInactiveBackgroundColor(COLOR(255, 0, 150, 236));

	updateLayout();
}

void OsuUISongBrowserSongDifficultyButton::draw(Graphics *g)
{
	OsuUISongBrowserButton::draw(g);
	if (!m_bVisible) return;

	OsuSkin *skin = m_osu->getSkin();

	// scaling
	const Vector2 pos = getActualPos();
	const Vector2 size = getActualSize();

	drawBeatmapBackgroundThumbnail(g, m_diff->backgroundImage);
	drawTitle(g, 0.2f);
	drawSubTitle(g, 0.2f);

	// draw diff name
	const float titleScale = (size.y*m_fTitleScale) / m_font->getHeight();
	const float subTitleScale = (size.y*m_fSubTitleScale) / m_font->getHeight();
	const float diffScale = (size.y*m_fDiffScale) / m_fontBold->getHeight();
	g->setColor(m_bSelected ? skin->getSongSelectActiveText() : skin->getSongSelectInactiveText());
	g->pushTransform();
		g->scale(diffScale, diffScale);
		g->translate(pos.x + m_fTextOffset, pos.y + size.y*m_fTextMarginScale + m_font->getHeight()*titleScale + size.y*m_fTextSpacingScale + m_font->getHeight()*subTitleScale*0.85f + size.y*m_fTextSpacingScale + m_fontBold->getHeight()*diffScale*0.8f);
		g->drawString(m_fontBold, buildDiffString());
	g->popTransform();

	// draw stars
	if (m_diff->starsNoMod > 0)
	{
		const float starOffsetY = (size.y*0.85);
		const float starWidth = (size.y*0.20);
		const float starScale = starWidth / skin->getStar()->getHeight();
		const int numFullStars = std::min((int)m_diff->starsNoMod, 25);
		const float partialStarScale = clamp<float>(m_diff->starsNoMod - numFullStars, 0.0f, 1.0f);

		g->setColor(m_bSelected ? skin->getSongSelectActiveText() : skin->getSongSelectInactiveText());
		for (int i=0; i<numFullStars; i++)
		{
			g->pushTransform();
				g->scale(starScale, starScale);
				g->translate(pos.x + m_fTextOffset + starWidth/2 + i*starWidth*1.5f, pos.y + starOffsetY);
				g->drawImage(skin->getStar());
			g->popTransform();
		}
		g->pushTransform();
			g->scale(starScale*partialStarScale, starScale*partialStarScale);
			g->translate(pos.x + m_fTextOffset + starWidth/2 + numFullStars*starWidth*1.5f, pos.y + starOffsetY);
			g->drawImage(skin->getStar());
		g->popTransform();
	}
}

void OsuUISongBrowserSongDifficultyButton::onSelected(bool wasSelected)
{
	if (!wasSelected)
		m_beatmap->selectDifficulty(m_diff, false);
	m_songBrowser->onDifficultySelected(m_beatmap, m_diff, wasSelected);
	m_songBrowser->scrollToSongButton(this);

	// automatically deselect previous selection
	if (previousButton != NULL && previousButton != this)
		previousButton->deselect();
	previousButton = this;
}

void OsuUISongBrowserSongDifficultyButton::onDeselected()
{
	// since unselected elements will NOT get a setVisible(false) event by the scrollview, we have to manually hide them if unselected
	setVisible(false); // this also unloads the thumbnail
}
