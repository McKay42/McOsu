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
class OsuBeatmapStandard;

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

	virtual void draw(Graphics *g);
	virtual void update();

	virtual void onResolutionChange(Vector2 newResolution);

	void drawDummy(Graphics *g);
	void drawVR(Graphics *g, Matrix4 &mvp, OsuVR *vr);
	void drawVRDummy(Graphics *g, Matrix4 &mvp, OsuVR *vr);

	void drawCursor(Graphics *g, Vector2 pos, float alphaMultiplier = 1.0f, bool secondTrail = false);
	void drawCursorSpectator1(Graphics *g, Vector2 pos, float alphaMultiplier = 1.0f);
	void drawCursorSpectator2(Graphics *g, Vector2 pos, float alphaMultiplier = 1.0f);
	void drawCursorVR1(Graphics *g, Matrix4 &mvp, Vector2 pos, float alphaMultiplier = 1.0f);
	void drawCursorVR2(Graphics *g, Matrix4 &mvp, Vector2 pos, float alphaMultiplier = 1.0f);
	void drawFps(Graphics *g) {drawFps(g, m_tempFont, m_fCurFps);}
	void drawHitErrorBar(Graphics *g, OsuBeatmapStandard *beatmapStd);
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
	void drawScorebarBg(Graphics *g, float alpha, float breakAnim);
	void drawSectionPass(Graphics *g, float alpha);
	void drawSectionFail(Graphics *g, float alpha);

	void animateCombo();
	void addHitError(long delta, bool miss = false, bool misaim = false);
	void addTarget(float delta, float angle);
	void animateInputoverlay(int key, bool down);

	void animateVolumeChange();
	void addCursorRipple(Vector2 pos);
	void animateCursorExpand();
	void animateCursorShrink();
	void animateKiBulge();
	void animateKiExplode();

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

	// ILLEGAL:
	inline float getScoreBarBreakAnim() const {return m_fScoreBarBreakAnim;}

private:
	struct CURSORTRAIL
	{
		Vector2 pos;
		float time;
		float alpha;
		float scale;
	};

	struct CURSORRIPPLE
	{
		Vector2 pos;
		float time;
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

		int index;
		int combo;
		unsigned long long score;
		float accuracy;

		bool missingBeatmap;
		bool downloadingBeatmap;
		bool dead;
		bool highlight;
	};

	struct BREAK
	{
		float startPercent;
		float endPercent;
	};

	void updateLayout();

	void addCursorTrailPosition(std::vector<CURSORTRAIL> &trail, Vector2 pos, bool empty = false);

	void drawCursorInt(Graphics *g, Shader *trailShader, std::vector<CURSORTRAIL> &trail, Matrix4 &mvp, Vector2 pos, float alphaMultiplier = 1.0f, bool emptyTrailFrame = false);
	void drawCursorRaw(Graphics *g, Vector2 pos, float alphaMultiplier = 1.0f);
	void drawCursorTrailRaw(Graphics *g, float alpha, Vector2 pos);
	void drawCursorRipples(Graphics *g);
	void drawFps(Graphics *g, McFont *font, float fps);
	void drawAccuracy(Graphics *g, float accuracy);
	void drawCombo(Graphics *g, int combo);
	void drawScore(Graphics *g, unsigned long long score);
	void drawHPBar(Graphics *g, double health, float alpha, float breakAnim);
	void drawScoreBoardInt(Graphics *g, const std::vector<SCORE_ENTRY> &scoreEntries);

	void drawWarningArrows(Graphics *g, float hitcircleDiameter = 0.0f);
	void drawContinue(Graphics *g, Vector2 cursor, float hitcircleDiameter = 0.0f);
	void drawHitErrorBar(Graphics *g, float hitWindow300, float hitWindow100, float hitWindow50, float hitWindowMiss);
	void drawHitErrorBarInt(Graphics *g, float hitWindow300, float hitWindow100, float hitWindow50, float hitWindowMiss);
	void drawProgressBar(Graphics *g, float percent, bool waiting);
	void drawProgressBarVR(Graphics *g, Matrix4 &mvp, OsuVR *vr, float percent, bool waiting);
	void drawStatistics(Graphics *g, int misses, int sliderbreaks, int maxPossibleCombo, int bpm, float ar, float cs, float od, float hp, int nps, int nd, int ur, float pp, float hitWindow300, int hitdeltaMin, int hitdeltaMax);
	void drawTargetHeatmap(Graphics *g, float hitcircleDiameter);
	void drawScrubbingTimeline(Graphics *g, unsigned long beatmapTime, unsigned long beatmapLength, unsigned long beatmapLengthPlayable, unsigned long beatmapStartTimePlayable, float beatmapPercentFinishedPlayable, const std::vector<BREAK> &breaks);
	void drawInputOverlay(Graphics *g, int numK1, int numK2, int numM1, int numM2);

	void drawStatisticText(Graphics *g, const UString text);

	float getCursorScaleFactor();
	float getCursorTrailScaleFactor();

	float getScoreScale();

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
	ConVar *m_osu_mod_fposu_ref;
	ConVar *m_osu_playfield_stretch_x_ref;
	ConVar *m_osu_playfield_stretch_y_ref;
	ConVar *m_osu_mp_win_condition_accuracy_ref;
	ConVar *m_osu_background_dim_ref;
	ConVar *m_osu_skip_intro_enabled_ref;
	ConVar *m_osu_skip_breaks_enabled_ref;

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

	// hit error bar
	std::vector<HITERROR> m_hiterrors;

	// inputoverlay / key overlay
	float m_fInputoverlayK1AnimScale;
	float m_fInputoverlayK2AnimScale;
	float m_fInputoverlayM1AnimScale;
	float m_fInputoverlayM2AnimScale;

	float m_fInputoverlayK1AnimColor;
	float m_fInputoverlayK2AnimColor;
	float m_fInputoverlayM1AnimColor;
	float m_fInputoverlayM2AnimColor;

	// volume
	float m_fLastVolume;
	float m_fVolumeChangeTime;
	float m_fVolumeChangeFade;
	CBaseUIContainer *m_volumeSliderOverlayContainer;
	OsuUIVolumeSlider *m_volumeMaster;
	OsuUIVolumeSlider *m_volumeEffects;
	OsuUIVolumeSlider *m_volumeMusic;

	// cursor & trail & ripples
	float m_fCursorExpandAnim;
	std::vector<CURSORTRAIL> m_cursorTrail;
	std::vector<CURSORTRAIL> m_cursorTrail2;
	std::vector<CURSORTRAIL> m_cursorTrailSpectator1;
	std::vector<CURSORTRAIL> m_cursorTrailSpectator2;
	std::vector<CURSORTRAIL> m_cursorTrailVR1;
	std::vector<CURSORTRAIL> m_cursorTrailVR2;
	Shader *m_cursorTrailShader;
	Shader *m_cursorTrailShaderVR;
	VertexArrayObject *m_cursorTrailVAO;
	std::vector<CURSORRIPPLE> m_cursorRipples;

	// target heatmap
	std::vector<TARGET> m_targets;

	// health
	double m_fHealth;
	float m_fScoreBarBreakAnim;
	float m_fKiScaleAnim;
};

#endif
