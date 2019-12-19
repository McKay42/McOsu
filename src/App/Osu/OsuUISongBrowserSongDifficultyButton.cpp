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
#include "OsuDatabase.h"
#include "OsuReplay.h"

#include "OsuUISongBrowserScoreButton.h"

#include "OpenGLHeaders.h"
#include "OpenGLLegacyInterface.h"
#include "OpenGL3Interface.h"
#include "OpenGLES2Interface.h"

ConVar *OsuUISongBrowserSongDifficultyButton::m_osu_scores_enabled = NULL;
OsuUISongBrowserSongDifficultyButton *OsuUISongBrowserSongDifficultyButton::previousButton = NULL;

OsuUISongBrowserSongDifficultyButton::OsuUISongBrowserSongDifficultyButton(Osu *osu, OsuSongBrowser2 *songBrowser, CBaseUIScrollView *view, float xPos, float yPos, float xSize, float ySize, UString name, OsuBeatmap *beatmap, OsuBeatmapDifficulty *diff) : OsuUISongBrowserSongButton(osu, songBrowser, view, xPos, yPos, xSize, ySize, name, NULL)
{
	m_beatmap = beatmap;
	m_diff = diff;

	if (m_osu_scores_enabled == NULL)
		m_osu_scores_enabled = convar->getConVarByName("osu_scores_enabled");

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

	updateLayoutEx();
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

	if (m_bHasGrade)
		drawGrade(g);

	drawTitle(g, 0.2f);
	drawSubTitle(g, 0.2f);

	// draw diff name
	const float titleScale = (size.y*m_fTitleScale) / m_font->getHeight();
	const float subTitleScale = (size.y*m_fSubTitleScale) / m_font->getHeight();
	const float diffScale = (size.y*m_fDiffScale) / m_fontBold->getHeight();
	g->setColor(m_bSelected ? skin->getSongSelectActiveText() : skin->getSongSelectInactiveText());
	g->pushTransform();
	{
		g->scale(diffScale, diffScale);
		g->translate(pos.x + m_fTextOffset, pos.y + size.y*m_fTextMarginScale + m_font->getHeight()*titleScale + size.y*m_fTextSpacingScale + m_font->getHeight()*subTitleScale*0.85f + size.y*m_fTextSpacingScale + m_fontBold->getHeight()*diffScale*0.8f);
		g->drawString(m_fontBold, buildDiffString());
	}
	g->popTransform();

	// draw stars
	const float starsNoMod = m_diff->starsNoMod;
	if (starsNoMod > 0)
	{
		const float starOffsetY = (size.y*0.85);
		const float starWidth = (size.y*0.2);
		const float starScale = starWidth / skin->getStar()->getHeight();
		const int numFullStars = std::min((int)starsNoMod, 25);
		const float partialStarScale = std::max(0.5f, clamp<float>(starsNoMod - numFullStars, 0.0f, 1.0f)); // at least 0.5x

		g->setColor(m_bSelected ? skin->getSongSelectActiveText() : skin->getSongSelectInactiveText());

		// full stars
		for (int i=0; i<numFullStars; i++)
		{
			const float scale = std::min(starScale*1.175f, starScale + i*0.015f); // more stars = getting bigger, up to a limit

			g->pushTransform();
			{
				g->scale(scale, scale);
				g->translate(pos.x + m_fTextOffset + starWidth/2 + i*starWidth*1.75f, pos.y + starOffsetY);
				g->drawImage(skin->getStar());
			}
			g->popTransform();
		}

		// partial star
		g->pushTransform();
		{
			g->scale(starScale*partialStarScale, starScale*partialStarScale);
			g->translate(pos.x + m_fTextOffset + starWidth/2 + numFullStars*starWidth*1.75f, pos.y + starOffsetY);
			g->drawImage(skin->getStar());
		}
		g->popTransform();

		// fill leftover space up to 10 stars total (background stars)
		g->setColor(0x1effffff);
		const float backgroundStarScale = 0.6f;

#if defined(MCENGINE_FEATURE_OPENGL)

			const bool isOpenGLRendererHack = (dynamic_cast<OpenGLLegacyInterface*>(g) != NULL || dynamic_cast<OpenGL3Interface*>(g) != NULL);

#elif defined(MCENGINE_FEATURE_OPENGLES)

			const bool isOpenGLRendererHack = (dynamic_cast<OpenGLES2Interface*>(g) != NULL);

#endif

#if defined(MCENGINE_FEATURE_OPENGL) || defined(MCENGINE_FEATURE_OPENGLES)

		if (isOpenGLRendererHack)
			glBlendFunc(GL_SRC_ALPHA, GL_ONE); // HACKHACK: OpenGL hardcoded

#endif

		for (int i=(numFullStars + 1); i<10; i++)
		{
			g->pushTransform();
			{
				g->scale(starScale*backgroundStarScale, starScale*backgroundStarScale);
				g->translate(pos.x + m_fTextOffset + starWidth/2 + i*starWidth*1.75f, pos.y + starOffsetY);
				g->drawImage(skin->getStar());
			}
			g->popTransform();
		}

#if defined(MCENGINE_FEATURE_OPENGL) || defined(MCENGINE_FEATURE_OPENGLES)

		if (isOpenGLRendererHack)
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // HACKHACK: OpenGL hardcoded

#endif

	}
}

void OsuUISongBrowserSongDifficultyButton::onSelected(bool wasSelected)
{
	if (!wasSelected)
		m_beatmap->selectDifficulty(m_diff, false);

	m_songBrowser->onDifficultySelected(m_beatmap, m_diff, wasSelected);
	m_songBrowser->scrollToSongButton(this);

	// automatically deselect previous selection
	if (m_osu->getInstanceID() < 2) // TODO: this leaks memory on slaves
	{
		if (previousButton != NULL && previousButton != this)
			previousButton->deselect();

		previousButton = this;
	}
}

void OsuUISongBrowserSongDifficultyButton::onDeselected()
{
	// since unselected elements will NOT get a setVisible(false) event by the scrollview, we have to manually hide them if unselected
	setVisible(false); // this also unloads the thumbnail
}

void OsuUISongBrowserSongDifficultyButton::updateGrade()
{
	if (!m_osu_scores_enabled->getBool())
	{
		m_bHasGrade = false;
		return;
	}

	bool hasGrade = false;
	OsuScore::GRADE grade;

	// old
	/*
	unsigned long long highestScore = 0;
	for (int i=0; i<(*m_osu->getSongBrowser()->getDatabase()->getScores())[m_diff->md5hash].size(); i++)
	{
		const unsigned long long score = (*m_osu->getSongBrowser()->getDatabase()->getScores())[m_diff->md5hash][i].score;
		const int num300s = (*m_osu->getSongBrowser()->getDatabase()->getScores())[m_diff->md5hash][i].num300s;
		const int num100s = (*m_osu->getSongBrowser()->getDatabase()->getScores())[m_diff->md5hash][i].num100s;
		const int num50s = (*m_osu->getSongBrowser()->getDatabase()->getScores())[m_diff->md5hash][i].num50s;
		const int numMisses = (*m_osu->getSongBrowser()->getDatabase()->getScores())[m_diff->md5hash][i].numMisses;
		const bool modHidden = (*m_osu->getSongBrowser()->getDatabase()->getScores())[m_diff->md5hash][i].modsLegacy & OsuReplay::Mods::Hidden;
		const bool modFlashlight = (*m_osu->getSongBrowser()->getDatabase()->getScores())[m_diff->md5hash][i].modsLegacy & OsuReplay::Mods::Flashlight;

		if (score > highestScore)
		{
			highestScore = score;
			hasGrade = true;
			grade = OsuScore::calculateGrade(num300s, num100s, num50s, numMisses, modHidden, modFlashlight);
		}
	}
	*/

	// new
	m_osu->getSongBrowser()->getDatabase()->sortScores(m_diff->md5hash);
	if ((*m_osu->getSongBrowser()->getDatabase()->getScores())[m_diff->md5hash].size() > 0)
	{
		const OsuDatabase::Score &score = (*m_osu->getSongBrowser()->getDatabase()->getScores())[m_diff->md5hash][0];
		hasGrade = true;
		grade = OsuScore::calculateGrade(score.num300s, score.num100s, score.num50s, score.numMisses, score.modsLegacy & OsuReplay::Mods::Hidden, score.modsLegacy & OsuReplay::Mods::Flashlight);
	}

	m_bHasGrade = hasGrade;
	if (m_bHasGrade)
		m_grade = grade;
}
