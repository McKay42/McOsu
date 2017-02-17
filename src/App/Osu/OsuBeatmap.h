//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		beatmap loader
//
// $NoKeywords: $osubm
//===============================================================================//

#ifndef OSUBEATMAP_H
#define OSUBEATMAP_H

#include "cbase.h"
#include "OsuScore.h"

#include <mutex>
#include "WinMinGW.Mutex.h" // necessary due to incomplete implementation in mingw-w64

class Sound;
class ConVar;

class Osu;
class OsuVR;
class OsuSkin;
class OsuHitObject;
class OsuBeatmapDifficulty;

class OsuBeatmap
{
public:
	struct CLICK
	{
		long musicPos;
		double realTime;
	};

public:
	OsuBeatmap(Osu *osu, UString filepath);
	virtual ~OsuBeatmap();

	void draw(Graphics *g);
	void drawVR(Graphics *g, Matrix4 &mvp, OsuVR *vr);
	void update();

	void skipEmptySection();

	// database logic
	void setDifficulties(std::vector<OsuBeatmapDifficulty*> diffs);

	// callbacks called by the Osu class
	void onUpdateMods(bool rebuildSliderVertexBuffers = true); // this makes all the necessary internal updates to hitobjects when legacy osu mods or static mods change live (but also on start)
	void keyPressed1();
	void keyPressed2();
	void keyReleased1();
	void keyReleased2();

	// songbrowser logic
	void select(); // loads the music of the currently selected diff and starts playing from the previewTime (e.g. clicking on a beatmap)
	void selectDifficulty(int index, bool deleteImage = true); // DEPRECATED deleteImage parameter
	void selectDifficulty(OsuBeatmapDifficulty *difficulty, bool deleteImage = true); // DEPRECATED deleteImage parameter
	void deselect(bool deleteImages = true); // stops + unloads the currently loaded music and deletes all hitobjects // DEPRECATED deleteImages parameter
	bool play();
	void restart(bool quick = false);
	void pause(bool quitIfWaiting = true);
	void pausePreviewMusic();
	void stop(bool quit = true);
	void fail();

	// music/sound
	void setVolume(float volume);
	void setSpeed(float speed);
	void setPitch(float pitch);
	void seekPercent(double percent);
	void seekPercentPlayable(double percent);
	void seekMS(unsigned long ms);

	inline Sound *getMusic() const {return m_music;}
	unsigned long getTime();
	unsigned long getStartTimePlayable();
	unsigned long getLength();
	unsigned long getLengthPlayable();
	float getPercentFinished();
	float getPercentFinishedPlayable();

	// live statistics
	int getBPM();
	float getSpeedMultiplier();
	inline int getNumMisses() {return m_iNumMisses;}
	inline int getNPS() {return m_iNPS;}
	inline int getND() {return m_iND;}

	// used by OsuHitObject children and OsuModSelector
	inline Osu *getOsu() const {return m_osu;}
	OsuSkin *getSkin();
	inline long getCurMusicPos() const {return m_iCurMusicPos;}

	float getRawAR();
	float getAR();
	float getCS();
	float getHP();
	float getRawOD();
	float getOD();

	inline float getHealth() {return m_fHealth;}

	// generic
	inline Vector2 getPlayfieldSize() {return m_vPlayfieldSize;}
	inline Vector2 getPlayfieldCenter() {return m_vPlayfieldCenter;}
	inline float getRawHitcircleDiameter() {return m_fRawHitcircleDiameter;} // in osu!pixels
	float getHitcircleDiameter(); // in actual scaled pixels to the current resolution
	inline float getNumberScale() {return m_fNumberScale;}
	inline float getHitcircleOverlapScale() {return m_fHitcircleOverlapScale;}
	inline float getSliderFollowCircleScale() {return m_fSliderFollowCircleScale;}
	inline float getSliderFollowCircleDiameter() {return m_fSliderFollowCircleDiameter;}
	inline float getRawSliderFollowCircleDiameter() {return m_fRawSliderFollowCircleDiameter;}
	inline float getPlayfieldRotation() const {return m_fPlayfieldRotation;}

	inline OsuBeatmapDifficulty *getSelectedDifficulty() {return m_selectedDifficulty;}
	inline std::vector<OsuBeatmapDifficulty*> getDifficulties() {return m_difficulties;}
	inline std::vector<OsuBeatmapDifficulty*> *getDifficultiesPointer() {return &m_difficulties;}
	inline int getNumDifficulties() {return m_difficulties.size();}

	inline bool isPlaying() {return m_bIsPlaying;}
	inline bool isPaused() {return m_bIsPaused;}
	inline bool isContinueScheduled() {return m_bContinueScheduled;}
	inline bool isWaiting() {return m_bIsWaiting;}
	inline bool isInSkippableSection() {return m_bIsInSkippableSection;}
	inline bool isSpinnerActive() {return m_bIsSpinnerActive;}
	inline bool shouldFlashWarningArrows() {return m_bShouldFlashWarningArrows;}
	bool isClickHeld();

	inline Vector2 getContinueCursorPoint() {return m_vContinueCursorPoint;}

	UString getTitle();
	UString getArtist();

	// OsuHitObject and other helper functions
	void consumeClickEvent();
	void addHitResult(OsuScore::HIT hit, long delta, bool ignoreOnHitErrorBar = false, bool hitErrorBarOnly = false, bool ignoreCombo = false, bool ignoreScore = false);
	void addSliderBreak();
	void addScorePoints(int points);
	void addHealth(float health);
	void playMissSound();

	Vector2 osuCoords2Pixels(Vector2 coords); // hitobjects should use this one (includes lots of special behaviour)
	Vector2 osuCoords2RawPixels(Vector2 coords); // raw transform from osu!pixels to absolute screen pixels (without any mods whatsoever)
	Vector2 osuCoords2VRPixels(Vector2 coords); // this gets called by osuCoords2Pixels() during a VR draw(), for easier backwards compatibility

	Vector2 osuCoords2LegacyPixels(Vector2 coords); // only applies vanilla osu mods and static mods to the coordinates (used for generating the static slider mesh) centered at (0, 0, 0)

	Vector2 getCursorPos();
	Vector2 getFirstPersonDelta();

private:
	void drawFollowPoints(Graphics *g);

	void actualRestart();

	void handlePreviewPlay();
	void loadMusic(bool stream = true);
	void unloadMusic();
	void unloadDiffs();
	void unloadHitObjects();
	void resetHitObjects(long curPos = 0);
	void resetScore();

	void updateAutoCursorPos();
	void updatePlayfieldMetrics();
	void updateHitobjectMetrics();
	void updateSliderVertexBuffers();

	void calculateStacks();

	unsigned long getMusicPositionMSInterpolated();

	static ConVar *m_osu_volume_music_ref;
	static ConVar *m_osu_speed_override_ref;
	static ConVar *m_osu_pitch_override_ref;

	Osu *m_osu;
	UString m_sFilePath;

	// beatmap
	bool m_bIsPlaying;
	bool m_bIsPaused;
	bool m_bIsWaiting;
	bool m_bIsRestartScheduled;
	bool m_bIsRestartScheduledQuick;

	bool m_bIsSpinnerActive;
	bool m_bIsInSkippableSection;
	bool m_bShouldFlashWarningArrows;
	int m_iSelectedDifficulty;
	float m_fWaitTime;
	bool m_bContinueScheduled;
	Vector2 m_vContinueCursorPoint;
	unsigned long m_iContinueMusicPos;

	std::vector<OsuBeatmapDifficulty*> m_difficulties;
	OsuBeatmapDifficulty *m_selectedDifficulty;
	Sound *m_music;

	// scaling & drawing
	float m_fScaleFactor;
	Vector2 m_vPlayfieldCenter;
	Vector2 m_vPlayfieldOffset;
	Vector2 m_vPlayfieldSize;
	float m_fXMultiplier;
	float m_fRawHitcircleDiameter;
	float m_fHitcircleDiameter;
	float m_fNumberScale;
	float m_fHitcircleOverlapScale;
	float m_fSliderFollowCircleScale;
	float m_fSliderFollowCircleDiameter;
	float m_fRawSliderFollowCircleDiameter;

	// sound
	float m_fBeatLength;
	long m_iCurMusicPos;
	long m_iPrevCurMusicPos;
	bool m_bWasSeekFrame; // workaround
	double m_fInterpolatedMusicPos; // for interpolation
	double m_fLastAudioTimeAccurateSet; // for interpolation
	double m_fLastRealTimeForInterpolationDelta; // for interpolation

	// gameplay
	float m_fHealth;
	float m_fBreakBackgroundFade;
	bool m_bInBreak;
	int m_iPreviousFollowPointObjectIndex;
	long m_iNextHitObjectTime;
	long m_iPreviousHitObjectTime;
	Vector2 m_vAutoCursorPos;
	float m_fPlayfieldRotation;
	int m_iAutoCursorDanceIndex;

	bool m_bClick1Held;
	bool m_bClick2Held;
	std::vector<CLICK> m_clicks;
	std::mutex m_clicksMutex;

	std::vector<OsuHitObject*> m_hitobjects;
	std::vector<OsuHitObject*> m_hitobjectsSortedByEndTime;
	std::vector<OsuHitObject*> m_misaimObjects;

	// statistics
	int m_iNumMisses;
	int m_iNPS;
	int m_iND;

	// custom
	bool m_bIsPreLoading;
	int m_iPreLoadingIndex;
	bool m_bWasHREnabled; // dynamic stack recalculation
	float m_fPrevHitCircleDiameter; // dynamic slider vertex buffer recalculation
	bool m_bWasHorizontalMirrorEnabled;
	bool m_bWasVerticalMirrorEnabled;
	bool m_bWasEZEnabled;
	bool m_bIsVRDraw; // for switching legacy drawing to osuCoords2Pixels/osuCoords2VRPixels
};

#endif
