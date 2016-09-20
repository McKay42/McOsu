//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		analog to OsuUISongBrowserInfoLabel, but for the ranking screen
//
// $NoKeywords: $osursil
//===============================================================================//

#ifndef OSUUIRANKINGSCREENINFOLABEL_H
#define OSUUIRANKINGSCREENINFOLABEL_H

#include "CBaseUIElement.h"

class McFont;

class Osu;
class OsuBeatmap;
class OsuBeatmapDifficulty;

class OsuUIRankingScreenInfoLabel : public CBaseUIElement
{
public:
	OsuUIRankingScreenInfoLabel(Osu *osu, float xPos, float yPos, float xSize, float ySize, UString name);

	void draw(Graphics *g);

	void setFromBeatmap(OsuBeatmap *beatmap, OsuBeatmapDifficulty *diff);

	void setArtist(UString artist) {m_sArtist = artist;}
	void setTitle(UString title) {m_sTitle = title;}
	void setDiff(UString diff) {m_sDiff = diff;}
	void setMapper(UString mapper) {m_sMapper = mapper;}
	void setPlayer(UString player) {m_sPlayer = player;}
	void setDate(UString date) {m_sDate = date;}

	float getMinimumWidth();
	float getMinimumHeight();

private:
	UString buildTitleString();
	UString buildSubTitleString();
	UString buildPlayerString();

	Osu *m_osu;
	McFont *m_font;

	int m_iMargin;
	float m_fSubTitleScale;

	UString m_sArtist;
	UString m_sTitle;
	UString m_sDiff;
	UString m_sMapper;
	UString m_sPlayer;
	UString m_sDate;
};

#endif
