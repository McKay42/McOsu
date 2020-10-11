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
#include "Mouse.h"

#include "Osu.h"
#include "OsuSkin.h"
#include "OsuSkinImage.h"
#include "OsuDatabaseBeatmap.h"
#include "OsuSongBrowser2.h"
#include "OsuBackgroundImageHandler.h"

#include "OsuUISongBrowserSongDifficultyButton.h"
#include "OsuUISongBrowserScoreButton.h"
#include "OsuUIContextMenu.h"

ConVar osu_draw_songbrowser_thumbnails("osu_draw_songbrowser_thumbnails", true);
ConVar osu_songbrowser_thumbnail_delay("osu_songbrowser_thumbnail_delay", 0.1f);
ConVar osu_songbrowser_thumbnail_fade_in_duration("osu_songbrowser_thumbnail_fade_in_duration", 0.1f);

float OsuUISongBrowserSongButton::thumbnailYRatio = 1.333333f;

OsuUISongBrowserSongButton::OsuUISongBrowserSongButton(Osu *osu, OsuSongBrowser2 *songBrowser, CBaseUIScrollView *view, OsuUIContextMenu *contextMenu, float xPos, float yPos, float xSize, float ySize, UString name, OsuDatabaseBeatmap *databaseBeatmap) : OsuUISongBrowserButton(osu, songBrowser, view, contextMenu, xPos, yPos, xSize, ySize, name)
{
	m_databaseBeatmap = databaseBeatmap;

	m_grade = OsuScore::GRADE::GRADE_D;
	m_bHasGrade = false;

	// settings
	setHideIfSelected(true);

	// labels
	/*
	m_sTitle = "Title";
	m_sArtist = "Artist";
	m_sMapper = "Mapper";
	*/

	m_fThumbnailFadeInTime = 0.0f;
	m_fTextOffset = 0.0f;
	m_fGradeOffset = 0.0f;
	m_fTextSpacingScale = 0.075f;
	m_fTextMarginScale = 0.075f;
	m_fTitleScale = 0.22f;
	m_fSubTitleScale = 0.14f;
	m_fGradeScale = 0.45f;

	// build children
	if (m_databaseBeatmap != NULL)
	{
		const std::vector<OsuDatabaseBeatmap*> &difficulties = m_databaseBeatmap->getDifficulties();

		// and add them
		for (int i=0; i<difficulties.size(); i++)
		{
			OsuUISongBrowserSongButton *songButton = new OsuUISongBrowserSongDifficultyButton(m_osu, m_songBrowser, m_view, m_contextMenu, 0, 0, 0, 0, "", difficulties[i], this);

			m_children.push_back(songButton);
		}
	}

	// select representative default diff for parent (beatmap button)
	if (m_databaseBeatmap != NULL && m_children.size() > 0)
	{
		m_representativeDatabaseBeatmap = (dynamic_cast<OsuUISongBrowserSongButton*>(m_children[m_children.size()-1]))->getDatabaseBeatmap();

		m_sTitle = m_representativeDatabaseBeatmap->getTitle();
		m_sArtist = m_representativeDatabaseBeatmap->getArtist();
		m_sMapper = m_representativeDatabaseBeatmap->getCreator();
	}

	updateLayoutEx();
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

	// draw background image
	if (m_representativeDatabaseBeatmap != NULL)
		drawBeatmapBackgroundThumbnail(g, m_osu->getBackgroundImageHandler()->getLoadBackgroundImage(m_representativeDatabaseBeatmap));

	drawTitle(g);
	drawSubTitle(g);
}

void OsuUISongBrowserSongButton::drawBeatmapBackgroundThumbnail(Graphics *g, Image *image)
{
	if (!osu_draw_songbrowser_thumbnails.getBool() || m_osu->getSkin()->getVersion() < 2.2f) return;

	float alpha = 1.0f;
	if (osu_songbrowser_thumbnail_fade_in_duration.getFloat() > 0.0f)
	{
		if (image == NULL || !image->isReady())
			m_fThumbnailFadeInTime = engine->getTime();
		else if (m_fThumbnailFadeInTime > 0.0f && engine->getTime() > m_fThumbnailFadeInTime)
		{
			alpha = clamp<float>((engine->getTime() - m_fThumbnailFadeInTime)/osu_songbrowser_thumbnail_fade_in_duration.getFloat(), 0.0f, 1.0f);
			alpha = 1.0f - (1.0f - alpha)*(1.0f - alpha);
		}
	}

	if (image == NULL || !image->isReady()) return;

	// scaling
	const Vector2 pos = getActualPos();
	const Vector2 size = getActualSize();

	const float beatmapBackgroundScale = Osu::getImageScaleToFillResolution(image, Vector2(size.y*thumbnailYRatio, size.y))*1.05f;

	Vector2 centerOffset = Vector2(std::round((size.y*thumbnailYRatio)/2.0f), std::round(size.y/2.0f));
	McRect clipRect = McRect(pos.x-2, pos.y+1, (int)(size.y*thumbnailYRatio)+5, size.y+2);

	g->setColor(0xffffffff);
	g->setAlpha(alpha);
	g->pushTransform();
	{
		g->scale(beatmapBackgroundScale, beatmapBackgroundScale);
		g->translate(pos.x + (int)centerOffset.x, pos.y + (int)centerOffset.y);
		g->pushClipRect(clipRect);
		{
			g->drawImage(image);
		}
		g->popClipRect();
	}
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

void OsuUISongBrowserSongButton::drawGrade(Graphics *g)
{
	// scaling
	const Vector2 pos = getActualPos();
	const Vector2 size = getActualSize();

	OsuSkinImage *grade = OsuUISongBrowserScoreButton::getGradeImage(m_osu, m_grade);
	g->pushTransform();
	{
		const float scale = calculateGradeScale();

		g->setColor(0xffffffff);
		grade->drawRaw(g, Vector2(pos.x + m_fGradeOffset + grade->getSizeBaseRaw().x*scale/2, pos.y + size.y/2), scale);
	}
	g->popTransform();
}

void OsuUISongBrowserSongButton::drawTitle(Graphics *g, float deselectedAlpha, bool forceSelectedStyle)
{
	// scaling
	const Vector2 pos = getActualPos();
	const Vector2 size = getActualSize();

	const float titleScale = (size.y*m_fTitleScale) / m_font->getHeight();
	g->setColor((m_bSelected || forceSelectedStyle) ? m_osu->getSkin()->getSongSelectActiveText() : m_osu->getSkin()->getSongSelectInactiveText());
	if (!(m_bSelected || forceSelectedStyle))
		g->setAlpha(deselectedAlpha);

	g->pushTransform();
	{
		g->scale(titleScale, titleScale);
		g->translate(pos.x + m_fTextOffset, pos.y + size.y*m_fTextMarginScale + m_font->getHeight()*titleScale);
		g->drawString(m_font, buildTitleString());

		// debugging
		//g->drawString(m_font, UString::format("%i, %i", m_diff->setID, m_diff->ID));
	}
	g->popTransform();
}

void OsuUISongBrowserSongButton::drawSubTitle(Graphics *g, float deselectedAlpha, bool forceSelectedStyle)
{
	// scaling
	const Vector2 pos = getActualPos();
	const Vector2 size = getActualSize();

	const float titleScale = (size.y*m_fTitleScale) / m_font->getHeight();
	const float subTitleScale = (size.y*m_fSubTitleScale) / m_font->getHeight();
	g->setColor((m_bSelected || forceSelectedStyle) ? m_osu->getSkin()->getSongSelectActiveText() : m_osu->getSkin()->getSongSelectInactiveText());
	if (!(m_bSelected || forceSelectedStyle))
		g->setAlpha(deselectedAlpha);

	g->pushTransform();
	{
		g->scale(subTitleScale, subTitleScale);
		g->translate(pos.x + m_fTextOffset, pos.y + size.y*m_fTextMarginScale + m_font->getHeight()*titleScale + size.y*m_fTextSpacingScale + m_font->getHeight()*subTitleScale*0.85f);
		g->drawString(m_font, buildSubTitleString());

		// debug stuff
		/*
		g->translate(-300, 0);
		long long oldestTime = std::numeric_limits<long long>::min();
		for (int i=0; i<m_beatmap->getNumDifficulties(); i++)
		{
			if ((*m_beatmap->getDifficultiesPointer())[i]->lastModificationTime > oldestTime)
				oldestTime = (*m_beatmap->getDifficultiesPointer())[i]->lastModificationTime;
		}
		g->drawString(m_font, UString::format("t = %I64d", oldestTime));
		*/
	}
	g->popTransform();
}

void OsuUISongBrowserSongButton::sortChildren()
{
	struct SortComparator
	{
		bool operator() (OsuUISongBrowserButton const *aDiff, OsuUISongBrowserButton const *bDiff) const
		{
			const OsuDatabaseBeatmap *a = ((OsuUISongBrowserSongDifficultyButton*)aDiff)->getDatabaseBeatmap();
			const OsuDatabaseBeatmap *b = ((OsuUISongBrowserSongDifficultyButton*)bDiff)->getDatabaseBeatmap();

			const unsigned long diff1 = (a->getAR()+1)*(a->getCS()+1)*(a->getHP()+1)*(a->getOD()+1)*(a->getMaxBPM() > 0 ? a->getMaxBPM() : 1);
			const unsigned long diff2 = (b->getAR()+1)*(b->getCS()+1)*(b->getHP()+1)*(b->getOD()+1)*(b->getMaxBPM() > 0 ? b->getMaxBPM() : 1);

			const float stars1 = a->getStarsNomod();
			const float stars2 = b->getStarsNomod();

			if (stars1 > 0 && stars2 > 0)
			{
				// strict weak ordering!
				if (stars1 == stars2)
					return a->getSortHack() < b->getSortHack();
				else
					return stars1 < stars2;
			}
			else
			{
				// strict weak ordering!
				if (diff1 == diff2)
					return a->getSortHack() < b->getSortHack();
				else
					return diff1 < diff2;
			}
		}
	};
	std::sort(m_children.begin(), m_children.end(), SortComparator());
}

void OsuUISongBrowserSongButton::updateLayoutEx()
{
	OsuUISongBrowserButton::updateLayoutEx();

	// scaling
	const Vector2 size = getActualSize();

	m_fTextOffset = 0.0f;
	m_fGradeOffset = 0.0f;

	if (m_bHasGrade)
		m_fTextOffset += calculateGradeWidth();

	if (m_osu->getSkin()->getVersion() < 2.2f)
	{
		m_fTextOffset += size.x*0.02f*2.0f;
		if (m_bHasGrade)
			m_fGradeOffset += calculateGradeWidth()/2;
	}
	else
	{
		m_fTextOffset += size.y*thumbnailYRatio + size.x*0.02f;
		m_fGradeOffset += size.y*thumbnailYRatio + size.x*0.0125f;
	}
}

void OsuUISongBrowserSongButton::onSelected(bool wasSelected)
{
	OsuUISongBrowserButton::onSelected(wasSelected);

	// resort children (since they might have been updated in the meantime)
	sortChildren();

	// update grade on child
	for (int c=0; c<m_children.size(); c++)
	{
		OsuUISongBrowserSongDifficultyButton *child = (OsuUISongBrowserSongDifficultyButton*)m_children[c];
		child->updateGrade();
	}

	m_songBrowser->onSelectionChange(this, false);

	// now, automatically select the bottom child (hardest diff, assuming default sorting)
	if (m_children.size() > 0)
		m_children[m_children.size()-1]->select();
}

void OsuUISongBrowserSongButton::onRightMouseUpInside()
{
	const Vector2 pos = engine->getMouse()->getPos();

	if (m_contextMenu != NULL)
	{
		m_contextMenu->setPos(pos);
		m_contextMenu->setRelPos(pos);
		m_contextMenu->begin(0, true);
		{
			m_contextMenu->addButton("[+]   Add to Collection");
			m_contextMenu->addButton("[+Set]   Add to Collection");
			m_contextMenu->addButton("[-]   Remove from Collection");
			m_contextMenu->addButton("[-Set]   Remove from Collection");
		}
		m_contextMenu->end();
		m_contextMenu->setClickCallback( fastdelegate::MakeDelegate(this, &OsuUISongBrowserSongButton::onContextMenu) );

		// clamp to right screen edge
		if (m_contextMenu->getRelPos().x + m_contextMenu->getSize().x > m_osu->getScreenWidth())
		{
			const int newRelPosX = m_osu->getScreenWidth() - m_contextMenu->getSize().x - 1;
			m_contextMenu->setRelPosX(newRelPosX);
			m_contextMenu->setPosX(newRelPosX);
		}

		// clamp to bottom screen edge
		if (m_contextMenu->getRelPos().y + m_contextMenu->getSize().y > m_osu->getScreenHeight())
		{
			const int newRelPosY = m_osu->getScreenHeight() - m_contextMenu->getSize().y - 1;
			m_contextMenu->setRelPosY(newRelPosY);
			m_contextMenu->setPosY(newRelPosY);
		}
	}
}

void OsuUISongBrowserSongButton::onContextMenu(UString text, int id)
{
	// TODO: implement
}

float OsuUISongBrowserSongButton::calculateGradeScale()
{
	const Vector2 size = getActualSize();

	OsuSkinImage *grade = OsuUISongBrowserScoreButton::getGradeImage(m_osu, m_grade);

	return Osu::getImageScaleToFitResolution(grade->getSizeBaseRaw(), Vector2(size.x, size.y*m_fGradeScale));
}

float OsuUISongBrowserSongButton::calculateGradeWidth()
{
	OsuSkinImage *grade = OsuUISongBrowserScoreButton::getGradeImage(m_osu, m_grade);

	return grade->getSizeBaseRaw().x * calculateGradeScale();
}
