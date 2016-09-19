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
#include "CBaseUIImageButton.h"

#include "Osu.h"
#include "OsuSkin.h"
#include "OsuBeatmap.h"
#include "OsuKeyBindings.h"
#include "OsuHUD.h"

ConVar osu_pause_dim_background("osu_pause_dim_background", true);



class OsuPauseMenuButton : public CBaseUIImageButton
{
public:
	OsuPauseMenuButton(Osu *osu, UString imageResourceName, float xPos, float yPos, float xSize, float ySize, UString name);

	void onMouseInside();
	void onMouseOutside();

	void setBaseScale(float xScale, float yScale);

private:
	Osu *m_osu;

	Vector2 m_vBaseScale;
	float m_fScaleMultiplier;
};



OsuPauseMenu::OsuPauseMenu(Osu *osu) : OsuScreen()
{
	m_osu = osu;
	m_bScheduledVisibility = false;
	m_bScheduledVisibilityChange = false;

	m_selectedButton = NULL;
	m_fWarningArrowsAnimStartTime = 0.0f;
	m_fWarningArrowsAnimAlpha = 0.0f;
	m_fWarningArrowsAnimX = 0.0f;
	m_fWarningArrowsAnimY = 0.0f;
	m_bInitialWarningArrowFlyIn = true;

	m_bContinueEnabled = true;

	m_container = new CBaseUIContainer(0, 0, m_osu->getScreenWidth(), m_osu->getScreenHeight(), "");

	OsuPauseMenuButton *continueButton = addButton();
	OsuPauseMenuButton *retryButton = addButton();
	OsuPauseMenuButton *backButton = addButton();

	continueButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuPauseMenu::onContinueClicked) );
	retryButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuPauseMenu::onRetryClicked) );
	backButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuPauseMenu::onBackClicked) );

	updateButtons();
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
		float offset = m_osu->getUIScale(m_osu, 20.0f + 45.0f*animation);

		g->setColor(arrowColor);
		g->setAlpha(m_fWarningArrowsAnimAlpha);
		m_osu->getHUD()->drawWarningArrow(g, Vector2(m_fWarningArrowsAnimX, m_fWarningArrowsAnimY) + Vector2(0, m_selectedButton->getSize().y/2) - Vector2(offset, 0), false, false);
		m_osu->getHUD()->drawWarningArrow(g, Vector2(m_osu->getScreenWidth() - m_fWarningArrowsAnimX, m_fWarningArrowsAnimY) + Vector2(0, m_selectedButton->getSize().y/2) + Vector2(offset, 0), true, false);
	}
}

void OsuPauseMenu::update()
{
	if (!m_bVisible) return;

	m_container->update();

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
	}
}

void OsuPauseMenu::onKeyDown(KeyboardEvent &e)
{
	if (!m_bVisible) return;

	if (e == KEY_1)
		onContinueClicked();
	else if (e == KEY_2)
		onRetryClicked();
	else if (e == KEY_3)
		onBackClicked();
	else if (e == (KEYCODE)OsuKeyBindings::LEFT_CLICK.getInt() || e == (KEYCODE)OsuKeyBindings::RIGHT_CLICK.getInt())
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

	// handle arrow keys selection
	if (m_buttons.size() > 0)
	{
		if (!engine->getKeyboard()->isAltDown() && e == KEY_DOWN)
		{
			OsuPauseMenuButton *nextSelectedButton = m_buttons[0];

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
			OsuPauseMenuButton *nextSelectedButton = m_buttons[m_buttons.size()-1];

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

	if (e != KEY_ESCAPE && e != (KEYCODE)OsuKeyBindings::GAME_PAUSE.getInt()) // needed for unpause in Osu.cpp
		e.consume();
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
}

void OsuPauseMenu::updateButtons()
{
	setButton(0, m_osu->getSkin()->getPauseContinue());
	setButton(1, m_osu->getSkin()->getPauseRetry());
	setButton(2, m_osu->getSkin()->getPauseBack());
}

void OsuPauseMenu::updateLayout()
{
	float height = (m_osu->getScreenHeight()/(float)m_buttons.size());
	float half = (m_buttons.size()-1)/2.0f;

	float maxWidth = 0.0f;
	float maxHeight = 0.0f;
	for (int i=0; i<m_buttons.size(); i++)
	{
		Image *img = engine->getResourceManager()->getImage(m_buttons[i]->getImageResourceName());
		if (img == NULL)
			img = engine->getResourceManager()->getImage("MISSING_TEXTURE");

		float scale = m_osu->getUIScale(m_osu, 256) / (411.0f * (m_osu->getSkin()->isPauseContinue2x() ? 2.0f : 1.0f));

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

		float pinch = std::max(0.0f, (height/2.0f - maxHeight/2.0f));
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

	// reset
	m_selectedButton = NULL;
	m_bInitialWarningArrowFlyIn = true;
	m_fWarningArrowsAnimAlpha = 0.0f;
	m_bScheduledVisibility = visible;
	m_bScheduledVisibilityChange = false;

	if (m_bVisible)
	{
		updateButtons();
		updateLayout();
	}

	m_osu->updateConfineCursor();
}

void OsuPauseMenu::setContinueEnabled(bool continueEnabled)
{
	m_bContinueEnabled = continueEnabled;
	if (m_buttons.size() > 0)
		m_buttons[0]->setVisible(m_bContinueEnabled);
}

OsuPauseMenuButton *OsuPauseMenu::addButton()
{
	OsuPauseMenuButton *button = new OsuPauseMenuButton(m_osu, "MISSING_TEXTURE", 0, 0, 0, 0, "");
	m_container->addBaseUIElement(button);
	m_buttons.push_back(button);
	return button;
}

void OsuPauseMenu::setButton(int i, Image *img)
{
	if (i > -1 && i < m_buttons.size())
	{
		m_buttons[i]->setImageResourceName(img == NULL ? "MISSING_TEXTURE" : img->getName());
	}
}



OsuPauseMenuButton::OsuPauseMenuButton(Osu *osu, UString imageResourceName, float xPos, float yPos, float xSize, float ySize, UString name) : CBaseUIImageButton(imageResourceName, xPos, yPos, xSize, ySize, name)
{
	m_osu = osu;
	m_fScaleMultiplier = 1.1f;
}

void OsuPauseMenuButton::setBaseScale(float xScale, float yScale)
{
	m_vBaseScale.x = xScale;
	m_vBaseScale.y = yScale;

	m_vScale = m_vBaseScale;
}

void OsuPauseMenuButton::onMouseInside()
{
	CBaseUIImageButton::onMouseInside();

	if (engine->hasFocus())
		engine->getSound()->play(m_osu->getSkin()->getMenuClick());

	float animationDuration = 0.09f;
	anim->moveLinear(&m_vScale.x, m_vBaseScale.x * m_fScaleMultiplier, animationDuration, true);
	anim->moveLinear(&m_vScale.y, m_vBaseScale.y * m_fScaleMultiplier, animationDuration, true);
}

void OsuPauseMenuButton::onMouseOutside()
{
	CBaseUIImageButton::onMouseOutside();

	float animationDuration = 0.09f;
	anim->moveLinear(&m_vScale.x, m_vBaseScale.x, animationDuration, true);
	anim->moveLinear(&m_vScale.y, m_vBaseScale.y, animationDuration, true);
}
