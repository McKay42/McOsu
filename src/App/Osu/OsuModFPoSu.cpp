//================ Copyright (c) 2019, Colin Brook & PG, All rights reserved. =================//
//
// Purpose:		real 3d first person mode for fps warmup/practice
//
// $NoKeywords: $fposu
//=============================================================================================//

#include "OsuModFPoSu.h"

#include "Engine.h"
#include "ConVar.h"
#include "Camera.h"
#include "Mouse.h"
#include "Environment.h"
#include "ResourceManager.h"

#include "Osu.h"
#include "OsuSkin.h"
#include "OsuBeatmapStandard.h"

ConVar osu_mod_fposu("osu_mod_fposu", false);

ConVar fposu_mouse_dpi("fposu_mouse_dpi", 400);
ConVar fposu_mouse_cm_360("fposu_mouse_cm_360", 30.0f);
ConVar fposu_absolute_mode("fposu_absolute_mode", false);

ConVar fposu_distance("fposu_distance", 0.5f);
ConVar fposu_fov("fposu_fov", 103.0f);
ConVar fposu_curved("fposu_curved", true);
ConVar fposu_cube("fposu_cube", true);
ConVar fposu_cube_tint_r("fposu_cube_tint_r", 255, "from 0 to 255");
ConVar fposu_cube_tint_g("fposu_cube_tint_g", 255, "from 0 to 255");
ConVar fposu_cube_tint_b("fposu_cube_tint_b", 255, "from 0 to 255");
ConVar fposu_invert_vertical("fposu_invert_vertical", false);
ConVar fposu_invert_horizontal("fposu_invert_horizontal", false);

int OsuModFPoSu::SUBDIVISIONS = 4;

OsuModFPoSu::OsuModFPoSu(Osu *osu)
{
	m_osu = osu;

	// convar refs
	m_mouse_sensitivity_ref = convar->getConVarByName("mouse_sensitivity");

	// vars
	m_fCircumLength = 0.0f;
	m_camera = new Camera(Vector3(0, 0, 0),Vector3(0, 0, -1));

	// load resources
	m_vao = engine->getResourceManager()->createVertexArrayObject();
	m_vaoCube = engine->getResourceManager()->createVertexArrayObject();

	// convar callbacks
	fposu_curved.setCallback( fastdelegate::MakeDelegate(this, &OsuModFPoSu::onCurvedChange) );
	fposu_distance.setCallback( fastdelegate::MakeDelegate(this, &OsuModFPoSu::onDistanceChange) );

	// init
	makePlayfield();
	makeBackgroundCube();
}

void OsuModFPoSu::draw(Graphics *g)
{
	if (!osu_mod_fposu.getBool()) return;

	Matrix4 projectionMatrix = buildMatrixPerspectiveFovHorizontal(deg2rad(fposu_fov.getFloat()), ((float)m_osu->getScreenHeight() / (float)m_osu->getScreenWidth()), 0.05f, 50.0f);
	Matrix4 viewMatrix = Camera::buildMatrixLookAt(m_camera->getPos(), m_camera->getViewDirection(), m_camera->getViewUp());

	// HACKHACK: there is currently no way to directly modify the viewport origin, so the only option for rendering non-2d stuff with correct offsets (i.e. top left) is by rendering into a rendertarget
	// HACKHACK: abusing sliderFrameBuffer

	m_osu->getSliderFrameBuffer()->enable();
	{
		const Vector2 resolutionBackup = g->getResolution();
		g->onResolutionChange(m_osu->getSliderFrameBuffer()->getSize()); // set renderer resolution to game resolution (to correctly support letterboxing etc.)
		{
			g->setDepthBuffer(true);
			g->clearDepthBuffer();
			g->pushTransform();
			{
				g->setWorldMatrix(viewMatrix);
				g->setProjectionMatrix(projectionMatrix);

				g->setBlending(false);
				{
					// draw background cube
					if (fposu_cube.getBool())
					{
						m_osu->getSkin()->getBackgroundCube()->bind();
						{
							g->setColor(COLOR(255, clamp<int>(fposu_cube_tint_r.getInt(), 0, 255), clamp<int>(fposu_cube_tint_g.getInt(), 0, 255), clamp<int>(fposu_cube_tint_b.getInt(), 0, 255)));
							g->drawVAO(m_vaoCube);
						}
						m_osu->getSkin()->getBackgroundCube()->unbind();
					}

					// draw playfield mesh
					g->setWorldMatrixMul(m_modelMatrix);
					{
						m_osu->getPlayfieldBuffer()->bind();
						{
							g->setColor(0xffffffff);
							g->drawVAO(m_vao);
						}
						m_osu->getPlayfieldBuffer()->unbind();
					}
				}
				g->setBlending(true);
			}
			g->popTransform();
			g->setDepthBuffer(false);
		}
		g->onResolutionChange(resolutionBackup);
	}
	m_osu->getSliderFrameBuffer()->disable();

	// finally, draw that to the screen
	g->setBlending(false);
	{
		m_osu->getSliderFrameBuffer()->draw(g, 0, 0);
	}
	g->setBlending(true);
}

void OsuModFPoSu::update()
{
	if (!osu_mod_fposu.getBool()) return;

	// laziness, also slightly move back by default to avoid aliasing with background cube
	m_modelMatrix = Matrix4().translate(0, 0, -0.0015f).scale(1.0f, (m_osu->getPlayfieldBuffer()->getHeight() / m_osu->getPlayfieldBuffer()->getWidth())*(m_fCircumLength), 1.0f);

	const bool isAutoCursor = (m_osu->getModAuto() || m_osu->getModAutopilot());

	if (!fposu_absolute_mode.getBool() && !isAutoCursor && env->getOS() == Environment::OS::OS_WINDOWS) // HACKHACK: windows only for now (raw input support)
	{
		// calculate mouse delta
		// HACKHACK: undo engine mouse sensitivity multiplier
		Vector2 rawDelta = engine->getMouse()->getRawDelta() / m_mouse_sensitivity_ref->getFloat();

		// apply fposu mouse sensitivity multiplier
		const double countsPerCm = (double)fposu_mouse_dpi.getInt() / 2.54;
		const double cmPer360 = fposu_mouse_cm_360.getFloat();
		const double countsPer360 = cmPer360 * countsPerCm;
		const double multiplier = 360.0 / countsPer360;
		rawDelta *= multiplier;

		// update camera
		if (rawDelta.x != 0.0f)
			m_camera->rotateY(rawDelta.x * (fposu_invert_horizontal.getBool() ? 1.0f : -1.0f));
		if (rawDelta.y != 0.0f)
			m_camera->rotateX(rawDelta.y * (fposu_invert_vertical.getBool() ? 1.0f : -1.0f));

		// calculate ray-mesh intersection and set new mouse pos
		Vector2 newMousePos = intersectRayMesh(m_camera->getPos(), m_camera->getViewDirection());

		const bool osCursorVisible = env->isCursorVisible() || !env->isCursorInWindow() || !engine->hasFocus();

		if (!osCursorVisible)
		{
			// special case: force to center of screen if no intersection
			if (newMousePos.x == 0.0f && newMousePos.y == 0.0f)
				newMousePos = m_osu->getScreenSize()/2;

			setMousePosCompensated(newMousePos);
		}
	}
	else // absolute mouse position mode
	{
		// auto support, because it looks pretty cool
		Vector2 mousePos = engine->getMouse()->getPos();
		if (isAutoCursor && m_osu->isInPlayMode() && m_osu->getSelectedBeatmap() != NULL)
		{
			OsuBeatmapStandard *beatmapStd = dynamic_cast<OsuBeatmapStandard*>(m_osu->getSelectedBeatmap());
			if (beatmapStd != NULL && !beatmapStd->isPaused())
				mousePos = beatmapStd->getCursorPos();
		}

		m_camera->lookAt(calculateUnProjectedVector(mousePos));
	}
}

void OsuModFPoSu::setMousePosCompensated(Vector2 newMousePos)
{
	// NOTE: letterboxing uses Mouse::setOffset() to offset the virtual engine cursor coordinate system, so we have to respect that when setting a new (absolute) position
	newMousePos -= engine->getMouse()->getOffset();

	engine->getMouse()->onPosChange(newMousePos);
	env->setMousePos(newMousePos.x, newMousePos.y);
}

Vector2 OsuModFPoSu::intersectRayMesh(Vector3 pos, Vector3 dir)
{
	std::list<VertexPair>::iterator begin = m_meshList.begin();
	std::list<VertexPair>::iterator next = ++m_meshList.begin();
	int face = 0;
	while (next != m_meshList.end())
	{
		const Vector4 topLeft = (m_modelMatrix * Vector4(	(*begin).a.x,	(*begin).a.y,	(*begin).a.z,	1.0f));
		const Vector4 right = (m_modelMatrix * Vector4(		(*next).a.x,	(*next).a.y,	(*next).a.z,	1.0f));
		const Vector4 down = (m_modelMatrix * Vector4(		(*begin).b.x,	(*begin).b.y,	(*begin).b.z,	1.0f));
		//const Vector3 normal = (modelMatrix * (*begin).normal).normalize();

		const Vector3 TopLeft = Vector3(topLeft.x, topLeft.y, topLeft.z);
		const Vector3 Right = Vector3(right.x, right.y, right.z);
		const Vector3 Down = Vector3(down.x, down.y, down.z);

		const Vector3 calculatedNormal = (Right - TopLeft).cross(Down - TopLeft);

		const float denominator = calculatedNormal.dot(dir);
		const float numerator = -calculatedNormal.dot(pos - TopLeft);

		// WARNING: this is a full line trace (i.e. backwards and forwards infinitely far)
		if (denominator == 0.0f)
		{
			begin++;
			next++;
			face++;
			continue;
		}

		const float t = numerator / denominator;
		const Vector3 intersectionPoint = pos + dir*t;

		if (std::abs(calculatedNormal.dot(intersectionPoint - TopLeft)) < 1e-6f)
		{
			const float u = (intersectionPoint - TopLeft).dot(Right - TopLeft);
			const float v = (intersectionPoint - TopLeft).dot(Down - TopLeft);

			if (u >= 0 && u <= (Right - TopLeft).dot(Right - TopLeft))
			{
				if (v >= 0 && v <= (Down - TopLeft).dot(Down - TopLeft))
				{
					const float rightLength = (Right - TopLeft).length();
					const float downLength = (Down - TopLeft).length();
					const float x = u / (rightLength * rightLength);
					const float y = v / (downLength * downLength);
					const float distancePerFace = (float)m_osu->getScreenWidth() / std::pow(2.0f, (float)SUBDIVISIONS);
					const float distanceInFace = distancePerFace * x;

					const Vector2 newMousePos = Vector2((distancePerFace * face) + distanceInFace, y * m_osu->getScreenHeight());

					return newMousePos;
				}
			}
		}

		begin++;
		next++;
		face++;
	}

	return Vector2(0, 0);
}

Vector3 OsuModFPoSu::calculateUnProjectedVector(Vector2 pos)
{
	// calculate 3d position of 2d cursor on screen mesh
	const float cursorXPercent = clamp<float>(pos.x / (float)m_osu->getScreenWidth(), 0.0f, 1.0f);
	const float cursorYPercent = clamp<float>(pos.y / (float)m_osu->getScreenHeight(), 0.0f, 1.0f);

	std::list<VertexPair>::iterator begin = m_meshList.begin();
	std::list<VertexPair>::iterator next = ++m_meshList.begin();
	while (next != m_meshList.end())
	{
		Vector3 topLeft = (*begin).a;
		Vector3 bottomLeft = (*begin).b;
		Vector3 topRight = (*next).a;
		//Vector3 bottomRight = (*next).b;

		const float leftTC = (*begin).textureCoordinate;
		const float rightTC = (*next).textureCoordinate;
		const float topTC = 1.0f;
		const float bottomTC = 0.0f;

		if (cursorXPercent >= leftTC && cursorXPercent <= rightTC && cursorYPercent >= bottomTC && cursorYPercent <= topTC)
		{
			const float tcRightPercent = (cursorXPercent - leftTC) / std::abs(leftTC - rightTC);
			Vector3 right = (topRight - topLeft);
			right.setLength(right.length() * tcRightPercent);

			const float tcDownPercent = (cursorYPercent - bottomTC) / std::abs(topTC - bottomTC);
			Vector3 down = (bottomLeft - topLeft);
			down.setLength(down.length() * tcDownPercent);

			const Vector3 modelPos = (topLeft + right + down);

			const Vector4 worldPos = m_modelMatrix * Vector4(modelPos.x, modelPos.y, modelPos.z, 1.0f);

			return Vector3(worldPos.x, worldPos.y, worldPos.z);
		}

		begin++;
		next++;
	}

	return Vector3(-0.5f, 0.5f, -0.5f);
}

void OsuModFPoSu::makePlayfield()
{
	m_vao->clear();
	m_meshList.clear();

	const float dist = -fposu_distance.getFloat();

	VertexPair vp1 = VertexPair(Vector3(-0.5, 0.5, dist),Vector3(-0.5, -0.5, dist), 0);
	VertexPair vp2 = VertexPair(Vector3(0.5, 0.5, dist),Vector3(0.5, -0.5, dist), 1);

	const float edgeDistance = Vector3(0, 0, 0).distance(Vector3(-0.5, 0.0, dist));

	m_meshList.push_back(vp1);
	m_meshList.push_back(vp2);

	std::list<VertexPair>::iterator begin = m_meshList.begin();
	std::list<VertexPair>::iterator end = m_meshList.end();
	--end;
	m_fCircumLength = subdivide(m_meshList, begin, end, SUBDIVISIONS, edgeDistance);

	begin = m_meshList.begin();
	std::list<VertexPair>::iterator next = ++m_meshList.begin();
	while (next != m_meshList.end())
	{
		Vector3 topLeft = (*begin).a;
		Vector3 bottomLeft = (*begin).b;
		Vector3 topRight = (*next).a;
		Vector3 bottomRight = (*next).b;

		const float leftTC = (*begin).textureCoordinate;
		const float rightTC = (*next).textureCoordinate;
		const float topTC = 1.0f;
		const float bottomTC = 0.0f;

		m_vao->addVertex(topLeft);
		m_vao->addTexcoord(leftTC, topTC);
		m_vao->addVertex(topRight);
		m_vao->addTexcoord(rightTC, topTC);
		m_vao->addVertex(bottomLeft);
		m_vao->addTexcoord(leftTC, bottomTC);

		m_vao->addVertex(bottomLeft);
		m_vao->addTexcoord(leftTC, bottomTC);
		m_vao->addVertex(topRight);
		m_vao->addTexcoord(rightTC, topTC);
		m_vao->addVertex(bottomRight);
		m_vao->addTexcoord(rightTC, bottomTC);

		(*begin).normal = normalFromTriangle(topLeft, topRight, bottomLeft);

		begin++;
		next++;
	}
}

void OsuModFPoSu::makeBackgroundCube()
{
	m_vaoCube->clear();

	m_vaoCube->addVertex(-2.5f, -2.5f, -2.5f);  m_vaoCube->addTexcoord(0.0f, 0.0f);
	m_vaoCube->addVertex( 2.5f, -2.5f, -2.5f);  m_vaoCube->addTexcoord(1.0f, 0.0f);
	m_vaoCube->addVertex( 2.5f,  2.5f, -2.5f);  m_vaoCube->addTexcoord(1.0f, 1.0f);
	m_vaoCube->addVertex( 2.5f,  2.5f, -2.5f);  m_vaoCube->addTexcoord(1.0f, 1.0f);
	m_vaoCube->addVertex(-2.5f,  2.5f, -2.5f);  m_vaoCube->addTexcoord(0.0f, 1.0f);
	m_vaoCube->addVertex(-2.5f, -2.5f, -2.5f);  m_vaoCube->addTexcoord(0.0f, 0.0f);
	m_vaoCube->addVertex(-2.5f, -2.5f,  2.5f);  m_vaoCube->addTexcoord(0.0f, 0.0f);
	m_vaoCube->addVertex( 2.5f, -2.5f,  2.5f);  m_vaoCube->addTexcoord(1.0f, 0.0f);
	m_vaoCube->addVertex( 2.5f,  2.5f,  2.5f);  m_vaoCube->addTexcoord(1.0f, 1.0f);
	m_vaoCube->addVertex( 2.5f,  2.5f,  2.5f);  m_vaoCube->addTexcoord(1.0f, 1.0f);
	m_vaoCube->addVertex(-2.5f,  2.5f,  2.5f);  m_vaoCube->addTexcoord(0.0f, 1.0f);
	m_vaoCube->addVertex(-2.5f, -2.5f,  2.5f);  m_vaoCube->addTexcoord(0.0f, 0.0f);
	m_vaoCube->addVertex(-2.5f,  2.5f,  2.5f);  m_vaoCube->addTexcoord(1.0f, 0.0f);
	m_vaoCube->addVertex(-2.5f,  2.5f, -2.5f);  m_vaoCube->addTexcoord(1.0f, 1.0f);
	m_vaoCube->addVertex(-2.5f, -2.5f, -2.5f);  m_vaoCube->addTexcoord(0.0f, 1.0f);
	m_vaoCube->addVertex(-2.5f, -2.5f, -2.5f);  m_vaoCube->addTexcoord(0.0f, 1.0f);
	m_vaoCube->addVertex(-2.5f, -2.5f,  2.5f);  m_vaoCube->addTexcoord(0.0f, 0.0f);
	m_vaoCube->addVertex(-2.5f,  2.5f,  2.5f);  m_vaoCube->addTexcoord(1.0f, 0.0f);
	m_vaoCube->addVertex( 2.5f,  2.5f,  2.5f);  m_vaoCube->addTexcoord(1.0f, 0.0f);
	m_vaoCube->addVertex( 2.5f,  2.5f, -2.5f);  m_vaoCube->addTexcoord(1.0f, 1.0f);
	m_vaoCube->addVertex( 2.5f, -2.5f, -2.5f);  m_vaoCube->addTexcoord(0.0f, 1.0f);
	m_vaoCube->addVertex( 2.5f, -2.5f, -2.5f);  m_vaoCube->addTexcoord(0.0f, 1.0f);
	m_vaoCube->addVertex( 2.5f, -2.5f,  2.5f);  m_vaoCube->addTexcoord(0.0f, 0.0f);
	m_vaoCube->addVertex( 2.5f,  2.5f,  2.5f);  m_vaoCube->addTexcoord(1.0f, 0.0f);
	m_vaoCube->addVertex(-2.5f, -2.5f, -2.5f);  m_vaoCube->addTexcoord(0.0f, 1.0f);
	m_vaoCube->addVertex( 2.5f, -2.5f, -2.5f);  m_vaoCube->addTexcoord(1.0f, 1.0f);
	m_vaoCube->addVertex( 2.5f, -2.5f,  2.5f);  m_vaoCube->addTexcoord(1.0f, 0.0f);
	m_vaoCube->addVertex( 2.5f, -2.5f,  2.5f);  m_vaoCube->addTexcoord(1.0f, 0.0f);
	m_vaoCube->addVertex(-2.5f, -2.5f,  2.5f);  m_vaoCube->addTexcoord(0.0f, 0.0f);
	m_vaoCube->addVertex(-2.5f, -2.5f, -2.5f);  m_vaoCube->addTexcoord(0.0f, 1.0f);
	m_vaoCube->addVertex(-2.5f,  2.5f, -2.5f);  m_vaoCube->addTexcoord(0.0f, 1.0f);
	m_vaoCube->addVertex( 2.5f,  2.5f, -2.5f);  m_vaoCube->addTexcoord(1.0f, 1.0f);
	m_vaoCube->addVertex( 2.5f,  2.5f,  2.5f);  m_vaoCube->addTexcoord(1.0f, 0.0f);
	m_vaoCube->addVertex( 2.5f,  2.5f,  2.5f);  m_vaoCube->addTexcoord(1.0f, 0.0f);
	m_vaoCube->addVertex(-2.5f,  2.5f,  2.5f);  m_vaoCube->addTexcoord(0.0f, 0.0f);
	m_vaoCube->addVertex(-2.5f,	 2.5f, -2.5f);	m_vaoCube->addTexcoord(0.0f, 1.0f);
}

void OsuModFPoSu::onCurvedChange(UString oldValue, UString newValue)
{
	makePlayfield();
}

void OsuModFPoSu::onDistanceChange(UString oldValue, UString newValue)
{
	makePlayfield();
}

float OsuModFPoSu::subdivide(std::list<VertexPair> meshList, const std::list<VertexPair>::iterator begin, const std::list<VertexPair>::iterator end, int n, float edgeDistance)
{
	const Vector3 a = Vector3((*begin).a.x, 0.0f, (*begin).a.z);
	const Vector3 b = Vector3((*end).a.x, 0, (*end).a.z);
	Vector3 middlePoint = Vector3(lerp<float>(a.x, b.x, 0.5f),
								  lerp<float>(a.y, b.y, 0.5f),
								  lerp<float>(a.z, b.z, 0.5f));

	if (fposu_curved.getBool())
		middlePoint.setLength(edgeDistance);

	Vector3 top, bottom;
	top = bottom = middlePoint;

	top.y = (*begin).a.y;
	bottom.y = (*begin).b.y;

	const float tc = lerp<float>((*begin).textureCoordinate, (*end).textureCoordinate, 0.5f);

	VertexPair newVP = VertexPair(top, bottom, tc);
	std::list<VertexPair>::iterator newPos = meshList.insert(end, newVP);

	float circumLength = 0.0f;

	if (n > 1)
	{
		circumLength += subdivide(meshList, begin, newPos, n-1, edgeDistance);
		circumLength += subdivide(meshList, newPos, end, n-1, edgeDistance);
	}
	else
	{
		circumLength += (*begin).a.distance(newVP.a);
		circumLength += newVP.a.distance((*end).a);
	}

	return circumLength;
}

Vector3 OsuModFPoSu::normalFromTriangle(Vector3 p1, Vector3 p2, Vector3 p3)
{
	const Vector3 u = (p2 - p1);
	const Vector3 v = (p3 - p1);

	return u.cross(v).normalize();
}

Matrix4 OsuModFPoSu::buildMatrixPerspectiveFovHorizontal(float fovRad, float aspect, float zn, float zf)
{
	const float f = 1.0f / std::tan(fovRad/2.0f);

	const float q = (zf+zn)/(zn-zf);
	const float qn = (2*zf*zn)/(zn-zf);

	return Matrix4(f,	0,			0, 		0,
				   0, 	f/aspect, 	0,		0,
				   0, 	0,			q,		qn,
				   0,	0,			-1,		0).transpose();
}
