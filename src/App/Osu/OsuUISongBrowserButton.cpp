//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		song browser button base class
//
// $NoKeywords: $osusbb
//===============================================================================//

#include "OsuUISongBrowserButton.h"

#include "Engine.h"
#include "SoundEngine.h"
#include "ResourceManager.h"
#include "AnimationHandler.h"
#include "ConVar.h"

#include "Osu.h"
#include "OsuSkin.h"
#include "OsuBeatmap.h"
#include "OsuBeatmapDifficulty.h"
#include "OsuSongBrowser2.h"

#include "CBaseUIContainer.h"
#include "CBaseUIScrollView.h"

#include <algorithm>

int OsuUISongBrowserButton::marginPixelsX = 9;
int OsuUISongBrowserButton::marginPixelsY = 9;
float OsuUISongBrowserButton::lastHoverSoundTime = 0;
float OsuUISongBrowserButton::maxOffsetAwaySelected = 12.0;
float OsuUISongBrowserButton::moveAwaySpeed = 0.4;

// Color OsuUISongBrowserButton::inactiveDifficultyBackgroundColor = COLOR(255, 0, 150, 236); // blue

OsuUISongBrowserButton::OsuUISongBrowserButton(Osu *osu, OsuSongBrowser2 *songBrowser, CBaseUIScrollView *view, float xPos, float yPos, float xSize, float ySize, UString name) : CBaseUIButton(xPos, yPos, xSize, ySize, name, "")
{
	m_osu = osu;
	m_view = view;
	m_songBrowser = songBrowser;

	m_font = m_osu->getSongBrowserFont();
	m_fontBold = m_osu->getSongBrowserFontBold();

	m_bVisible = false;
	m_bSelected = false;
	m_bHideIfSelected = false;

	m_fScale = 1.0f;
	m_fOffsetPercent = 0.0f;
	m_fHoverOffsetAnimation = 0.0f;
	m_fCenterOffsetAnimation = 0.0f;
	m_fCenterOffsetVelocityAnimation = 0.0f;

	m_inactiveBackgroundColor = COLOR(255, 235, 73, 153); // pink
	m_activeBackgroundColor = COLOR(255, 255, 255, 255); // white

	moveAwayState = MOVE_AWAY_STATE::MOVE_CENTER;
	m_fOffsetAwaySelected = 0.0f;
}

OsuUISongBrowserButton::~OsuUISongBrowserButton()
{
	deleteAnimations();
}

void OsuUISongBrowserButton::deleteAnimations()
{
	anim->deleteExistingAnimation(&m_fCenterOffsetAnimation);
	anim->deleteExistingAnimation(&m_fCenterOffsetVelocityAnimation);
	anim->deleteExistingAnimation(&m_fHoverOffsetAnimation);
}

void OsuUISongBrowserButton::draw(Graphics *g)
{
	if (!m_bVisible) return;

	drawMenuButtonBackground(g);

	// debug inner bounding box
	if (Osu::debug->getBool())
	{
		// scaling
		const Vector2 pos = getActualPos();
		const Vector2 size = getActualSize();

		g->setColor(0xffff00ff);
		g->drawLine(pos.x, pos.y, pos.x+size.x, pos.y);
		g->drawLine(pos.x, pos.y, pos.x, pos.y+size.y);
		g->drawLine(pos.x, pos.y+size.y, pos.x+size.x, pos.y+size.y);
		g->drawLine(pos.x+size.x, pos.y, pos.x+size.x, pos.y+size.y);
	}

	// debug outer/actual bounding box
	if (Osu::debug->getBool())
	{
		g->setColor(0xffff0000);
		g->drawLine(m_vPos.x, m_vPos.y, m_vPos.x+m_vSize.x, m_vPos.y);
		g->drawLine(m_vPos.x, m_vPos.y, m_vPos.x, m_vPos.y+m_vSize.y);
		g->drawLine(m_vPos.x, m_vPos.y+m_vSize.y, m_vPos.x+m_vSize.x, m_vPos.y+m_vSize.y);
		g->drawLine(m_vPos.x+m_vSize.x, m_vPos.y, m_vPos.x+m_vSize.x, m_vPos.y+m_vSize.y);
	}
}

void OsuUISongBrowserButton::drawMenuButtonBackground(Graphics *g)
{
	g->setColor(m_bSelected ? m_activeBackgroundColor : m_inactiveBackgroundColor);
	g->pushTransform();
		g->scale(m_fScale, m_fScale);
		g->translate(m_vPos.x + m_vSize.x/2, m_vPos.y + m_vSize.y/2);
		g->drawImage(m_osu->getSkin()->getMenuButtonBackground());
	g->popTransform();
}

void OsuUISongBrowserButton::update()
{
	// HACKHACK: DAYUM SON this is literally hitler
	// temporarily fool CBaseUIElement with modified position and size
	Vector2 posBackup = m_vPos;
	Vector2 sizeBackup = m_vSize;
	m_vPos = getActualPos();
	m_vSize = getActualSize();
	CBaseUIButton::update();
	m_vPos = posBackup;
	m_vSize = sizeBackup;

	if (!m_bVisible) return;

	// animations need constant layout updates anyway
	updateLayout();
}

void OsuUISongBrowserButton::updateLayout()
{
	Image *menuButtonBackground = m_osu->getSkin()->getMenuButtonBackground();
	Vector2 minimumSize = Vector2(699.0f, 103.0f)*(m_osu->getSkin()->isMenuButtonBackground2x() ? 2.0f : 1.0f);
	float minimumScale = Osu::getImageScaleToFitResolution(menuButtonBackground, minimumSize);
	m_fScale = Osu::getImageScale(m_osu, menuButtonBackground->getSize()*minimumScale, 64.0f);

	if (m_bVisible) // lag prevention (animationHandler overflow)
	{
		const float centerOffsetAnimationTarget = clamp<float>(std::abs((m_vPos.y+(m_vSize.y/2)-m_view->getPos().y-m_view->getSize().y/2)/(m_view->getSize().y/2)), 0.0f, 1.0f);
		anim->moveQuadOut(&m_fCenterOffsetAnimation, centerOffsetAnimationTarget, 0.5f, true);

		float centerOffsetVelocityAnimationTarget = clamp<float>((std::abs(m_view->getVelocity().y))/3500.0f, 0.0f, 1.0f);
		if (m_view->isScrolling())
			anim->moveQuadOut(&m_fCenterOffsetVelocityAnimation, 0.0f, 1.0f, true);
		else
			anim->moveQuadOut(&m_fCenterOffsetVelocityAnimation, centerOffsetVelocityAnimationTarget, 1.25f, true);
	}

	setSize((int)(menuButtonBackground->getWidth()*m_fScale), (int)(menuButtonBackground->getHeight()*m_fScale));
	setRelPosX(m_view->getSize().x*(0.04*m_fCenterOffsetAnimation + 0.3*m_fCenterOffsetVelocityAnimation - 0.075*m_fHoverOffsetAnimation - m_fOffsetPercent + 0.45));


	switch(moveAwayState) {
	// Undo the offset by moving to the opposite direction until it's 0 (center)
	case MOVE_AWAY_STATE::MOVE_CENTER: {
		auto offset = this->getOffsetAwaySelected();

		float offsetDecrease;
		if (offset < 0) {
			offsetDecrease = -std::min(-offset, moveAwaySpeed);
		} else {
			offsetDecrease = std::min(offset, moveAwaySpeed);
		}
		setRelPosY(this->getRelPos().y - offsetDecrease);

		this->setOffsetAwaySelected(offset - offsetDecrease);

		//anim->moveQuadOut(&m_fOffsetAwaySelected, 0, 0.8, false);
		break;

	}
	case MOVE_AWAY_STATE::MOVE_UP: {
		if(-this->getOffsetAwaySelected() < maxOffsetAwaySelected)
		{
			setRelPosY(this->getRelPos().y - moveAwaySpeed);
			this->setOffsetAwaySelected(this->getOffsetAwaySelected() - moveAwaySpeed);
			//anim->moveQuadOut(&m_fOffsetAwaySelected, -maxOffsetAwaySelected, 0.8, false);
		}
		break;
	}
	case MOVE_AWAY_STATE::MOVE_DOWN: {
		if(this->getOffsetAwaySelected() < maxOffsetAwaySelected)
		{

			setRelPosY(this->getRelPos().y + moveAwaySpeed);
			this->setOffsetAwaySelected(this->getOffsetAwaySelected() + moveAwaySpeed);
			//anim->moveQuadOut(&m_fOffsetAwaySelected, maxOffsetAwaySelected, 0.8, false);
		}
		break;
	}
	}
}

void OsuUISongBrowserButton::select()
{
	bool wasSelected = m_bSelected;
	m_bSelected = true;
	onSelected(wasSelected);

	/*
	// this must be first, because of shared resource deletion by name
	int scrollDiffY = m_vPos.y; // keep position consistent if children get removed above us
	if (getPreviousParent() != NULL)
	{
		getPreviousParent()->deselect();

		// compensate
		scrollDiffY = scrollDiffY - m_vPos.y;
		if (scrollDiffY > 0)
			m_view->scrollY(scrollDiffY, false);
	}
	*/

	// hide ourself, show children
	/*
	if (!isChild())
	{
		std::vector<OsuUISongBrowserButton*> children = getChildren();
		if (children.size() > 0)
		{
			for (int i=0; i<children.size(); i++)
			{
				if (m_bRemoveParentOnSelection)
					m_view->getContainer()->insertBaseUIElement(children[i], this);
				else
					m_view->getContainer()->insertBaseUIElementBack(children[i], this);
				children[i]->setRelPos(getRelPos());
				children[i]->setPos(getPos());
				children[i]->setVisible(true);
			}
			if (m_bRemoveParentOnSelection)
				m_view->getContainer()->removeBaseUIElement(this);

			// update layout
			m_songBrowser->updateSongButtonLayout();

			// select diff immediately, but after the layout update!
			if (m_bSelectFirstChildOnSelection)
			{
				if (selectTopChild)
					children[0]->select();
				else
					children[children.size()-1]->select(clicked);
			}

			if (m_bRemoveParentOnSelection)
				this->setVisible(false);
		}
	}
	*/
}

void OsuUISongBrowserButton::deselect()
{
	m_bSelected = false;
	onDeselected();

	/*
	// parent block
	onDeselectParent();
	m_bSelected = false;

	// show ourself, hide children
	std::vector<OsuUISongBrowserButton*> children = getChildren();
	if (children.size() > 0)
	{
		// note that this does work fine with searching, because it adds elements based on relative other elements
		// if the relative other elements can't be found, no insert or remove takes place
		if (m_bRemoveParentOnSelection)
			m_view->getContainer()->insertBaseUIElement(this, children[0]);
		setRelPos(children[0]->getRelPos());
		setPos(children[0]->getPos());
		for (int i=0; i<children.size(); i++)
		{
			m_view->getContainer()->removeBaseUIElement(children[i]);
			children[i]->setVisible(false);
		}

		// update layout
		m_songBrowser->updateSongButtonLayout();
	}
	*/
}

void OsuUISongBrowserButton::onClicked()
{
	engine->getSound()->play(m_osu->getSkin()->getMenuClick());
	CBaseUIButton::onClicked();
	select();
}

void OsuUISongBrowserButton::onMouseInside()
{

	CBaseUIButton::onMouseInside();

	if (engine->getTime() > lastHoverSoundTime + 0.05f) // to avoid earraep
	{
		if (engine->hasFocus())
			engine->getSound()->play(m_osu->getSkin()->getMenuClick());
		lastHoverSoundTime = engine->getTime();
	}

	anim->moveQuadOut(&m_fHoverOffsetAnimation, 1.0f, 1.0f*(1.0f - m_fHoverOffsetAnimation), true);


	//
	// Move the rest of the buttons away from hovered over one
	//

	std::vector<CBaseUIElement*> els = m_view->getContainer()->getAllBaseUIElements();

	auto const selected_iter = std::find(els.begin(),els.end(), this);
	auto const selected_idx = selected_iter - els.begin();

	std::vector<CBaseUIElement*> split_up(els.begin(), els.begin() + selected_idx);
	std::vector<CBaseUIElement*> split_down(els.begin() + selected_idx, els.end());

	for(auto &el: split_up) {
		dynamic_cast<OsuUISongBrowserButton*>(el)->setMoveAwayState(MOVE_AWAY_STATE::MOVE_UP);
	}

	for(auto &el: split_down) {
		dynamic_cast<OsuUISongBrowserButton*>(el)->setMoveAwayState(MOVE_AWAY_STATE::MOVE_DOWN);
	}

	// Move selected one to the center
	dynamic_cast<OsuUISongBrowserButton*>(*selected_iter)->setMoveAwayState(MOVE_AWAY_STATE::MOVE_CENTER);
}

void OsuUISongBrowserButton::onMouseOutside()
{
	CBaseUIButton::onMouseOutside();
	anim->moveQuadOut(&m_fHoverOffsetAnimation, 0.0f, 1.0f*m_fHoverOffsetAnimation, true);
}

void OsuUISongBrowserButton::setVisible(bool visible)
{
	m_bVisible = visible;

	// scrolling pinch effect
	m_fCenterOffsetAnimation = 1.0f;
	m_fHoverOffsetAnimation = 0.0f;
	const float centerOffsetVelocityAnimationTarget = clamp<float>((std::abs(m_view->getVelocity().y))/3500.0f, 0.0f, 1.0f);
	m_fCenterOffsetVelocityAnimation = centerOffsetVelocityAnimationTarget;
	deleteAnimations();

	// force early layout update
	updateLayout();
}

Vector2 OsuUISongBrowserButton::getActualOffset()
{
	const float hd2xMultiplier = m_osu->getSkin()->isMenuButtonBackground2x() ? 2.0f : 1.0f;
	///const float correctedMarginPixelsX = (2*marginPixelsX + m_beatmap->getOsu()->getSkin()->getMenuButtonBackground()->getWidth()/hd2xMultiplier - 699)/2.0f;
	const float correctedMarginPixelsY = (2*marginPixelsY + m_osu->getSkin()->getMenuButtonBackground()->getHeight()/hd2xMultiplier - 103.0f)/2.0f;
	return Vector2((int)(marginPixelsX*m_fScale*hd2xMultiplier), (int)(correctedMarginPixelsY*m_fScale*hd2xMultiplier));
}
