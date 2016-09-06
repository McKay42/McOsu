//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		score/results/ranking screen
//
// $NoKeywords: $osuss
//===============================================================================//

#ifndef OSU_OSURANKINGSCREEN_H
#define OSU_OSURANKINGSCREEN_H

#include "OsuScreenBackable.h"

class OsuRankingScreen : public OsuScreenBackable
{
public:
	OsuRankingScreen(Osu *osu);
	virtual ~OsuRankingScreen();

	void draw(Graphics *g);
	void update();

private:
	virtual void onBack();
};

#endif
