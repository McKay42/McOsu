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
#include "AnimationHandler.h"
#include "OpenVRInterface.h"
#include "OpenVRController.h"
#include "SteamworksInterface.h"
#include "Mouse.h"
#include "File.h"

#include "CBaseUIContainer.h"
#include "CBaseUIScrollView.h"
#include "CBaseUILabel.h"
#include "CBaseUICheckbox.h"
#include "CBaseUITextbox.h"

#include "Osu.h"
#include "OsuVR.h"
#include "OsuHUD.h"
#include "OsuSkin.h"
#include "OsuGameRules.h"
#include "OsuKeyBindings.h"
#include "OsuTooltipOverlay.h"
#include "OsuNotificationOverlay.h"
#include "OsuSteamWorkshop.h"
#include "OsuMainMenu.h"
#include "OsuSliderRenderer.h"
#include "OsuCircle.h"
#include "OsuIcons.h"

#include "OsuUIButton.h"
#include "OsuUISlider.h"
#include "OsuUICheckbox.h"
#include "OsuUIBackButton.h"
#include "OsuUIContextMenu.h"
#include "OsuUISearchOverlay.h"

#include <iostream>
#include <fstream>

#include "OpenGLHeaders.h"
#include "OpenGLLegacyInterface.h"
#include "OpenGL3Interface.h"
#include "OpenGLES2Interface.h"

ConVar osu_options_save_on_back("osu_options_save_on_back", true);
ConVar osu_options_high_quality_sliders("osu_options_high_quality_sliders", false);
ConVar osu_mania_keylayout_wizard("osu_mania_keylayout_wizard");

void _osuOptionsSliderQualityWrapper(UString oldValue, UString newValue)
{
	float value = lerp<float>(1.0f, 2.5f, 1.0f - newValue.toFloat());
	convar->getConVarByName("osu_slider_curve_points_separation")->setValue(value);
};
ConVar osu_options_slider_quality("osu_options_slider_quality", 0.0f, _osuOptionsSliderQualityWrapper);

const char *OsuOptionsMenu::OSU_CONFIG_FILE_NAME = ""; // set dynamically below in the constructor



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
			float approachCircleAlpha = approachAlpha;
			approachAlpha = 1.0f;

			OsuCircle::drawCircle(g, m_osu->getSkin(), m_vPos + Vector2(0, m_vSize.y/2) + Vector2(m_vSize.x*(1.0f/5.0f), 0.0f), hitcircleDiameter, numberScale, overlapScale, 1, 42, 0, approachScale, approachAlpha, approachAlpha, true, false);
			OsuCircle::drawHitResult(g, m_osu->getSkin(), hitcircleDiameter, hitcircleDiameter, m_vPos + Vector2(0, m_vSize.y/2) + Vector2(m_vSize.x*(2.0f/5.0f), 0.0f), OsuScore::HIT::HIT_100, 1.0f, 1.0f);
			OsuCircle::drawHitResult(g, m_osu->getSkin(), hitcircleDiameter, hitcircleDiameter, m_vPos + Vector2(0, m_vSize.y/2) + Vector2(m_vSize.x*(3.0f/5.0f), 0.0f), OsuScore::HIT::HIT_50, 1.0f, 1.0f);
			OsuCircle::drawHitResult(g, m_osu->getSkin(), hitcircleDiameter, hitcircleDiameter, m_vPos + Vector2(0, m_vSize.y/2) + Vector2(m_vSize.x*(4.0f/5.0f), 0.0f), OsuScore::HIT::HIT_MISS, 1.0f, 1.0f);
			OsuCircle::drawApproachCircle(g, m_osu->getSkin(), m_vPos + Vector2(0, m_vSize.y/2) + Vector2(m_vSize.x*(1.0f/5.0f), 0.0f), m_osu->getSkin()->getComboColorForCounter(42, 0), hitcircleDiameter, approachScale, approachCircleAlpha, false, false);
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
				m_osu->getHUD()->drawScoreNumber(g, i-1, 1.0f);
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

class OsuOptionsMenuSliderPreviewElement : public CBaseUIElement
{
public:
	OsuOptionsMenuSliderPreviewElement(Osu *osu, float xPos, float yPos, float xSize, float ySize, UString name) : CBaseUIElement(xPos, yPos, xSize, ySize, name)
	{
		m_osu = osu;
		m_bDrawSliderHack = true;
	}

	virtual void draw(Graphics *g)
	{
		/*
		g->setColor(0xffffffff);
		g->drawRect(m_vPos.x, m_vPos.y, m_vSize.x, m_vSize.y);
		*/

		const float hitcircleDiameter = m_vSize.y*0.5f;
		const float numberScale = (hitcircleDiameter / (160.0f * (m_osu->getSkin()->isDefault12x() ? 2.0f : 1.0f))) * 1 * convar->getConVarByName("osu_number_scale_multiplier")->getFloat();
		const float overlapScale = (hitcircleDiameter / (160.0f)) * 1 * convar->getConVarByName("osu_number_scale_multiplier")->getFloat();

		float approachScale = clamp<float>(1.0f + 1.5f - fmod(engine->getTime()*3, 3.0f), 0.0f, 2.5f);
		float approachAlpha = clamp<float>(fmod(engine->getTime()*3, 3.0f)/1.5f, 0.0f, 1.0f);

		approachAlpha = -approachAlpha*(approachAlpha-2.0f);
		approachAlpha = -approachAlpha*(approachAlpha-2.0f);

		float approachCircleAlpha = approachAlpha;
		approachAlpha = 1.0f;

		const float length = (m_vSize.x-hitcircleDiameter);
		const int numPoints = length;
		const float pointDist = length / numPoints;

		std::vector<Vector2> emptyVector;
		std::vector<Vector2> points;

		for (int i=0; i<numPoints; i++)
		{
			int heightAdd = i;
			if (i > numPoints/2)
				heightAdd = numPoints-i;

			float heightAddPercent = (float)heightAdd / (float)(numPoints/2.0f);
			float temp = 1.0f - heightAddPercent;
			temp *= temp;
			heightAddPercent = 1.0f - temp;

			points.push_back(Vector2(m_vPos.x + hitcircleDiameter/2 + i*pointDist, m_vPos.y + m_vSize.y/2 - hitcircleDiameter/3 + heightAddPercent*(m_vSize.y/2 - hitcircleDiameter/2)));
		}

		if (points.size() > 0)
		{
			OsuCircle::drawCircle(g, m_osu->getSkin(), points[numPoints/2], hitcircleDiameter, numberScale, overlapScale, 2, 420, 0, approachScale, approachAlpha, approachAlpha, true, false);
			OsuCircle::drawApproachCircle(g, m_osu->getSkin(), points[numPoints/2], m_osu->getSkin()->getComboColorForCounter(420, 0), hitcircleDiameter, approachScale, approachCircleAlpha, false, false);
			{
				// recursive shared usage of the same RenderTarget is invalid, therefore we block slider rendering while the options menu is animating
				if (m_bDrawSliderHack)
					OsuSliderRenderer::draw(g, m_osu, points, emptyVector, hitcircleDiameter, 0, 1, m_osu->getSkin()->getComboColorForCounter(420, 0));
			}
			OsuCircle::drawSliderStartCircle(g, m_osu->getSkin(), points[0], hitcircleDiameter, numberScale, overlapScale, 1, 420, 0);
			OsuCircle::drawSliderEndCircle(g, m_osu->getSkin(), points[points.size()-1], hitcircleDiameter, numberScale, overlapScale, 0, 0, 0, 1.0f, 1.0f, 0.0f, false, false);
		}
	}

	void setDrawSliderHack(bool drawSliderHack) {m_bDrawSliderHack = drawSliderHack;}

private:
	Osu *m_osu;
	bool m_bDrawSliderHack;
};

class OsuOptionsMenuKeyBindLabel : public CBaseUILabel
{
public:
	OsuOptionsMenuKeyBindLabel(float xPos, float yPos, float xSize, float ySize, UString name, UString text, ConVar *cvar) : CBaseUILabel(xPos, yPos, xSize, ySize, name, text)
	{
		m_key = cvar;

		m_textColorBound = 0xffffd700;
		m_textColorUnbound = 0xffbb0000;
	}

	virtual void update()
	{
		CBaseUILabel::update();
		if (!m_bVisible) return;

		const KEYCODE keyCode = (KEYCODE)m_key->getInt();

		// succ
		UString labelText = env->keyCodeToString(keyCode);
		if (labelText.find("?") != -1)
			labelText.append(UString::format("  (%i)", m_key->getInt()));

		// handle bound/unbound
		if (keyCode == 0)
		{
			labelText = "<UNBOUND>";
			setTextColor(m_textColorUnbound);
		}
		else
			setTextColor(m_textColorBound);

		// update text
		setText(labelText);
	}

	void setTextColorBound(Color textColorBound) {m_textColorBound = textColorBound;}
	void setTextColorUnbound(Color textColorUnbound) {m_textColorUnbound = textColorUnbound;}

private:
	ConVar *m_key;

	Color m_textColorBound;
	Color m_textColorUnbound;
};

class OsuOptionsMenuCategoryButton : public CBaseUIButton
{
public:
	OsuOptionsMenuCategoryButton(CBaseUIElement *section, float xPos, float yPos, float xSize, float ySize, UString name, UString text) : CBaseUIButton(xPos, yPos, xSize, ySize, name, text)
	{
		m_section = section;
		m_bActiveCategory = false;
	}

	virtual void drawText(Graphics *g)
	{
		if (m_font != NULL && m_sText.length() > 0)
		{
			g->pushClipRect(McRect(m_vPos.x+1, m_vPos.y+1, m_vSize.x-1, m_vSize.y-1));
			{
				g->setColor(m_textColor);
				g->pushTransform();
				{
					g->translate((int)(m_vPos.x + m_vSize.x/2.0f - (m_fStringWidth/2.0f)), (int)(m_vPos.y + m_vSize.y/2.0f + (m_fStringHeight/2.0f)));
					g->drawString(m_font, m_sText);
				}
				g->popTransform();
			}
			g->popClipRect();
		}
	}

	void setActiveCategory(bool activeCategory) {m_bActiveCategory = activeCategory;}

	inline CBaseUIElement *getSection() const {return m_section;}
	inline bool isActiveCategory() const {return m_bActiveCategory;}

private:
	CBaseUIElement *m_section;
	bool m_bActiveCategory;
};

class OsuOptionsMenuResetButton : public CBaseUIButton
{
public:
	OsuOptionsMenuResetButton(Osu *osu, float xPos, float yPos, float xSize, float ySize, UString name, UString text) : CBaseUIButton(xPos, yPos, xSize, ySize, name, text)
	{
		m_osu = osu;
		m_fAnim = 1.0f;
	}

	virtual ~OsuOptionsMenuResetButton()
	{
		anim->deleteExistingAnimation(&m_fAnim);
	}

	virtual void draw(Graphics *g)
	{
		if (!m_bVisible || m_fAnim <= 0.0f) return;

		const int fullColorBlockSize = 4 * Osu::getUIScale();

		Color left = COLOR((int)(255*m_fAnim), 255, 233, 50);
		Color middle = COLOR((int)(255*m_fAnim), 255, 211, 50);
		Color right = 0x00000000;

		g->fillGradient(m_vPos.x, m_vPos.y, m_vSize.x*1.25f, m_vSize.y, middle, right, middle, right);
		g->fillGradient(m_vPos.x, m_vPos.y, fullColorBlockSize, m_vSize.y, left, middle, left, middle);
	}

	virtual void update()
	{
		CBaseUIButton::update();
		if (!m_bVisible || !m_bEnabled) return;

		if (isMouseInside())
		{
			m_osu->getTooltipOverlay()->begin();
			{
				m_osu->getTooltipOverlay()->addLine("Reset");
			}
			m_osu->getTooltipOverlay()->end();
		}
	}

private:
	virtual void onEnabled()
	{
		CBaseUIButton::onEnabled();
		anim->moveQuadOut(&m_fAnim, 1.0f, (1.0f - m_fAnim)*0.15f, true);
	}

	virtual void onDisabled()
	{
		CBaseUIButton::onDisabled();
		anim->moveQuadOut(&m_fAnim, 0.0f, m_fAnim*0.15f, true);
	}

	Osu *m_osu;
	float m_fAnim;
};



OsuOptionsMenu::OsuOptionsMenu(Osu *osu) : OsuScreenBackable(osu)
{
	m_osu = osu;

	m_bFullscreen = false;
	m_fAnimation = 0.0f;

	// convar refs
	m_osu_slider_curve_points_separation_ref = convar->getConVarByName("osu_slider_curve_points_separation");
	m_osu_letterboxing_offset_x_ref = convar->getConVarByName("osu_letterboxing_offset_x");
	m_osu_letterboxing_offset_y_ref = convar->getConVarByName("osu_letterboxing_offset_y");
	m_osu_mod_fposu_ref = convar->getConVarByName("osu_mod_fposu");
	m_osu_skin_ref = convar->getConVarByName("osu_skin");
	m_osu_skin_is_from_workshop_ref = convar->getConVarByName("osu_skin_is_from_workshop");
	m_osu_skin_workshop_title_ref = convar->getConVarByName("osu_skin_workshop_title");
	m_osu_skin_workshop_id_ref = convar->getConVarByName("osu_skin_workshop_id");
	m_osu_ui_scale_ref = convar->getConVarByName("osu_ui_scale");

	// convar callbacks
	convar->getConVarByName("osu_skin_use_skin_hitsounds")->setCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onUseSkinsSoundSamplesChange) );
	osu_options_high_quality_sliders.setCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onHighQualitySlidersConVarChange) );
	osu_mania_keylayout_wizard.setCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onKeyBindingManiaPressedInt) );

	if (m_osu->isInVRMode())
		OSU_CONFIG_FILE_NAME = "osuvr";
	else
		OSU_CONFIG_FILE_NAME = "osu";

	m_osu->getNotificationOverlay()->addKeyListener(this);

	m_waitingKey = NULL;
	m_vrRenderTargetResolutionLabel = NULL;
	m_vrApproachDistanceSlider = NULL;
	m_vrVibrationStrengthSlider = NULL;
	m_vrSliderVibrationStrengthSlider = NULL;
	m_vrHudDistanceSlider = NULL;
	m_vrHudScaleSlider = NULL;
	m_resolutionLabel = NULL;
	m_fullscreenCheckbox = NULL;
	m_sliderQualitySlider = NULL;
	m_outputDeviceLabel = NULL;
	m_outputDeviceResetButton = NULL;
	m_dpiTextbox = NULL;
	m_cm360Textbox = NULL;
	m_letterboxingOffsetResetButton = NULL;
	m_skinSelectWorkshopButton = NULL;
	m_uiScaleSlider = NULL;
	m_uiScaleResetButton = NULL;

	m_fOsuFolderTextboxInvalidAnim = 0.0f;
	m_fVibrationStrengthExampleTimer = 0.0f;
	m_bLetterboxingOffsetUpdateScheduled = false;
	m_bWorkshopSkinSelectScheduled = false;
	m_bUIScaleChangeScheduled = false;
	m_bUIScaleScrollToSliderScheduled = false;
	m_bDPIScalingScrollToSliderScheduled = false;

	m_iNumResetAllKeyBindingsPressed = 0;

	m_iManiaK = 0;
	m_iManiaKey = 0;

	m_fSearchOnCharKeybindHackTime = 0.0f;

	m_container = new CBaseUIContainer(-1, 0, 0, 0, "");

	m_options = new CBaseUIScrollView(0, -1, 0, 0, "");
	m_options->setDrawFrame(true);
	m_options->setDrawBackground(true);
	m_options->setBackgroundColor(0xdd000000);
	m_options->setHorizontalScrolling(false);
	m_container->addBaseUIElement(m_options);

	m_categories = new CBaseUIScrollView(0, -1, 0, 0, "");
	m_categories->setDrawFrame(true);
	m_categories->setDrawBackground(true);
	m_categories->setBackgroundColor(0xff000000);
	m_categories->setHorizontalScrolling(false);
	m_categories->setVerticalScrolling(true);
	m_categories->setScrollResistance(30); // since all categories are always visible anyway
	m_container->addBaseUIElement(m_categories);

	m_contextMenu = new OsuUIContextMenu(m_osu, 50, 50, 150, 0, "", m_options);

	m_search = new OsuUISearchOverlay(m_osu, 0, 0, 0, 0, "");
	m_search->setOffsetRight(20);
	m_container->addBaseUIElement(m_search);

	m_spacer = new CBaseUILabel(0, 0, 1, 40, "", "");
	m_spacer->setDrawBackground(false);
	m_spacer->setDrawFrame(false);

	//**************************************************************************************************************************//

	CBaseUIElement *sectionGeneral = addSection("General");

	addSubSection("");
	OsuUIButton *downloadOsuButton = addButton("Download osu! and get some beatmaps!");
	downloadOsuButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onDownloadOsuClicked) );
	downloadOsuButton->setColor(0xff00ff00);
	downloadOsuButton->setTextColor(0xffffffff);

	addLabel("... or ...")->setCenterText(true);
	OsuUIButton *manuallyManageBeatmapsButton = addButton("Manually manage beatmap files?");
	manuallyManageBeatmapsButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onManuallyManageBeatmapsClicked) );
	manuallyManageBeatmapsButton->setColor(0xff10667b);

	addSubSection("osu!folder");
	addLabel("1) If you have an existing osu! installation:")->setTextColor(0xff666666);
	addLabel("2) osu! > Options > \"Open osu! folder\"")->setTextColor(0xff666666);
	addLabel("3) Copy paste the full path into the textbox:")->setTextColor(0xff666666);
	addLabel("");
	m_osuFolderTextbox = addTextbox(convar->getConVarByName("osu_folder")->getString(), convar->getConVarByName("osu_folder"));
	addSpacer();
	addCheckbox("Use osu!.db database (read-only)", "If you have an existing osu! installation,\nthen this will speed up the initial loading process.", convar->getConVarByName("osu_database_enabled"));
	if (env->getOS() != Environment::OS::OS_HORIZON)
		addCheckbox("Load osu! scores.db (read-only)", "If you have an existing osu! installation,\nalso load and display your achieved scores from there.", convar->getConVarByName("osu_scores_legacy_enabled"));

	addSubSection("Player (Name)");
	m_nameTextbox = addTextbox(convar->getConVarByName("name")->getString(), convar->getConVarByName("name"));
	addSpacer();
	addCheckbox("Include Relax/Autopilot for total weighted pp/acc", "NOTE: osu! does not allow this (since these mods are unranked).\nShould relax/autopilot scores be included in the weighted pp/acc calculation?", convar->getConVarByName("osu_user_include_relax_and_autopilot_for_stats"));
	addCheckbox("Show pp instead of score in scorebrowser", "Only McOsu scores will show pp.", convar->getConVarByName("osu_scores_sort_by_pp"));

	addSubSection("Window");
	addCheckbox("Pause on Focus Loss", "Should the game pause when you switch to another application?", convar->getConVarByName("osu_pause_on_focus_loss"));

	//**************************************************************************************************************************//

	CBaseUIElement *sectionGraphics = addSection("Graphics");

	addSubSection("Renderer");
	addCheckbox("VSync", "If enabled: plz enjoy input lag.", convar->getConVarByName("vsync"));

	if (env->getOS() == Environment::OS::OS_WINDOWS)
		addCheckbox("High Priority (!)", "WARNING: Only enable this if nothing else works!\nSets the process priority to High.\nMay fix microstuttering and other weird problems.\nTry to fix your broken computer/OS/drivers first!", convar->getConVarByName("win_processpriority"));

	addCheckbox("Show FPS Counter", convar->getConVarByName("osu_draw_fps"));
	if (env->getOS() != Environment::OS::OS_HORIZON)
	{
		addSpacer();

		if (!m_osu->isInVRMode())
			addCheckbox("Unlimited FPS", convar->getConVarByName("fps_unlimited"));

		CBaseUISlider *fpsSlider = addSlider("FPS Limiter:", 60.0f, 1000.0f, convar->getConVarByName("fps_max"));
		fpsSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangeInt) );
		fpsSlider->setKeyDelta(1);

		addSubSection("Layout");
		OPTIONS_ELEMENT resolutionSelect = addButton("Select Resolution", UString::format("%ix%i", m_osu->getScreenWidth(), m_osu->getScreenHeight()));
		((CBaseUIButton*)resolutionSelect.elements[0])->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onResolutionSelect) );
		m_resolutionLabel = (CBaseUILabel*)resolutionSelect.elements[1];
		m_resolutionSelectButton = resolutionSelect.elements[0];
		m_fullscreenCheckbox = addCheckbox("Fullscreen");
		m_fullscreenCheckbox->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onFullscreenChange) );
		addCheckbox("Borderless", "If enabled: plz enjoy input lag.", convar->getConVarByName("fullscreen_windowed_borderless"))->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onBorderlessWindowedChange) );
		addCheckbox("Keep Aspect Ratio", "Black borders instead of a stretched image.\nOnly relevant if fullscreen is enabled, and letterboxing is disabled.\nUse the two position sliders below to move the viewport around.", convar->getConVarByName("osu_resolution_keep_aspect_ratio"));
		addCheckbox("Letterboxing", "Useful to get the low latency of fullscreen with a smaller game resolution.\nUse the two position sliders below to move the viewport around.", convar->getConVarByName("osu_letterboxing"));
		m_letterboxingOffsetXSlider = addSlider("Horizontal position", -1.0f, 1.0f, convar->getConVarByName("osu_letterboxing_offset_x"), 170)->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangeLetterboxingOffset) )->setKeyDelta(0.01f)->setAnimated(false);
		m_letterboxingOffsetYSlider = addSlider("Vertical position", -1.0f, 1.0f, convar->getConVarByName("osu_letterboxing_offset_y"), 170)->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangeLetterboxingOffset) )->setKeyDelta(0.01f)->setAnimated(false);
	}

	if (!m_osu->isInVRMode())
	{
		addSubSection("UI Scaling");
		addCheckbox("DPI Scaling", UString::format("Automatically scale to the DPI of your display: %i DPI.\nScale factor = %i / 96 = %.2gx", env->getDPI(), env->getDPI(), env->getDPIScale()), convar->getConVarByName("osu_ui_scale_to_dpi"))->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onDPIScalingChange) );
		m_uiScaleSlider = addSlider("UI Scale:", 1.0f, 1.5f, convar->getConVarByName("osu_ui_scale"));
		m_uiScaleSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangeUIScale) );
		m_uiScaleSlider->setKeyDelta(0.01f);
		m_uiScaleSlider->setAnimated(false);
	}

	addSubSection("Detail Settings");
	addCheckbox("Mipmaps", "Reload your skin to apply! (CTRL + ALT + SHIFT + S)\nGenerate mipmaps for each skin element, at the cost of VRAM.\nProvides smoother visuals on lower resolutions for @2x-only skins.", convar->getConVarByName("osu_skin_mipmaps"));
	addSpacer();
	addCheckbox("Snaking in sliders", "\"Growing\" sliders.\nSliders gradually snake out from their starting point while fading in.\nHas no impact on performance whatsoever.", convar->getConVarByName("osu_snaking_sliders"));
	addCheckbox("Snaking out sliders", "\"Shrinking\" sliders.\nSliders will shrink with the sliderball while sliding.\nCan improve performance a tiny bit, since there will be less to draw overall.", convar->getConVarByName("osu_slider_shrink"));
	addSpacer();
	if (env->getOS() != Environment::OS::OS_HORIZON)
	{
		addCheckbox("Legacy Slider Renderer (!)", "WARNING: Only try enabling this on shitty old computers!\nMay or may not improve fps while few sliders are visible.\nGuaranteed lower fps while many sliders are visible!", convar->getConVarByName("osu_force_legacy_slider_renderer"));
		addCheckbox("Higher Quality Sliders (!)", "Disable this if your fps drop too low while sliders are visible.", convar->getConVarByName("osu_options_high_quality_sliders"))->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onHighQualitySlidersCheckboxChange) );
		m_sliderQualitySlider = addSlider("Slider Quality", 0.0f, 1.0f, convar->getConVarByName("osu_options_slider_quality"));
		m_sliderQualitySlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangeSliderQuality) );
	}

	//**************************************************************************************************************************//

	CBaseUIElement *sectionVR = NULL;
	if (m_osu->isInVRMode())
	{
		sectionVR = addSection("Virtual Reality");

		addSpacer();
		addSubSection("VR Renderer");
		addSpacer();
		m_vrRenderTargetResolutionLabel = addLabel("Final Resolution: %ix%i");
		addSpacer();
		CBaseUISlider *ssSlider = addSlider("SuperSampling Multiplier", 0.5f, 2.5f, convar->getConVarByName("vr_ss"), 230.0f);
		ssSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangeVRSuperSampling) );
		ssSlider->setKeyDelta(0.1f);
		ssSlider->setAnimated(false);
		CBaseUISlider *aaSlider = addSlider("AntiAliasing (MSAA)", 0.0f, 16.0f, convar->getConVarByName("vr_aa"), 230.0f);
		aaSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangeVRAntiAliasing) );
		aaSlider->setKeyDelta(2.0f);
		aaSlider->setAnimated(false);

		/*
		addSpacer();
#ifdef MCENGINE_FEATURE_DIRECTX

		addSubSection("LIV SDK");
		addCheckbox("LIV SDK Support", "Copy your externalcamera.cfg file into /<Steam>/steamapps/common/McOsu/\nUse the button below or \"vr_liv_reload_calibration\" to reload it during runtime.", convar->getConVarByName("vr_liv"));
		addLabel("");
		addButton("Reload externalcamera.cfg or liv-camera.cfg")->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onLIVReloadCalibrationClicked) );

#else

		addSubSection("LIV SDK (N/A)");
		addLabel("To enable LIV SDK support, follow these steps:");
		addLabel("");
		addLabel("1) In your Steam library, right click on McOsu")->setTextColor(0xff777777);
		addLabel("2) Click on Properties")->setTextColor(0xff777777);
		addLabel("3) BETAS > Select the \"cutting-edge\" beta")->setTextColor(0xff777777);

#endif
		*/

		addSpacer();
		addSubSection("Play Area / Playfield");
		addSpacer();
		addSlider("RenderModel Brightness", 0.1f, 10.0f, convar->getConVarByName("vr_controller_model_brightness_multiplier"))->setKeyDelta(0.1f);
		addSlider("Background Brightness", 0.0f, 1.0f, convar->getConVarByName("vr_background_brightness"))->setKeyDelta(0.05f);
		addSlider("VR Cursor Opacity", 0.0f, 1.0f, convar->getConVarByName("osu_vr_cursor_alpha"))->setKeyDelta(0.1f);
		addSpacer();
		m_vrApproachDistanceSlider = addSlider("Approach Distance", 0.0f, 50.0f, convar->getConVarByName("osu_vr_approach_distance"), 175.0f);
		m_vrApproachDistanceSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangeOneDecimalPlaceMeters) );
		m_vrApproachDistanceSlider->setKeyDelta(1.0f);
		m_vrHudDistanceSlider = addSlider("HUD Distance", 1.0f, 50.0f, convar->getConVarByName("osu_vr_hud_distance"), 175.0f);
		m_vrHudDistanceSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangeOneDecimalPlaceMeters) );
		m_vrHudDistanceSlider->setKeyDelta(1.0f);
		m_vrHudScaleSlider = addSlider("HUD Scale", 0.1f, 10.0f, convar->getConVarByName("osu_vr_hud_scale"));
		m_vrHudScaleSlider->setKeyDelta(0.1f);

		addSpacer();
		addSubSection("Haptic Feedback");
		addSpacer();
		m_vrVibrationStrengthSlider = addSlider("Circle Vibration Strength", 0.0f, 1.0f, convar->getConVarByName("osu_vr_controller_vibration_strength"));
		m_vrVibrationStrengthSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangePercent) );
		m_vrVibrationStrengthSlider->setKeyDelta(0.01f);
		m_vrSliderVibrationStrengthSlider = addSlider("Slider Vibration Strength", 0.0f, 1.0f, convar->getConVarByName("osu_vr_slider_controller_vibration_strength"));
		m_vrSliderVibrationStrengthSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangePercent) );
		m_vrSliderVibrationStrengthSlider->setKeyDelta(0.01f);

		addSpacer();
		addSubSection("Gameplay");
		addSpacer();
		addCheckbox("Draw VR Approach Circles", convar->getConVarByName("osu_vr_draw_approach_circles"));
		addCheckbox("Draw VR Approach Circles on top (!)", "\"on top\" = In front of closer objects, overlapping everything (!)", convar->getConVarByName("osu_vr_approach_circles_on_top"));
		addCheckbox("Draw VR Approach Circles on Playfield (flat)", "\"flat\" = On the Playfield plane, instead of flying in.", convar->getConVarByName("osu_vr_approach_circles_on_playfield"));

		addSpacer();
		addSubSection("Miscellaneous");
		addSpacer();
		addCheckbox("Spectator Camera (ALT + C)", "Needs \"Draw HMD to Window\" enabled to work.\nToggle camera-control-mode with ALT + C.\nUse mouse + keyboard (WASD) to rotate/position the camera.", convar->getConVarByName("vr_spectator_mode"));
		addCheckbox("Auto Switch Primary Controller", convar->getConVarByName("vr_auto_switch_primary_controller"));
		addSpacer();
		addCheckbox("Draw HMD to Window (!)", "If this is disabled, then nothing will be drawn to the window.", convar->getConVarByName("vr_draw_hmd_to_window"));
		addCheckbox("Draw Both Eyes to Window", "Only applies if \"Draw HMD to Window\" is enabled, see above.", convar->getConVarByName("vr_draw_hmd_to_window_draw_both_eyes"));
		addCheckbox("Draw Floor", convar->getConVarByName("osu_vr_draw_floor"));
		addCheckbox("Draw Controller Models", convar->getConVarByName("vr_draw_controller_models"));
		addCheckbox("Draw Lighthouse Models", convar->getConVarByName("vr_draw_lighthouse_models"));
		addSpacer();
		addCheckbox("Draw VR Game (3D)", "Only disable this if you want to play the flat version in VR.\nThat is, on the virtual flat screen with the mouse cursor.", convar->getConVarByName("osu_vr_draw_playfield"));
		addCheckbox("Draw Desktop Game (2D)", "Disable this to increase VR performance (by not drawing the flat/desktop game additionally).", convar->getConVarByName("osu_vr_draw_desktop_playfield"));
		addSpacer();
		addCheckbox("Controller Distance Color Warning", convar->getConVarByName("osu_vr_controller_warning_distance_enabled"));
		addCheckbox("Show Tutorial on Startup", convar->getConVarByName("osu_vr_tutorial"));
		addSpacer();
	}

	//**************************************************************************************************************************//

	CBaseUIElement *sectionAudio = addSection("Audio");

	addSubSection("Devices");
	if (env->getOS() != Environment::OS::OS_HORIZON)
	{
		OPTIONS_ELEMENT outputDeviceSelect = addButton("Select Output Device", "Default", true);
		((CBaseUIButton*)outputDeviceSelect.elements[0])->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onOutputDeviceSelect) );
		outputDeviceSelect.resetButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onOutputDeviceResetClicked) );

		m_outputDeviceResetButton = outputDeviceSelect.resetButton;
		m_outputDeviceSelectButton = outputDeviceSelect.elements[0];
		m_outputDeviceLabel = (CBaseUILabel*)outputDeviceSelect.elements[1];
	}
	else
		addButton("Restart SoundEngine (fix crackling)")->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onOutputDeviceRestart) );

#ifdef MCENGINE_FEATURE_BASS_WASAPI

	addSubSection("WASAPI");
	CBaseUISlider *wasapiBufferSlider = addSlider("Buffer Size:", 0.001f, 0.050f, convar->getConVarByName("win_snd_wasapi_buffer_size"));
	wasapiBufferSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangeFloatMS) );
	wasapiBufferSlider->setKeyDelta(0.001f);
	addLabel("Windows 7: Start at 11 ms,")->setTextColor(0xff666666);
	addLabel("Windows 10: Start at 1 ms,")->setTextColor(0xff666666);
	addLabel("and if crackling: increment until fixed.")->setTextColor(0xff666666);
	addLabel("(lower is better, non-wasapi has ~40 ms minimum)")->setTextColor(0xff666666);
	addCheckbox("Exclusive Mode", "Dramatically reduces audio latency, leave this enabled.\nExclusive Mode = No other application can use audio.", convar->getConVarByName("win_snd_wasapi_exclusive"));

#endif

	addSubSection("Volume");
	addSlider("Master:", 0.0f, 1.0f, convar->getConVarByName("osu_volume_master"), 70.0f)->setKeyDelta(0.01f);
	addSlider("Music:", 0.0f, 1.0f, convar->getConVarByName("osu_volume_music"), 70.0f)->setKeyDelta(0.01f);
	addSlider("Effects:", 0.0f, 1.0f, convar->getConVarByName("osu_volume_effects"), 70.0f)->setKeyDelta(0.01f);

	addSubSection("Offset Adjustment");
	CBaseUISlider *offsetSlider = addSlider("Universal Offset:", -300.0f, 300.0f, convar->getConVarByName("osu_universal_offset"));
	offsetSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangeIntMS) );
	offsetSlider->setKeyDelta(1);

	//**************************************************************************************************************************//

	m_skinSection = addSection("Skin");

	addSubSection("Skin");
	addSkinPreview();
	{
		OPTIONS_ELEMENT skinSelect;
		if (steam->isReady())
		{
			skinSelect = addButtonButtonLabel("Local Skin ...", "Workshop ...", "default");
			m_skinSelectLocalButton = skinSelect.elements[0];
			m_skinSelectWorkshopButton = skinSelect.elements[1];
			m_skinLabel = (CBaseUILabel*)skinSelect.elements[2];

			((OsuUIButton*)m_skinSelectWorkshopButton)->setColor(0xff108fe8);
			((CBaseUIButton*)m_skinSelectWorkshopButton)->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSkinSelectWorkshop) );
		}
		else
		{
			skinSelect = addButton("Select Skin", "default");
			m_skinSelectLocalButton = skinSelect.elements[0];
			m_skinLabel = (CBaseUILabel*)skinSelect.elements[1];
		}

		((CBaseUIButton*)m_skinSelectLocalButton)->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSkinSelect) );
	}
	addButton("Reload Skin (CTRL+ALT+SHIFT+S)")->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSkinReload) );
	addSpacer();
	CBaseUISlider *numberScaleSlider = addSlider("Number Scale:", 0.01f, 3.0f, convar->getConVarByName("osu_number_scale_multiplier"), 135.0f);
	numberScaleSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangePercent) );
	numberScaleSlider->setKeyDelta(0.01f);
	CBaseUISlider *hitResultScaleSlider = addSlider("HitResult Scale:", 0.01f, 3.0f, convar->getConVarByName("osu_hitresult_scale"), 135.0f);
	hitResultScaleSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangePercent) );
	hitResultScaleSlider->setKeyDelta(0.01f);
	addCheckbox("Draw Numbers", convar->getConVarByName("osu_draw_numbers"));
	addCheckbox("Draw ApproachCircles", convar->getConVarByName("osu_draw_approach_circles"));
	addSpacer();
	addCheckbox("Ignore Beatmap Sample Volume", "Ignore the effect volume values from beatmap timingpoints.\nToo quiet hitsounds can destroy your accuracy and concentration, enabling this will fix that.", convar->getConVarByName("osu_ignore_beatmap_sample_volume"));
	addCheckbox("Ignore Beatmap Combo Colors", convar->getConVarByName("osu_ignore_beatmap_combo_colors"));
	addCheckbox("Use skin's sound samples", "If this is not selected, then the default skin hitsounds will be used.", convar->getConVarByName("osu_skin_use_skin_hitsounds"));
	addCheckbox("Load HD @2x", "On very low resolutions (below 1600x900) you can disable this to get smoother visuals.", convar->getConVarByName("osu_skin_hd"));
	addSpacer();
	addCheckbox("Draw CursorTrail", convar->getConVarByName("osu_draw_cursor_trail"));
	addCheckbox("Force Smooth CursorTrail", "Usually, the presence of the cursormiddle.png skin image enables smooth cursortrails.\nThis option allows you to force enable smooth cursortrails for all skins.", convar->getConVarByName("osu_cursor_trail_smooth_force"));
	m_cursorSizeSlider = addSlider("Cursor Size:", 0.01f, 5.0f, convar->getConVarByName("osu_cursor_scale"));
	m_cursorSizeSlider->setAnimated(false);
	m_cursorSizeSlider->setKeyDelta(0.01f);
	addCheckbox("Automatic Cursor Size", "Cursor size will adjust based on the CS of the current beatmap.", convar->getConVarByName("osu_automatic_cursor_size"));
	addSpacer();
	m_sliderPreviewElement = (OsuOptionsMenuSliderPreviewElement*)addSliderPreview();
	addCheckbox("Use slidergradient.png", "Enabling this will improve performance,\nbut also block all dynamic slider (color/border) features.", convar->getConVarByName("osu_slider_use_gradient_image"));
	addCheckbox("Use osu!lazer Slider Style", "Only really looks good if your skin doesn't \"SliderTrackOverride\" too dark.", convar->getConVarByName("osu_slider_osu_next_style"));
	addCheckbox("Use combo color as tint for slider ball", convar->getConVarByName("osu_slider_ball_tint_combo_color"));
	addCheckbox("Use combo color as tint for slider border", convar->getConVarByName("osu_slider_border_tint_combo_color"));
	addCheckbox("Draw SliderEndCircle", convar->getConVarByName("osu_slider_draw_endcircle"));
	addSlider("Slider Opacity", 0.0f, 1.0f, convar->getConVarByName("osu_slider_alpha_multiplier"), 200.0f);
	addSlider("SliderBody Opacity", 0.0f, 1.0f, convar->getConVarByName("osu_slider_body_alpha_multiplier"), 200.0f);
	addSlider("SliderBody Saturation", 0.0f, 1.0f, convar->getConVarByName("osu_slider_body_color_saturation"), 200.0f);
	addSlider("SliderBorder Size", 0.0f, 9.0f, convar->getConVarByName("osu_slider_border_size_multiplier"))->setKeyDelta(0.01f);

	//**************************************************************************************************************************//

	CBaseUIElement *sectionInput = addSection("Input");

	addSubSection("Mouse");
	if (env->getOS() == Environment::OS::OS_WINDOWS || env->getOS() == Environment::OS::OS_MACOS || env->getOS() == Environment::OS::OS_HORIZON)
	{
		addSlider("Sensitivity:", (env->getOS() == Environment::OS::OS_HORIZON ? 1.0f : 0.1f), 6.0f, convar->getConVarByName("mouse_sensitivity"))->setKeyDelta(0.01f);

		if (env->getOS() == Environment::OS::OS_HORIZON)
			addSlider("Joystick S.:", 0.1f, 6.0f, convar->getConVarByName("sdl_joystick_mouse_sensitivity"))->setKeyDelta(0.01f);

		if (env->getOS() == Environment::OS::OS_MACOS)
		{
			addLabel("");
			addLabel("WARNING: Set Sensitivity to 1 for tablets!")->setTextColor(0xffff0000);
			addLabel("");
		}
	}
	if (env->getOS() == Environment::OS::OS_WINDOWS)
	{
		addCheckbox("Raw Input", convar->getConVarByName("mouse_raw_input"));
		addCheckbox("Map Absolute Raw Input to Window", convar->getConVarByName("mouse_raw_input_absolute_to_window"))->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onRawInputToAbsoluteWindowChange) );
	}
	if (env->getOS() == Environment::OS::OS_LINUX)
	{
		addLabel("Use system settings to change the mouse sensitivity.")->setTextColor(0xff555555);
		addLabel("");
		addLabel("Use xinput or xsetwacom to change the tablet area.")->setTextColor(0xff555555);
		addLabel("");
	}
	if (env->getOS() != Environment::OS::OS_HORIZON)
	{
		addCheckbox("Confine Cursor (Windowed)", convar->getConVarByName("osu_confine_cursor_windowed"));
		addCheckbox("Confine Cursor (Fullscreen)", convar->getConVarByName("osu_confine_cursor_fullscreen"));
		addCheckbox("Disable Mouse Wheel in Play Mode", convar->getConVarByName("osu_disable_mousewheel"));
	}
	addCheckbox("Disable Mouse Buttons in Play Mode", convar->getConVarByName("osu_disable_mousebuttons"));
	addCheckbox("Cursor ripples", "The cursor will ripple outwards on clicking.", convar->getConVarByName("osu_draw_cursor_ripples"));

	if (env->getOS() == Environment::OS::OS_WINDOWS)
	{
		addSubSection("Tablet");
		addCheckbox("OS TabletPC Support", "Enable this if your tablet clicks aren't handled correctly.", convar->getConVarByName("win_realtimestylus"));
		addCheckbox("Windows Ink Workaround", "Enable this if your tablet cursor is stuck in a tiny area on the top left of the screen.\nIf this doesn't fix it, use \"Ignore Sensitivity & Raw Input\" below.", convar->getConVarByName("win_ink_workaround"));
		addCheckbox("Ignore Sensitivity & Raw Input", "Only use this if nothing else works.\nIf this is enabled, then the in-game sensitivity slider will no longer work for tablets!\n(You can then instead use your tablet configuration software to change the tablet area.)", convar->getConVarByName("tablet_sensitivity_ignore"));
	}

	addSpacer();
	CBaseUIElement *subSectionKeyboard = addSubSection("Keyboard");
	addSubSection("Keys - osu! Standard Mode");
	addKeyBindButton("Left Click", &OsuKeyBindings::LEFT_CLICK);
	addKeyBindButton("Right Click", &OsuKeyBindings::RIGHT_CLICK);
	addSubSection("Keys - In-Game");
	addKeyBindButton("Game Pause", &OsuKeyBindings::GAME_PAUSE);
	addKeyBindButton("Skip Cutscene", &OsuKeyBindings::SKIP_CUTSCENE);
	addKeyBindButton("Toggle Scoreboard", &OsuKeyBindings::TOGGLE_SCOREBOARD);
	addKeyBindButton("Scrubbing (+ Click Drag!)", &OsuKeyBindings::SEEK_TIME);
	addKeyBindButton("Increase Local Song Offset", &OsuKeyBindings::INCREASE_LOCAL_OFFSET);
	addKeyBindButton("Decrease Local Song Offset", &OsuKeyBindings::DECREASE_LOCAL_OFFSET);
	addKeyBindButton("Quick Retry (hold briefly)", &OsuKeyBindings::QUICK_RETRY);
	addKeyBindButton("Quick Save", &OsuKeyBindings::QUICK_SAVE);
	addKeyBindButton("Quick Load", &OsuKeyBindings::QUICK_LOAD);
	addSubSection("Keys - Universal");
	addKeyBindButton("Save Screenshot", &OsuKeyBindings::SAVE_SCREENSHOT);
	addKeyBindButton("Increase Volume", &OsuKeyBindings::INCREASE_VOLUME);
	addKeyBindButton("Decrease Volume", &OsuKeyBindings::DECREASE_VOLUME);
	addKeyBindButton("Disable Mouse Buttons", &OsuKeyBindings::DISABLE_MOUSE_BUTTONS);
	addKeyBindButton("Boss Key (Minimize)", &OsuKeyBindings::BOSS_KEY);
	addSubSection("Keys - Mod Select");
	addKeyBindButton("Easy", &OsuKeyBindings::MOD_EASY);
	addKeyBindButton("No Fail", &OsuKeyBindings::MOD_NOFAIL);
	addKeyBindButton("Half Time", &OsuKeyBindings::MOD_HALFTIME);
	addKeyBindButton("Hard Rock", &OsuKeyBindings::MOD_HARDROCK);
	addKeyBindButton("Sudden Death", &OsuKeyBindings::MOD_SUDDENDEATH);
	addKeyBindButton("Double Time", &OsuKeyBindings::MOD_DOUBLETIME);
	addKeyBindButton("Hidden", &OsuKeyBindings::MOD_HIDDEN);
	addKeyBindButton("Flashlight", &OsuKeyBindings::MOD_FLASHLIGHT);
	addKeyBindButton("Relax", &OsuKeyBindings::MOD_RELAX);
	addKeyBindButton("Autopilot", &OsuKeyBindings::MOD_AUTOPILOT);
	addKeyBindButton("Spunout", &OsuKeyBindings::MOD_SPUNOUT);
	addKeyBindButton("Auto", &OsuKeyBindings::MOD_AUTO);
	addKeyBindButton("Score V2", &OsuKeyBindings::MOD_SCOREV2);
	addSpacer();
	OsuUIButton *resetAllKeyBindingsButton = addButton("Reset all key bindings");
	resetAllKeyBindingsButton->setColor(0xffff0000);
	resetAllKeyBindingsButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onKeyBindingsResetAllPressed) );
	///addButton("osu!mania layout")->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onKeyBindingManiaPressed) );

	//**************************************************************************************************************************//

	CBaseUIElement *sectionGameplay = addSection("Gameplay");

	addSubSection("General");
	m_backgroundDimSlider = addSlider("Background Dim:", 0.0f, 1.0f, convar->getConVarByName("osu_background_dim"), 220.0f);
	m_backgroundDimSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangePercent) );
	m_backgroundBrightnessSlider = addSlider("Background Brightness:", 0.0f, 1.0f, convar->getConVarByName("osu_background_brightness"), 220.0f);
	m_backgroundBrightnessSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangePercent) );
	addSpacer();
	addCheckbox("Don't change dim level during breaks", "Makes the background basically impossible to see during breaks.\nNot recommended.", convar->getConVarByName("osu_background_dont_fade_during_breaks"));
	addCheckbox("Show approach circle on first \"Hidden\" object", convar->getConVarByName("osu_show_approach_circle_on_first_hidden_object"));
	addCheckbox("SuddenDeath restart on miss", "Skips the failing animation, and instantly restarts like SS/PF.", convar->getConVarByName("osu_mod_suddendeath_restart"));
	addSpacer();
	addCheckbox("Note Blocking/Locking", "NOTE: osu! has this always enabled, so leave it enabled for practicing.\n\"Protects\" you by only allowing circles to be clicked in order.", convar->getConVarByName("osu_note_blocking"));
	addSpacer();
	addCheckbox("Load Background Images (!)", "NOTE: Disabling this will disable ALL beatmap images everywhere!", convar->getConVarByName("osu_load_beatmap_background_images"));
	addCheckbox("Draw Background in Beatmap", convar->getConVarByName("osu_draw_beatmap_background_image"));
	addCheckbox("Draw Background in SongBrowser", "NOTE: You can disable this if you always want menu-background.", convar->getConVarByName("osu_draw_songbrowser_background_image"));
	addCheckbox("Draw Background Thumbnails in SongBrowser", convar->getConVarByName("osu_draw_songbrowser_thumbnails"));
	addCheckbox("Draw Background in Ranking/Results Screen", convar->getConVarByName("osu_draw_rankingscreen_background_image"));
	addCheckbox("Draw menu-background in Menu", convar->getConVarByName("osu_draw_menu_background"));
	addCheckbox("Draw menu-background in SongBrowser", "NOTE: Only applies if \"Draw Background in SongBrowser\" is disabled.", convar->getConVarByName("osu_draw_songbrowser_menu_background_image"));
	addSpacer();
	//addCheckbox("Show pp on ranking screen", convar->getConVarByName("osu_rankingscreen_pp"));

	addSubSection("HUD");
	addCheckbox("Draw HUD", "NOTE: You can also press SHIFT + TAB while playing to toggle this.", convar->getConVarByName("osu_draw_hud"));
	addSpacer();
	addCheckbox("Draw Score", convar->getConVarByName("osu_draw_score"));
	addCheckbox("Draw Combo", convar->getConVarByName("osu_draw_combo"));
	addCheckbox("Draw Accuracy", convar->getConVarByName("osu_draw_accuracy"));
	addCheckbox("Draw ProgressBar", convar->getConVarByName("osu_draw_progressbar"));
	addCheckbox("Draw HitErrorBar", convar->getConVarByName("osu_draw_hiterrorbar"));
	addCheckbox("Draw ScoreBoard", convar->getConVarByName("osu_draw_scoreboard"));
	addCheckbox("Draw Key Overlay", convar->getConVarByName("osu_draw_inputoverlay"));
	addCheckbox("Draw Scrubbing Timeline", convar->getConVarByName("osu_draw_scrubbing_timeline"));
	addCheckbox("Draw Miss Window on HitErrorBar", convar->getConVarByName("osu_hud_hiterrorbar_showmisswindow"));
	addSpacer();
	addCheckbox("Draw Stats: pp", "Realtime pp counter.\nDynamically calculates currently earned pp by incrementally updating the star rating.", convar->getConVarByName("osu_draw_statistics_pp"));
	addCheckbox("Draw Stats: Misses", convar->getConVarByName("osu_draw_statistics_misses"));
	addCheckbox("Draw Stats: SliderBreaks", convar->getConVarByName("osu_draw_statistics_sliderbreaks"));
	addCheckbox("Draw Stats: BPM", convar->getConVarByName("osu_draw_statistics_bpm"));
	addCheckbox("Draw Stats: AR", convar->getConVarByName("osu_draw_statistics_ar"));
	addCheckbox("Draw Stats: CS", convar->getConVarByName("osu_draw_statistics_cs"));
	addCheckbox("Draw Stats: OD", convar->getConVarByName("osu_draw_statistics_od"));
	addCheckbox("Draw Stats: Notes Per Second", "How many clicks per second are currently required.", convar->getConVarByName("osu_draw_statistics_nps"));
	addCheckbox("Draw Stats: Note Density", "How many objects are visible at the same time.", convar->getConVarByName("osu_draw_statistics_nd"));
	addCheckbox("Draw Stats: Unstable Rate", convar->getConVarByName("osu_draw_statistics_ur"));
	addSpacer();
	m_hudSizeSlider = addSlider("HUD Scale:", 0.01f, 3.0f, convar->getConVarByName("osu_hud_scale"), 165.0f);
	m_hudSizeSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangePercent) );
	m_hudSizeSlider->setKeyDelta(0.01f);
	addSpacer();
	m_hudScoreScaleSlider = addSlider("Score Scale:", 0.01f, 3.0f, convar->getConVarByName("osu_hud_score_scale"), 165.0f);
	m_hudScoreScaleSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangePercent) );
	m_hudScoreScaleSlider->setKeyDelta(0.01f);
	m_hudComboScaleSlider = addSlider("Combo Scale:", 0.01f, 3.0f, convar->getConVarByName("osu_hud_combo_scale"), 165.0f);
	m_hudComboScaleSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangePercent) );
	m_hudComboScaleSlider->setKeyDelta(0.01f);
	m_hudAccuracyScaleSlider = addSlider("Accuracy Scale:", 0.01f, 3.0f, convar->getConVarByName("osu_hud_accuracy_scale"), 165.0f);
	m_hudAccuracyScaleSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangePercent) );
	m_hudAccuracyScaleSlider->setKeyDelta(0.01f);
	m_hudHiterrorbarScaleSlider = addSlider("HitErrorBar Scale:", 0.01f, 3.0f, convar->getConVarByName("osu_hud_hiterrorbar_scale"), 165.0f);
	m_hudHiterrorbarScaleSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangePercent) );
	m_hudHiterrorbarScaleSlider->setKeyDelta(0.01f);
	m_hudProgressbarScaleSlider = addSlider("ProgressBar Scale:", 0.01f, 3.0f, convar->getConVarByName("osu_hud_progressbar_scale"), 165.0f);
	m_hudProgressbarScaleSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangePercent) );
	m_hudProgressbarScaleSlider->setKeyDelta(0.01f);
	m_hudScoreBoardScaleSlider = addSlider("ScoreBoard Scale:", 0.01f, 3.0f, convar->getConVarByName("osu_hud_scoreboard_scale"), 165.0f);
	m_hudScoreBoardScaleSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangePercent) );
	m_hudScoreBoardScaleSlider->setKeyDelta(0.01f);
	m_hudInputoverlayScaleSlider = addSlider("Key Overlay Scale:", 0.01f, 3.0f, convar->getConVarByName("osu_hud_inputoverlay_scale"), 165.0f);
	m_hudInputoverlayScaleSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangePercent) );
	m_hudInputoverlayScaleSlider->setKeyDelta(0.01f);
	m_statisticsOverlayScaleSlider = addSlider("Statistics Scale:", 0.01f, 3.0f, convar->getConVarByName("osu_hud_statistics_scale"), 165.0f);
	m_statisticsOverlayScaleSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangePercent) );
	m_statisticsOverlayScaleSlider->setKeyDelta(0.01f);

	addSubSection("Playfield");
	addCheckbox("Draw FollowPoints", convar->getConVarByName("osu_draw_followpoints"));
	addCheckbox("Draw scorebar-bg", "Some skins abuse this as the playfield background image.\nIf not, then disable it (otherwise you see an empty health bar all the time).", convar->getConVarByName("osu_draw_scorebarbg"));
	addCheckbox("Draw Playfield Border", "Compared to the scorebar-bg hack above, this will draw the correct border relative to the current CS.", convar->getConVarByName("osu_draw_playfield_border"));
	addSpacer();
	m_playfieldBorderSizeSlider = addSlider("Playfield Border Size:", 0.0f, 500.0f, convar->getConVarByName("osu_hud_playfield_border_size"));
	m_playfieldBorderSizeSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangeInt) );
	m_playfieldBorderSizeSlider->setKeyDelta(1.0f);

	addSubSection("Hitobjects");
	addCheckbox("Use Fast Hidden Fading Sliders (!)", "NOTE: osu! doesn't do this, so don't enable it for serious practicing.\nIf enabled: Fade out sliders with the same speed as circles.", convar->getConVarByName("osu_mod_hd_slider_fast_fade"));
	addCheckbox("Use Score v2 Slider Accuracy", "Affects pp and accuracy calculations, but does not affect score.\nUse the score v2 mod if you want the 1000000 max score cap/calculation.", convar->getConVarByName("osu_slider_scorev2"));

	//**************************************************************************************************************************//

	CBaseUIElement *sectionFposu = addSection("FPoSu (3D)");

	addSubSection("FPoSu - General");
	addCheckbox("FPoSu", "The real 3D FPS mod.\nPlay from a first person shooter perspective in a 3D environment.\nThis is intended only for mouse! (Enable \"Tablet/Absolute Mode\" for tablets.)", convar->getConVarByName("osu_mod_fposu"));
	addCheckbox("Curved play area", convar->getConVarByName("fposu_curved"));
	addCheckbox("Background cube", convar->getConVarByName("fposu_cube"));
	addLabel("");
	addLabel("NOTE: Use CTRL + O during gameplay to get here!")->setTextColor(0xff777777);
	addLabel("");
	CBaseUISlider *fposuDistanceSlider = addSlider("Distance:", 0.01f, 2.0f, convar->getConVarByName("fposu_distance"));
	fposuDistanceSlider->setKeyDelta(0.01f);
	addCheckbox("Vertical FOV", "If enabled: Vertical FOV.\nIf disabled: Horizontal FOV (default).", convar->getConVarByName("fposu_vertical_fov"));
	CBaseUISlider *fovSlider = addSlider("FOV:", 20.0f, 160.0f, convar->getConVarByName("fposu_fov"));
	fovSlider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChangeOneDecimalPlace) );
	fovSlider->setKeyDelta(0.1f);
	addLabel("");
	addLabel("LEFT/RIGHT arrow keys to precisely adjust sliders.")->setTextColor(0xff777777);

	if (env->getOS() == Environment::OS::OS_WINDOWS)
	{
		addSubSection("FPoSu - Mouse");
		OsuUIButton *cm360CalculatorLinkButton = addButton("https://www.mouse-sensitivity.com/");
		cm360CalculatorLinkButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onCM360CalculatorLinkClicked) );
		cm360CalculatorLinkButton->setColor(0xff10667b);
		addLabel("");
		m_dpiTextbox = addTextbox(convar->getConVarByName("fposu_mouse_dpi")->getString(), "DPI:", convar->getConVarByName("fposu_mouse_dpi"));
		m_cm360Textbox = addTextbox(convar->getConVarByName("fposu_mouse_cm_360")->getString(), "cm per 360:", convar->getConVarByName("fposu_mouse_cm_360"));
		addLabel("");
		addCheckbox("Invert Vertical", convar->getConVarByName("fposu_invert_vertical"));
		addCheckbox("Invert Horizontal", convar->getConVarByName("fposu_invert_horizontal"));
		addCheckbox("Tablet/Absolute Mode (!)", "WARNING: Do NOT enable this if you are using a mouse!\nIf this is enabled, then DPI and cm per 360 will be ignored!", convar->getConVarByName("fposu_absolute_mode"));
	}

	//**************************************************************************************************************************//

	CBaseUIElement *sectionOnline = NULL;
	if (env->getOS() != Environment::OS::OS_HORIZON)
	{
		sectionOnline = addSection("Online");

		addSubSection("Integration");
		addCheckbox("Rich Presence (Discord + Steam)", "Shows your current game state in your friends' friendslists.\ne.g.: Playing Gavin G - Reach Out [Cherry Blossom's Insane]", convar->getConVarByName("osu_rich_presence"));
	}

	//**************************************************************************************************************************//

	addSection("Bullshit");

	addSubSection("Why");
	addCheckbox("Rainbow ApproachCircles", convar->getConVarByName("osu_circle_rainbow"));
	addCheckbox("Rainbow Sliders", convar->getConVarByName("osu_slider_rainbow"));
	addCheckbox("Rainbow Numbers", convar->getConVarByName("osu_circle_number_rainbow"));
	addCheckbox("SliderBreak Epilepsy", convar->getConVarByName("osu_slider_break_epilepsy"));
	addCheckbox("Invisible Cursor", convar->getConVarByName("osu_hide_cursor_during_gameplay"));
	addCheckbox("Draw 300s", convar->getConVarByName("osu_hitresult_draw_300s"));
	addSpacer();
	addSpacer();
	addSpacer();

	//**************************************************************************************************************************//

	// build categories
	addCategory(sectionGeneral, OsuIcons::GEAR);
	addCategory(sectionGraphics, OsuIcons::DESKTOP);

	if (m_osu->isInVRMode())
		addCategory(sectionVR, OsuIcons::EYE);

	addCategory(sectionAudio, OsuIcons::VOLUME_UP);
	addCategory(m_skinSection, OsuIcons::PAINTBRUSH);
	addCategory(sectionInput, OsuIcons::GAMEPAD);
	addCategory(subSectionKeyboard, OsuIcons::KEYBOARD);
	addCategory(sectionGameplay, OsuIcons::CIRCLE);
	m_fposuCategoryButton = addCategory(sectionFposu, OsuIcons::CUBE);

	if (sectionOnline != NULL)
		addCategory(sectionOnline, OsuIcons::GLOBE);

	//**************************************************************************************************************************//

	// the context menu gets added last (drawn on top of everything)
	m_options->getContainer()->addBaseUIElement(m_contextMenu);

	// HACKHACK: force current value update
	if (m_sliderQualitySlider != NULL)
		onHighQualitySlidersConVarChange("", osu_options_high_quality_sliders.getString());
}

OsuOptionsMenu::~OsuOptionsMenu()
{
	SAFE_DELETE(m_container);
}

void OsuOptionsMenu::draw(Graphics *g)
{
	const bool isAnimating = anim->isAnimating(&m_fAnimation);
	if (!m_bVisible && !isAnimating) return;

#if defined(MCENGINE_FEATURE_OPENGL)

			const bool isOpenGLRendererHack = (dynamic_cast<OpenGLLegacyInterface*>(g) != NULL || dynamic_cast<OpenGL3Interface*>(g) != NULL);

#elif defined(MCENGINE_FEATURE_OPENGLES)

			const bool isOpenGLRendererHack = (dynamic_cast<OpenGLES2Interface*>(g) != NULL);

#endif

	m_sliderPreviewElement->setDrawSliderHack(!isAnimating);

	if (isAnimating)
	{
		m_osu->getSliderFrameBuffer()->enable();

#if defined(MCENGINE_FEATURE_OPENGL) || defined(MCENGINE_FEATURE_OPENGLES)

		if (isOpenGLRendererHack)
			glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // HACKHACK: OpenGL hardcoded

#endif
	}

	const bool isPlayingBeatmap = m_osu->isInPlayMode();

	// interactive sliders

	if (m_backgroundBrightnessSlider->isActive())
	{
		if (!isPlayingBeatmap)
		{
			const short brightness = clamp<float>(m_backgroundBrightnessSlider->getFloat(), 0.0f, 1.0f)*255.0f;
			if (brightness > 0)
			{
				g->setColor(COLOR(255, brightness, brightness, brightness));
				g->fillRect(0, 0, m_osu->getScreenWidth(), m_osu->getScreenHeight());
			}
		}
	}

	if (m_backgroundDimSlider->isActive())
	{
		if (!isPlayingBeatmap)
		{
			const short dim = clamp<float>(m_backgroundDimSlider->getFloat(), 0.0f, 1.0f)*255.0f;
			g->setColor(COLOR(dim, 0, 0, 0));
			g->fillRect(0, 0, m_osu->getScreenWidth(), m_osu->getScreenHeight());
		}
	}

	m_container->draw(g);

	if (m_hudSizeSlider->isActive() || m_hudComboScaleSlider->isActive() || m_hudScoreScaleSlider->isActive() || m_hudAccuracyScaleSlider->isActive() || m_hudHiterrorbarScaleSlider->isActive() || m_hudProgressbarScaleSlider->isActive() || m_hudScoreBoardScaleSlider->isActive() || m_hudInputoverlayScaleSlider->isActive() || m_statisticsOverlayScaleSlider->isActive())
	{
		if (!isPlayingBeatmap)
			m_osu->getHUD()->drawDummy(g);
	}
	else if (m_playfieldBorderSizeSlider->isActive())
	{
		m_osu->getHUD()->drawPlayfieldBorder(g, OsuGameRules::getPlayfieldCenter(m_osu), OsuGameRules::getPlayfieldSize(m_osu), 100);
	}
	else
		OsuScreenBackable::draw(g);

	if (m_cursorSizeSlider->getFloat() < 0.15f)
		engine->getMouse()->drawDebug(g);

	/*
	if (m_sliderQualitySlider->isActive())
	{
		Vector2 startPos = Vector2(50, 50);
		Vector2 size = Vector2((int)(m_osu->getUIScale(m_osu, 250.0f)), (int)(m_osu->getUIScale(m_osu, 250.0f)));
		const float hitcircleDiameter = m_osu->getUIScale(m_osu, 75.0f);
		const float length = size.x - hitcircleDiameter;
		const float pointDist = m_osu_slider_curve_points_separation->getFloat()*2;
		const int numPoints = length / pointDist;
		std::vector<Vector2> pointsMouth;
		std::vector<Vector2> pointsEyeLeft;
		std::vector<Vector2> pointsEyeRight;
		for (int i=0; i<numPoints+1; i++)
		{
			int heightAdd = i;
			if (i > numPoints/2)
				heightAdd = numPoints-i;
			float heightAddPercent = (float)heightAdd / (float)(numPoints/2.0f);
			float temp = 1.0f - heightAddPercent;
			temp *= temp;
			heightAddPercent = 1.0f - temp;

			if (i*pointDist < length/3 - hitcircleDiameter/6)
				pointsEyeLeft.push_back(Vector2(startPos.x + hitcircleDiameter/2 + i*pointDist, startPos.y + hitcircleDiameter/2 - (i*pointDist - (length/3 - hitcircleDiameter/6))));
			else if (i*pointDist > 2*(length/3) + hitcircleDiameter/6)
				pointsEyeRight.push_back(Vector2(startPos.x + hitcircleDiameter/2 + i*pointDist, startPos.y + hitcircleDiameter/2));

			Vector2 mouthOffset = Vector2(0, size.y/2);
			pointsMouth.push_back(Vector2(startPos.x + hitcircleDiameter/2 + i*pointDist, startPos.y + hitcircleDiameter/2 + heightAddPercent*(size.y/2 - hitcircleDiameter)) + mouthOffset);
		}

		g->setColor(0xff999999);
		g->fillRect(startPos.x - 5, startPos.y - 5, size.x + 8, size.y + 10);
		g->setColor(0xffffffff);
		g->fillRect(startPos.x, startPos.y, size.x/2, size.y);
		g->setColor(0xff000000);
		g->fillRect(startPos.x + size.x/2 - 1, startPos.y, size.x/2, size.y);

		OsuSliderRenderer::draw(g, m_osu, pointsEyeLeft, hitcircleDiameter);
		OsuSliderRenderer::draw(g, m_osu, pointsEyeRight, hitcircleDiameter);
		OsuSliderRenderer::draw(g, m_osu, pointsMouth, hitcircleDiameter);
	}
	*/

	if (isAnimating)
	{
		// HACKHACK:
		if (!m_bVisible)
			m_backButton->draw(g);

#if defined(MCENGINE_FEATURE_OPENGL) || defined(MCENGINE_FEATURE_OPENGLES)

		if (isOpenGLRendererHack)
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // HACKHACK: OpenGL hardcoded

#endif

		m_osu->getSliderFrameBuffer()->disable();

		g->push3DScene(McRect(0, 0, m_options->getSize().x, m_options->getSize().y));
		{
			g->rotate3DScene(0, -(1.0f - m_fAnimation)*90, 0);
			g->translate3DScene(-(1.0f - m_fAnimation)*m_options->getSize().x*1.25f, 0, -(1.0f - m_fAnimation)*700);

			m_osu->getSliderFrameBuffer()->setColor(COLORf(m_fAnimation, 1.0f, 1.0f, 1.0f));
			m_osu->getSliderFrameBuffer()->draw(g, 0, 0);
		}
		g->pop3DScene();
	}
}

void OsuOptionsMenu::update()
{
	OsuScreenBackable::update();

	// workshop background refresh takes some time, open context menu after loading is finished
	// do this even if options menu is invisible (but only show context menu if visible after finished)
	if (m_bWorkshopSkinSelectScheduled)
	{
		if (m_osu->getSteamWorkshop()->isReady())
		{
			m_bWorkshopSkinSelectScheduled = false;

			if (isVisible())
				onSkinSelectWorkshop3();
		}
	}

	if (!m_bVisible) return;

	if (m_bDPIScalingScrollToSliderScheduled)
	{
		m_bDPIScalingScrollToSliderScheduled = false;
		m_options->scrollToElement(m_uiScaleSlider, 0, 200 * Osu::getUIScale());
	}

	if (m_osu->getHUD()->isVolumeOverlayBusy() || m_backButton->isActive())
		m_container->stealFocus();

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

	// flash osu!folder textbox red if incorrect
	if (m_fOsuFolderTextboxInvalidAnim > engine->getTime())
	{
		char redness = std::abs(std::sin((m_fOsuFolderTextboxInvalidAnim - engine->getTime())*3))*128;
		m_osuFolderTextbox->setBackgroundColor(COLOR(255, redness, 0, 0));
	}
	else
		m_osuFolderTextbox->setBackgroundColor(0xff000000);

	// demo vibration strength while sliding
	if (m_vrVibrationStrengthSlider != NULL && m_vrVibrationStrengthSlider->isActive())
	{
		if (engine->getTime() > m_fVibrationStrengthExampleTimer)
		{
			m_fVibrationStrengthExampleTimer = engine->getTime() + 0.65f;

			openvr->getController()->triggerHapticPulse(m_osu->getVR()->getHapticPulseStrength());
		}
	}
	if (m_vrSliderVibrationStrengthSlider != NULL && m_vrSliderVibrationStrengthSlider->isActive())
		openvr->getController()->triggerHapticPulse(m_osu->getVR()->getSliderHapticPulseStrength());

	// hack to avoid entering search text while binding keys
	if (m_osu->getNotificationOverlay()->isVisible())
		m_fSearchOnCharKeybindHackTime = engine->getTime() + 0.1f;

	// highlight active category depending on scroll position
	if (m_categoryButtons.size() > 0)
	{
		OsuOptionsMenuCategoryButton *activeCategoryButton = m_categoryButtons[0];
		for (int i=0; i<m_categoryButtons.size(); i++)
		{
			OsuOptionsMenuCategoryButton *categoryButton = m_categoryButtons[i];
			if (categoryButton != NULL && categoryButton->getSection() != NULL)
			{
				categoryButton->setActiveCategory(false);
				categoryButton->setTextColor(0xff737373);

				if (categoryButton->getSection()->getPos().y < m_options->getSize().y*0.4)
					activeCategoryButton = categoryButton;
			}
		}
		if (activeCategoryButton != NULL)
		{
			activeCategoryButton->setActiveCategory(true);
			activeCategoryButton->setTextColor(0xffffffff);
		}
	}

	// delayed update letterboxing mouse scale/offset settings
	if (m_bLetterboxingOffsetUpdateScheduled)
	{
		if (!m_letterboxingOffsetXSlider->isActive() && !m_letterboxingOffsetYSlider->isActive())
		{
			m_bLetterboxingOffsetUpdateScheduled = false;

			m_osu_letterboxing_offset_x_ref->setValue(m_letterboxingOffsetXSlider->getFloat());
			m_osu_letterboxing_offset_y_ref->setValue(m_letterboxingOffsetYSlider->getFloat());

			// and update reset buttons as usual
			onResetUpdate(m_letterboxingOffsetResetButton);
		}
	}

	if (m_bUIScaleScrollToSliderScheduled)
	{
		m_bUIScaleScrollToSliderScheduled = false;
		m_options->scrollToElement(m_uiScaleSlider, 0, 200 * Osu::getUIScale());
	}

	// delayed UI scale change
	if (m_bUIScaleChangeScheduled)
	{
		if (!m_uiScaleSlider->isActive())
		{
			m_bUIScaleChangeScheduled = false;

			const float oldUIScale = Osu::getUIScale();

			m_osu_ui_scale_ref->setValue(m_uiScaleSlider->getFloat());

			const float newUIScale = Osu::getUIScale();

			// and update reset buttons as usual
			onResetUpdate(m_uiScaleResetButton);

			// additionally compensate scroll pos (but delay 1 frame)
			if (oldUIScale != newUIScale)
				m_bUIScaleScrollToSliderScheduled = true;
		}
	}

	// apply textbox changes on enter key
	if (m_osuFolderTextbox->hitEnter())
		updateOsuFolder();
	if (m_nameTextbox->hitEnter())
		updateName();
	if (m_dpiTextbox != NULL && m_dpiTextbox->hitEnter())
		updateFposuDPI();
	if (m_cm360Textbox != NULL && m_cm360Textbox->hitEnter())
		updateFposuCMper360();
}

void OsuOptionsMenu::onKeyDown(KeyboardEvent &e)
{
	if (!m_bVisible) return;

	m_container->onKeyDown(e);
	if (e.isConsumed()) return;

	if (e == KEY_ESCAPE || e == (KEYCODE)OsuKeyBindings::GAME_PAUSE.getInt())
	{
		if (m_contextMenu->isVisible())
		{
			e.consume();

			m_contextMenu->setVisible2(false);
			return;
		}
	}

	// searching text delete & escape key handling
	if (m_sSearchString.length() > 0)
	{
		switch (e.getKeyCode())
		{
		case KEY_DELETE:
		case KEY_BACKSPACE:
			if (m_sSearchString.length() > 0)
			{
				if (engine->getKeyboard()->isControlDown())
				{
					// delete everything from the current caret position to the left, until after the first non-space character (but including it)
					// TODO: use a CBaseUITextbox instead for the search box
					bool foundNonSpaceChar = false;
					while (m_sSearchString.length() > 0)
					{
						UString curChar = m_sSearchString.substr(m_sSearchString.length()-1, 1);

						if (foundNonSpaceChar && curChar.isWhitespaceOnly())
							break;

						if (!curChar.isWhitespaceOnly())
							foundNonSpaceChar = true;

						m_sSearchString.erase(m_sSearchString.length()-1, 1);
					}
				}
				else
					m_sSearchString = m_sSearchString.substr(0, m_sSearchString.length()-1);
				scheduleSearchUpdate();
			}
			break;
		case KEY_ESCAPE:
			m_sSearchString = "";
			scheduleSearchUpdate();
			break;
		}
	}
	else
	{
		if (e == KEY_ESCAPE || e == (KEYCODE)OsuKeyBindings::GAME_PAUSE.getInt())
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

	// handle searching
	if (e.getCharCode() < 32 || !m_bVisible || (engine->getKeyboard()->isControlDown() && !engine->getKeyboard()->isAltDown()) || m_fSearchOnCharKeybindHackTime > engine->getTime())
		return;

	KEYCODE charCode = e.getCharCode();
	UString stringChar = "";
	stringChar.insert(0, charCode);
	m_sSearchString.append(stringChar);

	scheduleSearchUpdate();

	e.consume();
}

void OsuOptionsMenu::onResolutionChange(Vector2 newResolution)
{
	OsuScreenBackable::onResolutionChange(newResolution);

	// HACKHACK: magic
	if ((env->getOS() == Environment::OS::OS_WINDOWS && env->isFullscreen() && env->isFullscreenWindowedBorderless() && (int)newResolution.y == (int)env->getNativeScreenSize().y+1))
		newResolution.y--;

	if (m_resolutionLabel != NULL)
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
	else // mania layout logic
	{
		if (e.getKeyCode() != (KEYCODE)0) // if not the first call
		{
			if (m_iManiaK > -1 && m_iManiaK < 10 && m_iManiaKey > -1 && m_iManiaKey <= m_iManiaK)
				OsuKeyBindings::MANIA[m_iManiaK][m_iManiaKey]->setValue(e.getKeyCode());

			// go to next key
			m_iManiaKey++;
			if (m_iManiaKey > m_iManiaK)
			{
				m_iManiaKey = 0;
				m_iManiaK++;
			}
		}

		if (m_iManiaK > -1 && m_iManiaK < 10)
		{
			UString notificationText = UString::format("%ik:", (m_iManiaK+1));
			int curKey = 0;
			for (int i=m_iManiaK-m_iManiaKey; i<m_iManiaK; i++)
			{
				notificationText.append(" [x]");
				curKey++;
			}
			for (int i=0; i<=m_iManiaK-m_iManiaKey; i++)
			{
				if (curKey == m_iManiaKey)
					notificationText.append(" ([?])");
				else
					notificationText.append(" []");

				curKey++;
			}
			m_osu->getNotificationOverlay()->addNotification(notificationText, 0xffffffff, true);
		}
	}
}

void OsuOptionsMenu::setVisible(bool visible)
{
	setVisibleInt(visible, false);
}

void OsuOptionsMenu::setVisibleInt(bool visible, bool fromOnBack)
{
	if (visible != m_bVisible)
	{
		// open/close animation
		if (!m_bFullscreen)
		{
			if (!m_bVisible)
				anim->moveQuartOut(&m_fAnimation, 1.0f, 0.25f*(1.0f - m_fAnimation), true);
			else
				anim->moveQuadOut(&m_fAnimation, 0.0f, 0.25f*m_fAnimation, true);
		}

		// save even if not closed via onBack(), e.g. if closed via setVisible(false) from outside
		if (!visible && !fromOnBack)
		{
			m_osu->getNotificationOverlay()->stopWaitingForKey();
			save();
		}
	}

	m_bVisible = visible;

	if (visible)
		updateLayout();
	else
	{
		m_contextMenu->setVisible2(false);

		//anim->deleteExistingAnimation(&m_fAnimation);
		//m_fAnimation = 0.0f;
	}

	// auto scroll to fposu settings if opening options while in fposu gamemode
	if (visible && m_osu->isInPlayMode() && m_osu_mod_fposu_ref->getBool() && !m_fposuCategoryButton->isActiveCategory())
		onCategoryClicked(m_fposuCategoryButton);

	// reset key bind reset counter
	if (visible)
		m_iNumResetAllKeyBindingsPressed = 0;
}

void OsuOptionsMenu::setUsername(UString username)
{
	m_nameTextbox->setText(username);
}

bool OsuOptionsMenu::isMouseInside()
{
	return (m_backButton->isMouseInside() || m_options->isMouseInside() || m_categories->isMouseInside()) && isVisible();
}

bool OsuOptionsMenu::isBusy()
{
	return (m_backButton->isActive() || m_options->isBusy() || m_categories->isBusy()) && isVisible();
}

bool OsuOptionsMenu::shouldDrawVRDummyHUD()
{
	return isVisible();
	/*return m_vrHudDistanceSlider != NULL && m_vrHudScaleSlider != NULL && (m_vrHudDistanceSlider->isActive() || m_vrHudScaleSlider->isActive());*/
}

void OsuOptionsMenu::updateLayout()
{
	// set all elements to the current convar values, and update the reset button states
	for (int i=0; i<m_elements.size(); i++)
	{
		switch (m_elements[i].type)
		{
		case 6: // checkbox
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
		case 7: // slider
			if (m_elements[i].cvar != NULL)
			{
				if (m_elements[i].elements.size() == 3)
				{
					CBaseUISlider *sliderPointer = dynamic_cast<CBaseUISlider*>(m_elements[i].elements[1]);
					if (sliderPointer != NULL)
					{
						sliderPointer->setValue(m_elements[i].cvar->getFloat(), false);
						sliderPointer->fireChangeCallback();
					}
				}
			}
			break;
		case 8: // textbox
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

		onResetUpdate(m_elements[i].resetButton);
	}

	if (m_fullscreenCheckbox != NULL)
		m_fullscreenCheckbox->setChecked(env->isFullscreen(), false);

	updateVRRenderTargetResolutionLabel();
	updateSkinNameLabel();

	if (m_outputDeviceLabel != NULL)
		m_outputDeviceLabel->setText(engine->getSound()->getOutputDevice());

	onOutputDeviceResetUpdate();

	//************************************************************************************************************************************//

	// TODO: correctly scale individual UI elements to dpiScale (depend on initial value in e.g. addCheckbox())

	OsuScreenBackable::updateLayout();

	const float dpiScale = Osu::getUIScale();

	m_container->setSize(m_osu->getScreenSize());

	// options panel
	const float optionsScreenWidthPercent = 0.5f;
	const float categoriesOptionsPercent = 0.135f;

	int optionsWidth = (int)(m_osu->getScreenWidth()*optionsScreenWidthPercent);
	if (!m_bFullscreen)
		optionsWidth = std::min((int)(725.0f*(1.0f - categoriesOptionsPercent)), optionsWidth) * dpiScale;

	const int categoriesWidth = optionsWidth*categoriesOptionsPercent;

	m_options->setRelPosX((!m_bFullscreen ? -1 : m_osu->getScreenWidth()/2 - (optionsWidth + categoriesWidth)/2) + categoriesWidth);
	m_options->setSize(optionsWidth, m_osu->getScreenHeight()+1);

	m_search->setRelPos(m_options->getRelPos());
	m_search->setSize(m_options->getSize());

	m_categories->setRelPosX(m_options->getRelPos().x - categoriesWidth);
	m_categories->setSize(categoriesWidth, m_osu->getScreenHeight() + 1);

	// reset
	m_options->getContainer()->empty();

	// build layout
	bool enableHorizontalScrolling = false;
	int sideMargin = 25*2 * dpiScale;
	int spaceSpacing = 25 * dpiScale;
	int sectionSpacing = -15 * dpiScale; // section title to first element
	int subsectionSpacing = 15 * dpiScale; // subsection title to first element
	int sectionEndSpacing = /*70*/120 * dpiScale; // last section element to next section title
	int subsectionEndSpacing = 65 * dpiScale; // last subsection element to next subsection title
	int elementSpacing = 5 * dpiScale;
	int elementTextStartOffset = 11 * dpiScale; // e.g. labels in front of sliders
	int yCounter = sideMargin + 20 * dpiScale;
	bool inSkipSection = false;
	bool inSkipSubSection = false;
	bool sectionTitleMatch = false;
	bool subSectionTitleMatch = false;
	std::string search = m_sSearchString.length() > 0 ? m_sSearchString.toUtf8() : "";
	for (int i=0; i<m_elements.size(); i++)
	{
		// searching logic happens here:
		// section
		//     content
		//     subsection
		//           content

		// case 1: match in content -> section + subsection + content     -> section + subsection matcher
		// case 2: match in content -> section + content                  -> section matcher
		// if match in section or subsection -> display entire section (disregard content match)
		// matcher is run through all remaining elements at every section + subsection

		if (m_sSearchString.length() > 0)
		{
			// if this is a section
			if (m_elements[i].type == 1)
			{
				bool sectionMatch = false;

				std::string sectionTitle = m_elements[i].elements[0]->getName().toUtf8();
				sectionTitleMatch = Osu::findIgnoreCase(sectionTitle, search);

				subSectionTitleMatch = false;
				if (inSkipSection)
					inSkipSection = false;

				for (int s=i+1; s<m_elements.size(); s++)
				{
					if (m_elements[s].type == 1) // stop at next section
						break;

					for (int e=0; e<m_elements[s].elements.size(); e++)
					{
						if (m_elements[s].elements[e]->getName().length() > 0)
						{
							std::string tags = m_elements[s].elements[e]->getName().toUtf8();

							if (Osu::findIgnoreCase(tags, search))
							{
								sectionMatch = true;
								break;
							}
						}
					}
				}

				inSkipSection = !sectionMatch;
				if (!inSkipSection)
					inSkipSubSection = false;
			}

			// if this is a subsection
			if (m_elements[i].type == 2)
			{
				bool subSectionMatch = false;

				std::string subSectionTitle = m_elements[i].elements[0]->getName().toUtf8();
				subSectionTitleMatch = Osu::findIgnoreCase(subSectionTitle, search);

				if (inSkipSubSection)
					inSkipSubSection = false;

				for (int s=i+1; s<m_elements.size(); s++)
				{
					if (m_elements[s].type == 2) // stop at next subsection
						break;

					for (int e=0; e<m_elements[s].elements.size(); e++)
					{
						if (m_elements[s].elements[e]->getName().length() > 0)
						{
							std::string tags = m_elements[s].elements[e]->getName().toUtf8();

							if (Osu::findIgnoreCase(tags, search))
							{
								subSectionMatch = true;
								break;
							}
						}
					}
				}

				inSkipSubSection = !subSectionMatch;
			}

			bool inSkipContent = false;
			if (!inSkipSection && !inSkipSubSection)
			{
				bool contentMatch = false;

				if (m_elements[i].type > 2)
				{
					for (int e=0; e<m_elements[i].elements.size(); e++)
					{
						if (m_elements[i].elements[e]->getName().length() > 0)
						{
							std::string tags = m_elements[i].elements[e]->getName().toUtf8();

							if (Osu::findIgnoreCase(tags, search))
							{
								contentMatch = true;
								break;
							}
						}
					}

					// if section or subsection titles match, then include all content of that (sub)section (even if content doesn't match)
					inSkipContent = !contentMatch;
				}
			}

			if ((inSkipSection || inSkipSubSection || inSkipContent) && !sectionTitleMatch && !subSectionTitleMatch)
				continue;
		}

		// add all elements of the current entry
		{
			// reset button
			if (m_elements[i].resetButton != NULL)
				m_options->getContainer()->addBaseUIElement(m_elements[i].resetButton);

			// (sub-)elements
			for (int e=0; e<m_elements[i].elements.size(); e++)
			{
				m_options->getContainer()->addBaseUIElement(m_elements[i].elements[e]);
			}
		}

		// and build the layout

		// if this element is a new section, add even more spacing
		if (i > 0 && m_elements[i].type == 1)
			yCounter += sectionEndSpacing;

		// if this element is a new subsection, add even more spacing
		if (i > 0 && m_elements[i].type == 2)
			yCounter += subsectionEndSpacing;

		const int elementWidth = optionsWidth - 2*sideMargin - 2 * dpiScale;
		const bool isKeyBindButton = (m_elements[i].type == 5);

		if (m_elements[i].resetButton != NULL)
		{
			CBaseUIButton *resetButton = m_elements[i].resetButton;
			resetButton->setSize(Vector2(35, 50) * dpiScale);
			resetButton->setRelPosY(yCounter);
			resetButton->setRelPosX(0);
		}

		for (int j=0; j<m_elements[i].elements.size(); j++)
		{
			CBaseUIElement *e = m_elements[i].elements[j];
			e->setSizeY(e->getRelSize().y * dpiScale);
		}

		if (m_elements[i].elements.size() == 1)
		{
			CBaseUIElement *e = m_elements[i].elements[0];

			int sideMarginAdd = 0;
			CBaseUILabel *labelPointer = dynamic_cast<CBaseUILabel*>(e);
			if (labelPointer != NULL)
			{
				labelPointer->onResized(); // HACKHACK: framework, setSize*() does not update string metrics
				labelPointer->setSizeToContent(0, 0);
				sideMarginAdd += elementTextStartOffset;
			}

			CBaseUIButton *buttonPointer = dynamic_cast<CBaseUIButton*>(e);
			if (buttonPointer != NULL)
				buttonPointer->onResized(); // HACKHACK: framework, setSize*() does not update string metrics

			CBaseUICheckbox *checkboxPointer = dynamic_cast<CBaseUICheckbox*>(e);
			if (checkboxPointer != NULL)
			{
				checkboxPointer->onResized(); // HACKHACK: framework, setWidth*() does not update string metrics
				checkboxPointer->setWidthToContent(0);
				if (checkboxPointer->getSize().x > elementWidth)
					enableHorizontalScrolling = true;
				else
					e->setSizeX(elementWidth);
			}
			else
				e->setSizeX(elementWidth);

			e->setRelPosX(sideMargin + sideMarginAdd);
			e->setRelPosY(yCounter);

			yCounter += e->getSize().y;
		}
		else if (m_elements[i].elements.size() == 2 || isKeyBindButton)
		{
			CBaseUIElement *e1 = m_elements[i].elements[0];
			CBaseUIElement *e2 = m_elements[i].elements[1];

			const int spacing = 15 * dpiScale;

			int sideMarginAdd = 0;
			CBaseUILabel *labelPointer = dynamic_cast<CBaseUILabel*>(e1);
			if (labelPointer != NULL)
				sideMarginAdd += elementTextStartOffset;

			CBaseUIButton *buttonPointer = dynamic_cast<CBaseUIButton*>(e1);
			if (buttonPointer != NULL)
				buttonPointer->onResized(); // HACKHACK: framework, setSize*() does not update string metrics

			if (isKeyBindButton)
			{
				CBaseUIElement *e3 = m_elements[i].elements[2];

				const float dividerBegin = 5.0f / 8.0f;
				const float dividerMiddle = 1.0f / 8.0f;
				const float dividerEnd = 2.0f / 8.0f;

				e1->setRelPos(sideMargin, yCounter);
				e1->setSizeX(elementWidth*dividerBegin - spacing);

				e2->setRelPos(sideMargin + e1->getSize().x + 0.5f*spacing, yCounter);
				e2->setSizeX(elementWidth*dividerMiddle - spacing);

				e3->setRelPos(sideMargin + e1->getSize().x + e2->getSize().x + 1.5f*spacing, yCounter);
				e3->setSizeX(elementWidth*dividerEnd - spacing);
			}
			else
			{
				float dividerEnd = 1.0f / 2.0f;
				float dividerBegin = 1.0f - dividerEnd;

				e1->setRelPos(sideMargin + sideMarginAdd, yCounter);
				e1->setSizeX(elementWidth*dividerBegin - spacing);

				e2->setRelPos(sideMargin + e1->getSize().x + 2*spacing, yCounter);
				e2->setSizeX(elementWidth*dividerEnd - spacing);
			}

			yCounter += e1->getSize().y;
		}
		else if (m_elements[i].elements.size() == 3)
		{
			CBaseUIElement *e1 = m_elements[i].elements[0];
			CBaseUIElement *e2 = m_elements[i].elements[1];
			CBaseUIElement *e3 = m_elements[i].elements[2];

			if (m_elements[i].type == 4)
			{
				const int buttonButtonLabelOffset = 10 * dpiScale;

				const int buttonSize = elementWidth / 3 - 2*buttonButtonLabelOffset;

				CBaseUIButton *button1Pointer = dynamic_cast<CBaseUIButton*>(e1);
				if (button1Pointer != NULL)
					button1Pointer->onResized(); // HACKHACK: framework, setSize*() does not update string metrics

				CBaseUIButton *button2Pointer = dynamic_cast<CBaseUIButton*>(e2);
				if (button2Pointer != NULL)
					button2Pointer->onResized(); // HACKHACK: framework, setSize*() does not update string metrics

				e1->setSizeX(buttonSize);
				e2->setSizeX(buttonSize);
				e3->setSizeX(buttonSize);

				e1->setRelPos(sideMargin, yCounter);
				e2->setRelPos(e1->getRelPos().x + e1->getSize().x + buttonButtonLabelOffset, yCounter);
				e3->setRelPos(e2->getRelPos().x + e2->getSize().x + buttonButtonLabelOffset, yCounter);
			}
			else
			{
				const int labelSliderLabelOffset = 15 * dpiScale;

				// this is a big mess, because some elements rely on fixed initial widths from default strings, combined with variable font dpi on startup, will clean up whenever
				CBaseUILabel *label1Pointer = dynamic_cast<CBaseUILabel*>(e1);
				if (label1Pointer != NULL)
				{
					label1Pointer->onResized(); // HACKHACK: framework, setSize*() does not update string metrics
					if (m_elements[i].label1Width > 0.0f)
						label1Pointer->setSizeX(m_elements[i].label1Width * dpiScale);
					else
						label1Pointer->setSizeX(label1Pointer->getRelSize().x * (96.0f / m_elements[i].relSizeDPI) * dpiScale);
				}

				CBaseUISlider *sliderPointer = dynamic_cast<CBaseUISlider*>(e2);
				if (sliderPointer != NULL)
					sliderPointer->setBlockSize(20 * dpiScale, 20 * dpiScale);

				CBaseUILabel *label2Pointer = dynamic_cast<CBaseUILabel*>(e3);
				if (label2Pointer != NULL)
				{
					label2Pointer->onResized(); // HACKHACK: framework, setSize*() does not update string metrics
					label2Pointer->setSizeX(label2Pointer->getRelSize().x * (96.0f / m_elements[i].relSizeDPI) * dpiScale);
				}

				int sliderSize = elementWidth - e1->getSize().x - e3->getSize().x;
				if (sliderSize < 100)
				{
					enableHorizontalScrolling = true;
					sliderSize = 100;
				}

				e1->setRelPos(sideMargin + elementTextStartOffset, yCounter);

				e2->setRelPos(e1->getRelPos().x + e1->getSize().x + labelSliderLabelOffset, yCounter);
				e2->setSizeX(sliderSize - 2*elementTextStartOffset - labelSliderLabelOffset*2);

				e3->setRelPos(e2->getRelPos().x + e2->getSize().x + labelSliderLabelOffset, yCounter);
			}

			yCounter += e2->getSize().y;
		}

		yCounter += elementSpacing;

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
	}
	m_spacer->setPosY(yCounter);
	m_options->getContainer()->addBaseUIElement(m_spacer);

	m_options->setScrollSizeToContent();
	m_options->setHorizontalScrolling(enableHorizontalScrolling);

	m_options->getContainer()->addBaseUIElement(m_contextMenu);

	m_options->getContainer()->update_pos();

	const int categoryPaddingTopBottom = m_categories->getSize().y*0.15f;
	const int categoryHeight = (m_categories->getSize().y - categoryPaddingTopBottom*2) / m_categoryButtons.size();
	for (int i=0; i<m_categoryButtons.size(); i++)
	{
		CBaseUIElement *category = m_categoryButtons[i];
		category->setRelPosY(categoryPaddingTopBottom + categoryHeight*i);
		category->setSize(m_categories->getSize().x-1, categoryHeight);
	}
	m_categories->getContainer()->update_pos();
	m_categories->setScrollSizeToContent();

	m_container->update_pos();
}

void OsuOptionsMenu::onBack()
{
	m_osu->getNotificationOverlay()->stopWaitingForKey();

	engine->getSound()->play(m_osu->getSkin()->getMenuClick());
	save();

	if (m_bFullscreen)
		m_osu->toggleOptionsMenu();
	else
		setVisibleInt(false, true);
}

void OsuOptionsMenu::scheduleSearchUpdate()
{
	updateLayout();
	m_options->scrollToTop();
	m_search->setSearchString(m_sSearchString);

	if (m_contextMenu->isVisible())
		m_contextMenu->setVisible2(false);
}

void OsuOptionsMenu::updateOsuFolder()
{
	m_osuFolderTextbox->stealFocus();

	// automatically insert a slash at the end if the user forgets
	UString newOsuFolder = m_osuFolderTextbox->getText();
	newOsuFolder = newOsuFolder.trim();
	if (newOsuFolder.length() > 0)
	{
		if (newOsuFolder[newOsuFolder.length()-1] != L'/' && newOsuFolder[newOsuFolder.length()-1] != L'\\')
		{
			newOsuFolder.append("/");
			m_osuFolderTextbox->setText(newOsuFolder);
		}
	}

	// set convar to new folder
	convar->getConVarByName("osu_folder")->setValue(newOsuFolder);
}

void OsuOptionsMenu::updateName()
{
	m_nameTextbox->stealFocus();
	convar->getConVarByName("name")->setValue(m_nameTextbox->getText());
}

void OsuOptionsMenu::updateFposuDPI()
{
	if (m_dpiTextbox == NULL) return;

	m_dpiTextbox->stealFocus();
	convar->getConVarByName("fposu_mouse_dpi")->setValue(m_dpiTextbox->getText());
}

void OsuOptionsMenu::updateFposuCMper360()
{
	if (m_cm360Textbox == NULL) return;

	m_cm360Textbox->stealFocus();
	convar->getConVarByName("fposu_mouse_cm_360")->setValue(m_cm360Textbox->getText());
}

void OsuOptionsMenu::updateVRRenderTargetResolutionLabel()
{
	if (m_vrRenderTargetResolutionLabel == NULL || !openvr->isReady() || !m_osu->isInVRMode()) return;

	Vector2 vrRenderTargetResolution = openvr->getRenderTargetResolution();
	m_vrRenderTargetResolutionLabel->setText(UString::format(m_vrRenderTargetResolutionLabel->getName().toUtf8(), (int)vrRenderTargetResolution.x, (int)vrRenderTargetResolution.y));
}

void OsuOptionsMenu::updateSkinNameLabel()
{
	if (m_skinLabel == NULL) return;

	m_skinLabel->setText(m_osu_skin_is_from_workshop_ref->getBool() ? m_osu_skin_workshop_title_ref->getString() : m_osu_skin_ref->getString());
	m_skinLabel->setTextColor(m_osu_skin_is_from_workshop_ref->getBool() ? 0xff37adff : 0xffffffff);
}

void OsuOptionsMenu::onFullscreenChange(CBaseUICheckbox *checkbox)
{
	if (checkbox->isChecked())
		env->enableFullscreen();
	else
		env->disableFullscreen();

	// and update reset button as usual
	for (int i=0; i<m_elements.size(); i++)
	{
		for (int e=0; e<m_elements[i].elements.size(); e++)
		{
			if (m_elements[i].elements[e] == checkbox)
			{
				onResetUpdate(m_elements[i].resetButton);

				break;
			}
		}
	}
}

void OsuOptionsMenu::onBorderlessWindowedChange(CBaseUICheckbox *checkbox)
{
	onCheckboxChange(checkbox);

	if (checkbox->isChecked() && !env->isFullscreen())
		env->enableFullscreen();

	// and update reset button as usual
	for (int i=0; i<m_elements.size(); i++)
	{
		for (int e=0; e<m_elements[i].elements.size(); e++)
		{
			if (m_elements[i].elements[e] == checkbox)
			{
				onResetUpdate(m_elements[i].resetButton);

				break;
			}
		}
	}
}

void OsuOptionsMenu::onDPIScalingChange(CBaseUICheckbox *checkbox)
{
	for (int i=0; i<m_elements.size(); i++)
	{
		for (int e=0; e<m_elements[i].elements.size(); e++)
		{
			if (m_elements[i].elements[e] == checkbox)
			{
				const float prevUIScale = Osu::getUIScale();

				if (m_elements[i].cvar != NULL)
					m_elements[i].cvar->setValue(checkbox->isChecked());

				onResetUpdate(m_elements[i].resetButton);

				if (Osu::getUIScale() != prevUIScale)
					m_bDPIScalingScrollToSliderScheduled = true;

				break;
			}
		}
	}
}

void OsuOptionsMenu::onRawInputToAbsoluteWindowChange(CBaseUICheckbox *checkbox)
{
	for (int i=0; i<m_elements.size(); i++)
	{
		for (int e=0; e<m_elements[i].elements.size(); e++)
		{
			if (m_elements[i].elements[e] == checkbox)
			{
				if (m_elements[i].cvar != NULL)
					m_elements[i].cvar->setValue(checkbox->isChecked());

				onResetUpdate(m_elements[i].resetButton);

				// special case: this requires a virtual mouse offset update, but since it is an engine convar we can't use callbacks
				m_osu->updateMouseSettings();

				break;
			}
		}
	}
}

void OsuOptionsMenu::onSkinSelect()
{
	updateOsuFolder();

	if (m_osu->isSkinLoading()) return;


	UString skinFolder = convar->getConVarByName("osu_folder")->getString();
	skinFolder.append(convar->getConVarByName("osu_folder_sub_skins")->getString());
	std::vector<UString> skinFolders = env->getFoldersInFolder(skinFolder);




	/*
	std::vector<UString> skinFolders;
	std::vector<uint64_t> subscribedItems = steam->getWorkshopSubscribedItems();
	debugLog("subscribed to %i items\n", subscribedItems.size());
	for (int i=0; i<subscribedItems.size(); i++)
	{
		SteamworksInterface::WorkshopItemDetails details = steam->getWorkshopItemDetails(subscribedItems[i]);
		debugLog("item[%i] = %llu, installInfo = %s, title = %s, desc = %s\n", i, subscribedItems[i], steam->getWorkshopItemInstallInfo(subscribedItems[i]).toUtf8(), details.title.toUtf8(), details.description.toUtf8());
		skinFolders.push_back(details.title);
	}
	*/



	if (skinFolders.size() > 0)
	{
		m_contextMenu->setPos(m_skinSelectLocalButton->getPos());
		m_contextMenu->setRelPos(m_skinSelectLocalButton->getRelPos());
		m_contextMenu->begin();
		m_contextMenu->addButton("default");
		m_contextMenu->addButton("defaultvr");
		for (int i=0; i<skinFolders.size(); i++)
		{
			if (skinFolders[i] == "." || skinFolders[i] == "..") // is this universal in every file system? too lazy to check. should probably fix this in the engine and not here
				continue;

			m_contextMenu->addButton(skinFolders[i]);
		}
		m_contextMenu->end();
		m_contextMenu->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSkinSelect2) );
	}
	else
	{
		m_osu->getNotificationOverlay()->addNotification("Error: Couldn't find any skins", 0xffff0000);
		m_options->scrollToTop();
		m_fOsuFolderTextboxInvalidAnim = engine->getTime() + 3.0f;
	}
}

void OsuOptionsMenu::onSkinSelect2(UString skinName, int id)
{
	m_osu_skin_is_from_workshop_ref->setValue(0.0f);

	if (m_osu->getInstanceID() < 1)
		m_osu_skin_ref->setValue(skinName);
	else
		m_osu->setSkin(skinName);

	updateSkinNameLabel();
}

void OsuOptionsMenu::onSkinSelectWorkshop()
{
	if (m_skinSelectWorkshopButton == NULL) return;

	if (m_osu->getSteamWorkshop()->isReady())
	{
		onSkinSelectWorkshop3();
		return;
	}

	onSkinSelectWorkshop2();
}

void OsuOptionsMenu::onSkinSelectWorkshop2()
{
	m_osu->getSteamWorkshop()->refresh();
	m_bWorkshopSkinSelectScheduled = true;

	m_contextMenu->setPos(m_skinSelectWorkshopButton->getPos());
	m_contextMenu->setRelPos(m_skinSelectWorkshopButton->getRelPos());
	m_contextMenu->begin();
	{
		m_contextMenu->addButton("(Loading, please wait ...)", -4)->setEnabled(false);
	}
	m_contextMenu->end();
}

void OsuOptionsMenu::onSkinSelectWorkshop3()
{
	m_contextMenu->setPos(m_skinSelectWorkshopButton->getPos());
	m_contextMenu->setRelPos(m_skinSelectWorkshopButton->getRelPos());
	m_contextMenu->begin();
	{
		m_contextMenu->addButton(">>> Refresh <<<", -2)->setTextLeft(false);
		m_contextMenu->addButton("", -4)->setEnabled(false);

		const std::vector<OsuSteamWorkshop::SUBSCRIBED_ITEM> &subscribedItems = m_osu->getSteamWorkshop()->getSubscribedItems();
		if (subscribedItems.size() > 0)
		{
			for (int i=0; i<subscribedItems.size(); i++)
			{
				if (subscribedItems[i].status != OsuSteamWorkshop::SUBSCRIBED_ITEM_STATUS::INSTALLED)
					m_contextMenu->addButton("(Downloading ...)", -4)->setEnabled(false)->setTextColor(0xff888888)->setTextDarkColor(0xff000000);
				else
					m_contextMenu->addButton(subscribedItems[i].title, i);
			}
		}
		else
			m_contextMenu->addButton("(Empty. Click for Workshop!)", -3)->setTextLeft(false);
	}
	m_contextMenu->end();
	m_contextMenu->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSkinSelectWorkshop4) );
}

void OsuOptionsMenu::onSkinSelectWorkshop4(UString skinName, int id)
{
	if (id == -2) // ">>> Refresh <<<"
	{
		onSkinSelectWorkshop2();
		return;
	}
	else if (id == -3) // "(Empty. Click for Workshop!)"
	{
		OsuMainMenu::openSteamWorkshopInGameOverlay(m_osu, true);
		return;
	}

	const std::vector<OsuSteamWorkshop::SUBSCRIBED_ITEM> &subscribedItems = m_osu->getSteamWorkshop()->getSubscribedItems();
	if (id >= 0 && id < subscribedItems.size())
	{
		const OsuSteamWorkshop::SUBSCRIBED_ITEM &subscribedItem = subscribedItems[id];
		debugLog("installInfo = %s\n", subscribedItem.installInfo.toUtf8());
		if (env->directoryExists(subscribedItem.installInfo))
		{
			m_osu_skin_is_from_workshop_ref->setValue(1.0f);
			m_osu_skin_workshop_title_ref->setValue(subscribedItem.title);
			m_osu_skin_workshop_id_ref->setValue(UString::format("%llu", subscribedItem.id));

			if (m_osu->getInstanceID() < 1)
				m_osu_skin_ref->setValue(subscribedItem.installInfo);
			else
				m_osu->setSkin(subscribedItem.installInfo);

			updateSkinNameLabel();
		}
		else
			m_osu->getNotificationOverlay()->addNotification("Error: Workshop skin does not exist!", 0xffff0000);
	}
}

void OsuOptionsMenu::onSkinReload()
{
	m_osu->reloadSkin();
}

void OsuOptionsMenu::onResolutionSelect()
{
	std::vector<Vector2> resolutions;

	// 4:3
	resolutions.push_back(Vector2(800, 600));
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

    // get custom resolutions
    std::vector<Vector2> customResolutions;
    std::ifstream customres("cfg/customres.cfg");
	std::string curLine;
	while (std::getline(customres, curLine))
	{
		const char *curLineChar = curLine.c_str();
		if (curLine.find("//") == std::string::npos) // ignore comments
		{
			int width = 0;
			int height = 0;
			if (sscanf(curLineChar, "%ix%i\n", &width, &height) == 2)
			{
				if (width > 319 && height > 239) // 320x240 sanity check
					customResolutions.push_back(Vector2(width, height));
			}
		}
	}

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

		m_contextMenu->addButton(UString::format("%ix%i", (int)std::round(resolutions[i].x), (int)std::round(resolutions[i].y)));
	}
	for (int i=0; i<customResolutions.size(); i++)
	{
		m_contextMenu->addButton(UString::format("%ix%i", (int)std::round(customResolutions[i].x), (int)std::round(customResolutions[i].y)));
	}
	m_contextMenu->end();
	m_contextMenu->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onResolutionSelect2) );
}

void OsuOptionsMenu::onResolutionSelect2(UString resolution, int id)
{
	if (env->isFullscreen())
		convar->getConVarByName("osu_resolution")->setValue(resolution);
	else
		convar->getConVarByName("windowed")->execArgs(resolution);
}

void OsuOptionsMenu::onOutputDeviceSelect()
{
	std::vector<UString> outputDevices = engine->getSound()->getOutputDevices();

    // build context menu
	m_contextMenu->setPos(m_outputDeviceSelectButton->getPos());
	m_contextMenu->setRelPos(m_outputDeviceSelectButton->getRelPos());
	m_contextMenu->begin();
	for (int i=0; i<outputDevices.size(); i++)
	{
		m_contextMenu->addButton(outputDevices[i]);
	}
	m_contextMenu->end();
	m_contextMenu->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onOutputDeviceSelect2) );
}

void OsuOptionsMenu::onOutputDeviceSelect2(UString outputDeviceName, int id)
{
	engine->getSound()->setOutputDevice(outputDeviceName);
	m_outputDeviceLabel->setText(engine->getSound()->getOutputDevice());
	m_osu->reloadSkin(); // needed to reload sounds

	// and update reset button as usual
	onOutputDeviceResetUpdate();
}

void OsuOptionsMenu::onOutputDeviceResetClicked()
{
	if (engine->getSound()->getOutputDevices().size() > 0)
		onOutputDeviceSelect2(engine->getSound()->getOutputDevices()[0]);
}

void OsuOptionsMenu::onOutputDeviceResetUpdate()
{
	if (m_outputDeviceResetButton != NULL)
		m_outputDeviceResetButton->setEnabled(engine->getSound()->getOutputDevices().size() > 0 && engine->getSound()->getOutputDevice() != engine->getSound()->getOutputDevices()[0]);
}

void OsuOptionsMenu::onOutputDeviceRestart()
{
	engine->getSound()->setOutputDevice("Default");
}

void OsuOptionsMenu::onDownloadOsuClicked()
{
	if (env->getOS() == Environment::OS::OS_HORIZON)
	{
		m_osu->getNotificationOverlay()->addNotification("Go to https://osu.ppy.sh/", 0xffffffff, false, 0.75f);
		return;
	}

	m_osu->getNotificationOverlay()->addNotification("Opening browser, please wait ...", 0xffffffff, false, 0.75f);
	env->openURLInDefaultBrowser("https://osu.ppy.sh/");
}

void OsuOptionsMenu::onManuallyManageBeatmapsClicked()
{
	if (env->getOS() == Environment::OS::OS_HORIZON)
	{
		m_osu->getNotificationOverlay()->addNotification("Google \"How to use McOsu without osu!\"", 0xffffffff, false, 0.75f);
		return;
	}

	m_osu->getNotificationOverlay()->addNotification("Opening browser, please wait ...", 0xffffffff, false, 0.75f);
	env->openURLInDefaultBrowser("https://steamcommunity.com/sharedfiles/filedetails/?id=880768265");
}

void OsuOptionsMenu::onLIVReloadCalibrationClicked()
{
	convar->getConVarByName("vr_liv_reload_calibration")->exec();
}

void OsuOptionsMenu::onCM360CalculatorLinkClicked()
{
	if (env->getOS() == Environment::OS::OS_HORIZON)
	{
		m_osu->getNotificationOverlay()->addNotification("Go to https://www.mouse-sensitivity.com/", 0xffffffff, false, 0.75f);
		return;
	}

	m_osu->getNotificationOverlay()->addNotification("Opening browser, please wait ...", 0xffffffff, false, 0.75f);
	env->openURLInDefaultBrowser("https://www.mouse-sensitivity.com/");
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

				onResetUpdate(m_elements[i].resetButton);

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

				onResetUpdate(m_elements[i].resetButton);

				break;
			}
		}
	}
}

void OsuOptionsMenu::onSliderChangeOneDecimalPlace(CBaseUISlider *slider)
{
	for (int i=0; i<m_elements.size(); i++)
	{
		for (int e=0; e<m_elements[i].elements.size(); e++)
		{
			if (m_elements[i].elements[e] == slider)
			{
				if (m_elements[i].cvar != NULL)
					m_elements[i].cvar->setValue(std::round(slider->getFloat()*10.0f)/10.0f); // round to 1 decimal place

				if (m_elements[i].elements.size() == 3)
				{
					CBaseUILabel *labelPointer = dynamic_cast<CBaseUILabel*>(m_elements[i].elements[2]);
					labelPointer->setText(m_elements[i].cvar->getString());
				}

				onResetUpdate(m_elements[i].resetButton);

				break;
			}
		}
	}
}

void OsuOptionsMenu::onSliderChangeOneDecimalPlaceMeters(CBaseUISlider *slider)
{
	for (int i=0; i<m_elements.size(); i++)
	{
		for (int e=0; e<m_elements[i].elements.size(); e++)
		{
			if (m_elements[i].elements[e] == slider)
			{
				if (m_elements[i].cvar != NULL)
					m_elements[i].cvar->setValue(std::round(slider->getFloat()*10.0f)/10.0f); // round to 1 decimal place

				if (m_elements[i].elements.size() == 3)
				{
					CBaseUILabel *labelPointer = dynamic_cast<CBaseUILabel*>(m_elements[i].elements[2]);
					labelPointer->setText(UString::format("%.1f m", m_elements[i].cvar->getFloat()));
				}

				onResetUpdate(m_elements[i].resetButton);

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

				onResetUpdate(m_elements[i].resetButton);

				break;
			}
		}
	}
}

void OsuOptionsMenu::onSliderChangeIntMS(CBaseUISlider *slider)
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
					UString text = m_elements[i].cvar->getString();
					text.append(" ms");
					labelPointer->setText(text);
				}

				onResetUpdate(m_elements[i].resetButton);

				break;
			}
		}
	}
}

void OsuOptionsMenu::onSliderChangeFloatMS(CBaseUISlider *slider)
{
	for (int i=0; i<m_elements.size(); i++)
	{
		for (int e=0; e<m_elements[i].elements.size(); e++)
		{
			if (m_elements[i].elements[e] == slider)
			{
				if (m_elements[i].cvar != NULL)
					m_elements[i].cvar->setValue(slider->getFloat());

				if (m_elements[i].elements.size() == 3)
				{
					CBaseUILabel *labelPointer = dynamic_cast<CBaseUILabel*>(m_elements[i].elements[2]);
					UString text = UString::format("%i", (int)std::round(m_elements[i].cvar->getFloat()*1000.0f));
					text.append(" ms");
					labelPointer->setText(text);
				}

				onResetUpdate(m_elements[i].resetButton);

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

				onResetUpdate(m_elements[i].resetButton);

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

void OsuOptionsMenu::onKeyUnbindButtonPressed(CBaseUIButton *button)
{
	for (int i=0; i<m_elements.size(); i++)
	{
		for (int e=0; e<m_elements[i].elements.size(); e++)
		{
			if (m_elements[i].elements[e] == button)
			{
				if (m_elements[i].cvar != NULL)
				{
					m_elements[i].cvar->setValue(0.0f);
				}
				break;
			}
		}
	}
}

void OsuOptionsMenu::onKeyBindingsResetAllPressed(CBaseUIButton *button)
{
	m_iNumResetAllKeyBindingsPressed++;

	const int numRequiredPressesUntilReset = 4;
	const int remainingUntilReset = numRequiredPressesUntilReset - m_iNumResetAllKeyBindingsPressed;

	if (m_iNumResetAllKeyBindingsPressed > (numRequiredPressesUntilReset-1))
	{
		m_iNumResetAllKeyBindingsPressed = 0;

		for (ConVar *bind : OsuKeyBindings::ALL)
		{
			bind->setValue(bind->getDefaultFloat());
		}

		m_osu->getNotificationOverlay()->addNotification("All key bindings have been reset.", 0xff00ff00);
	}
	else
	{
		if (remainingUntilReset > 1)
			m_osu->getNotificationOverlay()->addNotification(UString::format("Press %i more times to confirm.", remainingUntilReset));
		else
			m_osu->getNotificationOverlay()->addNotification(UString::format("Press %i more time to confirm!", remainingUntilReset), 0xffffff00);
	}
}

void OsuOptionsMenu::onKeyBindingManiaPressedInt()
{
	onKeyBindingManiaPressed(NULL);
}

void OsuOptionsMenu::onKeyBindingManiaPressed(CBaseUIButton *button)
{
	m_waitingKey = NULL;
	m_iManiaK = 0;
	m_iManiaKey = 0;

	KeyboardEvent e(0);
	onKey(e);
}

void OsuOptionsMenu::onSliderChangeVRSuperSampling(CBaseUISlider *slider)
{
	for (int i=0; i<m_elements.size(); i++)
	{
		for (int e=0; e<m_elements[i].elements.size(); e++)
		{
			if (m_elements[i].elements[e] == slider)
			{
				if (m_elements[i].cvar != NULL)
					m_elements[i].cvar->setValue(std::round(slider->getFloat()*10.0f)/10.0f); // round to 1 decimal place

				if (m_elements[i].elements.size() == 3)
				{
					CBaseUILabel *labelPointer = dynamic_cast<CBaseUILabel*>(m_elements[i].elements[2]);
					UString labelText = m_elements[i].cvar->getString();
					labelText.append("x");
					labelPointer->setText(labelText);
				}

				onResetUpdate(m_elements[i].resetButton);

				break;
			}
		}
	}

	updateVRRenderTargetResolutionLabel();
}

void OsuOptionsMenu::onSliderChangeVRAntiAliasing(CBaseUISlider *slider)
{
	for (int i=0; i<m_elements.size(); i++)
	{
		for (int e=0; e<m_elements[i].elements.size(); e++)
		{
			if (m_elements[i].elements[e] == slider)
			{
				int number = std::round(slider->getFloat()); // round to int
				int aa = 0;
				if (number > 8)
					aa = 16;
				else if (number > 4)
					aa = 8;
				else if (number > 2)
					aa = 4;
				else if (number > 0)
					aa = 2;

				if (m_elements[i].cvar != NULL)
					m_elements[i].cvar->setValue(aa);

				if (m_elements[i].elements.size() == 3)
				{
					CBaseUILabel *labelPointer = dynamic_cast<CBaseUILabel*>(m_elements[i].elements[2]);
					labelPointer->setText(UString::format("%ix", aa));
				}

				onResetUpdate(m_elements[i].resetButton);

				break;
			}
		}
	}
}

void OsuOptionsMenu::onSliderChangeSliderQuality(CBaseUISlider *slider)
{
	for (int i=0; i<m_elements.size(); i++)
	{
		for (int e=0; e<m_elements[i].elements.size(); e++)
		{
			if (m_elements[i].elements[e] == slider)
			{
				if (m_elements[i].cvar != NULL)
				{
					m_elements[i].cvar->setValue(std::round(slider->getFloat()*100.0f)/100.0f); // round to 2 decimal places
				}

				if (m_elements[i].elements.size() == 3)
				{
					CBaseUILabel *labelPointer = dynamic_cast<CBaseUILabel*>(m_elements[i].elements[2]);

					int percent = std::round((slider->getPercent()) * 100.0f);
					UString text = UString::format(percent > 49 ? "%i !" : "%i", percent);
					labelPointer->setText(text);
				}

				onResetUpdate(m_elements[i].resetButton);

				break;
			}
		}
	}
}

void OsuOptionsMenu::onSliderChangeLetterboxingOffset(CBaseUISlider *slider)
{
	m_bLetterboxingOffsetUpdateScheduled = true;

	for (int i=0; i<m_elements.size(); i++)
	{
		for (int e=0; e<m_elements[i].elements.size(); e++)
		{
			if (m_elements[i].elements[e] == slider)
			{
				const float newValue = std::round(slider->getFloat()*100.0f)/100.0f;

				if (m_elements[i].elements.size() == 3)
				{
					const int percent = std::round(newValue*100.0f);

					CBaseUILabel *labelPointer = dynamic_cast<CBaseUILabel*>(m_elements[i].elements[2]);
					labelPointer->setText(UString::format("%i%%", percent));
				}

				m_letterboxingOffsetResetButton = m_elements[i].resetButton; // HACKHACK: disgusting

				onResetUpdate(m_elements[i].resetButton);

				break;
			}
		}
	}
}

void OsuOptionsMenu::onSliderChangeUIScale(CBaseUISlider *slider)
{
	m_bUIScaleChangeScheduled = true;

	for (int i=0; i<m_elements.size(); i++)
	{
		for (int e=0; e<m_elements[i].elements.size(); e++)
		{
			if (m_elements[i].elements[e] == slider)
			{
				const float newValue = std::round(slider->getFloat()*100.0f)/100.0f;

				if (m_elements[i].elements.size() == 3)
				{
					const int percent = std::round(newValue*100.0f);

					CBaseUILabel *labelPointer = dynamic_cast<CBaseUILabel*>(m_elements[i].elements[2]);
					labelPointer->setText(UString::format("%i%%", percent));
				}

				m_uiScaleResetButton = m_elements[i].resetButton; // HACKHACK: disgusting

				onResetUpdate(m_elements[i].resetButton);

				break;
			}
		}
	}
}

void OsuOptionsMenu::onUseSkinsSoundSamplesChange(UString oldValue, UString newValue)
{
	m_osu->reloadSkin();
}

void OsuOptionsMenu::onHighQualitySlidersCheckboxChange(CBaseUICheckbox *checkbox)
{
	onCheckboxChange(checkbox);

	// special case: if the checkbox is clicked and enabled via the UI, force set the quality to 100
	if (checkbox->isChecked())
		m_sliderQualitySlider->setValue(1.0f, false);
}

void OsuOptionsMenu::onHighQualitySlidersConVarChange(UString oldValue, UString newValue)
{
	const bool enabled = newValue.toFloat() > 0;
	for (int i=0; i<m_elements.size(); i++)
	{
		bool contains = false;
		for (int e=0; e<m_elements[i].elements.size(); e++)
		{
			if (m_elements[i].elements[e] == m_sliderQualitySlider)
			{
				contains = true;
				break;
			}
		}

		if (contains)
		{
			// HACKHACK: show/hide quality slider, this is ugly as fuck
			// TODO: containers use setVisible() on all elements. there needs to be a separate API for hiding elements while inside any kind of container
			for (int e=0; e<m_elements[i].elements.size(); e++)
			{
				m_elements[i].elements[e]->setEnabled(enabled);

				OsuUISlider *sliderPointer = dynamic_cast<OsuUISlider*>(m_elements[i].elements[e]);
				CBaseUILabel *labelPointer = dynamic_cast<CBaseUILabel*>(m_elements[i].elements[e]);

				if (sliderPointer != NULL)
					sliderPointer->setFrameColor(enabled ? 0xffffffff : 0xff000000);
				if (labelPointer != NULL)
					labelPointer->setTextColor(enabled ? 0xffffffff : 0xff000000);
			}

			// reset value if disabled
			if (!enabled)
			{
				m_sliderQualitySlider->setValue(m_elements[i].cvar->getDefaultFloat(), false);
				m_elements[i].cvar->setValue(m_elements[i].cvar->getDefaultFloat());
			}

			onResetUpdate(m_elements[i].resetButton);

			break;
		}
	}
}

void OsuOptionsMenu::onCategoryClicked(CBaseUIButton *button)
{
	// reset search
	m_sSearchString = "";
	scheduleSearchUpdate();

	// scroll to category
	OsuOptionsMenuCategoryButton *categoryButton = dynamic_cast<OsuOptionsMenuCategoryButton*>(button);
	if (categoryButton != NULL)
		m_options->scrollToElement(categoryButton->getSection(), 0, 100 * Osu::getUIScale());
}

void OsuOptionsMenu::onResetUpdate(CBaseUIButton *button)
{
	if (button == NULL) return;

	for (int i=0; i<m_elements.size(); i++)
	{
		if (m_elements[i].resetButton == button && m_elements[i].cvar != NULL)
		{
			switch (m_elements[i].type)
			{
			case 6: // checkbox
				m_elements[i].resetButton->setEnabled(m_elements[i].cvar->getBool() != (bool)m_elements[i].cvar->getDefaultFloat());
				break;
			case 7: // slider
				m_elements[i].resetButton->setEnabled(m_elements[i].cvar->getFloat() != m_elements[i].cvar->getDefaultFloat());
				break;
			}

			break;
		}
	}
}

void OsuOptionsMenu::onResetClicked(CBaseUIButton *button)
{
	if (button == NULL) return;

	for (int i=0; i<m_elements.size(); i++)
	{
		if (m_elements[i].resetButton == button && m_elements[i].cvar != NULL)
		{
			switch (m_elements[i].type)
			{
			case 6: // checkbox
				for (int e=0; e<m_elements[i].elements.size(); e++)
				{
					CBaseUICheckbox *checkboxPointer = dynamic_cast<CBaseUICheckbox*>(m_elements[i].elements[e]);
					if (checkboxPointer != NULL)
						checkboxPointer->setChecked((bool)m_elements[i].cvar->getDefaultFloat());
				}
				break;
			case 7: // slider
				if (m_elements[i].elements.size() == 3)
				{
					CBaseUISlider *sliderPointer = dynamic_cast<CBaseUISlider*>(m_elements[i].elements[1]);
					if (sliderPointer != NULL)
					{
						sliderPointer->setValue(m_elements[i].cvar->getDefaultFloat(), false);
						sliderPointer->fireChangeCallback();
					}
				}
				break;
			}

			break;
		}
	}

	onResetUpdate(button);
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
	//label->setTextColor(0xff58dafe);
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

CBaseUILabel *OsuOptionsMenu::addLabel(UString text)
{
	CBaseUILabel *label = new CBaseUILabel(0, 0, m_options->getSize().x, 25, text, text);
	label->setSizeToContent(0, 0);
	label->setDrawFrame(false);
	label->setDrawBackground(false);
	m_options->getContainer()->addBaseUIElement(label);

	OPTIONS_ELEMENT e;
	e.elements.push_back(label);
	e.type = 3;
	e.cvar = NULL;
	m_elements.push_back(e);

	return label;
}

OsuUIButton *OsuOptionsMenu::addButton(UString text)
{
	OsuUIButton *button = new OsuUIButton(m_osu, 0, 0, m_options->getSize().x, 50, text, text);
	button->setColor(0xff0e94b5);
	button->setUseDefaultSkin();
	m_options->getContainer()->addBaseUIElement(button);

	OPTIONS_ELEMENT e;
	e.elements.push_back(button);
	e.type = 4;
	m_elements.push_back(e);

	return button;
}

OsuOptionsMenu::OPTIONS_ELEMENT OsuOptionsMenu::addButton(UString text, UString labelText, bool withResetButton)
{
	OsuUIButton *button = new OsuUIButton(m_osu, 0, 0, m_options->getSize().x, 50, text, text);
	button->setColor(0xff0e94b5);
	button->setUseDefaultSkin();
	m_options->getContainer()->addBaseUIElement(button);

	CBaseUILabel *label = new CBaseUILabel(0, 0, m_options->getSize().x, 50, labelText, labelText);
	label->setDrawFrame(false);
	label->setDrawBackground(false);
	m_options->getContainer()->addBaseUIElement(label);

	OPTIONS_ELEMENT e;
	if (withResetButton)
	{
		e.resetButton = new OsuOptionsMenuResetButton(m_osu, 0, 0, 35, 50, "", "");
	}
	e.elements.push_back(button);
	e.elements.push_back(label);
	e.type = 4;
	m_elements.push_back(e);

	return e;
}

OsuOptionsMenu::OPTIONS_ELEMENT OsuOptionsMenu::addButtonButtonLabel(UString text1, UString text2, UString labelText, bool withResetButton)
{
	OsuUIButton *button = new OsuUIButton(m_osu, 0, 0, m_options->getSize().x, 50, text1, text1);
	button->setColor(0xff0e94b5);
	button->setUseDefaultSkin();
	m_options->getContainer()->addBaseUIElement(button);

	OsuUIButton *button2 = new OsuUIButton(m_osu, 0, 0, m_options->getSize().x, 50, text2, text2);
	button2->setColor(0xff0e94b5);
	button2->setUseDefaultSkin();
	m_options->getContainer()->addBaseUIElement(button2);

	CBaseUILabel *label = new CBaseUILabel(0, 0, m_options->getSize().x, 50, labelText, labelText);
	label->setDrawFrame(false);
	label->setDrawBackground(false);
	m_options->getContainer()->addBaseUIElement(label);

	OPTIONS_ELEMENT e;
	if (withResetButton)
	{
		e.resetButton = new OsuOptionsMenuResetButton(m_osu, 0, 0, 35, 50, "", "");
	}
	e.elements.push_back(button);
	e.elements.push_back(button2);
	e.elements.push_back(label);
	e.type = 4;
	m_elements.push_back(e);

	return e;
}

OsuUIButton *OsuOptionsMenu::addKeyBindButton(UString text, ConVar *cvar)
{
	OsuUIButton *button = new OsuUIButton(m_osu, 0, 0, m_options->getSize().x, 50, text, text);
	button->setColor(0xff0e94b5);
	button->setUseDefaultSkin();
	button->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onKeyBindingButtonPressed) );
	m_options->getContainer()->addBaseUIElement(button);

	///UString iconString; iconString.insert(0, OsuIcons::UNDO);
	OsuUIButton *button2 = new OsuUIButton(m_osu, 0, 0, m_options->getSize().x, 50, text, "");
	button2->setColor(0x77ff0000);
	button2->setUseDefaultSkin();
	button2->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onKeyUnbindButtonPressed) );
	///button2->setFont(m_osu->getFontIcons());
	m_options->getContainer()->addBaseUIElement(button2);

	OsuOptionsMenuKeyBindLabel *label = new OsuOptionsMenuKeyBindLabel(0, 0, m_options->getSize().x, 50, "", "", cvar);
	label->setDrawFrame(false);
	label->setDrawBackground(false);
	m_options->getContainer()->addBaseUIElement(label);

	OPTIONS_ELEMENT e;
	e.elements.push_back(button);
	e.elements.push_back(button2);
	e.elements.push_back(label);
	e.type = 5;
	e.cvar = cvar;
	m_elements.push_back(e);

	return button;
}

CBaseUICheckbox *OsuOptionsMenu::addCheckbox(UString text, ConVar *cvar)
{
	return addCheckbox(text, "", cvar);
}

CBaseUICheckbox *OsuOptionsMenu::addCheckbox(UString text, UString tooltipText, ConVar *cvar)
{
	OsuUICheckbox *checkbox = new OsuUICheckbox(m_osu, 0, 0, m_options->getSize().x, 50, text, text);
	checkbox->setDrawFrame(false);
	checkbox->setDrawBackground(false);

	if (tooltipText.length() > 0)
		checkbox->setTooltipText(tooltipText);

	if (cvar != NULL)
	{
		checkbox->setChecked(cvar->getBool());
		checkbox->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onCheckboxChange) );
	}

	m_options->getContainer()->addBaseUIElement(checkbox);

	OPTIONS_ELEMENT e;
	if (cvar != NULL)
	{
		e.resetButton = new OsuOptionsMenuResetButton(m_osu, 0, 0, 35, 50, "", "");
		e.resetButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onResetClicked) );
	}
	e.elements.push_back(checkbox);
	e.type = 6;
	e.cvar = cvar;
	m_elements.push_back(e);

	return checkbox;
}

OsuUISlider *OsuOptionsMenu::addSlider(UString text, float min, float max, ConVar *cvar, float label1Width)
{
	OsuUISlider *slider = new OsuUISlider(m_osu, 0, 0, 100, 50, text);
	slider->setAllowMouseWheel(false);
	slider->setBounds(min, max);
	slider->setLiveUpdate(true);
	if (cvar != NULL)
	{
		slider->setValue(cvar->getFloat(), false);
		slider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onSliderChange) );
	}
	m_options->getContainer()->addBaseUIElement(slider);

	CBaseUILabel *label1 = new CBaseUILabel(0, 0, m_options->getSize().x, 50, text, text);
	label1->setDrawFrame(false);
	label1->setDrawBackground(false);
	label1->setWidthToContent();
	if (label1Width > 1)
		label1->setSizeX(label1Width);
	label1->setRelSizeX(label1->getSize().x);
	m_options->getContainer()->addBaseUIElement(label1);

	CBaseUILabel *label2 = new CBaseUILabel(0, 0, m_options->getSize().x, 50, "", "8.81");
	label2->setDrawFrame(false);
	label2->setDrawBackground(false);
	label2->setWidthToContent();
	label2->setRelSizeX(label2->getSize().x);
	m_options->getContainer()->addBaseUIElement(label2);

	OPTIONS_ELEMENT e;
	if (cvar != NULL)
	{
		e.resetButton = new OsuOptionsMenuResetButton(m_osu, 0, 0, 35, 50, "", "");
		e.resetButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onResetClicked) );
	}
	e.elements.push_back(label1);
	e.elements.push_back(slider);
	e.elements.push_back(label2);
	e.type = 7;
	e.cvar = cvar;
	e.label1Width = label1Width;
	e.relSizeDPI = label1->getFont()->getDPI();
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
	e.type = 8;
	e.cvar = cvar;
	m_elements.push_back(e);

	return textbox;
}

CBaseUITextbox *OsuOptionsMenu::addTextbox(UString text, UString labelText, ConVar *cvar)
{
	CBaseUITextbox *textbox = new CBaseUITextbox(0, 0, m_options->getSize().x, 40, "");
	textbox->setText(text);
	m_options->getContainer()->addBaseUIElement(textbox);

	CBaseUILabel *label = new CBaseUILabel(0, 0, m_options->getSize().x, 40, labelText, labelText);
	label->setDrawFrame(false);
	label->setDrawBackground(false);
	label->setWidthToContent();
	m_options->getContainer()->addBaseUIElement(label);

	OPTIONS_ELEMENT e;
	e.elements.push_back(label);
	e.elements.push_back(textbox);
	e.type = 8;
	e.cvar = cvar;
	m_elements.push_back(e);

	return textbox;
}

CBaseUIElement *OsuOptionsMenu::addSkinPreview()
{
	CBaseUIElement *skinPreview = new OsuOptionsMenuSkinPreviewElement(m_osu, 0, 0, 0, 200, "skincirclenumberhitresultpreview");
	m_options->getContainer()->addBaseUIElement(skinPreview);

	OPTIONS_ELEMENT e;
	e.elements.push_back(skinPreview);
	e.type = 9;
	e.cvar = NULL;
	m_elements.push_back(e);

	return skinPreview;
}

CBaseUIElement *OsuOptionsMenu::addSliderPreview()
{
	CBaseUIElement *sliderPreview = new OsuOptionsMenuSliderPreviewElement(m_osu, 0, 0, 0, 200, "skinsliderpreview");
	m_options->getContainer()->addBaseUIElement(sliderPreview);

	OPTIONS_ELEMENT e;
	e.elements.push_back(sliderPreview);
	e.type = 9;
	e.cvar = NULL;
	m_elements.push_back(e);

	return sliderPreview;
}

OsuOptionsMenuCategoryButton *OsuOptionsMenu::addCategory(CBaseUIElement *section, wchar_t icon)
{
	UString iconString; iconString.insert(0, icon);
	OsuOptionsMenuCategoryButton *button = new OsuOptionsMenuCategoryButton(section, 0, 0, 50, 50, "", iconString);
	button->setFont(m_osu->getFontIcons());
	button->setDrawBackground(false);
	button->setDrawFrame(false);
	button->setClickCallback( fastdelegate::MakeDelegate(this, &OsuOptionsMenu::onCategoryClicked) );
	m_categories->getContainer()->addBaseUIElement(button);
	m_categoryButtons.push_back(button);

	return button;
}

void OsuOptionsMenu::save()
{
	if (!osu_options_save_on_back.getBool())
	{
		debugLog("DEACTIVATED SAVE!!!! @ %f\n", engine->getTime());
		return;
	}

	updateOsuFolder();
	updateName();
	updateFposuDPI();
	updateFposuCMper360();

	debugLog("Osu: Saving user config file ...\n");

	UString userConfigFile = "cfg/";
	userConfigFile.append(OSU_CONFIG_FILE_NAME);
	userConfigFile.append(".cfg");

	// manual commands (e.g. fullscreen, windowed, osu_resolution); meaning commands which are not necessarily visible in the options menu, or which need special handling & ordering
	std::vector<ConVar*> manualConCommands;
	std::vector<ConVar*> manualConVars;
	std::vector<ConVar*> removeConCommands;

	manualConVars.push_back(convar->getConVarByName("osu_songbrowser_sortingtype"));
	manualConVars.push_back(convar->getConVarByName("osu_songbrowser_scores_sortingtype"));
	if (m_osu->isInVRMode())
		manualConVars.push_back(convar->getConVarByName("osu_vr_layout_lock"));

	removeConCommands.push_back(convar->getConVarByName("monitor"));
	removeConCommands.push_back(convar->getConVarByName("windowed"));
	removeConCommands.push_back(m_osu_skin_ref);
	removeConCommands.push_back(m_osu_skin_workshop_title_ref);
	removeConCommands.push_back(m_osu_skin_workshop_id_ref);
	removeConCommands.push_back(m_osu_skin_is_from_workshop_ref);
	removeConCommands.push_back(convar->getConVarByName("snd_output_device"));

	if (m_fullscreenCheckbox != NULL)
	{
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
	}

	// get user stuff in the config file
	std::vector<UString> keepLines;
	{
		// in extra block because the File class would block the following std::ofstream from writing to it until it's destroyed
		File in(userConfigFile.toUtf8());
		if (!in.canRead())
			debugLog("Osu Error: Couldn't read user config file!\n");
		else
		{
			while (in.canRead())
			{
				UString uLine = in.readLine();
				const char *lineChar = uLine.toUtf8();
				std::string line(lineChar);

				bool keepLine = true;
				for (int i=0; i<m_elements.size(); i++)
				{
					if (m_elements[i].cvar != NULL && line.find(m_elements[i].cvar->getName().toUtf8()) != std::string::npos)
					{
						// we don't want to remove custom convars which start with options entry convars (e.g. osu_rich_presence and osu_rich_presence_dynamic_windowtitle)
						// so, keep lines which only have partial matches

						const size_t firstSpaceIndex = line.find(" ");
						const size_t endOfConVarNameIndex = (firstSpaceIndex != std::string::npos ? firstSpaceIndex : line.length());

						if (std::string(m_elements[i].cvar->getName().toUtf8()).find(line.c_str(), 0, endOfConVarNameIndex) != std::string::npos)
						{
							keepLine = false;
							break;
						}
						//else
						//	debugLog("ignoring match %s with %s\n", m_elements[i].cvar->getName().toUtf8(), line.c_str());
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
		}
	}

	// write new config file
	// thankfully this path is relative and hardcoded, and thus not susceptible to unicode characters
	std::ofstream out(userConfigFile.toUtf8());
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
	out << "monitor " << env->getMonitor() << "\n";
	if (engine->getSound()->getOutputDevice() != "Default")
		out << "snd_output_device " << engine->getSound()->getOutputDevice().toUtf8() << "\n";
	if (m_fullscreenCheckbox != NULL && !m_fullscreenCheckbox->isChecked())
		out << "windowed " << engine->getScreenWidth() << "x" << engine->getScreenHeight() << "\n";

	// write options elements convars
	for (int i=0; i<m_elements.size(); i++)
	{
		if (m_elements[i].cvar != NULL)
		{
			out << m_elements[i].cvar->getName().toUtf8() << " " << m_elements[i].cvar->getString().toUtf8() << "\n";
		}
	}

	out << "osu_skin_mipmaps " << convar->getConVarByName("osu_skin_mipmaps")->getString().toUtf8() << "\n";
	if (m_osu_skin_is_from_workshop_ref->getBool())
	{
		out << "osu_skin_is_from_workshop " << m_osu_skin_is_from_workshop_ref->getString().toUtf8() << "\n";
		out << "osu_skin_workshop_title " << m_osu_skin_workshop_title_ref->getString().toUtf8() << "\n";
		out << "osu_skin_workshop_id " << m_osu_skin_workshop_id_ref->getString().toUtf8() << "\n";
	}
	out << "osu_skin " << m_osu_skin_ref->getString().toUtf8() << "\n";

	out.close();
}

void OsuOptionsMenu::openAndScrollToSkinSection()
{
	const bool wasVisible = isVisible();
	if (!wasVisible)
		m_osu->toggleOptionsMenu();

	if (!m_skinSelectLocalButton->isVisible() || !wasVisible)
		m_options->scrollToElement(m_skinSection, 0, 100 * Osu::getUIScale());
}
