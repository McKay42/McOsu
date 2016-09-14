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

	bool load(); // loads the metadata of all difficulties and adds them

	void draw(Graphics *g);
	void update();

	void skipEmptySection();

	// callbacks called by the Osu class
	void onUpdateMods();
	void keyPressed1();
	void keyPressed2();
	void keyReleased1();
	void keyReleased2();

	// songbrowser logic
	void select(); // loads the music of the currently selected diff and starts playing from the previewTime (e.g. clicking on a beatmap)
	void selectDifficulty(int index, bool deleteImage = true); // DEPRECATED deleteImage
	void selectDifficulty(OsuBeatmapDifficulty *difficulty, bool deleteImage = true); // DEPRECATED deleteImage
	void deselect(bool deleteImages = true); // stops + unloads the currently loaded music and deletes all hitobjects // DEPRECATED deleteImages
	bool play();
	void restart(bool quick = false);
	void pause(bool quitIfWaiting = true);
	void pausePreviewMusic();
	void stop(bool quit = true);
	void fail();

	void setVolume(float volume);
	void setSpeed(float speed);
	void setPitch(float pitch);
	void seekPercent(double percent);
	void seekMS(unsigned long ms);

	inline Sound *getMusic() const {return m_music;}
	unsigned long getLength();
	float getPercentFinished();

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

	// generic
	inline Vector2 getPlayfieldSize() {return m_vPlayfieldSize;}
	inline Vector2 getPlayfieldCenter() {return m_vPlayfieldCenter;}
	inline float getRawHitcircleDiameter() {return m_fRawHitcircleDiameter;} // in osu!pixels
	inline float getHitcircleDiameter() {return m_fHitcircleDiameter;} // in actual scaled pixels to the current resolution
	inline float getNumberScale() {return m_fNumberScale;}
	inline float getHitcircleOverlapScale() {return m_fHitcircleOverlapScale;}
	inline float getSliderFollowCircleScale() {return m_fSliderFollowCircleScale;}
	inline float getSliderFollowCircleDiameter() {return m_fSliderFollowCircleDiameter;}
	inline float getPlayfieldRotation() const {return m_fPlayfieldRotation;}

	inline int getSelectedDifficultyIndex() {return m_iSelectedDifficulty;} // DEPRECATED
	inline OsuBeatmapDifficulty *getSelectedDifficulty() {return m_selectedDifficulty;}
	inline std::vector<OsuBeatmapDifficulty*> getDifficulties() {return m_difficulties;}

	inline bool isPlaying() {return m_bIsPlaying;}
	inline bool isPaused() {return m_bIsPaused;}
	inline bool isContinueScheduled() {return m_bContinueScheduled;}
	inline bool isWaiting() {return m_bIsWaiting;}
	inline bool isInSkippableSection() {return m_bIsInSkippableSection;}
	inline bool isSpinnerActive() {return m_bIsSpinnerActive;}
	inline bool shouldFlashWarningArrows() {return m_bShouldFlashWarningArrows;}
	inline bool isClickHeld() {return m_bClick1Held || m_bClick2Held;}

	inline Vector2 getContinueCursorPoint() {return m_vContinueCursorPoint;}

	UString getTitle();
	UString getArtist();

	// OsuHitObject and other helper functions
	void consumeClickEvent();
	void addHitResult(OsuScore::HIT hit, long delta, bool ignoreOnHitErrorBar = false, bool hitErrorBarOnly = false, bool ignoreCombo = false);
	void addSliderBreak();
	void playMissSound();

	Vector2 osuCoords2Pixels(Vector2 coords);
	Vector2 getCursorPos();
	inline float getAmplitude() {return m_fAmplitude;}

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

	void calculateStacks();
	Vector2 originalOsuCoords2Stack(Vector2 coords);

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

	// sound
	float m_fBeatLength;
	float m_fAmplitude;
	long m_iCurMusicPos;
	unsigned long m_iLastMusicPosition;
	double m_fLastMusicPositionForInterpolation;

	// gameplay
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
	std::vector<OsuHitObject*> m_misaimObjects;

	// statistics
	int m_iNumMisses;
	int m_iNPS;
	int m_iND;

	// custom
	bool m_bWasHREnabled; // dynamic stack recalculation
};

#endif
