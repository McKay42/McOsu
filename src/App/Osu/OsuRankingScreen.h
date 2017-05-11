//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		score/results/ranking screen
//
// $NoKeywords: $osuss
//===============================================================================//

#ifndef OSU_OSURANKINGSCREEN_H
#define OSU_OSURANKINGSCREEN_H

#include "OsuScreenBackable.h"
#include "OsuScore.h"

class CBaseUIContainer;
class CBaseUIScrollView;
class CBaseUIImage;

class OsuBeatmap;
class OsuBeatmapDifficulty;
class OsuSkinImage;

class OsuUIRankingScreenInfoLabel;
class OsuUIRankingScreenRankingPanel;

class OsuRankingScreen : public OsuScreenBackable
{
public:
	OsuRankingScreen(Osu *osu);
	virtual ~OsuRankingScreen();

	void draw(Graphics *g);
	void update();

	void setVisible(bool visible);
	void setScore(OsuScore *score);
	void setBeatmapInfo(OsuBeatmap *beatmap, OsuBeatmapDifficulty *diff);

private:
	virtual void updateLayout();
	virtual void onBack();

	void drawModImage(Graphics *g, OsuSkinImage *image, Vector2 &pos);

	void setGrade(OsuScore::GRADE grade);

	UString getPPString();
	Vector2 getPPPosRaw();
	Vector2 getPPPosCenterRaw();

	CBaseUIContainer *m_container;
	CBaseUIScrollView *m_rankings;

	OsuUIRankingScreenInfoLabel *m_songInfo;
	OsuUIRankingScreenRankingPanel *m_rankingPanel;
	CBaseUIImage *m_rankingTitle;
	CBaseUIImage *m_rankingGrade;

	OsuScore::GRADE m_grade;
	float m_fUnstableRate;
	float m_fHitErrorAvgMin;
	float m_fHitErrorAvgMax;

	float m_fStarsTomTotal;
	float m_fStarsTomAim;
	float m_fStarsTomSpeed;
	float m_fPPv2;

	float m_fSpeedMultiplier;
	float m_fCS;
	float m_fAR;
	float m_fOD;
	float m_fHP;

	Vector2 m_vPPCursorMagnetAnimation;
};

#endif
