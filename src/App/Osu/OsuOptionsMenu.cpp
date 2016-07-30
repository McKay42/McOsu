//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		settings
//
// $NoKeywords: $
//===============================================================================//

#include "OsuOptionsMenu.h"

#include "SoundEngine.h"
#include "Engine.h"
#include "ConVar.h"
#include "Keyboard.h"
#include "Environment.h"
#include "ResourceManager.h"
#include "Mouse.h"

#include "CBaseUIContainer.h"
#include "CBaseUIScrollView.h"
#include "CBaseUILabel.h"
#include "CBaseUICheckbox.h"
#include "CBaseUITextbox.h"

#include "Osu.h"
#include "OsuHUD.h"
#include "OsuSkin.h"
#include "OsuGameRules.h"
#include "OsuKeyBindings.h"
#include "OsuCircle.h"

#include "OsuUIButton.h"
#include "OsuUISlider.h"
#include "OsuUIBackButton.h"

#include <iostream>
#include <fstream>

ConVar osu_options_save_on_back("osu_options_save_on_back", true, "for debugging");



class OsuOptionsMenuSkinPreviewElement : public CBaseUIElement
{
public:
	OsuOptionsMenuSkinPreviewElement(Osu *osu, float xPos, float yPos, float xSize, float ySize, UString name) : CBaseUIElement(xPos, yPos, xSize, ySize, name) {m_osu = osu; m_iMode = 0;}

	virtual void draw(Graphics *g)
	{
		OsuSkin *skin = m_osu->getSkin();

		float hitcircleDiameter = m_vSize.y*0.5f;
		float numberScale = (hitcircleDiameter / (160.0f * (skin->isDefault12x() ? 2.0f : 1.0f))) * 1 * convar->getConVarByName("osu_number_scale_multiplier")->getFloat();
		float overlapScale = (hitcircleDiameter / (160.0f)) * 1 * convar->getConVarByName("osu_number_scale_multiplier")->getFloat();
		float scoreScale = 0.5f;

		if (m_iMode == 0)
		{
			float approachScale = clamp<float>(1.0f + 1.5f - fmod(engine->getTime()*3, 3.0f), 0.0f, 2.5f);
			float approachAlpha = clamp<float>(fmod(engine->getTime()*3, 3.0f)/1.5f, 0.0f, 1.0f);
			approachAlpha = -approachAlpha*(approachAlpha-2.0f);
			approachAlpha = -approachAlpha*(approachAlpha-2.0f);
			approachAlpha = 1.0f;

			OsuCircle::drawCircle(g, m_osu->getSkin(), m_vPos + Vector2(0, m_vSize.y/2) + Vector2(m_vSize.x*(1.0f/5.0f), 0.0f), hitcircleDiameter, numberScale, overlapScale, false, 42, 0, approachScale, approachAlpha, approachAlpha, true, false);
			OsuCircle::drawHitResult(g, m_osu->getSkin(), hitcircleDiameter, hitcircleDiameter, m_vPos + Vector2(0, m_vSize.y/2) + Vector2(m_vSize.x*(2.0f/5.0f), 0.0f), OsuBeatmap::HIT_100, 1.0f);
			OsuCircle::drawHitResult(g, m_osu->getSkin(), hitcircleDiameter, hitcircleDiameter, m_vPos + Vector2(0, m_vSize.y/2) + Vector2(m_vSize.x*(3.0f/5.0f), 0.0f), OsuBeatmap::HIT_50, 1.0f);
			OsuCircle::drawHitResult(g, m_osu->getSkin(), hitcircleDiameter, hitcircleDiameter, m_vPos + Vector2(0, m_vSize.y/2) + Vector2(m_vSize.x*(4.0f/5.0f), 0.0f), OsuBeatmap::HIT_MISS, 1.0f);
		}
		else if (m_iMode == 1)
		{
			const int numNumbers = 6;
			for (int i=1; i<numNumbers+1; i++)
			{
				OsuCircle::drawHitCircleNumber(g, skin, numberScale, overlapScale, m_vPos + Vector2(0, m_vSize.y/2) + Vector2(m_vSize.x*((float)i/(numNumbers+1.0f)), 0.0f), i-1, 1.0f);
			}
		}
		else if (m_iMode == 2)
		{
			const int numNumbers = 6;
			for (int i=1; i<numNumbers+1; i++)
			{
				Vector2 pos = m_vPos + Vector2(0, m_vSize.y/2) + Vector2(m_vSize.x*((float)i/(numNumbers+1.0f)), 0.0f);

				g->pushTransform();
				g->scale(scoreScale, scoreScale);
				g->translate(pos.x - skin->getScore0()->getWidth()*scoreScale, pos.y);
				m_osu->getHUD()->drawScoreNumber(g, i-1, 1.0f, false, 0);
				g->popTransform();
			}
		}
	}

	virtual void onMouseUpInside()
	{
		m_iMode++;
		m_iMode = m_iMode % 3;
	}

private:
	Osu *m_osu;
	int m_iMode;
};



OsuOptionsMenu::OsuOptionsMenu(Osu *osu) : OsuScreenBackable(osu)
{
	m_osu = osu;

	m_osu->getNotificationOverlay()->addKeyListener(this);
	m_waitingKey = NULL;

	m_fOsuFolderTextboxInvalidAnim = 0.0f;

	m_container = new CBaseUIContainer(0, 0, 0, 0, "");

	m_options = new CBaseUIScrollView(0, -1, 0, 0, "");
	m_options->setDrawFrame(true);
	m_options->setHorizontalScrolling(false);
	m_container->addBaseUIElement(m_options);

	m_contextMenu = new OsuUIListBoxContextMenu(50, 50, 150, 0, "");

	//**************************************************************************************************************************//

	addSection("General");

	addSubSection("osu!folder (Skins & Songs)");
	m_osuFolderTextbox = addTextbox(convar->getConVarByName("osu_folder")->getString(), convar->getConVarByName("osu_folder"));

	addSubSection("Window");
	addCheckbox("Pause on Focus Loss", convar->getConVarByName("osu_pause_on_focus_loss"));

	//**************************************************************************************************************************//

	addSection("Graphics");

	addSubSection("Renderer");
	addCheckbox("VSync", convar->getConVarByName("vsync"));
	addCheckbox("Show FPS Counter", convar->getConVarByName("osu_draw_fps"));
	addSpacer();
	addSlider("FPS Limiter:", 60.0f, 1000.0f, convar->getConVarByName("fps_max"))->setChangeCallback( MakeDelegate(this, &OsuOptionsMenu::onSliderChangeInt) );

	addSubSection("Layout");
	OPTIONS_ELEMENT resolutionSelect = addButton("Select Resolution", UString::format("%ix%i", Osu::getScreenWidth(), Osu::getScreenHeight()));
	((CBaseUIButton*)resolutionSelect.elements[0])->setClickCallback( MakeDelegate(this, &OsuOptionsMenu::onResolutionSelect) );
	m_resolutionLabel = (CBaseUILabel*)resolutionSelect.elements[1];
	m_resolutionSelectButton = resolutionSelect.elements[0];
	m_fullscreenCheckbox = addCheckbox("Fullscreen");
	m_fullscreenCheckbox->setChangeCallback( MakeDelegate(this, &OsuOptionsMenu::onFullscreenChange) );
	addCheckbox("Letterboxing", convar->getConVarByName("osu_letterboxing"));

	addSubSection("Detail Settings");
	addCheckbox("Snaking Sliders", convar->getConVarByName("osu_snaking_sliders"));

	//**************************************************************************************************************************//

	addSection("Audio");

	addSubSection("Volume");
	addSlider("Master:", 0.0f, 1.0f, convar->getConVarByName("osu_volume_master"), 70.0f);
	addSlider("Music:", 0.0f, 1.0f, convar->getConVarByName("osu_volume_music"), 70.0f);
	addSlider("Effects:", 0.0f, 1.0f, convar->getConVarByName("osu_volume_effects"), 70.0f);

	addSubSection("Offset Adjustment");
	addSlider("Global Offset:", -300.0f, 300.0f, convar->getConVarByName("osu_global_offset"))->setChangeCallback( MakeDelegate(this, &OsuOptionsMenu::onSliderChangeInt) );

	//**************************************************************************************************************************//

	addSection("Input");

	addSubSection("Mouse");
	addSlider("Sensitivity:", 0.4f, 6.0f, convar->getConVarByName("mouse_sensitivity"));
	addCheckbox("Raw input", convar->getConVarByName("mouse_raw_input"));
	addCheckbox("Confine Cursor (Windowed)", convar->getConVarByName("osu_confine_cursor_windowed"));
	addCheckbox("Confine Cursor (Fullscreen)", convar->getConVarByName("osu_confine_cursor_fullscreen"));
	addCheckbox("Disable Mouse Buttons in Play Mode", convar->getConVarByName("osu_disable_mousebuttons"));

	addSubSection("Keyboard");
	addButton("Left Click", &OsuKeyBindings::LEFT_CLICK)->setClickCallback( MakeDelegate(this, &OsuOptionsMenu::onKeyBindingButtonPressed) );
	addButton("Right Click", &OsuKeyBindings::RIGHT_CLICK)->setClickCallback( MakeDelegate(this, &OsuOptionsMenu::onKeyBindingButtonPressed) );
	addSpacer();
	addButton("Game Pause", &OsuKeyBindings::GAME_PAUSE)->setClickCallback( MakeDelegate(this, &OsuOptionsMenu::onKeyBindingButtonPressed) );
	addButton("Skip Cutscene", &OsuKeyBindings::SKIP_CUTSCENE)->setClickCallback( MakeDelegate(this, &OsuOptionsMenu::onKeyBindingButtonPressed) );
	addButton("Increase Local Song Offset", &OsuKeyBindings::INCREASE_LOCAL_OFFSET)->setClickCallback( MakeDelegate(this, &OsuOptionsMenu::onKeyBindingButtonPressed) );
	addButton("Decrease Local Song Offset", &OsuKeyBindings::DECREASE_LOCAL_OFFSET)->setClickCallback( MakeDelegate(this, &OsuOptionsMenu::onKeyBindingButtonPressed) );
	addButton("Quick Retry (hold briefly)", &OsuKeyBindings::QUICK_RETRY)->setClickCallback( MakeDelegate(this, &OsuOptionsMenu::onKeyBindingButtonPressed) );
	addSpacer();
	addButton("Increase Volume", &OsuKeyBindings::INCREASE_VOLUME)->setClickCallback( MakeDelegate(this, &OsuOptionsMenu::onKeyBindingButtonPressed) );
	addButton("Decrease Volume", &OsuKeyBindings::DECREASE_VOLUME)->setClickCallback( MakeDelegate(this, &OsuOptionsMenu::onKeyBindingButtonPressed) );

	//**************************************************************************************************************************//

	addSection("Skin");

	addSubSection("Skin");
	addSkinPreview();
	OPTIONS_ELEMENT skinSelect = addButton("Select Skin", "default");
	((CBaseUIButton*)skinSelect.elements[0])->setClickCallback( MakeDelegate(this, &OsuOptionsMenu::onSkinSelect) );
	m_skinLabel = (CBaseUILabel*)skinSelect.elements[1];
	m_skinSelectButton = skinSelect.elements[0];
	addButton("Reload Skin (CTRL+ALT+SHIFT+S)")->setClickCallback( MakeDelegate(this, &OsuOptionsMenu::onSkinReload) );
	addSpacer();
	addCheckbox("Ignore Beatmap Sample Volume", convar->getConVarByName("osu_ignore_beatmap_sample_volume"));
	addCheckbox("Ignore Beatmap Combo Colors", convar->getConVarByName("osu_ignore_beatmap_combo_colors"));
	addCheckbox("Load HD @2x", convar->getConVarByName("osu_skin_hd"));
	addSpacer();
	addCheckbox("Draw CursorTrail", convar->getConVarByName("osu_draw_cursor_trail"));
	m_cursorSizeSlider = addSlider("Cursor Size:", 0.01f, 5.0f, convar->getConVarByName("osu_cursor_scale"));
	m_cursorSizeSlider->setAnimated(false);
	addCheckbox("Use combo color as tint for slider ball", convar->getConVarByName("osu_slider_ball_tint_combo_color"));

	//**************************************************************************************************************************//

	addSection("Gameplay");

	addSubSection("General");
	addSlider("Background Dim:", 0.0f, 1.0f, convar->getConVarByName("osu_background_dim"))->setChangeCallback( MakeDelegate(this, &OsuOptionsMenu::onSliderChangePercent) );
	m_backgroundBrightnessSlider = addSlider("Background Brightness:", 0.0f, 1.0f, convar->getConVarByName("osu_background_brightness"));
	m_backgroundBrightnessSlider->setChangeCallback( MakeDelegate(this, &OsuOptionsMenu::onSliderChangePercent) );

	addSubSection("HUD");
	addCheckbox("Draw HUD", convar->getConVarByName("osu_draw_hud"));
	addCheckbox("Draw Combo", convar->getConVarByName("osu_draw_combo"));
	addCheckbox("Draw Accuracy", convar->getConVarByName("osu_draw_accuracy"));
	addCheckbox("Draw ProgressBar", convar->getConVarByName("osu_draw_progressbar"));
	addCheckbox("Draw HitErrorBar", convar->getConVarByName("osu_draw_hiterrorbar"));
	addCheckbox("Draw NPS and ND", convar->getConVarByName("osu_draw_statistics"));
	addSpacer();
	m_hudSizeSlider = addSlider("HUD Size:", 0.1f, 5.0f, convar->getConVarByName("osu_hud_scale"));

	addSubSection("Playfield");
	addCheckbox("Draw FollowPoints", convar->getConVarByName("osu_draw_followpoints"));
	addCheckbox("Draw Playfield Border", convar->getConVarByName("osu_draw_playfield_border"));
	addSpacer();
	m_playfieldBorderSizeSlider = addSlider("Playfield Border Size:", 0.0f, 500.0f, convar->getConVarByName("osu_hud_playfield_border_size"));
	m_playfieldBorderSizeSlider->setChangeCallback( MakeDelegate(this, &OsuOptionsMenu::onSliderChangeInt) );

	addSubSection("Hitobjects");
	addCheckbox("Draw Numbers", convar->getConVarByName("osu_draw_numbers"));
	addCheckbox("Draw ApproachCircles", convar->getConVarByName("osu_draw_approach_circles"));
	addCheckbox("Draw SliderEndCircle", convar->getConVarByName("osu_slider_draw_endcircle"));
	addCheckbox("Use New Hidden Fading Sliders", convar->getConVarByName("osu_hd_sliders_fade"));
	addCheckbox("Use Fast Hidden Fading Sliders (!)", convar->getConVarByName("osu_hd_sliders_fast_fade"));
	addCheckbox("Use Score V2 slider accuracy", convar->getConVarByName("osu_slider_scorev2"));

	//**************************************************************************************************************************//

	addSection("Bullshit");

	addSubSection("Why");
	addCheckbox("Rainbow ApproachCircles", convar->getConVarByName("osu_circle_rainbow"));
	addCheckbox("Rainbow Sliders", convar->getConVarByName("osu_slider_rainbow"));
	addCheckbox("SliderBreak Epilepsy", convar->getConVarByName("osu_slider_break_epilepsy"));
	addCheckbox("Shrinking Sliders", convar->getConVarByName("osu_slider_shrink"));



	// the context menu gets added last (drawn on top of everything)
	m_options->getContainer()->addBaseUIElement(m_contextMenu);
}

OsuOptionsMenu::~OsuOptionsMenu()
{
	SAFE_DELETE(m_container);
}

void OsuOptionsMenu::draw(Graphics *g)
{
	if (!m_bVisible) return;

	// interactive sliders

	if (m_backgroundBrightnessSlider->isActive())
	{
		short brightness = clamp<float>(m_backgroundBrightnessSlider->getFloat(), 0.0f, 1.0f)*255.0f;
		g->setColor(COLOR(255, brightness, brightness, brightness));
		g->fillRect(0, 0, Osu::getScreenWidth(), Osu::getScreenHeight());
	}

	m_container->draw(g);

	if (m_hudSizeSlider->isActive())
		m_osu->getHUD()->drawDummy(g);
	else if (m_playfieldBorderSizeSlider->isActive())
		m_osu->getHUD()->drawPlayfieldBorder(g, OsuGameRules::getPlayfieldCenter(), OsuGameRules::getPlayfieldSize(), 100);
	else
		m_backButton->draw(g);

	if (m_cursorSizeSlider->getFloat() < 0.15f)
		engine->getMouse()->drawDebug(g);
}

void OsuOptionsMenu::update()
{
	OsuScreenBackable::update();
	if (!m_bVisible) return;

	m_container->update();

	// force context menu focus
	if (m_contextMenu->isVisible())
	{
		std::vector<CBaseUIElement*> *options = m_options->getContainer()->getAllBaseUIElementsPointer();
		for (int i=0; i<options->size(); i++)
		{
			CBaseUIElement *e = ((*options)[i]);
			if (e == m_contextMenu)
				continue;

			e->stealFocus();
		}
	}

	if (m_fOsuFolderTextboxInvalidAnim > engine->getTime())
	{
		char redness = std::abs(std::sin((m_fOsuFolderTextboxInvalidAnim - engine->getTime())*3))*128;
		m_osuFolderTextbox->setBackgroundColor(COLOR(255, redness, 0, 0));
	}
	else
		m_osuFolderTextbox->setBackgroundColor(0xff000000);
}

void OsuOptionsMenu::onKeyDown(KeyboardEvent &e)
{
	if (!m_bVisible) return;

	m_container->onKeyDown(e);
	if (e.isConsumed()) return;

	if (e == KEY_ESCAPE || e == (KEYCODE)OsuKeyBindings::GAME_PAUSE.getInt())
	{
		if (m_contextMenu->isVisible())
			m_contextMenu->setVisible2(false);
		else
			onBack();
	}

	e.consume();
}

void OsuOptionsMenu::onKeyUp(KeyboardEvent &e)
{
	if (!m_bVisible) return;

	m_container->onKeyUp(e);
	if (e.isConsumed()) return;
}

void OsuOptionsMenu::onChar(KeyboardEvent &e)
{
	if (!m_bVisible) return;

	m_container->onChar(e);
	if (e.isConsumed()) return;
}

void OsuOptionsMenu::onResolutionChange(Vector2 newResolution)
{
	OsuScreenBackable::onResolutionChange(newResolution);

	m_resolutionLabel->setText(UString::format("%ix%i", (int)newResolution.x, (int)newResolution.y));
}

void OsuOptionsMenu::onKey(KeyboardEvent &e)
{
	// from OsuNotificationOverlay
	if (m_waitingKey != NULL)
	{
		m_waitingKey->setValue((float)e.getKeyCode());
		m_waitingKey = NULL;
	}
}

void OsuOptionsMenu::setVisible(bool visible)
{
	m_bVisible = visible;

	if (visible)
		updateLayout();
	else
		m_contextMenu->setVisible2(false);
}

void OsuOptionsMenu::updateLayout()
{
	// set all elements to the current convar values
	for (int i=0; i<m_elements.size(); i++)
	{
		switch (m_elements[i].type)
		{
		case 4: // checkbox
			if (m_elements[i].cvar != NULL)
			{
				for (int e=0; e<m_elements[i].elements.size(); e++)
				{
					CBaseUICheckbox *checkboxPointer = dynamic_cast<CBaseUICheckbox*>(m_elements[i].elements[e]);
					if (checkboxPointer != NULL)
						checkboxPointer->setChecked(m_elements[i].cvar->getBool());
				}
			}
			break;
		case 5: // slider
			if (m_elements[i].cvar != NULL)
			{
				if (m_elements[i].elements.size() == 3)
				{
					CBaseUISlider *sliderPointer = dynamic_cast<CBaseUISlider*>(m_elements[i].elements[1]);
					if (sliderPointer != NULL)
					{
						sliderPointer->setValue(m_elements[i].cvar->getFloat(), false);
						sliderPointer->forceCallCallback();
					}
				}
			}
			break;
		case 6: // textbox
			if (m_elements[i].cvar != NULL)
			{
				if (m_elements[i].elements.size() == 1)
				{
					CBaseUITextbox *textboxPointer = dynamic_cast<CBaseUITextbox*>(m_elements[i].elements[0]);
					if (textboxPointer != NULL)
						textboxPointer->setText(m_elements[i].cvar->getString());
				}
			}
			break;
		}
	}

	m_fullscreenCheckbox->setChecked(env->isFullscreen(), false);

	// HACKHACK: temp, not correct anymore if skin loading fails
	m_skinLabel->setText(convar->getConVarByName("osu_skin")->getString());

	//************************************************************************************************************************************//

	OsuScreenBackable::updateLayout();

	m_container->setSize(Osu::getScreenSize());

	// options panel
	int optionsWidth = (int)(Osu::getScreenWidth()*0.5f);
	m_options->setRelPosX(Osu::getScreenWidth()/2 - optionsWidth/2);
	m_options->setSize(optionsWidth, Osu::getScreenHeight()+1);

	bool enableHorizontalScrolling = false;
	int sideMargin = 25;
	int spaceSpacing = 25;
	int sectionSpacing = -15; // section title to first element
	int subsectionSpacing = 15; // subsection title to first element
	int sectionEndSpacing = 70; // last section element to next section title
	int subsectionEndSpacing = 65; // last subsection element to next subsection title
	int elementSpacing = 5;
	int yCounter = sideMargin;
	for (int i=0; i<m_elements.size(); i++)
	{
		int elementWidth = optionsWidth - 2*sideMargin - 2;

		if (m_elements[i].elements.size() == 1)
		{
			CBaseUIElement *e = m_elements[i].elements[0];

			CBaseUICheckbox *checkboxPointer = dynamic_cast<CBaseUICheckbox*>(e);
			if (checkboxPointer != NULL)
			{
				checkboxPointer->setWidthToContent(0);
				if (checkboxPointer->getSize().x > elementWidth)
					enableHorizontalScrolling = true;
				else
					e->setSizeX(elementWidth);
			}
			else
				e->setSizeX(elementWidth);

			e->setRelPosX(sideMargin);
			e->setRelPosY(yCounter);

			yCounter += e->getSize().y;
		}
		else if (m_elements[i].elements.size() == 2)
		{
			CBaseUIElement *e1 = m_elements[i].elements[0];
			CBaseUIElement *e2 = m_elements[i].elements[1];

			int spacing = 15;

			e1->setRelPos(sideMargin, yCounter);
			e1->setSizeX(elementWidth/2 - spacing);

			e2->setRelPos(sideMargin + e1->getSize().x + 2*spacing, yCounter);
			e2->setSizeX(elementWidth/2 - spacing);

			yCounter += e1->getSize().y;
		}
		else if (m_elements[i].elements.size() == 3)
		{
			CBaseUIElement *e1 = m_elements[i].elements[0];
			CBaseUIElement *e2 = m_elements[i].elements[1];
			CBaseUIElement *e3 = m_elements[i].elements[2];

			int offset = 11;
			int labelSliderLabelOffset = 15;
			int sliderSize = elementWidth - e1->getSize().x - e3->getSize().x;

			if (sliderSize < 100)
			{
				enableHorizontalScrolling = true;
				sliderSize = 100;
			}

			e1->setRelPos(sideMargin + offset, yCounter);

			e2->setRelPos(sideMargin + offset + e1->getSize().x + labelSliderLabelOffset, yCounter);
			e2->setSizeX(sliderSize - 2*offset - labelSliderLabelOffset*2);

			e3->setRelPos(sideMargin + offset + e1->getSize().x + e2->getSize().x + 2*labelSliderLabelOffset, yCounter);
			///e3->setSizeX(valueLabelSize - valueLabelOffset);

			yCounter += e2->getSize().y;
		}

		switch (m_elements[i].type)
		{
		case 0:
			yCounter += spaceSpacing;
			break;
		case 1:
			yCounter += sectionSpacing;
			break;
		case 2:
			yCounter += subsectionSpacing;
			break;
		default:
			break;
		}

		yCounter += elementSpacing;

		// if the next element is a new section, add even more spacing
		if (i+1 < m_elements.size() && m_elements[i+1].type == 1)
			yCounter += sectionEndSpacing;

		// if the next element is a new subsection, add even more spacing
		if (i+1 < m_elements.size() && m_elements[i+1].type == 2)
			yCounter += subsectionEndSpacing;
	}
	m_options->setScrollSizeToContent();
	m_options->setHorizontalScrolling(enableHorizontalScrolling);

	m_container->update_pos();
}

void OsuOptionsMenu::updateOsuFolder()
{
	// automatically insert a slash at the end if the user forgets
	// i don't know how i feel about this code
	UString newOsuFolder = m_osuFolderTextbox->getText();
	newOsuFolder = newOsuFolder.trim();
	if (newOsuFolder.length() > 0)
	{
		const char *utf8folder = newOsuFolder.toUtf8();
		if (utf8folder[newOsuFolder.length()-1] != '/' && utf8folder[newOsuFolder.length()-1] != '\\')
		{
			newOsuFolder.append("/");
			m_osuFolderTextbox->setText(newOsuFolder);
		}
	}

	// set convar to new folder
	convar->getConVarByName("osu_folder")->setValue(newOsuFolder);
}

void OsuOptionsMenu::onBack()
{
	engine->getSound()->play(m_osu->getSkin()->getMenuClick());
	m_osu->toggleOptionsMenu();

	save();
}

void OsuOptionsMenu::onFullscreenChange(CBaseUICheckbox *checkbox)
{
	if (checkbox->isChecked())
		env->enableFullscreen();
	else
		env->disableFullscreen();
}

void OsuOptionsMenu::onSkinSelect()
{
	updateOsuFolder();

	UString skinFolder = convar->getConVarByName("osu_folder")->getString();
	skinFolder.append("Skins/");
	std::vector<UString> skinFolders = env->getFoldersInFolder(skinFolder);

	if (skinFolders.size() > 0)
	{
		m_contextMenu->setPos(m_skinSelectButton->getPos());
		m_contextMenu->setRelPos(m_skinSelectButton->getRelPos());
		m_contextMenu->begin();
		m_contextMenu->addButton("default");
		for (int i=0; i<skinFolders.size(); i++)
		{
			if (skinFolders[i] == "." || skinFolders[i] == "..") // is this universal in every file system? too lazy to check. should probably fix this in the engine and not here
				continue;

			m_contextMenu->addButton(skinFolders[i]);
		}
		m_contextMenu->end();
		m_contextMenu->setClickCallback( MakeDelegate(this, &OsuOptionsMenu::onSkinSelect2) );
	}
	else
	{
		m_osu->getNotificationOverlay()->addNotification("Error: Couldn't find any skins", 0xffff0000);
		m_options->scrollToTop();
		m_fOsuFolderTextboxInvalidAnim = engine->getTime() + 3.0f;
	}
}

void OsuOptionsMenu::onSkinSelect2(UString skinName)
{
	convar->getConVarByName("osu_skin")->setValue(skinName);
	m_skinLabel->setText(skinName);
}

void OsuOptionsMenu::onSkinReload()
{
	m_osu->reloadSkin();
}

void OsuOptionsMenu::onResolutionSelect()
{
	std::vector<Vector2> resolutions;

	// 4:3
    resolutions.push_back(Vector2(1024, 768));
    resolutions.push_back(Vector2(1152, 864));
    resolutions.push_back(Vector2(1280, 960));
    resolutions.push_back(Vector2(1280, 1024));
    resolutions.push_back(Vector2(1600, 1200));
    resolutions.push_back(Vector2(1920, 1440));
    resolutions.push_back(Vector2(2560, 1920));

    // 16:9 and 16:10
    resolutions.push_back(Vector2(1024, 600));
    resolutions.push_back(Vector2(1280, 720));
    resolutions.push_back(Vector2(1280, 768));
    resolutions.push_back(Vector2(1280, 800));
    resolutions.push_back(Vector2(1360, 768));
    resolutions.push_back(Vector2(1366, 768));
    resolutions.push_back(Vector2(1440, 900));
    resolutions.push_back(Vector2(1600, 900));
    resolutions.push_back(Vector2(1600, 1024));
    resolutions.push_back(Vector2(1680, 1050));
    resolutions.push_back(Vector2(1920, 1080));
    resolutions.push_back(Vector2(1920, 1200));
    resolutions.push_back(Vector2(2560, 1440));
    resolutions.push_back(Vector2(2560, 1600));
    resolutions.push_back(Vector2(3840, 2160));
    resolutions.push_back(Vector2(5120, 2880));
    resolutions.push_back(Vector2(7680, 4320));

    // wtf
    resolutions.push_back(Vector2(4096, 2160));

    // native resolution at the end
    Vector2 nativeResolution = env->getNativeScreenSize();
    bool containsNativeResolution = false;
    for (int i=0; i<resolutions.size(); i++)
    {
    	if (resolutions[i] == nativeResolution)
    	{
    		containsNativeResolution = true;
    		break;
    	}
    }
    if (!containsNativeResolution)
    	resolutions.push_back(nativeResolution);

    // build context menu
	m_contextMenu->setPos(m_resolutionSelectButton->getPos());
	m_contextMenu->setRelPos(m_resolutionSelectButton->getRelPos());
	m_contextMenu->begin();
	for (int i=0; i<resolutions.size(); i++)
	{
		if (resolutions[i].x > nativeResolution.x || resolutions[i].y > nativeResolution.y)
			continue;

		m_contextMenu->addButton(UString::format("%ix%i", (int)resolutions[i].x, (int)resolutions[i].y));
	}
	m_contextMenu->end();
	m_contextMenu->setClickCallback( MakeDelegate(this, &OsuOptionsMenu::onResolutionSelect2) );
}

void OsuOptionsMenu::onResolutionSelect2(UString resolution)
{
	if (env->isFullscreen())
		convar->getConVarByName("osu_resolution")->setValue(resolution);
	else
		convar->getConVarByName("windowed")->exec_arg(resolution);
}

void OsuOptionsMenu::onCheckboxChange(CBaseUICheckbox *checkbox)
{
	for (int i=0; i<m_elements.size(); i++)
	{
		for (int e=0; e<m_elements[i].elements.size(); e++)
		{
			if (m_elements[i].elements[e] == checkbox)
			{
				if (m_elements[i].cvar != NULL)
					m_elements[i].cvar->setValue(checkbox->isChecked());
				break;
			}
		}
	}
}

void OsuOptionsMenu::onSliderChange(CBaseUISlider *slider)
{
	for (int i=0; i<m_elements.size(); i++)
	{
		for (int e=0; e<m_elements[i].elements.size(); e++)
		{
			if (m_elements[i].elements[e] == slider)
			{
				if (m_elements[i].cvar != NULL)
					m_elements[i].cvar->setValue(std::round(slider->getFloat()*100.0f)/100.0f); // round to 2 decimal places

				if (m_elements[i].elements.size() == 3)
				{
					CBaseUILabel *labelPointer = dynamic_cast<CBaseUILabel*>(m_elements[i].elements[2]);
					labelPointer->setText(m_elements[i].cvar->getString());
				}
				break;
			}
		}
	}
}

void OsuOptionsMenu::onSliderChangeInt(CBaseUISlider *slider)
{
	for (int i=0; i<m_elements.size(); i++)
	{
		for (int e=0; e<m_elements[i].elements.size(); e++)
		{
			if (m_elements[i].elements[e] == slider)
			{
				if (m_elements[i].cvar != NULL)
					m_elements[i].cvar->setValue(std::round(slider->getFloat())); // round to int

				if (m_elements[i].elements.size() == 3)
				{
					CBaseUILabel *labelPointer = dynamic_cast<CBaseUILabel*>(m_elements[i].elements[2]);
					labelPointer->setText(m_elements[i].cvar->getString());
				}
				break;
			}
		}
	}
}

void OsuOptionsMenu::onSliderChangePercent(CBaseUISlider *slider)
{
	for (int i=0; i<m_elements.size(); i++)
	{
		for (int e=0; e<m_elements[i].elements.size(); e++)
		{
			if (m_elements[i].elements[e] == slider)
			{
				if (m_elements[i].cvar != NULL)
					m_elements[i].cvar->setValue(std::round(slider->getFloat()*100.0f)/100.0f);

				if (m_elements[i].elements.size() == 3)
				{
					int percent = std::round(m_elements[i].cvar->getFloat()*100.0f);

					CBaseUILabel *labelPointer = dynamic_cast<CBaseUILabel*>(m_elements[i].elements[2]);
					labelPointer->setText(UString::format("%i%%", percent));
				}
				break;
			}
		}
	}
}

void OsuOptionsMenu::onKeyBindingButtonPressed(CBaseUIButton *button)
{
	for (int i=0; i<m_elements.size(); i++)
	{
		for (int e=0; e<m_elements[i].elements.size(); e++)
		{
			if (m_elements[i].elements[e] == button)
			{
				if (m_elements[i].cvar != NULL)
				{
					m_waitingKey = m_elements[i].cvar;

					UString notificationText = "Press new key for ";
					notificationText.append(button->getText());
					notificationText.append(":");
					m_osu->getNotificationOverlay()->addNotification(notificationText, 0xffffffff, true);
				}
				break;
			}
		}
	}
}

void OsuOptionsMenu::addSpacer()
{
	OPTIONS_ELEMENT e;
	e.type = 0;
	e.cvar = NULL;
	m_elements.push_back(e);
}

CBaseUILabel *OsuOptionsMenu::addSection(UString text)
{
	CBaseUILabel *label = new CBaseUILabel(0, 0, m_options->getSize().x, 25, text, text);
	label->setFont(m_osu->getTitleFont());
	label->setSizeToContent(0, 0);
	label->setTextJustification(CBaseUILabel::TEXT_JUSTIFICATION_RIGHT);
	label->setDrawFrame(false);
	label->setDrawBackground(false);
	m_options->getContainer()->addBaseUIElement(label);

	OPTIONS_ELEMENT e;
	e.elements.push_back(label);
	e.type = 1;
	e.cvar = NULL;
	m_elements.push_back(e);

	return label;
}

CBaseUILabel *OsuOptionsMenu::addSubSection(UString text)
{
	CBaseUILabel *label = new CBaseUILabel(0, 0, m_options->getSize().x, 25, text, text);
	label->setFont(m_osu->getSubTitleFont());
	label->setSizeToContent(0, 0);
	label->setDrawFrame(false);
	label->setDrawBackground(false);
	m_options->getContainer()->addBaseUIElement(label);

	OPTIONS_ELEMENT e;
	e.elements.push_back(label);
	e.type = 2;
	e.cvar = NULL;
	m_elements.push_back(e);

	return label;
}

OsuUIButton *OsuOptionsMenu::addButton(UString text, ConVar *cvar)
{
	OsuUIButton *button = new OsuUIButton(m_osu, 0, 0, m_options->getSize().x, 50, text, text);
	button->setColor(0xff0e94b5);
	button->setUseDefaultSkin();
	m_options->getContainer()->addBaseUIElement(button);

	OPTIONS_ELEMENT e;
	e.elements.push_back(button);
	e.type = 3;
	e.cvar = cvar;
	m_elements.push_back(e);

	return button;
}

OsuOptionsMenu::OPTIONS_ELEMENT OsuOptionsMenu::addButton(UString text, UString labelText)
{
	OsuUIButton *button = new OsuUIButton(m_osu, 0, 0, m_options->getSize().x, 50, text, text);
	button->setColor(0xff0e94b5);
	button->setUseDefaultSkin();
	m_options->getContainer()->addBaseUIElement(button);

	CBaseUILabel *label = new CBaseUILabel(0, 0, m_options->getSize().x, 50, "", labelText);
	label->setDrawFrame(false);
	label->setDrawBackground(false);
	m_options->getContainer()->addBaseUIElement(label);

	OPTIONS_ELEMENT e;
	e.elements.push_back(button);
	e.elements.push_back(label);
	e.type = 3;
	e.cvar = NULL;
	m_elements.push_back(e);

	return e;
}

CBaseUICheckbox *OsuOptionsMenu::addCheckbox(UString text, ConVar *cvar)
{
	CBaseUICheckbox *checkbox = new CBaseUICheckbox(0, 0, m_options->getSize().x, 50, text, text);
	checkbox->setDrawFrame(false);
	checkbox->setDrawBackground(false);
	if (cvar != NULL)
	{
		checkbox->setChecked(cvar->getBool());
		checkbox->setChangeCallback( MakeDelegate(this, &OsuOptionsMenu::onCheckboxChange) );
	}
	m_options->getContainer()->addBaseUIElement(checkbox);

	OPTIONS_ELEMENT e;
	e.elements.push_back(checkbox);
	e.type = 4;
	e.cvar = cvar;
	m_elements.push_back(e);

	return checkbox;
}

OsuUISlider *OsuOptionsMenu::addSlider(UString text, float min, float max, ConVar *cvar, float label1Width)
{
	OsuUISlider *slider = new OsuUISlider(m_osu, 0, 0, 100, 40, text);
	slider->setAllowMouseWheel(false);
	slider->setBounds(min, max);
	slider->setLiveUpdate(true);
	if (cvar != NULL)
	{
		slider->setValue(cvar->getFloat(), false);
		slider->setChangeCallback( MakeDelegate(this, &OsuOptionsMenu::onSliderChange) );
	}
	m_options->getContainer()->addBaseUIElement(slider);

	CBaseUILabel *label1 = new CBaseUILabel(0, 0, m_options->getSize().x, 40, text, text);
	label1->setDrawFrame(false);
	label1->setDrawBackground(false);
	label1->setWidthToContent();
	if (label1Width > 1)
		label1->setSizeX(label1Width);
	m_options->getContainer()->addBaseUIElement(label1);

	CBaseUILabel *label2 = new CBaseUILabel(0, 0, m_options->getSize().x, 40, "", "8.81");
	label2->setDrawFrame(false);
	label2->setDrawBackground(false);
	label2->setWidthToContent();
	m_options->getContainer()->addBaseUIElement(label2);

	OPTIONS_ELEMENT e;
	e.elements.push_back(label1);
	e.elements.push_back(slider);
	e.elements.push_back(label2);
	e.type = 5;
	e.cvar = cvar;
	m_elements.push_back(e);

	return slider;
}

CBaseUITextbox *OsuOptionsMenu::addTextbox(UString text, ConVar *cvar)
{
	CBaseUITextbox *textbox = new CBaseUITextbox(0, 0, m_options->getSize().x, 40, "");
	textbox->setText(text);
	m_options->getContainer()->addBaseUIElement(textbox);

	OPTIONS_ELEMENT e;
	e.elements.push_back(textbox);
	e.type = 6;
	e.cvar = cvar;
	m_elements.push_back(e);

	return textbox;
}

CBaseUIElement *OsuOptionsMenu::addSkinPreview()
{
	CBaseUIElement *skinPreview = new OsuOptionsMenuSkinPreviewElement(m_osu, 0, 0, 0, 200, "");
	m_options->getContainer()->addBaseUIElement(skinPreview);

	OPTIONS_ELEMENT e;
	e.elements.push_back(skinPreview);
	e.type = 7;
	e.cvar = NULL;
	m_elements.push_back(e);

	return skinPreview;
}

void OsuOptionsMenu::save()
{
	if (!osu_options_save_on_back.getBool())
	{
		debugLog("DEACTIVATED SAVE!!!!\n");
		return;
	}
	updateOsuFolder();

	debugLog("Osu: Saving user config file ...\n");

	const char *userConfigFile = "cfg/osu.cfg";

	// manual concommands (e.g. fullscreen, windowed, osu_resolution)
	std::vector<ConVar*> manualConCommands;
	std::vector<ConVar*> manualConVars;
	std::vector<ConVar*> removeConCommands;

	removeConCommands.push_back(convar->getConVarByName("windowed"));
	removeConCommands.push_back(convar->getConVarByName("osu_skin"));

	if (m_fullscreenCheckbox->isChecked())
	{
		manualConCommands.push_back(convar->getConVarByName("fullscreen"));
		if (convar->getConVarByName("osu_resolution_enabled")->getBool())
			manualConVars.push_back(convar->getConVarByName("osu_resolution"));
		else
			removeConCommands.push_back(convar->getConVarByName("osu_resolution"));
	}
	else
	{
		removeConCommands.push_back(convar->getConVarByName("fullscreen"));
		removeConCommands.push_back(convar->getConVarByName("osu_resolution"));
	}

	// get user stuff in the config file
	std::vector<UString> keepLines;
	std::ifstream in(userConfigFile);
	if (!in.good())
		debugLog("Osu Error: Couldn't read user config file!");
	else
	{
		while (in.good())
		{
			std::string line;
			std::getline(in, line);

			bool keepLine = true;
			for (int i=0; i<m_elements.size(); i++)
			{
				if (m_elements[i].cvar != NULL && line.find(m_elements[i].cvar->getName().toUtf8()) != std::string::npos)
				{
					keepLine = false;
					break;
				}
			}

			for (int i=0; i<manualConCommands.size(); i++)
			{
				if (line.find(manualConCommands[i]->getName().toUtf8()) != std::string::npos)
				{
					keepLine = false;
					break;
				}
			}

			for (int i=0; i<manualConVars.size(); i++)
			{
				if (line.find(manualConVars[i]->getName().toUtf8()) != std::string::npos)
				{
					keepLine = false;
					break;
				}
			}

			for (int i=0; i<removeConCommands.size(); i++)
			{
				if (line.find(removeConCommands[i]->getName().toUtf8()) != std::string::npos)
				{
					keepLine = false;
					break;
				}
			}

			if (keepLine && line.size() > 0)
				keepLines.push_back(line.c_str());
		}

		in.close();
	}

	// write new config file
	std::ofstream out(userConfigFile);
	if (!out.good())
	{
		engine->showMessageError("Osu Error", "Couldn't write user config file!");
		return;
	}

	// write user stuff back
	for (int i=0; i<keepLines.size(); i++)
	{
		out << keepLines[i].toUtf8() << "\n";
	}
	out << "\n";

	// write manual convars
	for (int i=0; i<manualConCommands.size(); i++)
	{
		out << manualConCommands[i]->getName().toUtf8() << "\n";
	}
	for (int i=0; i<manualConVars.size(); i++)
	{
		out << manualConVars[i]->getName().toUtf8() << " " << manualConVars[i]->getString().toUtf8() <<"\n";
	}

	// hardcoded (!)
	if (!m_fullscreenCheckbox->isChecked())
		out << "windowed " << engine->getScreenWidth() << "x" << engine->getScreenHeight() << "\n";

	// write options elements convars
	for (int i=0; i<m_elements.size(); i++)
	{
		if (m_elements[i].cvar != NULL)
		{
			out << m_elements[i].cvar->getName().toUtf8() << " " << m_elements[i].cvar->getString().toUtf8() << "\n";
		}
	}

	out << "osu_skin " << convar->getConVarByName("osu_skin")->getString().toUtf8() << "\n";

	out.close();
}



OsuUIListBoxContextMenu::OsuUIListBoxContextMenu(float xPos, float yPos, float xSize, float ySize, UString name) : CBaseUIElement(xPos, yPos, xSize, ySize, name)
{
	m_container = new CBaseUIContainer(xPos, yPos, xSize, ySize, name);
	m_iYCounter = 0;
	m_iWidthCounter = 0;

	m_bVisible = false;
	m_bVisible2 = false;
	m_clickCallback = NULL;
}

OsuUIListBoxContextMenu::~OsuUIListBoxContextMenu()
{
	SAFE_DELETE(m_container);
}

void OsuUIListBoxContextMenu::draw(Graphics *g)
{
	if (!m_bVisible || !m_bVisible2) return;

	// draw background
	g->setColor(0xff222222);
	g->fillRect(m_vPos.x+1, m_vPos.y+1, m_vSize.x-1, m_vSize.y-1);

	// draw frame
	g->setColor(0xffffffff);
	g->drawRect(m_vPos.x, m_vPos.y, m_vSize.x, m_vSize.y);

	m_container->draw(g);
}

void OsuUIListBoxContextMenu::update()
{
	CBaseUIElement::update();
	if (!m_bVisible || !m_bVisible2) return;

	m_container->update();
}

void OsuUIListBoxContextMenu::onResized()
{
	m_container->setSize(m_vSize);
}

void OsuUIListBoxContextMenu::onMoved()
{
	m_container->setPos(m_vPos);
}

void OsuUIListBoxContextMenu::onMouseDownOutside()
{
	setVisible2(false);
}

void OsuUIListBoxContextMenu::onFocusStolen()
{
	m_container->stealFocus();
}

void OsuUIListBoxContextMenu::begin()
{
	m_iYCounter = 0;
	m_iWidthCounter = 0;
	m_container->clear();
}

void OsuUIListBoxContextMenu::addButton(UString text)
{
	int buttonHeight = 30;
	int margin = 9;

	CBaseUIButton *button = new CBaseUIButton(margin, m_iYCounter + margin, 0, buttonHeight, text, text);
	button->setClickCallback( MakeDelegate(this, &OsuUIListBoxContextMenu::onClick) );
	button->setWidthToContent(3);
	button->setTextLeft(true);
	button->setDrawFrame(false);
	button->setDrawBackground(false);
	m_container->addBaseUIElement(button);

	if (button->getSize().x+2*margin > m_iWidthCounter)
	{
		m_iWidthCounter = button->getSize().x + 2*margin;
		setSizeX(m_iWidthCounter);
	}

	m_iYCounter += buttonHeight;
	setSizeY(m_iYCounter + 2*margin);
}

void OsuUIListBoxContextMenu::end()
{
	int margin = 9;

	std::vector<CBaseUIElement*> *elements = m_container->getAllBaseUIElementsPointer();
	for (int i=0; i<elements->size(); i++)
	{
		((*elements)[i])->setSizeX(m_iWidthCounter-2*margin);
	}

	setVisible2(true);
}

void OsuUIListBoxContextMenu::onClick(CBaseUIButton *button)
{
	setVisible2(false);
	if (m_clickCallback != NULL)
		m_clickCallback(button->getName());
}
