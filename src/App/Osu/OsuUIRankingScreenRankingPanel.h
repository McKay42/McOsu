//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		score + results panel (300, 100, 50, miss, combo, accuracy, score)
//
// $NoKeywords: $osursp
//===============================================================================//

#ifndef OSUUIRANKINGSCREENRANKINGPANEL_H
#define OSUUIRANKINGSCREENRANKINGPANEL_H

#include "CBaseUIImage.h"

class Osu;
class OsuScore;
class OsuSkinImage;

class OsuUIRankingScreenRankingPanel : public CBaseUIImage
{
public:
	OsuUIRankingScreenRankingPanel(Osu *osu);
	virtual ~OsuUIRankingScreenRankingPanel();

	virtual void draw(Graphics *g);

	void setScore(OsuScore *score);

private:
	void drawHitImage(Graphics *g, OsuSkinImage *img, float scale, Vector2 pos);
	void drawNumHits(Graphics *g, int numHits, float scale, Vector2 pos);

	Osu *m_osu;

	unsigned long long m_iScore;
	int m_iNum300s;
	int m_iNum300gs;
	int m_iNum100s;
	int m_iNum100ks;
	int m_iNum50s;
	int m_iNumMisses;
	int m_iCombo;
	float m_fAccuracy;
};

#endif
