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
#include "Mouse.h"

#include "Osu.h"
#include "OsuSkin.h"
#include "OsuSongBrowser2.h"

#include "CBaseUIContainer.h"
#include "CBaseUIScrollView.h"

ConVar osu_songbrowser_button_active_color_a("osu_songbrowser_button_active_color_a", 220 + 10);
ConVar osu_songbrowser_button_active_color_r("osu_songbrowser_button_active_color_r", 255);
ConVar osu_songbrowser_button_active_color_g("osu_songbrowser_button_active_color_g", 255);
ConVar osu_songbrowser_button_active_color_b("osu_songbrowser_button_active_color_b", 255);

ConVar osu_songbrowser_button_inactive_color_a("osu_songbrowser_button_inactive_color_a", 240);
ConVar osu_songbrowser_button_inactive_color_r("osu_songbrowser_button_inactive_color_r", 235);
ConVar osu_songbrowser_button_inactive_color_g("osu_songbrowser_button_inactive_color_g", 73);
ConVar osu_songbrowser_button_inactive_color_b("osu_songbrowser_button_inactive_color_b", 153);

int OsuUISongBrowserButton::marginPixelsX = 9;
int OsuUISongBrowserButton::marginPixelsY = 9;
float OsuUISongBrowserButton::lastHoverSoundTime = 0;
int OsuUISongBrowserButton::sortHackCounter = 0;

// Color OsuUISongBrowserButton::inactiveDifficultyBackgroundColor = COLOR(255, 0, 150, 236); // blue

OsuUISongBrowserButton::OsuUISongBrowserButton(Osu *osu, OsuSongBrowser2 *songBrowser, CBaseUIScrollView *view, OsuUIContextMenu *contextMenu, float xPos, float yPos, float xSize, float ySize, UString name) : CBaseUIButton(xPos, yPos, xSize, ySize, name, "")
{
	m_osu = osu;
	m_view = view;
	m_songBrowser = songBrowser;
	m_contextMenu = contextMenu;

	m_font = m_osu->getSongBrowserFont();
	m_fontBold = m_osu->getSongBrowserFontBold();

	m_bVisible = false;
	m_bSelected = false;
	m_bHideIfSelected = false;

	m_bRightClick = false;
	m_bRightClickCheck = false;

	m_fTargetRelPosY = yPos;
	m_fScale = 1.0f;
	m_fOffsetPercent = 0.0f;
	m_fHoverOffsetAnimation = 0.0f;
	m_fCenterOffsetAnimation = 0.0f;
	m_fCenterOffsetVelocityAnimation = 0.0f;

	m_iSortHack = sortHackCounter++;
	m_bIsSearchMatch = true;

	m_fHoverMoveAwayAnimation = 0.0f;
	m_moveAwayState = MOVE_AWAY_STATE::MOVE_CENTER;
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
	anim->deleteExistingAnimation(&m_fHoverMoveAwayAnimation);
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
	g->setColor(m_bSelected ? getActiveBackgroundColor() : getInactiveBackgroundColor());
	g->pushTransform();
	{
		g->scale(m_fScale, m_fScale);
		g->translate(m_vPos.x + m_vSize.x/2, m_vPos.y + m_vSize.y/2);
		g->drawImage(m_osu->getSkin()->getMenuButtonBackground());
	}
	g->popTransform();
}

void OsuUISongBrowserButton::update()
{
	// HACKHACK: absolutely disgusting
	// temporarily fool CBaseUIElement with modified position and size
	{
		Vector2 posBackup = m_vPos;
		Vector2 sizeBackup = m_vSize;

		m_vPos = getActualPos();
		m_vSize = getActualSize();
		{
			CBaseUIButton::update();
		}
		m_vPos = posBackup;
		m_vSize = sizeBackup;
	}

	if (!m_bVisible) return;

	// HACKHACK: this should really be part of the UI base
	// right click detection
	if (engine->getMouse()->isRightDown())
	{
		if (!m_bRightClickCheck)
		{
			m_bRightClickCheck = true;
			m_bRightClick = isMouseInside();
		}
	}
	else
	{
		if (m_bRightClick)
		{
			if (isMouseInside())
				onRightMouseUpInside();
		}

		m_bRightClickCheck = false;
		m_bRightClick = false;
	}

	// animations need constant layout updates while visible
	updateLayoutEx();
}

void OsuUISongBrowserButton::updateLayoutEx()
{
	const float uiScale = Osu::ui_scale->getFloat();

	Image *menuButtonBackground = m_osu->getSkin()->getMenuButtonBackground();
	{
		const Vector2 minimumSize = Vector2(699.0f, 103.0f)*(m_osu->getSkin()->isMenuButtonBackground2x() ? 2.0f : 1.0f);
		const float minimumScale = Osu::getImageScaleToFitResolution(menuButtonBackground, minimumSize);
		m_fScale = Osu::getImageScale(m_osu, menuButtonBackground->getSize()*minimumScale, 64.0f) * uiScale;
	}

	if (m_bVisible) // lag prevention (animationHandler overflow)
	{
		const float centerOffsetAnimationTarget = 1.0f - clamp<float>(std::abs((m_vPos.y+(m_vSize.y/2)-m_view->getPos().y-m_view->getSize().y/2)/(m_view->getSize().y/2)), 0.0f, 1.0f);
		anim->moveQuadOut(&m_fCenterOffsetAnimation, centerOffsetAnimationTarget, 0.5f, true);

		float centerOffsetVelocityAnimationTarget = clamp<float>((std::abs(m_view->getVelocity().y))/3500.0f, 0.0f, 1.0f);

		if (m_songBrowser->isRightClickScrolling())
			centerOffsetVelocityAnimationTarget = 0.0f;

		if (m_view->isScrolling())
			anim->moveQuadOut(&m_fCenterOffsetVelocityAnimation, 0.0f, 1.0f, true);
		else
			anim->moveQuadOut(&m_fCenterOffsetVelocityAnimation, centerOffsetVelocityAnimationTarget, 1.25f, true);
	}

	setSize((int)(menuButtonBackground->getWidth()*m_fScale), (int)(menuButtonBackground->getHeight()*m_fScale));

	const float percentCenterOffsetAnimation = 0.035f;
	const float percentVelocityOffsetAnimation = 0.35f;
	const float percentHoverOffsetAnimation = 0.075f;

	// this is the minimum offset necessary to not clip into the score scrollview (including all possible max animations which can push us to the left, worst case)
	float minOffset = m_view->getSize().x*(percentCenterOffsetAnimation + percentHoverOffsetAnimation);
	{
		// also respect the width of the button image: push to the right until the edge of the button image can never be visible even if all animations are fully active
		// the 0.85f here heuristically pushes the buttons a bit further to the right than would be necessary, to make animations work better on lower resolutions (would otherwise hit the left edge too early)
		const float buttonWidthCompensation = std::max(m_view->getSize().x - getActualSize().x*0.85f, 0.0f);
		minOffset += buttonWidthCompensation;
	}

	float offsetX = minOffset - m_view->getSize().x*(percentCenterOffsetAnimation*m_fCenterOffsetAnimation*(1.0f - m_fCenterOffsetVelocityAnimation) + percentHoverOffsetAnimation*m_fHoverOffsetAnimation - percentVelocityOffsetAnimation*m_fCenterOffsetVelocityAnimation + m_fOffsetPercent);
	offsetX = clamp<float>(offsetX, 0.0f, m_view->getSize().x - getActualSize().x*0.15f); // WARNING: hardcoded to match 0.85f above for buttonWidthCompensation

	setRelPosX(offsetX);
	setRelPosY(m_fTargetRelPosY + getSize().y*0.125f*m_fHoverMoveAwayAnimation);
}

OsuUISongBrowserButton *OsuUISongBrowserButton::setVisible(bool visible)
{
	CBaseUIButton::setVisible(visible);

	deleteAnimations();

	if (m_bVisible)
	{
		// scrolling pinch effect
		m_fCenterOffsetAnimation = 1.0f;
		m_fHoverOffsetAnimation = 0.0f;

		float centerOffsetVelocityAnimationTarget = clamp<float>((std::abs(m_view->getVelocity().y)) / 3500.0f, 0.0f, 1.0f);

		if (m_songBrowser->isRightClickScrolling())
			centerOffsetVelocityAnimationTarget = 0.0f;

		m_fCenterOffsetVelocityAnimation = centerOffsetVelocityAnimationTarget;

		// force early layout update
		updateLayout();
		updateLayoutEx();
	}

	return this;
}

void OsuUISongBrowserButton::select(bool fireCallbacks, bool autoSelectBottomMostChild, bool wasParentSelected)
{
	const bool wasSelected = m_bSelected;
	m_bSelected = true;

	// callback
	if (fireCallbacks)
		onSelected(wasSelected, autoSelectBottomMostChild, wasParentSelected);
}

void OsuUISongBrowserButton::deselect()
{
	m_bSelected = false;
}

void OsuUISongBrowserButton::resetAnimations()
{
	setMoveAwayState(MOVE_AWAY_STATE::MOVE_CENTER, false);
}

void OsuUISongBrowserButton::onClicked()
{
	engine->getSound()->play(m_osu->getSkin()->getMenuClick());

	CBaseUIButton::onClicked();

	select(true, true);
}

void OsuUISongBrowserButton::onMouseInside()
{
	CBaseUIButton::onMouseInside();

	// hover sound
	if (engine->getTime() > lastHoverSoundTime + 0.05f) // to avoid earraep
	{
		if (engine->hasFocus())
			engine->getSound()->play(m_osu->getSkin()->getMenuClick());

		lastHoverSoundTime = engine->getTime();
	}

	// hover anim
	anim->moveQuadOut(&m_fHoverOffsetAnimation, 1.0f, 1.0f*(1.0f - m_fHoverOffsetAnimation), true);

	// move the rest of the buttons away from hovered-over one
	const std::vector<CBaseUIElement*> &elements = m_view->getContainer()->getElements();
	bool foundCenter = false;
	for (size_t i=0; i<elements.size(); i++)
	{
		OsuUISongBrowserButton *b = dynamic_cast<OsuUISongBrowserButton*>(elements[i]);
		if (b != NULL) // sanity
		{
			if (b == this)
			{
				foundCenter = true;
				b->setMoveAwayState(MOVE_AWAY_STATE::MOVE_CENTER);
			}
			else
				b->setMoveAwayState(foundCenter ? MOVE_AWAY_STATE::MOVE_DOWN : MOVE_AWAY_STATE::MOVE_UP);
		}
	}
}

void OsuUISongBrowserButton::onMouseOutside()
{
	CBaseUIButton::onMouseOutside();

	// reverse hover anim
	anim->moveQuadOut(&m_fHoverOffsetAnimation, 0.0f, 1.0f*m_fHoverOffsetAnimation, true);

	// only reset all other elements' state if we still should do so (possible frame delay of onMouseOutside coming together with the next element already getting onMouseInside!)
	if (m_moveAwayState == MOVE_AWAY_STATE::MOVE_CENTER)
	{
		const std::vector<CBaseUIElement*> &elements = m_view->getContainer()->getElements();
		for (size_t i=0; i<elements.size(); i++)
		{
			OsuUISongBrowserButton *b = dynamic_cast<OsuUISongBrowserButton*>(elements[i]);
			if (b != NULL) // sanity check
				b->setMoveAwayState(MOVE_AWAY_STATE::MOVE_CENTER);
		}
	}
}

void OsuUISongBrowserButton::setTargetRelPosY(float targetRelPosY)
{
	m_fTargetRelPosY = targetRelPosY;
	setRelPosY(m_fTargetRelPosY);
}

Vector2 OsuUISongBrowserButton::getActualOffset() const
{
	const float hd2xMultiplier = m_osu->getSkin()->isMenuButtonBackground2x() ? 2.0f : 1.0f;
	//const float correctedMarginPixelsX = (2*marginPixelsX + m_beatmap->getOsu()->getSkin()->getMenuButtonBackground()->getWidth()/hd2xMultiplier - 699)/2.0f;
	const float correctedMarginPixelsY = (2*marginPixelsY + m_osu->getSkin()->getMenuButtonBackground()->getHeight()/hd2xMultiplier - 103.0f)/2.0f;
	return Vector2((int)(marginPixelsX * m_fScale * hd2xMultiplier), (int)(correctedMarginPixelsY * m_fScale * hd2xMultiplier));
}

void OsuUISongBrowserButton::setMoveAwayState(OsuUISongBrowserButton::MOVE_AWAY_STATE moveAwayState, bool animate)
{
	m_moveAwayState = moveAwayState;

	// if we are not visible, destroy possibly existing animation
	if (!isVisible() || !animate)
		anim->deleteExistingAnimation(&m_fHoverMoveAwayAnimation);

	// only submit a new animation if we are visible, otherwise we would overwhelm the animationhandler with a shitload of requests every time for every button
	// (if we are not visible then we can just directly set the new value)
	switch (m_moveAwayState)
	{
	case MOVE_AWAY_STATE::MOVE_CENTER:
		{
			if (!isVisible() || !animate)
				m_fHoverMoveAwayAnimation = 0.0f;
			else
				anim->moveQuartOut(&m_fHoverMoveAwayAnimation, 0, 0.7f, isMouseInside() ? 0.0f : 0.05f, true); // add a tiny bit of delay to avoid jerky movement if the cursor is briefly between songbuttons while moving
		}
		break;

	case MOVE_AWAY_STATE::MOVE_UP:
		{
			if (!isVisible() || !animate)
				m_fHoverMoveAwayAnimation = -1.0f;
			else
				anim->moveQuartOut(&m_fHoverMoveAwayAnimation, -1.0f, 0.7f, true);
		}
		break;

	case MOVE_AWAY_STATE::MOVE_DOWN:
		{
			if (!isVisible() || !animate)
				m_fHoverMoveAwayAnimation = 1.0f;
			else
				anim->moveQuartOut(&m_fHoverMoveAwayAnimation, 1.0f, 0.7f, true);
		}
		break;
	}
}

Color OsuUISongBrowserButton::getActiveBackgroundColor() const
{
	return COLOR(clamp<int>(osu_songbrowser_button_active_color_a.getInt(), 0, 255), clamp<int>(osu_songbrowser_button_active_color_r.getInt(), 0, 255), clamp<int>(osu_songbrowser_button_active_color_g.getInt(), 0, 255), clamp<int>(osu_songbrowser_button_active_color_b.getInt(), 0, 255));
}

Color OsuUISongBrowserButton::getInactiveBackgroundColor() const
{
	return COLOR(clamp<int>(osu_songbrowser_button_inactive_color_a.getInt(), 0, 255), clamp<int>(osu_songbrowser_button_inactive_color_r.getInt(), 0, 255), clamp<int>(osu_songbrowser_button_inactive_color_g.getInt(), 0, 255), clamp<int>(osu_songbrowser_button_inactive_color_b.getInt(), 0, 255));
}
