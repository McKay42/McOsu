//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		handles all VR related stuff (input, scene, screens)
//
// $NoKeywords: $osuvr
//===============================================================================//

#ifndef OSUVR_H
#define OSUVR_H

#include "cbase.h"

class Osu;
class OsuHUD;
class OsuBeatmap;

class Shader;
class RenderTarget;

class OsuVRUIElement;
class OsuVRUISlider;

class OsuVR
{
public:
	OsuVR(Osu *osu);
	~OsuVR();

	void drawVR(Graphics *g, Matrix4 &mvp, RenderTarget *screen);
	void drawVRBeatmap(Graphics *g, Matrix4 &mvp, OsuBeatmap *beatmap);
	void drawVRHUD(Graphics *g, Matrix4 &mvp, OsuHUD *hud);
	void drawVRHUDDummy(Graphics *g, Matrix4 &mvp, OsuHUD *hud);
	void drawVRPlayfieldDummy(Graphics *g, Matrix4 &mvp);
	void update();

	inline Shader *getShaderTexturedLegacyGeneric() {return m_shaderTexturedLegacyGeneric;}
	inline Shader *getShaderTexturedGeneric() {return m_shaderGenericTextured;}
	inline Shader *getShaderUntexturedLegacyGeneric() {return m_shaderUntexturedLegacyGeneric;}
	inline Shader *getShaderUntexturedGeneric() {return m_shaderGenericUntextured;}

	float getCircleHitboxScale(); // lenience multiplier, since the controllers/hands are not small points (would feel like shit without this)
	float getApproachDistance(); // in osu!pixels (scaled to the osu coordinate system)

	inline Vector2 getCursorPos1() {return m_vPlayfieldCursorPos1;} // 2d cursor position on virtual playfield in osu!pixels
	inline Vector2 getCursorPos2() {return m_vPlayfieldCursorPos2;} // 2d cursor position on virtual playfield in osu!pixels

	inline float getCursorDist1() {return m_fPlayfieldCursorDist1;} // 3d distance from controller to virtual 2d cursor on playfield
	inline float getCursorDist2() {return m_fPlayfieldCursorDist2;} // 3d distance from controller to virtual 2d cursor on playfield
	inline float getCursorDistSigned1() {return m_fPlayfieldCursorDistSigned1;} // 3d distance from controller to virtual 2d cursor on playfield
	inline float getCursorDistSigned2() {return m_fPlayfieldCursorDistSigned2;} // 3d distance from controller to virtual 2d cursor on playfield

	unsigned short getHapticPulseStrength();
	unsigned short getSliderHapticPulseStrength();

	inline bool isVirtualCursorOnScreen() {return m_bScreenIntersection;}
	inline bool isUIActive() {return m_bIsUIActive;} // if the user is currently interacting with the UI (this includes the laser touching an element!)

private:
	static const char *OSUVR_CONFIG_FILE_NAME;

	static float intersectRayPlane(Vector3 rayOrigin, Vector3 rayDir, Vector3 planeOrigin, Vector3 planeNormal);

	void drawVRCursors(Graphics *g, Matrix4 &mvp);

	void updateLayout();

	void save();
	void resetMatrices();

	float getDrawScale();
	inline Vector2 getVirtualScreenSize() {return m_vVirtualScreenSize;}

	void onScreenMatrixChange(UString oldValue, UString newValue);
	void onPlayfieldMatrixChange(UString oldValue, UString newValue);
	void onMatrixResetClicked();
	void onKeyboardButtonClicked();
	void onOffsetUpClicked();
	void onOffsetDownClicked();
	void onVolumeSliderChange(OsuVRUISlider *slider);
	void onScrubbingSliderChange(OsuVRUISlider *slider);

	Osu *m_osu;

	ConVar *m_osu_draw_cursor_trail_ref;
	ConVar *m_osu_volume_master_ref;

	Shader *m_shaderGenericTextured;
	Shader *m_shaderTexturedLegacyGeneric;
	Shader *m_shaderGenericUntextured;
	Shader *m_shaderUntexturedLegacyGeneric;

	Matrix4 m_screenMatrix;
	Matrix4 m_screenMatrixBackup;
	Matrix4 m_playfieldMatrix;
	Matrix4 m_playfieldMatrixBackup;
	bool m_bMovingPlayfieldCheck;
	bool m_bMovingPlayfieldAllow;
	bool m_bMovingScreenCheck;
	bool m_bMovingScreenAllow;
	bool m_bIsLeftControllerMovingScreen;
	bool m_bIsRightControllerMovingScreen;

	bool m_bDrawLaser;
	bool m_bScreenIntersection;
	bool m_bClickHeldStartedInScreen;
	bool m_bPlayfieldIntersection1;
	bool m_bPlayfieldIntersection2;
	Vector3 m_vScreenIntersectionPoint;
	Vector2 m_vScreenIntersectionPoint2D;
	Vector3 m_vPlayfieldIntersectionPoint1;
	Vector3 m_vPlayfieldIntersectionPoint2;
	Vector2 m_vPlayfieldCursorPos1;
	Vector2 m_vPlayfieldCursorPos2;
	float m_fPlayfieldCursorDist1;
	float m_fPlayfieldCursorDist2;
	float m_fPlayfieldCursorDistSigned1;
	float m_fPlayfieldCursorDistSigned2;

	float m_fAspect;
	Vector2 m_vVirtualScreenSize;

	bool m_bClickCheck;
	bool m_bMenuCheck;

	bool m_bScaleCheck;
	bool m_bIsPlayerScalingPlayfield;
	float m_fScaleBackup;
	float m_fDefaultScreenScale;
	float m_fDefaultPlayfieldScale;

	bool m_bIsUIActive;
	std::vector<OsuVRUIElement*> m_uiElements;
	OsuVRUIElement *m_keyboardButton;
	OsuVRUIElement *m_matrixResetButton;
	OsuVRUIElement *m_offsetDownButton;
	OsuVRUIElement *m_offsetUpButton;
	OsuVRUISlider *m_volumeSlider;
	OsuVRUISlider *m_scrubbingSlider;
};

#endif
