//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		handles all VR related stuff (input, scene, screens)
//
// $NoKeywords: $osuvr
//===============================================================================//

#include "OsuVR.h"
#include "Engine.h"
#include "SoundEngine.h"
#include "ResourceManager.h"
#include "OpenVRInterface.h"
#include "OpenVRController.h"
#include "VertexArrayObject.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "Console.h"
#include "ConVar.h"

#include "Osu.h"
#include "OsuHUD.h"
#include "OsuSkin.h"
#include "OsuBeatmap.h"
#include "OsuGameRules.h"
#include "OsuKeyBindings.h"
#include "OsuNotificationOverlay.h"

ConVar osu_vr_matrix_screen("osu_vr_matrix_screen");
ConVar osu_vr_matrix_playfield("osu_vr_matrix_playfield");
ConVar osu_vr_reset_matrices("osu_vr_reset_matrices");

ConVar osu_vr_screen_scale("osu_vr_screen_scale", 1.64613f);
ConVar osu_vr_playfield_scale("osu_vr_playfield_scale", 0.945593f);
ConVar osu_vr_hud_scale("osu_vr_hud_scale", 4.0f);
ConVar osu_vr_controller_pinch_scale_multiplier("osu_vr_controller_pinch_scale_multiplier", 2.0f);
ConVar osu_vr_controller_warning_distance_enabled("osu_vr_controller_warning_distance_enabled", true);
ConVar osu_vr_controller_warning_distance_start("osu_vr_controller_warning_distance_start", -0.20f);
ConVar osu_vr_controller_warning_distance_end("osu_vr_controller_warning_distance_end", -0.30f);

ConVar osu_vr_approach_distance("osu_vr_approach_distance", 15.0f, "distance from which hitobjects start flying + fading in, in meters");
ConVar osu_vr_hud_distance("osu_vr_hud_distance", 15.0f, "distance at which the HUD is drawn, in meters");
ConVar osu_vr_playfield_native_draw_scale("osu_vr_playfield_native_draw_scale", 0.003f);

ConVar osu_vr_circle_hitbox_scale("osu_vr_circle_hitbox_scale", 1.5f, "scales the invisible hitbox radius of all hitobjects, to make it feel better");

ConVar osu_vr_cursor_alpha("osu_vr_cursor_alpha", 0.15f);

ConVar osu_vr_controller_vibration_strength("osu_vr_controller_vibration_strength", 0.45f);
ConVar osu_vr_slider_controller_vibration_strength("osu_vr_slider_controller_vibration_strength", /*0.05f*/0.0f);

ConVar osu_vr_draw_playfield("osu_vr_draw_playfield", true);
ConVar osu_vr_draw_floor("osu_vr_draw_floor", true);

ConVar osu_vr_ui_offset("osu_vr_ui_offset", 0.1f);

const char *OsuVR::OSUVR_CONFIG_FILE_NAME = "osuvrplayarea";



class OsuVRUIElement
{
public:
	OsuVRUIElement(OsuVR *vr, float x, float y, float width, float height)
	{
		m_vr = vr;
		m_vPos = Vector2(x, y);
		m_vSize = Vector2(width, height);
		m_bIsVisible = true;
		m_bIsActive = false;
		m_bIsCursorInside = false;
	}
	virtual ~OsuVRUIElement() {;}

	virtual void drawVR(Graphics *g, Matrix4 &mvp) {;}
	virtual void update(Vector2 cursorPos)
	{
		McRect bounds(m_vPos.x, -m_vPos.y, m_vSize.x, m_vSize.y);
		if (bounds.contains(Vector2(cursorPos.x, -cursorPos.y)))
			m_bIsCursorInside = true;
		else
			m_bIsCursorInside = false;
	}

	void setPos(Vector2 pos) {m_vPos = pos;}
	void setPos(float x, float y) {m_vPos.x = x; m_vPos.y = y;}
	void setPosX(float x) {m_vPos.x = x;}
	void setPosY(float y) {m_vPos.y = y;}
	void setSize(Vector2 size) {m_vSize = size;}

	void setVisible(bool visible) {m_bIsVisible = visible;}

	inline const Vector2& getPos() const {return m_vPos;}
	inline const Vector2& getSize() const {return m_vSize;}

	inline bool isVisible() {return m_bIsVisible;}
	inline bool isActive() {return m_bIsActive && isVisible();}
	inline bool isCursorInside() {return m_bIsCursorInside && isVisible();}

protected:
	OsuVR *m_vr;

	Vector2 m_vPos;
	Vector2 m_vSize;

	bool m_bIsVisible;
	bool m_bIsActive;
	bool m_bIsCursorInside;
};

class OsuVRUIButton : public OsuVRUIElement
{
public:
	OsuVRUIButton(OsuVR *vr, float x, float y, float width, float height) : OsuVRUIElement(vr, x, y, width, height)
	{
		m_bClickCheck = false;
		m_clickVoidCallback = NULL;
	}

	virtual void drawVR(Graphics *g, Matrix4 &mvp)
	{
		OsuVRUIElement::drawVR(g, mvp);
		if (!m_bIsVisible) return;

		const float lineThickness = 0.0035f;

		// border line mesh
		VertexArrayObject vao2(Graphics::PRIMITIVE::PRIMITIVE_QUADS);

		vao2.addVertex(0, 0, 0);
		vao2.addVertex(lineThickness, 0, 0);
		vao2.addVertex(lineThickness, -m_vSize.y, 0);
		vao2.addVertex(0, -m_vSize.y, 0);

		vao2.addVertex(m_vSize.x, 0, 0);
		vao2.addVertex(m_vSize.x - lineThickness, 0, 0);
		vao2.addVertex(m_vSize.x - lineThickness, -m_vSize.y, 0);
		vao2.addVertex(m_vSize.x, -m_vSize.y, 0);

		vao2.addVertex(0, 0, 0);
		vao2.addVertex(m_vSize.x, 0, 0);
		vao2.addVertex(m_vSize.x, -lineThickness, 0);
		vao2.addVertex(0, -lineThickness, 0);

		vao2.addVertex(0, -m_vSize.y, 0);
		vao2.addVertex(m_vSize.x, -m_vSize.y, 0);
		vao2.addVertex(m_vSize.x, -m_vSize.y + lineThickness, 0);
		vao2.addVertex(0, -m_vSize.y + lineThickness, 0);

		// position
		Matrix4 translation;
		translation.translate(m_vPos.x, m_vPos.y, 0);
		Matrix4 finalMVP = mvp * translation;

		// draw border line
		m_vr->getShaderUntexturedGeneric()->enable();
		m_vr->getShaderUntexturedGeneric()->setUniformMatrix4fv("matrix", finalMVP);
		{
			g->setColor(m_bIsCursorInside ? 0xffffffff : 0xff666666);
			g->drawVAO(&vao2);
		}
		m_vr->getShaderUntexturedGeneric()->disable();
	}

	virtual void update(Vector2 cursorPos)
	{
		OsuVRUIElement::update(cursorPos);
		if (!m_bIsVisible) return;

		OpenVRController *controller = openvr->getController();

		if (controller->getTrigger() > 0.95f || controller->isButtonPressed(OpenVRController::BUTTON::BUTTON_STEAMVR_TOUCHPAD))
		{
			// trigger click callback once (until released again)
			if (!m_bClickCheck)
			{
				m_bClickCheck = true;

				// within bounds
				if (m_bIsCursorInside)
				{
					m_bIsActive = true;
					if (m_clickVoidCallback != NULL)
						m_clickVoidCallback();
				}
			}
		}
		else
		{
			m_bClickCheck = false;
			m_bIsActive = false;
		}
	}

	typedef fastdelegate::FastDelegate0<> ButtonClickVoidCallback;
	void setClickCallback(ButtonClickVoidCallback clickCallback) {m_clickVoidCallback = clickCallback;}

private:
	bool m_bClickCheck;
	ButtonClickVoidCallback m_clickVoidCallback;
};

class OsuVRUIImageButton : public OsuVRUIButton
{
public:
	OsuVRUIImageButton(OsuVR *vr, float x, float y, float width, float height, UString imageResourceName) : OsuVRUIButton(vr, x, y, width, height)
	{
		m_sImageResourceName = imageResourceName;
		updateImageResource();
	}

	virtual void drawVR(Graphics *g, Matrix4 &mvp)
	{
		OsuVRUIButton::drawVR(g, mvp);
		if (!m_bIsVisible) return;
		if (m_image == NULL) return;

		// icon mesh
		VertexArrayObject vao(Graphics::PRIMITIVE::PRIMITIVE_QUADS);

		Color color = m_bIsCursorInside ? 0xffffffff : 0xff666666;

		vao.addVertex(0, 0, 0);
		vao.addTexcoord(0, 0);
		vao.addColor(color);
		vao.addVertex(m_vSize.x, 0, 0);
		vao.addTexcoord(1, 0);
		vao.addColor(color);
		vao.addVertex(m_vSize.x, -m_vSize.y, 0);
		vao.addTexcoord(1, 1);
		vao.addColor(color);
		vao.addVertex(0, -m_vSize.y, 0);
		vao.addTexcoord(0, 1);
		vao.addColor(color);

		// position
		Matrix4 translation;
		translation.translate(m_vPos.x, m_vPos.y, 0);
		Matrix4 finalMVP = mvp * translation;

		// draw icon
		m_vr->getShaderTexturedGeneric()->enable();
		m_vr->getShaderTexturedGeneric()->setUniformMatrix4fv("matrix", finalMVP);
		{
			if (m_image != NULL)
				m_image->bind();
			g->drawVAO(&vao);
			if (m_image != NULL)
				m_image->unbind();
		}
		m_vr->getShaderTexturedGeneric()->disable();
	}

	virtual void update(Vector2 cursorPos)
	{
		OsuVRUIButton::update(cursorPos);
		if (!m_bIsVisible) return;

		if (m_image == NULL)
			updateImageResource();
	}

private:
	void updateImageResource()
	{
		m_image = engine->getResourceManager()->getImage(m_sImageResourceName);
	}

	UString m_sImageResourceName;
	Image *m_image;
};

class OsuVRUISlider : public OsuVRUIElement
{
public:
	OsuVRUISlider(OsuVR *vr, float x, float y, float width, float height) : OsuVRUIElement(vr, x, y, width, height)
	{
		m_bClickCheck = false;
		m_sliderChangeCallback = NULL;
		m_fMinValue = 0.0f;
		m_fMaxValue = 1.0f;
		m_fCurValue = 0.0f;
		m_fCurPercent = 0.0f;
	}

	virtual void drawVR(Graphics *g, Matrix4 &mvp)
	{
		OsuVRUIElement::drawVR(g, mvp);
		if (!m_bIsVisible) return;

		const float lineThickness = 0.0035f;

		Color color = (m_bIsCursorInside || m_bIsActive) ? 0xffffffff : 0xff666666;

		// border line mesh
		VertexArrayObject vao(Graphics::PRIMITIVE::PRIMITIVE_QUADS);

		vao.addVertex(0, 0, 0);
		vao.addVertex(lineThickness, 0, 0);
		vao.addVertex(lineThickness, -m_vSize.y, 0);
		vao.addVertex(0, -m_vSize.y, 0);

		vao.addVertex(m_vSize.x, 0, 0);
		vao.addVertex(m_vSize.x - lineThickness, 0, 0);
		vao.addVertex(m_vSize.x - lineThickness, -m_vSize.y, 0);
		vao.addVertex(m_vSize.x, -m_vSize.y, 0);

		vao.addVertex(0, 0, 0);
		vao.addVertex(m_vSize.x, 0, 0);
		vao.addVertex(m_vSize.x, -lineThickness, 0);
		vao.addVertex(0, -lineThickness, 0);

		vao.addVertex(0, -m_vSize.y, 0);
		vao.addVertex(m_vSize.x, -m_vSize.y, 0);
		vao.addVertex(m_vSize.x, -m_vSize.y + lineThickness, 0);
		vao.addVertex(0, -m_vSize.y + lineThickness, 0);

		// percentage fill mesh
		VertexArrayObject vao2(Graphics::PRIMITIVE::PRIMITIVE_QUADS);

		vao2.addVertex(0, 0, 0);
		vao2.addVertex(m_vSize.x*m_fCurPercent, 0, 0);
		vao2.addVertex(m_vSize.x*m_fCurPercent, -m_vSize.y, 0);
		vao2.addVertex(0, -m_vSize.y, 0);

		// position
		Matrix4 translation;
		translation.translate(m_vPos.x, m_vPos.y, 0);
		Matrix4 finalMVP = mvp * translation;

		// draw border line and fill
		m_vr->getShaderUntexturedGeneric()->enable();
		m_vr->getShaderUntexturedGeneric()->setUniformMatrix4fv("matrix", finalMVP);
		{
			g->setColor(color);
			g->drawVAO(&vao);
			g->drawVAO(&vao2);
		}
		m_vr->getShaderUntexturedGeneric()->disable();


	}

	virtual void update(Vector2 cursorPos)
	{
		OsuVRUIElement::update(cursorPos);
		if (!m_bIsVisible) return;

		OpenVRController *controller = openvr->getController();

		if (controller->getTrigger() > 0.95f || controller->isButtonPressed(OpenVRController::BUTTON::BUTTON_STEAMVR_TOUCHPAD))
		{
			// trigger once if clicked
			if (!m_bClickCheck)
			{
				m_bClickCheck = true;

				// within bounds
				if (m_bIsCursorInside)
					m_bIsActive = true;
			}
		}
		else
		{
			m_bClickCheck = false;
			m_bIsActive = false;
		}

		// handle sliding
		if (m_bIsActive)
		{
			const float percent = clamp<float>((cursorPos.x - m_vPos.x)/m_vSize.x, 0.0f, 1.0f);
			bool hasChanged = percent != m_fCurPercent;
			m_fCurPercent = percent;
			m_fCurValue = lerp<float>(m_fMinValue, m_fMaxValue, m_fCurPercent);

			if (m_sliderChangeCallback != NULL && hasChanged)
				m_sliderChangeCallback(this);
		}
	}

	typedef fastdelegate::FastDelegate1<OsuVRUISlider*> SliderChangeCallback;
	void setChangeCallback(SliderChangeCallback changeCallback) {m_sliderChangeCallback = changeCallback;}

	void setValue(float value, bool ignoreCallback = false)
	{
		if (value == m_fCurValue || m_bIsActive) return;

		float newValue = clamp<float>(value, m_fMinValue, m_fMaxValue);
		if (newValue == m_fCurValue) return;

		m_fCurValue = newValue;
		m_fCurPercent = clamp<float>((m_fCurValue-m_fMinValue) / (std::abs(m_fMaxValue-m_fMinValue)), 0.0f, 1.0f);

		if (m_sliderChangeCallback != NULL && !ignoreCallback)
			m_sliderChangeCallback(this);
	}

	inline float getFloat() {return m_fCurValue;}

private:
	SliderChangeCallback m_sliderChangeCallback;

	float m_fMinValue, m_fMaxValue, m_fCurValue, m_fCurPercent;
	bool m_bClickCheck;
};



OsuVR::OsuVR(Osu *osu)
{
	m_osu = osu;

	// convar refs
	m_osu_draw_cursor_trail_ref = convar->getConVarByName("osu_draw_cursor_trail");
	m_osu_volume_master_ref = convar->getConVarByName("osu_volume_master");

	// convar callbacks
	osu_vr_matrix_screen.setCallback( fastdelegate::MakeDelegate(this, &OsuVR::onScreenMatrixChange) );
	osu_vr_matrix_playfield.setCallback( fastdelegate::MakeDelegate(this, &OsuVR::onPlayfieldMatrixChange) );
	osu_vr_reset_matrices.setCallback( fastdelegate::MakeDelegate(this, &OsuVR::resetMatrices) );

	// vars
	m_bMovingPlayfieldCheck = true;
	m_bMovingPlayfieldAllow = true;
	m_bMovingScreenCheck = true;
	m_bMovingScreenAllow = true;
	m_bIsLeftControllerMovingScreen = false;
	m_bIsRightControllerMovingScreen = true;

	m_bDrawLaser = false;
	m_bScreenIntersection = false;
	m_bPlayfieldIntersection1 = false;
	m_bPlayfieldIntersection2 = false;
	m_fPlayfieldCursorDist1 = 0.0f;
	m_fPlayfieldCursorDist2 = 0.0f;
	m_fPlayfieldCursorDistSigned1 = 0.0f;
	m_fPlayfieldCursorDistSigned2 = 0.0f;

	m_fAspect = 1.0f;

	m_bClickCheck = true;
	m_bMenuCheck = true;

	m_bScaleCheck = true;
	m_bIsPlayerScalingPlayfield = false;
	m_fScaleBackup = osu_vr_screen_scale.getFloat();
	m_fDefaultScreenScale = osu_vr_screen_scale.getFloat();
	m_fDefaultPlayfieldScale = osu_vr_playfield_scale.getFloat();

	m_vPlayfieldCursorPos1.x = 0;
	m_vPlayfieldCursorPos1.y = 0;

	m_vPlayfieldCursorPos2.x = OsuGameRules::OSU_COORD_WIDTH;
	m_vPlayfieldCursorPos2.y = OsuGameRules::OSU_COORD_HEIGHT;

	// UI elements
	OsuVRUISlider *slider = new OsuVRUISlider(this, 0, 0, 1.0f, 0.125f);
	slider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuVR::onVolumeSliderChange) );
	m_volumeSlider = slider;
	m_uiElements.push_back(m_volumeSlider);

	OsuVRUIImageButton *button = new OsuVRUIImageButton(this, 0, 0, 0.36f, 0.30f, "OSU_VR_UI_ICON_KEYBOARD");
	button->setClickCallback( fastdelegate::MakeDelegate(this, &OsuVR::onKeyboardButtonClicked) );
	m_keyboardButton = button;
	m_uiElements.push_back(m_keyboardButton);

	button = new OsuVRUIImageButton(this, 0, 0, 0.30f, 0.30f, "OSU_VR_UI_ICON_MINUS");
	button->setClickCallback( fastdelegate::MakeDelegate(this, &OsuVR::onOffsetDownClicked) );
	m_offsetDownButton = button;
	m_offsetDownButton->setVisible(false);
	m_uiElements.push_back(m_offsetDownButton);

	button = new OsuVRUIImageButton(this, 0, 0, 0.30f, 0.30f, "OSU_VR_UI_ICON_PLUS");
	button->setClickCallback( fastdelegate::MakeDelegate(this, &OsuVR::onOffsetUpClicked) );
	m_offsetUpButton = button;
	m_offsetUpButton->setVisible(false);
	m_uiElements.push_back(m_offsetUpButton);

	slider = new OsuVRUISlider(this, 0, 0, 1.5f, 0.10f);
	slider->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuVR::onScrubbingSliderChange) );
	m_scrubbingSlider = slider;
	m_uiElements.push_back(m_scrubbingSlider);

	button = new OsuVRUIImageButton(this, 0, 0, 0.30f, 0.30f, "OSU_VR_UI_ICON_RESET");
	button->setClickCallback( fastdelegate::MakeDelegate(this, &OsuVR::onMatrixResetClicked) );
	m_matrixResetButton = button;
	m_uiElements.push_back(m_matrixResetButton);

	// load/execute VR stuff only in VR builds
	if (openvr->isReady())
	{
		engine->getResourceManager()->loadImage("ic_keyboard_white_48dp.png", "OSU_VR_UI_ICON_KEYBOARD");
		engine->getResourceManager()->loadImage("ic_replay_white_48dp.png", "OSU_VR_UI_ICON_RESET");
		engine->getResourceManager()->loadImage("ic_add_white_48dp.png", "OSU_VR_UI_ICON_PLUS");
		engine->getResourceManager()->loadImage("ic_remove_white_48dp.png", "OSU_VR_UI_ICON_MINUS");

		m_shaderGenericTextured = engine->getResourceManager()->createShader(

				// vertex Shader
				"#version 110\n"
				"uniform mat4 matrix;\n"
				"varying vec2 texCoords;\n"
				"void main()\n"
				"{\n"
				"	texCoords = gl_MultiTexCoord0.xy;\n"
				"	gl_Position = matrix * gl_Vertex;\n"
				"	gl_FrontColor = gl_Color;\n"
				"}\n",

				// fragment Shader
				"#version 110\n"
				"uniform sampler2D mytexture;\n"
				"varying vec2 texCoords;\n"
				"void main()\n"
				"{\n"
				"   gl_FragColor = texture2D(mytexture, texCoords) * gl_Color;\n"
				"}\n"
		);

		m_shaderTexturedLegacyGeneric = engine->getResourceManager()->createShader(

				// vertex Shader
				"#version 110\n"
				"uniform mat4 matrix;\n"
				"varying vec2 texCoords;\n"
				"void main()\n"
				"{\n"
				"	texCoords = gl_MultiTexCoord0.xy;\n"
				"	vec4 vertex = gl_Vertex;\n"
				"	gl_Position = matrix * gl_ModelViewMatrix * vertex;\n"
				"	gl_FrontColor = gl_Color;\n"
				"}\n",

				// fragment Shader
				"#version 110\n"
				"uniform sampler2D mytexture;\n"
				"varying vec2 texCoords;\n"
				"void main()\n"
				"{\n"
				"   gl_FragColor = texture2D(mytexture, texCoords) * gl_Color;\n"
				"}\n"
		);
		///m_shaderTexturedLegacyGeneric = engine->getResourceManager()->loadShader("texturedLegacyGeneric.vsh", "texturedLegacyGeneric.fsh");

		m_shaderGenericUntextured = engine->getResourceManager()->createShader(

				// vertex Shader
				"#version 110\n"
				"uniform mat4 matrix;\n"
				"void main()\n"
				"{\n"
				"	gl_Position = matrix * gl_Vertex;\n"
				"	gl_FrontColor = gl_Color;\n"
				"}\n",

				// fragment Shader
				"#version 110\n"
				"void main()\n"
				"{\n"
				"   gl_FragColor = gl_Color;\n"
				"}\n"
		);

		m_shaderUntexturedLegacyGeneric = engine->getResourceManager()->createShader(

				// vertex Shader
				"#version 110\n"
				"uniform mat4 matrix;\n"
				"void main()\n"
				"{\n"
				"	vec4 vertex = gl_Vertex;\n"
				"	gl_Position = matrix * gl_ModelViewMatrix * vertex;\n"
				"	gl_FrontColor = gl_Color;\n"
				"}\n",

				// fragment Shader
				"#version 110\n"
				"void main()\n"
				"{\n"
				"   gl_FragColor = gl_Color;\n"
				"}\n"
		);

		// set default matrices to conform to play area
		resetMatrices();

		// execute the vr config file (this then overwrites the matrices with the user settings)
		Console::execConfigFile(OSUVR_CONFIG_FILE_NAME);
	}
}

OsuVR::~OsuVR()
{
	for (int i=0; i<m_uiElements.size(); i++)
	{
		delete m_uiElements[i];
	}
}

void OsuVR::drawVR(Graphics *g, Matrix4 &mvp, RenderTarget *screen)
{
	// draw floor
	if (osu_vr_draw_floor.getBool())
	{
		m_shaderGenericUntextured->enable();
		{
			m_shaderGenericUntextured->setUniformMatrix4fv("matrix", mvp);

			VertexArrayObject vao(Graphics::PRIMITIVE::PRIMITIVE_QUADS);

			float width = openvr->getPlayAreaSize().x;
			float height = openvr->getPlayAreaSize().y;

			vao.addVertex(width/2.0f, 0.0f, height/2.0f);
			vao.addVertex(width/2.0f, 0.0f, -height/2.0f);
			vao.addVertex(-width/2.0f, 0.0f, -height/2.0f);
			vao.addVertex(-width/2.0f, 0.0f, height/2.0f);

			g->setColor(0xff111111);
			g->drawVAO(&vao);

			if (Osu::debug->getBool())
			{
				g->setColor(0xff00ff00);
				VertexArrayObject vao2(Graphics::PRIMITIVE::PRIMITIVE_LINES);
				OpenVRInterface::PLAY_AREA_RECT playAreaRect = openvr->getPlayAreaRect();
				for (int i=0; i<4; i++)
				{
					vao2.addVertex(playAreaRect.corners[i]);
					vao2.addVertex(playAreaRect.corners[i] + Vector3(0, 0.15f, 0));
				}
				g->drawVAO(&vao2);

				VertexArrayObject vao3(Graphics::PRIMITIVE::PRIMITIVE_LINES);
				Vector2 playAreaSize = openvr->getPlayAreaSize();
				vao3.addVertex(Vector3(0, 0.001f, 0));
				vao3.addColor(0xffff0000);
				vao3.addVertex(Vector3(playAreaSize.x/2.0f, 0.001f, 0));
				vao3.addColor(0xffff0000);
				vao3.addVertex(Vector3(0, 0.001f, 0));
				vao3.addColor(0xff00ff00);
				vao3.addVertex(Vector3(0, 0.001f, playAreaSize.y/2.0f));
				vao3.addColor(0xff00ff00);
				g->drawVAO(&vao3);
			}
		}
		m_shaderGenericUntextured->disable();
	}

	// draw screen
	Matrix4 finalScreenMatrix = mvp * m_screenMatrix;
	m_shaderGenericTextured->enable();
	{
		m_shaderGenericTextured->setUniformMatrix4fv("matrix", finalScreenMatrix);

		const float scale = osu_vr_screen_scale.getFloat();
		m_fAspect = screen->getHeight() / screen->getWidth();
		float x = 0;
		float y = 0;
		m_vVirtualScreenSize.x = scale;
		m_vVirtualScreenSize.y = -m_fAspect*scale;
		float width = m_vVirtualScreenSize.x;
		float height = m_vVirtualScreenSize.y;
		screen->bind();
		{
			VertexArrayObject vao;

			vao.addTexcoord(0, 1);
			vao.addVertex(x, y);

			vao.addTexcoord(0, 0);
			vao.addVertex(x, y+height);

			vao.addTexcoord(1, 0);
			vao.addVertex(x+width, y+height);

			vao.addTexcoord(1, 0);
			vao.addVertex(x+width, y+height);

			vao.addTexcoord(1, 1);
			vao.addVertex(x+width, y);

			vao.addTexcoord(0, 1);
			vao.addVertex(x, y);

			// if the keyboard is visible we want to make the screen pretty transparent, to reduce the amount of brainfuck due to depth overlaps
			if (!openvr->isKeyboardVisible())
				g->setBlending(false);

			g->setColor(0xffffffff);

			if (openvr->isKeyboardVisible())
				g->setAlpha(0.15f);

			g->drawVAO(&vao);

			if (!openvr->isKeyboardVisible())
				g->setBlending(true);
		}
		screen->unbind();
	}
	m_shaderGenericTextured->disable();

	// draw VR UI elements (buttons etc.)
	for (int i=0; i<m_uiElements.size(); i++)
	{
		m_uiElements[i]->drawVR(g, finalScreenMatrix);
	}

	// draw controller laser
	if (m_bDrawLaser && openvr->hasInputFocus())
	{
		m_shaderGenericUntextured->enable();
		{
			m_shaderGenericUntextured->setUniformMatrix4fv("matrix", mvp);

			VertexArrayObject vao(Graphics::PRIMITIVE::PRIMITIVE_LINES);

			vao.addVertex(openvr->getController()->getPosition());
			vao.addVertex(m_vScreenIntersectionPoint);

			g->setColor(0x44ffffff);
			g->drawVAO(&vao);
		}
		m_shaderGenericUntextured->disable();
	}

	/*
	VertexArrayObject playfieldWireframeVao(Graphics::PRIMITIVE::PRIMITIVE_LINES);
	Vector4 topLeft = m_playfieldMatrix * Vector4((-OsuGameRules::OSU_COORD_WIDTH/2.0f)*getDrawScale(), (OsuGameRules::OSU_COORD_HEIGHT/2.0f)*getDrawScale(), 0, 1);
	playfieldWireframeVao.addVertex(topLeft.x, topLeft.y, topLeft.z);
	playfieldWireframeVao.addVertex(topLeft.x, topLeft.y, topLeft.z - osu_vr_approach_distance.getFloat());
	playfieldWireframeVao.addVertex(topLeft.x + OsuGameRules::OSU_COORD_WIDTH*getDrawScale(), topLeft.y, topLeft.z);
	playfieldWireframeVao.addVertex(topLeft.x + OsuGameRules::OSU_COORD_WIDTH*getDrawScale(), topLeft.y, topLeft.z - osu_vr_approach_distance.getFloat());

	g->setColor(0x44ffffff);
	g->drawVAO(&playfieldWireframeVao);
	*/
}

void OsuVR::drawVRBeatmap(Graphics *g, Matrix4 &mvp, OsuBeatmap *beatmap)
{
	if (!osu_vr_draw_playfield.getBool())
		return;

	Matrix4 scale;
	scale.scale(getDrawScale(), -getDrawScale(), getDrawScale());
	Matrix4 finalMVP = mvp * m_playfieldMatrix * scale;

	// draw beatmap (this also draws the border, because the border is dependent on the current CS while playing)
	beatmap->drawVR(g, finalMVP, this);

	// draw cursors
	drawVRCursors(g, finalMVP);
}

void OsuVR::drawVRHUD(Graphics *g, Matrix4 &mvp, OsuHUD *hud)
{
	if (!osu_vr_draw_playfield.getBool())
		return;

	Matrix4 scale;
	scale.scale(getDrawScale()*osu_vr_hud_scale.getFloat(), -getDrawScale()*osu_vr_hud_scale.getFloat(), getDrawScale()*osu_vr_hud_scale.getFloat());
	Matrix4 translation;
	translation.translate(-(m_osu->getScreenWidth()/2.0f)*getDrawScale()*osu_vr_hud_scale.getFloat(), (m_osu->getScreenHeight()/2.0f)*getDrawScale()*osu_vr_hud_scale.getFloat(), -osu_vr_hud_distance.getFloat());
	Matrix4 finalMVP = mvp * m_playfieldMatrix * translation * scale;

	hud->drawVR(g, finalMVP, this);
}

void OsuVR::drawVRHUDDummy(Graphics *g, Matrix4 &mvp, OsuHUD *hud)
{
	Matrix4 scale;
	scale.scale(getDrawScale()*osu_vr_hud_scale.getFloat(), -getDrawScale()*osu_vr_hud_scale.getFloat(), getDrawScale()*osu_vr_hud_scale.getFloat());
	Matrix4 translation;
	translation.translate(-(m_osu->getScreenWidth()/2.0f)*getDrawScale()*osu_vr_hud_scale.getFloat(), (m_osu->getScreenHeight()/2.0f)*getDrawScale()*osu_vr_hud_scale.getFloat(), -osu_vr_hud_distance.getFloat());
	Matrix4 finalMVP = mvp * m_playfieldMatrix * translation * scale;

	hud->drawVRDummy(g, finalMVP, this);
}

void OsuVR::drawVRPlayfieldDummy(Graphics *g, Matrix4 &mvp)
{
	Matrix4 scale;
	scale.scale(getDrawScale(), -getDrawScale(), getDrawScale());
	Matrix4 finalMVP = mvp * m_playfieldMatrix * scale;

	// draw border
	m_shaderUntexturedLegacyGeneric->enable();
	{
		m_shaderUntexturedLegacyGeneric->setUniformMatrix4fv("matrix", finalMVP);

		g->setColor(0xffffffff);
		m_osu->getHUD()->drawPlayfieldBorder(g, Vector2(0,0), Vector2(OsuGameRules::OSU_COORD_WIDTH, OsuGameRules::OSU_COORD_HEIGHT), 80.0f);

		// back wall (hitobject spawnpoint)
		g->setColor(0xff00ff00);
		g->pushTransform();
			g->translate(0, 0, -getApproachDistance());
			m_osu->getHUD()->drawPlayfieldBorder(g, Vector2(0,0), Vector2(OsuGameRules::OSU_COORD_WIDTH, OsuGameRules::OSU_COORD_HEIGHT), 80.0f, 250.0f);
		g->popTransform();
	}
	m_shaderUntexturedLegacyGeneric->disable();

	// draw approach distance text
	m_shaderTexturedLegacyGeneric->enable();
	{
		const float textScaleMultiplier = 5.0f;

		Matrix4 textScale;
		textScale.scale(textScaleMultiplier);
		Matrix4 approachTextMVP = mvp * m_playfieldMatrix * textScale * scale;
		m_shaderTexturedLegacyGeneric->setUniformMatrix4fv("matrix", approachTextMVP);

		g->pushTransform();
			UString distanceString = UString::format(osu_vr_approach_distance.getFloat() == 1.0f ? "%.1f meter" : "%.1f meters", osu_vr_approach_distance.getFloat());
			g->translate(-m_osu->getTitleFont()->getStringWidth(distanceString)/2.0f, -m_osu->getTitleFont()->getHeight()*2.5f, -getApproachDistance()/textScaleMultiplier + 2.0f); // + 2.0f to avoid z-fighting with back playfieldborder
			g->setColor(0xffffffff);
			g->drawString(m_osu->getTitleFont(), distanceString);
		g->popTransform();
	}
	m_shaderTexturedLegacyGeneric->disable();

	// draw cursors
	drawVRCursors(g, finalMVP);
}

void OsuVR::drawVRCursors(Graphics *g, Matrix4 &mvp)
{
	// HACKHACK: temp disable until i fix it
	float prevValue = m_osu_draw_cursor_trail_ref->getFloat();
	m_osu_draw_cursor_trail_ref->setValue(0.0f);

	m_shaderTexturedLegacyGeneric->enable();
	{
		m_shaderTexturedLegacyGeneric->setUniformMatrix4fv("matrix", mvp);

		g->pushTransform();
			g->translate(0, 0, 0.8f); // to avoid z-fighting with the border and sliders (slider = 0.5, extra 0.3 for good measure, but can't go above 0.999f!)
			m_osu->getHUD()->drawCursor(g, m_vPlayfieldCursorPos1, osu_vr_cursor_alpha.getFloat());
			g->translate(0, 0, 0.9f); // to avoid z-fighting with the first cursor
			m_osu->getHUD()->drawCursor(g, m_vPlayfieldCursorPos2, osu_vr_cursor_alpha.getFloat());
		g->popTransform();
	}
	m_shaderTexturedLegacyGeneric->disable();

	m_osu_draw_cursor_trail_ref->setValue(prevValue);
}

void OsuVR::update()
{
	OpenVRController *primaryController = openvr->getController();
	OpenVRController *rightController = openvr->getRightController();
	OpenVRController *leftController = openvr->getLeftController();

	// calculate virtual screen plane and cursor pos
	float virtualScreenPlaneIntersectionDistance = 0.0f;
	{
		Vector4 topLeft = m_screenMatrix * Vector4(0, 0, 0, 1);
		Vector4 right = (m_screenMatrix * Vector4(1, 0, 1, 1));
		Vector4 down = (m_screenMatrix * Vector4(0, -1, 1, 1));
		Vector3 normal = (m_screenMatrix * Vector3(0, 0, 1)).normalize();

		// get ray intersection with screen
		virtualScreenPlaneIntersectionDistance = intersectRayPlane(primaryController->getPosition(), primaryController->getDirection(), Vector3(topLeft.x, topLeft.y, topLeft.z), normal);
		if (virtualScreenPlaneIntersectionDistance > 0.0f)
		{
			m_vScreenIntersectionPoint = primaryController->getPosition() + primaryController->getDirection()*virtualScreenPlaneIntersectionDistance;

			// calculate virtual cursor position
			Vector3 localRight = (Vector3(right.x, right.y, right.z) - Vector3(topLeft.x, topLeft.y, topLeft.z));
			Vector3 localDown = (Vector3(down.x, down.y, down.z) - Vector3(topLeft.x, topLeft.y, topLeft.z));
			Vector3 localIntersectionPoint = m_vScreenIntersectionPoint - Vector3(topLeft.x, topLeft.y, topLeft.z);

			float x = localRight.dot(localIntersectionPoint) / m_vVirtualScreenSize.x;
			float y = localDown.dot(localIntersectionPoint) / std::abs(m_vVirtualScreenSize.y); // abs because negative size due to image flipping

			m_vScreenIntersectionPoint2D.x = x*m_vVirtualScreenSize.x;
			m_vScreenIntersectionPoint2D.y = y*m_vVirtualScreenSize.y;

			Vector2 newMousePos = Vector2(x*engine->getScreenWidth(), y*engine->getScreenHeight());
			if (newMousePos.x >= 0 && newMousePos.y >= 0 && newMousePos.x < engine->getScreenWidth() && newMousePos.y < engine->getScreenHeight())
			{
				m_bDrawLaser = true;
				m_bScreenIntersection = true;
				engine->getMouse()->onPosChange(newMousePos);
			}
			else
			{
				// TODO: shitty override, but it works well enough for now => see Osu.cpp
				// if the laser has left the virtual screen, and the engine has focus, then move the cursor out of the window (so it doesn't jump back to the actual mouse pos)
				if (m_bScreenIntersection && engine->hasFocus() && env->isCursorInWindow())
					engine->getMouse()->setPos(engine->getScreenSize() + Vector2(50,50));

				m_bScreenIntersection = false;
				m_bDrawLaser = false;
			}
		}
		else
		{
			m_bScreenIntersection = false;
		}
	}

	// calculate virtual playfield plane and the two virtual cursors
	{
		Vector4 topLeft = m_playfieldMatrix * Vector4(0, 0, 0, 1);
		Vector4 right = (m_playfieldMatrix * Vector4(1, 0, 1, 1));
		Vector4 down = (m_playfieldMatrix * Vector4(0, -1, 1, 1));
		Vector3 normal = (m_playfieldMatrix * Vector3(0, 0, 1)).normalize();

		// get closest distance intersection with playfield
		float leftIntersectionDistance = intersectRayPlane(leftController->getPosition(), -normal, Vector3(topLeft.x, topLeft.y, topLeft.z), normal);
		if (leftIntersectionDistance != 0.0f)
		{
			m_vPlayfieldIntersectionPoint1 = leftController->getPosition() - normal*leftIntersectionDistance;

			// calculate virtual cursor position
			Vector3 localRight = (Vector3(right.x, right.y, right.z) - Vector3(topLeft.x, topLeft.y, topLeft.z));
			Vector3 localDown = (Vector3(down.x, down.y, down.z) - Vector3(topLeft.x, topLeft.y, topLeft.z));
			Vector3 localIntersectionPoint = m_vPlayfieldIntersectionPoint1 - Vector3(topLeft.x, topLeft.y, topLeft.z);

			float x = localRight.dot(localIntersectionPoint) / (getDrawScale()*OsuGameRules::OSU_COORD_WIDTH);
			float y = localDown.dot(localIntersectionPoint) / (getDrawScale()*OsuGameRules::OSU_COORD_HEIGHT);

			Vector2 newCursorPos = Vector2(x*OsuGameRules::OSU_COORD_WIDTH, y*OsuGameRules::OSU_COORD_HEIGHT);
			{
				m_vPlayfieldCursorPos1 = newCursorPos;
				m_fPlayfieldCursorDist1 = std::abs(leftIntersectionDistance);
				m_fPlayfieldCursorDistSigned1 = leftIntersectionDistance;
			}

			/*
			if (newCursorPos.x >= 0 && newCursorPos.y >= 0 && newCursorPos.x <= OsuGameRules::OSU_COORD_WIDTH && newCursorPos.y <= OsuGameRules::OSU_COORD_HEIGHT)
			{
			}
			*/
		}

		float rightIntersectionDistance = intersectRayPlane(rightController->getPosition(), -normal, Vector3(topLeft.x, topLeft.y, topLeft.z), normal);
		if (rightIntersectionDistance != 0.0f)
		{
			m_vPlayfieldIntersectionPoint2 = rightController->getPosition() - normal*rightIntersectionDistance;

			// calculate virtual cursor position
			Vector3 localRight = (Vector3(right.x, right.y, right.z) - Vector3(topLeft.x, topLeft.y, topLeft.z));
			Vector3 localDown = (Vector3(down.x, down.y, down.z) - Vector3(topLeft.x, topLeft.y, topLeft.z));
			Vector3 localIntersectionPoint = m_vPlayfieldIntersectionPoint2 - Vector3(topLeft.x, topLeft.y, topLeft.z);

			float x = localRight.dot(localIntersectionPoint) / (getDrawScale()*OsuGameRules::OSU_COORD_WIDTH);
			float y = localDown.dot(localIntersectionPoint) / (getDrawScale()*OsuGameRules::OSU_COORD_HEIGHT);

			Vector2 newCursorPos = Vector2(x*OsuGameRules::OSU_COORD_WIDTH, y*OsuGameRules::OSU_COORD_HEIGHT);
			{
				m_vPlayfieldCursorPos2 = newCursorPos;
				m_fPlayfieldCursorDist2 = std::abs(rightIntersectionDistance);
				m_fPlayfieldCursorDistSigned2 = rightIntersectionDistance;
			}

			/*
			if (newCursorPos.x >= 0 && newCursorPos.y >= 0 && newCursorPos.x <= OsuGameRules::OSU_COORD_WIDTH && newCursorPos.y <= OsuGameRules::OSU_COORD_HEIGHT)
			{
			}
			*/
		}

		// get ray intersection with playfield (this is only used for grip button movement logic atm)
		leftIntersectionDistance = intersectRayPlane(leftController->getPosition(), leftController->getDirection(), Vector3(topLeft.x, topLeft.y, topLeft.z), normal);
		if (leftIntersectionDistance != 0.0f)
		{
			Vector3 intersectionPoint = leftController->getPosition() + leftController->getDirection()*leftIntersectionDistance;

			// calculate virtual cursor position
			Vector3 localRight = (Vector3(right.x, right.y, right.z) - Vector3(topLeft.x, topLeft.y, topLeft.z));
			Vector3 localDown = (Vector3(down.x, down.y, down.z) - Vector3(topLeft.x, topLeft.y, topLeft.z));
			Vector3 localIntersectionPoint = intersectionPoint - Vector3(topLeft.x, topLeft.y, topLeft.z);

			float x = localRight.dot(localIntersectionPoint) / (getDrawScale()*OsuGameRules::OSU_COORD_WIDTH);
			float y = localDown.dot(localIntersectionPoint) / (getDrawScale()*OsuGameRules::OSU_COORD_HEIGHT);

			Vector2 newCursorPos = Vector2(x*OsuGameRules::OSU_COORD_WIDTH, y*OsuGameRules::OSU_COORD_HEIGHT);

			if (newCursorPos.x >= -OsuGameRules::OSU_COORD_WIDTH/2.0f && newCursorPos.y >= -OsuGameRules::OSU_COORD_HEIGHT/2.0f && newCursorPos.x <= OsuGameRules::OSU_COORD_WIDTH/2.0f && newCursorPos.y <= OsuGameRules::OSU_COORD_HEIGHT/2.0f)
				m_bPlayfieldIntersection1 = true;
			else
				m_bPlayfieldIntersection1 = false;
		}

		rightIntersectionDistance = intersectRayPlane(rightController->getPosition(), rightController->getDirection(), Vector3(topLeft.x, topLeft.y, topLeft.z), normal);
		if (rightIntersectionDistance != 0.0f)
		{
			Vector3 intersectionPoint = rightController->getPosition() + rightController->getDirection()*rightIntersectionDistance;

			// calculate virtual cursor position
			Vector3 localRight = (Vector3(right.x, right.y, right.z) - Vector3(topLeft.x, topLeft.y, topLeft.z));
			Vector3 localDown = (Vector3(down.x, down.y, down.z) - Vector3(topLeft.x, topLeft.y, topLeft.z));
			Vector3 localIntersectionPoint = intersectionPoint - Vector3(topLeft.x, topLeft.y, topLeft.z);

			float x = localRight.dot(localIntersectionPoint) / (getDrawScale()*OsuGameRules::OSU_COORD_WIDTH);
			float y = localDown.dot(localIntersectionPoint) / (getDrawScale()*OsuGameRules::OSU_COORD_HEIGHT);

			Vector2 newCursorPos = Vector2(x*OsuGameRules::OSU_COORD_WIDTH, y*OsuGameRules::OSU_COORD_HEIGHT);

			if (newCursorPos.x >= -OsuGameRules::OSU_COORD_WIDTH/2.0f && newCursorPos.y >= -OsuGameRules::OSU_COORD_HEIGHT/2.0f && newCursorPos.x <= OsuGameRules::OSU_COORD_WIDTH/2.0f && newCursorPos.y <= OsuGameRules::OSU_COORD_HEIGHT/2.0f)
				m_bPlayfieldIntersection2 = true;
			else
				m_bPlayfieldIntersection2 = false;
		}
	}

	if (m_osu->isNotInPlayModeOrPaused()) // only allow moving while not actively playing
	{
		// move virtual screen while grip button is pressed on right controller, anchor at controller
		if (rightController->isButtonPressed(OpenVRController::BUTTON::BUTTON_GRIP))
		{
			if (m_bMovingScreenAllow)
			{
				if (m_bMovingScreenCheck)
				{
					m_bMovingScreenCheck = false;

					// determine which screen to move, allow direct pointing to a screen to override it
					m_bIsRightControllerMovingScreen = (!m_bPlayfieldIntersection2 || m_bScreenIntersection) && primaryController == rightController;

					Matrix4 controllerMatrixPose = rightController->getMatrixPose();
					if (m_bIsRightControllerMovingScreen)
						m_screenMatrixBackup = controllerMatrixPose.invert() * m_screenMatrix; // relative base
					else
						m_playfieldMatrixBackup = controllerMatrixPose.invert() * m_playfieldMatrix; // relative base
				}

				if (!m_bMovingScreenCheck)
				{
					if (m_bIsRightControllerMovingScreen)
						m_screenMatrix = rightController->getMatrixPose() * m_screenMatrixBackup;
					else
					{
						m_bMovingPlayfieldAllow = false;
						m_playfieldMatrix = rightController->getMatrixPose() * m_playfieldMatrixBackup;
					}
				}
			}
		}
		else if (!m_bMovingScreenCheck)
		{
			m_bMovingScreenCheck = true;

			// immediately save the changes
			save();
		}
		else
			m_bMovingScreenAllow = true;

		// move virtual playfield while grip button is pressed on left controller, anchor at controller
		if (leftController->isButtonPressed(OpenVRController::BUTTON::BUTTON_GRIP))
		{
			if (m_bMovingPlayfieldAllow)
			{
				if (m_bMovingPlayfieldCheck)
				{
					m_bMovingPlayfieldCheck = false;

					// determine which screen to move, allow direct pointing to a screen to override it
					m_bIsLeftControllerMovingScreen = m_bScreenIntersection && primaryController == leftController; // bit of a hack, but it'll work without changing the primaryController screen intersection logic

					Matrix4 controllerMatrixPose = leftController->getMatrixPose();
					if (!m_bIsLeftControllerMovingScreen)
						m_playfieldMatrixBackup = controllerMatrixPose.invert() * m_playfieldMatrix; // relative base
					else
						m_screenMatrixBackup = controllerMatrixPose.invert() * m_screenMatrix; // relative base
				}

				if (!m_bMovingPlayfieldCheck)
				{
					if (!m_bIsLeftControllerMovingScreen)
						m_playfieldMatrix = leftController->getMatrixPose() * m_playfieldMatrixBackup;
					else
					{
						m_bMovingScreenAllow = false;
						m_screenMatrix = leftController->getMatrixPose() * m_screenMatrixBackup;
					}
				}
			}
		}
		else if (!m_bMovingPlayfieldCheck)
		{
			m_bMovingPlayfieldCheck = true;

			// immediately save the changes
			save();
		}
		else
			m_bMovingPlayfieldAllow = true;
	}

	// HACKHACK: if we are not drawing the VR playfield (e.g. because the player only wants to play regularly but with a virtual screen in VR), set the virtual cursor positions to somewhere they don't interfere
	if (!osu_vr_draw_playfield.getBool())
	{
		m_vPlayfieldCursorPos1 = Vector2(0, 9999);
		m_vPlayfieldCursorPos2 = m_vPlayfieldCursorPos1;

		m_fPlayfieldCursorDist1 = 9999;
		m_fPlayfieldCursorDist2 = 0;
	}

	// handle scaling (pinching/zooming/whatever)
	if (m_osu->isNotInPlayModeOrPaused()) // only allow scaling while not actively playing
	{
		if (openvr->getLeftController()->getTrigger() > 0.95f && openvr->getRightController()->getTrigger() > 0.95f)
		{
			const float controllerDistance = (openvr->getLeftController()->getPosition() - openvr->getRightController()->getPosition()).length();

			if (m_bScaleCheck)
			{
				m_bScaleCheck = false;
				if (m_bScreenIntersection)
				{
					m_bIsPlayerScalingPlayfield = false;
					m_fScaleBackup = osu_vr_screen_scale.getFloat() - controllerDistance*osu_vr_controller_pinch_scale_multiplier.getFloat();
				}
				else
				{
					m_bIsPlayerScalingPlayfield = true;
					m_fScaleBackup = osu_vr_playfield_scale.getFloat() - controllerDistance*osu_vr_controller_pinch_scale_multiplier.getFloat();
				}
			}

			if (m_bIsPlayerScalingPlayfield)
				osu_vr_playfield_scale.setValue(m_fScaleBackup + controllerDistance*osu_vr_controller_pinch_scale_multiplier.getFloat());
			else
				osu_vr_screen_scale.setValue(m_fScaleBackup + controllerDistance*osu_vr_controller_pinch_scale_multiplier.getFloat());
		}
		else if (!m_bScaleCheck)
		{
			m_bScaleCheck = true;

			// immediately save the changes
			save();
		}
	}
	else
		m_bScaleCheck = true;

	// handle clicking (trigger + touchpad click) (scaling has priority over clicking on the usage of the trigger button)
	if (((leftController->getTrigger() > 0.95f || rightController->getTrigger() > 0.95f) && m_bScaleCheck) || leftController->isButtonPressed(OpenVRController::BUTTON::BUTTON_STEAMVR_TOUCHPAD) || rightController->isButtonPressed(OpenVRController::BUTTON::BUTTON_STEAMVR_TOUCHPAD))
	{
		if (m_bClickCheck)
		{
			m_bClickCheck = false;
			engine->onMouseLeftChange(true);
		}
	}
	else
	{
		if (!m_bClickCheck)
		{
			m_bClickCheck = true;
			engine->onMouseLeftChange(false);
		}
	}

	// handle menu button (escape)
	if ((leftController->isButtonPressed(OpenVRController::BUTTON::BUTTON_APPLICATIONMENU) || rightController->isButtonPressed(OpenVRController::BUTTON::BUTTON_APPLICATIONMENU)))
	{
		if (m_bMenuCheck)
		{
			m_bMenuCheck = false;
			engine->onKeyboardKeyDown(KEY_ESCAPE);
		}
	}
	else
	{
		if (!m_bMenuCheck)
		{
			m_bMenuCheck = true;
			engine->onKeyboardKeyUp(KEY_ESCAPE);
		}
	}

	// controller distance warning by color (if reaching too far behind/into the playfield)
	float smallestDist = getCursorDistSigned1() < getCursorDistSigned2() ? getCursorDistSigned1() : getCursorDistSigned2();
	if (osu_vr_controller_warning_distance_enabled.getBool() && smallestDist < osu_vr_controller_warning_distance_start.getFloat())
	{
		float interpStart = osu_vr_controller_warning_distance_start.getFloat();
		float interpEnd = osu_vr_controller_warning_distance_end.getFloat();

		float percent = 1.0f - ((interpEnd - smallestDist) / (interpEnd - interpStart));
		openvr->setControllerColorOverride(COLORf(1.0f, percent, 0.0f, 0.0f));
	}
	else
		openvr->setControllerColorOverride(0xff000000);

	// update all VR UI elements
	updateLayout();
	float smallestBoundsY = std::numeric_limits<float>::min();
	float largestBoundsX = 0.0f;
	float largestBoundsY = std::numeric_limits<float>::max();
	m_bIsUIActive = false;
	for (int i=0; i<m_uiElements.size(); i++)
	{
		m_uiElements[i]->update(m_vScreenIntersectionPoint2D);

		m_bIsUIActive = m_bIsUIActive || m_uiElements[i]->isActive() || m_uiElements[i]->isCursorInside();

		float boundsX = m_uiElements[i]->getPos().x + m_uiElements[i]->getSize().x;
		if (boundsX > largestBoundsX)
			largestBoundsX = boundsX;

		float boundsY = m_uiElements[i]->getPos().y - m_uiElements[i]->getSize().y;
		if (boundsY < largestBoundsY)
			largestBoundsY = boundsY;

		float minY = m_uiElements[i]->getPos().y;
		if (minY > smallestBoundsY)
			smallestBoundsY = minY;

		// override laser
		if (m_uiElements[i]->isCursorInside())
			m_bDrawLaser = true;
	}

	// override laser for UI element block on the right
	if (m_vScreenIntersectionPoint2D.x > 0 && m_vScreenIntersectionPoint2D.x < largestBoundsX && m_vScreenIntersectionPoint2D.y < 0 && m_vScreenIntersectionPoint2D.y > m_vVirtualScreenSize.y)
		m_bDrawLaser = true;
	// and the block on top
	if (m_vScreenIntersectionPoint2D.x > 0 && m_vScreenIntersectionPoint2D.x < largestBoundsX && m_vScreenIntersectionPoint2D.y < smallestBoundsY && m_vScreenIntersectionPoint2D.y > m_vVirtualScreenSize.y)
		m_bDrawLaser = true;
	// and the block on the bottom
	if (m_vScreenIntersectionPoint2D.x > 0 && m_vScreenIntersectionPoint2D.x < largestBoundsX && m_vScreenIntersectionPoint2D.y < 0 && m_vScreenIntersectionPoint2D.y > largestBoundsY)
		m_bDrawLaser = true;
	if (!openvr->hasInputFocus()) // quickfix for 1 frame laser flicker between keyboard overlay switch
		m_bDrawLaser = false;

	// handle UI element visibility depending on game state
	m_keyboardButton->setVisible(!m_osu->isInPlayMode());
	m_matrixResetButton->setVisible(!m_osu->isInPlayMode());
	m_offsetDownButton->setVisible(m_osu->isInPlayMode());
	m_offsetUpButton->setVisible(m_osu->isInPlayMode());
	m_scrubbingSlider->setVisible(m_osu->isInPlayMode());

	// force update to current volume
	m_volumeSlider->setValue(m_osu_volume_master_ref->getFloat(), true);

	// force update to current songPos
	if (m_osu->isInPlayMode() && m_osu->getSelectedBeatmap() != NULL)
		m_scrubbingSlider->setValue(m_osu->getSelectedBeatmap()->getPercentFinishedPlayable(), true);
}

float OsuVR::intersectRayPlane(Vector3 rayOrigin, Vector3 rayDir, Vector3 planeOrigin, Vector3 planeNormal)
{
	float d = -(planeNormal.dot(planeOrigin));
	float numer = planeNormal.dot(rayOrigin) + d;
	float denom = planeNormal.dot(rayDir);

	if (denom == 0)  // normal is orthogonal to vector, can't intersect
		return 0.0f;

	return -(numer / denom);
}

float OsuVR::getCircleHitboxScale()
{
	return osu_vr_circle_hitbox_scale.getFloat();
}

float OsuVR::getApproachDistance()
{
	return osu_vr_approach_distance.getFloat() / getDrawScale(); // keep approach distance consistent in meters, even if the playfield scale changes
}

unsigned short OsuVR::getHapticPulseStrength()
{
	return (unsigned short)(osu_vr_controller_vibration_strength.getFloat() * 3999.0f);
}

unsigned short OsuVR::getSliderHapticPulseStrength()
{
	return (unsigned short)(osu_vr_slider_controller_vibration_strength.getFloat() * 3999.0f);
}

void OsuVR::updateLayout()
{
	m_keyboardButton->setPosX(m_vVirtualScreenSize.x + osu_vr_ui_offset.getFloat());
	m_matrixResetButton->setPos(m_vVirtualScreenSize.x + osu_vr_ui_offset.getFloat(), (-m_vVirtualScreenSize.y - m_matrixResetButton->getSize().y < m_keyboardButton->getPos().y + m_keyboardButton->getSize().y ? -(m_keyboardButton->getPos().y + m_keyboardButton->getSize().y) : m_vVirtualScreenSize.y + m_matrixResetButton->getSize().y));
	m_volumeSlider->setPos(m_vVirtualScreenSize.x/2.0f - m_volumeSlider->getSize().x/2.0f, m_volumeSlider->getSize().y + osu_vr_ui_offset.getFloat());

	const float gap = 0.05f;
	m_scrubbingSlider->setPos(m_vVirtualScreenSize.x/2.0f - m_scrubbingSlider->getSize().x/2.0f, m_vVirtualScreenSize.y - osu_vr_ui_offset.getFloat());
	m_offsetDownButton->setPos(m_vVirtualScreenSize.x/2.0f - m_offsetDownButton->getSize().x - gap/2.0f, m_scrubbingSlider->getPos().y - m_scrubbingSlider->getSize().y - gap);
	m_offsetUpButton->setPos(m_vVirtualScreenSize.x/2.0f + gap/2.0f, m_scrubbingSlider->getPos().y - m_scrubbingSlider->getSize().y - gap);
}

void OsuVR::save()
{
	debugLog("Osu: Saving VR config file ...\n");

	UString userVRConfigFile = "cfg/";
	userVRConfigFile.append(OSUVR_CONFIG_FILE_NAME);
	userVRConfigFile.append(".cfg");

	// write new config file
	// thankfully this path is relative and hardcoded, and thus not susceptible to unicode characters
	std::ofstream out(userVRConfigFile.toUtf8());
	if (!out.good())
	{
		if (m_osu != NULL)
			m_osu->getNotificationOverlay()->addNotification("Error: Couldn't save VR config file", 0xffff0000);
		return;
	}

	out << osu_vr_matrix_screen.getName().toUtf8() << " ";
	for (int i=0; i<16; i++)
	{
		if (i != 15)
			out << m_screenMatrix[i] << " ";
		else
			out << m_screenMatrix[i];
	}
	out << "\n";

	out << osu_vr_matrix_playfield.getName().toUtf8() << " ";
	for (int i=0; i<16; i++)
	{
		if (i != 15)
			out << m_playfieldMatrix[i] << " ";
		else
			out << m_playfieldMatrix[i];
	}
	out << "\n";

	out << osu_vr_screen_scale.getName().toUtf8() << " " << osu_vr_screen_scale.getString().toUtf8();
	out << "\n";
	out << osu_vr_playfield_scale.getName().toUtf8() << " " << osu_vr_playfield_scale.getString().toUtf8();
	out << "\n";

	out.close();
}

void OsuVR::resetMatrices()
{
	Matrix4 screenRotation;
	screenRotation.rotate(-90.0f, 0, 1, 0);
	Matrix4 screenTranslation;
	screenTranslation.translate(-openvr->getPlayAreaSize().y*0.4f, 2.0f, -openvr->getPlayAreaSize().x/2.0f);
	m_screenMatrix = screenRotation * screenTranslation;
	m_playfieldMatrix.identity();
	m_playfieldMatrix.translate(0, 1.5f, -(openvr->getPlayAreaSize().y/2.0f)*0.5f);

	osu_vr_screen_scale.setValue(m_fDefaultScreenScale);
	osu_vr_playfield_scale.setValue(m_fDefaultPlayfieldScale);
}

float OsuVR::getDrawScale()
{
	return osu_vr_playfield_native_draw_scale.getFloat() * osu_vr_playfield_scale.getFloat();
}

void OsuVR::onScreenMatrixChange(UString oldValue, UString newValue)
{
	std::vector<UString> tokens = newValue.split(" ");
	if (tokens.size() == 16)
	{
		float newMatrix[16];
		for (int i=0; i<16; i++)
		{
			newMatrix[i] = tokens[i].toFloat();
		}
		m_screenMatrix.set(newMatrix);
	}
}

void OsuVR::onPlayfieldMatrixChange(UString oldValue, UString newValue)
{
	std::vector<UString> tokens = newValue.split(" ");
	if (tokens.size() == 16)
	{
		float newMatrix[16];
		for (int i=0; i<16; i++)
		{
			newMatrix[i] = tokens[i].toFloat();
		}
		m_playfieldMatrix.set(newMatrix);
	}
}

void OsuVR::onMatrixResetClicked()
{
	resetMatrices();
	save();
}

void OsuVR::onKeyboardButtonClicked()
{
	openvr->getController()->triggerHapticPulse(2500);
	engine->getSound()->play(m_osu->getSkin()->getCheckOn());

	openvr->showKeyboard();
}

void OsuVR::onOffsetUpClicked()
{
	openvr->getController()->triggerHapticPulse(2500);
	engine->getSound()->play(m_osu->getSkin()->getCheckOn());

	engine->onKeyboardKeyDown((KEYCODE)OsuKeyBindings::INCREASE_LOCAL_OFFSET.getInt());
	engine->onKeyboardKeyUp((KEYCODE)OsuKeyBindings::INCREASE_LOCAL_OFFSET.getInt());
}

void OsuVR::onOffsetDownClicked()
{
	openvr->getController()->triggerHapticPulse(2500);
	engine->getSound()->play(m_osu->getSkin()->getCheckOff());

	engine->onKeyboardKeyDown((KEYCODE)OsuKeyBindings::DECREASE_LOCAL_OFFSET.getInt());
	engine->onKeyboardKeyUp((KEYCODE)OsuKeyBindings::DECREASE_LOCAL_OFFSET.getInt());
}

void OsuVR::onVolumeSliderChange(OsuVRUISlider *slider)
{
	m_osu_volume_master_ref->setValue(slider->getFloat());
}

void OsuVR::onScrubbingSliderChange(OsuVRUISlider *slider)
{
	if (m_osu->isInPlayMode() && m_osu->getSelectedBeatmap() != NULL)
		m_osu->getSelectedBeatmap()->seekPercentPlayable(slider->getFloat());
}
