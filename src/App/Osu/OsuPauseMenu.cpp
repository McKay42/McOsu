//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		pause menu (while playing)
//
// $NoKeywords: $
//===============================================================================//

#include "OsuPauseMenu.h"

#include "Engine.h"
#include "SoundEngine.h"
#include "ResourceManager.h"
#include "AnimationHandler.h"
#include "Keyboard.h"
#include "ConVar.h"

#include "CBaseUIContainer.h"

#include "Osu.h"
#include "OsuSkin.h"
#include "OsuBeatmap.h"
#include "OsuKeyBindings.h"
#include "OsuModSelector.h"
#include "OsuOptionsMenu.h"
#include "OsuHUD.h"

#include "OsuUIPauseMenuButton.h"

ConVar osu_pause_dim_background("osu_pause_dim_background", true);



OsuPauseMenu::OsuPauseMenu(Osu *osu) : OsuScreen(osu)
{
	m_bScheduledVisibility = false;
	m_bScheduledVisibilityChange = false;

	m_selectedButton = NULL;
	m_fWarningArrowsAnimStartTime = 0.0f;
	m_fWarningArrowsAnimAlpha = 0.0f;
	m_fWarningArrowsAnimX = 0.0f;
	m_fWarningArrowsAnimY = 0.0f;
	m_bInitialWarningArrowFlyIn = true;

	m_bContinueEnabled = true;
	m_bClick1Down = false;
	m_bClick2Down = false;

	m_container = new CBaseUIContainer(0, 0, m_osu->getScreenWidth(), m_osu->getScreenHeight(), "");

	OsuUIPauseMenuButton *continueButton = addButton([this]() -> Image *{return m_osu->getSkin()->getPauseContinue();});
	OsuUIPauseMenuButton *retryButton = addButton([this]() -> Image *{return m_osu->getSkin()->getPauseRetry();});
	OsuUIPauseMenuButton *backButton = addButton([this]() -> Image *{return m_osu->getSkin()->getPauseBack();});

	continueButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuPauseMenu::onContinueClicked) );
	retryButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuPauseMenu::onRetryClicked) );
	backButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuPauseMenu::onBackClicked) );

	updateLayout();
}

OsuPauseMenu::~OsuPauseMenu()
{
	SAFE_DELETE(m_container);
}

void OsuPauseMenu::draw(Graphics *g)
{
	if (!m_bVisible) return;

	if (osu_pause_dim_background.getBool())
	{
		g->setColor(COLOR(150, 20, 20, 20));
		g->fillRect(0, 0, m_osu->getScreenWidth(), m_osu->getScreenHeight());
	}

	// draw overlay
	if (m_osu->getSkin()->getPauseOverlay() != m_osu->getSkin()->getMissingTexture())
	{
		const float scale = Osu::getImageScaleToFillResolution(m_osu->getSkin()->getPauseOverlay(), m_osu->getScreenSize());
		const Vector2 centerTrans = (m_osu->getScreenSize()/2);

		g->setColor(COLOR(255, 255, 255, 255));
		g->pushTransform();
		{
			g->scale(scale, scale);
			g->translate((int)centerTrans.x, (int)centerTrans.y);
			g->drawImage(m_osu->getSkin()->getPauseOverlay());
		}
		g->popTransform();
	}

	/*
	g->setColor(0xffff0000);
	g->drawLine(0, m_osu->getScreenHeight()/3.0f, m_osu->getScreenWidth(), m_osu->getScreenHeight()/3.0f);
	g->drawLine(0, (m_osu->getScreenHeight()/3.0f)*2, m_osu->getScreenWidth(), (m_osu->getScreenHeight()/3.0f)*2);
	g->drawLine(0, (m_osu->getScreenHeight()/3.0f)*3, m_osu->getScreenWidth(), (m_osu->getScreenHeight()/3.0f)*3);
	*/

	m_container->draw(g);

	if (m_selectedButton != NULL)
	{
		const Color arrowColor = COLOR(255, 0, 114, 255);
		float animation = fmod((float)(engine->getTime()-m_fWarningArrowsAnimStartTime)*3.2f, 2.0f);
		if (animation > 1.0f)
			animation = 2.0f - animation;

		animation =  -animation*(animation-2); // quad out
		const float offset = m_osu->getUIScale(m_osu, 20.0f + 45.0f*animation);

		g->setColor(arrowColor);
		g->setAlpha(m_fWarningArrowsAnimAlpha);
		m_osu->getHUD()->drawWarningArrow(g, Vector2(m_fWarningArrowsAnimX, m_fWarningArrowsAnimY) + Vector2(0, m_selectedButton->getSize().y/2) - Vector2(offset, 0), false, false);
		m_osu->getHUD()->drawWarningArrow(g, Vector2(m_osu->getScreenWidth() - m_fWarningArrowsAnimX, m_fWarningArrowsAnimY) + Vector2(0, m_selectedButton->getSize().y/2) + Vector2(offset, 0), true, false);
	}
}

void OsuPauseMenu::update()
{
	if (!m_bVisible) return;

	// update and focus handling
	m_container->update();

	if (m_osu->getOptionsMenu()->isMouseInside())
		m_container->stealFocus();

	if (m_bScheduledVisibilityChange)
	{
		m_bScheduledVisibilityChange = false;
		setVisible(m_bScheduledVisibility);
	}

	if (anim->isAnimating(&m_fWarningArrowsAnimX))
		m_fWarningArrowsAnimStartTime = engine->getTime();
}

void OsuPauseMenu::onContinueClicked()
{
	if (!m_bContinueEnabled) return;

	engine->getSound()->play(m_osu->getSkin()->getMenuHit());

	if (m_osu->getSelectedBeatmap() != NULL)
		m_osu->getSelectedBeatmap()->pause();

	scheduleVisibilityChange(false);
}

void OsuPauseMenu::onRetryClicked()
{
	engine->getSound()->play(m_osu->getSkin()->getMenuHit());

	if (m_osu->getSelectedBeatmap() != NULL)
		m_osu->getSelectedBeatmap()->restart();

	scheduleVisibilityChange(false);
}

void OsuPauseMenu::onBackClicked()
{
	engine->getSound()->play(m_osu->getSkin()->getMenuHit());

	if (m_osu->getSelectedBeatmap() != NULL)
		m_osu->getSelectedBeatmap()->stop();

	scheduleVisibilityChange(false);
}

void OsuPauseMenu::onSelectionChange()
{
	if (m_selectedButton != NULL)
	{
		if (m_bInitialWarningArrowFlyIn)
		{
			m_bInitialWarningArrowFlyIn = false;

			m_fWarningArrowsAnimY = m_selectedButton->getPos().y;
			m_fWarningArrowsAnimX = m_selectedButton->getPos().x - m_osu->getUIScale(m_osu, 170.0f);

			anim->moveLinear(&m_fWarningArrowsAnimAlpha, 1.0f, 0.3f);
			anim->moveQuadIn(&m_fWarningArrowsAnimX, m_selectedButton->getPos().x, 0.3f);
		}
		else
			m_fWarningArrowsAnimX = m_selectedButton->getPos().x;

		anim->moveQuadOut(&m_fWarningArrowsAnimY, m_selectedButton->getPos().y, 0.1f);

		engine->getSound()->play(m_osu->getSkin()->getMenuClick());
	}
}

void OsuPauseMenu::onKeyDown(KeyboardEvent &e)
{
	OsuScreen::onKeyDown(e); // only used for options menu
	if (!m_bVisible || e.isConsumed()) return;

	if (e == (KEYCODE)OsuKeyBindings::LEFT_CLICK.getInt() || e == (KEYCODE)OsuKeyBindings::RIGHT_CLICK.getInt())
	{
		bool fireButtonClick = false;
		if (e == (KEYCODE)OsuKeyBindings::LEFT_CLICK.getInt() && !m_bClick1Down)
		{
			m_bClick1Down = true;
			fireButtonClick = true;
		}
		if (e == (KEYCODE)OsuKeyBindings::RIGHT_CLICK.getInt() && !m_bClick2Down)
		{
			m_bClick2Down = true;
			fireButtonClick = true;
		}
		if (fireButtonClick)
		{
			for (int i=0; i<m_buttons.size(); i++)
			{
				if (m_buttons[i]->isMouseInside())
				{
					m_buttons[i]->click();
					break;
				}
			}
		}
	}

	// handle arrow keys selection
	if (m_buttons.size() > 0)
	{
		if (!engine->getKeyboard()->isAltDown() && e == KEY_DOWN)
		{
			OsuUIPauseMenuButton *nextSelectedButton = m_buttons[0];

			// get first visible button
			for (int i=0; i<m_buttons.size(); i++)
			{
				if (!m_buttons[i]->isVisible())
					continue;

				nextSelectedButton = m_buttons[i];
				break;
			}

			// next selection logic
			bool next = false;
			for (int i=0; i<m_buttons.size(); i++)
			{
				if (!m_buttons[i]->isVisible())
					continue;

				if (next)
				{
					nextSelectedButton = m_buttons[i];
					break;
				}
				if (m_selectedButton == m_buttons[i])
					next = true;
			}
			m_selectedButton = nextSelectedButton;
			onSelectionChange();
		}

		if (!engine->getKeyboard()->isAltDown() && e == KEY_UP)
		{
			OsuUIPauseMenuButton *nextSelectedButton = m_buttons[m_buttons.size()-1];

			// get first visible button
			for (int i=m_buttons.size()-1; i>=0; i--)
			{
				if (!m_buttons[i]->isVisible())
					continue;

				nextSelectedButton = m_buttons[i];
				break;
			}

			// next selection logic
			bool next = false;
			for (int i=m_buttons.size()-1; i>=0; i--)
			{
				if (!m_buttons[i]->isVisible())
					continue;

				if (next)
				{
					nextSelectedButton = m_buttons[i];
					break;
				}
				if (m_selectedButton == m_buttons[i])
					next = true;
			}
			m_selectedButton = nextSelectedButton;
			onSelectionChange();
		}

		if (m_selectedButton != NULL && e == KEY_ENTER)
			m_selectedButton->click();
	}

	// consume ALL events, except for a few special binds which are allowed through (e.g. for unpause or changing the local offset in Osu.cpp)
	if (e != KEY_ESCAPE && e != (KEYCODE)OsuKeyBindings::GAME_PAUSE.getInt() && e != (KEYCODE)OsuKeyBindings::INCREASE_LOCAL_OFFSET.getInt() && e != (KEYCODE)OsuKeyBindings::DECREASE_LOCAL_OFFSET.getInt())
		e.consume();
}

void OsuPauseMenu::onKeyUp(KeyboardEvent &e)
{
	if (e == (KEYCODE)OsuKeyBindings::LEFT_CLICK.getInt())
		m_bClick1Down = false;

	if (e == (KEYCODE)OsuKeyBindings::RIGHT_CLICK.getInt())
		m_bClick2Down = false;
}

void OsuPauseMenu::onChar(KeyboardEvent &e)
{
	if (!m_bVisible) return;

	e.consume();
}

void OsuPauseMenu::scheduleVisibilityChange(bool visible)
{
	if (visible != m_bVisible)
	{
		m_bScheduledVisibilityChange = true;
		m_bScheduledVisibility = visible;
	}

	// HACKHACK:
	if (!visible)
		setContinueEnabled(true);
}

void OsuPauseMenu::updateLayout()
{
	const float height = (m_osu->getScreenHeight()/(float)m_buttons.size());
	const float half = (m_buttons.size()-1)/2.0f;

	float maxWidth = 0.0f;
	float maxHeight = 0.0f;
	for (int i=0; i<m_buttons.size(); i++)
	{
		Image *img = m_buttons[i]->getImage();
		if (img == NULL)
			img = m_osu->getSkin()->getMissingTexture();

		const float scale = m_osu->getUIScale(m_osu, 256) / (411.0f * (m_osu->getSkin()->isPauseContinue2x() ? 2.0f : 1.0f));

		m_buttons[i]->setBaseScale(scale, scale);
		m_buttons[i]->setSize(img->getWidth()*scale, img->getHeight()*scale);

		if (m_buttons[i]->getSize().x > maxWidth)
			maxWidth = m_buttons[i]->getSize().x;
		if (m_buttons[i]->getSize().y > maxHeight)
			maxHeight = m_buttons[i]->getSize().y;
	}

	for (int i=0; i<m_buttons.size(); i++)
	{
		Vector2 newPos = Vector2(m_osu->getScreenWidth()/2.0f - maxWidth/2, (i+1)*height - height/2.0f - maxHeight/2.0f);

		const float pinch = std::max(0.0f, (height/2.0f - maxHeight/2.0f));
		if ((float)i < half)
			newPos.y += pinch;
		else if ((float)i > half)
			newPos.y -= pinch;

		m_buttons[i]->setPos(newPos);
		m_buttons[i]->setSize(maxWidth, maxHeight);
	}

	onSelectionChange();
}

void OsuPauseMenu::onResolutionChange(Vector2 newResolution)
{
	m_container->setSize(newResolution);
	updateLayout();
}

void OsuPauseMenu::setVisible(bool visible)
{
	m_bVisible = visible;

	if (m_osu->isInPlayMode() && m_osu->getSelectedBeatmap() != NULL)
		setContinueEnabled(!m_osu->getSelectedBeatmap()->hasFailed());
	else
		setContinueEnabled(true);

	// HACKHACK:
	m_osu->getModSelector()->setVisible(false);

	// reset
	m_selectedButton = NULL;
	m_bInitialWarningArrowFlyIn = true;
	m_fWarningArrowsAnimAlpha = 0.0f;
	m_bScheduledVisibility = visible;
	m_bScheduledVisibilityChange = false;

	if (m_bVisible)
		updateLayout();

	m_osu->updateConfineCursor();
}

void OsuPauseMenu::setContinueEnabled(bool continueEnabled)
{
	m_bContinueEnabled = continueEnabled;
	if (m_buttons.size() > 0)
		m_buttons[0]->setVisible(m_bContinueEnabled);
}

OsuUIPauseMenuButton *OsuPauseMenu::addButton(std::function<Image*()> getImageFunc)
{
	OsuUIPauseMenuButton *button = new OsuUIPauseMenuButton(m_osu, getImageFunc, 0, 0, 0, 0, "");
	m_container->addBaseUIElement(button);
	m_buttons.push_back(button);
	return button;
}
