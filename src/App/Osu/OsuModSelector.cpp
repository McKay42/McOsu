//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		difficulty modifier selection screen
//
// $NoKeywords: $osums
//===============================================================================//

#include "OsuModSelector.h"

#include "Engine.h"
#include "SoundEngine.h"
#include "Keyboard.h"
#include "Environment.h"
#include "Mouse.h"
#include "ResourceManager.h"
#include "AnimationHandler.h"
#include "VertexArrayObject.h"
#include "ConVar.h"

#include "Osu.h"
#include "OsuSkin.h"
#include "OsuBeatmap.h"
#include "OsuBeatmapDifficulty.h"
#include "OsuSongBrowser.h"
#include "OsuTooltipOverlay.h"
#include "OsuKeyBindings.h"
#include "OsuGameRules.h"

#include "CBaseUIContainer.h"
#include "CBaseUIImageButton.h"
#include "CBaseUILabel.h"
#include "CBaseUISlider.h"
#include "CBaseUICheckbox.h"

#include "OsuUIButton.h"
#include "OsuUISlider.h"
#include "OsuUICheckbox.h"



class OsuModSelectorModButton : public CBaseUIImageButton
{
public:
	OsuModSelectorModButton(Osu *osu, OsuModSelector* osuModSelector, float xPos, float yPos, float xSize, float ySize, UString name);

	virtual void draw(Graphics *g);
	virtual void update();

	virtual void onMouseDownInside();

	void resetState();

	void setBaseScale(float xScale, float yScale);
	void setAvailable(bool available) {m_bAvailable = available;}
	void setTooltipText(UString text);

	inline int getState() const {return m_iState;}

private:
	void setState(int state);

	Osu *m_osu;
	OsuModSelector *m_osuModSelector;

	std::vector<UString> m_tooltipTextLines;
	bool m_bAvailable;
	int m_iState;
	float m_fEnabledScaleMultiplier;
	float m_fEnabledRotationDeg;

	Vector2 m_vBaseScale;
};



OsuModSelector::OsuModSelector(Osu *osu) : OsuScreen()
{
	m_osu = osu;

	m_fAnimation = 0.0f;
	m_fExperimentalAnimation = 0.0f;
	m_bScheduledHide = false;
	m_bExperimentalVisible = false;
	m_container = new CBaseUIContainer(0, 0, Osu::getScreenWidth(), Osu::getScreenHeight(), "");
	m_overrideSliderContainer = new CBaseUIContainer(0, 0, Osu::getScreenWidth(), Osu::getScreenHeight(), "");
	m_experimentalContainer = new CBaseUIContainer(0, 0, Osu::getScreenWidth(), Osu::getScreenHeight(), "");

	m_bWaitForF1KeyUp = false;

	m_previousDifficulty = NULL;

	// build mod grid buttons
	m_iGridWidth = 5;
	m_iGridHeight = 3;

	for (int x=0; x<m_iGridWidth; x++)
	{
		for (int y=0; y<m_iGridHeight; y++)
		{
			OsuModSelectorModButton *imageButton = new OsuModSelectorModButton(m_osu, this, 50, 50, 100, 100, "");
			imageButton->setDrawBackground(false);
			imageButton->setVisible(false);

			m_container->addBaseUIElement(imageButton);
			m_modButtons.push_back(imageButton);
		}
	}

	// build override sliders
	OVERRIDE_SLIDER overrideCS = addOverrideSlider("CS Override", "CS:", convar->getConVarByName("osu_cs_override"));
	OVERRIDE_SLIDER overrideAR = addOverrideSlider("AR Override", "AR:", convar->getConVarByName("osu_ar_override"));
	OVERRIDE_SLIDER overrideOD = addOverrideSlider("OD Override", "OD:", convar->getConVarByName("osu_od_override"));
	OVERRIDE_SLIDER overrideHP = addOverrideSlider("HP Override", "HP:", convar->getConVarByName("osu_hp_override"));
	OVERRIDE_SLIDER overrideBPM = addOverrideSlider("BPM Override", "BPM:", convar->getConVarByName("osu_speed_override"), 0.0f, 2.5f);
	OVERRIDE_SLIDER overrideSpeed = addOverrideSlider("Speed Multiplier", "x", convar->getConVarByName("osu_speed_override"), 0.0f, 2.5f);
	overrideCS.slider->setChangeCallback( MakeDelegate(this, &OsuModSelector::onOverrideSliderChange) );
	overrideAR.slider->setChangeCallback( MakeDelegate(this, &OsuModSelector::onOverrideSliderChange) );
	overrideOD.slider->setChangeCallback( MakeDelegate(this, &OsuModSelector::onOverrideSliderChange) );
	overrideHP.slider->setChangeCallback( MakeDelegate(this, &OsuModSelector::onOverrideSliderChange) );
	overrideBPM.slider->setChangeCallback( MakeDelegate(this, &OsuModSelector::onOverrideSliderChange) );
	overrideBPM.slider->setValue(-1.0f, false);
	overrideSpeed.slider->setChangeCallback( MakeDelegate(this, &OsuModSelector::onOverrideSliderChange) );
	overrideSpeed.slider->setValue(-1.0f, false);

	m_ARSlider = overrideAR.slider;
	m_ODSlider = overrideOD.slider;
	m_BPMSlider = overrideBPM.slider;
	m_speedSlider = overrideSpeed.slider;

	// build experimental buttons
	addExperimentalLabel(" Experimental Mods");
	addExperimentalCheckbox("Wobble", "Playfield rotates and moves.", convar->getConVarByName("osu_mod_wobble"));
	addExperimentalCheckbox("AR Wobble", "Approach rate oscillates between -1 and +1.", convar->getConVarByName("osu_mod_arwobble"));
	addExperimentalCheckbox("MinG3012", "No 100s, only 300s or 50s. Git gud.", convar->getConVarByName("osu_mod_ming3012"));
	addExperimentalCheckbox("Timewarp", "Speed increases from 100% to 150% over the course of the beatmap.", convar->getConVarByName("osu_mod_timewarp"));
	addExperimentalCheckbox("AR Timewarp", "Approach rate decreases from 100% to 50% over the course of the beatmap.", convar->getConVarByName("osu_mod_artimewarp"));
	addExperimentalCheckbox("Minimize", "Circle size decreases from 100% to 50% over the course of the beatmap.", convar->getConVarByName("osu_mod_minimize"));
	addExperimentalCheckbox("First Person", "Centered cursor.", convar->getConVarByName("osu_mod_fps"));
	addExperimentalCheckbox("Jigsaw 1", "Wrong clicks count as misses.", convar->getConVarByName("osu_mod_jigsaw1"));
	addExperimentalCheckbox("Jigsaw 2", "Massively reduced slider follow circle radius.", convar->getConVarByName("osu_mod_jigsaw2"));
	addExperimentalCheckbox("Flip Horizontally", "Playfield is flipped horizontally.", convar->getConVarByName("osu_playfield_mirror_horizontal"));
	addExperimentalCheckbox("Flip Vertically", "Playfield is flipped vertically.", convar->getConVarByName("osu_playfield_mirror_vertical"));

	// build action buttons
	m_resetModsButton = addActionButton("1. Reset All Mods");
	m_resetModsButton->setClickCallback( MakeDelegate(this, &OsuModSelector::resetMods) );
	m_resetModsButton->setColor(0xffff3800);
	m_closeButton = addActionButton("2. Close");
	m_closeButton->setClickCallback( MakeDelegate(this, &OsuModSelector::close) );
	m_closeButton->setColor(0xff8f8f8f);

	updateButtons();
	updateLayout();
}

void OsuModSelector::updateButtons()
{
	setModButtonOnGrid(0, 0, "ez", "Reduces overall difficulty - larger circles, more forgiving HP drain, less accuracy required.", m_osu->getSkin()->getSelectionModEasy());
	setModButtonOnGrid(1, 0, "nf", "You can't fail. No matter what.", m_osu->getSkin()->getSelectionModNoFail());
	getModButtonOnGrid(1, 0)->setAvailable(false);
	setModButtonOnGrid(2, 0, "ht", "Less zoom.", m_osu->getSkin()->getSelectionModHalfTime());
	//getModButtonOnGrid(2, 0)->setAvailable(!isInCompactMode());
	setModButtonOnGrid(4, 0, "nm", "Massively reduced slider follow circle radius. Wrong clicks count as misses.", m_osu->getSkin()->getSelectionModNightmare());

	setModButtonOnGrid(0, 1, "hr", "Everything just got a bit harder...", m_osu->getSkin()->getSelectionModHardRock());
	setModButtonOnGrid(1, 1, "sd", "Miss a note and fail.", m_osu->getSkin()->getSelectionModSuddenDeath());
	getModButtonOnGrid(1, 1)->setAvailable(false);
	setModButtonOnGrid(2, 1, "dt", "Zoooooooooom.", m_osu->getSkin()->getSelectionModDoubleTime());
	//getModButtonOnGrid(2, 1)->setAvailable(!isInCompactMode());
	setModButtonOnGrid(3, 1, "hd", "Play with no approach circles and fading notes for a slight score advantage.", m_osu->getSkin()->getSelectionModHidden());
	setModButtonOnGrid(4, 1, "fl", "Restricted view area.", m_osu->getSkin()->getSelectionModFlashlight());
	getModButtonOnGrid(4, 1)->setAvailable(false);

	setModButtonOnGrid(0, 2, "relax", "You don't need to click.\nGive your clicking/tapping fingers a break from the heat of things.\n** UNRANKED **", m_osu->getSkin()->getSelectionModRelax());
	setModButtonOnGrid(1, 2, "autopilot", "Automatic cursor movement - just follow the rhythm.\n** UNRANKED **", m_osu->getSkin()->getSelectionModAutopilot());
	setModButtonOnGrid(2, 2, "spunout", "Spinners will be automatically completed.", m_osu->getSkin()->getSelectionModSpunOut());
	setModButtonOnGrid(3, 2, "auto", "Watch a perfect automated play through the song.", m_osu->getSkin()->getSelectionModAutoplay());
	setModButtonOnGrid(4, 2, "target", "Accuracy is based on the distance to the center of all hitobjects.\n300s still require at least being in the hit window of a 100 in addition to the rule above.", m_osu->getSkin()->getSelectionModTarget());
}

OsuModSelector::~OsuModSelector()
{
	SAFE_DELETE(m_container);
	SAFE_DELETE(m_overrideSliderContainer);
	SAFE_DELETE(m_experimentalContainer);
}

void OsuModSelector::draw(Graphics *g)
{
	if (!m_bVisible && !m_bScheduledHide)
		return;

	// for compact mode (and experimental mods)
	const int margin = 10;
	const Color backgroundColor = 0x88000000;

	// if we are in compact mode, draw some backgrounds under the override sliders & mod grid buttons
	if (isInCompactMode())
	{
		// get override slider element bounds
		Vector2 overrideSlidersStart = Vector2(Osu::getScreenWidth(), 0);
		Vector2 overrideSlidersSize;
		for (int i=0; i<m_overrideSliders.size(); i++)
		{
			CBaseUILabel *desc = m_overrideSliders[i].desc;
			CBaseUILabel *label = m_overrideSliders[i].label;

			if (desc->getPos().x < overrideSlidersStart.x)
				overrideSlidersStart.x = desc->getPos().x;

			if ((label->getPos().x + label->getSize().x) > overrideSlidersSize.x)
				overrideSlidersSize.x = (label->getPos().x + label->getSize().x);
			if (desc->getPos().y + desc->getSize().y > overrideSlidersSize.y)
				overrideSlidersSize.y = desc->getPos().y + desc->getSize().y;
		}
		overrideSlidersSize.x -= overrideSlidersStart.x;

		// get mod button element bounds
		Vector2 modGridButtonsStart = Vector2(Osu::getScreenWidth(), Osu::getScreenHeight());
		Vector2 modGridButtonsSize = Vector2(0, Osu::getScreenHeight());
		for (int i=0; i<m_modButtons.size(); i++)
		{
			CBaseUIButton *button = m_modButtons[i];

			if (button->getPos().x < modGridButtonsStart.x)
				modGridButtonsStart.x = button->getPos().x;
			if (button->getPos().y < modGridButtonsStart.y)
				modGridButtonsStart.y = button->getPos().y;

			if (button->getPos().x + button->getSize().x > modGridButtonsSize.x)
				modGridButtonsSize.x = button->getPos().x + button->getSize().x;
			if (button->getPos().y < modGridButtonsSize.y)
				modGridButtonsSize.y = button->getPos().y;
		}
		modGridButtonsSize.x -= modGridButtonsStart.x;

		// draw mod grid buttons
		g->pushTransform();
		g->translate(0, (1.0f - m_fAnimation)*modGridButtonsSize.y);
		g->setColor(backgroundColor);
		g->fillRect(modGridButtonsStart.x - margin, modGridButtonsStart.y - margin, modGridButtonsSize.x + 2*margin, modGridButtonsSize.y + 2*margin);
		m_container->draw(g);
		g->popTransform();

		// draw override sliders
		g->pushTransform();
		g->translate(0, -(1.0f - m_fAnimation)*overrideSlidersSize.y);
		g->setColor(backgroundColor);
		g->fillRect(overrideSlidersStart.x - margin, 0, overrideSlidersSize.x + 2*margin, overrideSlidersSize.y + margin);
		m_overrideSliderContainer->draw(g);
		g->popTransform();
	}
	else // normal mode, just draw everything
	{
		m_container->draw(g);
		m_overrideSliderContainer->draw(g);
	}

	// draw experimental mods
	g->pushTransform();
	g->translate(-(m_experimentalContainer->getSize().x + 2.0f)*(1.0f - m_fExperimentalAnimation), 0);
	g->setColor(backgroundColor);
	g->fillRect(m_experimentalContainer->getPos().x - margin, m_experimentalContainer->getPos().y - margin, m_experimentalContainer->getSize().x + 2*margin*m_fExperimentalAnimation, m_experimentalContainer->getSize().y + 2*margin);
	m_experimentalContainer->draw(g);
	g->popTransform();
}

void OsuModSelector::update()
{
	// HACKHACK: updating while invisible is stupid, but the only quick solution for still animating otherwise stuck sliders while closed
	if (!m_bVisible)
	{
		for (int i=0; i<m_overrideSliders.size(); i++)
		{
			if (m_overrideSliders[i].slider->hasChanged())
				m_overrideSliders[i].slider->update();
		}
		if (m_bScheduledHide)
		{
			if (m_fAnimation == 0.0f)
			{
				m_bScheduledHide = false;
			}
		}
		return;
	}

	// update experimental mods, they take focus precedence over everything else
	if (m_bExperimentalVisible)
	{
		m_experimentalContainer->update();
		if (m_experimentalContainer->isMouseInside())
		{
			if (!m_container->isActive())
				m_container->stealFocus();
			if (!m_overrideSliderContainer->isActive())
				m_overrideSliderContainer->stealFocus();
		}
	}
	else
		m_experimentalContainer->stealFocus();

	// update
	m_container->update();
	m_overrideSliderContainer->update();

	// handle experimental mods visibility
	bool experimentalModEnabled = false;
	for (int i=0; i<m_experimentalMods.size(); i++)
	{
		CBaseUICheckbox *checkboxPointer = dynamic_cast<CBaseUICheckbox*>(m_experimentalMods[i].element);
		if (checkboxPointer != NULL && checkboxPointer->isChecked())
		{
			experimentalModEnabled = true;
			break;
		}
	}
	Rect experimentalTrigger = Rect(0, 0, m_bExperimentalVisible ? m_experimentalContainer->getSize().x : Osu::getScreenWidth()*0.05f, Osu::getScreenHeight());
	if (experimentalTrigger.contains(engine->getMouse()->getPos()))
	{
		if (!m_bExperimentalVisible)
		{
			m_bExperimentalVisible = true;
			anim->moveQuadOut(&m_fExperimentalAnimation, 1.0f, (1.0f - m_fExperimentalAnimation)*0.15f, 0.0f, true);
		}
	}
	else if (m_bExperimentalVisible && !m_experimentalContainer->isMouseInside() && !m_experimentalContainer->isActive() && !experimentalModEnabled)
	{
		m_bExperimentalVisible = false;
		anim->moveQuadIn(&m_fExperimentalAnimation, 0.0f, m_fExperimentalAnimation*0.15f, 0.0f, true);
	}
}

void OsuModSelector::onKeyDown(KeyboardEvent &key)
{
	if (!m_bVisible)
		return;

	if (key == KEY_1)
		resetMods();

	if ((key == KEY_F1 && !m_bWaitForF1KeyUp) || key == KEY_2 || key == (KEYCODE)OsuKeyBindings::GAME_PAUSE.getInt() || key == KEY_ESCAPE)
		close();

	key.consume();
}

void OsuModSelector::onKeyUp(KeyboardEvent &key)
{
	if (!m_bVisible)
		return;

	if (key == KEY_F1)
		m_bWaitForF1KeyUp = false;
}

void OsuModSelector::setVisible(bool visible)
{
	if (visible && !m_bVisible)
	{
		m_bScheduledHide = false;

		updateButtons();
		updateLayout();
		updateOverrideSliderLabels();
		checkUpdateBPMSliderSlaves();

		m_fAnimation = 0.0f;
		anim->moveQuadOut(&m_fAnimation, 1.0f, 0.1f, 0.0f, true);

		bool experimentalModEnabled = false;
		for (int i=0; i<m_experimentalMods.size(); i++)
		{
			CBaseUICheckbox *checkboxPointer = dynamic_cast<CBaseUICheckbox*>(m_experimentalMods[i].element);
			if (checkboxPointer != NULL && checkboxPointer->isChecked())
			{
				experimentalModEnabled = true;
				break;
			}
		}
		if (experimentalModEnabled)
		{
			m_bExperimentalVisible = true;
			if (isInCompactMode())
				anim->moveQuadOut(&m_fExperimentalAnimation, 1.0f, (1.0f - m_fExperimentalAnimation)*0.06f, 0.0f, true);
			else
				m_fExperimentalAnimation = 1.0f;
		}
		else
		{
			m_bExperimentalVisible = false;
			m_fExperimentalAnimation = 0.0f;
			anim->deleteExistingAnimation(&m_fExperimentalAnimation);
		}
	}
	else if (!visible && m_bVisible)
	{
		m_bScheduledHide = isInCompactMode();

		m_fAnimation = 1.0f;
		anim->moveQuadIn(&m_fAnimation, 0.0f, 0.06f, 0.0f, true);
		updateModConVar();

		m_bExperimentalVisible = false;
		anim->moveQuadIn(&m_fExperimentalAnimation, 0.0f, 0.06f, 0.0f, true);
	}

	m_bVisible = visible;
}

bool OsuModSelector::isInCompactMode()
{
	return m_osu->isInPlayMode();
}

void OsuModSelector::updateLayout()
{
	if (m_modButtons.size() < 1 || m_overrideSliders.size() < 1)
		return;

	if (!isInCompactMode()) // normal layout
	{
		// mod grid buttons
		Vector2 center = Osu::getScreenSize()/2.0f;
		float scale = m_osu->getImageScale(m_osu->getSkin()->getSelectionModEasy(), 42.0f);
		Vector2 size = Vector2(m_osu->getSkin()->getSelectionModEasy()->getWidth(), m_osu->getSkin()->getSelectionModEasy()->getHeight())*scale;
		Vector2 offset = Vector2(size.x*1.0f, size.y*0.25f);
		Vector2 start = Vector2(center.x - (size.x*m_iGridWidth)/2.0f - (offset.x*(m_iGridWidth-1))/2.0f, center.y - (size.y*m_iGridHeight)/2.0f  - (offset.y*(m_iGridHeight-1))/2.0f);

		for (int x=0; x<m_iGridWidth; x++)
		{
			for (int y=0; y<m_iGridHeight; y++)
			{
				OsuModSelectorModButton *button = getModButtonOnGrid(x, y);

				if (button != NULL)
				{
					Image *buttonImage = engine->getResourceManager()->getImage(button->getImageResourceName());

					button->setPos(start + Vector2(size.x*x + offset.x*x, size.y*y + offset.y*y));
					if (buttonImage != NULL && buttonImage->getWidth()*scale > size.x+5)
						button->setBaseScale(scale*0.5f, scale*0.5f); // HACKHACK: @2x hack
					else
						button->setBaseScale(scale, scale);
					button->setSize(size);
				}
			}
		}

		// override sliders (down here because they depend on the mod grid button alignment)
		int overrideSliderWidth = m_osu->getUIScale(250.0f);
		int overrideSliderHeight = m_overrideSliders.size() > 0 ? m_overrideSliders[0].slider->getSize().y : 25;
		int overrideSliderOffsetY = ((start.y - m_overrideSliders.size()*overrideSliderHeight)/(m_overrideSliders.size()-1))*0.35f;
		Vector2 overrideSliderStart = Vector2(Osu::getScreenWidth()/2 - overrideSliderWidth/2, start.y/2 - (m_overrideSliders.size()*overrideSliderHeight + (m_overrideSliders.size()-1)*overrideSliderOffsetY)/1.75f);
		for (int i=0; i<m_overrideSliders.size(); i++)
		{
			m_overrideSliders[i].desc->setSizeToContent(0, 0);
			m_overrideSliders[i].desc->setSizeY(m_overrideSliders[i].slider->getSize().y);

			m_overrideSliders[i].slider->setPos(overrideSliderStart.x, overrideSliderStart.y + i*overrideSliderHeight + i*overrideSliderOffsetY);
			m_overrideSliders[i].slider->setSizeX(overrideSliderWidth);

			m_overrideSliders[i].desc->setPos(m_overrideSliders[i].slider->getPos().x - m_overrideSliders[i].desc->getSize().x - 10, m_overrideSliders[i].slider->getPos().y);

			m_overrideSliders[i].label->setPos(m_overrideSliders[i].slider->getPos().x + m_overrideSliders[i].slider->getSize().x + 10, m_overrideSliders[i].slider->getPos().y);
			m_overrideSliders[i].label->setSizeToContent(0,0);
			m_overrideSliders[i].label->setSizeY(m_overrideSliders[i].slider->getSize().y);
		}

		// action buttons
		float actionMinY = start.y + size.y*m_iGridHeight + offset.y*(m_iGridHeight-1); // exact bottom of the mod buttons
		Vector2 actionSize = Vector2(m_osu->getUIScale(448.0f), size.y*0.75f);
		float actionOffsetY = actionSize.y*0.5f;
		Vector2 actionStart = Vector2(Osu::getScreenWidth()/2.0f - actionSize.x/2.0f, actionMinY + (Osu::getScreenHeight() - actionMinY)/2.0f  - (actionSize.y*m_actionButtons.size() + actionOffsetY*(m_actionButtons.size()-1))/2.0f);
		for (int i=0; i<m_actionButtons.size(); i++)
		{
			m_actionButtons[i]->setVisible(true);
			m_actionButtons[i]->setPos(actionStart.x, actionStart.y + actionSize.y*i + actionOffsetY*i);
			m_actionButtons[i]->setSize(actionSize);
		}
	}
	else // compact in-beatmap mode
	{
		// mod grid buttons
		Vector2 center = Osu::getScreenSize()/2.0f;
		float scale = m_osu->getImageScale(m_osu->getSkin()->getSelectionModEasy(), 48.0f)*0.75f;
		Vector2 blockSize = Vector2(m_osu->getSkin()->getSelectionModEasy()->getWidth(), m_osu->getSkin()->getSelectionModEasy()->getHeight())*scale;
		Vector2 offset = Vector2(blockSize.x*0.15f, blockSize.y*0.05f);
		Vector2 size = Vector2((blockSize.x*m_iGridWidth) + (offset.x*(m_iGridWidth-1)), (blockSize.y*m_iGridHeight) + (offset.y*(m_iGridHeight-1)));
		center.y = Osu::getScreenHeight() - size.y/2 - offset.y*3.0f;
		Vector2 start = Vector2(center.x - size.x/2.0f, center.y - size.y/2.0f);

		for (int x=0; x<m_iGridWidth; x++)
		{
			for (int y=0; y<m_iGridHeight; y++)
			{
				OsuModSelectorModButton *button = getModButtonOnGrid(x, y);

				if (button != NULL)
				{
					Image *buttonImage = engine->getResourceManager()->getImage(button->getImageResourceName());

					button->setPos(start + Vector2(blockSize.x*x + offset.x*x, blockSize.y*y + offset.y*y));
					if (buttonImage != NULL && buttonImage->getWidth()*scale > blockSize.x+5)
						button->setBaseScale(scale*0.5f, scale*0.5f); // HACKHACK: heuristic @2x hack, wtf
					else
						button->setBaseScale(scale, scale);
					button->setSize(blockSize);
				}
			}
		}

		// override sliders (down here because they depend on the mod grid button alignment)
		int overrideSliderWidth = m_osu->getUIScale(250.0f);
		int overrideSliderHeight = m_overrideSliders.size() > 0 ? m_overrideSliders[0].slider->getSize().y : 25;
		int overrideSliderOffsetY = 5;
		Vector2 overrideSliderStart = Vector2(Osu::getScreenWidth()/2 - overrideSliderWidth/2, 5);
		for (int i=0; i<m_overrideSliders.size(); i++)
		{
			m_overrideSliders[i].desc->setSizeToContent(0, 0);
			m_overrideSliders[i].desc->setSizeY(m_overrideSliders[i].slider->getSize().y);

			m_overrideSliders[i].slider->setPos(overrideSliderStart.x, overrideSliderStart.y + i*overrideSliderHeight + i*overrideSliderOffsetY);
			m_overrideSliders[i].slider->setSizeX(overrideSliderWidth);

			m_overrideSliders[i].desc->setPos(m_overrideSliders[i].slider->getPos().x - m_overrideSliders[i].desc->getSize().x - 10, m_overrideSliders[i].slider->getPos().y);

			m_overrideSliders[i].label->setPos(m_overrideSliders[i].slider->getPos().x + m_overrideSliders[i].slider->getSize().x + 10, m_overrideSliders[i].slider->getPos().y);
			m_overrideSliders[i].label->setSizeToContent(0,0);
			m_overrideSliders[i].label->setSizeY(m_overrideSliders[i].slider->getSize().y);
		}

		// action buttons
		for (int i=0; i<m_actionButtons.size(); i++)
		{
			m_actionButtons[i]->setVisible(false);
		}
	}

	// experimental mods
	int yCounter = 0;
	int experimentalMaxWidth = 0;
	int experimentalOffsetY = 6;
	for (int i=0; i<m_experimentalMods.size(); i++)
	{
		CBaseUIElement *e = m_experimentalMods[i].element;
		e->setRelPosY(yCounter);

		if (e->getSize().x > experimentalMaxWidth)
			experimentalMaxWidth = e->getSize().x;

		yCounter += e->getSize().y + experimentalOffsetY;

		if (i == 0)
			yCounter += 8;
	}
	m_experimentalContainer->setSize(experimentalMaxWidth, yCounter);
	m_experimentalContainer->setPosY(Osu::getScreenSize().y/2 - m_experimentalContainer->getSize().y/2);
}

void OsuModSelector::updateModConVar()
{
	ConVar *osu_mods_ref = convar->getConVarByName("osu_mods");

	UString modString = "";
	for (int i=0; i<m_modButtons.size(); i++)
	{
		OsuModSelectorModButton *button = m_modButtons[i];
		if (button->getState() > 0)
			modString.append(button->getName());
	}

	osu_mods_ref->setValue(modString);

	updateOverrideSliderLabels();
}

void OsuModSelector::updateOverrideSliders()
{
	for (int i=0; i<m_overrideSliders.size(); i++)
	{
		onOverrideSliderChange(m_overrideSliders[i].slider);
	}
}

void OsuModSelector::setModButtonOnGrid(int x, int y, UString modName, UString tooltipText, Image *img)
{
	OsuModSelectorModButton *imageButton = getModButtonOnGrid(x, y);

	if (imageButton != NULL)
	{
		imageButton->setName(modName);
		imageButton->setImageResourceName(img->getName());
		imageButton->setVisible(true);
		imageButton->setTooltipText(tooltipText);
	}
}

OsuModSelectorModButton *OsuModSelector::getModButtonOnGrid(int x, int y)
{
	int index = x*m_iGridHeight + y;

	if (index < m_modButtons.size())
		return m_modButtons[index];
	else
		return NULL;
}

OsuModSelector::OVERRIDE_SLIDER OsuModSelector::addOverrideSlider(UString text, UString labelText, ConVar *cvar, float min, float max)
{
	int height = 25;

	OVERRIDE_SLIDER os;
	os.desc = new CBaseUILabel(0, 0, 100, height, "", text);
	os.slider = new OsuUISlider(m_osu, 0, 0, 100, height, "");
	os.label = new CBaseUILabel(0, 0, 100, height, labelText, labelText);
	os.cvar = cvar;

	bool debugDrawFrame = false;

	os.slider->setDrawFrame(debugDrawFrame);
	os.slider->setDrawBackground(false);
	os.slider->setFrameColor(0xff777777);
	os.desc->setDrawFrame(debugDrawFrame);
	os.desc->setDrawBackground(false);
	os.desc->setTextColor(0xff777777);
	os.label->setDrawFrame(debugDrawFrame);
	os.label->setDrawBackground(false);
	os.label->setTextColor(0xff777777);

	os.slider->setBounds(min, max+1.0f);
	os.slider->setLiveUpdate(true);
	os.slider->setAllowMouseWheel(false);

	m_overrideSliderContainer->addBaseUIElement(os.desc);
	m_overrideSliderContainer->addBaseUIElement(os.slider);
	m_overrideSliderContainer->addBaseUIElement(os.label);
	m_overrideSliders.push_back(os);
	return os;
}

OsuUIButton *OsuModSelector::addActionButton(UString text)
{
	OsuUIButton *actionButton = new OsuUIButton(m_osu, 50, 50, 100, 100, text, text);
	m_actionButtons.push_back(actionButton);
	m_container->addBaseUIElement(actionButton);

	return actionButton;
}

CBaseUILabel *OsuModSelector::addExperimentalLabel(UString text)
{
	CBaseUILabel *label = new CBaseUILabel(0, 0, 0, 25, text, text);
	label->setFont(m_osu->getSubTitleFont());
	label->setWidthToContent(0);
	label->setDrawBackground(false);
	label->setDrawFrame(false);
	m_experimentalContainer->addBaseUIElement(label);

	EXPERIMENTAL_MOD em;
	em.element = label;
	em.cvar = NULL;
	m_experimentalMods.push_back(em);

	return label;
}

CBaseUICheckbox *OsuModSelector::addExperimentalCheckbox(UString text, UString tooltipText, ConVar *cvar)
{
	OsuUICheckbox *checkbox = new OsuUICheckbox(m_osu, 0, 0, 0, 50, text, text);
	checkbox->setTooltipText(tooltipText);
	checkbox->setWidthToContent(0);
	if (cvar != NULL)
	{
		checkbox->setChecked(cvar->getBool());
		checkbox->setChangeCallback( MakeDelegate(this, &OsuModSelector::onCheckboxChange) );
	}
	m_experimentalContainer->addBaseUIElement(checkbox);

	EXPERIMENTAL_MOD em;
	em.element = checkbox;
	em.cvar = cvar;
	m_experimentalMods.push_back(em);

	return checkbox;
}

void OsuModSelector::resetMods()
{
	m_resetModsButton->animateClickColor();

	for (int i=0; i<m_modButtons.size(); i++)
	{
		m_modButtons[i]->resetState();
	}

	for (int i=0; i<m_overrideSliders.size(); i++)
	{
		m_overrideSliders[i].slider->setValue(m_overrideSliders[i].slider->getMin(), true);
	}

	for (int i=0; i<m_experimentalMods.size(); i++)
	{
		CBaseUICheckbox *checkboxPointer = dynamic_cast<CBaseUICheckbox*>(m_experimentalMods[i].element);
		if (checkboxPointer != NULL)
			checkboxPointer->setChecked(false);
	}
}

void OsuModSelector::close()
{
	m_closeButton->animateClickColor();
	m_osu->toggleModSelection();
}

void OsuModSelector::onOverrideSliderChange(CBaseUISlider *slider)
{
	for (int i=0; i<m_overrideSliders.size(); i++)
	{
		if (m_overrideSliders[i].slider == slider)
		{
			float sliderValue = slider->getFloat()-1.0f;

			// alt key allows rounding to only 1 decimal digit
			if (engine->getKeyboard()->isAltDown())
				sliderValue = std::round(sliderValue * 10.0f) / 10.0f;
			else
				sliderValue = std::round(sliderValue * 100.0f) / 100.0f;

			if (sliderValue < 0.0f)
			{
				sliderValue = -1.0f;
				m_overrideSliders[i].label->setWidthToContent(0);

				// HACKHACK: dirty
				if (m_osu->getSelectedBeatmap() != NULL && m_osu->getSelectedBeatmap()->getSelectedDifficulty() != NULL)
				{
					if (m_overrideSliders[i].label->getName().find("BPM") != -1)
					{
						// reset AR and OD override sliders if the bpm slider was reset
						m_ARSlider->setValue(0.0f, false);
						m_ODSlider->setValue(0.0f, false);
					}
				}
			}
			else
			{
				// HACKHACK: dirty
				if (m_osu->getSelectedBeatmap() != NULL && m_osu->getSelectedBeatmap()->getSelectedDifficulty() != NULL)
				{
					if (m_overrideSliders[i].label->getName().find("BPM") != -1)
					{
						// speed slider may not be used in conjunction
						m_speedSlider->setValue(0.0f, false);

						// force early update
						m_overrideSliders[i].cvar->setValue(sliderValue);

						// force change all other depending sliders
						float newAR = OsuGameRules::getConstantApproachRateForSpeedMultiplier(m_osu->getSelectedBeatmap());
						float newOD = OsuGameRules::getConstantOverallDifficultyForSpeedMultiplier(m_osu->getSelectedBeatmap());

						m_ARSlider->setValue(newAR+1.0f, false); // '+1' to compensate for turn-off area of the override sliders
						m_ODSlider->setValue(newOD+1.0f, false);
					}
					else if (m_overrideSliders[i].desc->getText().find("Speed") != -1)
					{
						// bpm slider may not be used in conjunction
						m_BPMSlider->setValue(0.0f, false);
					}
				}
			}

			m_overrideSliders[i].cvar->setValue(sliderValue);
			updateOverrideSliderLabels();
			break;
		}
	}
}

void OsuModSelector::updateOverrideSliderLabels()
{
	const Color inactiveColor = 0xff777777;
	const Color activeColor = 0xffffffff;
	const Color inactiveLabelColor = 0xff1166ff;

	for (int i=0; i<m_overrideSliders.size(); i++)
	{
		const float convarValue = m_overrideSliders[i].cvar->getFloat();

		// update colors
		if (convarValue < 0.0f)
		{
			m_overrideSliders[i].label->setTextColor(inactiveLabelColor);
			m_overrideSliders[i].desc->setTextColor(inactiveColor);
			m_overrideSliders[i].slider->setFrameColor(inactiveColor);
		}
		else
		{
			m_overrideSliders[i].label->setTextColor(activeColor);
			m_overrideSliders[i].desc->setTextColor(activeColor);
			m_overrideSliders[i].slider->setFrameColor(activeColor);
		}

		// update label text
		m_overrideSliders[i].label->setText(getOverrideSliderLabelText(m_overrideSliders[i], convarValue >= 0.0f));
		m_overrideSliders[i].label->setWidthToContent(0);
	}
}

UString OsuModSelector::getOverrideSliderLabelText(OsuModSelector::OVERRIDE_SLIDER s, bool active)
{
	const float convarValue = s.cvar->getFloat();

	UString newLabelText = s.label->getName();
	if (m_osu->getSelectedBeatmap() != NULL && m_osu->getSelectedBeatmap()->getSelectedDifficulty() != NULL)
	{
		// HACKHACK: dirty
		bool wasBPMslider = false;
		float beatmapValue = 1.0f;
		if (s.label->getName().find("CS") != -1)
			beatmapValue = m_osu->getSelectedBeatmap()->getSelectedDifficulty()->CS;
		else if (s.label->getName().find("AR") != -1)
			beatmapValue = active ? OsuGameRules::getRawApproachRateForSpeedMultiplier(m_osu->getSelectedBeatmap()) : OsuGameRules::getApproachRateForSpeedMultiplier(m_osu->getSelectedBeatmap());
		else if (s.label->getName().find("OD") != -1)
			beatmapValue = active ? OsuGameRules::getRawOverallDifficultyForSpeedMultiplier(m_osu->getSelectedBeatmap()) : OsuGameRules::getOverallDifficultyForSpeedMultiplier(m_osu->getSelectedBeatmap());
		else if (s.label->getName().find("HP") != -1)
			beatmapValue = m_osu->getSelectedBeatmap()->getSelectedDifficulty()->HP;
		else if (s.label->getName().find("BPM") != -1)
		{
			wasBPMslider = true;

			int minBPM = m_osu->getSelectedBeatmap()->getSelectedDifficulty()->minBPM;
			int maxBPM = m_osu->getSelectedBeatmap()->getSelectedDifficulty()->maxBPM;
			int newMinBPM = minBPM * m_osu->getSpeedMultiplier();
			int newMaxBPM = maxBPM * m_osu->getSpeedMultiplier();
			if (!active)
			{
				if (minBPM == maxBPM)
					newLabelText.append(UString::format(" %i", newMaxBPM));
				else
					newLabelText.append(UString::format(" %i-%i", newMinBPM, newMaxBPM));
			}
			else
			{
				if (m_osu->getSpeedMultiplier() == 1.0f)
					newLabelText.append(UString::format(" %i", newMaxBPM));
				else
				{
					if (minBPM == maxBPM)
						newLabelText.append(UString::format(" %i  ->  %i", maxBPM, newMaxBPM));
					else
						newLabelText.append(UString::format(" %i-%i  ->  %i-%i", minBPM, maxBPM,  newMinBPM, newMaxBPM));
				}
			}
		}
		else if (s.desc->getText().find("Speed") != -1)
			beatmapValue = active ? m_osu->getRawSpeedMultiplier() : m_osu->getSpeedMultiplier();

		// always round beatmapValue to 1 decimal digit
		beatmapValue = std::round(beatmapValue * 10.0f) / 10.0f;

		// update label
		if (!wasBPMslider)
		{
			if (!active)
				newLabelText.append(UString::format(" %.4g", beatmapValue));
			else
				newLabelText.append(UString::format(" %.4g -> %.4g", beatmapValue, convarValue));
		}
	}

	return newLabelText;
}

void OsuModSelector::checkUpdateBPMSliderSlaves()
{
	// only force OD/AR slider constant update if diff changed
	if (m_osu->getSelectedBeatmap() != NULL && m_osu->getSelectedBeatmap()->getSelectedDifficulty() != NULL)
	{
		OsuBeatmapDifficulty *diff = m_osu->getSelectedBeatmap()->getSelectedDifficulty();
		if (diff != m_previousDifficulty)
		{
			m_previousDifficulty = diff;

			for (int i=0; i<m_overrideSliders.size(); i++)
			{
				if (m_overrideSliders[i].slider == m_BPMSlider)
				{
					float sliderValue = m_overrideSliders[i].slider->getFloat()-1.0f;
					sliderValue = std::floor(sliderValue * 100) / 100.0f;

					if (sliderValue >= 0.0f) // force constant AR/OD recalculation if bpm slider is active and we have changed diffs/maps
						onOverrideSliderChange(m_overrideSliders[i].slider);
					break;
				}
			}
		}
	}
}

void OsuModSelector::onCheckboxChange(CBaseUICheckbox *checkbox)
{
	for (int i=0; i<m_experimentalMods.size(); i++)
	{
		if (m_experimentalMods[i].element == checkbox)
		{
			if (m_experimentalMods[i].cvar != NULL)
				m_experimentalMods[i].cvar->setValue(checkbox->isChecked());
			break;
		}
	}
}

void OsuModSelector::onResolutionChange(Vector2 newResolution)
{
	m_container->setSize(newResolution);
	m_overrideSliderContainer->setSize(newResolution);
	m_experimentalContainer->setSize(newResolution);
	updateLayout();
}



OsuModSelectorModButton::OsuModSelectorModButton(Osu *osu, OsuModSelector *osuModSelector, float xPos, float yPos, float xSize, float ySize, UString name) : CBaseUIImageButton("MISSING_TEXTURE", xPos, yPos, xSize, ySize, name)
{
	m_osu = osu;
	m_osuModSelector = osuModSelector;
	m_iState = 0;
	m_vBaseScale = Vector2(1, 1);

	m_fEnabledScaleMultiplier = 1.25f;
	m_fEnabledRotationDeg = 6.0f;
	m_bAvailable = true;
}

void OsuModSelectorModButton::resetState()
{
	setState(0);
}

void OsuModSelectorModButton::draw(Graphics *g)
{
	CBaseUIImageButton::draw(g);
	if (!m_bVisible) return;

	if (!m_bAvailable)
	{
		g->setColor(0xffffffff);
		g->drawLine(m_vPos, m_vPos + m_vSize);
		g->drawLine(m_vPos.x + m_vSize.x, m_vPos.y, m_vPos.x, m_vPos.y + m_vSize.y);
	}
}

void OsuModSelectorModButton::update()
{
	CBaseUIImageButton::update();
	if (!m_bVisible) return;

	if (isMouseInside() && m_bAvailable)
	{
		m_osu->getTooltipOverlay()->begin();
		for (int i=0; i<m_tooltipTextLines.size(); i++)
		{
			m_osu->getTooltipOverlay()->addLine(m_tooltipTextLines[i]);
		}
		m_osu->getTooltipOverlay()->end();
	}
}

void OsuModSelectorModButton::onMouseDownInside()
{
	CBaseUIImageButton::onMouseDownInside();

	if (!m_bAvailable)
		return;

	if (m_iState == 0)
	{
		setState(1);
		engine->getSound()->play(m_osu->getSkin()->getCheckOn());
	}
	else
	{
		setState(0);
		engine->getSound()->play(m_osu->getSkin()->getCheckOff());
	}
}

void OsuModSelectorModButton::setBaseScale(float xScale, float yScale)
{
	m_vBaseScale.x = xScale;
	m_vBaseScale.y = yScale;
	m_vScale = m_vBaseScale;

	if (m_iState > 0)
	{
		m_vScale = m_vBaseScale * m_fEnabledScaleMultiplier;
		m_fRot = m_fEnabledRotationDeg;
	}
}

void OsuModSelectorModButton::setState(int state)
{
	m_iState = state;

	float animationDuration = 0.05;

	if (m_iState == 1)
	{
		anim->moveLinear(&m_fRot, m_fEnabledRotationDeg, animationDuration, true);
		anim->moveLinear(&m_vScale.x, m_vBaseScale.x * m_fEnabledScaleMultiplier, animationDuration, true);
		anim->moveLinear(&m_vScale.y, m_vBaseScale.y * m_fEnabledScaleMultiplier, animationDuration, true);
	}
	else
	{
		anim->moveLinear(&m_fRot, 0.0f, animationDuration, true);
		anim->moveLinear(&m_vScale.x, m_vBaseScale.x, animationDuration, true);
		anim->moveLinear(&m_vScale.y, m_vBaseScale.y, animationDuration, true);
	}

	m_osuModSelector->updateModConVar();
}

void OsuModSelectorModButton::setTooltipText(UString text)
{
	m_tooltipTextLines = text.split("\n");
}
