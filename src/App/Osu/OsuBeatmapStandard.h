//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		osu!standard circle clicking
//
// $NoKeywords: $osustd
//===============================================================================//

#ifndef OSUBEATMAPSTANDARD_H
#define OSUBEATMAPSTANDARD_H

#include "OsuBeatmap.h"

class OsuBackgroundStarCacheLoader;

class OsuBeatmapStandard : public OsuBeatmap
{
public:
	OsuBeatmapStandard(Osu *osu);
	virtual ~OsuBeatmapStandard();

	virtual void draw(Graphics *g);
	virtual void drawInt(Graphics *g);
	virtual void drawVR(Graphics *g, Matrix4 &mvp, OsuVR *vr);
	virtual void draw3D(Graphics *g);
	virtual void draw3D2(Graphics *g);
	virtual void update();

	virtual void onModUpdate() {onModUpdate(true, true);}
	void onModUpdate(bool rebuildSliderVertexBuffers = true, bool recomputeDrainRate = true); // this seems very dangerous compiler-wise, but it works
	virtual bool isLoading();

	Vector2 legacyPixels2RawPixels(Vector2 coords) const; // only used for bounds calculations atm (just scales, nothing else)
	Vector2 pixels2OsuCoords(Vector2 pixelCoords) const; // only used for positional audio atm
	Vector2 osuCoords2Pixels(Vector2 coords) const; // hitobjects should use this one (includes lots of special behaviour)
	Vector2 osuCoords2RawPixels(Vector2 coords) const; // raw transform from osu!pixels to absolute screen pixels (without any mods whatsoever)
	Vector2 osuCoords2VRPixels(Vector2 coords) const; // this gets called by osuCoords2Pixels() during a VR draw(), for easier backwards compatibility
	Vector3 osuCoordsTo3D(Vector2 coords, const OsuHitObject *hitObject) const;
	Vector3 osuCoordsToRaw3D(Vector2 coords) const; // (without any mods whatsoever)
	Vector2 osuCoords2LegacyPixels(Vector2 coords) const; // only applies vanilla osu mods and static mods to the coordinates (used for generating the static slider mesh) centered at (0, 0, 0)

	// cursor
	Vector2 getCursorPos() const;
	Vector2 getFirstPersonCursorDelta() const;
	inline Vector2 getContinueCursorPoint() const {return m_vContinueCursorPoint;}

	// playfield
	inline Vector2 getPlayfieldSize() const {return m_vPlayfieldSize;}
	inline Vector2 getPlayfieldCenter() const {return m_vPlayfieldCenter;}
	inline float getPlayfieldRotation() const {return m_fPlayfieldRotation;}

	// hitobjects
	float getHitcircleDiameter() const; // in actual scaled pixels to the current resolution
	inline float getRawHitcircleDiameter() const {return m_fRawHitcircleDiameter;} // in osu!pixels
	inline float getHitcircleXMultiplier() const {return m_fXMultiplier;} // multiply osu!pixels with this to get screen pixels
	inline float getNumberScale() const {return m_fNumberScale;}
	inline float getHitcircleOverlapScale() const {return m_fHitcircleOverlapScale;}
	inline float getSliderFollowCircleDiameter() const {return m_fSliderFollowCircleDiameter;}
	inline float getRawSliderFollowCircleDiameter() const {return m_fRawSliderFollowCircleDiameter;}
	inline bool isInMafhamRenderChunk() const {return m_bInMafhamRenderChunk;}

	// score
	inline int getNumHitObjects() const {return m_hitobjects.size();}
	inline OsuDifficultyCalculator::DifficultyAttributes getDifficultyAttributes() const {return m_difficultyAttributes;}

	// hud
	inline bool isSpinnerActive() const {return m_bIsSpinnerActive;}

private:
	static ConVar *m_osu_draw_statistics_pp_ref;
	static ConVar *m_osu_draw_statistics_livestars_ref;
	static ConVar *m_osu_mod_fullalternate_ref;
	static ConVar *m_fposu_distance_ref;
	static ConVar *m_fposu_curved_ref;
	static ConVar *m_fposu_3d_curve_multiplier_ref;
	static ConVar *m_fposu_mod_strafing_ref;
	static ConVar *m_fposu_mod_strafing_frequency_x_ref;
	static ConVar *m_fposu_mod_strafing_frequency_y_ref;
	static ConVar *m_fposu_mod_strafing_frequency_z_ref;
	static ConVar *m_fposu_mod_strafing_strength_x_ref;
	static ConVar *m_fposu_mod_strafing_strength_y_ref;
	static ConVar *m_fposu_mod_strafing_strength_z_ref;
	static ConVar *m_fposu_mod_3d_depthwobble_ref;

	static inline Vector2 mapNormalizedCoordsOntoUnitCircle(const Vector2 &in)
	{
		return Vector2(in.x * std::sqrt(1.0f - in.y * in.y / 2.0f), in.y * std::sqrt(1.0f - in.x * in.x / 2.0f));
	}

	static float quadLerp3f(float left, float center, float right, float percent)
	{
		if (percent >= 0.5f)
		{
			percent = (percent - 0.5f) / 0.5f;
			percent *= percent;
			return lerp<float>(center, right, percent);
		}
		else
		{
			percent = percent / 0.5f;
			percent = 1.0f - (1.0f - percent)*(1.0f - percent);
			return lerp<float>(left, center, percent);
		}
	}

	virtual void onBeforeLoad();
	virtual void onLoad();
	virtual void onPlayStart();
	virtual void onBeforeStop(bool quit);
	virtual void onStop(bool quit);
	virtual void onPaused(bool first);
	virtual void onUnpaused();
	virtual void onRestart(bool quick);

	void drawFollowPoints(Graphics *g);
	void drawHitObjects(Graphics *g);

	void updateAutoCursorPos();
	void updatePlayfieldMetrics();
	void updateHitobjectMetrics();
	void updateSliderVertexBuffers();

	void calculateStacks();
	void computeDrainRate();

	void updateStarCache();
	void stopStarCacheLoader();
	bool isLoadingStarCache();
	bool isLoadingInt();

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
	OsuDifficultyCalculator::DifficultyAttributes m_difficultyAttributes;

	OsuBackgroundStarCacheLoader *m_starCacheLoader;
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

	RenderTarget *m_mafhamActiveRenderTarget;
	RenderTarget *m_mafhamFinishedRenderTarget;
	bool m_bMafhamRenderScheduled;
	int m_iMafhamHitObjectRenderIndex; // scene buffering for rendering entire beatmaps at once with an acceptable framerate
	int m_iMafhamPrevHitObjectIndex;
	int m_iMafhamActiveRenderHitObjectIndex;
	int m_iMafhamFinishedRenderHitObjectIndex;
	bool m_bInMafhamRenderChunk; // used by OsuSlider to not animate the reverse arrow, and by OsuCircle to not animate note blocking shaking, while being rendered into the scene buffer

	int m_iMandalaIndex;
};

#endif
