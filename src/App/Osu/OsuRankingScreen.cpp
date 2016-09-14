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
#include "ConVar.h"

#include "CBaseUIContainer.h"
#include "CBaseUIScrollView.h"
#include "CBaseUIImage.h"

#include "Osu.h"
#include "OsuSkin.h"
#include "OsuScore.h"

#include "OsuUIRankingScreenRankingPanel.h"

ConVar osu_rankingscreen_topbar_height_percent("osu_rankingscreen_topbar_height_percent", 0.785f);

OsuRankingScreen::OsuRankingScreen(Osu *osu) : OsuScreenBackable(osu)
{
	m_container = new CBaseUIContainer(0, 0, 0, 0, "");
	m_rankings = new CBaseUIScrollView(0, 0, 0, 0, "");
	m_rankings->setHorizontalScrolling(false);
	m_rankings->setVerticalScrolling(true);
	m_rankings->setDrawFrame(false);
	m_rankings->setDrawBackground(false);
	m_rankings->setDrawScrollbars(false);
	m_container->addBaseUIElement(m_rankings);

	m_rankingTitle = new CBaseUIImage(m_osu->getSkin()->getRankingTitle()->getName(), 0, 0, 0, 0, "");
	m_rankingTitle->setDrawBackground(false);
	m_rankingTitle->setDrawFrame(true);
	m_container->addBaseUIElement(m_rankingTitle);

	m_rankingPanel = new OsuUIRankingScreenRankingPanel(osu);
	m_rankingPanel->setDrawBackground(false);
	m_rankingPanel->setDrawFrame(true);
	m_rankings->getContainer()->addBaseUIElement(m_rankingPanel);

	updateLayout();
}

OsuRankingScreen::~OsuRankingScreen()
{
	SAFE_DELETE(m_container);
}

void OsuRankingScreen::draw(Graphics *g)
{
	if (!m_bVisible) return;

	// TODO: draw beatmap background image
	g->setColor(0xffaaaaaa);
	g->fillRect(0, 0, 9000, 9000);

	m_rankings->draw(g);

	g->setColor(0xff000000);
	g->fillRect(0, 0, m_osu->getScreenWidth(), m_rankingTitle->getSize().y*osu_rankingscreen_topbar_height_percent.getFloat());

	m_rankingTitle->draw(g);

	OsuScreenBackable::draw(g);
}

void OsuRankingScreen::update()
{
	OsuScreenBackable::update();
	if (!m_bVisible) return;

	m_container->update();
}

void OsuRankingScreen::setScore(OsuScore *score)
{
	m_rankingPanel->setScore(score);
}

void OsuRankingScreen::updateLayout()
{
	OsuScreenBackable::updateLayout();

	m_container->setSize(m_osu->getScreenSize());
	m_rankings->setSize(m_osu->getScreenSize());

	m_rankingTitle->setImage(m_osu->getSkin()->getRankingTitle());
	m_rankingTitle->setScale(Osu::getImageScale(m_osu, m_rankingTitle->getImage(), 75.0f), Osu::getImageScale(m_osu, m_rankingTitle->getImage(), 75.0f));
	m_rankingTitle->setSize(m_rankingTitle->getImage()->getWidth()*m_rankingTitle->getScale().x, m_rankingTitle->getImage()->getHeight()*m_rankingTitle->getScale().y);
	m_rankingTitle->setRelPos(m_container->getSize().x - m_rankingTitle->getSize().x - m_osu->getUIScale(m_osu, 20.0f), 0);

	Vector2 hardcodedOsuRankingPanelImageSize = Vector2(622, 505) * (m_osu->getSkin()->isRankingPanel2x() ? 2.0f : 1.0f);
	m_rankingPanel->setImage(m_osu->getSkin()->getRankingPanel());
	m_rankingPanel->setScale(Osu::getImageScale(m_osu, hardcodedOsuRankingPanelImageSize, 317.0f), Osu::getImageScale(m_osu, hardcodedOsuRankingPanelImageSize, 317.0f));
	m_rankingPanel->setSize(m_rankingPanel->getImage()->getWidth()*m_rankingPanel->getScale().x, m_rankingPanel->getImage()->getHeight()*m_rankingPanel->getScale().y);
	m_rankingPanel->setRelPosY(m_rankingTitle->getSize().y*0.825f);

	m_container->update_pos();
	m_rankings->getContainer()->update_pos();
	m_rankings->setScrollSizeToContent();
}

void OsuRankingScreen::onBack()
{
	engine->getSound()->play(m_osu->getSkin()->getMenuClick());

	// stop applause sound
	if (m_osu->getSkin()->getApplause() != NULL && m_osu->getSkin()->getApplause()->isPlaying())
		engine->getSound()->stop(m_osu->getSkin()->getApplause());

	m_osu->toggleRankingScreen();
}
