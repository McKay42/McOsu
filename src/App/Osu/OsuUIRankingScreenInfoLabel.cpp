//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		analog to OsuUIRankingScreenInfoLabel, but for the ranking screen
//
// $NoKeywords: $osursil
//===============================================================================//

#include "OsuUIRankingScreenInfoLabel.h"

#include "Engine.h"
#include "ResourceManager.h"

#include "Osu.h"
#include "OsuBeatmap.h"
#include "OsuBeatmapDifficulty.h"

#include <chrono>

OsuUIRankingScreenInfoLabel::OsuUIRankingScreenInfoLabel(Osu *osu, float xPos, float yPos, float xSize, float ySize, UString name) : CBaseUIElement(xPos, yPos, xSize, ySize, name)
{
	m_osu = osu;
	m_font = m_osu->getSubTitleFont();

	m_iMargin = 10;

	float globalScaler = 1.3f;
	m_fSubTitleScale = 0.6f*globalScaler;

	m_sArtist = "Artist";
	m_sTitle = "Title";
	m_sDiff = "Difficulty";
	m_sMapper = "Mapper";
	m_sPlayer = "Guest";
	m_sDate = "?";
}

void OsuUIRankingScreenInfoLabel::draw(Graphics *g)
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
	const UString playerText = buildPlayerString();

	const float globalScale = std::max((m_vSize.y / getMinimumHeight())*0.741f, 1.0f);

	// draw title
	g->setColor(0xffffffff);
	g->pushTransform();
	{
		const float scale = globalScale;

		g->scale(scale, scale);
		g->translate(m_vPos.x, m_vPos.y + m_font->getHeight()*scale);
		g->drawString(m_font, titleText);
	}
	g->popTransform();

	// draw subtitle
	g->setColor(0xffffffff);
	g->pushTransform();
	{
		const float scale = m_fSubTitleScale*globalScale;

		const float subTitleStringWidth = m_font->getStringWidth(subTitleText);

		g->translate((int)(-subTitleStringWidth/2), (int)(m_font->getHeight()/2));
		g->scale(scale, scale);
		g->translate((int)(m_vPos.x + (subTitleStringWidth/2)*scale), (int)(m_vPos.y + m_font->getHeight()*globalScale + (m_font->getHeight()/2)*scale + m_iMargin));
		g->drawString(m_font, subTitleText);
	}
	g->popTransform();

	// draw subsubtitle (player text + datetime)
	g->setColor(0xffffffff);
	g->pushTransform();
	{
		const float scale = m_fSubTitleScale*globalScale;

		const float playerStringWidth = m_font->getStringWidth(playerText);

		g->translate((int)(-playerStringWidth/2), (int)(m_font->getHeight()/2));
		g->scale(scale, scale);
		g->translate((int)(m_vPos.x + (playerStringWidth/2)*scale), (int)(m_vPos.y + m_font->getHeight()*globalScale + m_font->getHeight()*scale + (m_font->getHeight()/2)*scale + m_iMargin*2));
		g->drawString(m_font, playerText);
	}
	g->popTransform();
}

void OsuUIRankingScreenInfoLabel::setFromBeatmap(OsuBeatmap *beatmap, OsuBeatmapDifficulty *diff)
{
	setArtist(diff->artist);
	setTitle(diff->title);
	setDiff(diff->name);
	setMapper(diff->creator);

	std::time_t now_c = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	m_sDate = std::ctime(&now_c);
	m_sDate = m_sDate.trim();
}

UString OsuUIRankingScreenInfoLabel::buildTitleString()
{
	UString titleString = m_sArtist;
	titleString.append(" - ");
	titleString.append(m_sTitle);
	titleString.append(" [");
	titleString.append(m_sDiff);
	titleString.append("]");

	return titleString;
}

UString OsuUIRankingScreenInfoLabel::buildSubTitleString()
{
	UString subTitleString = "Beatmap by ";
	subTitleString.append(m_sMapper);

	return subTitleString;
}

UString OsuUIRankingScreenInfoLabel::buildPlayerString()
{
	UString playerString = "Played by ";
	playerString.append(m_sPlayer);
	playerString.append(" on ");
	playerString.append(m_sDate);

	return playerString;
}

float OsuUIRankingScreenInfoLabel::getMinimumWidth()
{
	/*
	float titleWidth = m_font->getStringWidth(buildTitleString());
	float subTitleWidth = m_font->getStringWidth(buildSubTitleString()) * m_fSubTitleScale;
	*/
	float titleWidth = 0;
	float subTitleWidth = 0;
	float playerWidth = m_font->getStringWidth(buildPlayerString()) * m_fSubTitleScale;

	return std::max(std::max(titleWidth, subTitleWidth), playerWidth);
}

float OsuUIRankingScreenInfoLabel::getMinimumHeight()
{
	float titleHeight = m_font->getHeight();
	float subTitleHeight = m_font->getHeight() * m_fSubTitleScale;
	float playerHeight = m_font->getHeight() * m_fSubTitleScale;

	return titleHeight + subTitleHeight + playerHeight + m_iMargin*2;
}


