//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		score/results/ranking screen
//
// $NoKeywords: $osuss
//===============================================================================//

#include "OsuRankingScreen.h"

#include "Engine.h"
#include "ResourceManager.h"
#include "SoundEngine.h"

#include "Osu.h"
#include "OsuSkin.h"

OsuRankingScreen::OsuRankingScreen(Osu *osu) : OsuScreenBackable(osu)
{

}

OsuRankingScreen::~OsuRankingScreen()
{

}

void OsuRankingScreen::draw(Graphics *g)
{
	if (!m_bVisible) return;

	g->setColor(0xffffffff);
	g->pushTransform();
	g->translate(200, 200);
	g->drawString(engine->getResourceManager()->getFont("FONT_DEFAULT"), "Ranking:");
	g->popTransform();

	OsuScreenBackable::draw(g);
}

void OsuRankingScreen::update()
{
	OsuScreenBackable::update();
	if (!m_bVisible) return;
}

void OsuRankingScreen::onBack()
{
	engine->getSound()->play(m_osu->getSkin()->getMenuClick());

	// stop applause sound
	if (m_osu->getSkin()->getApplause() != NULL && m_osu->getSkin()->getApplause()->isPlaying())
		engine->getSound()->stop(m_osu->getSkin()->getApplause());

	m_osu->toggleRankingScreen();
}
