//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		text bar overlay which can eat inputs (also used for key bindings)
//
// $NoKeywords: $osunot
//===============================================================================//

#ifndef OSUNOTIFICATIONOVERLAY_H
#define OSUNOTIFICATIONOVERLAY_H

#include "OsuScreen.h"
#include "KeyboardEvent.h"

class Osu;

class OsuNotificationOverlayKeyListener
{
public:
	virtual ~OsuNotificationOverlayKeyListener() {;}
	virtual void onKey(KeyboardEvent &e) = 0;
};

class OsuNotificationOverlay : public OsuScreen
{
public:
	OsuNotificationOverlay(Osu *osu);
	virtual ~OsuNotificationOverlay() {;}

	virtual void draw(Graphics *g);

	virtual void onKeyDown(KeyboardEvent &e);
	virtual void onKeyUp(KeyboardEvent &e);
	virtual void onChar(KeyboardEvent &e);

	void addNotification(UString text, Color textColor = 0xffffffff, bool waitForKey = false, float duration = -1.0f);
	void setDisallowWaitForKeyLeftClick(bool disallowWaitForKeyLeftClick) {m_bWaitForKeyDisallowsLeftClick = disallowWaitForKeyLeftClick;}

	void stopWaitingForKey(bool stillConsumeNextChar = false);

	void addKeyListener(OsuNotificationOverlayKeyListener *keyListener) {m_keyListener = keyListener;}

	virtual bool isVisible();

	inline bool isWaitingForKey() {return m_bWaitForKey || m_bConsumeNextChar;}

private:
	struct NOTIFICATION
	{
		UString text;
		Color textColor;

		float time;
		float alpha;
		float backgroundAnim;
		float fallAnim;

		NOTIFICATION()
		{
			textColor = 0xffffffff;

			time = 0.0f;
			alpha = 0.0f;
			backgroundAnim = 0.0f;
			fallAnim = 0.0f;
		}
	};

	void drawNotificationText(Graphics *g, NOTIFICATION &n);
	void drawNotificationBackground(Graphics *g, NOTIFICATION &n);

	NOTIFICATION m_notification1;
	NOTIFICATION m_notification2;

	bool m_bWaitForKey;
	bool m_bWaitForKeyDisallowsLeftClick;
	bool m_bConsumeNextChar;
	OsuNotificationOverlayKeyListener *m_keyListener;
};

#endif
