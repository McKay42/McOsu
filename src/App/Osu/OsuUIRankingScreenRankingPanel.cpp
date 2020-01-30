//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		score + results panel (300, 100, 50, miss, combo, accuracy, score)
//
// $NoKeywords: $osursp
//===============================================================================//

#include "OsuUIRankingScreenRankingPanel.h"

#include "Engine.h"
#include "ResourceManager.h"
#include "ConVar.h"

#include "Osu.h"
#include "OsuSkin.h"
#include "OsuSkinImage.h"
#include "OsuScore.h"
#include "OsuHUD.h"

OsuUIRankingScreenRankingPanel::OsuUIRankingScreenRankingPanel(Osu *osu) : CBaseUIImage("", 0, 0, 0, 0, "")
{
	m_osu = osu;

	setImage(m_osu->getSkin()->getRankingPanel());
	setDrawFrame(true);

	m_iScore = 0;
	m_iNum300s = 0;
	m_iNum300gs = 0;
	m_iNum100s = 0;
	m_iNum100ks = 0;
	m_iNum50s = 0;
	m_iNumMisses = 0;
	m_iCombo = 0;
	m_fAccuracy = 0.0f;
}

void OsuUIRankingScreenRankingPanel::draw(Graphics *g)
{
	CBaseUIImage::draw(g);
	if (!m_bVisible) return;

	const float uiScale = /*Osu::ui_scale->getFloat()*/1.0f; // NOTE: commented for now, doesn't really work due to legacy layout expectations

	const float globalScoreScale = (m_osu->getSkin()->getVersion() > 1.0f ? 1.3f : 1.05f) * uiScale;

	const int globalYOffsetRaw = -1;
	const int globalYOffset = m_osu->getUIScale(m_osu, globalYOffsetRaw);

	// draw score
	g->setColor(0xffffffff);
	float scale = m_osu->getImageScale(m_osu, m_osu->getSkin()->getScore0(), 20.0f) * globalScoreScale;
	g->pushTransform();
	{
		g->scale(scale, scale);
		g->translate(m_vPos.x + m_osu->getUIScale(m_osu, 111.0f) * uiScale, m_vPos.y + (m_osu->getSkin()->getScore0()->getHeight()/2)*scale + (m_osu->getUIScale(m_osu, 11.0f) + globalYOffset) * uiScale);
		m_osu->getHUD()->drawScoreNumber(g, m_iScore, scale);
	}
	g->popTransform();

    // draw hit images
    const Vector2 hitImageStartPos = Vector2(40, 100 + globalYOffsetRaw);
    const Vector2 hitGridOffsetX = Vector2(200, 0);
    const Vector2 hitGridOffsetY = Vector2(0, 60);

    drawHitImage(g, m_osu->getSkin()->getHit300(), scale, hitImageStartPos);
	drawHitImage(g, m_osu->getSkin()->getHit100(), scale, hitImageStartPos + hitGridOffsetY);
	drawHitImage(g, m_osu->getSkin()->getHit50(), scale, hitImageStartPos + hitGridOffsetY*2);
	drawHitImage(g, m_osu->getSkin()->getHit300g(), scale, hitImageStartPos + hitGridOffsetX);
	drawHitImage(g, m_osu->getSkin()->getHit100k(), scale, hitImageStartPos + hitGridOffsetX + hitGridOffsetY);
	drawHitImage(g, m_osu->getSkin()->getHit0(), scale, hitImageStartPos + hitGridOffsetX + hitGridOffsetY*2);

	// draw numHits
	const Vector2 numHitStartPos = hitImageStartPos + Vector2(40, m_osu->getSkin()->getVersion() > 1.0f ? -16 : -25);
	scale = m_osu->getImageScale(m_osu, m_osu->getSkin()->getScore0(), 17.0f) * globalScoreScale;

	drawNumHits(g, m_iNum300s, scale, numHitStartPos);
	drawNumHits(g, m_iNum100s, scale, numHitStartPos + hitGridOffsetY);
	drawNumHits(g, m_iNum50s, scale, numHitStartPos + hitGridOffsetY*2);

	drawNumHits(g, m_iNum300gs, scale, numHitStartPos + hitGridOffsetX);
	drawNumHits(g, m_iNum100ks, scale, numHitStartPos + hitGridOffsetX + hitGridOffsetY);
	drawNumHits(g, m_iNumMisses, scale, numHitStartPos + hitGridOffsetX + hitGridOffsetY*2);

	const int row4 = 260;
	const int row4ImageOffset = (m_osu->getSkin()->getVersion() > 1.0f ? 20 : 8) - 20;

	// draw combo
	scale = m_osu->getImageScale(m_osu, m_osu->getSkin()->getScore0(), 17.0f) * globalScoreScale;
	g->pushTransform();
	{
		g->scale(scale, scale);
		g->translate(m_vPos.x + m_osu->getUIScale(m_osu, 15.0f) * uiScale, m_vPos.y + (m_osu->getSkin()->getScore0()->getHeight()/2)*scale + (m_osu->getUIScale(m_osu, row4 + 10) + globalYOffset) * uiScale);
		m_osu->getHUD()->drawComboSimple(g, m_iCombo, scale);
	}
	g->popTransform();

	// draw maxcombo label
	Vector2 hardcodedOsuRankingMaxComboImageSize = Vector2(162, 50) * (m_osu->getSkin()->isRankingMaxCombo2x() ? 2.0f : 1.0f);
	scale = m_osu->getImageScale(m_osu, hardcodedOsuRankingMaxComboImageSize, 32.0f) * uiScale;
	g->pushTransform();
	{
		g->scale(scale, scale);
		g->translate(m_vPos.x + m_osu->getSkin()->getRankingMaxCombo()->getWidth()*scale*0.5f + m_osu->getUIScale(m_osu, 4.0f) * uiScale, m_vPos.y + (m_osu->getUIScale(m_osu, row4 - 5 - row4ImageOffset) + globalYOffset) * uiScale);
		g->drawImage(m_osu->getSkin()->getRankingMaxCombo());
	}
	g->popTransform();

	// draw accuracy
	scale = m_osu->getImageScale(m_osu, m_osu->getSkin()->getScore0(), 17.0f) * globalScoreScale;
	g->pushTransform();
	{
		g->scale(scale, scale);
		g->translate(m_vPos.x + m_osu->getUIScale(m_osu, 195.0f) * uiScale, m_vPos.y + (m_osu->getSkin()->getScore0()->getHeight()/2)*scale + (m_osu->getUIScale(m_osu, row4 + 10) + globalYOffset) * uiScale);
		m_osu->getHUD()->drawAccuracySimple(g, m_fAccuracy*100.0f, scale);
	}
	g->popTransform();

	// draw accuracy label
	Vector2 hardcodedOsuRankingAccuracyImageSize = Vector2(192, 58) * (m_osu->getSkin()->isRankingAccuracy2x() ? 2.0f : 1.0f);
	scale = m_osu->getImageScale(m_osu, hardcodedOsuRankingAccuracyImageSize, 36.0f) * uiScale;
	g->pushTransform();
	{
		g->scale(scale, scale);
		g->translate(m_vPos.x + m_osu->getSkin()->getRankingAccuracy()->getWidth()*scale*0.5f + m_osu->getUIScale(m_osu, 183.0f) * uiScale, m_vPos.y + (m_osu->getUIScale(m_osu, row4 - 3 - row4ImageOffset) + globalYOffset) * uiScale);
		g->drawImage(m_osu->getSkin()->getRankingAccuracy());
	}
	g->popTransform();
}

void OsuUIRankingScreenRankingPanel::drawHitImage(Graphics *g, OsuSkinImage *img, float scale, Vector2 pos)
{
	const float uiScale = /*Osu::ui_scale->getFloat()*/1.0f; // NOTE: commented for now, doesn't really work due to legacy layout expectations

	///img->setAnimationFrameForce(0);
	img->draw(g, Vector2(m_vPos.x + m_osu->getUIScale(m_osu, pos.x) * uiScale, m_vPos.y + m_osu->getUIScale(m_osu, pos.y) * uiScale), uiScale);
}

void OsuUIRankingScreenRankingPanel::drawNumHits(Graphics *g, int numHits, float scale, Vector2 pos)
{
	const float uiScale = /*Osu::ui_scale->getFloat()*/1.0f; // NOTE: commented for now, doesn't really work due to legacy layout expectations

	g->pushTransform();
	{
		g->scale(scale, scale);
		g->translate(m_vPos.x + m_osu->getUIScale(m_osu, pos.x) * uiScale, m_vPos.y + (m_osu->getSkin()->getScore0()->getHeight()/2)*scale + m_osu->getUIScale(m_osu, pos.y) * uiScale);
		m_osu->getHUD()->drawComboSimple(g, numHits, scale);
	}
	g->popTransform();
}

void OsuUIRankingScreenRankingPanel::setScore(OsuScore *score)
{
	m_iScore = score->getScore();
	m_iNum300s = score->getNum300s();
	m_iNum300gs = score->getNum300gs();
	m_iNum100s = score->getNum100s();
	m_iNum100ks = score->getNum100ks();
	m_iNum50s = score->getNum50s();
	m_iNumMisses = score->getNumMisses();
	m_iCombo = score->getComboMax();
	m_fAccuracy = score->getAccuracy();
}

void OsuUIRankingScreenRankingPanel::setScore(OsuDatabase::Score score)
{
	m_iScore = score.score;
	m_iNum300s = score.num300s;
	m_iNum300gs = score.numGekis;
	m_iNum100s = score.num100s;
	m_iNum100ks = score.numKatus;
	m_iNum50s = score.num50s;
	m_iNumMisses = score.numMisses;
	m_iCombo = score.comboMax;
	m_fAccuracy = OsuScore::calculateAccuracy(score.num300s, score.num100s, score.num50s, score.numMisses);

	// round acc up from two decimal places
	if (m_fAccuracy > 0.0f)
		m_fAccuracy = std::round(m_fAccuracy*10000.0f) / 10000.0f;
}
