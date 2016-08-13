//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		difficulty & playfield behaviour
//
// $NoKeywords: $osugr
//===============================================================================//

#include "OsuGameRules.h"

ConVar OsuGameRules::osu_playfield_border_top_percent("osu_playfield_border_top_percent", 0.117f);
ConVar OsuGameRules::osu_playfield_border_bottom_percent("osu_playfield_border_bottom_percent", 0.0834f);

ConVar OsuGameRules::osu_circle_fade_out_time("osu_circle_fade_out_time", 0.3f);
ConVar OsuGameRules::osu_circle_fade_out_scale("osu_circle_fade_out_scale", 0.4f);

ConVar OsuGameRules::osu_slider_followcircle_fadein_fade_time("osu_slider_followcircle_fadein_fade_time", 0.06f);
ConVar OsuGameRules::osu_slider_followcircle_fadeout_fade_time("osu_slider_followcircle_fadeout_fade_time", 0.2f);
ConVar OsuGameRules::osu_slider_followcircle_fadein_scale("osu_slider_followcircle_fadein_scale", 0.5f);
ConVar OsuGameRules::osu_slider_followcircle_fadein_scale_time("osu_slider_followcircle_fadein_scale_time", 0.18f);
ConVar OsuGameRules::osu_slider_followcircle_fadeout_scale("osu_slider_followcircle_fadeout_scale", 0.8f);
ConVar OsuGameRules::osu_slider_followcircle_fadeout_scale_time("osu_slider_followcircle_fadeout_scale_time", 0.2f);
ConVar OsuGameRules::osu_slider_followcircle_tick_pulse_time("osu_slider_followcircle_tick_pulse_time", 0.2f);
ConVar OsuGameRules::osu_slider_followcircle_tick_pulse_scale("osu_slider_followcircle_tick_pulse_scale", 0.1f);

ConVar OsuGameRules::osu_mod_ming3012("osu_mod_ming3012", false);
ConVar OsuGameRules::osu_mod_millhioref("osu_mod_millhioref", false);
ConVar OsuGameRules::osu_mod_millhioref_multiplier("osu_mod_millhioref_multiplier", 2.0f);
