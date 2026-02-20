//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		hud element drawing functions (accuracy, combo, score, etc.)
//
// $NoKeywords: $osuhud
//===============================================================================//

#include "OsuHUD.h"

#include "Engine.h"
#include "Environment.h"
#include "ConVar.h"
#include "Mouse.h"
#include "NetworkHandler.h"
#include "ResourceManager.h"
#include "AnimationHandler.h"
#include "VertexArrayObject.h"
#include "Shader.h"

#include "CBaseUIContainer.h"

#include "Osu.h"
#include "OsuVR.h"
#include "OsuMultiplayer.h"
#include "OsuModFPoSu.h"
#include "OsuSkin.h"
#include "OsuSkinImage.h"

#include "OsuBeatmap.h"
#include "OsuDatabaseBeatmap.h"
#include "OsuBeatmapStandard.h"
#include "OsuBeatmapMania.h"

#include "OsuGameRules.h"
#include "OsuGameRulesMania.h"

#include "OsuScore.h"
#include "OsuSongBrowser2.h"
#include "OsuDatabase.h"

#include "OsuHitObject.h"
#include "OsuCircle.h"

#include "OsuUIVolumeSlider.h"

#include "DirectX11Interface.h"
#include "OpenGLES2Interface.h"

ConVar osu_automatic_cursor_size("osu_automatic_cursor_size", false, FCVAR_NONE);

ConVar osu_cursor_alpha("osu_cursor_alpha", 1.0f, FCVAR_NONE);
ConVar osu_cursor_scale("osu_cursor_scale", 1.5f, FCVAR_NONE);
ConVar osu_cursor_expand_scale_multiplier("osu_cursor_expand_scale_multiplier", 1.3f, FCVAR_NONE);
ConVar osu_cursor_expand_duration("osu_cursor_expand_duration", 0.1f, FCVAR_NONE);
ConVar osu_cursor_trail_scale("osu_cursor_trail_scale", 1.0f, FCVAR_NONE);
ConVar osu_cursor_trail_length("osu_cursor_trail_length", 0.17f, FCVAR_NONE, "how long unsmooth cursortrails should be, in seconds");
ConVar osu_cursor_trail_spacing("osu_cursor_trail_spacing", 0.015f, FCVAR_NONE, "how big the gap between consecutive unsmooth cursortrail images should be, in seconds");
ConVar osu_cursor_trail_alpha("osu_cursor_trail_alpha", 1.0f, FCVAR_NONE);
ConVar osu_cursor_trail_smooth_force("osu_cursor_trail_smooth_force", false, FCVAR_NONE);
ConVar osu_cursor_trail_smooth_length("osu_cursor_trail_smooth_length", 0.5f, FCVAR_NONE, "how long smooth cursortrails should be, in seconds");
ConVar osu_cursor_trail_smooth_div("osu_cursor_trail_smooth_div", 4.0f, FCVAR_NONE, "divide the cursortrail.png image size by this much, for determining the distance to the next trail image");
ConVar osu_cursor_trail_max_size("osu_cursor_trail_max_size", 2048, FCVAR_NONE, "maximum number of rendered trail images, array size limit");
ConVar osu_cursor_trail_expand("osu_cursor_trail_expand", true, FCVAR_NONE, "if \"CursorExpand: 1\" in your skin.ini, whether the trail should then also expand or not");
ConVar osu_cursor_ripple_duration("osu_cursor_ripple_duration", 0.7f, FCVAR_NONE, "time in seconds each cursor ripple is visible");
ConVar osu_cursor_ripple_alpha("osu_cursor_ripple_alpha", 1.0f, FCVAR_NONE);
ConVar osu_cursor_ripple_additive("osu_cursor_ripple_additive", true, FCVAR_NONE, "use additive blending");
ConVar osu_cursor_ripple_anim_start_scale("osu_cursor_ripple_anim_start_scale", 0.05f, FCVAR_NONE, "start size multiplier");
ConVar osu_cursor_ripple_anim_end_scale("osu_cursor_ripple_anim_end_scale", 0.5f, FCVAR_NONE, "end size multiplier");
ConVar osu_cursor_ripple_anim_start_fadeout_delay("osu_cursor_ripple_anim_start_fadeout_delay", 0.0f, FCVAR_NONE, "delay in seconds after which to start fading out (limited by osu_cursor_ripple_duration of course)");
ConVar osu_cursor_ripple_tint_r("osu_cursor_ripple_tint_r", 255, FCVAR_NONE, "from 0 to 255");
ConVar osu_cursor_ripple_tint_g("osu_cursor_ripple_tint_g", 255, FCVAR_NONE, "from 0 to 255");
ConVar osu_cursor_ripple_tint_b("osu_cursor_ripple_tint_b", 255, FCVAR_NONE, "from 0 to 255");

ConVar osu_hud_shift_tab_toggles_everything("osu_hud_shift_tab_toggles_everything", true, FCVAR_NONE);
ConVar osu_hud_scale("osu_hud_scale", 1.0f, FCVAR_NONE);
ConVar osu_hud_hiterrorbar_alpha("osu_hud_hiterrorbar_alpha", 1.0f, FCVAR_NONE, "opacity multiplier for entire hiterrorbar");
ConVar osu_hud_hiterrorbar_bar_alpha("osu_hud_hiterrorbar_bar_alpha", 1.0f, FCVAR_NONE, "opacity multiplier for background color bar");
ConVar osu_hud_hiterrorbar_centerline_alpha("osu_hud_hiterrorbar_centerline_alpha", 1.0f, FCVAR_NONE, "opacity multiplier for center line");
ConVar osu_hud_hiterrorbar_entry_additive("osu_hud_hiterrorbar_entry_additive", true, FCVAR_NONE, "whether to use additive blending for all hit error entries/lines");
ConVar osu_hud_hiterrorbar_entry_alpha("osu_hud_hiterrorbar_entry_alpha", 0.75f, FCVAR_NONE, "opacity multiplier for all hit error entries/lines");
ConVar osu_hud_hiterrorbar_entry_300_r("osu_hud_hiterrorbar_entry_300_r", 50, FCVAR_NONE);
ConVar osu_hud_hiterrorbar_entry_300_g("osu_hud_hiterrorbar_entry_300_g", 188, FCVAR_NONE);
ConVar osu_hud_hiterrorbar_entry_300_b("osu_hud_hiterrorbar_entry_300_b", 231, FCVAR_NONE);
ConVar osu_hud_hiterrorbar_entry_100_r("osu_hud_hiterrorbar_entry_100_r", 87, FCVAR_NONE);
ConVar osu_hud_hiterrorbar_entry_100_g("osu_hud_hiterrorbar_entry_100_g", 227, FCVAR_NONE);
ConVar osu_hud_hiterrorbar_entry_100_b("osu_hud_hiterrorbar_entry_100_b", 19, FCVAR_NONE);
ConVar osu_hud_hiterrorbar_entry_50_r("osu_hud_hiterrorbar_entry_50_r", 218, FCVAR_NONE);
ConVar osu_hud_hiterrorbar_entry_50_g("osu_hud_hiterrorbar_entry_50_g", 174, FCVAR_NONE);
ConVar osu_hud_hiterrorbar_entry_50_b("osu_hud_hiterrorbar_entry_50_b", 70, FCVAR_NONE);
ConVar osu_hud_hiterrorbar_entry_miss_r("osu_hud_hiterrorbar_entry_miss_r", 205, FCVAR_NONE);
ConVar osu_hud_hiterrorbar_entry_miss_g("osu_hud_hiterrorbar_entry_miss_g", 0, FCVAR_NONE);
ConVar osu_hud_hiterrorbar_entry_miss_b("osu_hud_hiterrorbar_entry_miss_b", 0, FCVAR_NONE);
ConVar osu_hud_hiterrorbar_centerline_r("osu_hud_hiterrorbar_centerline_r", 255, FCVAR_NONE);
ConVar osu_hud_hiterrorbar_centerline_g("osu_hud_hiterrorbar_centerline_g", 255, FCVAR_NONE);
ConVar osu_hud_hiterrorbar_centerline_b("osu_hud_hiterrorbar_centerline_b", 255, FCVAR_NONE);
ConVar osu_hud_hiterrorbar_entry_hit_fade_time("osu_hud_hiterrorbar_entry_hit_fade_time", 6.0f, FCVAR_NONE, "fade duration of 50/100/300 hit entries/lines in seconds");
ConVar osu_hud_hiterrorbar_entry_miss_fade_time("osu_hud_hiterrorbar_entry_miss_fade_time", 4.0f, FCVAR_NONE, "fade duration of miss entries/lines in seconds");
ConVar osu_hud_hiterrorbar_entry_miss_height_multiplier("osu_hud_hiterrorbar_entry_miss_height_multiplier", 1.5f, FCVAR_NONE);
ConVar osu_hud_hiterrorbar_entry_misaim_height_multiplier("osu_hud_hiterrorbar_entry_misaim_height_multiplier", 4.0f, FCVAR_NONE);
ConVar osu_hud_hiterrorbar_scale("osu_hud_hiterrorbar_scale", 1.0f, FCVAR_NONE);
ConVar osu_hud_hiterrorbar_showmisswindow("osu_hud_hiterrorbar_showmisswindow", false, FCVAR_NONE);
ConVar osu_hud_hiterrorbar_width_percent_with_misswindow("osu_hud_hiterrorbar_width_percent_with_misswindow", 0.4f, FCVAR_NONE);
ConVar osu_hud_hiterrorbar_width_percent("osu_hud_hiterrorbar_width_percent", 0.15f, FCVAR_NONE);
ConVar osu_hud_hiterrorbar_height_percent("osu_hud_hiterrorbar_height_percent", 0.007f, FCVAR_NONE);
ConVar osu_hud_hiterrorbar_offset_percent("osu_hud_hiterrorbar_offset_percent", 0.0f, FCVAR_NONE);
ConVar osu_hud_hiterrorbar_offset_bottom_percent("osu_hud_hiterrorbar_offset_bottom_percent", 0.0f, FCVAR_NONE);
ConVar osu_hud_hiterrorbar_offset_top_percent("osu_hud_hiterrorbar_offset_top_percent", 0.0f, FCVAR_NONE);
ConVar osu_hud_hiterrorbar_offset_left_percent("osu_hud_hiterrorbar_offset_left_percent", 0.0f, FCVAR_NONE);
ConVar osu_hud_hiterrorbar_offset_right_percent("osu_hud_hiterrorbar_offset_right_percent", 0.0f, FCVAR_NONE);
ConVar osu_hud_hiterrorbar_bar_width_scale("osu_hud_hiterrorbar_bar_width_scale", 0.6f, FCVAR_NONE);
ConVar osu_hud_hiterrorbar_bar_height_scale("osu_hud_hiterrorbar_bar_height_scale", 3.4f, FCVAR_NONE);
ConVar osu_hud_hiterrorbar_max_entries("osu_hud_hiterrorbar_max_entries", 32, FCVAR_NONE, "maximum number of entries/lines");
ConVar osu_hud_hiterrorbar_hide_during_spinner("osu_hud_hiterrorbar_hide_during_spinner", true, FCVAR_NONE);
ConVar osu_hud_hiterrorbar_ur_scale("osu_hud_hiterrorbar_ur_scale", 1.0f, FCVAR_NONE);
ConVar osu_hud_hiterrorbar_ur_alpha("osu_hud_hiterrorbar_ur_alpha", 0.5f, FCVAR_NONE, "opacity multiplier for unstable rate text above hiterrorbar");
ConVar osu_hud_hiterrorbar_ur_offset_x_percent("osu_hud_hiterrorbar_ur_offset_x_percent", 0.0f, FCVAR_NONE);
ConVar osu_hud_hiterrorbar_ur_offset_y_percent("osu_hud_hiterrorbar_ur_offset_y_percent", 0.0f, FCVAR_NONE);
ConVar osu_hud_scorebar_scale("osu_hud_scorebar_scale", 1.0f, FCVAR_NONE);
ConVar osu_hud_scorebar_hide_during_breaks("osu_hud_scorebar_hide_during_breaks", true, FCVAR_NONE);
ConVar osu_hud_scorebar_hide_anim_duration("osu_hud_scorebar_hide_anim_duration", 0.5f, FCVAR_NONE);
ConVar osu_hud_combo_scale("osu_hud_combo_scale", 1.0f, FCVAR_NONE);
ConVar osu_hud_score_scale("osu_hud_score_scale", 1.0f, FCVAR_NONE);
ConVar osu_hud_accuracy_scale("osu_hud_accuracy_scale", 1.0f, FCVAR_NONE);
ConVar osu_hud_progressbar_scale("osu_hud_progressbar_scale", 1.0f, FCVAR_NONE);
ConVar osu_hud_playfield_border_size("osu_hud_playfield_border_size", 5.0f, FCVAR_NONE);
ConVar osu_hud_statistics_scale("osu_hud_statistics_scale", 1.0f, FCVAR_NONE);
ConVar osu_hud_statistics_spacing_scale("osu_hud_statistics_spacing_scale", 1.1f, FCVAR_NONE);
ConVar osu_hud_statistics_offset_x("osu_hud_statistics_offset_x", 5.0f, FCVAR_NONE);
ConVar osu_hud_statistics_offset_y("osu_hud_statistics_offset_y", 50.0f, FCVAR_NONE);
ConVar osu_hud_statistics_pp_decimal_places("osu_hud_statistics_pp_decimal_places", 0, FCVAR_NONE, "number of decimal places for the live pp counter (min = 0, max = 2)");
ConVar osu_hud_statistics_pp_offset_x("osu_hud_statistics_pp_offset_x", 0.0f, FCVAR_NONE);
ConVar osu_hud_statistics_pp_offset_y("osu_hud_statistics_pp_offset_y", 0.0f, FCVAR_NONE);
ConVar osu_hud_statistics_perfectpp_offset_x("osu_hud_statistics_perfectpp_offset_x", 0.0f, FCVAR_NONE);
ConVar osu_hud_statistics_perfectpp_offset_y("osu_hud_statistics_perfectpp_offset_y", 0.0f, FCVAR_NONE);
ConVar osu_hud_statistics_misses_offset_x("osu_hud_statistics_misses_offset_x", 0.0f, FCVAR_NONE);
ConVar osu_hud_statistics_misses_offset_y("osu_hud_statistics_misses_offset_y", 0.0f, FCVAR_NONE);
ConVar osu_hud_statistics_sliderbreaks_offset_x("osu_hud_statistics_sliderbreaks_offset_x", 0.0f, FCVAR_NONE);
ConVar osu_hud_statistics_sliderbreaks_offset_y("osu_hud_statistics_sliderbreaks_offset_y", 0.0f, FCVAR_NONE);
ConVar osu_hud_statistics_maxpossiblecombo_offset_x("osu_hud_statistics_maxpossiblecombo_offset_x", 0.0f, FCVAR_NONE);
ConVar osu_hud_statistics_maxpossiblecombo_offset_y("osu_hud_statistics_maxpossiblecombo_offset_y", 0.0f, FCVAR_NONE);
ConVar osu_hud_statistics_livestars_offset_x("osu_hud_statistics_livestars_offset_x", 0.0f, FCVAR_NONE);
ConVar osu_hud_statistics_livestars_offset_y("osu_hud_statistics_livestars_offset_y", 0.0f, FCVAR_NONE);
ConVar osu_hud_statistics_totalstars_offset_x("osu_hud_statistics_totalstars_offset_x", 0.0f, FCVAR_NONE);
ConVar osu_hud_statistics_totalstars_offset_y("osu_hud_statistics_totalstars_offset_y", 0.0f, FCVAR_NONE);
ConVar osu_hud_statistics_bpm_offset_x("osu_hud_statistics_bpm_offset_x", 0.0f, FCVAR_NONE);
ConVar osu_hud_statistics_bpm_offset_y("osu_hud_statistics_bpm_offset_y", 0.0f, FCVAR_NONE);
ConVar osu_hud_statistics_ar_offset_x("osu_hud_statistics_ar_offset_x", 0.0f, FCVAR_NONE);
ConVar osu_hud_statistics_ar_offset_y("osu_hud_statistics_ar_offset_y", 0.0f, FCVAR_NONE);
ConVar osu_hud_statistics_cs_offset_x("osu_hud_statistics_cs_offset_x", 0.0f, FCVAR_NONE);
ConVar osu_hud_statistics_cs_offset_y("osu_hud_statistics_cs_offset_y", 0.0f, FCVAR_NONE);
ConVar osu_hud_statistics_od_offset_x("osu_hud_statistics_od_offset_x", 0.0f, FCVAR_NONE);
ConVar osu_hud_statistics_od_offset_y("osu_hud_statistics_od_offset_y", 0.0f, FCVAR_NONE);
ConVar osu_hud_statistics_hp_offset_x("osu_hud_statistics_hp_offset_x", 0.0f, FCVAR_NONE);
ConVar osu_hud_statistics_hp_offset_y("osu_hud_statistics_hp_offset_y", 0.0f, FCVAR_NONE);
ConVar osu_hud_statistics_hitwindow300_offset_x("osu_hud_statistics_hitwindow300_offset_x", 0.0f, FCVAR_NONE);
ConVar osu_hud_statistics_hitwindow300_offset_y("osu_hud_statistics_hitwindow300_offset_y", 0.0f, FCVAR_NONE);
ConVar osu_hud_statistics_nps_offset_x("osu_hud_statistics_nps_offset_x", 0.0f, FCVAR_NONE);
ConVar osu_hud_statistics_nps_offset_y("osu_hud_statistics_nps_offset_y", 0.0f, FCVAR_NONE);
ConVar osu_hud_statistics_nd_offset_x("osu_hud_statistics_nd_offset_x", 0.0f, FCVAR_NONE);
ConVar osu_hud_statistics_nd_offset_y("osu_hud_statistics_nd_offset_y", 0.0f, FCVAR_NONE);
ConVar osu_hud_statistics_ur_offset_x("osu_hud_statistics_ur_offset_x", 0.0f, FCVAR_NONE);
ConVar osu_hud_statistics_ur_offset_y("osu_hud_statistics_ur_offset_y", 0.0f, FCVAR_NONE);
ConVar osu_hud_statistics_hitdelta_offset_x("osu_hud_statistics_hitdelta_offset_x", 0.0f, FCVAR_NONE);
ConVar osu_hud_statistics_hitdelta_offset_y("osu_hud_statistics_hitdelta_offset_y", 0.0f, FCVAR_NONE);
ConVar osu_hud_volume_duration("osu_hud_volume_duration", 1.0f, FCVAR_NONE);
ConVar osu_hud_volume_size_multiplier("osu_hud_volume_size_multiplier", 1.5f, FCVAR_NONE);
ConVar osu_hud_scoreboard_scale("osu_hud_scoreboard_scale", 1.0f, FCVAR_NONE);
ConVar osu_hud_scoreboard_offset_y_percent("osu_hud_scoreboard_offset_y_percent", 0.11f, FCVAR_NONE);
ConVar osu_hud_scoreboard_use_menubuttonbackground("osu_hud_scoreboard_use_menubuttonbackground", true, FCVAR_NONE);
ConVar osu_hud_inputoverlay_scale("osu_hud_inputoverlay_scale", 1.0f, FCVAR_NONE);
ConVar osu_hud_inputoverlay_offset_x("osu_hud_inputoverlay_offset_x", 0.0f, FCVAR_NONE);
ConVar osu_hud_inputoverlay_offset_y("osu_hud_inputoverlay_offset_y", 0.0f, FCVAR_NONE);
ConVar osu_hud_inputoverlay_anim_scale_duration("osu_hud_inputoverlay_anim_scale_duration", 0.16f, FCVAR_NONE);
ConVar osu_hud_inputoverlay_anim_scale_multiplier("osu_hud_inputoverlay_anim_scale_multiplier", 0.8f, FCVAR_NONE);
ConVar osu_hud_inputoverlay_anim_color_duration("osu_hud_inputoverlay_anim_color_duration", 0.1f, FCVAR_NONE);
ConVar osu_hud_fps_smoothing("osu_hud_fps_smoothing", true, FCVAR_NONE);
ConVar osu_hud_scrubbing_timeline_hover_tooltip_offset_multiplier("osu_hud_scrubbing_timeline_hover_tooltip_offset_multiplier", 1.0f, FCVAR_NONE);
ConVar osu_hud_scrubbing_timeline_strains_height("osu_hud_scrubbing_timeline_strains_height", 200.0f, FCVAR_NONE);
ConVar osu_hud_scrubbing_timeline_strains_alpha("osu_hud_scrubbing_timeline_strains_alpha", 0.4f, FCVAR_NONE);
ConVar osu_hud_scrubbing_timeline_strains_aim_color_r("osu_hud_scrubbing_timeline_strains_aim_color_r", 0, FCVAR_NONE);
ConVar osu_hud_scrubbing_timeline_strains_aim_color_g("osu_hud_scrubbing_timeline_strains_aim_color_g", 255, FCVAR_NONE);
ConVar osu_hud_scrubbing_timeline_strains_aim_color_b("osu_hud_scrubbing_timeline_strains_aim_color_b", 0, FCVAR_NONE);
ConVar osu_hud_scrubbing_timeline_strains_speed_color_r("osu_hud_scrubbing_timeline_strains_speed_color_r", 255, FCVAR_NONE);
ConVar osu_hud_scrubbing_timeline_strains_speed_color_g("osu_hud_scrubbing_timeline_strains_speed_color_g", 0, FCVAR_NONE);
ConVar osu_hud_scrubbing_timeline_strains_speed_color_b("osu_hud_scrubbing_timeline_strains_speed_color_b", 0, FCVAR_NONE);

ConVar osu_draw_cursor_trail("osu_draw_cursor_trail", true, FCVAR_NONE);
ConVar osu_draw_cursor_ripples("osu_draw_cursor_ripples", false, FCVAR_NONE);
ConVar osu_draw_hud("osu_draw_hud", true, FCVAR_NONE);
ConVar osu_draw_scorebar("osu_draw_scorebar", true, FCVAR_NONE);
ConVar osu_draw_scorebarbg("osu_draw_scorebarbg", true, FCVAR_NONE);
ConVar osu_draw_hiterrorbar("osu_draw_hiterrorbar", true, FCVAR_NONE);
ConVar osu_draw_hiterrorbar_ur("osu_draw_hiterrorbar_ur", true, FCVAR_NONE);
ConVar osu_draw_hiterrorbar_bottom("osu_draw_hiterrorbar_bottom", true, FCVAR_NONE);
ConVar osu_draw_hiterrorbar_top("osu_draw_hiterrorbar_top", false, FCVAR_NONE);
ConVar osu_draw_hiterrorbar_left("osu_draw_hiterrorbar_left", false, FCVAR_NONE);
ConVar osu_draw_hiterrorbar_right("osu_draw_hiterrorbar_right", false, FCVAR_NONE);
ConVar osu_draw_progressbar("osu_draw_progressbar", true, FCVAR_NONE);
ConVar osu_draw_combo("osu_draw_combo", true, FCVAR_NONE);
ConVar osu_draw_score("osu_draw_score", true, FCVAR_NONE);
ConVar osu_draw_accuracy("osu_draw_accuracy", true, FCVAR_NONE);
ConVar osu_draw_target_heatmap("osu_draw_target_heatmap", true, FCVAR_NONE);
ConVar osu_draw_scrubbing_timeline("osu_draw_scrubbing_timeline", true, FCVAR_NONE);
ConVar osu_draw_scrubbing_timeline_breaks("osu_draw_scrubbing_timeline_breaks", true, FCVAR_NONE);
ConVar osu_draw_scrubbing_timeline_strain_graph("osu_draw_scrubbing_timeline_strain_graph", false, FCVAR_NONE);
ConVar osu_draw_continue("osu_draw_continue", true, FCVAR_NONE);
ConVar osu_draw_scoreboard("osu_draw_scoreboard", true, FCVAR_NONE);
ConVar osu_draw_inputoverlay("osu_draw_inputoverlay", true, FCVAR_NONE);

ConVar osu_draw_statistics_misses("osu_draw_statistics_misses", false, FCVAR_NONE);
ConVar osu_draw_statistics_sliderbreaks("osu_draw_statistics_sliderbreaks", false, FCVAR_NONE);
ConVar osu_draw_statistics_perfectpp("osu_draw_statistics_perfectpp", false, FCVAR_NONE);
ConVar osu_draw_statistics_maxpossiblecombo("osu_draw_statistics_maxpossiblecombo", false, FCVAR_NONE);
ConVar osu_draw_statistics_livestars("osu_draw_statistics_livestars", false, FCVAR_NONE);
ConVar osu_draw_statistics_totalstars("osu_draw_statistics_totalstars", false, FCVAR_NONE);
ConVar osu_draw_statistics_bpm("osu_draw_statistics_bpm", false, FCVAR_NONE);
ConVar osu_draw_statistics_ar("osu_draw_statistics_ar", false, FCVAR_NONE);
ConVar osu_draw_statistics_cs("osu_draw_statistics_cs", false, FCVAR_NONE);
ConVar osu_draw_statistics_od("osu_draw_statistics_od", false, FCVAR_NONE);
ConVar osu_draw_statistics_hp("osu_draw_statistics_hp", false, FCVAR_NONE);
ConVar osu_draw_statistics_nps("osu_draw_statistics_nps", false, FCVAR_NONE);
ConVar osu_draw_statistics_nd("osu_draw_statistics_nd", false, FCVAR_NONE);
ConVar osu_draw_statistics_ur("osu_draw_statistics_ur", false, FCVAR_NONE);
ConVar osu_draw_statistics_pp("osu_draw_statistics_pp", false, FCVAR_NONE);
ConVar osu_draw_statistics_hitwindow300("osu_draw_statistics_hitwindow300", false, FCVAR_NONE);
ConVar osu_draw_statistics_hitdelta("osu_draw_statistics_hitdelta", false, FCVAR_NONE);

ConVar osu_combo_anim1_duration("osu_combo_anim1_duration", 0.15f, FCVAR_NONE);
ConVar osu_combo_anim1_size("osu_combo_anim1_size", 0.15f, FCVAR_NONE);
ConVar osu_combo_anim2_duration("osu_combo_anim2_duration", 0.4f, FCVAR_NONE);
ConVar osu_combo_anim2_size("osu_combo_anim2_size", 0.5f, FCVAR_NONE);

OsuHUD::OsuHUD(Osu *osu) : OsuScreen(osu)
{
	m_osu = osu;

	// convar refs
	m_name_ref = convar->getConVarByName("name");
	m_host_timescale_ref = convar->getConVarByName("host_timescale");
	m_osu_volume_master_ref = convar->getConVarByName("osu_volume_master");
	m_osu_volume_effects_ref = convar->getConVarByName("osu_volume_effects");
	m_osu_volume_music_ref = convar->getConVarByName("osu_volume_music");
	m_osu_volume_change_interval_ref = convar->getConVarByName("osu_volume_change_interval");
	m_osu_mod_target_300_percent_ref = convar->getConVarByName("osu_mod_target_300_percent");
	m_osu_mod_target_100_percent_ref = convar->getConVarByName("osu_mod_target_100_percent");
	m_osu_mod_target_50_percent_ref = convar->getConVarByName("osu_mod_target_50_percent");
	m_osu_mod_fposu_ref = convar->getConVarByName("osu_mod_fposu");
	m_fposu_draw_scorebarbg_on_top_ref = convar->getConVarByName("fposu_draw_scorebarbg_on_top");
	m_osu_playfield_stretch_x_ref = convar->getConVarByName("osu_playfield_stretch_x");
	m_osu_playfield_stretch_y_ref = convar->getConVarByName("osu_playfield_stretch_y");
	m_osu_mp_win_condition_accuracy_ref = convar->getConVarByName("osu_mp_win_condition_accuracy");
	m_osu_background_dim_ref = convar->getConVarByName("osu_background_dim");
	m_osu_skip_intro_enabled_ref = convar->getConVarByName("osu_skip_intro_enabled");
	m_osu_skip_breaks_enabled_ref = convar->getConVarByName("osu_skip_breaks_enabled");

	// convar callbacks
	osu_hud_volume_size_multiplier.setCallback( fastdelegate::MakeDelegate(this, &OsuHUD::onVolumeOverlaySizeChange) );

	// resources
	m_tempFont = engine->getResourceManager()->getFont("FONT_DEFAULT");
	m_cursorTrailShader = engine->getResourceManager()->loadShader2("cursortrail.mcshader", "cursortrail");
	m_cursorTrail.reserve(osu_cursor_trail_max_size.getInt()*2);
	if (env->getOS() == Environment::OS::OS_HORIZON)
		m_cursorTrail2.reserve(osu_cursor_trail_max_size.getInt()*2);

	m_cursorTrailShaderVR = NULL;
	if (m_osu->isInVRMode())
	{
		m_cursorTrailShaderVR = engine->getResourceManager()->loadShader("cursortrailVR.vsh", "cursortrailVR.fsh", "cursortrailVR");
		m_cursorTrailVR1.reserve(osu_cursor_trail_max_size.getInt()*2);
		m_cursorTrailVR2.reserve(osu_cursor_trail_max_size.getInt()*2);
		m_cursorTrailSpectator1.reserve(osu_cursor_trail_max_size.getInt()*2);
		m_cursorTrailSpectator2.reserve(osu_cursor_trail_max_size.getInt()*2);
	}
	m_cursorTrailVAO = engine->getResourceManager()->createVertexArrayObject(Graphics::PRIMITIVE::PRIMITIVE_QUADS, Graphics::USAGE_TYPE::USAGE_DYNAMIC);

	m_fCurFps = 60.0f;
	m_fCurFpsSmooth = 60.0f;
	m_fFpsUpdate = 0.0f;

	m_fInputoverlayK1AnimScale = 1.0f;
	m_fInputoverlayK2AnimScale = 1.0f;
	m_fInputoverlayM1AnimScale = 1.0f;
	m_fInputoverlayM2AnimScale = 1.0f;

	m_fInputoverlayK1AnimColor = 0.0f;
	m_fInputoverlayK2AnimColor = 0.0f;
	m_fInputoverlayM1AnimColor = 0.0f;
	m_fInputoverlayM2AnimColor = 0.0f;

	m_fAccuracyXOffset = 0.0f;
	m_fAccuracyYOffset = 0.0f;
	m_fScoreHeight = 0.0f;

	m_fComboAnim1 = 0.0f;
	m_fComboAnim2 = 0.0f;

	m_fVolumeChangeTime = 0.0f;
	m_fVolumeChangeFade = 1.0f;
	m_fLastVolume = m_osu_volume_master_ref->getFloat();
	m_volumeSliderOverlayContainer = new CBaseUIContainer();

	m_volumeMaster = new OsuUIVolumeSlider(m_osu, 0, 0, 450, 75, "");
	m_volumeMaster->setType(OsuUIVolumeSlider::TYPE::MASTER);
	m_volumeMaster->setBlockSize(m_volumeMaster->getSize().y + 7, m_volumeMaster->getSize().y);
	m_volumeMaster->setAllowMouseWheel(false);
	m_volumeMaster->setAnimated(false);
	m_volumeMaster->setSelected(true);
	m_volumeSliderOverlayContainer->addBaseUIElement(m_volumeMaster);
	m_volumeEffects = new OsuUIVolumeSlider(m_osu, 0, 0, m_volumeMaster->getSize().x, m_volumeMaster->getSize().y/1.5f, "");
	m_volumeEffects->setType(OsuUIVolumeSlider::TYPE::EFFECTS);
	m_volumeEffects->setBlockSize((m_volumeMaster->getSize().y + 7)/1.5f, m_volumeMaster->getSize().y/1.5f);
	m_volumeEffects->setAllowMouseWheel(false);
	m_volumeEffects->setKeyDelta(m_osu_volume_change_interval_ref->getFloat());
	m_volumeEffects->setAnimated(false);
	m_volumeSliderOverlayContainer->addBaseUIElement(m_volumeEffects);
	m_volumeMusic = new OsuUIVolumeSlider(m_osu, 0, 0, m_volumeMaster->getSize().x, m_volumeMaster->getSize().y/1.5f, "");
	m_volumeMusic->setType(OsuUIVolumeSlider::TYPE::MUSIC);
	m_volumeMusic->setBlockSize((m_volumeMaster->getSize().y + 7)/1.5f, m_volumeMaster->getSize().y/1.5f);
	m_volumeMusic->setAllowMouseWheel(false);
	m_volumeMusic->setKeyDelta(m_osu_volume_change_interval_ref->getFloat());
	m_volumeMusic->setAnimated(false);
	m_volumeSliderOverlayContainer->addBaseUIElement(m_volumeMusic);

	onVolumeOverlaySizeChange(UString::format("%f", osu_hud_volume_size_multiplier.getFloat()), UString::format("%f", osu_hud_volume_size_multiplier.getFloat()));

	m_fCursorExpandAnim = 1.0f;

	m_fHealth = 1.0f;
	m_fScoreBarBreakAnim = 0.0f;
	m_fKiScaleAnim = 0.8f;
}

OsuHUD::~OsuHUD()
{
	SAFE_DELETE(m_volumeSliderOverlayContainer);
}

void OsuHUD::draw(Graphics *g)
{
	OsuBeatmap *beatmap = m_osu->getSelectedBeatmap();
	if (beatmap == NULL) return; // sanity check

	OsuBeatmapStandard *beatmapStd = dynamic_cast<OsuBeatmapStandard*>(beatmap);
	OsuBeatmapMania *beatmapMania = dynamic_cast<OsuBeatmapMania*>(beatmap);

	if (osu_draw_hud.getBool())
	{
		if (osu_draw_inputoverlay.getBool() && beatmapStd != NULL)
		{
			const bool isAutoClicking = (m_osu->getModAuto() || m_osu->getModRelax());
			if (!isAutoClicking)
				drawInputOverlay(g, m_osu->getScore()->getKeyCount(1), m_osu->getScore()->getKeyCount(2), m_osu->getScore()->getKeyCount(3), m_osu->getScore()->getKeyCount(4));
		}

		if (osu_draw_scoreboard.getBool())
		{
			if (m_osu->isInMultiplayer())
				drawScoreBoardMP(g);
			else if (beatmap->getSelectedDifficulty2() != NULL)
				drawScoreBoard(g, (std::string&)beatmap->getSelectedDifficulty2()->getMD5Hash(), m_osu->getScore());
		}

		if (!m_osu->isSkipScheduled() && beatmap->isInSkippableSection() && ((m_osu_skip_intro_enabled_ref->getBool() && beatmap->getHitObjectIndexForCurrentTime() < 1) || (m_osu_skip_breaks_enabled_ref->getBool() && beatmap->getHitObjectIndexForCurrentTime() > 0)))
			drawSkip(g);

		g->pushTransform();
		{
			if (m_osu->getModTarget() && osu_draw_target_heatmap.getBool() && beatmapStd != NULL)
				g->translate(0, beatmapStd->getHitcircleDiameter()*(1.0f / (osu_hud_scale.getFloat()*osu_hud_statistics_scale.getFloat())));

			const int hitObjectIndexForCurrentTime = (beatmap->getHitObjectIndexForCurrentTime() < 1 ? -1 : beatmap->getHitObjectIndexForCurrentTime());

			drawStatistics(g,
					m_osu->getScore()->getNumMisses(),
					m_osu->getScore()->getNumSliderBreaks(),
					beatmap->getMaxPossibleCombo(),
					OsuDifficultyCalculator::calculateTotalStarsFromSkills(beatmap->getDifficultyAttributesForUpToHitObjectIndex(hitObjectIndexForCurrentTime).AimDifficulty, beatmap->getDifficultyAttributesForUpToHitObjectIndex(hitObjectIndexForCurrentTime).SpeedDifficulty),
					m_osu->getSongBrowser()->getDynamicStarCalculator()->getTotalStars(),
					beatmap->getMostCommonBPM(),
					OsuGameRules::getApproachRateForSpeedMultiplier(beatmap, beatmap->getSpeedMultiplier()),
					beatmap->getCS(),
					OsuGameRules::getOverallDifficultyForSpeedMultiplier(beatmap, beatmap->getSpeedMultiplier()),
					beatmap->getHP(),
					beatmap->getNPS(),
					beatmap->getND(),
					m_osu->getScore()->getUnstableRate(),
					m_osu->getScore()->getPPv2(),
					m_osu->getSongBrowser()->getDynamicStarCalculator()->getPPv2(),
					((int)OsuGameRules::getHitWindow300(beatmap) - 0.5f) * (1.0f / m_osu->getSpeedMultiplier()), // see OsuUISongBrowserInfoLabel::update()
					m_osu->getScore()->getHitErrorAvgCustomMin(),
					m_osu->getScore()->getHitErrorAvgCustomMax());
		}
		g->popTransform();

		// NOTE: special case for FPoSu, if players manually set fposu_draw_scorebarbg_on_top to 1
		if (osu_draw_scorebarbg.getBool() && m_osu_mod_fposu_ref->getBool() && m_fposu_draw_scorebarbg_on_top_ref->getBool())
			drawScorebarBg(g, osu_hud_scorebar_hide_during_breaks.getBool() ? (1.0f - beatmap->getBreakBackgroundFadeAnim()) : 1.0f, m_fScoreBarBreakAnim);

		if (osu_draw_scorebar.getBool())
			drawHPBar(g, m_fHealth, osu_hud_scorebar_hide_during_breaks.getBool() ? (1.0f - beatmap->getBreakBackgroundFadeAnim()) : 1.0f, m_fScoreBarBreakAnim);

		// NOTE: moved to draw behind hitobjects in OsuBeatmapStandard::draw()
		if (m_osu_mod_fposu_ref->getBool())
		{
			if (osu_draw_hiterrorbar.getBool() && (beatmapStd == NULL || (!beatmapStd->isSpinnerActive() || !osu_hud_hiterrorbar_hide_during_spinner.getBool())) && !beatmap->isLoading())
			{
				if (beatmapStd != NULL)
					drawHitErrorBar(g, OsuGameRules::getHitWindow300(beatmap), OsuGameRules::getHitWindow100(beatmap), OsuGameRules::getHitWindow50(beatmap), OsuGameRules::getHitWindowMiss(beatmap), m_osu->getScore()->getUnstableRate());
				else if (beatmapMania != NULL)
					drawHitErrorBar(g, OsuGameRulesMania::getHitWindow300(beatmap), OsuGameRulesMania::getHitWindow100(beatmap), OsuGameRulesMania::getHitWindow50(beatmap), OsuGameRulesMania::getHitWindowMiss(beatmap), m_osu->getScore()->getUnstableRate());
			}
		}

		if (osu_draw_score.getBool())
			drawScore(g, m_osu->getScore()->getScore());

		if (osu_draw_combo.getBool())
			drawCombo(g, m_osu->getScore()->getCombo());

		if (osu_draw_progressbar.getBool())
			drawProgressBar(g, beatmap->getPercentFinishedPlayable(), beatmap->isWaiting());

		if (osu_draw_accuracy.getBool())
			drawAccuracy(g, m_osu->getScore()->getAccuracy()*100.0f);

		if (m_osu->getModTarget() && osu_draw_target_heatmap.getBool() && beatmapStd != NULL)
			drawTargetHeatmap(g, beatmapStd->getHitcircleDiameter());
	}
	else if (!osu_hud_shift_tab_toggles_everything.getBool())
	{
		if (osu_draw_inputoverlay.getBool() && beatmapStd != NULL)
		{
			const bool isAutoClicking = (m_osu->getModAuto() || m_osu->getModRelax());
			if (!isAutoClicking)
				drawInputOverlay(g, m_osu->getScore()->getKeyCount(1), m_osu->getScore()->getKeyCount(2), m_osu->getScore()->getKeyCount(3), m_osu->getScore()->getKeyCount(4));
		}

		// NOTE: moved to draw behind hitobjects in OsuBeatmapStandard::draw()
		if (m_osu_mod_fposu_ref->getBool())
		{
			if (osu_draw_hiterrorbar.getBool() && (beatmapStd == NULL || (!beatmapStd->isSpinnerActive() || !osu_hud_hiterrorbar_hide_during_spinner.getBool())) && !beatmap->isLoading())
			{
				if (beatmapStd != NULL)
					drawHitErrorBar(g, OsuGameRules::getHitWindow300(beatmap), OsuGameRules::getHitWindow100(beatmap), OsuGameRules::getHitWindow50(beatmap), OsuGameRules::getHitWindowMiss(beatmap), m_osu->getScore()->getUnstableRate());
				else if (beatmapMania != NULL)
					drawHitErrorBar(g, OsuGameRulesMania::getHitWindow300(beatmap), OsuGameRulesMania::getHitWindow100(beatmap), OsuGameRulesMania::getHitWindow50(beatmap), OsuGameRulesMania::getHitWindowMiss(beatmap), m_osu->getScore()->getUnstableRate());
			}
		}
	}

	if (beatmap->shouldFlashSectionPass())
		drawSectionPass(g, beatmap->shouldFlashSectionPass());
	if (beatmap->shouldFlashSectionFail())
		drawSectionFail(g, beatmap->shouldFlashSectionFail());

	if (beatmap->shouldFlashWarningArrows())
		drawWarningArrows(g, beatmapStd != NULL ? beatmapStd->getHitcircleDiameter() : 0);

	if (beatmap->isContinueScheduled() && beatmapStd != NULL && osu_draw_continue.getBool())
		drawContinue(g, beatmapStd->getContinueCursorPoint(), beatmapStd->getHitcircleDiameter());

	if (osu_draw_scrubbing_timeline.getBool() && m_osu->isSeeking())
	{
		static std::vector<BREAK> breaks;
		breaks.clear();

		if (osu_draw_scrubbing_timeline_breaks.getBool())
		{
			const unsigned long lengthPlayableMS = beatmap->getLengthPlayable();
			const unsigned long startTimePlayableMS = beatmap->getStartTimePlayable();
			const unsigned long endTimePlayableMS = startTimePlayableMS + lengthPlayableMS;

			const std::vector<OsuDatabaseBeatmap::BREAK> &beatmapBreaks = beatmap->getBreaks();

			breaks.reserve(beatmapBreaks.size());

			for (int i=0; i<beatmapBreaks.size(); i++)
			{
				const OsuDatabaseBeatmap::BREAK &bk = beatmapBreaks[i];

				// ignore breaks after last hitobject
				if (/*bk.endTime <= (int)startTimePlayableMS ||*/ bk.startTime >= (int)(startTimePlayableMS + lengthPlayableMS))
					continue;

				BREAK bk2;

				bk2.startPercent = (float)(bk.startTime) / (float)(endTimePlayableMS);
				bk2.endPercent = (float)(bk.endTime) / (float)(endTimePlayableMS);

				//debugLog("%i: s = %f, e = %f\n", i, bk2.startPercent, bk2.endPercent);

				breaks.push_back(bk2);
			}
		}

		drawScrubbingTimeline(g, beatmap->getTime(), beatmap->getLength(), beatmap->getLengthPlayable(), beatmap->getStartTimePlayable(), beatmap->getPercentFinishedPlayable(), breaks);
	}
}

void OsuHUD::update()
{
	OsuBeatmap *beatmap = m_osu->getSelectedBeatmap();

	if (beatmap != NULL)
	{
		// health anim
		const double currentHealth = beatmap->getHealth();
		const double elapsedMS = engine->getFrameTime() * 1000.0;
		const double frameAimTime = 1000.0 / 60.0;
		const double frameRatio = elapsedMS / frameAimTime;
		if (m_fHealth < currentHealth)
			m_fHealth = std::min(1.0, m_fHealth + std::abs(currentHealth - m_fHealth) / 4.0 * frameRatio);
		else if (m_fHealth > currentHealth)
			m_fHealth = std::max(0.0, m_fHealth - std::abs(m_fHealth - currentHealth) / 6.0 * frameRatio);

		if (osu_hud_scorebar_hide_during_breaks.getBool())
		{
			if (!anim->isAnimating(&m_fScoreBarBreakAnim) && !beatmap->isWaiting())
			{
				if (m_fScoreBarBreakAnim == 0.0f && beatmap->isInBreak())
					anim->moveLinear(&m_fScoreBarBreakAnim, 1.0f, osu_hud_scorebar_hide_anim_duration.getFloat(), true);
				else if (m_fScoreBarBreakAnim == 1.0f && !beatmap->isInBreak())
					anim->moveLinear(&m_fScoreBarBreakAnim, 0.0f, osu_hud_scorebar_hide_anim_duration.getFloat(), true);
			}
		}
		else
			m_fScoreBarBreakAnim = 0.0f;
	}

	// dynamic hud scaling updates
	m_fScoreHeight = m_osu->getSkin()->getScore0()->getHeight() * getScoreScale();

	// fps string update
	if (osu_hud_fps_smoothing.getBool())
	{
		const float smooth = std::pow(0.05, engine->getFrameTime());
		m_fCurFpsSmooth = smooth*m_fCurFpsSmooth + (1.0f - smooth)*(1.0f / engine->getFrameTime());
		if (engine->getTime() > m_fFpsUpdate || std::abs(m_fCurFpsSmooth-m_fCurFps) > 2.0f)
		{
			m_fFpsUpdate = engine->getTime() + 0.25f;
			m_fCurFps = m_fCurFpsSmooth;
		}
	}
	else
		m_fCurFps = (1.0f / engine->getFrameTime());

	// target heatmap cleanup
	if (m_osu->getModTarget())
	{
		if (m_targets.size() > 0 && engine->getTime() > m_targets[0].time)
			m_targets.erase(m_targets.begin());
	}

	// cursor ripples cleanup
	if (osu_draw_cursor_ripples.getBool())
	{
		if (m_cursorRipples.size() > 0 && engine->getTime() > m_cursorRipples[0].time)
			m_cursorRipples.erase(m_cursorRipples.begin());
	}

	// volume overlay
	m_volumeMaster->setEnabled(m_fVolumeChangeTime > engine->getTime());
	m_volumeEffects->setEnabled(m_volumeMaster->isEnabled());
	m_volumeMusic->setEnabled(m_volumeMaster->isEnabled());
	m_volumeSliderOverlayContainer->setSize(m_osu->getScreenSize());
	m_volumeSliderOverlayContainer->update();

	if (!m_volumeMaster->isBusy())
		m_volumeMaster->setValue(m_osu_volume_master_ref->getFloat(), false);
	else
	{
		m_osu_volume_master_ref->setValue(m_volumeMaster->getFloat());
		m_fLastVolume = m_volumeMaster->getFloat();
	}

	if (!m_volumeMusic->isBusy())
		m_volumeMusic->setValue(m_osu_volume_music_ref->getFloat(), false);
	else
		m_osu_volume_music_ref->setValue(m_volumeMusic->getFloat());

	if (!m_volumeEffects->isBusy())
		m_volumeEffects->setValue(m_osu_volume_effects_ref->getFloat(), false);
	else
		m_osu_volume_effects_ref->setValue(m_volumeEffects->getFloat());

	// force focus back to master if no longer active
	if (engine->getTime() > m_fVolumeChangeTime && !m_volumeMaster->isSelected())
	{
		m_volumeMusic->setSelected(false);
		m_volumeEffects->setSelected(false);
		m_volumeMaster->setSelected(true);
	}

	// keep overlay alive as long as the user is doing something
	// switch selection if cursor moves inside one of the sliders
	if (m_volumeSliderOverlayContainer->isBusy() || (m_volumeMaster->isEnabled() && (m_volumeMaster->isMouseInside() || m_volumeEffects->isMouseInside() || m_volumeMusic->isMouseInside())))
	{
		animateVolumeChange();

		const std::vector<CBaseUIElement*> &elements = m_volumeSliderOverlayContainer->getElements();
		for (int i=0; i<elements.size(); i++)
		{
			if (((OsuUIVolumeSlider*)elements[i])->checkWentMouseInside())
			{
				for (int c=0; c<elements.size(); c++)
				{
					if (c != i)
						((OsuUIVolumeSlider*)elements[c])->setSelected(false);
				}
				((OsuUIVolumeSlider*)elements[i])->setSelected(true);
			}
		}
	}
}

void OsuHUD::onResolutionChange(Vector2 newResolution)
{
	updateLayout();
}

void OsuHUD::updateLayout()
{
	// volume overlay
	{
		const float dpiScale = Osu::getUIScale(m_osu);
		const float sizeMultiplier = osu_hud_volume_size_multiplier.getFloat() * dpiScale;

		m_volumeMaster->setSize(300*sizeMultiplier, 50*sizeMultiplier);
		m_volumeMaster->setBlockSize(m_volumeMaster->getSize().y + 7 * dpiScale, m_volumeMaster->getSize().y);

		m_volumeEffects->setSize(m_volumeMaster->getSize().x, m_volumeMaster->getSize().y/1.5f);
		m_volumeEffects->setBlockSize((m_volumeMaster->getSize().y + 7 * dpiScale)/1.5f, m_volumeMaster->getSize().y/1.5f);

		m_volumeMusic->setSize(m_volumeMaster->getSize().x, m_volumeMaster->getSize().y/1.5f);
		m_volumeMusic->setBlockSize((m_volumeMaster->getSize().y + 7 * dpiScale)/1.5f, m_volumeMaster->getSize().y/1.5f);
	}
}

void OsuHUD::drawDummy(Graphics *g)
{
	drawPlayfieldBorder(g, OsuGameRules::getPlayfieldCenter(m_osu), OsuGameRules::getPlayfieldSize(m_osu), 0);

	if (osu_draw_scorebarbg.getBool())
		drawScorebarBg(g, 1.0f, 0.0f);

	if (osu_draw_scorebar.getBool())
		drawHPBar(g, 1.0, 1.0f, 0.0);

	if (osu_draw_inputoverlay.getBool())
		drawInputOverlay(g, 0, 0, 0, 0);

	SCORE_ENTRY scoreEntry;
	scoreEntry.name = m_name_ref->getString();
	scoreEntry.index = 0;
	scoreEntry.combo = 420;
	scoreEntry.score = 12345678;
	scoreEntry.accuracy = 1.0f;
	scoreEntry.missingBeatmap = false;
	scoreEntry.downloadingBeatmap = false;
	scoreEntry.dead = false;
	scoreEntry.highlight = true;
	if (osu_draw_scoreboard.getBool())
	{
		static std::vector<SCORE_ENTRY> scoreEntries;
		scoreEntries.clear();
		{
			scoreEntries.push_back(scoreEntry);
		}
		drawScoreBoardInt(g, scoreEntries);
	}

	drawSkip(g);

	drawStatistics(g, 0, 0, 727, 2.3f, 5.5f, 180, 9.0f, 4.0f, 8.0f, 6.0f, 4, 6, 90.0f, 123, 1234, 25, -5, 15);

	drawWarningArrows(g);

	if (osu_draw_combo.getBool())
		drawCombo(g, scoreEntry.combo);

	if (osu_draw_score.getBool())
		drawScore(g, scoreEntry.score);

	if (osu_draw_progressbar.getBool())
		drawProgressBar(g, 0.25f, false);

	if (osu_draw_accuracy.getBool())
		drawAccuracy(g, scoreEntry.accuracy*100.0f);

	if (osu_draw_hiterrorbar.getBool())
		drawHitErrorBar(g, 50, 100, 150, 400, 70);
}

void OsuHUD::drawVR(Graphics *g, Matrix4 &mvp, OsuVR *vr)
{
	OsuBeatmapStandard *beatmap = dynamic_cast<OsuBeatmapStandard*>(m_osu->getSelectedBeatmap());
	if (beatmap == NULL) return; // sanity check

	vr->getShaderTexturedLegacyGeneric()->enable();
	vr->getShaderTexturedLegacyGeneric()->setUniformMatrix4fv("matrix", mvp);
	{
		if (osu_draw_hud.getBool())
		{
			if (osu_draw_scorebarbg.getBool())
				drawScorebarBg(g, osu_hud_scorebar_hide_during_breaks.getBool() ? (1.0f - beatmap->getBreakBackgroundFadeAnim()) : 1.0f, m_fScoreBarBreakAnim);

			if (osu_draw_scorebar.getBool())
				drawHPBar(g, m_fHealth, osu_hud_scorebar_hide_during_breaks.getBool() ? (1.0f - beatmap->getBreakBackgroundFadeAnim()) : 1.0f, m_fScoreBarBreakAnim);

			if (osu_draw_scoreboard.getBool())
			{
				if (m_osu->isInMultiplayer())
					drawScoreBoardMP(g);
				else if (beatmap->getSelectedDifficulty2() != NULL)
					drawScoreBoard(g, (std::string&)beatmap->getSelectedDifficulty2()->getMD5Hash(), m_osu->getScore());
			}

			if (!m_osu->isSkipScheduled() && beatmap->isInSkippableSection() && ((m_osu_skip_intro_enabled_ref->getBool() && beatmap->getHitObjectIndexForCurrentTime() < 1) || (m_osu_skip_breaks_enabled_ref->getBool() && beatmap->getHitObjectIndexForCurrentTime() > 0)))
				drawSkip(g);

			const int hitObjectIndexForCurrentTime = (beatmap->getHitObjectIndexForCurrentTime() < 1 ? -1 : beatmap->getHitObjectIndexForCurrentTime());

			drawStatistics(g,
					m_osu->getScore()->getNumMisses(),
					m_osu->getScore()->getNumSliderBreaks(),
					beatmap->getMaxPossibleCombo(),
					OsuDifficultyCalculator::calculateTotalStarsFromSkills(beatmap->getDifficultyAttributesForUpToHitObjectIndex(hitObjectIndexForCurrentTime).AimDifficulty, beatmap->getDifficultyAttributesForUpToHitObjectIndex(hitObjectIndexForCurrentTime).SpeedDifficulty),
					m_osu->getSongBrowser()->getDynamicStarCalculator()->getTotalStars(),
					beatmap->getMostCommonBPM(),
					OsuGameRules::getApproachRateForSpeedMultiplier(beatmap, beatmap->getSpeedMultiplier()),
					beatmap->getCS(),
					OsuGameRules::getOverallDifficultyForSpeedMultiplier(beatmap, beatmap->getSpeedMultiplier()),
					beatmap->getHP(),
					beatmap->getNPS(),
					beatmap->getND(),
					m_osu->getScore()->getUnstableRate(),
					m_osu->getScore()->getPPv2(),
					m_osu->getSongBrowser()->getDynamicStarCalculator()->getPPv2(),
					((int)OsuGameRules::getHitWindow300(beatmap) - 0.5f) * (1.0f / m_osu->getSpeedMultiplier()), // see OsuUISongBrowserInfoLabel::update()
					m_osu->getScore()->getHitErrorAvgCustomMin(),
					m_osu->getScore()->getHitErrorAvgCustomMax());

			if (osu_draw_score.getBool())
				drawScore(g, m_osu->getScore()->getScore());

			if (osu_draw_combo.getBool())
				drawCombo(g, m_osu->getScore()->getCombo());

			if (osu_draw_progressbar.getBool())
				drawProgressBarVR(g, mvp, vr, beatmap->getPercentFinishedPlayable(), beatmap->isWaiting());

			if (osu_draw_accuracy.getBool())
				drawAccuracy(g, m_osu->getScore()->getAccuracy()*100.0f);
		}

		if (beatmap->shouldFlashSectionPass())
			drawSectionPass(g, beatmap->shouldFlashSectionPass());
		if (beatmap->shouldFlashSectionFail())
			drawSectionFail(g, beatmap->shouldFlashSectionFail());

		if (beatmap->shouldFlashWarningArrows())
			drawWarningArrows(g, beatmap->getHitcircleDiameter());

		if (beatmap->isLoading())
			drawLoadingSmall(g);
	}
	vr->getShaderTexturedLegacyGeneric()->disable();
}

void OsuHUD::drawVRDummy(Graphics *g, Matrix4 &mvp, OsuVR *vr)
{
	vr->getShaderTexturedLegacyGeneric()->enable();
	vr->getShaderTexturedLegacyGeneric()->setUniformMatrix4fv("matrix", mvp);
	{
		if (osu_draw_scorebarbg.getBool())
			drawScorebarBg(g, 1.0f, 0.0f);

		if (osu_draw_scorebar.getBool())
			drawHPBar(g, 1.0, 1.0f, 0.0f);

		SCORE_ENTRY scoreEntry;
		scoreEntry.name = m_name_ref->getString();
		scoreEntry.combo = 1234;
		scoreEntry.score = 12345678;
		scoreEntry.accuracy = 1.0f;
		scoreEntry.missingBeatmap = false;
		scoreEntry.downloadingBeatmap = false;
		scoreEntry.dead = false;
		scoreEntry.highlight = true;
		if (osu_draw_scoreboard.getBool())
		{
			static std::vector<SCORE_ENTRY> scoreEntries;
			scoreEntries.clear();
			{
				scoreEntries.push_back(scoreEntry);
			}
			drawScoreBoardInt(g, scoreEntries);
		}

		drawSkip(g);

		drawStatistics(g, 0, 0, 727, 2.3f, 5.5f, 180, 9.0f, 4.0f, 8.0f, 6.0f, 4, 6, 90.0f, 123, 1234, 25, -5, 15);

		if (osu_draw_score.getBool())
			drawScore(g, scoreEntry.score);

		if (osu_draw_combo.getBool())
			drawCombo(g, scoreEntry.combo);

		if (osu_draw_progressbar.getBool())
			drawProgressBarVR(g, mvp, vr, 0.25f, false);

		if (osu_draw_accuracy.getBool())
			drawAccuracy(g, scoreEntry.accuracy*100.0f);

		drawWarningArrows(g);
	}
	vr->getShaderTexturedLegacyGeneric()->disable();
}

void OsuHUD::drawCursor(Graphics *g, Vector2 pos, float alphaMultiplier, bool secondTrail, bool updateAndDrawTrail)
{
	if (osu_draw_cursor_ripples.getBool() && (!m_osu_mod_fposu_ref->getBool() || !m_osu->isInPlayMode()))
		drawCursorRipples(g);

	const bool vrTrailJumpFix = (m_osu->isInVRMode() && !m_osu->getVR()->isVirtualCursorOnScreen());

	Matrix4 mvp;
	drawCursorInt(g, m_cursorTrailShader, secondTrail ? m_cursorTrail2 : m_cursorTrail, mvp, pos, alphaMultiplier, vrTrailJumpFix, updateAndDrawTrail);
}

void OsuHUD::drawCursorTrail(Graphics *g, Vector2 pos, float alphaMultiplier, bool secondTrail)
{
	const bool vrTrailJumpFix = (m_osu->isInVRMode() && !m_osu->getVR()->isVirtualCursorOnScreen());
	const bool fposuTrailJumpFix = (m_osu_mod_fposu_ref->getBool() && m_osu->isInPlayMode() && !m_osu->getFPoSu()->isCrosshairIntersectingScreen());

	const bool trailJumpFix = (vrTrailJumpFix || fposuTrailJumpFix);

	Matrix4 mvp;
	drawCursorTrailInt(g, m_cursorTrailShader, secondTrail ? m_cursorTrail2 : m_cursorTrail, mvp, pos, alphaMultiplier, trailJumpFix);
}

void OsuHUD::drawCursorSpectator1(Graphics *g, Vector2 pos, float alphaMultiplier)
{
	Matrix4 mvp;
	drawCursorInt(g, m_cursorTrailShader, m_cursorTrailSpectator1, mvp, pos, alphaMultiplier);
}

void OsuHUD::drawCursorSpectator2(Graphics *g, Vector2 pos, float alphaMultiplier)
{
	Matrix4 mvp;
	drawCursorInt(g, m_cursorTrailShader, m_cursorTrailSpectator2, mvp, pos, alphaMultiplier);
}

void OsuHUD::drawCursorVR1(Graphics *g, Matrix4 &mvp, Vector2 pos, float alphaMultiplier)
{
	drawCursorInt(g, m_cursorTrailShaderVR, m_cursorTrailVR1, mvp, pos, alphaMultiplier);
}

void OsuHUD::drawCursorVR2(Graphics *g, Matrix4 &mvp, Vector2 pos, float alphaMultiplier)
{
	drawCursorInt(g, m_cursorTrailShaderVR, m_cursorTrailVR2, mvp, pos, alphaMultiplier);
}

void OsuHUD::drawCursorInt(Graphics *g, Shader *trailShader, std::vector<CURSORTRAIL> &trail, Matrix4 &mvp, Vector2 pos, float alphaMultiplier, bool emptyTrailFrame, bool updateAndDrawTrail)
{
	if (updateAndDrawTrail)
		drawCursorTrailInt(g, trailShader, trail, mvp, pos, alphaMultiplier, emptyTrailFrame);

	drawCursorRaw(g, pos, alphaMultiplier);
}

void OsuHUD::drawCursorTrailInt(Graphics *g, Shader *trailShader, std::vector<CURSORTRAIL> &trail, Matrix4 &mvp, Vector2 pos, float alphaMultiplier, bool emptyTrailFrame)
{
	Image *trailImage = m_osu->getSkin()->getCursorTrail();

	if (osu_draw_cursor_trail.getBool() && trailImage->isReady())
	{
		const bool smoothCursorTrail = m_osu->getSkin()->useSmoothCursorTrail() || osu_cursor_trail_smooth_force.getBool();

		const float trailWidth = trailImage->getWidth() * getCursorTrailScaleFactor() * osu_cursor_scale.getFloat();
		const float trailHeight = trailImage->getHeight() * getCursorTrailScaleFactor() * osu_cursor_scale.getFloat();

		if (smoothCursorTrail)
			m_cursorTrailVAO->empty();

		// add the sample for the current frame
		if (!m_osu->isSeekFrame())
			addCursorTrailPosition(trail, pos, emptyTrailFrame);

		// disable depth buffer for drawing any trail in VR
		if (trailShader == m_cursorTrailShaderVR)
		{
			g->setDepthBuffer(false);
		}

		// this loop draws the old style trail, and updates the alpha values for each segment, and fills the vao for the new style trail
		const float trailLength = smoothCursorTrail ? osu_cursor_trail_smooth_length.getFloat() : osu_cursor_trail_length.getFloat();
		int i = trail.size() - 1;
		while (i >= 0)
		{
			trail[i].alpha = clamp<float>(((trail[i].time - engine->getTime()) / trailLength) * alphaMultiplier, 0.0f, 1.0f) * osu_cursor_trail_alpha.getFloat();

			if (smoothCursorTrail)
			{
				const float scaleAnimTrailWidthHalf = (trailWidth/2) * trail[i].scale;
				const float scaleAnimTrailHeightHalf = (trailHeight/2) * trail[i].scale;

				const Vector3 topLeft = Vector3(trail[i].pos.x - scaleAnimTrailWidthHalf, trail[i].pos.y - scaleAnimTrailHeightHalf, trail[i].alpha);
				m_cursorTrailVAO->addVertex(topLeft);
				m_cursorTrailVAO->addTexcoord(0, 0);

				const Vector3 topRight = Vector3(trail[i].pos.x + scaleAnimTrailWidthHalf, trail[i].pos.y - scaleAnimTrailHeightHalf, trail[i].alpha);
				m_cursorTrailVAO->addVertex(topRight);
				m_cursorTrailVAO->addTexcoord(1, 0);

				const Vector3 bottomRight = Vector3(trail[i].pos.x + scaleAnimTrailWidthHalf, trail[i].pos.y + scaleAnimTrailHeightHalf, trail[i].alpha);
				m_cursorTrailVAO->addVertex(bottomRight);
				m_cursorTrailVAO->addTexcoord(1, 1);

				const Vector3 bottomLeft = Vector3(trail[i].pos.x - scaleAnimTrailWidthHalf, trail[i].pos.y + scaleAnimTrailHeightHalf, trail[i].alpha);
				m_cursorTrailVAO->addVertex(bottomLeft);
				m_cursorTrailVAO->addTexcoord(0, 1);
			}
			else // old style trail
			{
				if (trail[i].alpha > 0.0f)
					drawCursorTrailRaw(g, trail[i].alpha, trail[i].pos);
			}

			i--;
		}

		// draw new style continuous smooth trail
		if (smoothCursorTrail)
		{
			trailShader->enable();
			{
				if (trailShader == m_cursorTrailShaderVR)
				{
					trailShader->setUniformMatrix4fv("matrix", mvp);
				}

				trailShader->setUniform1f("time", engine->getTime());

#ifdef MCENGINE_FEATURE_OPENGLES
				{
					OpenGLES2Interface *gles2 = dynamic_cast<OpenGLES2Interface*>(g);
					if (gles2 != NULL)
					{
						g->forceUpdateTransform();
						Matrix4 mvp = g->getMVP();
						trailShader->setUniformMatrix4fv("mvp", mvp);
					}
				}
#endif

#ifdef MCENGINE_FEATURE_DIRECTX11
				{
					DirectX11Interface *dx11 = dynamic_cast<DirectX11Interface*>(g);
					if (dx11 != NULL)
					{
						g->forceUpdateTransform();
						Matrix4 mvp = g->getMVP();
						trailShader->setUniformMatrix4fv("mvp", mvp);
					}
				}
#endif

				trailImage->bind();
				{
					g->setBlendMode(Graphics::BLEND_MODE::BLEND_MODE_ADDITIVE);
					{
						g->setColor(0xffffffff);
						g->drawVAO(m_cursorTrailVAO);
					}
					g->setBlendMode(Graphics::BLEND_MODE::BLEND_MODE_ALPHA);
				}
				trailImage->unbind();
			}
			trailShader->disable();
		}

		if (trailShader == m_cursorTrailShaderVR)
		{
			g->setDepthBuffer(true);
		}
	}

	// trail cleanup
	while ((trail.size() > 1 && engine->getTime() > trail[0].time) || trail.size() > osu_cursor_trail_max_size.getInt()) // always leave at least 1 previous entry in there
	{
		trail.erase(trail.begin());
	}
}

void OsuHUD::drawCursorRaw(Graphics *g, Vector2 pos, float alphaMultiplier)
{
	Image *cursor = m_osu->getSkin()->getCursor();
	const float scale = getCursorScaleFactor() * (m_osu->getSkin()->isCursor2x() ? 0.5f : 1.0f);
	const float animatedScale = scale * (m_osu->getSkin()->getCursorExpand() ? m_fCursorExpandAnim : 1.0f);

	// draw cursor
	g->setColor(0xffffffff);
	g->setAlpha(osu_cursor_alpha.getFloat()*alphaMultiplier);
	g->pushTransform();
	{
		g->scale(animatedScale*osu_cursor_scale.getFloat(), animatedScale*osu_cursor_scale.getFloat());

		if (!m_osu->getSkin()->getCursorCenter())
			g->translate((cursor->getWidth()/2.0f)*animatedScale*osu_cursor_scale.getFloat(), (cursor->getHeight()/2.0f)*animatedScale*osu_cursor_scale.getFloat());

		if (m_osu->getSkin()->getCursorRotate())
			g->rotate(fmod(engine->getTime()*37.0f, 360.0f));

		g->translate(pos.x, pos.y);
		g->drawImage(cursor);
	}
	g->popTransform();

	// draw cursor middle
	if (m_osu->getSkin()->getCursorMiddle() != m_osu->getSkin()->getMissingTexture())
	{
		g->setColor(0xffffffff);
		g->setAlpha(osu_cursor_alpha.getFloat()*alphaMultiplier);
		g->pushTransform();
		{
			g->scale(scale*osu_cursor_scale.getFloat(), scale*osu_cursor_scale.getFloat());
			g->translate(pos.x, pos.y, 0.05f);

			if (!m_osu->getSkin()->getCursorCenter())
				g->translate((m_osu->getSkin()->getCursorMiddle()->getWidth()/2.0f)*scale*osu_cursor_scale.getFloat(), (m_osu->getSkin()->getCursorMiddle()->getHeight()/2.0f)*scale*osu_cursor_scale.getFloat());

			g->drawImage(m_osu->getSkin()->getCursorMiddle());
		}
		g->popTransform();
	}
}

void OsuHUD::drawCursorTrailRaw(Graphics *g, float alpha, Vector2 pos)
{
	Image *trailImage = m_osu->getSkin()->getCursorTrail();
	const float scale = getCursorTrailScaleFactor();
	const float animatedScale = scale * (m_osu->getSkin()->getCursorExpand() && osu_cursor_trail_expand.getBool() ? m_fCursorExpandAnim : 1.0f) * osu_cursor_trail_scale.getFloat();

	g->setColor(0xffffffff);
	g->setAlpha(alpha);
	g->pushTransform();
	{
		g->scale(animatedScale*osu_cursor_scale.getFloat(), animatedScale*osu_cursor_scale.getFloat());
		g->translate(pos.x, pos.y);
		g->drawImage(trailImage);
	}
	g->popTransform();
}

void OsuHUD::drawCursorRipples(Graphics *g)
{
	if (m_osu->getSkin()->getCursorRipple() == m_osu->getSkin()->getMissingTexture()) return;

	// allow overscale/underscale as usual
	// this does additionally scale with the resolution (which osu doesn't do for some reason for cursor ripples)
	const float normalized2xScale = (m_osu->getSkin()->isCursorRipple2x() ? 0.5f : 1.0f);
	const float imageScale = Osu::getImageScale(m_osu, Vector2(520.0f, 520.0f), 233.0f);

	const float normalizedWidth = m_osu->getSkin()->getCursorRipple()->getWidth() * normalized2xScale * imageScale;
	const float normalizedHeight = m_osu->getSkin()->getCursorRipple()->getHeight() * normalized2xScale * imageScale;

	const float duration = std::max(osu_cursor_ripple_duration.getFloat(), 0.0001f);
	const float fadeDuration = std::max(osu_cursor_ripple_duration.getFloat() - osu_cursor_ripple_anim_start_fadeout_delay.getFloat(), 0.0001f);

	if (osu_cursor_ripple_additive.getBool())
		g->setBlendMode(Graphics::BLEND_MODE::BLEND_MODE_ADDITIVE);

	g->setColor(COLOR(255, clamp<int>(osu_cursor_ripple_tint_r.getInt(), 0, 255), clamp<int>(osu_cursor_ripple_tint_g.getInt(), 0, 255), clamp<int>(osu_cursor_ripple_tint_b.getInt(), 0, 255)));
	m_osu->getSkin()->getCursorRipple()->bind();
	{
		for (int i=0; i<m_cursorRipples.size(); i++)
		{
			const Vector2 &pos = m_cursorRipples[i].pos;
			const float &time = m_cursorRipples[i].time;

			const float animPercent = 1.0f - clamp<float>((time - engine->getTime()) / duration, 0.0f, 1.0f);
			const float fadePercent = 1.0f - clamp<float>((time - engine->getTime()) / fadeDuration, 0.0f, 1.0f);

			const float scale = lerp<float>(osu_cursor_ripple_anim_start_scale.getFloat(), osu_cursor_ripple_anim_end_scale.getFloat(), 1.0f - (1.0f - animPercent)*(1.0f - animPercent)); // quad out

			g->setAlpha(osu_cursor_ripple_alpha.getFloat() * (1.0f - fadePercent));
			g->drawQuad(pos.x - normalizedWidth*scale/2, pos.y - normalizedHeight*scale/2, normalizedWidth*scale, normalizedHeight*scale);
		}
	}
	m_osu->getSkin()->getCursorRipple()->unbind();

	if (osu_cursor_ripple_additive.getBool())
		g->setBlendMode(Graphics::BLEND_MODE::BLEND_MODE_ALPHA);
}

void OsuHUD::drawFps(Graphics *g, McFont *font, float fps)
{
	fps = std::round(fps);
	const UString fpsString = UString::format("%i fps", (int)(fps));
	const UString msString = UString::format("%.1f ms", (1.0f/fps)*1000.0f);

	const float dpiScale = Osu::getUIScale(m_osu);

	const int margin = std::round(3.0f * dpiScale);
	const int shadowOffset = std::round(1.0f * dpiScale);

	// shadow
	g->setColor(0xff000000);
	g->pushTransform();
	{
		g->translate(m_osu->getScreenWidth() - font->getStringWidth(fpsString) - margin + shadowOffset, m_osu->getScreenHeight() - margin - font->getHeight() - margin + shadowOffset);
		g->drawString(font, fpsString);
	}
	g->popTransform();
	g->pushTransform();
	{
		g->translate(m_osu->getScreenWidth() - font->getStringWidth(msString) - margin + shadowOffset, m_osu->getScreenHeight() - margin + shadowOffset);
		g->drawString(font, msString);
	}
	g->popTransform();

	// top
	if (fps >= 200 || (m_osu->isInVRMode() && fps >= 80) || (env->getOS() == Environment::OS::OS_HORIZON && fps >= 50))
		g->setColor(0xffffffff);
	else if (fps >= 120 || (m_osu->isInVRMode() && fps >= 60) || (env->getOS() == Environment::OS::OS_HORIZON && fps >= 40))
		g->setColor(0xffdddd00);
	else
	{
		const float pulse = std::abs(std::sin(engine->getTime()*4));
		g->setColor(COLORf(1.0f, 1.0f, 0.26f*pulse, 0.26f*pulse));
	}

	g->pushTransform();
	{
		g->translate(m_osu->getScreenWidth() - font->getStringWidth(fpsString) - margin, m_osu->getScreenHeight() - margin - font->getHeight() - margin);
		g->drawString(font, fpsString);
	}
	g->popTransform();
	g->pushTransform();
	{
		g->translate(m_osu->getScreenWidth() - font->getStringWidth(msString) - margin, m_osu->getScreenHeight() - margin);
		g->drawString(font, msString);
	}
	g->popTransform();
}

void OsuHUD::drawPlayfieldBorder(Graphics *g, Vector2 playfieldCenter, Vector2 playfieldSize, float hitcircleDiameter)
{
	drawPlayfieldBorder(g, playfieldCenter, playfieldSize, hitcircleDiameter, osu_hud_playfield_border_size.getInt());
}

void OsuHUD::drawPlayfieldBorder(Graphics *g, Vector2 playfieldCenter, Vector2 playfieldSize, float hitcircleDiameter, float borderSize)
{
	if (borderSize <= 0.0f) return;

	// respect playfield stretching
	playfieldSize.x += playfieldSize.x*m_osu_playfield_stretch_x_ref->getFloat();
	playfieldSize.y += playfieldSize.y*m_osu_playfield_stretch_y_ref->getFloat();

	Vector2 playfieldBorderTopLeft = Vector2((int)(playfieldCenter.x - playfieldSize.x/2 - hitcircleDiameter/2 - borderSize), (int)(playfieldCenter.y - playfieldSize.y/2 - hitcircleDiameter/2 - borderSize));
	Vector2 playfieldBorderSize = Vector2((int)(playfieldSize.x + hitcircleDiameter), (int)(playfieldSize.y + hitcircleDiameter));

	const Color innerColor = 0x44ffffff;
	const Color outerColor = 0x00000000;

	g->pushTransform();
	{
		g->translate(0, 0, 0.2f);

		// top
		{
			static VertexArrayObject vao(Graphics::PRIMITIVE::PRIMITIVE_QUADS);
			vao.empty();

			vao.addVertex(playfieldBorderTopLeft);
			vao.addColor(outerColor);
			vao.addVertex(playfieldBorderTopLeft + Vector2(playfieldBorderSize.x + borderSize*2, 0));
			vao.addColor(outerColor);
			vao.addVertex(playfieldBorderTopLeft + Vector2(playfieldBorderSize.x + borderSize, borderSize));
			vao.addColor(innerColor);
			vao.addVertex(playfieldBorderTopLeft + Vector2(borderSize, borderSize));
			vao.addColor(innerColor);

			g->drawVAO(&vao);
		}

		// left
		{
			static VertexArrayObject vao(Graphics::PRIMITIVE::PRIMITIVE_QUADS);
			vao.empty();

			vao.addVertex(playfieldBorderTopLeft);
			vao.addColor(outerColor);
			vao.addVertex(playfieldBorderTopLeft + Vector2(borderSize, borderSize));
			vao.addColor(innerColor);
			vao.addVertex(playfieldBorderTopLeft + Vector2(borderSize, playfieldBorderSize.y + borderSize));
			vao.addColor(innerColor);
			vao.addVertex(playfieldBorderTopLeft + Vector2(0, playfieldBorderSize.y + 2*borderSize));
			vao.addColor(outerColor);

			g->drawVAO(&vao);
		}

		// right
		{
			static VertexArrayObject vao(Graphics::PRIMITIVE::PRIMITIVE_QUADS);
			vao.empty();

			vao.addVertex(playfieldBorderTopLeft + Vector2(playfieldBorderSize.x + 2*borderSize, 0));
			vao.addColor(outerColor);
			vao.addVertex(playfieldBorderTopLeft + Vector2(playfieldBorderSize.x + 2*borderSize, playfieldBorderSize.y + 2*borderSize));
			vao.addColor(outerColor);
			vao.addVertex(playfieldBorderTopLeft + Vector2(playfieldBorderSize.x + borderSize, playfieldBorderSize.y + borderSize));
			vao.addColor(innerColor);
			vao.addVertex(playfieldBorderTopLeft + Vector2(playfieldBorderSize.x + borderSize, borderSize));
			vao.addColor(innerColor);

			g->drawVAO(&vao);
		}

		// bottom
		{
			static VertexArrayObject vao(Graphics::PRIMITIVE::PRIMITIVE_QUADS);
			vao.empty();

			vao.addVertex(playfieldBorderTopLeft + Vector2(borderSize, playfieldBorderSize.y + borderSize));
			vao.addColor(innerColor);
			vao.addVertex(playfieldBorderTopLeft + Vector2(playfieldBorderSize.x + borderSize, playfieldBorderSize.y + borderSize));
			vao.addColor(innerColor);
			vao.addVertex(playfieldBorderTopLeft + Vector2(playfieldBorderSize.x + 2*borderSize, playfieldBorderSize.y + 2*borderSize));
			vao.addColor(outerColor);
			vao.addVertex(playfieldBorderTopLeft + Vector2(0, playfieldBorderSize.y + 2*borderSize));
			vao.addColor(outerColor);

			g->drawVAO(&vao);
		}
	}
	g->popTransform();

	/*
	//g->setColor(0x44ffffff);
	// top
	g->fillRect(playfieldBorderTopLeft.x, playfieldBorderTopLeft.y, playfieldBorderSize.x + borderSize*2, borderSize);

	// left
	g->fillRect(playfieldBorderTopLeft.x, playfieldBorderTopLeft.y + borderSize, borderSize, playfieldBorderSize.y);

	// right
	g->fillRect(playfieldBorderTopLeft.x + playfieldBorderSize.x + borderSize, playfieldBorderTopLeft.y + borderSize, borderSize, playfieldBorderSize.y);

	// bottom
	g->fillRect(playfieldBorderTopLeft.x, playfieldBorderTopLeft.y+playfieldBorderSize.y + borderSize, playfieldBorderSize.x + borderSize*2, borderSize);
	*/
}

void OsuHUD::drawLoadingSmall(Graphics *g)
{
	/*
	McFont *font = engine->getResourceManager()->getFont("FONT_DEFAULT");
	UString loadingText = "Loading ...";
	float stringWidth = font->getStringWidth(loadingText);

	g->setColor(0xffffffff);
	g->pushTransform();
	g->translate(-stringWidth/2, font->getHeight()/2);
	g->rotate(engine->getTime()*180, 0, 0, 1);
	g->translate(0, 0);
	g->translate(m_osu->getScreenWidth()/2, m_osu->getScreenHeight()/2);
	g->drawString(font, loadingText);
	g->popTransform();
	*/
	const float scale = Osu::getImageScale(m_osu, m_osu->getSkin()->getLoadingSpinner(), 29);

	g->setColor(0xffffffff);
	g->pushTransform();
	{
		g->rotate(engine->getTime()*180, 0, 0, 1);
		g->scale(scale, scale);
		g->translate(m_osu->getScreenWidth()/2, m_osu->getScreenHeight()/2);
		g->drawImage(m_osu->getSkin()->getLoadingSpinner());
	}
	g->popTransform();
}

void OsuHUD::drawBeatmapImportSpinner(Graphics *g)
{
	const float scale = Osu::getImageScale(m_osu, m_osu->getSkin()->getBeatmapImportSpinner(), 100);

	g->setColor(0xffffffff);
	g->pushTransform();
	{
		g->rotate(engine->getTime()*180, 0, 0, 1);
		g->scale(scale, scale);
		g->translate(m_osu->getScreenWidth()/2, m_osu->getScreenHeight()/2);
		g->drawImage(m_osu->getSkin()->getBeatmapImportSpinner());
	}
	g->popTransform();
}

void OsuHUD::drawVolumeChange(Graphics *g)
{
	if (engine->getTime() > m_fVolumeChangeTime) return;

	const float dpiScale = Osu::getUIScale(m_osu);
	const float sizeMultiplier = osu_hud_volume_size_multiplier.getFloat() * dpiScale;
	const float height = m_osu->getScreenHeight() - m_volumeMusic->getPos().y;

	// legacy
	/*
	g->setColor(COLOR((char)(68*m_fVolumeChangeFade), 255, 255, 255));
	g->fillRect(0, m_osu->getScreenHeight()*(1.0f - m_fLastVolume), m_osu->getScreenWidth(), m_osu->getScreenHeight()*m_fLastVolume);
	*/

	if (m_fVolumeChangeFade != 1.0f)
	{
		g->push3DScene(McRect(m_volumeMaster->getPos().x, m_volumeMaster->getPos().y, m_volumeMaster->getSize().x, (m_osu->getScreenHeight() - m_volumeMaster->getPos().y)*2.05f));
		//g->rotate3DScene(-(1.0f - m_fVolumeChangeFade)*90, 0, 0);
		//g->translate3DScene(0, (m_fVolumeChangeFade*60 - 60) * sizeMultiplier / 1.5f, ((m_fVolumeChangeFade)*500 - 500) * sizeMultiplier / 1.5f);
		g->translate3DScene(0, (m_fVolumeChangeFade*height - height)*1.05f, 0.0f);

	}

	m_volumeMaster->setPos(m_osu->getScreenSize() - m_volumeMaster->getSize() - Vector2(m_volumeMaster->getMinimumExtraTextWidth(), m_volumeMaster->getSize().y));
	m_volumeEffects->setPos(m_volumeMaster->getPos() - Vector2(0, m_volumeEffects->getSize().y + 20 * sizeMultiplier));
	m_volumeMusic->setPos(m_volumeEffects->getPos() - Vector2(0, m_volumeMusic->getSize().y + 20 * sizeMultiplier));

	m_volumeSliderOverlayContainer->draw(g);

	if (m_fVolumeChangeFade != 1.0f)
		g->pop3DScene();
}

void OsuHUD::drawScoreNumber(Graphics *g, unsigned long long number, float scale, bool drawLeadingZeroes)
{
	// get digits
	static std::vector<int> digits;
	digits.clear();
	while (number >= 10)
	{
		int curDigit = number % 10;
		number /= 10;

		digits.insert(digits.begin(), curDigit);
	}
	digits.insert(digits.begin(), number);
	if (digits.size() == 1)
	{
		if (drawLeadingZeroes)
			digits.insert(digits.begin(), 0);
	}

	// draw them
	// NOTE: just using the width here is incorrect, but it is the quickest solution instead of painstakingly reverse-engineering how osu does it
	float lastWidth = m_osu->getSkin()->getScore0()->getWidth();
	for (int i=0; i<digits.size(); i++)
	{
		switch (digits[i])
		{
		case 0:
			g->translate(lastWidth*0.5f*scale, 0);
			g->drawImage(m_osu->getSkin()->getScore0());
			g->translate(lastWidth*0.5f*scale, 0);
			break;
		case 1:
			g->translate(lastWidth*0.5f*scale, 0);
			g->drawImage(m_osu->getSkin()->getScore1());
			g->translate(lastWidth*0.5f*scale, 0);
			break;
		case 2:
			g->translate(lastWidth*0.5f*scale, 0);
			g->drawImage(m_osu->getSkin()->getScore2());
			g->translate(lastWidth*0.5f*scale, 0);
			break;
		case 3:
			g->translate(lastWidth*0.5f*scale, 0);
			g->drawImage(m_osu->getSkin()->getScore3());
			g->translate(lastWidth*0.5f*scale, 0);
			break;
		case 4:
			g->translate(lastWidth*0.5f*scale, 0);
			g->drawImage(m_osu->getSkin()->getScore4());
			g->translate(lastWidth*0.5f*scale, 0);
			break;
		case 5:
			g->translate(lastWidth*0.5f*scale, 0);
			g->drawImage(m_osu->getSkin()->getScore5());
			g->translate(lastWidth*0.5f*scale, 0);
			break;
		case 6:
			g->translate(lastWidth*0.5f*scale, 0);
			g->drawImage(m_osu->getSkin()->getScore6());
			g->translate(lastWidth*0.5f*scale, 0);
			break;
		case 7:
			g->translate(lastWidth*0.5f*scale, 0);
			g->drawImage(m_osu->getSkin()->getScore7());
			g->translate(lastWidth*0.5f*scale, 0);
			break;
		case 8:
			g->translate(lastWidth*0.5f*scale, 0);
			g->drawImage(m_osu->getSkin()->getScore8());
			g->translate(lastWidth*0.5f*scale, 0);
			break;
		case 9:
			g->translate(lastWidth*0.5f*scale, 0);
			g->drawImage(m_osu->getSkin()->getScore9());
			g->translate(lastWidth*0.5f*scale, 0);
			break;
		}

		g->translate(-m_osu->getSkin()->getScoreOverlap()*(m_osu->getSkin()->isScore02x() ? 2 : 1)*scale, 0);
	}
}

void OsuHUD::drawComboNumber(Graphics *g, unsigned long long number, float scale, bool drawLeadingZeroes)
{
	// get digits
	static std::vector<int> digits;
	digits.clear();
	while (number >= 10)
	{
		int curDigit = number % 10;
		number /= 10;

		digits.insert(digits.begin(), curDigit);
	}
	digits.insert(digits.begin(), number);
	if (digits.size() == 1)
	{
		if (drawLeadingZeroes)
			digits.insert(digits.begin(), 0);
	}

	// draw them
	// NOTE: just using the width here is incorrect, but it is the quickest solution instead of painstakingly reverse-engineering how osu does it
	float lastWidth = m_osu->getSkin()->getCombo0()->getWidth();
	for (int i=0; i<digits.size(); i++)
	{
		switch (digits[i])
		{
		case 0:
			g->translate(lastWidth*0.5f*scale, 0);
			g->drawImage(m_osu->getSkin()->getCombo0());
			g->translate(lastWidth*0.5f*scale, 0);
			break;
		case 1:
			g->translate(lastWidth*0.5f*scale, 0);
			g->drawImage(m_osu->getSkin()->getCombo1());
			g->translate(lastWidth*0.5f*scale, 0);
			break;
		case 2:
			g->translate(lastWidth*0.5f*scale, 0);
			g->drawImage(m_osu->getSkin()->getCombo2());
			g->translate(lastWidth*0.5f*scale, 0);
			break;
		case 3:
			g->translate(lastWidth*0.5f*scale, 0);
			g->drawImage(m_osu->getSkin()->getCombo3());
			g->translate(lastWidth*0.5f*scale, 0);
			break;
		case 4:
			g->translate(lastWidth*0.5f*scale, 0);
			g->drawImage(m_osu->getSkin()->getCombo4());
			g->translate(lastWidth*0.5f*scale, 0);
			break;
		case 5:
			g->translate(lastWidth*0.5f*scale, 0);
			g->drawImage(m_osu->getSkin()->getCombo5());
			g->translate(lastWidth*0.5f*scale, 0);
			break;
		case 6:
			g->translate(lastWidth*0.5f*scale, 0);
			g->drawImage(m_osu->getSkin()->getCombo6());
			g->translate(lastWidth*0.5f*scale, 0);
			break;
		case 7:
			g->translate(lastWidth*0.5f*scale, 0);
			g->drawImage(m_osu->getSkin()->getCombo7());
			g->translate(lastWidth*0.5f*scale, 0);
			break;
		case 8:
			g->translate(lastWidth*0.5f*scale, 0);
			g->drawImage(m_osu->getSkin()->getCombo8());
			g->translate(lastWidth*0.5f*scale, 0);
			break;
		case 9:
			g->translate(lastWidth*0.5f*scale, 0);
			g->drawImage(m_osu->getSkin()->getCombo9());
			g->translate(lastWidth*0.5f*scale, 0);
			break;
		}

		g->translate(-m_osu->getSkin()->getComboOverlap()*(m_osu->getSkin()->isCombo02x() ? 2 : 1)*scale, 0);
	}
}

void OsuHUD::drawComboSimple(Graphics *g, int combo, float scale)
{
	g->pushTransform();
	{
		drawComboNumber(g, combo, scale);

		// draw 'x' at the end
		if (m_osu->getSkin()->getComboX() != m_osu->getSkin()->getMissingTexture())
		{
			g->translate(m_osu->getSkin()->getComboX()->getWidth()*0.5f*scale, 0);
			g->drawImage(m_osu->getSkin()->getComboX());
		}
	}
	g->popTransform();
}

void OsuHUD::drawCombo(Graphics *g, int combo)
{
	g->setColor(0xffffffff);

	const int offset = 5;

	// draw back (anim)
	float animScaleMultiplier = 1.0f + m_fComboAnim2*osu_combo_anim2_size.getFloat();
	float scale = m_osu->getImageScale(m_osu, m_osu->getSkin()->getCombo0(), 32)*animScaleMultiplier * osu_hud_scale.getFloat() * osu_hud_combo_scale.getFloat();
	if (m_fComboAnim2 > 0.01f)
	{
		g->setAlpha(m_fComboAnim2*0.65f);
		g->pushTransform();
		{
			g->scale(scale, scale);
			g->translate(offset, m_osu->getScreenHeight() - m_osu->getSkin()->getCombo0()->getHeight()*scale/2.0f, m_osu->isInVRMode() ? 0.25f : 0.0f);
			drawComboNumber(g, combo, scale);

			// draw 'x' at the end
			if (m_osu->getSkin()->getComboX() != m_osu->getSkin()->getMissingTexture())
			{
				g->translate(m_osu->getSkin()->getComboX()->getWidth()*0.5f*scale, 0);
				g->drawImage(m_osu->getSkin()->getComboX());
			}
		}
		g->popTransform();
	}

	// draw front
	g->setAlpha(1.0f);
	const float animPercent = (m_fComboAnim1 < 1.0f ? m_fComboAnim1 : 2.0f - m_fComboAnim1);
	animScaleMultiplier = 1.0f + (0.5f*animPercent*animPercent)*osu_combo_anim1_size.getFloat();
	scale = m_osu->getImageScale(m_osu, m_osu->getSkin()->getCombo0(), 32) * animScaleMultiplier * osu_hud_scale.getFloat() * osu_hud_combo_scale.getFloat();
	g->pushTransform();
	{
		g->scale(scale, scale);
		g->translate(offset, m_osu->getScreenHeight() - m_osu->getSkin()->getCombo0()->getHeight()*scale/2.0f, m_osu->isInVRMode() ? 0.45f : 0.0f);
		drawComboNumber(g, combo, scale);

		// draw 'x' at the end
		if (m_osu->getSkin()->getComboX() != m_osu->getSkin()->getMissingTexture())
		{
			g->translate(m_osu->getSkin()->getComboX()->getWidth()*0.5f*scale, 0);
			g->drawImage(m_osu->getSkin()->getComboX());
		}
	}
	g->popTransform();
}

void OsuHUD::drawScore(Graphics *g, unsigned long long score)
{
	g->setColor(0xffffffff);

	int numDigits = 1;
	unsigned long long scoreCopy = score;
	while (scoreCopy >= 10)
	{
		scoreCopy /= 10;
		numDigits++;
	}

	const float scale = getScoreScale();
	g->pushTransform();
	{
		g->scale(scale, scale);
		g->translate(m_osu->getScreenWidth() - m_osu->getSkin()->getScore0()->getWidth()*scale*numDigits + m_osu->getSkin()->getScoreOverlap()*(m_osu->getSkin()->isScore02x() ? 2 : 1)*scale*(numDigits-1), m_osu->getSkin()->getScore0()->getHeight()*scale/2);
		drawScoreNumber(g, score, scale, false);
	}
	g->popTransform();
}

void OsuHUD::drawScorebarBg(Graphics *g, float alpha, float breakAnim)
{
	if (m_osu->getSkin()->getScorebarBg()->isMissingTexture()) return;

	const float scale = osu_hud_scale.getFloat() * osu_hud_scorebar_scale.getFloat();
	const float ratio = Osu::getImageScale(m_osu, Vector2(1, 1), 1.0f);

	const Vector2 breakAnimOffset = Vector2(0, -20.0f * breakAnim) * ratio;

	if (m_osu->isInVRDraw())
	{
		g->pushTransform();
		g->translate(0.0f, 0.0f, -0.2f);
	}

	g->setColor(0xffffffff);
	g->setAlpha(alpha * (1.0f - breakAnim));
	m_osu->getSkin()->getScorebarBg()->draw(g, (m_osu->getSkin()->getScorebarBg()->getSize() / 2.0f) * scale + (breakAnimOffset * scale), scale);

	if (m_osu->isInVRDraw())
	{
		g->popTransform();
	}
}

void OsuHUD::drawSectionPass(Graphics *g, float alpha)
{
	if (!m_osu->getSkin()->getSectionPassImage()->isMissingTexture())
	{
		if (m_osu->isInVRDraw())
		{
			g->pushTransform();
			g->translate(0.0f, 0.0f, 0.2f);
		}

		g->setColor(0xffffffff);
		g->setAlpha(alpha);
		m_osu->getSkin()->getSectionPassImage()->draw(g, m_osu->getScreenSize() / 2);

		if (m_osu->isInVRDraw())
		{
			g->popTransform();
		}
	}
}

void OsuHUD::drawSectionFail(Graphics *g, float alpha)
{
	if (!m_osu->getSkin()->getSectionFailImage()->isMissingTexture())
	{
		if (m_osu->isInVRDraw())
		{
			g->pushTransform();
			g->translate(0.0f, 0.0f, 0.2f);
		}

		g->setColor(0xffffffff);
		g->setAlpha(alpha);
		m_osu->getSkin()->getSectionFailImage()->draw(g, m_osu->getScreenSize() / 2);

		if (m_osu->isInVRDraw())
		{
			g->popTransform();
		}
	}
}

void OsuHUD::drawHPBar(Graphics *g, double health, float alpha, float breakAnim)
{
	const bool useNewDefault = !m_osu->getSkin()->getScorebarMarker()->isMissingTexture();

	const float scale = osu_hud_scale.getFloat() * osu_hud_scorebar_scale.getFloat();
	const float ratio = Osu::getImageScale(m_osu, Vector2(1, 1), 1.0f);

	const Vector2 colourOffset = (useNewDefault ? Vector2(7.5f, 7.8f) : Vector2(3.0f, 10.0f)) * ratio;
	const float currentXPosition = (colourOffset.x + (health * m_osu->getSkin()->getScorebarColour()->getSize().x));
	const Vector2 markerOffset = (useNewDefault ? Vector2(currentXPosition, (8.125f + 2.5f) * ratio) : Vector2(currentXPosition, 10.0f * ratio));
	const Vector2 breakAnimOffset = Vector2(0, -20.0f * breakAnim) * ratio;

	// lerp color depending on health
	if (useNewDefault)
	{
		if (health < 0.2)
		{
			const float factor = std::max(0.0, (0.2 - health) / 0.2);
			const float value = lerp<float>(0.0f, 1.0f, factor);
			g->setColor(COLORf(1.0f, value, 0.0f, 0.0f));
		}
		else if (health < 0.5)
		{
			const float factor = std::max(0.0, (0.5 - health) / 0.5);
			const float value = lerp<float>(1.0f, 0.0f, factor);
			g->setColor(COLORf(1.0f, value, value, value));
		}
		else
			g->setColor(0xffffffff);
	}
	else
		g->setColor(0xffffffff);

	if (breakAnim != 0.0f || alpha != 1.0f)
		g->setAlpha(alpha * (1.0f - breakAnim));

	if (m_osu->isInVRDraw())
	{
		g->pushTransform();
	}

	// draw health bar fill
	{
		if (m_osu->isInVRDraw())
		{
			g->translate(0.0f, 0.0f, 0.15f);
		}

		// DEBUG:
		/*
		g->fillRect(0, 25, m_osu->getScreenWidth()*health, 10);
		*/

		m_osu->getSkin()->getScorebarColour()->setDrawClipWidthPercent(health);
		m_osu->getSkin()->getScorebarColour()->draw(g, (m_osu->getSkin()->getScorebarColour()->getSize() / 2.0f * scale) + (colourOffset * scale) + (breakAnimOffset * scale), scale);
	}

	// draw ki
	{
		OsuSkinImage *ki = NULL;

		if (useNewDefault)
			ki = m_osu->getSkin()->getScorebarMarker();
		else if (m_osu->getSkin()->getScorebarColour()->isFromDefaultSkin() || !m_osu->getSkin()->getScorebarKi()->isFromDefaultSkin())
		{
			if (health < 0.2)
				ki = m_osu->getSkin()->getScorebarKiDanger2();
			else if (health < 0.5)
				ki = m_osu->getSkin()->getScorebarKiDanger();
			else
				ki = m_osu->getSkin()->getScorebarKi();
		}

		if (ki != NULL && !ki->isMissingTexture())
		{
			if (!useNewDefault || health >= 0.2)
			{
				if (m_osu->isInVRDraw())
				{
					g->translate(0.0f, 0.0f, 0.15f);
				}

				ki->draw(g, (markerOffset * scale) + (breakAnimOffset * scale), scale * m_fKiScaleAnim);
			}
		}
	}

	if (m_osu->isInVRDraw())
	{
		g->popTransform();
	}
}

void OsuHUD::drawAccuracySimple(Graphics *g, float accuracy, float scale)
{
	// get integer & fractional parts of the number
	const int accuracyInt = (int)accuracy;
	const int accuracyFrac = clamp<int>(((int)(std::round((accuracy - accuracyInt)*100.0f))), 0, 99); // round up

	// draw it
	g->pushTransform();
	{
		drawScoreNumber(g, accuracyInt, scale, true);

		// draw dot '.' between the integer and fractional part
		if (m_osu->getSkin()->getScoreDot() != m_osu->getSkin()->getMissingTexture())
		{
			g->setColor(0xffffffff);
			g->translate(m_osu->getSkin()->getScoreDot()->getWidth()*0.5f*scale, 0);
			g->drawImage(m_osu->getSkin()->getScoreDot());
			g->translate(m_osu->getSkin()->getScoreDot()->getWidth()*0.5f*scale, 0);
			g->translate(-m_osu->getSkin()->getScoreOverlap()*(m_osu->getSkin()->isScore02x() ? 2 : 1)*scale, 0);
		}

		drawScoreNumber(g, accuracyFrac, scale, true);

		// draw '%' at the end
		if (m_osu->getSkin()->getScorePercent() != m_osu->getSkin()->getMissingTexture())
		{
			g->setColor(0xffffffff);
			g->translate(m_osu->getSkin()->getScorePercent()->getWidth()*0.5f*scale, 0);
			g->drawImage(m_osu->getSkin()->getScorePercent());
		}
	}
	g->popTransform();
}

void OsuHUD::drawAccuracy(Graphics *g, float accuracy)
{
	g->setColor(0xffffffff);

	// get integer & fractional parts of the number
	const int accuracyInt = (int)accuracy;
	const int accuracyFrac = clamp<int>(((int)(std::round((accuracy - accuracyInt)*100.0f))), 0, 99); // round up

	// draw it
	const int offset = 5;
	const float scale = m_osu->getImageScale(m_osu, m_osu->getSkin()->getScore0(), 13) * osu_hud_scale.getFloat() * osu_hud_accuracy_scale.getFloat();
	g->pushTransform();
	{
		const int numDigits = (accuracyInt > 99 ? 5 : 4);
		const float xOffset = m_osu->getSkin()->getScore0()->getWidth()*scale*numDigits + (m_osu->getSkin()->getScoreDot() != m_osu->getSkin()->getMissingTexture() ? m_osu->getSkin()->getScoreDot()->getWidth() : 0)*scale + (m_osu->getSkin()->getScorePercent() != m_osu->getSkin()->getMissingTexture() ? m_osu->getSkin()->getScorePercent()->getWidth() : 0)*scale - m_osu->getSkin()->getScoreOverlap()*(m_osu->getSkin()->isScore02x() ? 2 : 1)*scale*(numDigits+1);

		m_fAccuracyXOffset = m_osu->getScreenWidth() - xOffset - offset;
		m_fAccuracyYOffset = (osu_draw_score.getBool() ? m_fScoreHeight : 0.0f) + m_osu->getSkin()->getScore0()->getHeight()*scale/2 + offset*2;

		g->scale(scale, scale);
		g->translate(m_fAccuracyXOffset, m_fAccuracyYOffset);

		drawScoreNumber(g, accuracyInt, scale, true);

		// draw dot '.' between the integer and fractional part
		if (m_osu->getSkin()->getScoreDot() != m_osu->getSkin()->getMissingTexture())
		{
			g->setColor(0xffffffff);
			g->translate(m_osu->getSkin()->getScoreDot()->getWidth()*0.5f*scale, 0);
			g->drawImage(m_osu->getSkin()->getScoreDot());
			g->translate(m_osu->getSkin()->getScoreDot()->getWidth()*0.5f*scale, 0);
			g->translate(-m_osu->getSkin()->getScoreOverlap()*(m_osu->getSkin()->isScore02x() ? 2 : 1)*scale, 0);
		}

		drawScoreNumber(g, accuracyFrac, scale, true);

		// draw '%' at the end
		if (m_osu->getSkin()->getScorePercent() != m_osu->getSkin()->getMissingTexture())
		{
			g->setColor(0xffffffff);
			g->translate(m_osu->getSkin()->getScorePercent()->getWidth()*0.5f*scale, 0);
			g->drawImage(m_osu->getSkin()->getScorePercent());
		}
	}
	g->popTransform();
}

void OsuHUD::drawSkip(Graphics *g)
{
	const float scale = osu_hud_scale.getFloat();

	g->setColor(0xffffffff);
	m_osu->getSkin()->getPlaySkip()->draw(g, m_osu->getScreenSize() - (m_osu->getSkin()->getPlaySkip()->getSize()/2)*scale, osu_hud_scale.getFloat());
}

void OsuHUD::drawWarningArrow(Graphics *g, Vector2 pos, bool flipVertically, bool originLeft)
{
	const float scale = osu_hud_scale.getFloat() * m_osu->getImageScale(m_osu, m_osu->getSkin()->getPlayWarningArrow(), 78);

	g->pushTransform();
	{
		g->scale(flipVertically ? -scale : scale, scale);
		g->translate(pos.x + (flipVertically ? (-m_osu->getSkin()->getPlayWarningArrow()->getWidth()*scale/2.0f) : (m_osu->getSkin()->getPlayWarningArrow()->getWidth()*scale/2.0f)) * (originLeft ? 1.0f : -1.0f), pos.y, m_osu->isInVRMode() ? 0.75f : 0.0f);
		g->drawImage(m_osu->getSkin()->getPlayWarningArrow());
	}
	g->popTransform();
}

void OsuHUD::drawWarningArrows(Graphics *g, float hitcircleDiameter)
{
	const float divider = 18.0f;
	const float part = OsuGameRules::getPlayfieldSize(m_osu).y * (1.0f / divider);

	g->setColor(0xffffffff);
	drawWarningArrow(g, Vector2(m_osu->getUIScale(m_osu, 28), OsuGameRules::getPlayfieldCenter(m_osu).y - OsuGameRules::getPlayfieldSize(m_osu).y/2 + part*2), false);
	drawWarningArrow(g, Vector2(m_osu->getUIScale(m_osu, 28), OsuGameRules::getPlayfieldCenter(m_osu).y - OsuGameRules::getPlayfieldSize(m_osu).y/2 + part*2 + part*13), false);

	drawWarningArrow(g, Vector2(m_osu->getScreenWidth() - m_osu->getUIScale(m_osu, 28), OsuGameRules::getPlayfieldCenter(m_osu).y - OsuGameRules::getPlayfieldSize(m_osu).y/2 + part*2), true);
	drawWarningArrow(g, Vector2(m_osu->getScreenWidth() - m_osu->getUIScale(m_osu, 28), OsuGameRules::getPlayfieldCenter(m_osu).y - OsuGameRules::getPlayfieldSize(m_osu).y/2 + part*2 + part*13), true);
}

void OsuHUD::drawScoreBoard(Graphics *g, std::string &beatmapMD5Hash, OsuScore *currentScore)
{
	const int maxVisibleDatabaseScores = m_osu->isInVRDraw() ? 3 : 4;

	const std::vector<OsuDatabase::Score> *scores = &((*m_osu->getSongBrowser()->getDatabase()->getScores())[beatmapMD5Hash]);
	const int numScores = scores->size();

	if (numScores < 1) return;

	static std::vector<SCORE_ENTRY> scoreEntries;
	scoreEntries.clear();
	scoreEntries.reserve(numScores);

	const bool isUnranked = (m_osu->getModAuto() || (m_osu->getModAutopilot() && m_osu->getModRelax()));

	bool injectCurrentScore = true;
	for (int i=0; i<numScores && i<maxVisibleDatabaseScores; i++)
	{
		SCORE_ENTRY scoreEntry;

		scoreEntry.name = (*scores)[i].playerName;

		scoreEntry.index = -1;
		scoreEntry.combo = (*scores)[i].comboMax;
		scoreEntry.score = (*scores)[i].score;
		scoreEntry.accuracy = OsuScore::calculateAccuracy((*scores)[i].num300s, (*scores)[i].num100s, (*scores)[i].num50s, (*scores)[i].numMisses);

		scoreEntry.missingBeatmap = false;
		scoreEntry.downloadingBeatmap = false;
		scoreEntry.dead = false;
		scoreEntry.highlight = false;

		const bool isLastScore = (i == numScores-1) || (i == maxVisibleDatabaseScores-1);
		const bool isCurrentScoreMore = (currentScore->getScore() > scoreEntry.score);
		bool scoreAlreadyAdded = false;
		if (injectCurrentScore && (isCurrentScoreMore || isLastScore))
		{
			injectCurrentScore = false;

			SCORE_ENTRY currentScoreEntry;

			currentScoreEntry.name = (isUnranked ? "McOsu" : m_name_ref->getString());

			currentScoreEntry.combo = currentScore->getComboMax();
			currentScoreEntry.score = currentScore->getScore();
			currentScoreEntry.accuracy = currentScore->getAccuracy();

			currentScoreEntry.index = i;
			if (!isCurrentScoreMore)
			{
				for (int j=i; j<numScores; j++)
				{
					if (currentScoreEntry.score > (*scores)[j].score)
						break;
					else
						currentScoreEntry.index = j+1;
				}
			}

			currentScoreEntry.missingBeatmap = false;
			currentScoreEntry.downloadingBeatmap = false;
			currentScoreEntry.dead = currentScore->isDead();
			currentScoreEntry.highlight = true;

			if (isLastScore)
			{
				if (isCurrentScoreMore)
					scoreEntries.push_back(std::move(currentScoreEntry));

				scoreEntries.push_back(std::move(scoreEntry));
				scoreAlreadyAdded = true;

				if (!isCurrentScoreMore)
					scoreEntries.push_back(std::move(currentScoreEntry));
			}
			else
				scoreEntries.push_back(std::move(currentScoreEntry));
		}

		if (!scoreAlreadyAdded)
			scoreEntries.push_back(std::move(scoreEntry));
	}

	drawScoreBoardInt(g, scoreEntries);
}

void OsuHUD::drawScoreBoardMP(Graphics *g)
{
	const int numPlayers = m_osu->getMultiplayer()->getPlayers()->size();

	static std::vector<SCORE_ENTRY> scoreEntries;
	scoreEntries.clear();
	scoreEntries.reserve(numPlayers);

	for (int i=0; i<numPlayers; i++)
	{
		SCORE_ENTRY scoreEntry;

		scoreEntry.name = (*m_osu->getMultiplayer()->getPlayers())[i].name;

		scoreEntry.index = -1;
		scoreEntry.combo = (*m_osu->getMultiplayer()->getPlayers())[i].combo;
		scoreEntry.score = (*m_osu->getMultiplayer()->getPlayers())[i].score;
		scoreEntry.accuracy = (*m_osu->getMultiplayer()->getPlayers())[i].accuracy;

		scoreEntry.missingBeatmap = (*m_osu->getMultiplayer()->getPlayers())[i].missingBeatmap;
		scoreEntry.downloadingBeatmap = (*m_osu->getMultiplayer()->getPlayers())[i].downloadingBeatmap;
		scoreEntry.dead = (*m_osu->getMultiplayer()->getPlayers())[i].dead;
		scoreEntry.highlight = ((*m_osu->getMultiplayer()->getPlayers())[i].id == engine->getNetworkHandler()->getLocalClientID());

		scoreEntries.push_back(std::move(scoreEntry));
	}

	drawScoreBoardInt(g, scoreEntries);
}

void OsuHUD::drawScoreBoardInt(Graphics *g, const std::vector<OsuHUD::SCORE_ENTRY> &scoreEntries)
{
	if (scoreEntries.size() < 1) return;

	const bool useMenuButtonBackground = osu_hud_scoreboard_use_menubuttonbackground.getBool();
	OsuSkinImage *backgroundImage = m_osu->getSkin()->getMenuButtonBackground2();
	const float oScale = backgroundImage->getResolutionScale() * 0.99f; // for converting harcoded osu offset pixels to screen pixels

	McFont *indexFont = m_osu->getSongBrowserFontBold();
	McFont *nameFont = m_osu->getSongBrowserFont();
	McFont *scoreFont = m_osu->getSongBrowserFont();
	McFont *comboFont = scoreFont;
	McFont *accFont = comboFont;

	const Color backgroundColor = 0x55114459;
	const Color backgroundColorHighlight = 0x55777777;
	const Color backgroundColorTop = 0x551b6a8c;
	const Color backgroundColorDead = 0x55660000;
	const Color backgroundColorMissingBeatmap = 0x55aa0000;

	const Color indexColor = 0x11ffffff;
	const Color indexColorHighlight = 0x22ffffff;

	const Color nameScoreColor = 0xffaaaaaa;
	const Color nameScoreColorHighlight = 0xffffffff;
	const Color nameScoreColorTop = 0xffeeeeee;
	const Color nameScoreColorDownloading = 0xffeeee00;
	const Color nameScoreColorDead = 0xffee0000;

	const Color comboAccuracyColor = 0xff5d9ca1;
	const Color comboAccuracyColorHighlight = 0xff99fafe;
	const Color comboAccuracyColorTop = 0xff84dbe0;

	const Color textShadowColor = 0x66000000;

	const bool drawTextShadow = (m_osu_background_dim_ref->getFloat() < 0.7f);

	const float scale = osu_hud_scoreboard_scale.getFloat() * osu_hud_scale.getFloat();
	const float height = m_osu->getScreenHeight() * 0.07f * scale;
	const float width = height*2.6f; // was 2.75f
	const float margin = height*0.1f;
	const float padding = height*0.05f;

	const float minStartPosY = m_osu->getScreenHeight() - (scoreEntries.size()*height + (scoreEntries.size()-1)*margin);

	const float startPosX = 0;
	const float startPosY = clamp<float>(m_osu->getScreenHeight()/2 - (scoreEntries.size()*height + (scoreEntries.size()-1)*margin)/2 + osu_hud_scoreboard_offset_y_percent.getFloat()*m_osu->getScreenHeight(), 0.0f, minStartPosY);
	for (int i=0; i<scoreEntries.size(); i++)
	{
		const float x = startPosX;
		const float y = startPosY + i*(height + margin);

		if (m_osu->isInVRDraw())
		{
			g->setBlending(false);
		}

		// draw background
		g->setColor((scoreEntries[i].missingBeatmap ? backgroundColorMissingBeatmap : (scoreEntries[i].dead ? backgroundColorDead : (scoreEntries[i].highlight ? backgroundColorHighlight : (i == 0 ? backgroundColorTop : backgroundColor)))));
		if (useMenuButtonBackground && !m_osu->isInVRDraw()) // NOTE: in vr you would see the other 3/4ths of menu-button-background sticking out left, just fallback to fillRect for now
		{
			const float backgroundScale = 0.62f + 0.005f;
			backgroundImage->draw(g, Vector2(x + (backgroundImage->getSizeBase().x/2)*backgroundScale*scale - (470*oScale)*backgroundScale*scale, y + height/2), backgroundScale*scale);
		}
		else
			g->fillRect(x, y, width, height);

		if (m_osu->isInVRDraw())
		{
			g->setBlending(true);

			g->pushTransform();
			g->translate(0, 0, 0.1f);
		}

		// draw index
		const float indexScale = 0.5f;
		g->pushTransform();
		{
			const float scale = (height / indexFont->getHeight())*indexScale;

			UString indexString = UString::format("%i", (scoreEntries[i].index > -1 ? scoreEntries[i].index : i) + 1);
			const float stringWidth = indexFont->getStringWidth(indexString);

			g->scale(scale, scale);
			g->translate(x + width - stringWidth*scale - 2*padding, y + indexFont->getHeight()*scale);
			g->setColor((scoreEntries[i].highlight ? indexColorHighlight : indexColor));
			g->drawString(indexFont, indexString);
		}
		g->popTransform();

		// draw name
		const bool isDownloadingOrHasNoMap = (scoreEntries[i].downloadingBeatmap || scoreEntries[i].missingBeatmap);
		const float nameScale = 0.315f;
		if (!isDownloadingOrHasNoMap)
		{
			g->pushTransform();
			{
				const bool isInPlayModeAndAlsoNotInVR = m_osu->isInPlayMode() && !m_osu->isInVRMode();

				if (isInPlayModeAndAlsoNotInVR)
					g->pushClipRect(McRect(x, y, width - 2*padding, height));

				UString nameString = scoreEntries[i].name;
				if (scoreEntries[i].downloadingBeatmap)
					nameString.append(" [downloading]");
				else if (scoreEntries[i].missingBeatmap)
					nameString.append(" [no map]");

				const float scale = (height / nameFont->getHeight())*nameScale;

				g->scale(scale, scale);
				g->translate(x + padding, y + padding + nameFont->getHeight()*scale);
				if (drawTextShadow)
				{
					g->translate(1, 1);
					g->setColor(textShadowColor);
					g->drawString(nameFont, nameString);
					g->translate(-1, -1);
				}
				g->setColor((scoreEntries[i].dead || scoreEntries[i].missingBeatmap ? (scoreEntries[i].downloadingBeatmap ? nameScoreColorDownloading : nameScoreColorDead) : (scoreEntries[i].highlight ? nameScoreColorHighlight : (i == 0 ? nameScoreColorTop : nameScoreColor))));
				g->drawString(nameFont, nameString);

				if (isInPlayModeAndAlsoNotInVR)
					g->popClipRect();
			}
			g->popTransform();
		}

		// draw score
		const float scoreScale = 0.26f;
		g->pushTransform();
		{
			const float scale = (height / scoreFont->getHeight())*scoreScale;

			UString scoreString = UString::format("%llu", scoreEntries[i].score);

			g->scale(scale, scale);
			g->translate(x + padding*1.35f, y + height - 2*padding);
			if (drawTextShadow)
			{
				g->translate(1, 1);
				g->setColor(textShadowColor);
				g->drawString(scoreFont, scoreString);
				g->translate(-1, -1);
			}
			g->setColor((scoreEntries[i].dead ? nameScoreColorDead : (scoreEntries[i].highlight ? nameScoreColorHighlight : (i == 0 ? nameScoreColorTop : nameScoreColor))));
			g->drawString(scoreFont, scoreString);
		}
		g->popTransform();

		// draw combo
		const float comboScale = scoreScale;
		g->pushTransform();
		{
			const float scale = (height / comboFont->getHeight())*comboScale;

			UString comboString = UString::format("%ix", scoreEntries[i].combo);
			const float stringWidth = comboFont->getStringWidth(comboString);

			g->scale(scale, scale);
			g->translate(x + width - stringWidth*scale - padding*1.35f, y + height - 2*padding);
			if (drawTextShadow)
			{
				g->translate(1, 1);
				g->setColor(textShadowColor);
				g->drawString(comboFont, comboString);
				g->translate(-1, -1);
			}
			g->setColor((scoreEntries[i].highlight ? comboAccuracyColorHighlight : (i == 0 ? comboAccuracyColorTop : comboAccuracyColor)));
			g->drawString(scoreFont, comboString);
		}
		g->popTransform();

		// draw accuracy
		if (m_osu->isInMultiplayer() && (!m_osu->isInPlayMode() || m_osu_mp_win_condition_accuracy_ref->getBool()))
		{
			const float accScale = comboScale;
			g->pushTransform();
			{
				const float scale = (height / accFont->getHeight())*accScale;

				UString accString = UString::format("%.2f%%", scoreEntries[i].accuracy*100.0f);
				const float stringWidth = accFont->getStringWidth(accString);

				g->scale(scale, scale);
				g->translate(x + width - stringWidth*scale - padding*1.35f, y + accFont->getHeight()*scale + 2*padding);
				///if (drawTextShadow)
				{
					g->translate(1, 1);
					g->setColor(textShadowColor);
					g->drawString(accFont, accString);
					g->translate(-1, -1);
				}
				g->setColor((scoreEntries[i].highlight ? comboAccuracyColorHighlight : (i == 0 ? comboAccuracyColorTop : comboAccuracyColor)));
				g->drawString(accFont, accString);
			}
			g->popTransform();
		}



		// HACKHACK: code duplication
		if (isDownloadingOrHasNoMap)
		{
			g->pushTransform();
			{
				const bool isInPlayModeAndAlsoNotInVR = m_osu->isInPlayMode() && !m_osu->isInVRMode();

				if (isInPlayModeAndAlsoNotInVR)
					g->pushClipRect(McRect(x, y, width - 2*padding, height));

				UString nameString = scoreEntries[i].name;
				if (scoreEntries[i].downloadingBeatmap)
					nameString.append(" [downloading]");
				else if (scoreEntries[i].missingBeatmap)
					nameString.append(" [no map]");

				const float scale = (height / nameFont->getHeight())*nameScale;

				g->scale(scale, scale);
				g->translate(x + padding, y + padding + nameFont->getHeight()*scale);
				if (drawTextShadow)
				{
					g->translate(1, 1);
					g->setColor(textShadowColor);
					g->drawString(nameFont, nameString);
					g->translate(-1, -1);
				}
				g->setColor((scoreEntries[i].dead || scoreEntries[i].missingBeatmap ? (scoreEntries[i].downloadingBeatmap ? nameScoreColorDownloading : nameScoreColorDead) : (scoreEntries[i].highlight ? nameScoreColorHighlight : (i == 0 ? nameScoreColorTop : nameScoreColor))));
				g->drawString(nameFont, nameString);

				if (isInPlayModeAndAlsoNotInVR)
					g->popClipRect();
			}
			g->popTransform();
		}



		if (m_osu->isInVRDraw())
		{
			g->popTransform();
		}
	}
}

void OsuHUD::drawContinue(Graphics *g, Vector2 cursor, float hitcircleDiameter)
{
	Image *unpause = m_osu->getSkin()->getUnpause();
	const float unpauseScale = Osu::getImageScale(m_osu, unpause, 80);

	Image *cursorImage = m_osu->getSkin()->getDefaultCursor();
	const float cursorScale = Osu::getImageScaleToFitResolution(cursorImage, Vector2(hitcircleDiameter, hitcircleDiameter));

	// bleh
	if (cursor.x < cursorImage->getWidth() || cursor.y < cursorImage->getHeight() || cursor.x > m_osu->getScreenWidth()-cursorImage->getWidth() || cursor.y > m_osu->getScreenHeight()-cursorImage->getHeight())
		cursor = m_osu->getScreenSize()/2;

	// base
	g->setColor(COLOR(255, 255, 153, 51));
	g->pushTransform();
	{
		g->scale(cursorScale, cursorScale);
		g->translate(cursor.x, cursor.y);
		g->drawImage(cursorImage);
	}
	g->popTransform();

	// pulse animation
	const float cursorAnimPulsePercent = clamp<float>(fmod(engine->getTime(), 1.35f), 0.0f, 1.0f);
	g->setColor(COLOR((short)(255.0f*(1.0f-cursorAnimPulsePercent)), 255, 153, 51));
	g->pushTransform();
	{
		g->scale(cursorScale*(1.0f + cursorAnimPulsePercent), cursorScale*(1.0f + cursorAnimPulsePercent));
		g->translate(cursor.x, cursor.y);
		g->drawImage(cursorImage);
	}
	g->popTransform();

	// unpause click message
	g->setColor(0xffffffff);
	g->pushTransform();
	{
		g->scale(unpauseScale, unpauseScale);
		g->translate(cursor.x + 20 + (unpause->getWidth()/2)*unpauseScale, cursor.y + 20 + (unpause->getHeight()/2)*unpauseScale);
		g->drawImage(unpause);
	}
	g->popTransform();
}

void OsuHUD::drawHitErrorBar(Graphics *g, OsuBeatmapStandard *beatmapStd)
{
	if (osu_draw_hud.getBool() || !osu_hud_shift_tab_toggles_everything.getBool())
	{
		if (osu_draw_hiterrorbar.getBool() && (!beatmapStd->isSpinnerActive() || !osu_hud_hiterrorbar_hide_during_spinner.getBool()) && !beatmapStd->isLoading())
			drawHitErrorBar(g, OsuGameRules::getHitWindow300(beatmapStd), OsuGameRules::getHitWindow100(beatmapStd), OsuGameRules::getHitWindow50(beatmapStd), OsuGameRules::getHitWindowMiss(beatmapStd), m_osu->getScore()->getUnstableRate());
	}
}

void OsuHUD::drawHitErrorBar(Graphics *g, float hitWindow300, float hitWindow100, float hitWindow50, float hitWindowMiss, int ur)
{
	const Vector2 center = Vector2(m_osu->getScreenWidth()/2.0f, m_osu->getScreenHeight() - m_osu->getScreenHeight()*2.15f*osu_hud_hiterrorbar_height_percent.getFloat()*osu_hud_scale.getFloat()*osu_hud_hiterrorbar_scale.getFloat() - m_osu->getScreenHeight()*osu_hud_hiterrorbar_offset_percent.getFloat());

	if (osu_draw_hiterrorbar_bottom.getBool())
	{
		g->pushTransform();
		{
			const Vector2 localCenter = Vector2(center.x, center.y - (m_osu->getScreenHeight() * osu_hud_hiterrorbar_offset_bottom_percent.getFloat()));

			drawHitErrorBarInt2(g, localCenter, ur);
			g->translate(localCenter.x, localCenter.y);
			drawHitErrorBarInt(g, hitWindow300, hitWindow100, hitWindow50, hitWindowMiss);
		}
		g->popTransform();
	}

	if (osu_draw_hiterrorbar_top.getBool())
	{
		g->pushTransform();
		{
			const Vector2 localCenter = Vector2(center.x, m_osu->getScreenHeight() - center.y + (m_osu->getScreenHeight() * osu_hud_hiterrorbar_offset_top_percent.getFloat()));

			g->scale(1, -1);
			//drawHitErrorBarInt2(g, localCenter, ur);
			g->translate(localCenter.x, localCenter.y);
			drawHitErrorBarInt(g, hitWindow300, hitWindow100, hitWindow50, hitWindowMiss);
		}
		g->popTransform();
	}

	if (osu_draw_hiterrorbar_left.getBool())
	{
		g->pushTransform();
		{
			const Vector2 localCenter = Vector2(m_osu->getScreenHeight() - center.y + (m_osu->getScreenWidth() * osu_hud_hiterrorbar_offset_left_percent.getFloat()), m_osu->getScreenHeight()/2.0f);

			g->rotate(90);
			//drawHitErrorBarInt2(g, localCenter, ur);
			g->translate(localCenter.x, localCenter.y);
			drawHitErrorBarInt(g, hitWindow300, hitWindow100, hitWindow50, hitWindowMiss);
		}
		g->popTransform();
	}

	if (osu_draw_hiterrorbar_right.getBool())
	{
		g->pushTransform();
		{
			const Vector2 localCenter = Vector2(m_osu->getScreenWidth() - (m_osu->getScreenHeight() - center.y) - (m_osu->getScreenWidth() * osu_hud_hiterrorbar_offset_right_percent.getFloat()), m_osu->getScreenHeight()/2.0f);

			g->scale(-1, 1);
			g->rotate(-90);
			//drawHitErrorBarInt2(g, localCenter, ur);
			g->translate(localCenter.x, localCenter.y);
			drawHitErrorBarInt(g, hitWindow300, hitWindow100, hitWindow50, hitWindowMiss);
		}
		g->popTransform();
	}
}

void OsuHUD::drawHitErrorBarInt(Graphics *g, float hitWindow300, float hitWindow100, float hitWindow50, float hitWindowMiss)
{
	const float alpha = osu_hud_hiterrorbar_alpha.getFloat();
	if (alpha <= 0.0f) return;

	const float alphaEntry = alpha * osu_hud_hiterrorbar_entry_alpha.getFloat();
	const int alphaCenterlineInt = clamp<int>((int)(alpha * osu_hud_hiterrorbar_centerline_alpha.getFloat() * 255.0f), 0, 255);
	const int alphaBarInt = clamp<int>((int)(alpha * osu_hud_hiterrorbar_bar_alpha.getFloat() * 255.0f), 0, 255);

	const Color color300 = COLOR(alphaBarInt, clamp<int>(osu_hud_hiterrorbar_entry_300_r.getInt(), 0, 255), clamp<int>(osu_hud_hiterrorbar_entry_300_g.getInt(), 0, 255), clamp<int>(osu_hud_hiterrorbar_entry_300_b.getInt(), 0, 255));
	const Color color100 = COLOR(alphaBarInt, clamp<int>(osu_hud_hiterrorbar_entry_100_r.getInt(), 0, 255), clamp<int>(osu_hud_hiterrorbar_entry_100_g.getInt(), 0, 255), clamp<int>(osu_hud_hiterrorbar_entry_100_b.getInt(), 0, 255));
	const Color color50 = COLOR(alphaBarInt, clamp<int>(osu_hud_hiterrorbar_entry_50_r.getInt(), 0, 255), clamp<int>(osu_hud_hiterrorbar_entry_50_g.getInt(), 0, 255), clamp<int>(osu_hud_hiterrorbar_entry_50_b.getInt(), 0, 255));
	const Color colorMiss = COLOR(alphaBarInt, clamp<int>(osu_hud_hiterrorbar_entry_miss_r.getInt(), 0, 255), clamp<int>(osu_hud_hiterrorbar_entry_miss_g.getInt(), 0, 255), clamp<int>(osu_hud_hiterrorbar_entry_miss_b.getInt(), 0, 255));

	Vector2 size = Vector2(m_osu->getScreenWidth()*osu_hud_hiterrorbar_width_percent.getFloat(), m_osu->getScreenHeight()*osu_hud_hiterrorbar_height_percent.getFloat())*osu_hud_scale.getFloat()*osu_hud_hiterrorbar_scale.getFloat();
	if (osu_hud_hiterrorbar_showmisswindow.getBool())
		size = Vector2(m_osu->getScreenWidth()*osu_hud_hiterrorbar_width_percent_with_misswindow.getFloat(), m_osu->getScreenHeight()*osu_hud_hiterrorbar_height_percent.getFloat())*osu_hud_scale.getFloat()*osu_hud_hiterrorbar_scale.getFloat();

	const Vector2 center = Vector2(0, 0); // NOTE: moved to drawHitErrorBar()

	const float entryHeight = size.y*osu_hud_hiterrorbar_bar_height_scale.getFloat();
	const float entryWidth = size.y*osu_hud_hiterrorbar_bar_width_scale.getFloat();

	float totalHitWindowLength = hitWindow50;
	if (osu_hud_hiterrorbar_showmisswindow.getBool())
		totalHitWindowLength = hitWindowMiss;

	const float percent50 = hitWindow50 / totalHitWindowLength;
	const float percent100 = hitWindow100 / totalHitWindowLength;
	const float percent300 = hitWindow300 / totalHitWindowLength;

	// draw background bar with color indicators for 300s, 100s and 50s (and the miss window)
	if (alphaBarInt > 0)
	{
		const bool half = OsuGameRules::osu_mod_halfwindow.getBool();
		const bool halfAllow300s = OsuGameRules::osu_mod_halfwindow_allow_300s.getBool();

		if (osu_hud_hiterrorbar_showmisswindow.getBool())
		{
			g->setColor(colorMiss);
			g->fillRect(center.x - size.x/2.0f, center.y - size.y/2.0f, size.x, size.y);
		}

		if (!OsuGameRules::osu_mod_no100s.getBool() && !OsuGameRules::osu_mod_no50s.getBool())
		{
			g->setColor(color50);
			g->fillRect(center.x - size.x*percent50/2.0f, center.y - size.y/2.0f, size.x*percent50 * (half ? 0.5f : 1.0f), size.y);
		}

		if (!OsuGameRules::osu_mod_ming3012.getBool() && !OsuGameRules::osu_mod_no100s.getBool())
		{
			g->setColor(color100);
			g->fillRect(center.x - size.x*percent100/2.0f, center.y - size.y/2.0f, size.x*percent100 * (half ? 0.5f : 1.0f), size.y);
		}

		g->setColor(color300);
		g->fillRect(center.x - size.x*percent300/2.0f, center.y - size.y/2.0f, size.x*percent300 * (half && !halfAllow300s ? 0.5f : 1.0f), size.y);
	}

	// draw hit errors
	{
		if (osu_hud_hiterrorbar_entry_additive.getBool())
			g->setBlendMode(Graphics::BLEND_MODE::BLEND_MODE_ADDITIVE);

		const bool modMing3012 = OsuGameRules::osu_mod_ming3012.getBool();
		const float hitFadeDuration = osu_hud_hiterrorbar_entry_hit_fade_time.getFloat();
		const float missFadeDuration = osu_hud_hiterrorbar_entry_miss_fade_time.getFloat();
		for (int i=m_hiterrors.size()-1; i>=0; i--)
		{
			const float percent = clamp<float>((float)m_hiterrors[i].delta / (float)totalHitWindowLength, -5.0f, 5.0f);
			float fade = clamp<float>((m_hiterrors[i].time - engine->getTime()) / (m_hiterrors[i].miss || m_hiterrors[i].misaim ? missFadeDuration : hitFadeDuration), 0.0f, 1.0f);
			fade *= fade; // quad out

			Color barColor;
			{
				if (m_hiterrors[i].miss || m_hiterrors[i].misaim)
					barColor = colorMiss;
				else
					barColor = (std::abs(percent) <= percent300 ? color300 : (std::abs(percent) <= percent100 && !modMing3012 ? color100 : color50));
			}

			g->setColor(barColor);
			g->setAlpha(alphaEntry * fade);

			float missHeightMultiplier = 1.0f;
			if (m_hiterrors[i].miss)
				missHeightMultiplier = osu_hud_hiterrorbar_entry_miss_height_multiplier.getFloat();
			if (m_hiterrors[i].misaim)
				missHeightMultiplier = osu_hud_hiterrorbar_entry_misaim_height_multiplier.getFloat();

			//Color leftColor = COLOR((int)((255/2) * alphaEntry * fade), COLOR_GET_Ri(barColor), COLOR_GET_Gi(barColor), COLOR_GET_Bi(barColor));
			//Color centerColor = COLOR((int)(COLOR_GET_Ai(barColor) * alphaEntry * fade), COLOR_GET_Ri(barColor), COLOR_GET_Gi(barColor), COLOR_GET_Bi(barColor));
			//Color rightColor = leftColor;

			g->fillRect(center.x - (entryWidth/2.0f) + percent*(size.x/2.0f), center.y - (entryHeight*missHeightMultiplier)/2.0f, entryWidth, (entryHeight*missHeightMultiplier));
			//g->fillGradient((int)(center.x - (entryWidth/2.0f) + percent*(size.x/2.0f)), center.y - (entryHeight*missHeightMultiplier)/2.0f, (int)(entryWidth/2.0f), (entryHeight*missHeightMultiplier), leftColor, centerColor, leftColor, centerColor);
			//g->fillGradient((int)(center.x - (entryWidth/2.0f/2.0f) + percent*(size.x/2.0f)), center.y - (entryHeight*missHeightMultiplier)/2.0f, (int)(entryWidth/2.0f), (entryHeight*missHeightMultiplier), centerColor, rightColor, centerColor, rightColor);
		}

		if (osu_hud_hiterrorbar_entry_additive.getBool())
			g->setBlendMode(Graphics::BLEND_MODE::BLEND_MODE_ALPHA);
	}

	// white center line
	if (alphaCenterlineInt > 0)
	{
		g->setColor(COLOR(alphaCenterlineInt, clamp<int>(osu_hud_hiterrorbar_centerline_r.getInt(), 0, 255), clamp<int>(osu_hud_hiterrorbar_centerline_g.getInt(), 0, 255), clamp<int>(osu_hud_hiterrorbar_centerline_b.getInt(), 0, 255)));
		g->fillRect(center.x - entryWidth/2.0f/2.0f, center.y - entryHeight/2.0f, entryWidth/2.0f, entryHeight);
	}
}

void OsuHUD::drawHitErrorBarInt2(Graphics *g, Vector2 center, int ur)
{
	const float alpha = osu_hud_hiterrorbar_alpha.getFloat() * osu_hud_hiterrorbar_ur_alpha.getFloat();
	if (alpha <= 0.0f) return;

	const float dpiScale = Osu::getUIScale(m_osu);

	const float hitErrorBarSizeY = m_osu->getScreenHeight()*osu_hud_hiterrorbar_height_percent.getFloat()*osu_hud_scale.getFloat()*osu_hud_hiterrorbar_scale.getFloat();
	const float entryHeight = hitErrorBarSizeY*osu_hud_hiterrorbar_bar_height_scale.getFloat();

	if (osu_draw_hiterrorbar_ur.getBool())
	{
		g->pushTransform();
		{
			UString urText = UString::format("%i UR", ur);
			McFont *urTextFont = m_osu->getSongBrowserFont();

			const float hitErrorBarScale = osu_hud_scale.getFloat() * osu_hud_hiterrorbar_scale.getFloat();
			const float urTextScale = hitErrorBarScale * osu_hud_hiterrorbar_ur_scale.getFloat() * 0.5f;
			const float urTextWidth = urTextFont->getStringWidth(urText) * urTextScale;
			const float urTextHeight = urTextFont->getHeight() * hitErrorBarScale;

			g->scale(urTextScale, urTextScale);
			g->translate((int)(center.x + (-urTextWidth/2.0f) + (urTextHeight)*(osu_hud_hiterrorbar_ur_offset_x_percent.getFloat())*dpiScale) + 1, (int)(center.y + (urTextHeight)*(osu_hud_hiterrorbar_ur_offset_y_percent.getFloat())*dpiScale - entryHeight/1.25f) + 1);

			// shadow
			g->setColor(0xff000000);
			g->setAlpha(alpha);
			g->drawString(urTextFont, urText);

			g->translate(-1, -1);

			// text
			g->setColor(0xffffffff);
			g->setAlpha(alpha);
			g->drawString(urTextFont, urText);
		}
		g->popTransform();
	}
}

void OsuHUD::drawProgressBar(Graphics *g, float percent, bool waiting)
{
	if (!osu_draw_accuracy.getBool())
		m_fAccuracyXOffset = m_osu->getScreenWidth();

	const float num_segments = 15*8;
	const int offset = 20;
	const float radius = m_osu->getUIScale(m_osu, 10.5f) * osu_hud_scale.getFloat() * osu_hud_progressbar_scale.getFloat();
	const float circularMetreScale = ((2*radius)/m_osu->getSkin()->getCircularmetre()->getWidth()) * 1.3f; // hardcoded 1.3 multiplier?!
	const float actualCircularMetreScale = ((2*radius)/m_osu->getSkin()->getCircularmetre()->getWidth());
	Vector2 center = Vector2(m_fAccuracyXOffset - radius - offset, m_fAccuracyYOffset);

	// clamp to top edge of screen
	if (center.y - (m_osu->getSkin()->getCircularmetre()->getHeight()*actualCircularMetreScale + 5)/2.0f < 0)
		center.y += std::abs(center.y - (m_osu->getSkin()->getCircularmetre()->getHeight()*actualCircularMetreScale + 5)/2.0f);

	// clamp to bottom edge of score numbers
	if (osu_draw_score.getBool() && center.y-radius < m_fScoreHeight)
		center.y = m_fScoreHeight + radius;

	const float theta = 2 * PI / float(num_segments);
	const float s = sinf(theta); // precalculate the sine and cosine
	const float c = cosf(theta);
	float t;
	float x = 0;
	float y = -radius; // we start at the top

	if (waiting)
		g->setColor(0xaa00f200);
	else
		g->setColor(0xaaf2f2f2);

	{
		static VertexArrayObject vao;
		vao.empty();

		Vector2 prevVertex;
		for (int i=0; i<num_segments+1; i++)
		{
			float curPercent = (i*(360.0f / num_segments)) / 360.0f;
			if (curPercent > percent)
				break;

			// build current vertex
			Vector2 curVertex = Vector2(x + center.x, y + center.y);

			// add vertex, triangle strip style (counter clockwise)
			if (i > 0)
			{
				vao.addVertex(curVertex);
				vao.addVertex(prevVertex);
				vao.addVertex(center);
			}

			// apply the rotation
			t = x;
			x = c * x - s * y;
			y = s * t + c * y;

			// save
			prevVertex = curVertex;
		}

		// draw it
		g->drawVAO(&vao);
	}

	// draw circularmetre
	g->setColor(0xffffffff);
	g->pushTransform();
	{
		g->scale(circularMetreScale, circularMetreScale);
		g->translate(center.x, center.y, 0.65f);
		g->drawImage(m_osu->getSkin()->getCircularmetre());
	}
	g->popTransform();
}

void OsuHUD::drawProgressBarVR(Graphics *g, Matrix4 &mvp, OsuVR *vr, float percent, bool waiting)
{
	if (!osu_draw_accuracy.getBool())
		m_fAccuracyXOffset = m_osu->getScreenWidth();

	const float num_segments = 15*8;
	const int offset = 20;
	const float radius = m_osu->getUIScale(m_osu, 10.5f) * osu_hud_scale.getFloat() * osu_hud_progressbar_scale.getFloat();
	const float circularMetreScale = ((2*radius)/m_osu->getSkin()->getCircularmetre()->getWidth()) * 1.3f; // hardcoded 1.3 multiplier?!
	const float actualCircularMetreScale = ((2*radius)/m_osu->getSkin()->getCircularmetre()->getWidth());
	Vector2 center = Vector2(m_fAccuracyXOffset - radius - offset, m_fAccuracyYOffset);

	// clamp to top edge of screen
	if (center.y - (m_osu->getSkin()->getCircularmetre()->getHeight()*actualCircularMetreScale + 5)/2.0f < 0)
		center.y += std::abs(center.y - (m_osu->getSkin()->getCircularmetre()->getHeight()*actualCircularMetreScale + 5)/2.0f);

	// clamp to bottom edge of score numbers
	if (osu_draw_score.getBool() && center.y-radius < m_fScoreHeight)
		center.y = m_fScoreHeight + radius;

	const float theta = 2 * PI / float(num_segments);
	const float s = sinf(theta); // precalculate the sine and cosine
	const float c = cosf(theta);
	float t;
	float x = 0;
	float y = -radius; // we start at the top

	if (waiting)
		g->setColor(0xaa00f200);
	else
		g->setColor(0xaaf2f2f2);

	{
		static VertexArrayObject vao;
		vao.empty();

		Vector2 prevVertex;
		for (int i=0; i<num_segments+1; i++)
		{
			float curPercent = (i*(360.0f / num_segments)) / 360.0f;
			if (curPercent > percent)
				break;

			// build current vertex
			Vector2 curVertex = Vector2(x + center.x, y + center.y);

			// add vertex, triangle strip style (counter clockwise)
			if (i > 0)
			{
				vao.addVertex(curVertex);
				vao.addVertex(prevVertex);
				vao.addVertex(center);
			}

			// apply the rotation
			t = x;
			x = c * x - s * y;
			y = s * t + c * y;

			// save
			prevVertex = curVertex;
		}

		// draw it
		g->setAntialiasing(true);
		vr->getShaderUntexturedLegacyGeneric()->enable();
		vr->getShaderUntexturedLegacyGeneric()->setUniformMatrix4fv("matrix", mvp);
		{
			g->drawVAO(&vao);
		}
		vr->getShaderUntexturedLegacyGeneric()->disable();
		g->setAntialiasing(false);
	}

	// draw circularmetre
	vr->getShaderTexturedLegacyGeneric()->enable();
	vr->getShaderTexturedLegacyGeneric()->setUniformMatrix4fv("matrix", mvp);
	{
		g->setColor(0xffffffff);
		g->pushTransform();
		{
			g->scale(circularMetreScale, circularMetreScale);
			g->translate(center.x, center.y, m_osu->isInVRMode() ? 0.65f : 0.0f);
			g->drawImage(m_osu->getSkin()->getCircularmetre());
		}
		g->popTransform();
	}
}

void OsuHUD::drawStatistics(Graphics *g, int misses, int sliderbreaks, int maxPossibleCombo, float liveStars,  float totalStars, int bpm, float ar, float cs, float od, float hp, int nps, int nd, int ur, float pp, float ppfc, float hitWindow300, int hitdeltaMin, int hitdeltaMax)
{
	g->pushTransform();
	{
		const float offsetScale = Osu::getImageScale(m_osu, Vector2(1.0f, 1.0f), 1.0f);

		g->scale(osu_hud_statistics_scale.getFloat()*osu_hud_scale.getFloat(), osu_hud_statistics_scale.getFloat()*osu_hud_scale.getFloat());
		g->translate(osu_hud_statistics_offset_x.getInt()/* * offsetScale*/, (int)((m_osu->getTitleFont()->getHeight())*osu_hud_scale.getFloat()*osu_hud_statistics_scale.getFloat()) + (osu_hud_statistics_offset_y.getInt() * offsetScale));

		const int yDelta = (int)((m_osu->getTitleFont()->getHeight() + 10)*osu_hud_scale.getFloat()*osu_hud_statistics_scale.getFloat()*osu_hud_statistics_spacing_scale.getFloat());
		if (osu_draw_statistics_pp.getBool())
		{
			g->translate(osu_hud_statistics_pp_offset_x.getInt(), osu_hud_statistics_pp_offset_y.getInt());
			drawStatisticText(g, (osu_hud_statistics_pp_decimal_places.getInt() < 1 ? UString::format("%ipp", (int)std::round(pp)) : (osu_hud_statistics_pp_decimal_places.getInt() > 1 ? UString::format("%.2fpp", pp) : UString::format("%.1fpp", pp))));
			g->translate(-osu_hud_statistics_pp_offset_x.getInt(), yDelta - osu_hud_statistics_pp_offset_y.getInt());
		}
		if (osu_draw_statistics_perfectpp.getBool())
		{
			g->translate(osu_hud_statistics_perfectpp_offset_x.getInt(), osu_hud_statistics_perfectpp_offset_y.getInt());
			drawStatisticText(g, (osu_hud_statistics_pp_decimal_places.getInt() < 1 ? UString::format("SS: %ipp", (int)std::round(ppfc)) : (osu_hud_statistics_pp_decimal_places.getInt() > 1 ? UString::format("SS: %.2fpp", ppfc) : UString::format("SS: %.1fpp", ppfc))));
			g->translate(-osu_hud_statistics_perfectpp_offset_x.getInt(), yDelta - osu_hud_statistics_perfectpp_offset_y.getInt());
		}
		if (osu_draw_statistics_misses.getBool())
		{
			g->translate(osu_hud_statistics_misses_offset_x.getInt(), osu_hud_statistics_misses_offset_y.getInt());
			drawStatisticText(g, UString::format("Miss: %i", misses));
			g->translate(-osu_hud_statistics_misses_offset_x.getInt(), yDelta - osu_hud_statistics_misses_offset_y.getInt());
		}
		if (osu_draw_statistics_sliderbreaks.getBool())
		{
			g->translate(osu_hud_statistics_sliderbreaks_offset_x.getInt(), osu_hud_statistics_sliderbreaks_offset_y.getInt());
			drawStatisticText(g, UString::format("SBrk: %i", sliderbreaks));
			g->translate(-osu_hud_statistics_sliderbreaks_offset_x.getInt(), yDelta - osu_hud_statistics_sliderbreaks_offset_y.getInt());
		}
		if (osu_draw_statistics_maxpossiblecombo.getBool())
		{
			g->translate(osu_hud_statistics_maxpossiblecombo_offset_x.getInt(), osu_hud_statistics_maxpossiblecombo_offset_y.getInt());
			drawStatisticText(g, UString::format("FC: %ix", maxPossibleCombo));
			g->translate(-osu_hud_statistics_maxpossiblecombo_offset_x.getInt(), yDelta - osu_hud_statistics_maxpossiblecombo_offset_y.getInt());
		}
		if (osu_draw_statistics_livestars.getBool())
		{
			g->translate(osu_hud_statistics_livestars_offset_x.getInt(), osu_hud_statistics_livestars_offset_y.getInt());
			drawStatisticText(g, UString::format("%.3g***", liveStars));
			g->translate(-osu_hud_statistics_livestars_offset_x.getInt(), yDelta - osu_hud_statistics_livestars_offset_y.getInt());
		}
		if (osu_draw_statistics_totalstars.getBool())
		{
			g->translate(osu_hud_statistics_totalstars_offset_x.getInt(), osu_hud_statistics_totalstars_offset_y.getInt());
			drawStatisticText(g, UString::format("%.3g*", totalStars));
			g->translate(-osu_hud_statistics_totalstars_offset_x.getInt(), yDelta - osu_hud_statistics_totalstars_offset_y.getInt());
		}
		if (osu_draw_statistics_bpm.getBool())
		{
			g->translate(osu_hud_statistics_bpm_offset_x.getInt(), osu_hud_statistics_bpm_offset_y.getInt());
			drawStatisticText(g, UString::format("BPM: %i", bpm));
			g->translate(-osu_hud_statistics_bpm_offset_x.getInt(), yDelta - osu_hud_statistics_bpm_offset_y.getInt());
		}
		if (osu_draw_statistics_ar.getBool())
		{
			ar = std::round(ar * 100.0f) / 100.0f;
			g->translate(osu_hud_statistics_ar_offset_x.getInt(), osu_hud_statistics_ar_offset_y.getInt());
			drawStatisticText(g, UString::format("AR: %g", ar));
			g->translate(-osu_hud_statistics_ar_offset_x.getInt(), yDelta - osu_hud_statistics_ar_offset_y.getInt());
		}
		if (osu_draw_statistics_cs.getBool())
		{
			cs = std::round(cs * 100.0f) / 100.0f;
			g->translate(osu_hud_statistics_cs_offset_x.getInt(), osu_hud_statistics_cs_offset_y.getInt());
			drawStatisticText(g, UString::format("CS: %g", cs));
			g->translate(-osu_hud_statistics_cs_offset_x.getInt(), yDelta - osu_hud_statistics_cs_offset_y.getInt());
		}
		if (osu_draw_statistics_od.getBool())
		{
			od = std::round(od * 100.0f) / 100.0f;
			g->translate(osu_hud_statistics_od_offset_x.getInt(), osu_hud_statistics_od_offset_y.getInt());
			drawStatisticText(g, UString::format("OD: %g", od));
			g->translate(-osu_hud_statistics_od_offset_x.getInt(), yDelta - osu_hud_statistics_od_offset_y.getInt());
		}
		if (osu_draw_statistics_hp.getBool())
		{
			hp = std::round(hp * 100.0f) / 100.0f;
			g->translate(osu_hud_statistics_hp_offset_x.getInt(), osu_hud_statistics_hp_offset_y.getInt());
			drawStatisticText(g, UString::format("HP: %g", hp));
			g->translate(-osu_hud_statistics_hp_offset_x.getInt(), yDelta - osu_hud_statistics_hp_offset_y.getInt());
		}
		if (osu_draw_statistics_hitwindow300.getBool())
		{
			g->translate(osu_hud_statistics_hitwindow300_offset_x.getInt(), osu_hud_statistics_hitwindow300_offset_y.getInt());
			drawStatisticText(g, UString::format("300: +-%ims", (int)hitWindow300));
			g->translate(-osu_hud_statistics_hitwindow300_offset_x.getInt(), yDelta - osu_hud_statistics_hitwindow300_offset_y.getInt());
		}
		if (osu_draw_statistics_nps.getBool())
		{
			g->translate(osu_hud_statistics_nps_offset_x.getInt(), osu_hud_statistics_nps_offset_y.getInt());
			drawStatisticText(g, UString::format("NPS: %i", nps));
			g->translate(-osu_hud_statistics_nps_offset_x.getInt(), yDelta - osu_hud_statistics_nps_offset_y.getInt());
		}
		if (osu_draw_statistics_nd.getBool())
		{
			g->translate(osu_hud_statistics_nd_offset_x.getInt(), osu_hud_statistics_nd_offset_y.getInt());
			drawStatisticText(g, UString::format("ND: %i", nd));
			g->translate(-osu_hud_statistics_nd_offset_x.getInt(), yDelta - osu_hud_statistics_nd_offset_y.getInt());
		}
		if (osu_draw_statistics_ur.getBool())
		{
			g->translate(osu_hud_statistics_ur_offset_x.getInt(), osu_hud_statistics_ur_offset_y.getInt());
			drawStatisticText(g, UString::format("UR: %i", ur));
			g->translate(-osu_hud_statistics_ur_offset_x.getInt(), yDelta - osu_hud_statistics_ur_offset_y.getInt());
		}
		if (osu_draw_statistics_hitdelta.getBool())
		{
			g->translate(osu_hud_statistics_hitdelta_offset_x.getInt(), osu_hud_statistics_hitdelta_offset_y.getInt());
			drawStatisticText(g, UString::format("-%ims +%ims", std::abs(hitdeltaMin), hitdeltaMax));
			g->translate(-osu_hud_statistics_hitdelta_offset_x.getInt(), yDelta - osu_hud_statistics_hitdelta_offset_y.getInt());
		}
	}
	g->popTransform();
}

void OsuHUD::drawStatisticText(Graphics *g, const UString text)
{
	if (text.length() < 1) return;

	g->pushTransform();
	{
		g->setColor(0xff000000);
		g->translate(0, 0, 0.25f);
		g->drawString(m_osu->getTitleFont(), text);

		g->setColor(0xffffffff);
		g->translate(-1, -1, 0.325f);
		g->drawString(m_osu->getTitleFont(), text);
	}
	g->popTransform();
}

void OsuHUD::drawTargetHeatmap(Graphics *g, float hitcircleDiameter)
{
	const Vector2 center = Vector2((int)(hitcircleDiameter/2.0f + 5.0f), (int)(hitcircleDiameter/2.0f + 5.0f));

	const int brightnessSub = 0;
	const Color color300 = COLOR(255, 0, 255-brightnessSub, 255-brightnessSub);
	const Color color100 = COLOR(255, 0, 255-brightnessSub, 0);
	const Color color50 = COLOR(255, 255-brightnessSub, 165-brightnessSub, 0);
	const Color colorMiss = COLOR(255, 255-brightnessSub, 0, 0);

	OsuCircle::drawCircle(g, m_osu->getSkin(), center, hitcircleDiameter, COLOR(255, 50, 50, 50));

	const int size = hitcircleDiameter*0.075f;
	for (int i=0; i<m_targets.size(); i++)
	{
		const float delta = m_targets[i].delta;

		const float overlap = 0.15f;
		Color color;
		if (delta < m_osu_mod_target_300_percent_ref->getFloat()-overlap)
			color = color300;
		else if (delta < m_osu_mod_target_300_percent_ref->getFloat()+overlap)
		{
			const float factor300 = (m_osu_mod_target_300_percent_ref->getFloat() + overlap - delta) / (2.0f*overlap);
			const float factor100 = 1.0f - factor300;
			color = COLORf(1.0f, COLOR_GET_Rf(color300)*factor300 + COLOR_GET_Rf(color100)*factor100, COLOR_GET_Gf(color300)*factor300 + COLOR_GET_Gf(color100)*factor100, COLOR_GET_Bf(color300)*factor300 + COLOR_GET_Bf(color100)*factor100);
		}
		else if (delta < m_osu_mod_target_100_percent_ref->getFloat()-overlap)
			color = color100;
		else if (delta < m_osu_mod_target_100_percent_ref->getFloat()+overlap)
		{
			const float factor100 = (m_osu_mod_target_100_percent_ref->getFloat() + overlap - delta) / (2.0f*overlap);
			const float factor50 = 1.0f - factor100;
			color = COLORf(1.0f, COLOR_GET_Rf(color100)*factor100 + COLOR_GET_Rf(color50)*factor50, COLOR_GET_Gf(color100)*factor100 + COLOR_GET_Gf(color50)*factor50, COLOR_GET_Bf(color100)*factor100 + COLOR_GET_Bf(color50)*factor50);
		}
		else if (delta < m_osu_mod_target_50_percent_ref->getFloat())
			color = color50;
		else
			color = colorMiss;

		g->setColor(color);
		g->setAlpha(clamp<float>((m_targets[i].time - engine->getTime())/3.5f, 0.0f, 1.0f));

		const float theta = deg2rad(m_targets[i].angle);
		const float cs = std::cos(theta);
		const float sn = std::sin(theta);

		Vector2 up = Vector2(-1, 0);
		Vector2 offset;
		offset.x = up.x * cs - up.y * sn;
		offset.y = up.x * sn + up.y * cs;
		offset.normalize();
		offset *= (delta*(hitcircleDiameter/2.0f));

		//g->fillRect(center.x-size/2 - offset.x, center.y-size/2 - offset.y, size, size);

		const float imageScale = m_osu->getImageScaleToFitResolution(m_osu->getSkin()->getCircleFull(), Vector2(size, size));
		g->pushTransform();
		{
			g->scale(imageScale, imageScale);
			g->translate(center.x - offset.x, center.y - offset.y);
			g->drawImage(m_osu->getSkin()->getCircleFull());
		}
		g->popTransform();
	}
}

void OsuHUD::drawScrubbingTimeline(Graphics *g, unsigned long beatmapTime, unsigned long beatmapLength, unsigned long beatmapLengthPlayable, unsigned long beatmapStartTimePlayable, float beatmapPercentFinishedPlayable, const std::vector<BREAK> &breaks)
{
	const float dpiScale = Osu::getUIScale(m_osu);

	const Vector2 cursorPos = engine->getMouse()->getPos();

	const Color grey = 0xffbbbbbb;
	const Color greyTransparent = 0xbbbbbbbb;
	const Color greyDark = 0xff777777;
	const Color green = 0xff00ff00;

	McFont *timeFont = m_osu->getSubTitleFont();

	const float breakHeight = 15 * dpiScale;

	const float currentTimeTopTextOffset = 7 * dpiScale;
	const float currentTimeLeftRightTextOffset = 5 * dpiScale;
	const float startAndEndTimeTextOffset = 5 * dpiScale + breakHeight;

	const unsigned long lengthFullMS = beatmapLength;
	const unsigned long lengthMS = beatmapLengthPlayable;
	const unsigned long startTimeMS = beatmapStartTimePlayable;
	const unsigned long endTimeMS = startTimeMS + lengthMS;
	const unsigned long currentTimeMS = beatmapTime;

	// draw strain graph
	if (osu_draw_scrubbing_timeline_strain_graph.getBool() && m_osu->getSongBrowser()->getDynamicStarCalculator()->isAsyncReady())
	{
		// this is still WIP

		// TODO: should use strains from beatmap, not songbrowser (because songbrowser doesn't update onModUpdate() while playing)
		const std::vector<double> &aimStrains = m_osu->getSongBrowser()->getDynamicStarCalculator()->getAimStrains();
		const std::vector<double> &speedStrains = m_osu->getSongBrowser()->getDynamicStarCalculator()->getSpeedStrains();
		const float speedMultiplier = m_osu->getSpeedMultiplier();

		if (aimStrains.size() > 0 && aimStrains.size() == speedStrains.size())
		{
			const float strainStepMS = 400.0f * speedMultiplier;

			// get highest strain values for normalization
			double highestAimStrain = 0.0;
			double highestSpeedStrain = 0.0;
			double highestStrain = 0.0;
			int highestStrainIndex = -1;
			for (int i=0; i<aimStrains.size(); i++)
			{
				const double aimStrain = aimStrains[i];
				const double speedStrain = speedStrains[i];
				const double strain = aimStrain + speedStrain;

				if (strain > highestStrain)
				{
					highestStrain = strain;
					highestStrainIndex = i;
				}
				if (aimStrain > highestAimStrain)
					highestAimStrain = aimStrain;
				if (speedStrain > highestSpeedStrain)
					highestSpeedStrain = speedStrain;
			}

			// draw strain bar graph
			if (highestAimStrain > 0.0 && highestSpeedStrain > 0.0 && highestStrain > 0.0)
			{
				const float msPerPixel = (float)lengthMS / (float)(m_osu->getScreenWidth() - (((float)startTimeMS / (float)endTimeMS))*m_osu->getScreenWidth());
				const float strainWidth = strainStepMS / msPerPixel;
				const float strainHeightMultiplier = osu_hud_scrubbing_timeline_strains_height.getFloat() * dpiScale;

				const float offsetX = ((float)startTimeMS / (float)strainStepMS) * strainWidth; // compensate for startTimeMS

				const float alpha = osu_hud_scrubbing_timeline_strains_alpha.getFloat();

				const Color aimStrainColor = COLORf(alpha, osu_hud_scrubbing_timeline_strains_aim_color_r.getInt() / 255.0f, osu_hud_scrubbing_timeline_strains_aim_color_g.getInt() / 255.0f, osu_hud_scrubbing_timeline_strains_aim_color_b.getInt() / 255.0f);
				const Color speedStrainColor = COLORf(alpha, osu_hud_scrubbing_timeline_strains_speed_color_r.getInt() / 255.0f, osu_hud_scrubbing_timeline_strains_speed_color_g.getInt() / 255.0f, osu_hud_scrubbing_timeline_strains_speed_color_b.getInt() / 255.0f);

				g->setDepthBuffer(true);
				for (int i=0; i<aimStrains.size(); i++)
				{
					const double aimStrain = (aimStrains[i]) / highestStrain;
					const double speedStrain = (speedStrains[i]) / highestStrain;
					//const double strain = (aimStrains[i] + speedStrains[i]) / highestStrain;

					const double aimStrainHeight = aimStrain * strainHeightMultiplier;
					const double speedStrainHeight = speedStrain * strainHeightMultiplier;
					//const double strainHeight = strain * strainHeightMultiplier;

					g->setColor(aimStrainColor);
					g->fillRect(i*strainWidth + offsetX, cursorPos.y - aimStrainHeight, std::max(1.0f, std::round(strainWidth + 0.5f)), aimStrainHeight);

					g->setColor(speedStrainColor);
					g->fillRect(i*strainWidth + offsetX, cursorPos.y - aimStrainHeight - speedStrainHeight, std::max(1.0f, std::round(strainWidth + 0.5f)), speedStrainHeight + 1);
				}
				g->setDepthBuffer(false);

				// highlight highest total strain value (+- section block)
				if (highestStrainIndex > -1)
				{
					const double aimStrain = (aimStrains[highestStrainIndex]) / highestStrain;
					const double speedStrain = (speedStrains[highestStrainIndex]) / highestStrain;
					//const double strain = (aimStrains[i] + speedStrains[i]) / highestStrain;

					const double aimStrainHeight = aimStrain * strainHeightMultiplier;
					const double speedStrainHeight = speedStrain * strainHeightMultiplier;
					//const double strainHeight = strain * strainHeightMultiplier;

					Vector2 topLeftCenter = Vector2(highestStrainIndex*strainWidth + offsetX + strainWidth/2.0f, cursorPos.y - aimStrainHeight - speedStrainHeight);

					const float margin = 5.0f * dpiScale;

					g->setColor(0xffffffff);
					g->setAlpha(alpha);
					g->drawRect(topLeftCenter.x - margin*strainWidth, topLeftCenter.y - margin*strainWidth, strainWidth*2*margin, aimStrainHeight + speedStrainHeight + 2*margin*strainWidth);
				}
			}
		}
	}

	// breaks
	g->setColor(greyTransparent);
	for (int i=0; i<breaks.size(); i++)
	{
		const int width = std::max((int)(m_osu->getScreenWidth() * clamp<float>(breaks[i].endPercent - breaks[i].startPercent, 0.0f, 1.0f)), 2);
		g->fillRect(m_osu->getScreenWidth() * breaks[i].startPercent, cursorPos.y + 1, width, breakHeight);
	}

	// line
	g->setColor(0xff000000);
	g->drawLine(0, cursorPos.y + 1, m_osu->getScreenWidth(), cursorPos.y + 1);
	g->setColor(grey);
	g->drawLine(0, cursorPos.y, m_osu->getScreenWidth(), cursorPos.y);

	// current time triangle
	Vector2 triangleTip = Vector2(m_osu->getScreenWidth()*beatmapPercentFinishedPlayable, cursorPos.y);
	g->pushTransform();
	{
		g->translate(triangleTip.x + 1, triangleTip.y - m_osu->getSkin()->getSeekTriangle()->getHeight()/2.0f + 1);
		g->setColor(0xff000000);
		g->drawImage(m_osu->getSkin()->getSeekTriangle());
		g->translate(-1, -1);
		g->setColor(green);
		g->drawImage(m_osu->getSkin()->getSeekTriangle());
	}
	g->popTransform();

	// current time text
	UString currentTimeText = UString::format("%i:%02i", (currentTimeMS/1000) / 60, (currentTimeMS/1000) % 60);
	g->pushTransform();
	{
		g->translate(clamp<float>(triangleTip.x - timeFont->getStringWidth(currentTimeText)/2.0f, currentTimeLeftRightTextOffset, m_osu->getScreenWidth() - timeFont->getStringWidth(currentTimeText) - currentTimeLeftRightTextOffset) + 1, triangleTip.y - m_osu->getSkin()->getSeekTriangle()->getHeight() - currentTimeTopTextOffset + 1);
		g->setColor(0xff000000);
		g->drawString(timeFont, currentTimeText);
		g->translate(-1, -1);
		g->setColor(green);
		g->drawString(timeFont, currentTimeText);
	}
	g->popTransform();

	// start time text
	UString startTimeText = UString::format("(%i:%02i)", (startTimeMS/1000) / 60, (startTimeMS/1000) % 60);
	g->pushTransform();
	{
		g->translate((int)(startAndEndTimeTextOffset + 1), (int)(triangleTip.y + startAndEndTimeTextOffset + timeFont->getHeight() + 1));
		g->setColor(0xff000000);
		g->drawString(timeFont, startTimeText);
		g->translate(-1, -1);
		g->setColor(greyDark);
		g->drawString(timeFont, startTimeText);
	}
	g->popTransform();

	// end time text
	UString endTimeText = UString::format("%i:%02i", (endTimeMS/1000) / 60, (endTimeMS/1000) % 60);
	g->pushTransform();
	{
		g->translate((int)(m_osu->getScreenWidth() - timeFont->getStringWidth(endTimeText) - startAndEndTimeTextOffset + 1), (int)(triangleTip.y + startAndEndTimeTextOffset + timeFont->getHeight() + 1));
		g->setColor(0xff000000);
		g->drawString(timeFont, endTimeText);
		g->translate(-1, -1);
		g->setColor(greyDark);
		g->drawString(timeFont, endTimeText);
	}
	g->popTransform();

	// quicksave time triangle & text
	if (m_osu->getQuickSaveTime() != 0.0f)
	{
		const float quickSaveTimeToPlayablePercent = clamp<float>(((lengthFullMS*m_osu->getQuickSaveTime())) / (float)endTimeMS, 0.0f, 1.0f);
		triangleTip = Vector2(m_osu->getScreenWidth()*quickSaveTimeToPlayablePercent, cursorPos.y);
		g->pushTransform();
		{
			g->rotate(180);
			g->translate(triangleTip.x + 1, triangleTip.y + m_osu->getSkin()->getSeekTriangle()->getHeight()/2.0f + 1);
			g->setColor(0xff000000);
			g->drawImage(m_osu->getSkin()->getSeekTriangle());
			g->translate(-1, -1);
			g->setColor(grey);
			g->drawImage(m_osu->getSkin()->getSeekTriangle());
		}
		g->popTransform();

		// end time text
		const unsigned long quickSaveTimeMS = lengthFullMS*m_osu->getQuickSaveTime();
		UString endTimeText = UString::format("%i:%02i", (quickSaveTimeMS/1000) / 60, (quickSaveTimeMS/1000) % 60);
		g->pushTransform();
		{
			g->translate((int)(clamp<float>(triangleTip.x - timeFont->getStringWidth(currentTimeText)/2.0f, currentTimeLeftRightTextOffset, m_osu->getScreenWidth() - timeFont->getStringWidth(currentTimeText) - currentTimeLeftRightTextOffset) + 1), (int)(triangleTip.y + startAndEndTimeTextOffset + timeFont->getHeight()*2.2f + 1 + currentTimeTopTextOffset*std::max(1.0f, getCursorScaleFactor()*osu_cursor_scale.getFloat())*osu_hud_scrubbing_timeline_hover_tooltip_offset_multiplier.getFloat()));
			g->setColor(0xff000000);
			g->drawString(timeFont, endTimeText);
			g->translate(-1, -1);
			g->setColor(grey);
			g->drawString(timeFont, endTimeText);
		}
		g->popTransform();
	}

	// current time hover text
	const unsigned long hoverTimeMS = clamp<float>((cursorPos.x / (float)m_osu->getScreenWidth()), 0.0f, 1.0f) * endTimeMS;
	UString hoverTimeText = UString::format("%i:%02i", (hoverTimeMS/1000) / 60, (hoverTimeMS/1000) % 60);
	triangleTip = Vector2(cursorPos.x, cursorPos.y);
	g->pushTransform();
	{
		g->translate((int)clamp<float>(triangleTip.x - timeFont->getStringWidth(currentTimeText)/2.0f, currentTimeLeftRightTextOffset, m_osu->getScreenWidth() - timeFont->getStringWidth(currentTimeText) - currentTimeLeftRightTextOffset) + 1, (int)(triangleTip.y - m_osu->getSkin()->getSeekTriangle()->getHeight() - timeFont->getHeight()*1.2f - currentTimeTopTextOffset*std::max(1.0f, getCursorScaleFactor()*osu_cursor_scale.getFloat())*osu_hud_scrubbing_timeline_hover_tooltip_offset_multiplier.getFloat()*2.0f - 1));
		g->setColor(0xff000000);
		g->drawString(timeFont, hoverTimeText);
		g->translate(-1, -1);
		g->setColor(0xff666666);
		g->drawString(timeFont, hoverTimeText);
	}
	g->popTransform();
}

void OsuHUD::drawInputOverlay(Graphics *g, int numK1, int numK2, int numM1, int numM2)
{
	OsuSkinImage *inputoverlayBackground = m_osu->getSkin()->getInputoverlayBackground();
	OsuSkinImage *inputoverlayKey = m_osu->getSkin()->getInputoverlayKey();

	const float scale = osu_hud_scale.getFloat() * osu_hud_inputoverlay_scale.getFloat();	// global scaler
	const float oScale = inputoverlayBackground->getResolutionScale() * 1.6f;				// for converting harcoded osu offset pixels to screen pixels
	const float offsetScale = Osu::getImageScale(m_osu, Vector2(1.0f, 1.0f), 1.0f);			// for scaling the x/y offset convars relative to screen size

	const float xStartOffset = osu_hud_inputoverlay_offset_x.getFloat()*offsetScale;
	const float yStartOffset = osu_hud_inputoverlay_offset_y.getFloat()*offsetScale;

	const float xStart = m_osu->getScreenWidth() - xStartOffset;
	const float yStart = m_osu->getScreenHeight()/2 - (40.0f*oScale)*scale + yStartOffset;

	// background
	{
		const float xScale = 1.05f + 0.001f;
		const float rot = 90.0f;

		const float xOffset = (inputoverlayBackground->getSize().y / 2);
		const float yOffset = (inputoverlayBackground->getSize().x / 2 ) * xScale;

		g->setColor(0xffffffff);
		g->pushTransform();
		{
			g->scale(xScale, 1.0f);
			g->rotate(rot);
			inputoverlayBackground->draw(g, Vector2(xStart - xOffset*scale + 1, yStart + yOffset*scale), scale);
		}
		g->popTransform();
	}

	// keys
	{
		const float textFontHeightPercent = 0.3f;
		const Color colorIdle = COLOR(255, 255, 255, 255);
		const Color colorKeyboard = COLOR(255, 255, 222, 0);
		const Color colorMouse = COLOR(255, 248, 0, 158);

		McFont *textFont = m_osu->getSongBrowserFont();
		McFont *textFontBold = m_osu->getSongBrowserFontBold();

		for (int i=0; i<4; i++)
		{
			textFont = m_osu->getSongBrowserFont(); // reset

			UString text;
			Color color = colorIdle;
			float animScale = 1.0f;
			float animColor = 0.0f;
			switch (i)
			{
			case 0:
				text = numK1 > 0 ? UString::format("%i", numK1) : UString("K1");
				color = colorKeyboard;
				animScale = m_fInputoverlayK1AnimScale;
				animColor = m_fInputoverlayK1AnimColor;
				if (numK1 > 0)
					textFont = textFontBold;
				break;
			case 1:
				text = numK2 > 0 ? UString::format("%i", numK2) : UString("K2");
				color = colorKeyboard;
				animScale = m_fInputoverlayK2AnimScale;
				animColor = m_fInputoverlayK2AnimColor;
				if (numK2 > 0)
					textFont = textFontBold;
				break;
			case 2:
				text = numM1 > 0 ? UString::format("%i", numM1) : UString("M1");
				color = colorMouse;
				animScale = m_fInputoverlayM1AnimScale;
				animColor = m_fInputoverlayM1AnimColor;
				if (numM1 > 0)
					textFont = textFontBold;
				break;
			case 3:
				text = numM2 > 0 ? UString::format("%i", numM2) : UString("M2");
				color = colorMouse;
				animScale = m_fInputoverlayM2AnimScale;
				animColor = m_fInputoverlayM2AnimColor;
				if (numM2 > 0)
					textFont = textFontBold;
				break;
			}

			// key
			const Vector2 pos = Vector2(xStart - (15.0f*oScale)*scale + 1, yStart + (19.0f*oScale + i*29.5f*oScale)*scale);
			g->setColor(COLORf(1.0f,
					(1.0f - animColor)*COLOR_GET_Rf(colorIdle) + animColor*COLOR_GET_Rf(color),
					(1.0f - animColor)*COLOR_GET_Gf(colorIdle) + animColor*COLOR_GET_Gf(color),
					(1.0f - animColor)*COLOR_GET_Bf(colorIdle) + animColor*COLOR_GET_Bf(color)));
			inputoverlayKey->draw(g, pos, scale*animScale);

			// text
			const float keyFontScale = (inputoverlayKey->getSizeBase().y * textFontHeightPercent) / textFont->getHeight();
			const float stringWidth = textFont->getStringWidth(text) * keyFontScale;
			const float stringHeight = textFont->getHeight() * keyFontScale;
			g->setColor(m_osu->getSkin()->getInputOverlayText());
			g->pushTransform();
			{
				g->scale(keyFontScale*scale*animScale, keyFontScale*scale*animScale);
				g->translate(pos.x - (stringWidth/2.0f)*scale*animScale, pos.y + (stringHeight/2.0f)*scale*animScale);
				g->drawString(textFont, text);
			}
			g->popTransform();
		}
	}
}

float OsuHUD::getCursorScaleFactor()
{
	// FUCK OSU hardcoded piece of shit code
	const float spriteRes = 768.0f;

	float mapScale = 1.0f;
	if (osu_automatic_cursor_size.getBool() && m_osu->isInPlayMode())
		mapScale = 1.0f - 0.7f * (float)(m_osu->getSelectedBeatmap()->getCS() - 4.0f) / 5.0f;

	return ((float)m_osu->getScreenHeight() / spriteRes) * mapScale;
}

float OsuHUD::getCursorTrailScaleFactor()
{
	return getCursorScaleFactor() * (m_osu->getSkin()->isCursorTrail2x() ? 0.5f : 1.0f);
}

float OsuHUD::getScoreScale()
{
	return m_osu->getImageScale(m_osu, m_osu->getSkin()->getScore0(), 13*1.5f) * osu_hud_scale.getFloat() * osu_hud_score_scale.getFloat();
}

void OsuHUD::onVolumeOverlaySizeChange(UString oldValue, UString newValue)
{
	updateLayout();
}

void OsuHUD::animateCombo()
{
	m_fComboAnim1 = 0.0f;
	m_fComboAnim2 = 1.0f;

	anim->moveLinear(&m_fComboAnim1, 2.0f, osu_combo_anim1_duration.getFloat(), true);
	anim->moveQuadOut(&m_fComboAnim2, 0.0f, osu_combo_anim2_duration.getFloat(), 0.0f, true);
}

void OsuHUD::addHitError(long delta, bool miss, bool misaim)
{
	// add entry
	{
		HITERROR h;

		h.delta = delta;
		h.time = engine->getTime() + (miss || misaim ? osu_hud_hiterrorbar_entry_miss_fade_time.getFloat() : osu_hud_hiterrorbar_entry_hit_fade_time.getFloat());
		h.miss = miss;
		h.misaim = misaim;

		m_hiterrors.push_back(h);
	}

	// remove old
	for (int i=0; i<m_hiterrors.size(); i++)
	{
		if (engine->getTime() > m_hiterrors[i].time)
		{
			m_hiterrors.erase(m_hiterrors.begin() + i);
			i--;
		}
	}

	if (m_hiterrors.size() > osu_hud_hiterrorbar_max_entries.getInt())
		m_hiterrors.erase(m_hiterrors.begin());
}

void OsuHUD::addTarget(float delta, float angle)
{
	TARGET t;
	t.time = engine->getTime() + 3.5f;
	t.delta = delta;
	t.angle = angle;

	m_targets.push_back(t);
}

void OsuHUD::animateInputoverlay(int key, bool down)
{
	if (!osu_draw_inputoverlay.getBool() || (!osu_draw_hud.getBool() && osu_hud_shift_tab_toggles_everything.getBool())) return;

	float *animScale = &m_fInputoverlayK1AnimScale;
	float *animColor = &m_fInputoverlayK1AnimColor;

	switch (key)
	{
	case 1:
		animScale = &m_fInputoverlayK1AnimScale;
		animColor = &m_fInputoverlayK1AnimColor;
		break;
	case 2:
		animScale = &m_fInputoverlayK2AnimScale;
		animColor = &m_fInputoverlayK2AnimColor;
		break;
	case 3:
		animScale = &m_fInputoverlayM1AnimScale;
		animColor = &m_fInputoverlayM1AnimColor;
		break;
	case 4:
		animScale = &m_fInputoverlayM2AnimScale;
		animColor = &m_fInputoverlayM2AnimColor;
		break;
	}

	if (down)
	{
		// scale
		*animScale = 1.0f;
		anim->moveQuadOut(animScale, osu_hud_inputoverlay_anim_scale_multiplier.getFloat(), osu_hud_inputoverlay_anim_scale_duration.getFloat(), true);

		// color
		*animColor = 1.0f;
		anim->deleteExistingAnimation(animColor);
	}
	else
	{
		// scale
		// NOTE: osu is running the keyup anim in parallel, but only allowing it to override once the keydown anim has finished, and with some weird speedup?
		const float remainingDuration = anim->getRemainingDuration(animScale);
		anim->moveQuadOut(animScale, 1.0f, osu_hud_inputoverlay_anim_scale_duration.getFloat() - std::min(remainingDuration*1.4f, osu_hud_inputoverlay_anim_scale_duration.getFloat()), remainingDuration);

		// color
		anim->moveLinear(animColor, 0.0f, osu_hud_inputoverlay_anim_color_duration.getFloat(), true);
	}
}

void OsuHUD::animateVolumeChange()
{
	const bool active = m_fVolumeChangeTime > engine->getTime();

	m_fVolumeChangeTime = engine->getTime() + osu_hud_volume_duration.getFloat() + 0.2f;

	if (!active)
	{
		m_fVolumeChangeFade = 0.0f;
		anim->moveQuadOut(&m_fVolumeChangeFade, 1.0f, 0.15f, true);
	}
	else
		anim->moveQuadOut(&m_fVolumeChangeFade, 1.0f, 0.1f * (1.0f - m_fVolumeChangeFade), true);

	anim->moveQuadOut(&m_fVolumeChangeFade, 0.0f, 0.25f, osu_hud_volume_duration.getFloat(), false);
	anim->moveQuadOut(&m_fLastVolume, m_osu_volume_master_ref->getFloat(), 0.15f, 0.0f, true);
}

void OsuHUD::addCursorRipple(Vector2 pos)
{
	if (!osu_draw_cursor_ripples.getBool()) return;

	CURSORRIPPLE ripple;
	ripple.pos = pos;
	ripple.time = engine->getTime() + osu_cursor_ripple_duration.getFloat();

	m_cursorRipples.push_back(ripple);
}

void OsuHUD::animateCursorExpand()
{
	m_fCursorExpandAnim = 1.0f;
	anim->moveQuadOut(&m_fCursorExpandAnim, osu_cursor_expand_scale_multiplier.getFloat(), osu_cursor_expand_duration.getFloat(), 0.0f, true);
}

void OsuHUD::animateCursorShrink()
{
	anim->moveQuadOut(&m_fCursorExpandAnim, 1.0f, osu_cursor_expand_duration.getFloat(), 0.0f, true);
}

void OsuHUD::animateKiBulge()
{
	m_fKiScaleAnim = 1.2f;
	anim->moveLinear(&m_fKiScaleAnim, 0.8f, 0.150f, true);
}

void OsuHUD::animateKiExplode()
{
	// TODO: scale + fadeout of extra ki image additive, duration = 0.120, quad out:
	// if additive: fade from 0.5 alpha to 0, scale from 1.0 to 2.0
	// if not additive: fade from 1.0 alpha to 0, scale from 1.0 to 1.6
}

void OsuHUD::addCursorTrailPosition(std::vector<CURSORTRAIL> &trail, Vector2 pos, bool empty)
{
	if (empty) return;
	if (pos.x < -m_osu->getScreenWidth() || pos.x > m_osu->getScreenWidth()*2 || pos.y < -m_osu->getScreenHeight() || pos.y > m_osu->getScreenHeight()*2) return; // fuck oob trails

	Image *trailImage = m_osu->getSkin()->getCursorTrail();

	const bool smoothCursorTrail = m_osu->getSkin()->useSmoothCursorTrail() || osu_cursor_trail_smooth_force.getBool();

	const float scaleAnim = (m_osu->getSkin()->getCursorExpand() && osu_cursor_trail_expand.getBool() ? m_fCursorExpandAnim : 1.0f) * osu_cursor_trail_scale.getFloat();
	const float trailWidth = trailImage->getWidth() * getCursorTrailScaleFactor() * scaleAnim * osu_cursor_scale.getFloat();

	CURSORTRAIL ct;
	ct.pos = pos;
	ct.time = engine->getTime() + (smoothCursorTrail ? osu_cursor_trail_smooth_length.getFloat() : osu_cursor_trail_length.getFloat());
	ct.alpha = 1.0f;
	ct.scale = scaleAnim;

	if (smoothCursorTrail)
	{
		// interpolate mid points between the last point and the current point
		if (trail.size() > 0)
		{
			const Vector2 prevPos = trail[trail.size()-1].pos;
			const float prevTime = trail[trail.size()-1].time;
			const float prevScale = trail[trail.size()-1].scale;

			Vector2 delta = pos - prevPos;
			const int numMidPoints = (int)(delta.length() / (trailWidth/osu_cursor_trail_smooth_div.getFloat()));
			if (numMidPoints > 0)
			{
				const Vector2 step = delta.normalize() * (trailWidth/osu_cursor_trail_smooth_div.getFloat());
				const float timeStep = (ct.time - prevTime) / (float)(numMidPoints);
				const float scaleStep = (ct.scale - prevScale) / (float)(numMidPoints);
				for (int i=clamp<int>(numMidPoints-osu_cursor_trail_max_size.getInt()/2, 0, osu_cursor_trail_max_size.getInt()); i<numMidPoints; i++) // limit to half the maximum new mid points per frame
				{
					CURSORTRAIL mid;
					mid.pos = prevPos + step*(i+1);
					mid.time = prevTime + timeStep*(i+1);
					mid.alpha = 1.0f;
					mid.scale = prevScale + scaleStep*(i+1);
					trail.push_back(mid);
				}
			}
		}
		else
			trail.push_back(ct);
	}
	else if ((trail.size() > 0 && engine->getTime() > trail[trail.size()-1].time-osu_cursor_trail_length.getFloat()+osu_cursor_trail_spacing.getFloat()) || trail.size() == 0)
	{
		if (trail.size() > 0 && trail[trail.size()-1].pos == pos)
		{
			trail[trail.size()-1].time = ct.time;
			trail[trail.size()-1].alpha = 1.0f;
			trail[trail.size()-1].scale = ct.scale;
		}
		else
			trail.push_back(ct);
	}

	// early cleanup
	while (trail.size() > osu_cursor_trail_max_size.getInt())
	{
		trail.erase(trail.begin());
	}
}

void OsuHUD::selectVolumePrev()
{
	const std::vector<CBaseUIElement*> &elements = m_volumeSliderOverlayContainer->getElements();
	for (int i=0; i<elements.size(); i++)
	{
		if (((OsuUIVolumeSlider*)elements[i])->isSelected())
		{
			const int prevIndex = (i == 0 ? elements.size()-1 : i-1);
			((OsuUIVolumeSlider*)elements[i])->setSelected(false);
			((OsuUIVolumeSlider*)elements[prevIndex])->setSelected(true);
			break;
		}
	}
	animateVolumeChange();
}

void OsuHUD::selectVolumeNext()
{
	const std::vector<CBaseUIElement*> &elements = m_volumeSliderOverlayContainer->getElements();
	for (int i=0; i<elements.size(); i++)
	{
		if (((OsuUIVolumeSlider*)elements[i])->isSelected())
		{
			const int nextIndex = (i == elements.size()-1 ? 0 : i+1);
			((OsuUIVolumeSlider*)elements[i])->setSelected(false);
			((OsuUIVolumeSlider*)elements[nextIndex])->setSelected(true);
			break;
		}
	}
	animateVolumeChange();
}

void OsuHUD::resetHitErrorBar()
{
	m_hiterrors.clear();
}

McRect OsuHUD::getSkipClickRect()
{
	const float skipScale = osu_hud_scale.getFloat();
	return McRect(m_osu->getScreenWidth() - m_osu->getSkin()->getPlaySkip()->getSize().x*skipScale, m_osu->getScreenHeight() - m_osu->getSkin()->getPlaySkip()->getSize().y*skipScale, m_osu->getSkin()->getPlaySkip()->getSize().x*skipScale, m_osu->getSkin()->getPlaySkip()->getSize().y*skipScale);
}

bool OsuHUD::isVolumeOverlayVisible()
{
	return engine->getTime() < m_fVolumeChangeTime;
}

bool OsuHUD::isVolumeOverlayBusy()
{
	return (m_volumeMaster->isEnabled() && (m_volumeMaster->isBusy() || m_volumeMaster->isMouseInside()))
		|| (m_volumeEffects->isEnabled() && (m_volumeEffects->isBusy() || m_volumeEffects->isMouseInside()))
		|| (m_volumeMusic->isEnabled() && (m_volumeMusic->isBusy() || m_volumeMusic->isMouseInside()));
}
