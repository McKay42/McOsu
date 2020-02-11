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

class Osu;

class OsuUIButton;
class OsuUISlider;
class OsuUIContextMenu;
class OsuUISearchOverlay;

class OsuOptionsMenuSliderPreviewElement;
class OsuOptionsMenuCategoryButton;
class OsuOptionsMenuResetButton;

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

	virtual void draw(Graphics *g);
	virtual void update();

	virtual void onKeyDown(KeyboardEvent &e);
	virtual void onKeyUp(KeyboardEvent &e);
	virtual void onChar(KeyboardEvent &e);

	virtual void onResolutionChange(Vector2 newResolution);

	virtual void onKey(KeyboardEvent &e);

	virtual void setVisible(bool visible);

	void save();

	void openAndScrollToSkinSection();

	void setFullscreen(bool fullscreen) {m_bFullscreen = fullscreen;}

	void setUsername(UString username);

	inline bool isFullscreen() const {return m_bFullscreen;}
	bool isMouseInside();
	bool isBusy();
	inline bool isWorkshopLoading() const {return m_bWorkshopSkinSelectScheduled;}

	bool shouldDrawVRDummyHUD();

private:
	static const char *OSU_CONFIG_FILE_NAME;

	struct OPTIONS_ELEMENT
	{
		OPTIONS_ELEMENT()
		{
			resetButton = NULL;
			cvar = NULL;
			type = -1;

			label1Width = 0.0f;
			relSizeDPI = 96.0f;

			allowOverscale = false;
			allowUnderscale = false;
		}

		OsuOptionsMenuResetButton *resetButton;
		std::vector<CBaseUIElement*> elements;
		ConVar *cvar;
		int type;

		float label1Width;
		float relSizeDPI;

		bool allowOverscale;
		bool allowUnderscale;
	};

	virtual void updateLayout();
	virtual void onBack();

	void setVisibleInt(bool visible, bool fromOnBack = false);
	void scheduleSearchUpdate();

	void updateOsuFolder();
	void updateName();
	void updateFposuDPI();
	void updateFposuCMper360();
	void updateVRRenderTargetResolutionLabel();
	void updateSkinNameLabel();
	void updateHPDrainSelectLabel();

	// options
	void onFullscreenChange(CBaseUICheckbox *checkbox);
	void onBorderlessWindowedChange(CBaseUICheckbox *checkbox);
	void onDPIScalingChange(CBaseUICheckbox *checkbox);
	void onRawInputToAbsoluteWindowChange(CBaseUICheckbox *checkbox);
	void onSkinSelect();
	void onSkinSelect2(UString skinName, int id = -1);
	void onSkinSelectWorkshop();
	void onSkinSelectWorkshop2();
	void onSkinSelectWorkshop3();
	void onSkinSelectWorkshop4(UString skinName, int id);
	void onSkinReload();
	void onSkinRandom();
	void onResolutionSelect();
	void onResolutionSelect2(UString resolution, int id = -1);
	void onOutputDeviceSelect();
	void onOutputDeviceSelect2(UString outputDeviceName, int id = -1);
	void onOutputDeviceResetClicked();
	void onOutputDeviceResetUpdate();
	void onOutputDeviceRestart();
	void onDownloadOsuClicked();
	void onManuallyManageBeatmapsClicked();
	void onLIVReloadCalibrationClicked();
	void onCM360CalculatorLinkClicked();
	void onHPDrainSelect();
	void onHPDrainSelect2(UString hpDrainType, int id = -1);
	void onHPDrainSelectResetClicked();
	void onHPDrainSelectResetUpdate();

	void onCheckboxChange(CBaseUICheckbox *checkbox);
	void onSliderChange(CBaseUISlider *slider);
	void onSliderChangeOneDecimalPlace(CBaseUISlider *slider);
	void onSliderChangeOneDecimalPlaceMeters(CBaseUISlider *slider);
	void onSliderChangeInt(CBaseUISlider *slider);
	void onSliderChangeIntMS(CBaseUISlider *slider);
	void onSliderChangeFloatMS(CBaseUISlider *slider);
	void onSliderChangePercent(CBaseUISlider *slider);

	void onKeyBindingButtonPressed(CBaseUIButton *button);
	void onKeyUnbindButtonPressed(CBaseUIButton *button);
	void onKeyBindingsResetAllPressed(CBaseUIButton *button);
	void onKeyBindingManiaPressedInt();
	void onKeyBindingManiaPressed(CBaseUIButton *button);
	void onSliderChangeVRSuperSampling(CBaseUISlider *slider);
	void onSliderChangeVRAntiAliasing(CBaseUISlider *slider);
	void onSliderChangeSliderQuality(CBaseUISlider *slider);
	void onSliderChangeLetterboxingOffset(CBaseUISlider *slider);
	void onSliderChangeUIScale(CBaseUISlider *slider);

	void onWASAPIBufferChange(CBaseUISlider *slider);
	void onWASAPIPeriodChange(CBaseUISlider *slider);

	void onUseSkinsSoundSamplesChange(UString oldValue, UString newValue);
	void onHighQualitySlidersCheckboxChange(CBaseUICheckbox *checkbox);
	void onHighQualitySlidersConVarChange(UString oldValue, UString newValue);

	// categories
	void onCategoryClicked(CBaseUIButton *button);

	// reset
	void onResetUpdate(CBaseUIButton *button);
	void onResetClicked(CBaseUIButton *button);

	// options
	void addSpacer();
	CBaseUILabel *addSection(UString text);
	CBaseUILabel *addSubSection(UString text);
	CBaseUILabel *addLabel(UString text);
	OsuUIButton *addButton(UString text);
	OPTIONS_ELEMENT addButton(UString text, UString labelText, bool withResetButton = false);
	OPTIONS_ELEMENT addButtonButton(UString text1, UString text2);
	OPTIONS_ELEMENT addButtonButtonLabel(UString text1, UString text2, UString labelText, bool withResetButton = false);
	OsuUIButton *addKeyBindButton(UString text, ConVar *cvar);
	CBaseUICheckbox *addCheckbox(UString text, ConVar *cvar);
	CBaseUICheckbox *addCheckbox(UString text, UString tooltipText = "", ConVar *cvar = NULL);
	OsuUISlider *addSlider(UString text, float min = 0.0f, float max = 1.0f, ConVar *cvar = NULL, float label1Width = 0.0f, bool allowOverscale = false, bool allowUnderscale = false);
	CBaseUITextbox *addTextbox(UString text, ConVar *cvar = NULL);
	CBaseUITextbox *addTextbox(UString text, UString labelText, ConVar *cvar = NULL);
	CBaseUIElement *addSkinPreview();
	CBaseUIElement *addSliderPreview();

	// categories
	OsuOptionsMenuCategoryButton *addCategory(CBaseUIElement *section, wchar_t icon);

	// vars
	Osu *m_osu;
	CBaseUIContainer *m_container;
	CBaseUIScrollView *m_categories;
	CBaseUIScrollView *m_options;
	OsuUIContextMenu *m_contextMenu;
	OsuUISearchOverlay *m_search;
	CBaseUILabel *m_spacer;
	OsuOptionsMenuCategoryButton *m_fposuCategoryButton;

	std::vector<OsuOptionsMenuCategoryButton*> m_categoryButtons;
	std::vector<OPTIONS_ELEMENT> m_elements;

	// custom
	bool m_bFullscreen;
	float m_fAnimation;

	CBaseUICheckbox *m_fullscreenCheckbox;
	CBaseUISlider *m_backgroundDimSlider;
	CBaseUISlider *m_backgroundBrightnessSlider;
	CBaseUISlider *m_hudSizeSlider;
	CBaseUISlider *m_hudComboScaleSlider;
	CBaseUISlider *m_hudScoreScaleSlider;
	CBaseUISlider *m_hudAccuracyScaleSlider;
	CBaseUISlider *m_hudHiterrorbarScaleSlider;
	CBaseUISlider *m_hudProgressbarScaleSlider;
	CBaseUISlider *m_hudScoreBarScaleSlider;
	CBaseUISlider *m_hudScoreBoardScaleSlider;
	CBaseUISlider *m_hudInputoverlayScaleSlider;
	CBaseUISlider *m_playfieldBorderSizeSlider;
	CBaseUISlider *m_statisticsOverlayScaleSlider;
	CBaseUISlider *m_statisticsOverlayXOffsetSlider;
	CBaseUISlider *m_statisticsOverlayYOffsetSlider;
	CBaseUISlider *m_cursorSizeSlider;
	CBaseUILabel *m_skinLabel;
	CBaseUIElement *m_skinSelectLocalButton;
	CBaseUIElement *m_skinSelectWorkshopButton;
	CBaseUIElement *m_resolutionSelectButton;
	CBaseUILabel *m_resolutionLabel;
	CBaseUITextbox *m_osuFolderTextbox;
	CBaseUITextbox *m_nameTextbox;
	CBaseUIElement *m_outputDeviceSelectButton;
	CBaseUILabel *m_outputDeviceLabel;
	OsuOptionsMenuResetButton *m_outputDeviceResetButton;
	CBaseUISlider *m_wasapiBufferSizeSlider;
	CBaseUISlider *m_wasapiPeriodSizeSlider;
	OsuOptionsMenuResetButton *m_wasapiBufferSizeResetButton;
	OsuOptionsMenuResetButton *m_wasapiPeriodSizeResetButton;
	CBaseUILabel *m_vrRenderTargetResolutionLabel;
	CBaseUISlider *m_vrApproachDistanceSlider;
	CBaseUISlider *m_vrVibrationStrengthSlider;
	CBaseUISlider *m_vrSliderVibrationStrengthSlider;
	CBaseUISlider *m_vrHudDistanceSlider;
	CBaseUISlider *m_vrHudScaleSlider;
	CBaseUISlider *m_sliderQualitySlider;
	CBaseUISlider *m_letterboxingOffsetXSlider;
	CBaseUISlider *m_letterboxingOffsetYSlider;
	CBaseUIButton *m_letterboxingOffsetResetButton;
	OsuOptionsMenuSliderPreviewElement *m_sliderPreviewElement;
	CBaseUITextbox *m_dpiTextbox;
	CBaseUITextbox *m_cm360Textbox;
	CBaseUIElement *m_skinSection;
	CBaseUISlider *m_uiScaleSlider;
	OsuOptionsMenuResetButton *m_uiScaleResetButton;
	CBaseUIElement *m_hpDrainSelectButton;
	CBaseUILabel *m_hpDrainSelectLabel;
	OsuOptionsMenuResetButton *m_hpDrainSelectResetButton;

	ConVar *m_waitingKey;
	ConVar *m_osu_slider_curve_points_separation_ref;
	ConVar *m_osu_letterboxing_offset_x_ref;
	ConVar *m_osu_letterboxing_offset_y_ref;
	ConVar *m_osu_mod_fposu_ref;
	ConVar *m_osu_skin_ref;
	ConVar *m_osu_skin_is_from_workshop_ref;
	ConVar *m_osu_skin_workshop_title_ref;
	ConVar *m_osu_skin_workshop_id_ref;
	ConVar *m_osu_skin_random_ref;
	ConVar *m_osu_ui_scale_ref;
	ConVar *m_win_snd_wasapi_buffer_size_ref;
	ConVar *m_win_snd_wasapi_period_size_ref;
	ConVar *m_osu_drain_type_ref;

	float m_fOsuFolderTextboxInvalidAnim;
	float m_fVibrationStrengthExampleTimer;
	bool m_bLetterboxingOffsetUpdateScheduled;
	bool m_bWorkshopSkinSelectScheduled;
	bool m_bUIScaleChangeScheduled;
	bool m_bUIScaleScrollToSliderScheduled;
	bool m_bDPIScalingScrollToSliderScheduled;
	bool m_bWASAPIBufferChangeScheduled;
	bool m_bWASAPIPeriodChangeScheduled;

	// key bindings
	int m_iNumResetAllKeyBindingsPressed;

	// mania layout
	int m_iManiaK;
	int m_iManiaKey;

	// search
	UString m_sSearchString;
	float m_fSearchOnCharKeybindHackTime;

	// drain
	std::vector<UString> m_drainTypes;
};

#endif
