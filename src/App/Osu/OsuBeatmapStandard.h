//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		osu!standard circle clicking
//
// $NoKeywords: $osustd
//===============================================================================//

#ifndef OSUBEATMAPSTANDARD_H
#define OSUBEATMAPSTANDARD_H

#include "OsuBeatmap.h"

class StarCacheLoader;

class OsuBeatmapStandard : public OsuBeatmap
{
public:
	OsuBeatmapStandard(Osu *osu);
	virtual ~OsuBeatmapStandard();

	virtual void draw(Graphics *g);
	virtual void drawVR(Graphics *g, Matrix4 &mvp, OsuVR *vr);
	virtual void update();

	virtual void onModUpdate() {onModUpdate(true);}
	void onModUpdate(bool rebuildSliderVertexBuffers = true); // this seems very dangerous compiler-wise, but it works
	virtual bool isLoading();

	Vector2 pixels2OsuCoords(Vector2 pixelCoords); // only used for positional audio atm
	Vector2 osuCoords2Pixels(Vector2 coords); // hitobjects should use this one (includes lots of special behaviour)
	Vector2 osuCoords2RawPixels(Vector2 coords); // raw transform from osu!pixels to absolute screen pixels (without any mods whatsoever)
	Vector2 osuCoords2VRPixels(Vector2 coords); // this gets called by osuCoords2Pixels() during a VR draw(), for easier backwards compatibility
	Vector2 osuCoords2LegacyPixels(Vector2 coords); // only applies vanilla osu mods and static mods to the coordinates (used for generating the static slider mesh) centered at (0, 0, 0)

	// cursor
	Vector2 getCursorPos();
	Vector2 getFirstPersonCursorDelta();
	inline Vector2 getContinueCursorPoint() {return m_vContinueCursorPoint;}

	// playfield
	inline Vector2 getPlayfieldSize() {return m_vPlayfieldSize;}
	inline Vector2 getPlayfieldCenter() {return m_vPlayfieldCenter;}
	inline float getPlayfieldRotation() const {return m_fPlayfieldRotation;}

	// hitobjects
	float getHitcircleDiameter(); // in actual scaled pixels to the current resolution
	inline float getRawHitcircleDiameter() {return m_fRawHitcircleDiameter;} // in osu!pixels
	inline float getHitcircleXMultiplier() {return m_fXMultiplier;} // multiply osu!pixels with this to get screen pixels
	inline float getNumberScale() {return m_fNumberScale;}
	inline float getHitcircleOverlapScale() {return m_fHitcircleOverlapScale;}
	inline float getSliderFollowCircleDiameter() {return m_fSliderFollowCircleDiameter;}
	inline float getRawSliderFollowCircleDiameter() {return m_fRawSliderFollowCircleDiameter;}
	inline bool isInMafhamRenderChunk() const {return m_bInMafhamRenderChunk;}

	// score
	inline int getNumHitObjects() {return m_hitobjects.size();}
	inline float getAimStars() {return m_fAimStars;}
	inline float getSpeedStars() {return m_fSpeedStars;}

	// hud
	inline bool isSpinnerActive() {return m_bIsSpinnerActive;}

private:
	static ConVar *m_osu_draw_statistics_pp_ref;
	static ConVar *m_osu_mod_fullalternate_ref;

	virtual void onBeforeLoad();
	virtual void onLoad();
	virtual void onPlayStart();
	virtual void onBeforeStop(bool quit);
	virtual void onStop(bool quit);
	virtual void onPaused(bool first);
	virtual void onUnpaused();
	virtual void onRestart(bool quick);

	void drawFollowPoints(Graphics *g);

	void updateAutoCursorPos();
	void updatePlayfieldMetrics();
	void updateHitobjectMetrics();
	void updateSliderVertexBuffers();

	void calculateStacks();

	void updateStarCache();
	void stopStarCacheLoader();
	bool isLoadingStarCache();
	bool isLoadingInt();

	void checkHandleStarDiscrepancy(OsuBeatmapDifficulty *selectedDiff, float stars);

	// beatmap
	bool m_bIsSpinnerActive;
	Vector2 m_vContinueCursorPoint;

	// playfield
	float m_fPlayfieldRotation;
	float m_fScaleFactor;
	Vector2 m_vPlayfieldCenter;
	Vector2 m_vPlayfieldOffset;
	Vector2 m_vPlayfieldSize;

	// hitobject scaling
	float m_fXMultiplier;
	float m_fRawHitcircleDiameter;
	float m_fHitcircleDiameter;
	float m_fNumberScale;
	float m_fHitcircleOverlapScale;
	float m_fSliderFollowCircleDiameter;
	float m_fRawSliderFollowCircleDiameter;

	// auto
	Vector2 m_vAutoCursorPos;
	int m_iAutoCursorDanceIndex;

	// pp calculation buffer (only needs to be recalculated in onModUpdate(), instead of on every hit)
	float m_fAimStars;
	float m_fSpeedStars;
	StarCacheLoader *m_starCacheLoader;
	float m_fStarCacheTime;

	// dynamic slider vertex buffer and other recalculation checks (for live mod switching)
	float m_fPrevHitCircleDiameter;
	bool m_bWasHorizontalMirrorEnabled;
	bool m_bWasVerticalMirrorEnabled;
	bool m_bWasEZEnabled;
	bool m_bWasMafhamEnabled;
	float m_fPrevPlayfieldRotationFromConVar;
	float m_fPrevPlayfieldStretchX;
	float m_fPrevPlayfieldStretchY;
	float m_fPrevHitCircleDiameterForStarCache;
	float m_fPrevSpeedForStarCache;

	// custom
	bool m_bIsPreLoading;
	int m_iPreLoadingIndex;
	bool m_bIsVRDraw; // for switching legacy drawing to osuCoords2Pixels/osuCoords2VRPixels
	bool m_bWasHREnabled; // dynamic stack recalculation
	bool m_bWasStarDiffWarningIssued; // star diff warning once

	RenderTarget *m_mafhamActiveRenderTarget;
	RenderTarget *m_mafhamFinishedRenderTarget;
	bool m_bMafhamRenderScheduled;
	int m_iMafhamHitObjectRenderIndex; // scene buffering for rendering entire beatmaps at once with an acceptable framerate
	int m_iMafhamPrevHitObjectIndex;
	int m_iMafhamActiveRenderHitObjectIndex;
	int m_iMafhamFinishedRenderHitObjectIndex;
	bool m_bInMafhamRenderChunk; // used by OsuSlider to not animate the reverse arrow, and by OsuCircle to not animate note blocking shaking, while being rendered into the scene buffer
};

#endif
