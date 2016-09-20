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

	void setGrade(OsuScore::GRADE grade);

	CBaseUIContainer *m_container;
	CBaseUIScrollView *m_rankings;

	OsuUIRankingScreenInfoLabel *m_songInfo;
	OsuUIRankingScreenRankingPanel *m_rankingPanel;
	CBaseUIImage *m_rankingTitle;
	CBaseUIImage *m_rankingGrade;

	OsuScore::GRADE m_grade;
	float m_fUnstableRate;
};

#endif
