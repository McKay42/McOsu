//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		settings
//
// $NoKeywords: $
//===============================================================================//

#ifndef OSUOPTIONSMENU_H
#define OSUOPTIONSMENU_H

#include "OsuScreenBackable.h"
#include "OsuNotificationOverlay.h"
#include "CBaseUIElement.h"

class Osu;

class OsuUIButton;
class OsuUISlider;
class OsuUIContextMenu;

class CBaseUIContainer;
class CBaseUIImageButton;
class CBaseUICheckbox;
class CBaseUIButton;
class CBaseUISlider;
class CBaseUIScrollView;
class CBaseUILabel;
class CBaseUITextbox;

class ConVar;

class OsuOptionsMenu : public OsuScreenBackable, public OsuNotificationOverlayKeyListener
{
public:
	OsuOptionsMenu(Osu *osu);
	virtual ~OsuOptionsMenu();

	void draw(Graphics *g);
	void update();

	void onKeyDown(KeyboardEvent &e);
	void onKeyUp(KeyboardEvent &e);
	void onChar(KeyboardEvent &e);

	void onResolutionChange(Vector2 newResolution);

	virtual void onKey(KeyboardEvent &e);

	void setVisible(bool visible);

private:
	virtual void updateLayout();
	void updateOsuFolder();
	void updateName();

	void save();

	virtual void onBack();
	void onFullscreenChange(CBaseUICheckbox *checkbox);
	void onSkinSelect();
	void onSkinSelect2(UString skinName);
	void onSkinReload();
	void onResolutionSelect();
	void onResolutionSelect2(UString resolution);

	void onCheckboxChange(CBaseUICheckbox *checkbox);
	void onSliderChange(CBaseUISlider *slider);
	void onSliderChangeInt(CBaseUISlider *slider);
	void onSliderChangePercent(CBaseUISlider *slider);
	void onKeyBindingButtonPressed(CBaseUIButton *button);

	struct OPTIONS_ELEMENT
	{
		std::vector<CBaseUIElement*> elements;
		ConVar *cvar;
		int type;
	};
	std::vector<OPTIONS_ELEMENT> m_elements;

	void addSpacer();
	CBaseUILabel *addSection(UString text);
	CBaseUILabel *addSubSection(UString text);
	OsuUIButton *addButton(UString text, ConVar *cvar = NULL);
	OPTIONS_ELEMENT addButton(UString text, UString labelText);
	CBaseUICheckbox *addCheckbox(UString text, ConVar *cvar = NULL);
	OsuUISlider *addSlider(UString text, float min = 0.0f, float max = 1.0f, ConVar *cvar = NULL, float label1Width = 0.0f);
	CBaseUITextbox *addTextbox(UString text, ConVar *cvar = NULL);
	CBaseUIElement *addSkinPreview();
	CBaseUIElement *addSliderPreview();

	Osu *m_osu;
	CBaseUIContainer *m_container;
	CBaseUIScrollView *m_options;

	// custom
	CBaseUICheckbox *m_fullscreenCheckbox;
	CBaseUISlider *m_backgroundBrightnessSlider;
	CBaseUISlider *m_hudSizeSlider;
	CBaseUISlider *m_hudComboScaleSlider;
	CBaseUISlider *m_hudScoreScaleSlider;
	CBaseUISlider *m_hudAccuracyScaleSlider;
	CBaseUISlider *m_hudHiterrorbarScaleSlider;
	CBaseUISlider *m_hudProgressbarScaleSlider;
	CBaseUISlider *m_playfieldBorderSizeSlider;
	CBaseUISlider *m_statisticsOverlayScaleSlider;
	CBaseUISlider *m_cursorSizeSlider;
	CBaseUILabel *m_skinLabel;
	CBaseUIElement *m_skinSelectButton;
	CBaseUIElement *m_resolutionSelectButton;
	CBaseUILabel *m_resolutionLabel;
	CBaseUITextbox *m_osuFolderTextbox;
	CBaseUITextbox *m_nameTextbox;

	ConVar *m_waitingKey;

	OsuUIContextMenu *m_contextMenu;

	float m_fOsuFolderTextboxInvalidAnim;
};

#endif
