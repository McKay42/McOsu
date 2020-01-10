//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		difficulty modifier selection screen
//
// $NoKeywords: $osums
//===============================================================================//

#ifndef OSUMODSELECTOR_H
#define OSUMODSELECTOR_H

#include "OsuScreen.h"

class Osu;
class OsuSkinImage;
class OsuSongBrowser;
class OsuBeatmapDifficulty;

class CBaseUIElement;
class CBaseUIContainer;
class CBaseUIScrollView;
class CBaseUIButton;
class CBaseUILabel;
class CBaseUISlider;
class CBaseUICheckbox;

class OsuUIModSelectorModButton;
class OsuUIButton;

class ConVar;

class OsuModSelector : public OsuScreen
{
public:
	OsuModSelector(Osu *osu);
	virtual ~OsuModSelector();

	virtual void draw(Graphics *g);
	virtual void update();

	virtual void onKeyDown(KeyboardEvent &key);
	virtual void onKeyUp(KeyboardEvent &key);

	virtual void onResolutionChange(Vector2 newResolution);

	virtual void setVisible(bool visible);

	void checkUpdateBPMSliderSlaves();
	void enableAuto();
	void toggleAuto();

	void setWaitForF1KeyUp(bool waitForF1KeyUp) {m_bWaitForF1KeyUp = waitForF1KeyUp;}

	bool isInCompactMode();
	bool isCSOverrideSliderActive();
	bool isMouseInScrollView();

	void updateModConVar();

private:
	struct OVERRIDE_SLIDER
	{
		OVERRIDE_SLIDER()
		{
			lock = NULL;
			desc = NULL;
			slider = NULL;
			label = NULL;
			cvar = NULL;
			lockCvar = NULL;
		}

		CBaseUICheckbox *lock;
		CBaseUIButton *desc;
		CBaseUISlider *slider;
		CBaseUILabel *label;
		ConVar *cvar;
		ConVar *lockCvar;
	};

	struct EXPERIMENTAL_MOD
	{
		CBaseUIElement *element;
		ConVar *cvar;
	};

	void updateButtons(bool initial = false);
	void updateExperimentalButtons(bool initial);
	void updateLayout();
	void updateExperimentalLayout();

	OsuUIModSelectorModButton *setModButtonOnGrid(int x, int y, int state, bool initialState, UString modName, UString tooltipText, std::function<OsuSkinImage*()> getImageFunc);
	OsuUIModSelectorModButton *getModButtonOnGrid(int x, int y);

	OVERRIDE_SLIDER addOverrideSlider(UString text, UString labelText, ConVar *cvar, float min, float max, ConVar *lockCvar = NULL);
	void onOverrideSliderChange(CBaseUISlider *slider);
	void onOverrideSliderLockChange(CBaseUICheckbox *checkbox);
	void onOverrideARSliderDescClicked(CBaseUIButton *button);
	void onOverrideODSliderDescClicked(CBaseUIButton *button);
	void updateOverrideSliderLabels();
	UString getOverrideSliderLabelText(OVERRIDE_SLIDER s, bool active);

	CBaseUILabel *addExperimentalLabel(UString text);
	CBaseUICheckbox *addExperimentalCheckbox(UString text, UString tooltipText, ConVar *cvar = NULL);
	void onCheckboxChange(CBaseUICheckbox *checkbox);

	OsuUIButton *addActionButton(UString text);

	void resetMods();
	void close();

	float m_fAnimation;
	float m_fExperimentalAnimation;
	bool m_bScheduledHide;
	bool m_bExperimentalVisible;
	CBaseUIContainer *m_container;
	CBaseUIContainer *m_overrideSliderContainer;
	CBaseUIScrollView *m_experimentalContainer;

	bool m_bWaitForF1KeyUp;

	bool m_bWaitForCSChangeFinished;
	bool m_bWaitForSpeedChangeFinished;

	// override sliders
	std::vector<OVERRIDE_SLIDER> m_overrideSliders;
	CBaseUISlider *m_CSSlider;
	CBaseUISlider *m_ARSlider;
	CBaseUISlider *m_ODSlider;
	CBaseUISlider *m_BPMSlider;
	CBaseUISlider *m_speedSlider;
	CBaseUICheckbox *m_ARLock;
	CBaseUICheckbox *m_ODLock;
	OsuBeatmapDifficulty *m_previousDifficulty;
	bool m_bShowOverrideSliderALTHint;

	// mod grid buttons
	int m_iGridWidth;
	int m_iGridHeight;
	std::vector<OsuUIModSelectorModButton*> m_modButtons;
	OsuUIModSelectorModButton *m_modButtonEasy;
	OsuUIModSelectorModButton *m_modButtonNofail;
	OsuUIModSelectorModButton *m_modButtonHalftime;
	OsuUIModSelectorModButton *m_modButtonHardrock;
	OsuUIModSelectorModButton *m_modButtonSuddendeath;
	OsuUIModSelectorModButton *m_modButtonDoubletime;
	OsuUIModSelectorModButton *m_modButtonHidden;
	OsuUIModSelectorModButton *m_modButtonFlashlight;
	OsuUIModSelectorModButton *m_modButtonRelax;
	OsuUIModSelectorModButton *m_modButtonAutopilot;
	OsuUIModSelectorModButton *m_modButtonSpunout;
	OsuUIModSelectorModButton *m_modButtonAuto;
	OsuUIModSelectorModButton *m_modButtonScoreV2;
	OsuUIModSelectorModButton *m_modButtonTD;

	// experimental mods
	std::vector<EXPERIMENTAL_MOD> m_experimentalMods;

	// action buttons
	std::vector<OsuUIButton*> m_actionButtons;
	OsuUIButton *m_resetModsButton;
	OsuUIButton *m_closeButton;

	// convar refs
	ConVar *m_osu_drain_enabled_ref;
	ConVar *m_osu_mod_touchdevice_ref;
};

#endif
