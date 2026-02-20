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
#include "Keyboard.h"
#include "Environment.h"
#include "ResourceManager.h"
#include "AnimationHandler.h"
#include "DirectX11Interface.h"

#include "Osu.h"
#include "OsuSkin.h"
#include "OsuKeyBindings.h"
#include "OsuBeatmapStandard.h"
#include "OsuBackgroundImageHandler.h"
#include "OsuOptionsMenu.h"
#include "OsuModSelector.h"

#include "OpenGLHeaders.h"
#include "OpenGLLegacyInterface.h"
#include "OpenGL3Interface.h"
#include "OpenGLES2Interface.h"

#include <sstream>

ConVar osu_mod_fposu("osu_mod_fposu", false, FCVAR_NONE);

ConVar fposu_mouse_dpi("fposu_mouse_dpi", 400, FCVAR_NONE);
ConVar fposu_mouse_cm_360("fposu_mouse_cm_360", 30.0f, FCVAR_NONE);
ConVar fposu_absolute_mode("fposu_absolute_mode", false, FCVAR_NONE);

ConVar fposu_distance("fposu_distance", 0.5f, FCVAR_NONE);
ConVar fposu_playfield_position_x("fposu_playfield_position_x", 0.0f, FCVAR_NONE);
ConVar fposu_playfield_position_y("fposu_playfield_position_y", 0.0f, FCVAR_NONE);
ConVar fposu_playfield_position_z("fposu_playfield_position_z", 0.0f, FCVAR_NONE);
ConVar fposu_playfield_rotation_x("fposu_playfield_rotation_x", 0.0f, FCVAR_NONE);
ConVar fposu_playfield_rotation_y("fposu_playfield_rotation_y", 0.0f, FCVAR_NONE);
ConVar fposu_playfield_rotation_z("fposu_playfield_rotation_z", 0.0f, FCVAR_NONE);
ConVar fposu_fov("fposu_fov", 103.0f, FCVAR_NONE);
ConVar fposu_zoom_fov("fposu_zoom_fov", 45.0f, FCVAR_NONE);
ConVar fposu_zoom_sensitivity_ratio("fposu_zoom_sensitivity_ratio", 1.0f, FCVAR_NONE, "replicates zoom_sensitivity_ratio behavior on css/csgo/tf2/etc.");
ConVar fposu_zoom_anim_duration("fposu_zoom_anim_duration", 0.065f, FCVAR_NONE, "time in seconds for the zoom/unzoom animation");
ConVar fposu_zoom_toggle("fposu_zoom_toggle", false, FCVAR_NONE, "whether the zoom key acts as a toggle");
ConVar fposu_vertical_fov("fposu_vertical_fov", false, FCVAR_NONE);
ConVar fposu_curved("fposu_curved", true, FCVAR_NONE);
ConVar fposu_skybox("fposu_skybox", true, FCVAR_NONE);
ConVar fposu_cube("fposu_cube", true, FCVAR_NONE);
ConVar fposu_cube_size("fposu_cube_size", 500.0f, FCVAR_NONE);
ConVar fposu_cube_tint_r("fposu_cube_tint_r", 255, FCVAR_NONE, "from 0 to 255");
ConVar fposu_cube_tint_g("fposu_cube_tint_g", 255, FCVAR_NONE, "from 0 to 255");
ConVar fposu_cube_tint_b("fposu_cube_tint_b", 255, FCVAR_NONE, "from 0 to 255");
ConVar fposu_invert_vertical("fposu_invert_vertical", false, FCVAR_NONE);
ConVar fposu_invert_horizontal("fposu_invert_horizontal", false, FCVAR_NONE);

ConVar fposu_noclip("fposu_noclip", false, FCVAR_NONE);
ConVar fposu_noclipspeed("fposu_noclipspeed", 2.0f, FCVAR_NONE);
ConVar fposu_noclipaccelerate("fposu_noclipaccelerate", 20.0f, FCVAR_NONE);
ConVar fposu_noclipfriction("fposu_noclipfriction", 10.0f, FCVAR_NONE);

#ifndef MCOSU_FPOSU_4D_MODE_FINISHED
#define MCOSU_FPOSU_4D_MODE_CVAR_FLAGS (FCVAR_DEVELOPMENTONLY|FCVAR_HARDCODED)
#else
#define MCOSU_FPOSU_4D_MODE_CVAR_FLAGS (FCVAR_NONE)
#endif
ConVar fposu_3d("fposu_3d", false, MCOSU_FPOSU_4D_MODE_CVAR_FLAGS);
ConVar fposu_3d_playfield_scale("fposu_3d_playfield_scale", 1.0f, MCOSU_FPOSU_4D_MODE_CVAR_FLAGS);
ConVar fposu_3d_spheres("fposu_3d_spheres", true, MCOSU_FPOSU_4D_MODE_CVAR_FLAGS, "whether to draw combocolored lit 3d spheres instead of flat skin image quads");
ConVar fposu_3d_spheres_aa("fposu_3d_spheres_aa", 4, MCOSU_FPOSU_4D_MODE_CVAR_FLAGS, "MSAA factor, valid values are: 0, 2, 4, 8, 16");
ConVar fposu_3d_spheres_light_ambient("fposu_3d_spheres_light_ambient", 0.15f, MCOSU_FPOSU_4D_MODE_CVAR_FLAGS);
ConVar fposu_3d_spheres_light_brightness("fposu_3d_spheres_light_brightness", 1.15f, MCOSU_FPOSU_4D_MODE_CVAR_FLAGS);
ConVar fposu_3d_spheres_light_diffuse("fposu_3d_spheres_light_diffuse", 1.0f, MCOSU_FPOSU_4D_MODE_CVAR_FLAGS);
ConVar fposu_3d_spheres_light_phong("fposu_3d_spheres_light_phong", 1.0f, MCOSU_FPOSU_4D_MODE_CVAR_FLAGS);
ConVar fposu_3d_spheres_light_phong_exponent("fposu_3d_spheres_light_phong_exponent", 10.0f, MCOSU_FPOSU_4D_MODE_CVAR_FLAGS);
ConVar fposu_3d_spheres_light_position_x("fposu_3d_spheres_light_position_x", 500.0f, MCOSU_FPOSU_4D_MODE_CVAR_FLAGS);
ConVar fposu_3d_spheres_light_position_y("fposu_3d_spheres_light_position_y", 500.0f, MCOSU_FPOSU_4D_MODE_CVAR_FLAGS);
ConVar fposu_3d_spheres_light_position_z("fposu_3d_spheres_light_position_z", 325.0f, MCOSU_FPOSU_4D_MODE_CVAR_FLAGS);
ConVar fposu_3d_spheres_light_rim("fposu_3d_spheres_light_rim", 1.0f, MCOSU_FPOSU_4D_MODE_CVAR_FLAGS);
ConVar fposu_3d_curve_multiplier("fposu_3d_curve_multiplier", 1.0f, MCOSU_FPOSU_4D_MODE_CVAR_FLAGS, "multiplier for the default curving factor (only relevant if fposu_curved is enabled)");
ConVar fposu_3d_hitobjects_look_at_player("fposu_3d_hitobjects_look_at_player", true, MCOSU_FPOSU_4D_MODE_CVAR_FLAGS);
ConVar fposu_3d_approachcircles_look_at_player("fposu_3d_approachcircles_look_at_player", true, MCOSU_FPOSU_4D_MODE_CVAR_FLAGS);
ConVar fposu_3d_draw_beatmap_background_image("fposu_3d_draw_beatmap_background_image", true, MCOSU_FPOSU_4D_MODE_CVAR_FLAGS);
ConVar fposu_3d_beatmap_background_image_distance_multiplier("fposu_3d_beatmap_background_image_distance_multiplier", 1.0f, MCOSU_FPOSU_4D_MODE_CVAR_FLAGS);
ConVar fposu_3d_skybox("fposu_3d_skybox", true, MCOSU_FPOSU_4D_MODE_CVAR_FLAGS);
ConVar fposu_3d_skybox_size("fposu_3d_skybox_size", 450.0f, MCOSU_FPOSU_4D_MODE_CVAR_FLAGS);
ConVar fposu_3d_wireframe("fposu_3d_wireframe", false, MCOSU_FPOSU_4D_MODE_CVAR_FLAGS);

ConVar fposu_draw_cursor_trail("fposu_draw_cursor_trail", true, FCVAR_NONE);
ConVar fposu_draw_scorebarbg_on_top("fposu_draw_scorebarbg_on_top", false, FCVAR_NONE);

ConVar fposu_mod_strafing("fposu_mod_strafing", false, FCVAR_NONE);
ConVar fposu_mod_strafing_strength_x("fposu_mod_strafing_strength_x", 0.3f, FCVAR_NONE);
ConVar fposu_mod_strafing_frequency_x("fposu_mod_strafing_frequency_x", 0.1f, FCVAR_NONE);
ConVar fposu_mod_strafing_strength_y("fposu_mod_strafing_strength_y", 0.1f, FCVAR_NONE);
ConVar fposu_mod_strafing_frequency_y("fposu_mod_strafing_frequency_y", 0.2f, FCVAR_NONE);
ConVar fposu_mod_strafing_strength_z("fposu_mod_strafing_strength_z", 0.15f, FCVAR_NONE);
ConVar fposu_mod_strafing_frequency_z("fposu_mod_strafing_frequency_z", 0.15f, FCVAR_NONE);

ConVar fposu_mod_3d_depthwobble("fposu_mod_3d_depthwobble", false, MCOSU_FPOSU_4D_MODE_CVAR_FLAGS);

constexpr const float OsuModFPoSu::SIZEDIV3D;
constexpr const int OsuModFPoSu::SUBDIVISIONS;

OsuModFPoSu::OsuModFPoSu(Osu *osu)
{
	m_osu = osu;

	// convar refs
	m_mouse_sensitivity_ref = convar->getConVarByName("mouse_sensitivity");
	m_osu_draw_beatmap_background_image_ref = convar->getConVarByName("osu_draw_beatmap_background_image");
	m_osu_background_alpha_ref = convar->getConVarByName("osu_background_alpha");

	// vars
	m_fCircumLength = 0.0f;
	m_camera = new Camera(Vector3(0, 0, 0), Vector3(0, 0, -1));
	m_bKeyLeftDown= false;
	m_bKeyUpDown= false;
	m_bKeyRightDown= false;
	m_bKeyDownDown= false;
	m_bKeySpaceDown= false;
	m_bKeySpaceUpDown= false;
	m_bZoomKeyDown = false;
	m_bZoomed = false;
	m_fZoomFOVAnimPercent = 0.0f;

	m_fEdgeDistance = 0.0f;
	m_bCrosshairIntersectsScreen = false;

	// load resources
	m_vao = engine->getResourceManager()->createVertexArrayObject();
	m_vaoCube = engine->getResourceManager()->createVertexArrayObject();

	m_uvPlaneModel = NULL;
	m_skyboxModel = NULL;
	m_hitcircleModel = NULL;

	m_hitcircleShader = NULL;

	// convar callbacks
	fposu_curved.setCallback( fastdelegate::MakeDelegate(this, &OsuModFPoSu::onCurvedChange) );
	fposu_distance.setCallback( fastdelegate::MakeDelegate(this, &OsuModFPoSu::onDistanceChange) );
	fposu_noclip.setCallback( fastdelegate::MakeDelegate(this, &OsuModFPoSu::onNoclipChange) );

	// init
	makePlayfield();
	makeBackgroundCube();
}

OsuModFPoSu::~OsuModFPoSu()
{
	anim->deleteExistingAnimation(&m_fZoomFOVAnimPercent);

	SAFE_DELETE(m_uvPlaneModel);
}

void OsuModFPoSu::draw(Graphics *g)
{
	if (!osu_mod_fposu.getBool()) return;

	const float fov = lerp<float>(fposu_fov.getFloat(), fposu_zoom_fov.getFloat(), m_fZoomFOVAnimPercent);
	Matrix4 projectionMatrix = fposu_vertical_fov.getBool() ? Camera::buildMatrixPerspectiveFovVertical(deg2rad(fov), ((float)m_osu->getScreenWidth()/(float)m_osu->getScreenHeight()), 0.05f, 1000.0f)
															: Camera::buildMatrixPerspectiveFovHorizontal(deg2rad(fov), ((float)m_osu->getScreenHeight() / (float)m_osu->getScreenWidth()), 0.05f, 1000.0f);
	Matrix4 viewMatrix = Camera::buildMatrixLookAt(m_camera->getPos(), m_camera->getPos() + m_camera->getViewDirection(), m_camera->getViewUp());

	// HACKHACK: there is currently no way to directly modify the viewport origin, so the only option for rendering non-2d stuff with correct offsets (i.e. top left) is by rendering into a rendertarget
	// HACKHACK: abusing sliderFrameBuffer

	m_osu->getSliderFrameBuffer()->enable();
	{
		const Vector2 resolutionBackup = g->getResolution();
		g->onResolutionChange(m_osu->getSliderFrameBuffer()->getSize()); // set renderer resolution to game resolution (to correctly support letterboxing etc.)
		{
			g->clearDepthBuffer();
			g->pushTransform();
			{
				g->setWorldMatrix(viewMatrix);
				g->setProjectionMatrix(projectionMatrix);

				g->setBlending(false);
				{
					if (!fposu_3d.getBool())
					{
						// regular fposu "2d" render path

						g->setDepthBuffer(true);
						{
							// axis lines at (0, 0, 0)
							if (fposu_noclip.getBool())
							{
								static VertexArrayObject vao(Graphics::PRIMITIVE::PRIMITIVE_LINES);
								vao.empty();
								{
									Vector3 pos = Vector3(0, 0, 0);
									float length = 1.0f;

									vao.addColor(0xffff0000);
									vao.addVertex(pos.x, pos.y, pos.z);
									vao.addColor(0xffff0000);
									vao.addVertex(pos.x + length, pos.y, pos.z);

									vao.addColor(0xff00ff00);
									vao.addVertex(pos.x, pos.y, pos.z);
									vao.addColor(0xff00ff00);
									vao.addVertex(pos.x, pos.y + length, pos.z);

									vao.addColor(0xff0000ff);
									vao.addVertex(pos.x, pos.y, pos.z);
									vao.addColor(0xff0000ff);
									vao.addVertex(pos.x, pos.y, pos.z + length);
								}
								g->setColor(0xffffffff);
								g->drawVAO(&vao);
							}

							// skybox/cube
							if (fposu_skybox.getBool())
							{
								handleLazyLoad3DModels();

								g->pushTransform();
								{
									Matrix4 modelMatrix;
									{
										Matrix4 scale;
										scale.scale(fposu_3d_skybox_size.getFloat());

										modelMatrix = scale;
									}
									g->setWorldMatrixMul(modelMatrix);

									g->setColor(0xffffffff);
									m_osu->getSkin()->getSkybox()->bind();
									{
										m_skyboxModel->draw3D(g);
									}
									m_osu->getSkin()->getSkybox()->unbind();
								}
								g->popTransform();
							}
							else if (fposu_cube.getBool())
							{
								m_osu->getSkin()->getBackgroundCube()->bind();
								{
									g->setColor(COLOR(255, clamp<int>(fposu_cube_tint_r.getInt(), 0, 255), clamp<int>(fposu_cube_tint_g.getInt(), 0, 255), clamp<int>(fposu_cube_tint_b.getInt(), 0, 255)));
									g->drawVAO(m_vaoCube);
								}
								m_osu->getSkin()->getBackgroundCube()->unbind();
							}
						}
						g->setDepthBuffer(false);

						const bool isTransparent = (m_osu_background_alpha_ref->getFloat() < 1.0f);
						if (isTransparent)
						{
							g->setBlending(true);
							g->setBlendMode(Graphics::BLEND_MODE::BLEND_MODE_PREMUL_COLOR);
						}

						Matrix4 worldMatrix = m_modelMatrix;

#ifdef MCENGINE_FEATURE_DIRECTX11
						{
							DirectX11Interface *dx11 = dynamic_cast<DirectX11Interface*>(engine->getGraphics());
							if (dx11 != NULL)
							{
								// NOTE: convert from OpenGL coordinate system
								static Matrix4 zflip = Matrix4().scale(1, 1, -1);
								worldMatrix = worldMatrix * zflip;
							}
						}
#endif

						g->setWorldMatrixMul(worldMatrix);
						{
							m_osu->getPlayfieldBuffer()->bind();
							{
								g->setColor(0xffffffff);
								g->drawVAO(m_vao);
							}
							m_osu->getPlayfieldBuffer()->unbind();
						}

						if (isTransparent)
							g->setBlendMode(Graphics::BLEND_MODE::BLEND_MODE_ALPHA);

						// (no setBlending(false), since we are already at the end)
					}
					else if (m_osu->isInPlayMode() && m_osu->getSelectedBeatmap() != NULL) // sanity
					{
						// real 3d render path (fposu_3d)

						// NOTE: the render path for fposu_3d_spheres is a bit weird in order to support max blending for the spheres
						// NOTE: this is necessary, since using regular depth testing looks like shit if spheres overlap (also "unreadable" streams/stacks/etc., also because we want proper transparency)

						handleLazyLoad3DModels();

#if defined(MCENGINE_FEATURE_OPENGL)

						const bool isOpenGLRendererHack = (dynamic_cast<OpenGLLegacyInterface*>(g) != NULL || dynamic_cast<OpenGL3Interface*>(g) != NULL);

#elif defined(MCENGINE_FEATURE_OPENGLES)

						const bool isOpenGLRendererHack = (dynamic_cast<OpenGLES2Interface*>(g) != NULL);

#endif

						if (fposu_3d_wireframe.getBool())
							g->setWireframe(true);

						if (fposu_3d_spheres.getBool())
						{
							handleLazyLoad3DShaders();

							m_hitcircleShader->enable();
							{
								m_hitcircleShader->setUniform1f("time", (float)engine->getTime());
								m_hitcircleShader->setUniform3f("eye", m_camera->getPos().x, m_camera->getPos().y, m_camera->getPos().z);
								m_hitcircleShader->setUniform1f("brightness", fposu_3d_spheres_light_brightness.getFloat());
								m_hitcircleShader->setUniform1f("ambient", fposu_3d_spheres_light_ambient.getFloat());
								m_hitcircleShader->setUniform3f("lightPosition", fposu_3d_spheres_light_position_x.getFloat(), fposu_3d_spheres_light_position_y.getFloat(), fposu_3d_spheres_light_position_z.getFloat());
								m_hitcircleShader->setUniform1f("diffuse", fposu_3d_spheres_light_diffuse.getFloat());
								m_hitcircleShader->setUniform1f("phong", fposu_3d_spheres_light_phong.getFloat());
								m_hitcircleShader->setUniform1f("phongExponent", fposu_3d_spheres_light_phong_exponent.getFloat());
								m_hitcircleShader->setUniform1f("rim", fposu_3d_spheres_light_rim.getFloat());
							}
							m_hitcircleShader->disable();

							if (fposu_3d_spheres_aa.getInt() > 0)
								g->setAntialiasing(true);

							g->setBlending(true);
							g->setCulling(true);
							{
								m_osu->getSelectedBeatmap()->draw3D(g);
							}
							g->setCulling(false);
							g->setBlending(false);

							if (fposu_3d_spheres_aa.getInt() > 0)
								g->setAntialiasing(false);
						}

						g->setDepthBuffer(true);
						{
							// axis lines at (0, 0, 0)
							if (fposu_noclip.getBool())
							{
								static VertexArrayObject vao(Graphics::PRIMITIVE::PRIMITIVE_LINES);
								vao.empty();
								{
									Vector3 pos = Vector3(0, 0, 0);
									float length = 1.0f;

									vao.addColor(0xffff0000);
									vao.addVertex(pos.x, pos.y, pos.z);
									vao.addColor(0xffff0000);
									vao.addVertex(pos.x + length, pos.y, pos.z);

									vao.addColor(0xff00ff00);
									vao.addVertex(pos.x, pos.y, pos.z);
									vao.addColor(0xff00ff00);
									vao.addVertex(pos.x, pos.y + length, pos.z);

									vao.addColor(0xff0000ff);
									vao.addVertex(pos.x, pos.y, pos.z);
									vao.addColor(0xff0000ff);
									vao.addVertex(pos.x, pos.y, pos.z + length);
								}
								g->setColor(0xffffffff);
								g->drawVAO(&vao);
							}

							g->setBlending(true);
							{
#if defined(MCENGINE_FEATURE_OPENGL) || defined (MCENGINE_FEATURE_OPENGLES)

								if (isOpenGLRendererHack)
								{
									// HACKHACK: OpenGL hardcoded
									glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_DST_ALPHA);
									glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
								}

#endif

								// beatmap background image
								if (fposu_3d_draw_beatmap_background_image.getBool())
								{
									g->pushTransform();
									{
										Matrix4 modelMatrix;
										{
											Matrix4 translate;
											translate.translate(0, 0, -(fposu_3d_beatmap_background_image_distance_multiplier.getFloat() - 1.0f)*fposu_distance.getFloat());

											Matrix4 scale;
											scale.scale(1.0f, (m_osu->getPlayfieldBuffer()->getHeight() / m_osu->getPlayfieldBuffer()->getWidth())*m_fCircumLength, 1.0f);

											modelMatrix = translate * scale;
										}
										g->setWorldMatrixMul(modelMatrix);

										g->setColor(0xffffffff);
										m_osu->getPlayfieldBuffer()->bind();
										{
											g->drawVAO(m_vao);
										}
										m_osu->getPlayfieldBuffer()->unbind();
									}
									g->popTransform();
								}

								// skybox/cube
								if (fposu_skybox.getBool() && fposu_3d_skybox.getBool())
								{
									g->pushTransform();
									{
										Matrix4 modelMatrix;
										{
											Matrix4 scale;
											scale.scale(fposu_3d_skybox_size.getFloat());

											modelMatrix = scale;
										}
										g->setWorldMatrixMul(modelMatrix);

										g->setColor(0xffffffff);
										m_osu->getSkin()->getSkybox()->bind();
										{
											m_skyboxModel->draw3D(g);
										}
										m_osu->getSkin()->getSkybox()->unbind();
									}
									g->popTransform();
								}
								else if (fposu_cube.getBool())
								{
									m_osu->getSkin()->getBackgroundCube()->bind();
									{
										g->setColor(COLOR(255, clamp<int>(fposu_cube_tint_r.getInt(), 0, 255), clamp<int>(fposu_cube_tint_g.getInt(), 0, 255), clamp<int>(fposu_cube_tint_b.getInt(), 0, 255)));
										g->drawVAO(m_vaoCube);
									}
									m_osu->getSkin()->getBackgroundCube()->unbind();
								}

#if defined(MCENGINE_FEATURE_OPENGL) || defined (MCENGINE_FEATURE_OPENGLES)

								if (isOpenGLRendererHack)
								{
									// HACKHACK: OpenGL hardcoded
									glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
									glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
								}

#endif
							}
							g->setBlending(false);
						}
						g->setDepthBuffer(false);

						if (!fposu_3d_spheres.getBool())
						{
							g->setBlending(true);
							g->setCulling(true);
							{
								m_osu->getSelectedBeatmap()->draw3D(g);
								m_osu->getSelectedBeatmap()->draw3D2(g);
							}
							g->setCulling(false);
							// (no setBlending(false), since we are already at the end)
						}
						else
						{
							g->setBlending(true);
							g->setCulling(true);
							{
								m_osu->getSelectedBeatmap()->draw3D2(g);
							}
							g->setCulling(false);
							// (no setBlending(false), since we are already at the end)
						}

						if (fposu_3d_wireframe.getBool())
							g->setWireframe(false);
					}
				}
				// (no setBlending(false), since we are already at the end)
			}
			g->popTransform();
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

	if (fposu_noclip.getBool())
		noclipMove();

	m_modelMatrix = Matrix4();
	{
		m_modelMatrix.scale(1.0f, (m_osu->getPlayfieldBuffer()->getHeight() / m_osu->getPlayfieldBuffer()->getWidth())*(m_fCircumLength), 1.0f);

		// rotate around center
		{
			m_modelMatrix.translate(0, 0, fposu_distance.getFloat()); // (compensate for mesh offset)
			{
				m_modelMatrix.rotateX(fposu_playfield_rotation_x.getFloat());
				m_modelMatrix.rotateY(fposu_playfield_rotation_y.getFloat());
				m_modelMatrix.rotateZ(fposu_playfield_rotation_z.getFloat());
			}
			m_modelMatrix.translate(0, 0, -fposu_distance.getFloat()); // (restore)
		}

		m_modelMatrix.translate(fposu_playfield_position_x.getFloat(), fposu_playfield_position_y.getFloat(), -0.0015f + fposu_playfield_position_z.getFloat()); // NOTE: slightly move back by default to avoid aliasing with background cube

		if (fposu_mod_strafing.getBool())
		{
			if (m_osu->isInPlayMode() && m_osu->getSelectedBeatmap() != NULL)
			{
				const long curMusicPos = m_osu->getSelectedBeatmap()->getCurMusicPos();

				const float speedMultiplierCompensation = 1.0f / m_osu->getSelectedBeatmap()->getSpeedMultiplier();

				const float x = std::sin((curMusicPos/1000.0f)*5*speedMultiplierCompensation*fposu_mod_strafing_frequency_x.getFloat())*fposu_mod_strafing_strength_x.getFloat();
				const float y = std::sin((curMusicPos/1000.0f)*5*speedMultiplierCompensation*fposu_mod_strafing_frequency_y.getFloat())*fposu_mod_strafing_strength_y.getFloat();
				const float z = std::sin((curMusicPos/1000.0f)*5*speedMultiplierCompensation*fposu_mod_strafing_frequency_z.getFloat())*fposu_mod_strafing_strength_z.getFloat();

				m_modelMatrix.translate(x, y, z);
			}
		}
	}

	const bool isAutoCursor = (m_osu->getModAuto() || m_osu->getModAutopilot());

	m_bCrosshairIntersectsScreen = true;
	if (!fposu_3d.getBool() && !fposu_absolute_mode.getBool() && !isAutoCursor && env->getOS() == Environment::OS::OS_WINDOWS) // HACKHACK: windows only for now (raw input support)
	{
		// regular mouse position mode

		// calculate mouse delta
		Vector2 rawDelta = engine->getMouse()->getRawDelta() / m_mouse_sensitivity_ref->getFloat(); // HACKHACK: undo engine mouse sensitivity multiplier

		// apply fposu mouse sensitivity multiplier
		const double countsPerCm = (double)fposu_mouse_dpi.getInt() / 2.54;
		const double cmPer360 = fposu_mouse_cm_360.getFloat();
		const double countsPer360 = cmPer360 * countsPerCm;
		const double multiplier = 360.0 / countsPer360;
		rawDelta *= multiplier;

		// apply zoom_sensitivity_ratio if zoomed
		if (m_bZoomed && fposu_zoom_sensitivity_ratio.getFloat() > 0.0f)
			rawDelta *= (fposu_zoom_fov.getFloat() / fposu_fov.getFloat()) * fposu_zoom_sensitivity_ratio.getFloat(); // see https://www.reddit.com/r/GlobalOffensive/comments/3vxkav/how_zoomed_sensitivity_works/

		// update camera
		if (rawDelta.x != 0.0f)
			m_camera->rotateY(rawDelta.x * (fposu_invert_horizontal.getBool() ? 1.0f : -1.0f));
		if (rawDelta.y != 0.0f)
			m_camera->rotateX(rawDelta.y * (fposu_invert_vertical.getBool() ? 1.0f : -1.0f));

		// calculate ray-mesh intersection and set new mouse pos
		Vector2 newMousePos = intersectRayMesh(m_camera->getPos(), m_camera->getViewDirection());

		const bool osCursorVisible = (env->isCursorVisible() || !env->isCursorInWindow() || !engine->hasFocus());

		if (!osCursorVisible)
		{
			// special case: force to center of screen if no intersection
			if (newMousePos.x == 0.0f && newMousePos.y == 0.0f)
			{
				m_bCrosshairIntersectsScreen = false;
				newMousePos = m_osu->getScreenSize() / 2;
			}

			setMousePosCompensated(newMousePos);
		}
	}
	else if (!fposu_3d.getBool())
	{
		// absolute mouse position mode (or auto)

		m_bCrosshairIntersectsScreen = true;

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
	else if (fposu_3d.getBool())
	{
		// 3d mouse position mode (and auto)

		// calculate mouse delta
		Vector2 delta;
		{
			if (env->getOS() == Environment::OS::OS_WINDOWS)
			{
				delta = (engine->getMouse()->getRawDelta() / m_mouse_sensitivity_ref->getFloat()); // HACKHACK: undo engine mouse sensitivity multiplier

				// apply fposu mouse sensitivity multiplier
				const double countsPerCm = (double)fposu_mouse_dpi.getInt() / 2.54;
				const double cmPer360 = fposu_mouse_cm_360.getFloat();
				const double countsPer360 = cmPer360 * countsPerCm;
				const double multiplier = 360.0 / countsPer360;
				delta *= multiplier;
			}
			else
				delta = engine->getMouse()->getDelta() * m_mouse_sensitivity_ref->getFloat();

			// apply zoom_sensitivity_ratio if zoomed
			if (m_bZoomed && fposu_zoom_sensitivity_ratio.getFloat() > 0.0f)
				delta *= (fposu_zoom_fov.getFloat() / fposu_fov.getFloat()) * fposu_zoom_sensitivity_ratio.getFloat(); // see https://www.reddit.com/r/GlobalOffensive/comments/3vxkav/how_zoomed_sensitivity_works/
		}

		// update camera
		if (delta.x != 0.0f)
			m_camera->rotateY(delta.x * (fposu_invert_horizontal.getBool() ? 1.0f : -1.0f));
		if (delta.y != 0.0f)
			m_camera->rotateX(delta.y * (fposu_invert_vertical.getBool() ? 1.0f : -1.0f));

		if (m_osu->isInPlayMode() && m_osu->getSelectedBeatmap() != NULL)
		{
			if (!isAutoCursor)
			{
				const bool osCursorVisible = (env->isCursorVisible() || !env->isCursorInWindow() || !engine->hasFocus());

				if (!osCursorVisible)
				{
					if (!m_osu->getSelectedBeatmap()->isPaused() && !m_osu->getOptionsMenu()->isVisible() && !m_osu->getModSelector()->isVisible())
						setMousePosCompensated(engine->getScreenSize()/2);
				}
			}
			else
			{
				// 3d auto support
				OsuBeatmapStandard *beatmapStd = dynamic_cast<OsuBeatmapStandard*>(m_osu->getSelectedBeatmap());
				if (beatmapStd != NULL && !beatmapStd->isPaused())
					m_camera->lookAt(beatmapStd->osuCoordsToRaw3D(beatmapStd->pixels2OsuCoords(beatmapStd->getCursorPos())));
			}
		}
	}
}

void OsuModFPoSu::noclipMove()
{
	const float noclipSpeed = fposu_noclipspeed.getFloat() * (engine->getKeyboard()->isShiftDown() ? 3.0f : 1.0f) * (engine->getKeyboard()->isControlDown() ? 0.2f : 1);
	const float noclipAccelerate = fposu_noclipaccelerate.getFloat();
	const float friction = fposu_noclipfriction.getFloat();

	// build direction vector based on player key inputs
	Vector3 wishdir;
	{
		wishdir += (m_bKeyUpDown ? m_camera->getViewDirection() : Vector3());
		wishdir -= (m_bKeyDownDown ? m_camera->getViewDirection() : Vector3());
		wishdir += (m_bKeyLeftDown ? m_camera->getViewRight() : Vector3());
		wishdir -= (m_bKeyRightDown ? m_camera->getViewRight() : Vector3());
		wishdir += (m_bKeySpaceDown ? (m_bKeySpaceUpDown ? Vector3(0.0f, 1.0f, 0.0f) : Vector3(0.0f, -1.0f, 0.0f)) : Vector3());
	}

	// normalize
	float wishspeed = 0.0f;
	{
		const float length = wishdir.length();
		if (length > 0.0f)
		{
			wishdir /= length; // normalize
			wishspeed = noclipSpeed;
		}
	}

	// friction (deccelerate)
	{
		const float spd = m_vVelocity.length();
		if (spd > 0.00000001f)
		{
			// only apply friction once we "stop" moving (special case for noclip mode)
			if (wishspeed == 0.0f)
			{
				const float drop = spd * friction * engine->getFrameTime();

				float newSpeed = spd - drop;
				{
					if (newSpeed < 0.0f)
						newSpeed = 0.0f;
				}
				newSpeed /= spd;

				m_vVelocity *= newSpeed;
			}
		}
		else
			m_vVelocity.zero();
	}

	// accelerate
	{
		float addspeed = wishspeed;
		if (addspeed > 0.0f)
		{
			float accelspeed = noclipAccelerate * engine->getFrameTime() * wishspeed;

			if (accelspeed > addspeed)
				accelspeed = addspeed;

			m_vVelocity += accelspeed * wishdir;
		}
	}

	// clamp to max speed
	if (m_vVelocity.length() > noclipSpeed)
		m_vVelocity.setLength(noclipSpeed);

	// move
	m_camera->setPos(m_camera->getPos() + m_vVelocity * engine->getFrameTime());
}

void OsuModFPoSu::onKeyDown(KeyboardEvent &key)
{
	if (key == (KEYCODE)OsuKeyBindings::FPOSU_ZOOM.getInt() && !m_bZoomKeyDown)
	{
		m_bZoomKeyDown = true;

		if (!m_bZoomed || fposu_zoom_toggle.getBool())
		{
			if (!fposu_zoom_toggle.getBool())
				m_bZoomed = true;
			else
				m_bZoomed = !m_bZoomed;

			handleZoomedChange();
		}
	}

	if (key == KEY_A)
		m_bKeyLeftDown = true;
	if (key == KEY_W)
		m_bKeyUpDown = true;
	if (key == KEY_D)
		m_bKeyRightDown = true;
	if (key == KEY_S)
		m_bKeyDownDown = true;
	if (key == KEY_SPACE)
	{
		if (!m_bKeySpaceDown)
			m_bKeySpaceUpDown = !m_bKeySpaceUpDown;

		m_bKeySpaceDown = true;
	}
}

void OsuModFPoSu::onKeyUp(KeyboardEvent &key)
{
	if (key == (KEYCODE)OsuKeyBindings::FPOSU_ZOOM.getInt())
	{
		m_bZoomKeyDown = false;

		if (m_bZoomed && !fposu_zoom_toggle.getBool())
		{
			m_bZoomed = false;
			handleZoomedChange();
		}
	}

	if (key == KEY_A)
		m_bKeyLeftDown = false;
	if (key == KEY_W)
		m_bKeyUpDown = false;
	if (key == KEY_D)
		m_bKeyRightDown = false;
	if (key == KEY_S)
		m_bKeyDownDown = false;
	if (key == KEY_SPACE)
		m_bKeySpaceDown = false;
}

float OsuModFPoSu::get3DPlayfieldScale() const
{
	return fposu_3d_playfield_scale.getFloat() * 0.75f;
}

void OsuModFPoSu::handleZoomedChange()
{
	if (m_bZoomed)
		anim->moveQuadOut(&m_fZoomFOVAnimPercent, 1.0f, (1.0f - m_fZoomFOVAnimPercent)*fposu_zoom_anim_duration.getFloat(), true);
	else
		anim->moveQuadOut(&m_fZoomFOVAnimPercent, 0.0f, m_fZoomFOVAnimPercent*fposu_zoom_anim_duration.getFloat(), true);
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
					if (denominator > 0.0f) // only allow forwards trace
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

#ifdef MCENGINE_FEATURE_DIRECTX11

	float topTC = 1.0f;
	float bottomTC = 0.0f;
	{
		DirectX11Interface *dx11 = dynamic_cast<DirectX11Interface*>(engine->getGraphics());
		if (dx11 != NULL)
		{
			topTC = 0.0f;
			bottomTC = 1.0f;
		}
	}

#else

	const float topTC = 1.0f;
	const float bottomTC = 0.0f;

#endif

	const float dist = -fposu_distance.getFloat();

	VertexPair vp1 = VertexPair(Vector3(-0.5, 0.5, dist), Vector3(-0.5, -0.5, dist), 0);
	VertexPair vp2 = VertexPair(Vector3(0.5, 0.5, dist), Vector3(0.5, -0.5, dist), 1);

	m_fEdgeDistance = Vector3(0, 0, 0).distance(Vector3(-0.5, 0.0, dist));

	m_meshList.push_back(vp1);
	m_meshList.push_back(vp2);

	std::list<VertexPair>::iterator begin = m_meshList.begin();
	std::list<VertexPair>::iterator end = m_meshList.end();
	--end;
	m_fCircumLength = subdivide(m_meshList, begin, end, SUBDIVISIONS, m_fEdgeDistance);

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

	const float size = fposu_cube_size.getFloat();

	// front
	m_vaoCube->addVertex(-size, -size, -size);  m_vaoCube->addTexcoord(0.0f, 1.0f);
	m_vaoCube->addVertex( size, -size, -size);  m_vaoCube->addTexcoord(1.0f, 1.0f);
	m_vaoCube->addVertex( size,  size, -size);  m_vaoCube->addTexcoord(1.0f, 0.0f);

	m_vaoCube->addVertex( size,  size, -size);  m_vaoCube->addTexcoord(1.0f, 0.0f);
	m_vaoCube->addVertex(-size,  size, -size);  m_vaoCube->addTexcoord(0.0f, 0.0f);
	m_vaoCube->addVertex(-size, -size, -size);  m_vaoCube->addTexcoord(0.0f, 1.0f);

	// back
	m_vaoCube->addVertex(-size, -size,  size);  m_vaoCube->addTexcoord(1.0f, 1.0f);
	m_vaoCube->addVertex( size, -size,  size);  m_vaoCube->addTexcoord(0.0f, 1.0f);
	m_vaoCube->addVertex( size,  size,  size);  m_vaoCube->addTexcoord(0.0f, 0.0f);

	m_vaoCube->addVertex( size,  size,  size);  m_vaoCube->addTexcoord(0.0f, 0.0f);
	m_vaoCube->addVertex(-size,  size,  size);  m_vaoCube->addTexcoord(1.0f, 0.0f);
	m_vaoCube->addVertex(-size, -size,  size);  m_vaoCube->addTexcoord(1.0f, 1.0f);

	// left
	m_vaoCube->addVertex(-size,  size,  size);  m_vaoCube->addTexcoord(0.0f, 0.0f);
	m_vaoCube->addVertex(-size,  size, -size);  m_vaoCube->addTexcoord(1.0f, 0.0f);
	m_vaoCube->addVertex(-size, -size, -size);  m_vaoCube->addTexcoord(1.0f, 1.0f);

	m_vaoCube->addVertex(-size, -size, -size);  m_vaoCube->addTexcoord(1.0f, 1.0f);
	m_vaoCube->addVertex(-size, -size,  size);  m_vaoCube->addTexcoord(0.0f, 1.0f);
	m_vaoCube->addVertex(-size,  size,  size);  m_vaoCube->addTexcoord(0.0f, 0.0f);

	// right
	m_vaoCube->addVertex( size,  size,  size);  m_vaoCube->addTexcoord(1.0f, 0.0f);
	m_vaoCube->addVertex( size,  size, -size);  m_vaoCube->addTexcoord(0.0f, 0.0f);
	m_vaoCube->addVertex( size, -size, -size);  m_vaoCube->addTexcoord(0.0f, 1.0f);

	m_vaoCube->addVertex( size, -size, -size);  m_vaoCube->addTexcoord(0.0f, 1.0f);
	m_vaoCube->addVertex( size, -size,  size);  m_vaoCube->addTexcoord(1.0f, 1.0f);
	m_vaoCube->addVertex( size,  size,  size);  m_vaoCube->addTexcoord(1.0f, 0.0f);

	// bottom
	m_vaoCube->addVertex(-size, -size, -size);  m_vaoCube->addTexcoord(0.0f, 0.0f);
	m_vaoCube->addVertex( size, -size, -size);  m_vaoCube->addTexcoord(1.0f, 0.0f);
	m_vaoCube->addVertex( size, -size,  size);  m_vaoCube->addTexcoord(1.0f, 1.0f);

	m_vaoCube->addVertex( size, -size,  size);  m_vaoCube->addTexcoord(1.0f, 1.0f);
	m_vaoCube->addVertex(-size, -size,  size);  m_vaoCube->addTexcoord(0.0f, 1.0f);
	m_vaoCube->addVertex(-size, -size, -size);  m_vaoCube->addTexcoord(0.0f, 0.0f);

	// top
	m_vaoCube->addVertex(-size,  size, -size);  m_vaoCube->addTexcoord(0.0f, 1.0f);
	m_vaoCube->addVertex( size,  size, -size);  m_vaoCube->addTexcoord(1.0f, 1.0f);
	m_vaoCube->addVertex( size,  size,  size);  m_vaoCube->addTexcoord(1.0f, 0.0f);

	m_vaoCube->addVertex( size,  size,  size);  m_vaoCube->addTexcoord(1.0f, 0.0f);
	m_vaoCube->addVertex(-size,  size,  size);  m_vaoCube->addTexcoord(0.0f, 0.0f);
	m_vaoCube->addVertex(-size,	 size, -size);	m_vaoCube->addTexcoord(0.0f, 1.0f);
}

void OsuModFPoSu::handleLazyLoad3DModels()
{
	const char *uvplaneObj = "# Blender 3.6.0\r\n"
			"# www.blender.org\r\n"
			"o Plane\r\n"
			"v -0.500000 -0.500000 -0.000000\r\n"
			"v 0.500000 -0.500000 -0.000000\r\n"
			"v -0.500000 0.500000 0.000000\r\n"
			"v 0.500000 0.500000 0.000000\r\n"
			"vt 1.000000 0.000000\r\n"
			"vt 0.000000 1.000000\r\n"
			"vt 0.000000 0.000000\r\n"
			"vt 1.000000 1.000000\r\n"
			"s 0\r\n"
			"f 2/1 3/2 1/3\r\n"
			"f 2/1 4/4 3/2\r\n";

	if (m_uvPlaneModel == NULL)
		m_uvPlaneModel = new OsuModFPoSu3DModel(uvplaneObj, NULL, true);
	if (m_skyboxModel == NULL)
		m_skyboxModel = new OsuModFPoSu3DModel("skybox.obj");
	if (m_hitcircleModel == NULL)
		m_hitcircleModel = new OsuModFPoSu3DModel("hitcircle.obj");
}

void OsuModFPoSu::handleLazyLoad3DShaders()
{
	if (m_hitcircleShader == NULL)
		m_hitcircleShader = engine->getResourceManager()->loadShader2("hitcircle3D.mcshader", "hitcircle3D");
}

void OsuModFPoSu::onCurvedChange(UString oldValue, UString newValue)
{
	makePlayfield();
}

void OsuModFPoSu::onDistanceChange(UString oldValue, UString newValue)
{
	makePlayfield();
}

void OsuModFPoSu::onNoclipChange(UString oldValue, UString newValue)
{
	if (fposu_noclip.getBool())
		m_camera->setPos(m_vPrevNoclipCameraPos);
	else
	{
		m_vPrevNoclipCameraPos = m_camera->getPos();
		m_camera->setPos(Vector3(0, 0, 0));
	}
}

float OsuModFPoSu::subdivide(std::list<VertexPair> &meshList, const std::list<VertexPair>::iterator &begin, const std::list<VertexPair>::iterator &end, int n, float edgeDistance)
{
	const Vector3 a = Vector3((*begin).a.x, 0.0f, (*begin).a.z);
	const Vector3 b = Vector3((*end).a.x, 0.0f, (*end).a.z);
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
	const std::list<VertexPair>::iterator newPos = meshList.insert(end, newVP);

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



OsuModFPoSu3DModel::OsuModFPoSu3DModel(const UString &objFilePathOrContents, Image *texture, bool source)
{
	m_texture = texture;

	m_vao = engine->getResourceManager()->createVertexArrayObject(Graphics::PRIMITIVE::PRIMITIVE_TRIANGLES);

	// load
	{
		struct RAW_FACE
		{
			int vertexIndex1;
			int vertexIndex2;
			int vertexIndex3;
			int uvIndex1;
			int uvIndex2;
			int uvIndex3;
			int normalIndex1;
			int normalIndex2;
			int normalIndex3;

			RAW_FACE()
			{
				vertexIndex1 = 0;
				vertexIndex2 = 0;
				vertexIndex3 = 0;
				uvIndex1 = 0;
				uvIndex2 = 0;
				uvIndex3 = 0;
				normalIndex1 = 0;
				normalIndex2 = 0;
				normalIndex3 = 0;
			}
		};

		// load model data
		std::vector<Vector3> rawVertices;
		std::vector<Vector2> rawTexcoords;
		std::vector<Color> rawColors;
		std::vector<Vector3> rawNormals;
		std::vector<RAW_FACE> rawFaces;
		{
			UString fileContents;
			if (!source)
			{
				UString filePath = "models/";
				filePath.append(objFilePathOrContents);

				std::string stdFileContents;
				{
					std::ifstream f(filePath.toUtf8(), std::ios::in | std::ios::binary);
					if (f.good())
					{
						f.seekg(0, std::ios::end);
						const std::streampos numBytes = f.tellg();
						f.seekg(0, std::ios::beg);

						stdFileContents.resize(numBytes);
						f.read(&stdFileContents[0], numBytes);
					}
					else
						debugLog("Failed to load %s\n", objFilePathOrContents.toUtf8());

					// engine->showMessageError("OsuModFPoSu3DModel Error", UString::format("Failed to load \"%s\"", objFilePathOrContents.toUtf8()));
				}
				fileContents = UString(stdFileContents.c_str(), stdFileContents.size());
			}

			std::istringstream iss(source ? objFilePathOrContents.toUtf8() : fileContents.toUtf8());
			std::string line;
			while (std::getline(iss, line))
			{
				if (line.find("v ") == 0)
				{
					Vector3 vertex;
					Vector3 rgb;

					if (sscanf(line.c_str(), "v %f %f %f %f %f %f ", &vertex.x, &vertex.y, &vertex.z, &rgb.x, &rgb.y, &rgb.z) == 6)
					{
						rawVertices.push_back(vertex);
						rawColors.push_back(COLORf(1.0f, rgb.x, rgb.y, rgb.z));
					}
					else if (sscanf(line.c_str(), "v %f %f %f ", &vertex.x, &vertex.y, &vertex.z) == 3)
						rawVertices.push_back(vertex);
				}
				else if (line.find("vt ") == 0)
				{
					Vector2 uv;
					if (sscanf(line.c_str(), "vt %f %f ", &uv.x, &uv.y) == 2)
						rawTexcoords.push_back(Vector2(uv.x, 1.0f - uv.y));
				}
				else if (line.find("vn ") == 0)
				{
					Vector3 normal;
					if (sscanf(line.c_str(), "vn %f %f %f ", &normal.x, &normal.y, &normal.z) == 3)
						rawNormals.push_back(normal);
				}
				else if (line.find("f ") == 0)
				{
					RAW_FACE face;
					if (sscanf(line.c_str(), "f %i/%i/%i %i/%i/%i %i/%i/%i ", &face.vertexIndex1, &face.uvIndex1, &face.normalIndex1, &face.vertexIndex2, &face.uvIndex2, &face.normalIndex2, &face.vertexIndex3, &face.uvIndex3, &face.normalIndex3) == 9
					 || sscanf(line.c_str(), "f %i//%i %i//%i %i//%i ", &face.vertexIndex1, &face.normalIndex1, &face.vertexIndex2, &face.normalIndex2, &face.vertexIndex3, &face.normalIndex3) == 6
					 || sscanf(line.c_str(), "f %i/%i/ %i/%i/ %i/%i/ ", &face.vertexIndex1, &face.uvIndex1, &face.vertexIndex2, &face.uvIndex2, &face.vertexIndex3, &face.uvIndex3) == 6
					 || sscanf(line.c_str(), "f %i/%i %i/%i %i/%i ", &face.vertexIndex1, &face.uvIndex1, &face.vertexIndex2, &face.uvIndex2, &face.vertexIndex3, &face.uvIndex3) == 6)
					{
						rawFaces.push_back(face);
					}
				}
			}
		}

		// build vao
		if (rawVertices.size() > 0)
		{
			const bool hasTexcoords = (rawTexcoords.size() > 0);
			const bool hasColors = (rawColors.size() > 0);
			const bool hasNormals = (rawNormals.size() > 0);

			bool hasAtLeastOneTriangle = false;

			for (size_t i=0; i<rawFaces.size(); i++)
			{
				const RAW_FACE &face = rawFaces[i];

				if ((size_t)(face.vertexIndex1 - 1) < rawVertices.size()
				 && (size_t)(face.vertexIndex2 - 1) < rawVertices.size()
				 && (size_t)(face.vertexIndex3 - 1) < rawVertices.size()
				 && (!hasTexcoords || (size_t)(face.uvIndex1 - 1) < rawTexcoords.size())
				 && (!hasTexcoords || (size_t)(face.uvIndex2 - 1) < rawTexcoords.size())
				 && (!hasTexcoords || (size_t)(face.uvIndex3 - 1) < rawTexcoords.size())
				 && (!hasColors || (size_t)(face.vertexIndex1 - 1) < rawColors.size())
				 && (!hasColors || (size_t)(face.vertexIndex2 - 1) < rawColors.size())
				 && (!hasColors || (size_t)(face.vertexIndex3 - 1) < rawColors.size())
				 && (!hasNormals || (size_t)(face.normalIndex1 - 1) < rawNormals.size())
				 && (!hasNormals || (size_t)(face.normalIndex2 - 1) < rawNormals.size())
				 && (!hasNormals || (size_t)(face.normalIndex3 - 1) < rawNormals.size()))
				{
					hasAtLeastOneTriangle = true;

					m_vao->addVertex(rawVertices[(size_t)(face.vertexIndex1 - 1)]);
					if (hasTexcoords)
						m_vao->addTexcoord(rawTexcoords[(size_t)(face.uvIndex1 - 1)]);
					if (hasColors)
						m_vao->addColor(rawColors[(size_t)(face.vertexIndex1 - 1)]);
					if (hasNormals)
						m_vao->addNormal(rawNormals[(size_t)(face.normalIndex1 - 1)]);

					m_vao->addVertex(rawVertices[(size_t)(face.vertexIndex2 - 1)]);
					if (hasTexcoords)
						m_vao->addTexcoord(rawTexcoords[(size_t)(face.uvIndex2 - 1)]);
					if (hasColors)
						m_vao->addColor(rawColors[(size_t)(face.vertexIndex2 - 1)]);
					if (hasNormals)
						m_vao->addNormal(rawNormals[(size_t)(face.normalIndex2 - 1)]);

					m_vao->addVertex(rawVertices[(size_t)(face.vertexIndex3 - 1)]);
					if (hasTexcoords)
						m_vao->addTexcoord(rawTexcoords[(size_t)(face.uvIndex3 - 1)]);
					if (hasColors)
						m_vao->addColor(rawColors[(size_t)(face.vertexIndex3 - 1)]);
					if (hasNormals)
						m_vao->addNormal(rawNormals[(size_t)(face.normalIndex3 - 1)]);
				}
			}

			// bake it for performance
			if (hasAtLeastOneTriangle)
				engine->getResourceManager()->loadResource(m_vao);
		}
	}
}

OsuModFPoSu3DModel::~OsuModFPoSu3DModel()
{
	engine->getResourceManager()->destroyResource(m_vao);
}

void OsuModFPoSu3DModel::draw3D(Graphics *g)
{
	if (m_texture != NULL)
		m_texture->bind();

	g->drawVAO(m_vao);

	if (m_texture != NULL)
		m_texture->unbind();
}
