//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		song browser beatmap info (top left)
//
// $NoKeywords: $osusbil
//===============================================================================//

#ifndef OSUUISONGBROWSERINFOLABEL_H
#define OSUUISONGBROWSERINFOLABEL_H

#include "CBaseUIButton.h"

class McFont;

class Osu;
class OsuBeatmap;
class OsuBeatmapDifficulty;

class OsuUISongBrowserInfoLabel : public CBaseUIButton
{
public:
	OsuUISongBrowserInfoLabel(Osu *osu, float xPos, float yPos, float xSize, float ySize, UString name);

	void draw(Graphics *g);
	void update();

	void setFromBeatmap(OsuBeatmap *beatmap, OsuBeatmapDifficulty *diff);
	void setFromMissingBeatmap(long beatmapId);

	void setArtist(UString artist) {m_sArtist = artist;}
	void setTitle(UString title) {m_sTitle = title;}
	void setDiff(UString diff) {m_sDiff = diff;}
	void setMapper(UString mapper) {m_sMapper = mapper;}

	void setLengthMS(unsigned long lengthMS) {m_iLengthMS = lengthMS;}
	void setBPM(int minBPM, int maxBPM) {m_iMinBPM = minBPM;m_iMaxBPM = maxBPM;}
	void setNumObjects(int numObjects) {m_iNumObjects = numObjects;}

	void setCS(float CS) {m_fCS = CS;}
	void setAR(float AR) {m_fAR = AR;}
	void setOD(float OD) {m_fOD = OD;}
	void setHP(float HP) {m_fHP = HP;}
	void setStars(float stars) {m_fStars = stars;}
	void setStarsRecalculating(bool recalculating) {m_bStarsRecalculating = recalculating;}

	void setLocalOffset(long localOffset) {m_iLocalOffset = localOffset;}
	void setOnlineOffset(long onlineOffset) {m_iOnlineOffset = onlineOffset;}

	float getMinimumWidth();
	float getMinimumHeight();

	long getBeatmapID() const {return m_iBeatmapId;}

private:
	virtual void onClicked();

	UString buildTitleString();
	UString buildSubTitleString();
	UString buildSongInfoString();
	UString buildDiffInfoString();
	UString buildOffsetInfoString();

	ConVar *m_osu_debug_ref;

	Osu *m_osu;
	McFont *m_font;

	int m_iMargin;
	float m_fTitleScale;
	float m_fSubTitleScale;
	float m_fSongInfoScale;
	float m_fDiffInfoScale;
	float m_fOffsetInfoScale;

	UString m_sArtist;
	UString m_sTitle;
	UString m_sDiff;
	UString m_sMapper;

	unsigned long m_iLengthMS;
	int m_iMinBPM;
	int m_iMaxBPM;
	int m_iNumObjects;

	float m_fCS;
	float m_fAR;
	float m_fOD;
	float m_fHP;
	float m_fStars;
	bool m_bStarsRecalculating;

	long m_iLocalOffset;
	long m_iOnlineOffset;

	// custom
	long m_iBeatmapId;
};

#endif
