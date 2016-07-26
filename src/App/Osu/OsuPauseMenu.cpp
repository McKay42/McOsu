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
#include "OsuSongBrowser.h"
#include "OsuKeyBindings.h"

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

	m_container = new CBaseUIContainer(0, 0, Osu::getScreenWidth(), Osu::getScreenHeight(), "");

	OsuPauseMenuButton *continueButton = addButton();
	OsuPauseMenuButton *retryButton = addButton();
	OsuPauseMenuButton *backButton = addButton();

	continueButton->setClickCallback( MakeDelegate(this, &OsuPauseMenu::onContinueClicked) );
	retryButton->setClickCallback( MakeDelegate(this, &OsuPauseMenu::onRetryClicked) );
	backButton->setClickCallback( MakeDelegate(this, &OsuPauseMenu::onBackClicked) );

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
		g->fillRect(0, 0, Osu::getScreenWidth(), Osu::getScreenHeight());
	}

	/*
	g->setColor(0xffff0000);
	g->drawLine(0, Osu::getScreenHeight()/3.0f, Osu::getScreenWidth(), Osu::getScreenHeight()/3.0f);
	g->drawLine(0, (Osu::getScreenHeight()/3.0f)*2, Osu::getScreenWidth(), (Osu::getScreenHeight()/3.0f)*2);
	g->drawLine(0, (Osu::getScreenHeight()/3.0f)*3, Osu::getScreenWidth(), (Osu::getScreenHeight()/3.0f)*3);
	*/

	m_container->draw(g);
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
}

void OsuPauseMenu::onContinueClicked()
{
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
	float height = (Osu::getScreenHeight()/(float)m_buttons.size());
	float half = (m_buttons.size()-1)/2.0f;

	for (int i=0; i<m_buttons.size(); i++)
	{
		Image *img = engine->getResourceManager()->getImage(m_buttons[i]->getImageResourceName());
		if (img == NULL)
			img = engine->getResourceManager()->getImage("MISSING_TEXTURE");

		float scale = m_osu->getImageScale(m_osu->getSkin()->getPauseContinue(), 38.0f);

		if (m_osu->getSkin()->isPauseContinue2x())
			scale *= 2.0f;

		Vector2 newPos = Vector2(Osu::getScreenWidth()/2.0f - img->getWidth()*scale/2.0f, (i+1)*height - height/2.0f - img->getHeight()*scale/2.0f);

		float pinch = std::max(0.0f, (height/2.0f - img->getHeight()*scale/2.0f));
		if ((float)i < half)
			newPos.y += pinch;
		else if ((float)i > half)
			newPos.y -= pinch;

		m_buttons[i]->setPos(newPos);
		m_buttons[i]->setBaseScale(scale, scale);
		m_buttons[i]->setSize(img->getWidth()*scale, img->getHeight()*scale);
	}
}

void OsuPauseMenu::onResolutionChange(Vector2 newResolution)
{
	m_container->setSize(newResolution);
	updateLayout();
}

void OsuPauseMenu::setVisible(bool visible)
{
	m_bVisible = visible;

	if (m_bVisible)
	{
		updateButtons();
		updateLayout();
	}

	m_osu->updateConfineCursor();
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
