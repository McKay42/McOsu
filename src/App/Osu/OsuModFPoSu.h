//================ Copyright (c) 2019, Colin Brook & PG, All rights reserved. =================//
//
// Purpose:		real 3d first person mode for fps warmup/practice
//
// $NoKeywords: $fposu
//=============================================================================================//

#ifndef OSUMODFPOSU_H
#define OSUMODFPOSU_H

//#define FPOSU_FEATURE_MAP3D

#include "cbase.h"

#include <list>

class Osu;

class Camera;
class ConVar;

#ifdef FPOSU_FEATURE_MAP3D

class Map3DLoader;

#endif

class Image;
class VertexArrayObject;

class OsuModFPoSu
{
public:
	OsuModFPoSu(Osu *osu);
	~OsuModFPoSu();

	void draw(Graphics *g);
	void update();

	void onKeyDown(KeyboardEvent &key);
	void onKeyUp(KeyboardEvent &key);

private:
	static int SUBDIVISIONS;

	struct VertexPair
	{
		Vector3 a;
		Vector3 b;
		float textureCoordinate;
		Vector3 normal;

		VertexPair(Vector3 a, Vector3 b, float tc) : a(a), b(b), textureCoordinate(tc) {;}
	};

	void handleZoomedChange();

	void setMousePosCompensated(Vector2 newMousePos);
	Vector2 intersectRayMesh(Vector3 pos, Vector3 dir);
	Vector3 calculateUnProjectedVector(Vector2 pos);

	void makePlayfield();
	void makeBackgroundCube();

	void onCurvedChange(UString oldValue, UString newValue);
	void onDistanceChange(UString oldValue, UString newValue);

	void onMap(UString args);

	Osu *m_osu;

	ConVar *m_mouse_sensitivity_ref;

	VertexArrayObject *m_vao;
	VertexArrayObject *m_vaoCube;

	std::list<VertexPair> m_meshList;
	float m_fCircumLength;
	Camera *m_camera;
	bool m_bZoomKeyDown;
	bool m_bZoomed;
	float m_fZoomFOVAnimPercent;

	Matrix4 m_modelMatrix;

#ifdef FPOSU_FEATURE_MAP3D

	Map3DLoader *m_map;

#endif

	// helper functions
	static float subdivide(std::list<VertexPair> meshList, const std::list<VertexPair>::iterator begin, const std::list<VertexPair>::iterator end, int n, float edgeDistance);
	static Vector3 normalFromTriangle(Vector3 p1, Vector3 p2, Vector3 p3);
};

#endif
