//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		song browser beatmap info (top left)
//
// $NoKeywords: $osusbil
//===============================================================================//

#include "OsuUISongBrowserInfoLabel.h"

#include "Engine.h"
#include "ResourceManager.h"

#include "Osu.h"
#include "OsuBeatmap.h"
#include "OsuBeatmapDifficulty.h"

OsuUISongBrowserInfoLabel::OsuUISongBrowserInfoLabel(Osu *osu, float xPos, float yPos, float xSize, float ySize, UString name) : CBaseUIElement(xPos, yPos, xSize, ySize, name)
{
	m_osu = osu;
	m_font = m_osu->getSubTitleFont();

	m_iMargin = 10;

	float globalScaler = 1.3f;
	m_fSubTitleScale = 0.6f*globalScaler;
	m_fSongInfoScale = 0.7f*globalScaler;
	m_fDiffInfoScale = 0.65f*globalScaler;

	m_sArtist = "Artist";
	m_sTitle = "Title";
	m_sDiff = "Difficulty";
	m_sMapper = "Mapper";

	m_iLengthMS = 0;
	m_iMinBPM = 0;
	m_iMaxBPM = 0;
	m_iNumObjects = 0;

	m_fCS = 5.0f;
	m_fAR = 5.0f;
	m_fOD = 5.0f;
	m_fHP = 5.0f;
	m_fStars = 5.0f;
}

void OsuUISongBrowserInfoLabel::draw(Graphics *g)
{
	// debug bounding box
	/*
	g->setColor(0xffffffff);
	g->drawLine(m_vPos.x, m_vPos.y, m_vPos.x+m_vSize.x, m_vPos.y);
	g->drawLine(m_vPos.x, m_vPos.y, m_vPos.x, m_vPos.y+m_vSize.y);
	g->drawLine(m_vPos.x, m_vPos.y+m_vSize.y, m_vPos.x+m_vSize.x, m_vPos.y+m_vSize.y);
	g->drawLine(m_vPos.x+m_vSize.x, m_vPos.y, m_vPos.x+m_vSize.x, m_vPos.y+m_vSize.y);
	*/

	// build strings
	const UString titleText = buildTitleString();
	const UString subTitleText = buildSubTitleString();
	const UString songInfoText = buildSongInfoString();
	const UString diffInfoText = buildDiffInfoString();

	// draw
	g->setColor(0xffffffff);
	g->pushTransform();
	g->translate(m_vPos.x, m_vPos.y + m_font->getHeight());
	g->drawString(m_font, titleText);
	g->popTransform();

	float subTitleStringWidth = m_font->getStringWidth(subTitleText);
	g->setColor(0xffffffff);
	g->pushTransform();
	g->translate((int)(-subTitleStringWidth/2), (int)(m_font->getHeight()/2));
	g->scale(m_fSubTitleScale, m_fSubTitleScale);
	g->translate((int)(m_vPos.x + (subTitleStringWidth/2)*m_fSubTitleScale), (int)(m_vPos.y + m_font->getHeight() + (m_font->getHeight()/2)*m_fSubTitleScale + m_iMargin));
	g->drawString(m_font, subTitleText);
	g->popTransform();

	float songInfoStringWidth = m_font->getStringWidth(songInfoText);
	g->setColor(0xffffffff);
	if (m_osu->getSpeedMultiplier() != 1.0f)
		g->setColor(0xffff0000);
	g->pushTransform();
	g->translate((int)(-songInfoStringWidth/2), (int)(m_font->getHeight()/2));
	g->scale(m_fSongInfoScale, m_fSongInfoScale);
	g->translate((int)(m_vPos.x + (songInfoStringWidth/2)*m_fSongInfoScale), (int)(m_vPos.y + m_font->getHeight() + m_font->getHeight()*m_fSubTitleScale + (m_font->getHeight()/2)*m_fSongInfoScale + m_iMargin*2));
	g->drawString(m_font, songInfoText);
	g->popTransform();

	float diffInfoStringWidth = m_font->getStringWidth(diffInfoText);
	g->setColor(0xffffffff);
	g->pushTransform();
	g->translate((int)(-diffInfoStringWidth/2), (int)(m_font->getHeight()/2));
	g->scale(m_fDiffInfoScale, m_fDiffInfoScale);
	g->translate((int)(m_vPos.x + (diffInfoStringWidth/2)*m_fDiffInfoScale), (int)(m_vPos.y + m_font->getHeight() + m_font->getHeight()*m_fSubTitleScale + m_font->getHeight()*m_fSongInfoScale + (m_font->getHeight()/2)*m_fDiffInfoScale + m_iMargin*3));
	g->drawString(m_font, diffInfoText);
	g->popTransform();
}

void OsuUISongBrowserInfoLabel::setFromBeatmap(OsuBeatmap *beatmap, OsuBeatmapDifficulty *diff)
{
	setArtist(diff->artist);
	setTitle(diff->title);
	setDiff(diff->name);
	setMapper(diff->creator);

	setLengthMS(beatmap->getLength()); // TODO: beatmap db
	setBPM(diff->minBPM, diff->maxBPM);
	setNumObjects(diff->numObjects); // TODO: beatmap db

	setCS(diff->CS);
	setAR(diff->AR);
	setOD(diff->OD);
	setHP(diff->HP);
	setStars(diff->starsNoMod); // TODO: beatmap db
}

UString OsuUISongBrowserInfoLabel::buildTitleString()
{
	UString titleString = m_sArtist;
	titleString.append(" - ");
	titleString.append(m_sTitle);
	titleString.append(" [");
	titleString.append(m_sDiff);
	titleString.append("]");

	return titleString;
}

UString OsuUISongBrowserInfoLabel::buildSubTitleString()
{
	UString subTitleString = "Mapped by ";
	subTitleString.append(m_sMapper);

	return subTitleString;
}

UString OsuUISongBrowserInfoLabel::buildSongInfoString()
{
	const unsigned long fullSeconds = (m_iLengthMS*(1.0 / m_osu->getSpeedMultiplier())) / 1000.0;
	const int minutes = fullSeconds / 60;
	const int seconds = fullSeconds % 60;

	const int minBPM = m_iMinBPM * m_osu->getSpeedMultiplier();
	const int maxBPM = m_iMaxBPM * m_osu->getSpeedMultiplier();

	if (m_iMinBPM == m_iMaxBPM)
		return UString::format("Length: %02i:%02i BPM: %i Objects: %i", minutes, seconds, maxBPM, m_iNumObjects);
	else
		return UString::format("Length: %02i:%02i BPM: %i-%i Objects: %i", minutes, seconds, minBPM, maxBPM, m_iNumObjects);
}

UString OsuUISongBrowserInfoLabel::buildDiffInfoString()
{
	return UString::format("CS:%.2g AR:%.2g OD:%.2g HP:%.2g Stars:%.2g", m_fCS, m_fAR, m_fOD, m_fHP, m_fStars);
}

float OsuUISongBrowserInfoLabel::getMinimumWidth()
{
	/*
	float titleWidth = m_font->getStringWidth(buildTitleString());
	float subTitleWidth = m_font->getStringWidth(buildSubTitleString()) * m_fSubTitleScale;
	*/
	float titleWidth = 0;
	float subTitleWidth = 0;
	float songInfoWidth = m_font->getStringWidth(buildSongInfoString()) * m_fSongInfoScale;
	float diffInfoWidth = m_font->getStringWidth(buildDiffInfoString()) * m_fDiffInfoScale;

	return std::max(std::max(std::max(titleWidth, subTitleWidth), songInfoWidth), diffInfoWidth);
}

float OsuUISongBrowserInfoLabel::getMinimumHeight()
{
	float titleHeight = m_font->getHeight();
	float subTitleHeight = m_font->getHeight() * m_fSubTitleScale;
	float songInfoHeight = m_font->getHeight() * m_fSongInfoScale;
	float diffInfoHeight = m_font->getHeight() * m_fDiffInfoScale;

	return titleHeight + subTitleHeight + songInfoHeight + diffInfoHeight + m_iMargin*3;
}
