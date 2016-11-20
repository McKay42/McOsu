//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		beatmap + diff button
//
// $NoKeywords: $osusbsb
//===============================================================================//

#include "OsuUISongBrowserSongButton.h"

#include "Engine.h"
#include "ResourceManager.h"
#include "ConVar.h"

#include "Osu.h"
#include "OsuSkin.h"
#include "OsuBeatmap.h"
#include "OsuBeatmapDifficulty.h"
#include "OsuSongBrowser2.h"

#include "OsuUISongBrowserSongDifficultyButton.h"

ConVar osu_songbrowser_thumbnail_delay("osu_songbrowser_thumbnail_delay", 0.1f);

float OsuUISongBrowserSongButton::thumbnailYRatio = 1.333333f;
OsuUISongBrowserSongButton *OsuUISongBrowserSongButton::previousButton = NULL;

OsuUISongBrowserSongButton::OsuUISongBrowserSongButton(Osu *osu, OsuSongBrowser2 *songBrowser, CBaseUIScrollView *view, float xPos, float yPos, float xSize, float ySize, UString name, OsuBeatmap *beatmap) : OsuUISongBrowserButton(osu, songBrowser, view, xPos, yPos, xSize, ySize, name)
{
	m_beatmap = beatmap;
	m_diff = NULL;
	m_parent = NULL;

	previousButton = NULL; // reset

	// settings
	setHideIfSelected(true);

	// labels
	/*
	m_sTitle = "Title";
	m_sArtist = "Artist";
	m_sMapper = "Mapper";
	*/

	m_fImageLoadScheduledTime = 0.0f;
	m_fTextOffset = 0.0f;
	m_fTextSpacingScale = 0.075f;
	m_fTextMarginScale = 0.075f;
	m_fTitleScale = 0.22f;
	m_fSubTitleScale = 0.14f;

	// build children
	if (m_beatmap != NULL)
	{
		// sort difficulties by difficulty
		std::vector<OsuBeatmapDifficulty*> difficulties = m_beatmap->getDifficulties();
		struct SortComparator
		{
			bool operator() (OsuBeatmapDifficulty const *a, OsuBeatmapDifficulty const *b) const
			{
				const unsigned long diff1 = (a->AR+1)*(a->CS+1)*(a->HP+1)*(a->OD+1)*(a->maxBPM > 0 ? a->maxBPM : 1);
				const unsigned long diff2 = (b->AR+1)*(b->CS+1)*(b->HP+1)*(b->OD+1)*(b->maxBPM > 0 ? b->maxBPM : 1);

				const float stars1 = a->starsNoMod;
				const float stars2 = b->starsNoMod;

				if (stars1 > 0 && stars2 > 0)
					return stars1 < stars2;
				else
					return diff1 < diff2;
			}
		};
		std::sort(difficulties.begin(), difficulties.end(), SortComparator());

		// and add them
		for (int i=0; i<difficulties.size(); i++)
		{
			OsuUISongBrowserSongButton *songButton = new OsuUISongBrowserSongDifficultyButton(m_osu, m_songBrowser, m_view, 0, 0, 0, 0, "", m_beatmap, difficulties[i]);
			songButton->setParent(this);
			m_children.push_back(songButton);
		}
	}

	// select representative default diff for parent (beatmap button)
	if (m_beatmap != NULL && m_children.size() > 0)
	{
		OsuBeatmapDifficulty *defaultDiff = (dynamic_cast<OsuUISongBrowserSongButton*>(m_children[m_children.size()-1]))->getDiff();

		m_diff = defaultDiff;

		m_sTitle = defaultDiff->title;
		m_sArtist = defaultDiff->artist;
		m_sMapper = defaultDiff->creator;
	}

	updateLayout();
}

OsuUISongBrowserSongButton::~OsuUISongBrowserSongButton()
{
	for (int i=0; i<m_children.size(); i++)
	{
		delete m_children[i];
	}
}

void OsuUISongBrowserSongButton::draw(Graphics *g)
{
	OsuUISongBrowserButton::draw(g);
	if (!m_bVisible) return;

	if (m_diff != NULL)
		drawBeatmapBackgroundThumbnail(g, m_diff->backgroundImage);

	drawTitle(g);
	drawSubTitle(g);
}

void OsuUISongBrowserSongButton::drawBeatmapBackgroundThumbnail(Graphics *g, Image *image)
{
	if (image == NULL) return;

	// scaling
	const Vector2 pos = getActualPos();
	const Vector2 size = getActualSize();

	const float beatmapBackgroundScale = Osu::getImageScaleToFillResolution(image, Vector2(size.y*thumbnailYRatio, size.y))*1.05f;

	Vector2 centerOffset = Vector2(std::round((size.y*thumbnailYRatio)/2.0f), std::round(size.y/2.0f));
	Rect clipRect = Rect(pos.x-2, pos.y+1, (int)(size.y*thumbnailYRatio)+5, size.y+2);

	g->setColor(0xffffffff);
	g->pushTransform();
		g->scale(beatmapBackgroundScale, beatmapBackgroundScale);
		g->translate(pos.x + (int)centerOffset.x, pos.y + (int)centerOffset.y);
		g->pushClipRect(clipRect);
			g->drawImage(image);
		g->popClipRect();
	g->popTransform();

	// debug cliprect bounding box
	if (Osu::debug->getBool())
	{
		Vector2 clipRectPos = Vector2(clipRect.getX(), clipRect.getY()-1);
		Vector2 clipRectSize = Vector2(clipRect.getWidth(), clipRect.getHeight());

		g->setColor(0xffffff00);
		g->drawLine(clipRectPos.x, clipRectPos.y, clipRectPos.x+clipRectSize.x, clipRectPos.y);
		g->drawLine(clipRectPos.x, clipRectPos.y, clipRectPos.x, clipRectPos.y+clipRectSize.y);
		g->drawLine(clipRectPos.x, clipRectPos.y+clipRectSize.y, clipRectPos.x+clipRectSize.x, clipRectPos.y+clipRectSize.y);
		g->drawLine(clipRectPos.x+clipRectSize.x, clipRectPos.y, clipRectPos.x+clipRectSize.x, clipRectPos.y+clipRectSize.y);
	}
}

void OsuUISongBrowserSongButton::drawTitle(Graphics *g, float deselectedAlpha)
{
	// scaling
	const Vector2 pos = getActualPos();
	const Vector2 size = getActualSize();

	const float titleScale = (size.y*m_fTitleScale) / m_font->getHeight();
	g->setColor(m_bSelected ? m_osu->getSkin()->getSongSelectActiveText() : m_osu->getSkin()->getSongSelectInactiveText());
	if (!m_bSelected)
		g->setAlpha(deselectedAlpha);
	g->pushTransform();
		g->scale(titleScale, titleScale);
		g->translate(pos.x + m_fTextOffset, pos.y + size.y*m_fTextMarginScale + m_font->getHeight()*titleScale);
		g->drawString(m_font, buildTitleString());
	g->popTransform();
}

void OsuUISongBrowserSongButton::drawSubTitle(Graphics *g, float deselectedAlpha)
{
	// scaling
	const Vector2 pos = getActualPos();
	const Vector2 size = getActualSize();

	const float titleScale = (size.y*m_fTitleScale) / m_font->getHeight();
	const float subTitleScale = (size.y*m_fSubTitleScale) / m_font->getHeight();
	g->setColor(m_bSelected ? m_osu->getSkin()->getSongSelectActiveText() : m_osu->getSkin()->getSongSelectInactiveText());
	if (!m_bSelected)
		g->setAlpha(deselectedAlpha);
	g->pushTransform();
		g->scale(subTitleScale, subTitleScale);
		g->translate(pos.x + m_fTextOffset, pos.y + size.y*m_fTextMarginScale + m_font->getHeight()*titleScale + size.y*m_fTextSpacingScale + m_font->getHeight()*subTitleScale*0.85f);
		g->drawString(m_font, buildSubTitleString());
	g->popTransform();
}

void OsuUISongBrowserSongButton::update()
{
	OsuUISongBrowserButton::update();
	if (!m_bVisible) return;

	// delayed image loading
	if (m_fImageLoadScheduledTime != 0.0f && engine->getTime() > m_fImageLoadScheduledTime)
	{
		m_fImageLoadScheduledTime = 0.0f;
		if (isVisible() && m_diff != NULL) // only load if we are still visible
			m_diff->loadBackgroundImage();
	}
}

void OsuUISongBrowserSongButton::updateLayout()
{
	OsuUISongBrowserButton::updateLayout();

	// scaling
	const Vector2 size = getActualSize();

	m_fTextOffset = size.y*thumbnailYRatio + size.x*0.02f;
}

void OsuUISongBrowserSongButton::setVisible(bool visible)
{
	OsuUISongBrowserButton::setVisible(visible);

	// this is called in all cases (outside viewing volume of scrollView, and if not visible because replaced by children)
	// and also if the selection changes (due to previousParent and previousChild)
	checkLoadUnloadImage();
}

std::vector<OsuUISongBrowserButton*> OsuUISongBrowserSongButton::getChildren()
{
	if (m_bSelected)
		return m_children;
	return std::vector<OsuUISongBrowserButton*>();
}

void OsuUISongBrowserSongButton::onSelected(bool wasSelected)
{
	if (wasSelected)
	{
		deselect();
		return;
	}

	// automatically deselect previous selection if another beatmap is selected
	if (previousButton != NULL && previousButton != this)
		previousButton->deselect();
	else
		m_songBrowser->rebuildSongButtons(false); // this is in the else-branch to avoid double execution (since it is already executed in deselect())
	previousButton = this;

	// now, automatically select the bottom child
	if (m_children.size() > 0)
		m_children[m_children.size()-1]->select();
}

void OsuUISongBrowserSongButton::onDeselected()
{
	for (int i=0; i<m_children.size(); i++)
	{
		m_children[i]->deselect();
	}
	m_beatmap->deselect(false);
	m_songBrowser->rebuildSongButtons(false);

	// after the songbrowser has now rebuilt the layout, check if we are still visible or could potentially unload
	checkLoadUnloadImage();
}

void OsuUISongBrowserSongButton::checkLoadUnloadImage()
{
	// dynamically load/unload thumbnails
	if (m_bVisible)
	{
		// visibility delay, don't load immediately but only if we are visible for a minimum amount of time (to avoid useless loads)
		m_fImageLoadScheduledTime = engine->getTime() + osu_songbrowser_thumbnail_delay.getFloat();

		// the actual loading is happening in OsuUISongBrowserSongButton::update()
	}
	else
	{
		// block/stop all potentially scheduled loads
		m_fImageLoadScheduledTime = 0.0f;

		// in pure OsuUISongBrowserSongButtons, m_diff will be the default diff which represents this beatmap (selected in the constructor)
		// in OsuUISongBrowserSongDifficultyButtons, m_diff will be the diff this button is responsible for

		if (m_diff != NULL)
		{
			// only allow unloading if we are not selected and no children of us are selected
			bool areChildrenSelected = false;
			for (int i=0; i<m_children.size(); i++)
			{
				if (m_children[i]->isSelected())
				{
					areChildrenSelected = true;
					break;
				}
			}

			bool isChild = m_parent != NULL;
			bool isChildBlockingUnload = false;
			if (isChild)
			{
				// only allow unloading if we are not the representative child of the parent, and if our parent is not selected
				// this also forces the thumbnails of the currently opened beatmap (all diffs) to not be unloaded (even if some diffs become invisible due to scrolling)
				// TODO: if a beatmap has a shitload of diffs, this will keep all images loaded and could potentially crash on GPUs with very little VRAM
				if (m_diff == m_parent->getDiff() || m_parent->isSelected())
					isChildBlockingUnload = true;
			}

			if (!isSelected() && !areChildrenSelected && !isChildBlockingUnload)
				m_diff->unloadBackgroundImage();
		}
	}
}
