//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		hud element drawing functions (accuracy, combo, score, etc.)
//
// $NoKeywords: $osuhud
//===============================================================================//

#ifndef OSUHUD_H
#define OSUHUD_H

#include "OsuScreen.h"

class Osu;
class OsuVR;
class OsuScore;

class McFont;
class ConVar;
class Image;
class Shader;
class VertexArrayObject;

class OsuUIVolumeSlider;

class CBaseUIContainer;

class OsuHUD : public OsuScreen
{
public:
	OsuHUD(Osu *osu);
	virtual ~OsuHUD();

	void draw(Graphics *g);
	void drawDummy(Graphics *g);
	void drawVR(Graphics *g, Matrix4 &mvp, OsuVR *vr);
	void drawVRDummy(Graphics *g, Matrix4 &mvp, OsuVR *vr);
	void update();

	void drawCursor(Graphics *g, Vector2 pos, float alphaMultiplier = 1.0f);
	void drawCursorSpectator1(Graphics *g, Vector2 pos, float alphaMultiplier = 1.0f);
	void drawCursorSpectator2(Graphics *g, Vector2 pos, float alphaMultiplier = 1.0f);
	void drawCursorVR1(Graphics *g, Matrix4 &mvp, Vector2 pos, float alphaMultiplier = 1.0f);
	void drawCursorVR2(Graphics *g, Matrix4 &mvp, Vector2 pos, float alphaMultiplier = 1.0f);
	void drawFps(Graphics *g) {drawFps(g, m_tempFont, m_fCurFps);}
	void drawPlayfieldBorder(Graphics *g, Vector2 playfieldCenter, Vector2 playfieldSize, float hitcircleDiameter);
	void drawPlayfieldBorder(Graphics *g, Vector2 playfieldCenter, Vector2 playfieldSize, float hitcircleDiameter, float borderSize);
	void drawLoadingSmall(Graphics *g);
	void drawBeatmapImportSpinner(Graphics *g);
	void drawVolumeChange(Graphics *g);
	void drawScoreNumber(Graphics *g, unsigned long long number, float scale = 1.0f, bool drawLeadingZeroes = false, int offset = 2);
	void drawComboSimple(Graphics *g, int combo, float scale = 1.0f); // used by OsuRankingScreen
	void drawAccuracySimple(Graphics *g, float accuracy, float scale = 1.0f); // used by OsuRankingScreen
	void drawWarningArrow(Graphics *g, Vector2 pos, bool flipVertically, bool originLeft = true);
	void drawScoreBoard(Graphics *g, std::string &beatmapMD5Hash, OsuScore *currentScore);
	void drawScoreBoardMP(Graphics *g);

	void animateCombo();
	void addHitError(long delta, bool miss = false, bool misaim = false);
	void addTarget(float delta, float angle);

	void animateVolumeChange();
	void animateCursorExpand();
	void animateCursorShrink();

	void selectVolumePrev();
	void selectVolumeNext();

	void resetHitErrorBar();

	McRect getSkipClickRect();
	bool isVolumeOverlayVisible();
	bool isVolumeOverlayBusy();
	OsuUIVolumeSlider *getVolumeMasterSlider() {return m_volumeMaster;}
	OsuUIVolumeSlider *getVolumeEffectsSlider() {return m_volumeEffects;}
	OsuUIVolumeSlider *getVolumeMusicSlider() {return m_volumeMusic;}

	void drawSkip(Graphics *g);

private:
	struct CURSORTRAIL
	{
		Vector2 pos;
		float time;
		float alpha;
		float scale;
	};

	struct HITERROR
	{
		float time;
		long delta;
		bool miss;
		bool misaim;
	};

	struct TARGET
	{
		float time;
		float delta;
		float angle;
	};

	struct SCORE_ENTRY
	{
		UString name;

		int combo;
		unsigned long long score;
		float accuracy;

		bool missingBeatmap;
		bool dead;
		bool highlight;
	};

	void addCursorTrailPosition(std::vector<CURSORTRAIL> &trail, Vector2 pos, bool empty = false);

	void drawCursorInt(Graphics *g, Shader *trailShader, std::vector<CURSORTRAIL> &trail, Matrix4 &mvp, Vector2 pos, float alphaMultiplier = 1.0f, bool emptyTrailFrame = false);
	void drawCursorRaw(Graphics *g, Vector2 pos, float alphaMultiplier = 1.0f);
	void drawCursorTrailRaw(Graphics *g, float alpha, Vector2 pos);
	void drawFps(Graphics *g, McFont *font, float fps);
	void drawAccuracy(Graphics *g, float accuracy);
	void drawCombo(Graphics *g, int combo);
	void drawScore(Graphics *g, unsigned long long score);
	void drawHP(Graphics *g, float health);
	void drawScoreBoardInt(Graphics *g, std::vector<SCORE_ENTRY> &scoreEntries);

	void drawWarningArrows(Graphics *g, float hitcircleDiameter = 0.0f);
	void drawContinue(Graphics *g, Vector2 cursor, float hitcircleDiameter = 0.0f);
	void drawHitErrorBar(Graphics *g, float hitWindow300, float hitWindow100, float hitWindow50, float hitWindowMiss);
	void drawProgressBar(Graphics *g, float percent, bool waiting);
	void drawProgressBarVR(Graphics *g, Matrix4 &mvp, OsuVR *vr, float percent, bool waiting);
	void drawStatistics(Graphics *g, int misses, int sliderbreaks, int bpm, float ar, float cs, float od, int nps, int nd, int ur, int pp);
	void drawTargetHeatmap(Graphics *g, float hitcircleDiameter);
	void drawScrubbingTimeline(Graphics *g, unsigned long beatmapTime, unsigned long beatmapLength, unsigned long beatmapLengthPlayable, unsigned long beatmapStartTimePlayable, float beatmapPercentFinishedPlayable);

	void drawStatisticText(Graphics *g, const UString text);

	float getCursorScaleFactor();
	float getCursorTrailScaleFactor();

	void onVolumeOverlaySizeChange(UString oldValue, UString newValue);

	McFont *m_tempFont;

	ConVar *m_name_ref;
	ConVar *m_host_timescale_ref;
	ConVar *m_osu_volume_master_ref;
	ConVar *m_osu_volume_effects_ref;
	ConVar *m_osu_volume_music_ref;
	ConVar *m_osu_volume_change_interval_ref;
	ConVar *m_osu_mod_target_300_percent_ref;
	ConVar *m_osu_mod_target_100_percent_ref;
	ConVar *m_osu_mod_target_50_percent_ref;
	ConVar *m_osu_playfield_stretch_x_ref;
	ConVar *m_osu_playfield_stretch_y_ref;
	ConVar *m_osu_mp_win_condition_accuracy_ref;
	ConVar *m_osu_background_dim_ref;

	// shit code
	float m_fAccuracyXOffset;
	float m_fAccuracyYOffset;
	float m_fScoreHeight;

	float m_fComboAnim1;
	float m_fComboAnim2;

	// fps counter
	float m_fCurFps;
	float m_fCurFpsSmooth;
	float m_fFpsUpdate;
	float m_fFpsFontHeight;

	// hit error bar
	std::vector<HITERROR> m_hiterrors;

	// volume
	float m_fLastVolume;
	float m_fVolumeChangeTime;
	float m_fVolumeChangeFade;
	CBaseUIContainer *m_volumeSliderOverlayContainer;
	OsuUIVolumeSlider *m_volumeMaster;
	OsuUIVolumeSlider *m_volumeEffects;
	OsuUIVolumeSlider *m_volumeMusic;

	// cursor & trail
	float m_fCursorExpandAnim;
	std::vector<CURSORTRAIL> m_cursorTrail;
	std::vector<CURSORTRAIL> m_cursorTrailSpectator1;
	std::vector<CURSORTRAIL> m_cursorTrailSpectator2;
	std::vector<CURSORTRAIL> m_cursorTrailVR1;
	std::vector<CURSORTRAIL> m_cursorTrailVR2;
	Shader *m_cursorTrailShader;
	Shader *m_cursorTrailShaderVR;
	VertexArrayObject *m_cursorTrailVAO;

	// target heatmap
	std::vector<TARGET> m_targets;
};

#endif
