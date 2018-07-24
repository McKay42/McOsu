//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		score/results/ranking screen
//
// $NoKeywords: $osuss
//===============================================================================//

#ifndef OSU_OSURANKINGSCREEN_H
#define OSU_OSURANKINGSCREEN_H

#include "OsuScreenBackable.h"
#include "OsuDatabase.h"
#include "OsuScore.h"

class CBaseUIContainer;
class CBaseUIScrollView;
class CBaseUIImage;
class CBaseUILabel;

class OsuBeatmap;
class OsuBeatmapDifficulty;
class OsuSkinImage;

class OsuUIRankingScreenInfoLabel;
class OsuUIRankingScreenRankingPanel;

class OsuRankingScreenIndexLabel;
class OsuRankingScreenBottomElement;
class OsuRankingScreenScrollDownInfoButton;

class OsuRankingScreen : public OsuScreenBackable
{
public:
	OsuRankingScreen(Osu *osu);
	virtual ~OsuRankingScreen();

	void draw(Graphics *g);
	void update();

	void setVisible(bool visible);
	void setScore(OsuScore *score);
	void setScore(OsuDatabase::Score score, UString dateTime);
	void setBeatmapInfo(OsuBeatmap *beatmap, OsuBeatmapDifficulty *diff);

private:
	virtual void updateLayout();
	virtual void onBack();

	void drawModImage(Graphics *g, OsuSkinImage *image, Vector2 &pos, Vector2 &max);

	void onScrollDownClicked();

	void setGrade(OsuScore::GRADE grade);
	void setIndex(int index);

	UString getPPString();
	Vector2 getPPPosRaw();
	Vector2 getPPPosCenterRaw();

	CBaseUIContainer *m_container;
	CBaseUIScrollView *m_rankings;

	OsuUIRankingScreenInfoLabel *m_songInfo;
	OsuUIRankingScreenRankingPanel *m_rankingPanel;
	CBaseUIImage *m_rankingTitle;
	CBaseUIImage *m_rankingGrade;
	OsuRankingScreenIndexLabel *m_rankingIndex;
	OsuRankingScreenBottomElement *m_rankingBottom;

	OsuRankingScreenScrollDownInfoButton *m_rankingScrollDownInfoButton;
	float m_fRankingScrollDownInfoButtonAlphaAnim;

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

	bool m_bModSS;
	bool m_bModSD;
	bool m_bModEZ;
	bool m_bModHD;
	bool m_bModHR;
	bool m_bModNC;
	bool m_bModDT;
	bool m_bModNM;
	bool m_bModScorev2;
	bool m_bModTarget;
	bool m_bModSpunout;
	bool m_bModRelax;
	bool m_bModNF;
	bool m_bModHT;
	bool m_bModAutopilot;
	bool m_bModAuto;

	std::vector<ConVar*> m_enabledExperimentalMods;

	// custom
	Vector2 m_vPPCursorMagnetAnimation;
	bool m_bIsLegacyScore;
};

#endif
