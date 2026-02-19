//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		changelog screen
//
// $NoKeywords: $osulog
//===============================================================================//

#include "OsuChangelog.h"

#include "Engine.h"
#include "ResourceManager.h"
#include "SoundEngine.h"
#include "Mouse.h"

#include "CBaseUIContainer.h"
#include "CBaseUIScrollView.h"
#include "CBaseUILabel.h"
#include "CBaseUIButton.h"

#include "Osu.h"
#include "OsuSkin.h"
#include "OsuOptionsMenu.h"
#include "OsuHUD.h"

OsuChangelog::OsuChangelog(Osu *osu) : OsuScreenBackable(osu)
{
	m_container = new CBaseUIContainer(-1, -1, 0, 0, "");
	m_scrollView = new CBaseUIScrollView(-1, -1, 0, 0, "");
	m_scrollView->setVerticalScrolling(true);
	m_scrollView->setHorizontalScrolling(true);
	m_scrollView->setDrawBackground(false);
	m_scrollView->setDrawFrame(false);
	m_scrollView->setScrollResistance(0);
	m_container->addBaseUIElement(m_scrollView);

	std::vector<CHANGELOG> changelogs;

	CHANGELOG alpha317;
	alpha317.title = UString::format("33.14 (Build Date: %s, %s)", __DATE__, __TIME__); // (09.01.2022 - ?)
	alpha317.changes.push_back("- Improved slider rendering performance at higher resolutions (reduced rt blending overdraw)");
	alpha317.changes.push_back("- Updated user switcher to sort names alphabetically");
	alpha317.changes.push_back("- Updated cursortrail handling to reduce spam while click-seeking via the scrubbing timeline");
	alpha317.changes.push_back("- Fixed game extremely rarely randomly starting with dimmed colors/overlay");
	alpha317.changes.push_back("- Fixed button text not being centered sometimes under certain DPI/UI scaling conditions");
	alpha317.changes.push_back("- Fixed spinnerspin/sliderslide sounds sometimes getting stuck playing indefinitely when changing skins while they are playing");
	alpha317.changes.push_back("");
	alpha317.changes.push_back("");
	alpha317.changes.push_back("- Updated pp algorithm to better handle ScoreV2 cases for ScoreV1-based misscount estimation (thanks to @Givikap120!)");
	alpha317.changes.push_back("- Updated pp algorithm to disable ScoreV1-based misscount estimation for all older ScoreV2 McOsu scores (discrepancy with osu_slider_scorev2)");
	alpha317.changes.push_back("- Updated \"Use ScoreV2 Slider Accuracy\" to only affect slider head accuracy and not force store the ScoreV2 mod as enabled (osu_slider_scorev2)");
	alpha317.changes.push_back("");
	alpha317.changes.push_back("");
	alpha317.changes.push_back("- For more info on the star/pp changes in this update see https://osu.ppy.sh/home/news/2025-10-29-performance-points-star-rating-updates");
	alpha317.changes.push_back("- Updated star + pp algorithms to match current lazer implementation aka 20251007 (20) (thanks to @Givikap120!)");
	alpha317.changes.push_back("- Added new experimental mod \"No Spinners\"");
	alpha317.changes.push_back("- Added Options > Songbrowser > \"Song Buttons Velocity Animation\"");
	alpha317.changes.push_back("- Added Options > Songbrowser > \"Song Buttons Curved Layout\"");
	alpha317.changes.push_back("- Updated songbrowser song buttons to have the velocity animation disabled by default");
	alpha317.changes.push_back("- Updated \"Sort by Artist\" to secondarily sort by title");
	alpha317.changes.push_back("- Updated mod selector when changing AR/OD live while playing a beatmap to instantly update the live star/pp cache instead of deferred");
	alpha317.changes.push_back("- Fixed override slider locks in combination with non-1.0x music speeds sometimes causing weird live pp counter desyncs");
	alpha317.changes.push_back("");
	alpha317.changes.push_back("");
	alpha317.changes.push_back("- For more info on the star/pp changes in this update see https://osu.ppy.sh/home/news/2025-03-06-performance-points-star-rating-updates");
	alpha317.changes.push_back("- Updated star + pp algorithms to match current lazer implementation aka 20250306 (19) (thanks to @Khangaroo!)");
	alpha317.changes.push_back("- Added \"Sort by Unstable Rate (Mc)\" to score sorting options");
	alpha317.changes.push_back("- Added ConVars: osu_unpause_continue_delay, osu_confine_cursor_gameplay");
	alpha317.changes.push_back("- Updated \"Sort by pp (Mc)\" to always show pp scores above scores without pp values");
	alpha317.changes.push_back("- Updated Top Ranks screen to show speed multiplier on non-1.0x scores");
	alpha317.changes.push_back("- Updated scorebrowser tooltips to also display UR next to Accuracy");
	alpha317.changes.push_back("- Updated right click > \"Use Mods\" to persist override slider locks state");
	alpha317.changes.push_back("- Fixed invalid 1x1 songselect-top.png/songselect-bottom.png skin images breaking songbrowser layout");
	alpha317.changes.push_back("");
	alpha317.changes.push_back("");
	alpha317.changes.push_back("- Updated osu!stable database loader to support new 20250108 format (see https://osu.ppy.sh/home/changelog/stable40/20250108.3)");
	alpha317.changes.push_back("");
	alpha317.changes.push_back("");
	alpha317.changes.push_back("- For more info on the star/pp changes in this update see https://osu.ppy.sh/home/news/2024-10-28-performance-points-star-rating-updates");
	alpha317.changes.push_back("- Updated star + pp algorithms to match current lazer implementation aka CSR aka Combo Scaling Removal aka 20241007 (18) (thanks to @Khangaroo!)");
	alpha317.changes.push_back("- Increased performance of live star/pp calc by ~100x (yes, two orders of magnitude faster. thanks to @Khangaroo!)");
	alpha317.changes.push_back("- FPoSu: Added Skybox cubemap support (Options > FPoSu - Playfield > \"Skybox\", enabled by default)");
	alpha317.changes.push_back("- FPoSu: Added Options > FPoSu - Playfield > \"Background Opacity\" (transparent playfield backgrounds so you can see the skybox/cube through it)");
	alpha317.changes.push_back("- Added C/F4 hotkeys to pause music at main menu");
	alpha317.changes.push_back("- Added ConVars (1): osu_stars_always_recalc_live_strains, osu_stars_ignore_clamped_sliders, osu_user_beatmap_pp_sanity_limit_for_stats, osu_background_alpha");
	alpha317.changes.push_back("- Added ConVars (2): osu_hud_hiterrorbar_entry_miss_height_multiplier, osu_hud_hiterrorbar_entry_misaim_height_multiplier");
	alpha317.changes.push_back("- Added ConVars (3): osu_draw_main_menu_button, osu_draw_main_menu_button_subtext, osu_main_menu_slider_text_scissor, osu_main_menu_slider_text_feather");
	alpha317.changes.push_back("- Updated bonus pp algorithm (17) (see https://osu.ppy.sh/home/news/2024-03-19-changes-to-performance-points)");
	alpha317.changes.push_back("- Increased osu_beatmap_max_num_hitobjects from 32768 to 40000");
	alpha317.changes.push_back("- Fixed snd_restart not reloading skin sound buffers automatically");
	alpha317.changes.push_back("- Fixed extremely rare cases of getting stuck on a black screen permanently due to quick menu navigation skills");
	alpha317.changes.push_back("");
	alpha317.changes.push_back("");
	alpha317.changes.push_back("- Added hittable dim (hitobjects outside even the miss-range are dimmed, see https://github.com/ppy/osu/pull/20572)");
	alpha317.changes.push_back("- Added Options > Gameplay > HUD > \"Draw HitErrorBar UR\" (Unstable Rate text display above hiterrorbar, enabled by default)");
	alpha317.changes.push_back("- Added ConVars (1): osu_hud_hiterrorbar_ur_scale, osu_hud_hiterrorbar_ur_alpha, osu_hud_hiterrorbar_ur_offset_x/y_percent");
	alpha317.changes.push_back("- Added ConVars (2): osu_beatmap_max_num_hitobjects, osu_beatmap_max_num_slider_scoringtimes");
	alpha317.changes.push_back("- Added ConVars (3): osu_hitobject_hittable_dim, osu_hitobject_hittable_dim_start_percent, osu_hitobject_hittable_dim_duration, osu_mod_mafham_ignore_hittable_dim");
	alpha317.changes.push_back("- FPoSu: Updated FOV sliders to allow two decimal places");
	alpha317.changes.push_back("- Updated supported beatmap version from 14 to 128 (lazer exports)");
	alpha317.changes.push_back("- Updated \"Game Pause\" keybind to prevent binding to left mouse click (to avoid menu deadlocks)");
	alpha317.changes.push_back("- Updated mod selection screen to also close when ENTER key is pressed");
	alpha317.changes.push_back("- Fixed even more star calc crashes on stupid deliberate game-breaking beatmaps (~65k sliders * ~9k repeats * 234 ticks = ~126149263360 scoring events)");
	alpha317.changes.push_back("");
	alpha317.changes.push_back("");
	alpha317.changes.push_back("- Reenabled IME support to fix blocking keyboard language switching hotkeys (add \"-noime\" launch arg to get the old behavior back in case of problems)");
	alpha317.changes.push_back("- Improved console autocomplete");
	alpha317.changes.push_back("- Fixed pie progressbar fill being invisible under certain conditions");
	alpha317.changes.push_back("");
	alpha317.changes.push_back("");
	alpha317.changes.push_back("- Added ConVars: osu_mod_random_seed, osu_hud_statistics_*_offset_x/y, osu_slider_max_ticks");
	alpha317.changes.push_back("- Updated mod selection screen to show rng seed when hovering over enabled \"Random\" experimental mod checkbox");
	alpha317.changes.push_back("- Fixed another set of star calc crashes on stupid aspire beatmaps (lowered slider tick limit, no timingpoints)");
	alpha317.changes.push_back("");
	alpha317.changes.push_back("");
	alpha317.changes.push_back("- Added option \"[Beta] RawInputBuffer\" (Options > Input > Mouse)");
	alpha317.changes.push_back("- Added ConVars: osu_background_color_r/g/b");
	alpha317.changes.push_back("- Linux: Fixed major executable corruption on newer distros (Ubuntu 23+) caused by gold linker (all files written were corrupt, e.g. scores.db/osu.cfg, also segfaults etc.)");
	alpha317.changes.push_back("");
	alpha317.changes.push_back("");
	alpha317.changes.push_back("- Fixed visual vs scoring slider end check in new lazer star calc (@Khangaroo)");
	alpha317.changes.push_back("- Fixed star calc stacking sometimes being calculated inaccurately");
	alpha317.changes.push_back("");
	alpha317.changes.push_back("");
	alpha317.changes.push_back("- Updated star + pp algorithms to match current lazer implementation aka 20220902 (16) (thanks to @Khangaroo!)");
	alpha317.changes.push_back("- Added option \"Disable osu!lazer star/pp algorithm nerfs for relax/autopilot\" (Options > General > Player)");
	alpha317.changes.push_back("- Fixed extremely rare AMD OpenGL driver crash when slider preview in options menu comes into view (via workaround)");
	alpha317.changes.push_back("- Added ConVars: osu_options_slider_preview_use_legacy_renderer, osu_songbrowser_scorebrowser_enabled");
	alpha317.changes.push_back("- Disabled new star/pp algorithm relax/autopilot nerfs by default in order to match previous behavior");
	alpha317.changes.push_back("");
	alpha317.changes.push_back("");
	alpha317.changes.push_back("- Added new experimental mod \"Half Timing Window\"");
	alpha317.changes.push_back("- Added \"Quick Seek\" key bindings (jump +-5 seconds, default LEFT/RIGHT arrow keys)");
	alpha317.changes.push_back("- Added hitobject type percentage support to songbrowser search (e.g. \"sliders>80%\")");
	alpha317.changes.push_back("- Added ConVars (1): osu_seek_delta, osu_end_skip, osu_mod_halfwindow_allow_300s");
	alpha317.changes.push_back("- Added ConVars (2): osu_songbrowser_search_hardcoded_filter, osu_hud_scrubbing_timeline_hover_tooltip_offset_multiplier");
	alpha317.changes.push_back("- Added ConVars (3): osu_skin_force_hitsound_sample_set, osu_hitobject_fade_in_time");
	alpha317.changes.push_back("- Added ConVars (4): osu_ar_overridenegative, osu_cs_overridenegative");
	alpha317.changes.push_back("- Added ConVars (5): osu_songbrowser_button_active/inactive_color_a/r/g/b, ...collection_active/inactive_color_a/r/g/b, ...difficulty_inactive_color_a/r/g/b");
	alpha317.changes.push_back("- Added ConVars (6): osu_hitresult_delta_colorize, osu_hitresult_delta_colorize_interpolate/multiplier/early_r/g/b/late_r/g/b");
	alpha317.changes.push_back("- Linux: Upgraded build system from Ubuntu 16 to Ubuntu 18 (anything running older glibc is no longer supported)");
	alpha317.changes.push_back("- Updated scrubbing to keep player invincible while scrubbing timeline is being clicked (even if mouse position does not change)");
	alpha317.changes.push_back("- Updated CS override to hard cap at CS +12.1429 (more than that never made sense anyway, the circle radius just goes negative)");
	alpha317.changes.push_back("- Updated audio output device change logic to restore music state (only in menu, changing output devices while playing will still kick you out)");
	alpha317.changes.push_back("- Improved songbrowser scrolling smoothness when switching beatmaps/sets (should reduce eye strain with less jumping around all the time)");
	alpha317.changes.push_back("- Improved songbrowser scrolling behavior when right-click absolute scrolling to always show full songbuttons (disabled scroll velocity offset)");
	alpha317.changes.push_back("- Improved songbrowser thumbnail/background image loading behavior");
	alpha317.changes.push_back("- Increased osu_ui_top_ranks_max from 100 to 200 by default");
	alpha317.changes.push_back("- Fixed very old legacy beatmaps (< v8) sometimes generating mismatched slider ticks (compared to stable) because of different tickDistance algorithm");
	alpha317.changes.push_back("- Fixed extremely rare infinite font/layout/resolution reloading bug killing performance caused by custom display scaling percentages (e.g. 124%, yes 124% scaling in Windows)");
	alpha317.changes.push_back("- Fixed extremely rare freeze bug caused by potential infinite stars in osu!.db");
	alpha317.changes.push_back("- Fixed multiple audio output devices with the exact same name not being selectable/handled correctly");
	alpha317.changes.push_back("- Fixed minimize_on_focus_lost_if_borderless_windowed_fullscreen not working");
	alpha317.changes.push_back("- Fixed fposu_mouse_cm_360 + fposu_mouse_dpi not updating in options menu if changed live via console/cfg");
	alpha317.changes.push_back("");
	alpha317.changes.push_back("");
	alpha317.changes.push_back("- Windows: Added support for mixed-DPI-scaling-multi-monitor setups (automatic detection based on which monitor the game is on)");
	alpha317.changes.push_back("- Windows: Added support for key binding all remaining mouse buttons (all mouse buttons can now be bound to key binding actions)");
	alpha317.changes.push_back("- Fixed osu_mod_random in cfg affecting main menu button logo text sliders");
	alpha317.changes.push_back("- Fixed very wide back button skin images overlapping other songbrowser buttons and making them impossible/invisible to click");
	alpha317.changes.push_back("- Fixed pen dragging playstyles potentially causing unintentional UI clicks when in-game screens/panels are switched and the pen is released");
	alpha317.changes.push_back("");
	alpha317.changes.push_back("");
	alpha317.changes.push_back("- Linux: Switched to SDL backend (mostly for Steam Deck multitouch support, also because both X11 and Wayland suck hard)");
	alpha317.changes.push_back("- FPoSu: Added cursor trail support (can be disabled in Options > Skin > \"Draw Cursor Trail\", or fposu_draw_cursor_trail)");
	alpha317.changes.push_back("- Added new experimental mod \"Approach Different\"");
	alpha317.changes.push_back("- Added new experimental mod \"Strict Tracking\"");
	alpha317.changes.push_back("- Added new main menu button logo text");
	alpha317.changes.push_back("- Added \"most common BPM\" in parentheses to top left songbrowser info label (e.g. \"BPM: 120-240 (190)\")");
	alpha317.changes.push_back("- Added beatmapID and beatmapSetID columns to osu_scores_export csv");
	alpha317.changes.push_back("- Added \"Reset all settings\" button to bottom of options menu");
	alpha317.changes.push_back("- Added PAGEUP/PAGEDOWN key support to songbrowser");
	alpha317.changes.push_back("- Added ConVars (1): osu_followpoints_connect_spinners, fposu_transparent_playfield");
	alpha317.changes.push_back("- Added ConVars (2): fposu_playfield_position_x/y/z, fposu_playfield_rotation_x/y/z");
	alpha317.changes.push_back("- Added ConVars (3): osu_mod_approach_different_initial_size, osu_mod_approach_different_style");
	alpha317.changes.push_back("- Added ConVars (4): osu_cursor_trail_scale, osu_hud_hiterrorbar_entry_additive, fposu_draw_cursor_trail");
	alpha317.changes.push_back("- Added ConVars (5): osu_mod_strict_tracking_remove_slider_ticks");
	alpha317.changes.push_back("- Updated songbrowser search to use \"most common BPM\" instead of \"max BPM\"");
	alpha317.changes.push_back("- Updated \"Draw Stats: BPM\" to use \"most common BPM\" instead of \"max BPM\"");
	alpha317.changes.push_back("- Updated \"Sort by BPM\" to use \"most common BPM\" instead of \"max BPM\"");
	alpha317.changes.push_back("- Updated UI DPI scaling to automatically enable/disable itself based on in-game resolution (instead of OS DPI)");
	alpha317.changes.push_back("- Updated hiterrorbar to use additive blending for entries/lines");
	alpha317.changes.push_back("- Updated preview music handling to fallback to 40% of song length (instead of beginning) if invalid/missing PreviewTime in beatmap");
	alpha317.changes.push_back("- Improved performance slightly (shader uniform caching)");
	alpha317.changes.push_back("- Fixed pp algorithm to allow AR/OD above 10 for non-1.0x speed multipliers and/or EZ/HT/HR/DT (please do Top Ranks > \"Recalculate pp\")");
	alpha317.changes.push_back("- Fixed \"Use mods\" inconsistent behavior (custom speed multiplier \"ignored once\", \"sticky\" experimental mods)");
	alpha317.changes.push_back("");
	alpha317.changes.push_back("");
	alpha317.changes.push_back("- Added collection management support (Add/Delete/Set/Unset collections, right-click context menu on beatmap buttons)");
	alpha317.changes.push_back("- FPoSu: Added new experimental mod \"Strafing\"");
	alpha317.changes.push_back("- Added gamemode selection button to songbrowser (no, skins abusing this as a decoration overlay are not supported)");
	alpha317.changes.push_back("- Added support for \"ScorePrefix\" + \"ComboPrefix\" + \"ScoreOverlap\" + \"ComboOverlap\" in skin.ini");
	alpha317.changes.push_back("- Added option \"Show osu! scores.db user names in user switcher\" (Options > General > Player)");
	alpha317.changes.push_back("- Added option \"Draw Strain Graph in Songbrowser\" (Options > General > Songbrowser)");
	alpha317.changes.push_back("- Added option \"Draw Strain Graph in Scrubbing Timeline\" (Options > General > Songbrowser)");
	alpha317.changes.push_back("- Added startup loading screen and animation");
	alpha317.changes.push_back("- Added beatmap ID to songbrowser tooltip");
	alpha317.changes.push_back("- Added hint text for experimental mods in mod selection screen");
	alpha317.changes.push_back("- Added ConVars (1): osu_mod_fposu_sound_panning, osu_mod_fps_sound_panning, osu_stacking_leniency_override");
	alpha317.changes.push_back("- Added ConVars (2): fposu_mod_strafing_strength_x/y/z, fposu_mod_strafing_frequency_x/y/z");
	alpha317.changes.push_back("- Added ConVars (3): snd_updateperiod, snd_dev_period, snd_dev_buffer, snd_wav_file_min_size");
	alpha317.changes.push_back("- Added ConVars (4): osu_ignore_beatmap_combo_numbers, osu_number_max");
	alpha317.changes.push_back("- Added ConVars (5): osu_scores_export, osu_auto_and_relax_block_user_input");
	alpha317.changes.push_back("- Updated songbrowser search to be async (avoids freezing the entire game when searching through 100k+ beatmaps)\n");
	alpha317.changes.push_back("- Updated \"PF\" text on scores to differentiate \"PFC\" (for perfect max possible combo) and \"FC\" (for no combo break, dropped sliderends allowed)");
	alpha317.changes.push_back("- Updated hitresult draw order to be correct (new results are now on top of old ones, was inverted previously and nobody noticed until now)");
	alpha317.changes.push_back("- Updated user switcher to be scrollable if the list gets too large");
	alpha317.changes.push_back("- Updated score list scrollbar size as to not overlap with text");
	alpha317.changes.push_back("- Updated score buttons to show AR/CS/OD/HP overrides directly in songbrowser (avoids having to open the score and waiting for the tooltip)");
	alpha317.changes.push_back("- Updated osu!.db loading to ignore corrupt entries with empty values (instead of producing empty songbuttons with \"//\" text in songbrowser)");
	alpha317.changes.push_back("- Linux: Updated osu! database loader to automatically rewrite backslashes into forward slashes for beatmap filepaths (as a workaround)");
	alpha317.changes.push_back("- Improved startup performance (skin loading)");
	alpha317.changes.push_back("- Fixed slider start circle hitresult getting overwritten by slider end circle hitresult in target practice mod");
	alpha317.changes.push_back("- Fixed animated hitresults being broken in target practice mod");
	alpha317.changes.push_back("- Fixed NotificationOverlay sometimes eating key inputs in options menu even while not in keybinding mode");
	alpha317.changes.push_back("- Fixed \"osu!stable\" notelock type eating the second input of frame perfect double inputs on overlapping/2b slider startcircles");
	alpha317.changes.push_back("- Fixed malformed/corrupt spinnerspin.wav skin files crashing the BASS audio library");
	changelogs.push_back(alpha317);

	CHANGELOG alpha316;
	alpha316.title = "32 - 32.04 (18.01.2021 - 25.07.2021)";
	alpha316.changes.push_back("- Added ability to copy (all) scores between profiles (Songbrowser > User > Top Ranks > Menu > Copy All Scores from ...)");
	alpha316.changes.push_back("- Added 2B support to \"osu!lazer 2020\" and \"McOsu\" notelock types");
	alpha316.changes.push_back("- Added SHIFT hotkey support to LEFT/RIGHT songbrowser group navigation");
	alpha316.changes.push_back("- Added new grouping options to songbrowser: \"By Artist\", \"By Creator\", \"By Difficulty\", \"By Length\", \"By Title\"");
	alpha316.changes.push_back("- Added dynamic star recalculation for all mods in songbrowser (stars will now recalculate to reflect active mods, including overrides and experimentals)");
	alpha316.changes.push_back("- Added ability to recalculate all McOsu pp scores (Songbrowser > User > Top Ranks > Menu > Recalculate)");
	alpha316.changes.push_back("- Added ability to convert/import osu! scores into McOsu pp scores (Songbrowser > User > Top Ranks > Menu > Import)");
	alpha316.changes.push_back("- Added ability to delete all scores of active user (Songbrowser > User > Top Ranks > Menu > Delete)");
	alpha316.changes.push_back("- Added menu button to \"Top Ranks\" screen (Recalculate pp, Import osu! Scores, Delete All Scores, etc.)");
	alpha316.changes.push_back("- Added \"Use Mods\" to context menu for score buttons (sets all mods, including overrides and experimentals, to whatever the score has)");
	alpha316.changes.push_back("- Added extra set of keybinds for key1/key2 (Options > Input > Keyboard)");
	alpha316.changes.push_back("- Added bonus pp calculation to user stats (previously total user pp were without bonus. Bonus is purely based on number of scores.)");
	alpha316.changes.push_back("- Added \"Max Possible pp\" to top left songbrowser info label (shows max possible pp with all active mods applied, including overrides and experimentals)");
	alpha316.changes.push_back("- Added option \"Draw Stats: Max Possible Combo\" (Options > Gameplay > HUD)");
	alpha316.changes.push_back("- Added option \"Draw Stats: pp (SS)\" (Options > Gameplay > HUD)");
	alpha316.changes.push_back("- Added option \"Draw Stats: Stars* (Total)\" (Options > Gameplay > HUD)");
	alpha316.changes.push_back("- Added option \"Draw Stats: Stars*** (Until Now)\" (aka live stars) (Options > Gameplay > HUD)");
	alpha316.changes.push_back("- Added support for OGG files in skin sound samples");
	alpha316.changes.push_back("- Added Tolerance2B handling to osu!stable notelock algorithm (unlock if within 3 ms overlap)");
	alpha316.changes.push_back("- Added score multiplier info label to mod selection screen");
	alpha316.changes.push_back("- FPoSu: Added zooming/scoping (Options > Input > Keyboard > FPoSu > Zoom) (Options > FPoSu > \"FOV (Zoom)\")");
	alpha316.changes.push_back("- Added ConVars (1): osu_spinner_use_ar_fadein, osu_notelock_stable_tolerance2b");
	alpha316.changes.push_back("- Added ConVars (2): fposu_zoom_fov, fposu_zoom_sensitivity_ratio, fposu_zoom_anim_duration");
	alpha316.changes.push_back("- Added ConVars (3): osu_scores_rename, osu_scores_bonus_pp, osu_collections_legacy_enabled");
	alpha316.changes.push_back("- Added ConVars (4): osu_songbrowser_dynamic_star_recalc, osu_draw_songbrowser_strain_graph, osu_draw_scrubbing_timeline_strain_graph");
	alpha316.changes.push_back("- Added ConVars (5): osu_hud_hiterrorbar_hide_during_spinner, osu_hud_inputoverlay_offset_x/y");
	alpha316.changes.push_back("- Added ConVars (6): osu_slider_snake_duration_multiplier, osu_slider_reverse_arrow_fadein_duration");
	alpha316.changes.push_back("- Added ConVars (7): fposu_draw_scorebarbg_on_top");
	alpha316.changes.push_back("- Added ConVars (8): cursor_trail_expand");
	alpha316.changes.push_back("- Added ConVars (9): osu_playfield_circular, osu_quick_retry_time");
	alpha316.changes.push_back("- Initial rewrite of songbrowser and entire internal database class architecture (not fully finished yet)");
	alpha316.changes.push_back("- Songbrowser can now handle individual diff buttons and/or split from their parent beatmap/set button");
	alpha316.changes.push_back("- Collections now correctly only show contained diffs (previously always showed entire set)");
	alpha316.changes.push_back("- Similarly, searching will now match individual diffs, instead of always the entire set");
	alpha316.changes.push_back("- However, sorting still only sorts by beatmap set heuristics, this will be fixed over time with one of the next updates");
	alpha316.changes.push_back("- All pp scores can now be recalculated at will, so click on \"Recalculate pp\" as soon as possible (Songbrowser > User > Top Ranks > Menu)");
	alpha316.changes.push_back("- Updated star algorithm (15) (Added emu1337's diff spike nerf), see https://github.com/ppy/osu/pull/13483/");
	alpha316.changes.push_back("- Updated pp algorithm (14) (Added emu1337's AR/FL adjustments), see https://github.com/ppy/osu-performance/pull/137/");
	alpha316.changes.push_back("- Updated pp algorithm (13) (Added StanR's AR11 buff), see https://github.com/ppy/osu-performance/pull/135/");
	alpha316.changes.push_back("- Updated hiterrorbar colors to more closely match osu!");
	alpha316.changes.push_back("- Updated profile switcher to show all usernames which have any score in any db (so both osu!/McOsu scores.db)");
	alpha316.changes.push_back("- Updated universal offset handling by compensating for speed multiplier to match osu!");
	alpha316.changes.push_back("- Updated beatmap length calc to include initial skippable section/break to match osu!");
	alpha316.changes.push_back("- Updated slow speed (less than 1.0x) offset compensation to match osu!");
	alpha316.changes.push_back("- Updated fposu_fov and fposu_zoom_fov to allow underscale + overscale (via console or cfg)");
	alpha316.changes.push_back("- Updated all right-click context menus to be bigger and easier to hit (score buttons, song buttons)");
	alpha316.changes.push_back("- Updated SearchUIOverlay to simply move left on text overflow");
	alpha316.changes.push_back("- Updated \"DPI\" and \"cm per 360\" textboxes to support decimal values with comma (e.g. 4,2 vs 4.2)");
	alpha316.changes.push_back("- Updated mouse_raw_input_absolute_to_window to be ignored if raw input is disabled");
	alpha316.changes.push_back("- Updated pp algorithm (12) (Reverted Xexxar's accidental AR8 buff), see https://github.com/ppy/osu-performance/pull/133");
	alpha316.changes.push_back("- Updated pp algorithm (11) (Xexxar's miss curve changes), see https://github.com/ppy/osu-performance/pull/129/");
	alpha316.changes.push_back("- Updated pp algorithm (10) (Xexxar's low acc speed nerf), see https://github.com/ppy/osu-performance/pull/128/");
	alpha316.changes.push_back("- Updated pp algorithm (9) (StanR's NF multiplier based on amount of misses), see https://github.com/ppy/osu-performance/pull/127/");
	alpha316.changes.push_back("- Updated pp algorithm (8) (StanR's SO multiplier based on amount of spinners in map), see https://github.com/ppy/osu-performance/pull/110/");
	alpha316.changes.push_back("- Updated pp algorithm (7) (Xexxar's AR11 nerf and AR8 buff), see https://github.com/ppy/osu-performance/pull/125/");
	alpha316.changes.push_back("- Linux: Updated BASS + BASSFX libraries");
	alpha316.changes.push_back("- macOS: Updated BASS + BASSFX libraries");
	alpha316.changes.push_back("- Improved background star calc speed");
	alpha316.changes.push_back("- Improved star calc memory usage (slider curves, mostly fixes aspire out-of-memory crashes)");
	alpha316.changes.push_back("- Fixed disabled snd_speed_compensate_pitch causing broken speed multiplier calculations (e.g. always 1.0x for statistics overlay)");
	alpha316.changes.push_back("- Fixed incorrect visually shown OPM/CPM/SPM values in songbrowser tooltip (multiplied by speed multiplier twice)");
	alpha316.changes.push_back("- Fixed crash on star calc for sliders with 85899394 repeats (clamp to 9000 to match osu!)");
	alpha316.changes.push_back("- Fixed K1/K2 keys triggering volume change when bound to both and held until key repeat during gameplay");
	alpha316.changes.push_back("- Fixed beatmap combo colors not getting loaded");
	alpha316.changes.push_back("- Fixed background star calc not calculating osu database entries with negative stars");
	alpha316.changes.push_back("- Fixed keys getting stuck if held until after beatmap end");
	alpha316.changes.push_back("- Fixed aspire infinite slider ticks crashes");
	alpha316.changes.push_back("- Fixed object counts not being updated by background star calc for people without osu!.db");
	alpha316.changes.push_back("- Fixed slider ball color handling (default to white instead of osu!default-skin special case)");
	alpha316.changes.push_back("- Fixed cursor ripples in FPoSu mode being drawn on wrong layer");
	alpha316.changes.push_back("- Fixed collections not showing all contained beatmaps/diffs");
	alpha316.changes.push_back("- Fixed invalid default selection when opening beatmap sets with active search");
	alpha316.changes.push_back("- Fixed partial corrupt PNG skin images not loading due to newer/stricter lodepng library version (disabled CRC check)");
	alpha316.changes.push_back("- Fixed ScoreV2 score multipliers for HR and DT and NF (1.06x -> 1.10x, 1.12x -> 1.20x, 0.5x -> 1.0x)");
	alpha316.changes.push_back("- Fixed UI toggle being hardcoded to SHIFT+TAB and not respecting \"Toggle Scoreboard\" keybind (in combination with SHIFT)");
	alpha316.changes.push_back("- Fixed star cache not updating instantly when changing Speed Override with keyboard keys while playing (previously only recalculated upon closing mod selection screen)");
	alpha316.changes.push_back("- Fixed drain not being recalculated instantly when changing HP Override while playing (previously only recalculated upon closing mod selection screen)");
	alpha316.changes.push_back("- Fixed clicking mod selection screen buttons also triggering \"click on the orange cursor to continue play\" (unwanted click-through)");
	alpha316.changes.push_back("- Fixed animated followpoint.png scaling not respecting inconsistent @2x frames");
	alpha316.changes.push_back("- Fixed drawHitCircleNumber for variable number width skins (@yclee126)");
	alpha316.changes.push_back("- Fixed spinners not using hardcoded 400 ms fadein (previously used same AR-dependent fadein as circles, because that makes sense compared to this insanity)");
	alpha316.changes.push_back("- Fixed mod selection screen visually rounding non-1.0x difficulty multipliers to one decimal digit (e.g. HR CS showed 4.55 in songbrowser but 4.5 in override)");
	alpha316.changes.push_back("- Fixed songbrowser visually always showing raw beatmap HP value (without applying mods or overrides)");
	alpha316.changes.push_back("- Fixed skipping while loading potentially breaking hitobject state");
	alpha316.changes.push_back("- Fixed very rare beatmaps ending prematurely with music (hitobjects at exact end of mp3) causing lost scores due to missing judgements");
	changelogs.push_back(alpha316);

	CHANGELOG alpha315;
	alpha315.title = "31.1 - 31.12 (14.02.2020 - 15.07.2020)";
	alpha315.changes.push_back("- Added 3 new notelock algorithms: McOsu, osu!lazer 2018, osu!lazer 2020 (Karoo13's algorithm)");
	alpha315.changes.push_back("- Added option \"Select Notelock\" (Options > Gameplay > Mechanics)");
	alpha315.changes.push_back("- Added option \"Kill Player upon Failing\" (Options > Gameplay > Mechanics)");
	alpha315.changes.push_back("- Added option \"Draw Stats: HP\" (Options > Gameplay > HUD)");
	alpha315.changes.push_back("- Added support for ranking-perfect (skin element for full combo on ranking screen)");
	alpha315.changes.push_back("- Added \"FC\" text after 123x to indicate a perfect full combo on highscore and top ranks list");
	alpha315.changes.push_back("- Added new search keywords: opm, cpm, spm, objects, circles, sliders (objects/circles/sliders per minute, total count)");
	alpha315.changes.push_back("- Added support for fail-background (skin element)");
	alpha315.changes.push_back("- Added option \"Inactive\" (Options > Audio > Volume)");
	alpha315.changes.push_back("- Added hitresult fadein + scale wobble animations (previously became visible instantly as is)");
	alpha315.changes.push_back("- VR: Added option \"Draw Laser in Game\" (Options > Virtual Reality > Miscellaneous)");
	alpha315.changes.push_back("- VR: Added option \"Draw Laser in Menu\" (Options > Virtual Reality > Miscellaneous)");
	alpha315.changes.push_back("- VR: Added option \"Head Model Brightness\" (Options > Virtual Reality > Play Area / Playfield)");
	alpha315.changes.push_back("- Windows: Added option \"Audio compatibility mode\" (Options > Audio > Devices)");
	alpha315.changes.push_back("- Added ConVars (1): osu_slider_end_miss_breaks_combo");
	alpha315.changes.push_back("- Added ConVars (2): osu_hitresult_fadein_duration, osu_hitresult_fadeout_start_time, osu_hitresult_fadeout_duration");
	alpha315.changes.push_back("- Added ConVars (3): osu_hitresult_miss_fadein_scale, osu_hitresult_animated, osu_volume_master_inactive");
	alpha315.changes.push_back("- Added ConVars (4): osu_drain_kill_notification_duration, snd_play_interp_duration, snd_play_interp_ratio");
	alpha315.changes.push_back("- Linux: Added _NET_WM_BYPASS_COMPOSITOR extended window manager hint (avoids vsync)");
	alpha315.changes.push_back("- Improved hitresult animation timing and movement accuracy to exactly match osu!stable (fadein, fadeout, scaleanim)");
	alpha315.changes.push_back("- Increased maximum file size limit from 200 MB to 512 MB (giant osu!.db support)");
	alpha315.changes.push_back("- Improved osu!.db database loading speed");
	alpha315.changes.push_back("- Improved beatmap grouping for beatmaps with invalid SetID (find ID in folder name before falling back to artist/title)");
	alpha315.changes.push_back("- Improved scroll wheel scrolling behavior");
	alpha315.changes.push_back("- General engine improvements");
	alpha315.changes.push_back("- Updated mod selection screen to display two digits after comma for non-1.0x speed multipliers");
	alpha315.changes.push_back("- Updated CPM/SPM/OPM metric calculations to multiply with speed multiplier (search, tooltip)");
	alpha315.changes.push_back("- Updated osu_drain_lazer_break_before and osu_drain_lazer_break_after to match recent updates (Lazer 2020.602.0)");
	alpha315.changes.push_back("- Updated hp drain type \"osu!lazer 2020\" for slider tails to match recent updates (Lazer 2020.603.0)");
	alpha315.changes.push_back("- Updated scrubbing to cancel the failing animation");
	alpha315.changes.push_back("- Unhappily bring followpoint behavior back in line with osu!");
	alpha315.changes.push_back("- NOTE (1): I hate osu!'s followpoint behavior, because it allows cheesing high AR reading. You WILL develop bad habits.");
	alpha315.changes.push_back("- NOTE (2): osu! draws followpoints with a fixed hardcoded AR of ~7.68 (800 ms).");
	alpha315.changes.push_back("- NOTE (3): Up until now, McOsu has clamped this to the real AR. To go back, use \"osu_followpoints_clamp 1\".");
	alpha315.changes.push_back("- NOTE (4): This is more of an experiment, and the change may be reverted, we will see.");
	alpha315.changes.push_back("- VR: Updated option \"Draw Desktop Game (2D)\" to ignore screen clicks during gameplay if disabled");
	alpha315.changes.push_back("- VR: Updated option \"Draw Desktop Game (2D)\" to not draw spectator/desktop cursors if disabled");
	alpha315.changes.push_back("- Only show ranking tooltip on left half of screen");
	alpha315.changes.push_back("- Windows: Workaround for Windows 10 bug in \"OS TabletPC Support\" in combination with raw mouse input:");
	alpha315.changes.push_back("- Windows: Everyone who uses Windows 10 and plays with a mouse should DISABLE \"OS TabletPC Support\"!");
	alpha315.changes.push_back("- Fixed \"Inactive\" volume not applying when minimizing in windowed mode");
	alpha315.changes.push_back("- Fixed disabled \"Show pp instead of score in scorebrowser\" applying to Top Ranks screen");
	alpha315.changes.push_back("- Fixed aspire regression breaking timingpoints on old osu file format v5 beatmaps");
	alpha315.changes.push_back("- Fixed hitresult animations not respecting speed multiplier (previously always faded at 1x time)");
	alpha315.changes.push_back("- Fixed aspire freeze on mangled 3-point sliders (e.g. Ping)");
	alpha315.changes.push_back("- Fixed aspire timingpoint handling (e.g. XNOR) (2)");
	alpha315.changes.push_back("- Fixed \"Quick Load\" keybind not working while in failing animation");
	alpha315.changes.push_back("- Fixed very old beatmaps not using the old stacking algorithm (version < 6)");
	alpha315.changes.push_back("- VR: Fixed slider bodies not being drawn on top of virtual screen (reversed depth buffer)");
	alpha315.changes.push_back("- VR: Fixed oversized scorebar-bg skin elements z-fighting with other HUD elements (like score)");
	alpha315.changes.push_back("- FPoSu: Fixed hiterrorbar being drawn on the playfield instead of the HUD overlay");
	alpha315.changes.push_back("- Fixed section-pass/fail appearing in too small breaks");
	alpha315.changes.push_back("- Fixed F1 as K1/K2 interfering with mod select during gameplay");
	alpha315.changes.push_back("- Fixed approximate beatmap length (used for search) not getting populated during star calculation if without osu!.db");
	alpha315.changes.push_back("- Fixed quick retry sometimes causing weird small speedup/slowdown at start");
	alpha315.changes.push_back("- Fixed section-pass/fail appearing in empty sections which are not break events");
	alpha315.changes.push_back("- Fixed scorebar-ki always being drawn for legacy skins (unwanted default skin fallback)");
	alpha315.changes.push_back("- Fixed skip intro button appearing 1 second later than usual");
	alpha315.changes.push_back("- Fixed warning arrows appearing 1 second later than usual");
	changelogs.push_back(alpha315);

	CHANGELOG alpha314;
	alpha314.title = "31 (11.02.2020)";
	alpha314.changes.push_back("- Added HP drain support");
	alpha314.changes.push_back("- Added 4 different HP drain algorithms: None, VR, osu!stable, osu!lazer");
	alpha314.changes.push_back("- Added option \"Select HP Drain\" (Options > Gameplay > Mechanics)");
	alpha314.changes.push_back("- Added geki/katu combo finisher (scoring, skin elements, health)");
	alpha314.changes.push_back("- Added Health/HP/Score Bar to HUD");
	alpha314.changes.push_back("- Added option \"Draw ScoreBar\" (Options > Gameplay > HUD)");
	alpha314.changes.push_back("- Added option \"ScoreBar Scale\" (Options > Gameplay > HUD)");
	alpha314.changes.push_back("- Added section-pass/section-fail (sounds, skin elements)");
	alpha314.changes.push_back("- Added option \"Statistics X Offset\" (Options > Gameplay > HUD)");
	alpha314.changes.push_back("- Added option \"Statistics Y Offset\" (Options > Gameplay > HUD)");
	alpha314.changes.push_back("- Added keybind \"Toggle Mod Selection Screen\" (Options > Input > Keyboard > Keys - Song Select)");
	alpha314.changes.push_back("- Added keybind \"Random Beatmap\" (Options > Input > Keyboard > Keys - Song Select)");
	alpha314.changes.push_back("- Added ConVars (1): osu_hud_hiterrorbar_alpha, osu_hud_hiterrorbar_bar_alpha, osu_hud_hiterrorbar_centerline_alpha");
	alpha314.changes.push_back("- Added ConVars (2): osu_hud_hiterrorbar_entry_alpha, osu_hud_hiterrorbar_entry_300/100/50/miss_r/g/b");
	alpha314.changes.push_back("- Added ConVars (3): osu_hud_hiterrorbar_centerline_r/g/b, osu_hud_hiterrorbar_max_entries");
	alpha314.changes.push_back("- Added ConVars (4): osu_hud_hiterrorbar_entry_hit/miss_fade_time, osu_hud_hiterrorbar_offset_percent");
	alpha314.changes.push_back("- Added ConVars (5): osu_draw_hiterrorbar_bottom/top/left/right, osu_hud_hiterrorbar_offset_bottom/top/left/right_percent");
	alpha314.changes.push_back("- Added ConVars (6): osu_drain_*, osu_drain_vr_*, osu_drain_stable_*, osu_drain_lazer_*");
	alpha314.changes.push_back("- Added ConVars (7): osu_pause_dim_alpha/duration, osu_hud_scorebar_hide_during_breaks, osu_hud_scorebar_hide_anim_duration");
	alpha314.changes.push_back("- Updated BASS audio library to 2020 2.4.15.2 (all offset problems have been fixed, yay!)");
	alpha314.changes.push_back("- FPoSu: Rotated/Flipped/Mirrored background cube UV coordinates to wrap horizontally as expected");
	alpha314.changes.push_back("- Relaxed notelock (1) to unlock 2B circles at the exact end of sliders (previously unlocked after slider end)");
	alpha314.changes.push_back("- Relaxed notelock (2) to allow mashing both buttons within the same frame (previously did not update lock)");
	alpha314.changes.push_back("- Moved hiterrorbar behind hitobjects");
	alpha314.changes.push_back("- Updated SHIFT + TAB and SHIFT scoreboard toggle behavior");
	alpha314.changes.push_back("- Improved spinner accuracy");
	alpha314.changes.push_back("- Fixed kinetic tablet scrolling at very high framerates (> ~600 fps)");
	alpha314.changes.push_back("- Fixed ranking screen layout partially for weird skins (long grade overflow)");
	alpha314.changes.push_back("- Fixed enabling \"Ignore Beatmap Sample Volume\" not immediately updating sample volume");
	alpha314.changes.push_back("- Fixed stale context menu in top ranks screen potentially allowing random score deletion if clicked");
	changelogs.push_back(alpha314);

	CHANGELOG alpha313;
	alpha313.title = "30.13 (12.01.2020)";
	alpha313.changes.push_back("- Added searching by beatmap ID + beatmap set ID");
	alpha313.changes.push_back("- Added CTRL + V support to songbrowser search (paste clipboard)");
	alpha313.changes.push_back("- Added speed display to score buttons");
	alpha313.changes.push_back("- Added support for sliderslide sound");
	alpha313.changes.push_back("- Added Touch Device mod (allows simulating pp nerf)");
	alpha313.changes.push_back("- Added option \"Always enable touch device pp nerf mod\" (Options > General > Player)");
	alpha313.changes.push_back("- Added option \"Apply speed/pitch mods while browsing\" (Options > Audio > Songbrowser)");
	alpha313.changes.push_back("- Added option \"Draw Stats: 300 hitwindow\" (Options > Gameplay > HUD)");
	alpha313.changes.push_back("- Added option \"Draw Stats: Accuracy Error\" (Options > Gameplay > HUD)");
	alpha313.changes.push_back("- Added option \"Show Skip Button during Intro\" (Options > Gameplay > General)");
	alpha313.changes.push_back("- Added option \"Show Skip Button during Breaks\" (Options > Gameplay > General)");
	alpha313.changes.push_back("- Added ConVars (1): osu_followpoints_separation_multiplier, osu_songbrowser_search_delay");
	alpha313.changes.push_back("- Added ConVars (2): osu_slider_body_fade_out_time_multiplier, osu_beatmap_preview_music_loop");
	alpha313.changes.push_back("- Added ConVars (3): osu_skin_export, osu_hud_statistics_hitdelta_chunksize");
	alpha313.changes.push_back("- Windows: Added WASAPI option \"Period Size\" (Options > Audio > WASAPI) (wasapi-test beta)");
	alpha313.changes.push_back("- Allow overscaling osu_slider_body_alpha_multiplier/color_saturation, osu_cursor_scale, fposu_distance");
	alpha313.changes.push_back("- Improved engine background async loading (please report crashes)");
	alpha313.changes.push_back("- Loop music");
	alpha313.changes.push_back("- Fixed skin hit0/hit50/hit100/hit300 animation handling (keep last frame and fade)");
	alpha313.changes.push_back("- Fixed scrubbing during lead-in time breaking things");
	alpha313.changes.push_back("- Fixed right click scrolling in songbrowser stalling if cursor goes outside container");
	alpha313.changes.push_back("- Windows: Fixed Windows key not unlocking on focus loss if \"Pause on Focus Loss\" is disabled");
	changelogs.push_back(alpha313);

	/*
	CHANGELOG alpha312;
	alpha312.title = "30.12 (19.12.2019)";
	alpha312.changes.push_back("- Added button \"Random Skin\" (Options > Skin)");
	alpha312.changes.push_back("- Added option \"SHIFT + TAB toggles everything\" (Options > Gameplay > HUD)");
	alpha312.changes.push_back("- Added ConVars (1): osu_mod_random_circle/slider/spinner_offset_x/y_percent, osu_mod_hd_circle_fadein/fadeout_start/end_percent");
	alpha312.changes.push_back("- Added ConVars (2): osu_play_hitsound_on_click_while_playing, osu_alt_f4_quits_even_while_playing");
	alpha312.changes.push_back("- Added ConVars (3): osu_skin_random, osu_skin_random_elements, osu_slider_body_unit_circle_subdivisions");
	alpha312.changes.push_back("- Windows: Ignore Windows key while playing (osu_win_disable_windows_key_while_playing)");
	alpha312.changes.push_back("- Made skip button only skip if click started inside");
	alpha312.changes.push_back("- Made mod \"Jigsaw\" allow clicks during breaks and before first hitobject");
	alpha312.changes.push_back("- Made experimental mod \"Full Alternate\" allow any key for first hitobjects, and after break, and during/after spinners");
	alpha312.changes.push_back("- Improved Steam Workshop subscribed items refresh speed");
	alpha312.changes.push_back("- Fixed grade image on songbuttons ignoring score sorting setting");
	alpha312.changes.push_back("- Fixed notelock unlocking sliders too early (previously unlocked after sliderstartcircle, now unlocks after slider end)");
	alpha312.changes.push_back("- Fixed rare hitsound timingpoint offsets (accurate on slider start/end now)");
	alpha312.changes.push_back("- Fixed NaN timingpoint handling for aspire (maybe)");
	changelogs.push_back(alpha312);

	CHANGELOG alpha311;
	alpha311.title = "30.11 (07.11.2019)";
	alpha311.changes.push_back("- Improved osu collection.db loading speed");
	alpha311.changes.push_back("- Fixed new osu database format breaking loading");
	alpha311.changes.push_back("- Added upper osu database version loading limit");
	alpha311.changes.push_back("- Added \"Sort By Misses\" to score sorting options");
	alpha311.changes.push_back("- Added ConVars: osu_rich_presence_dynamic_windowtitle, osu_database_ignore_version");
	alpha311.changes.push_back("- FPoSu: Fixed disabling \"Show FPS Counter\" not working (was always shown)");
	alpha311.changes.push_back("- Fixed rare custom manual ConVars getting removed from osu.cfg");
	changelogs.push_back(alpha311);

	CHANGELOG alpha301;
	alpha301.title = "30.1 (13.09.2019)";
	alpha301.changes.push_back("- Added proper support for HiDPI displays (scaling)");
	alpha301.changes.push_back("- Added option \"UI Scale\" (Options > Graphics > UI Scaling)");
	alpha301.changes.push_back("- Added option \"DPI Scaling\" (Options > Graphics > UI Scaling)");
	alpha301.changes.push_back("- Added context menu for deleting scores in \"Top Ranks\" screen");
	alpha301.changes.push_back("- Added sorting options for local scores (sort by pp, accuracy, combo, date)");
	alpha301.changes.push_back("- FPoSu: Added option \"Vertical FOV\" (Options > FPoSu > General)");
	alpha301.changes.push_back("- Draw breaks in scrubbing timeline");
	alpha301.changes.push_back("- Made scrubbing smoother by only seeking if the cursor position actually changed");
	alpha301.changes.push_back("- Windows: Added option \"High Priority\" (Options > Graphics > Renderer)");
	alpha301.changes.push_back("- Windows: Allow windowed resolutions to overshoot window borders (offscreen)");
	alpha301.changes.push_back("- Added ConVars: osu_followpoints_connect_combos, osu_scrubbing_smooth");
	alpha301.changes.push_back("- VR: Removed LIV support (for now)");
	alpha301.changes.push_back("- Allow loading incorrect skin.ini \"[General]\" section props before section start");
	alpha301.changes.push_back("- FPoSu: Fixed rare pause menu button jitter/unclickable");
	alpha301.changes.push_back("- Windows: Fixed toggling fullscreen sometimes causing weird windowed resolutions");
	alpha301.changes.push_back("- Windows: Fixed letterboxed \"Map Absolute Raw Input to Window\" offsets not matching osu!");
	changelogs.push_back(alpha301);

	CHANGELOG alpha30;
	alpha30.title =  "30.0 (13.06.2019)";
	alpha30.changes.push_back("- Added Steam Workshop support (for skins)");
	alpha30.changes.push_back("- Added option \"Cursor ripples\" (Options > Input > Mouse)");
	alpha30.changes.push_back("- Added skinning support for menu-background and cursor-ripple");
	alpha30.changes.push_back("- Added support for using custom BeatmapDirectory even without an osu installation/database");
	alpha30.changes.push_back("- Added ConVars: osu_cursor_ripple_duration/alpha/additive/anim_start_scale/end/fadeout_delay/tint_r/g/b");
	alpha30.changes.push_back("- General engine stability improvements");
	alpha30.changes.push_back("- Fixed AR/OD lock buttons being ignored by Timewarp experimental mod");
	alpha30.changes.push_back("- Fixed custom ConVars being ignored in cfg: osu_mods, osu_speed/ar/od/cs_override");
	changelogs.push_back(alpha30);

	CHANGELOG alpha295;
	alpha295.title =  "29.5 (26.05.2019)";
	alpha295.changes.push_back("- Added lock buttons to AR/OD override sliders (force constant AR/OD even with speed multiplier)");
	alpha295.changes.push_back("- Added reset buttons to all options settings");
	alpha295.changes.push_back("- Added ConVars: osu_slider_reverse_arrow_alpha_multiplier, snd_speed_compensate_pitch");
	alpha295.changes.push_back("- Fixed BPM statistics overlay only applying speed multiplier after music is loaded");
	alpha295.changes.push_back("- Windows: Fixed random stuck cursor on engine startup if launched in background (invalid focus)");
	changelogs.push_back(alpha295);

	CHANGELOG alpha294;
	alpha294.title =  "29.4 (14.03.2019)";
	alpha294.changes.push_back("- Merged FPoSu (Options > FPoSu)");
	alpha294.changes.push_back("- FPoSu is a real 3D first person gamemode, contrary to the 2D experimental mod \"First Person\"");
	alpha294.changes.push_back("- Thanks to Colin Brook (aka SnakeModule on GitHub)");
	alpha294.changes.push_back("- FPoSu: Moved \"Playfield Edge Distance\" from Mod Selector to Options > FPoSu > \"Distance\"");
	alpha294.changes.push_back("- FPoSu: Made mouse movement handling independent from regular osu sensitivity settings");
	alpha294.changes.push_back("- FPoSu: Made backgroundcube.png skinnable");
	alpha294.changes.push_back("- FPoSu: Added tablet support (Options > FPoSu > \"Tablet/Absolute Mode\")");
	alpha294.changes.push_back("- FPoSu: Added auto/pilot support");
	alpha294.changes.push_back("- FPoSu: Added ConVars: fposu_cube_tint_r, fposu_cube_tint_g, fposu_cube_tint_b");
	alpha294.changes.push_back("- FPoSu: Added letterboxing support");
	alpha294.changes.push_back("- FPoSu: Fixed mouse position getting set while engine is in background");
	alpha294.changes.push_back("- Added support for searching in collections");
	alpha294.changes.push_back("- Added support for changing grouping/sorting while in active search");
	alpha294.changes.push_back("- Added ConVar: osu_hud_statistics_pp_decimal_places");
	alpha294.changes.push_back("- Fixed boss key not pausing music");
	alpha294.changes.push_back("- Fixed another ArithmeticException in main menu (Aspire, Acid Rain - Covetous Beaver)");
	changelogs.push_back(alpha294);

	CHANGELOG alpha293;
	alpha293.title = "29.3 (15.02.2019)";
	alpha293.changes.push_back("- NOTE: New stars/pp are accurate with an average delta of ~0.1% or ~0.003 stars, except for very few Aspire/2B maps with ~15%");
	alpha293.changes.push_back("- Updated star algorithm to respect slider curves/repeats/ticks/tails and stacking (7)");
	alpha293.changes.push_back("- Updated star algorithm (Xexxar) (6), see https://osu.ppy.sh/home/news/2019-02-05-new-changes-to-star-rating-performance-points");
	alpha293.changes.push_back("- Updated pp algorithm (5), see https://github.com/ppy/osu-performance/pull/74/");
	alpha293.changes.push_back("- Updated beatmap parser to allow bullshit sliders (e.g. Aleph-0)");
	alpha293.changes.push_back("- Updated search behaviour to additively match words separated by spaces, instead of the entire phrase");
	alpha293.changes.push_back("- Added option \"Keep Aspect Ratio\" (Options > Graphics > Layout)");
	alpha293.changes.push_back("- Added hotkey: CTRL + Click to play with auto");
	alpha293.changes.push_back("- Added hotkey: CTRL + ENTER to play with auto");
	alpha293.changes.push_back("- Added hotkey: CTRL + A to toggle auto in songbrowser");
	alpha293.changes.push_back("- Added unbind buttons to keybinds");
	alpha293.changes.push_back("- Show <artist> - <name> [<diff>] in window title while playing");
	alpha293.changes.push_back("- Draw up to 10 background stars on song diff buttons");
	alpha293.changes.push_back("- Default to 0pp in ranking screen for incomplete scores due to scrubbing");
	alpha293.changes.push_back("- Windows: Fixed touchscreen handling being broken");
	alpha293.changes.push_back("- Fixed rare override slider reset crash");
	alpha293.changes.push_back("- Fixed lv only counting top pp scores");
	alpha293.changes.push_back("- Fixed top rank score button blinking animation not resetting");
	alpha293.changes.push_back("- Fixed top left info label in songbrowser not updating 0 stars after slow calculation finishes");
	alpha293.changes.push_back("- Fixed star calculation not prioritizing active selected beatmap if background image loading is disabled");
	alpha293.changes.push_back("- Fixed potential crashes and data mangling due to race conditions for slow background image/star loading");
	alpha293.changes.push_back("- Fixed changing audio output device breaking default skin sounds");
	alpha293.changes.push_back("- Fixed search not working as expected after behaviour change");
	alpha293.changes.push_back("- Added ConVars: osu_stars_xexxar_angles_sliders, osu_stars_stacking, osu_ui_top_ranks_max");
	changelogs.push_back(alpha293);

	CHANGELOG alpha292;
	alpha292.title = "29.2 (06.01.2019)";
	alpha292.changes.push_back("- Added \"Top Ranks\"/\"Best Performance\" screen showing all weighted scores (Songbrowser > User > Top Ranks)");
	alpha292.changes.push_back("- Added option \"Include Relax/Autopilot for total weighted pp/acc\" (Options > General > Player (Name))");
	alpha292.changes.push_back("- Added option \"Show pp instead of score in scorebrowser\" (Options > General > Player (Name))");
	alpha292.changes.push_back("- Added option \"SuddenDeath restart on miss\" (Options > Gameplay > General)");
	alpha292.changes.push_back("- Added ConVars: osu_relax_offset, osu_user_draw_pp, osu_user_draw_accuracy, osu_user_draw_level, osu_user_draw_level_bar");
	alpha292.changes.push_back("- Scale top left info label in songbrowser to screen resolution");
	alpha292.changes.push_back("- McOsu scores will now show pp instead of score by default (Options > General > Player (Name))");
	alpha292.changes.push_back("- Updated pp algorithm (4), see https://github.com/ppy/osu-performance/pull/76/");
	alpha292.changes.push_back("- Updated pp algorithm (3), see https://github.com/ppy/osu-performance/pull/72/");
	alpha292.changes.push_back("- Fixed total weighted pp counting multiple scores on the same diff");
	changelogs.push_back(alpha292);

	CHANGELOG alpha291;
	alpha291.title = "29.1 (31.12.2018)";
	alpha291.changes.push_back("- Added rich presence support (Discord + Steam)");
	alpha291.changes.push_back("- Added user profile info + switcher to songbrowser (total weighted pp/acc/lv)");
	alpha291.changes.push_back("- Added option \"Rich Presence\" (Options > Online > Integration)");
	alpha291.changes.push_back("- Added option \"Automatic Cursor Size\" (Options > Skin > Skin)");
	alpha291.changes.push_back("- Added letterboxing option \"Horizontal position\" (Options > Graphics > Layout)");
	alpha291.changes.push_back("- Added letterboxing option \"Vertical position\" (Options > Graphics > Layout)");
	alpha291.changes.push_back("- Added positional audio for hitsounds");
	alpha291.changes.push_back("- Added Score V2 keybind (defaults to 'B')");
	alpha291.changes.push_back("- Added new experimental mod \"Reverse Sliders\"");
	alpha291.changes.push_back("- Added mouse sidebutton support (mouse4, mouse5)");
	alpha291.changes.push_back("- Added detail info tooltip (approach time, hit timings, etc.) when hovering over diff info label in songbrowser (CS AR etc.)");
	alpha291.changes.push_back("- Added ConVar: osu_slider_end_inside_check_offset");
	alpha291.changes.push_back("- Added ConVars: osu_sound_panning, osu_sound_panning_multiplier, osu_approachtime_min, osu_approachtime_mid, osu_approachtime_max");
	alpha291.changes.push_back("- Added ConVars: osu_songbrowser_thumbnail_fade_in_duration, osu_songbrowser_background_fade_in_duration, osu_background_fade_after_load");
	alpha291.changes.push_back("- Added key overlay");
	alpha291.changes.push_back("- Updated combo color handling to match osu!");
	alpha291.changes.push_back("- Fade in songbrowser thumbnails");
	alpha291.changes.push_back("- Fade in songbrowser background");
	alpha291.changes.push_back("- Fade out background after load");
	alpha291.changes.push_back("- Updated diff info label in songbrowser to respect mods/overrides");
	alpha291.changes.push_back("- Updated Score v1 calculation to be more accurate");
	alpha291.changes.push_back("- Improved frame pacing");
	alpha291.changes.push_back("- Fixed object count always being 0 without osu! database");
	alpha291.changes.push_back("- Fixed slider end/tail judgements being too strict and not matching osu! exactly");
	alpha291.changes.push_back("- Fixed cursortrail being too small for some skins (if cursor@2x with non-@2x cursortrail)");
	alpha291.changes.push_back("- Fixed missing 24 ms offset for beatmaps version < 5");
	alpha291.changes.push_back("- Fixed local score tooltips not applying speed multiplier to AR/OD");
	changelogs.push_back(alpha291);

	CHANGELOG alpha29;
	alpha29.title = "29 (24.07.2018)";
	alpha29.changes.push_back("- Added local scores");
	alpha29.changes.push_back("- Added osu! scores.db support (read-only)");
	alpha29.changes.push_back("- Allow options menu anywhere (CTRL + O)");
	alpha29.changes.push_back("- VR: Added cursortrails, New cursor");
	alpha29.changes.push_back("- VR: Allow 0 meter approach distance");
	alpha29.changes.push_back("- VR: Added option \"Draw VR Approach Circles\" (Options > Virtual Reality > Gameplay)");
	alpha29.changes.push_back("- VR: Added option \"Draw VR Approach Circles on top\" (Options > Virtual Reality > Gameplay)");
	alpha29.changes.push_back("- VR: Added option \"Draw VR Approach Circles on Playfield\" (Options > Virtual Reality > Gameplay)");
	alpha29.changes.push_back("- Show enabled experimental mods on ranking screen");
	alpha29.changes.push_back("- Added scorebar-bg skin element support (usually abused for playfield background)");
	alpha29.changes.push_back("- Added option \"Draw scorebar-bg\" (Options > Gameplay > Playfield)");
	alpha29.changes.push_back("- Added option \"Legacy Slider Renderer\" (Options > Graphics > Detail Settings)");
	alpha29.changes.push_back("- Added option \"Mipmaps\" (Options > Graphics > Detail Settings)");
	alpha29.changes.push_back("- Added option \"Load osu! scores.db\" (Options > General > osu!folder)");
	alpha29.changes.push_back("- Added notification during active background star calculation in songbrowser");
	alpha29.changes.push_back("- Removed CTRL + ALT hardcoded hotkeys for scrubbing timeline");
	alpha29.changes.push_back("- General engine performance and stability improvements");
	alpha29.changes.push_back("- Fixed very old beatmaps not loading hitobjects which had float coordinates");
	alpha29.changes.push_back("- (see https://github.com/ppy/osu/pull/3072)");
	alpha29.changes.push_back("- Fixed scroll jerks/jumping randomly on all scrollviews");
	alpha29.changes.push_back("- Fixed random crash on shutdown due to double delete (OsuBeatmap::m_music)");
	changelogs.push_back(alpha29);

	CHANGELOG alpha2898;
	alpha2898.title = "28.98 (03.01.2018)";
	alpha2898.changes.push_back("- Updated pp algorithm (2), see https://github.com/ppy/osu-performance/pull/47");
	alpha2898.changes.push_back("- Updated pp algorithm (1), see https://github.com/ppy/osu-performance/pull/42");
	alpha2898.changes.push_back("- Added Score v2 mod");
	alpha2898.changes.push_back("- Added search support to options menu");
	alpha2898.changes.push_back("- Added proper volume overlay HUD with individual sliders for master/effects/music");
	alpha2898.changes.push_back("- Added/Fixed ConVars: osu_slider_followcircle_size_multiplier, osu_cursor_trail_alpha, osu_hud_volume_duration, osu_hud_volume_size_multiplier");
	alpha2898.changes.push_back("- Removed number keys being hardcoded keybinds for pause menu (1,2,3)");
	alpha2898.changes.push_back("- Improved multi-monitor handling: Remember game monitor, Fullscreen to closest monitor");
	alpha2898.changes.push_back("- Don't auto minimize window on focus lost if \"Borderless Windowed Fullscreen\" is enabled");
	alpha2898.changes.push_back("- Added ConVars: monitor, minimize_on_focus_lost_if_borderless_windowed_fullscreen");
	alpha2898.changes.push_back("- Reworked Mouse/Tablet input handling");
	alpha2898.changes.push_back("- Windows: Disable IME by default");
	alpha2898.changes.push_back("- Fixed cursor jerking to bottom right corner when accidentally wiggling mouse while using tablet");
	alpha2898.changes.push_back("- Fixed letterboxing cursor behavior (clipping/confining)");
	alpha2898.changes.push_back("- Fixed desynced slider ticks (e.g. Der Wald [Maze], first three sliders)");
	alpha2898.changes.push_back("- Fixed entering search text while binding keys in options menu");
	alpha2898.changes.push_back("- Fixed smooth cursortrail not expanding with animation");
	alpha2898.changes.push_back("- Fixed sample volumes being reset when tabbing out or losing window focus");
	alpha2898.changes.push_back("- VR: Fixed reverse arrows not being animated");
	alpha2898.changes.push_back("- Linux: Fixed crash when reloading osu database beatmaps via F5 in songbrowser");
	alpha2898.changes.push_back("- Linux: Updated BASS audio library to version 2.4.13 (19/12/2017)");
	alpha2898.changes.push_back("- Linux: Added support for confine cursor (i.e. Options > Input > Mouse > Confine Cursor, works now)");
	alpha2898.changes.push_back("- Linux: Added support for key names (i.e. Options > Input > Keyboard, key names instead of numbers)");
	alpha2898.changes.push_back("- Linux: Added support for focus changes (i.e. Options > General > Window > Pause on Focus Loss, fps_max_background)");
	alpha2898.changes.push_back("- Linux: Window is no longer resizable, added support for setWindowResizable()");
	alpha2898.changes.push_back("- Linux: Disabling fullscreen no longer causes annoying oversized window");
	alpha2898.changes.push_back("");
	changelogs.push_back(alpha2898);

	CHANGELOG alpha2897;
	alpha2897.title = "28.91 to 28.97 (29.10.2017)";
	alpha2897.changes.push_back("- Star ratings in songbrowser for everyone (even without osu!.db database)");
	alpha2897.changes.push_back("- Added smooth cursortrail support");
	alpha2897.changes.push_back("- Added option \"Force Smooth CursorTrail\" (Options > Skin)");
	alpha2897.changes.push_back("- Added sliderbreak counter to statistics overlay (Options > HUD > \"Draw Stats: SliderBreaks\")");
	alpha2897.changes.push_back("- Added new experimental mod \"Mafham\"");
	alpha2897.changes.push_back("- Added a few ConVars (osu_drain_*, osu_slider_reverse_arrow_animated)");
	alpha2897.changes.push_back("- Added multiple background image drawing toggles (Options > Gameplay > General)");
	alpha2897.changes.push_back("- Added partial support for \"BeatmapDirectory\" parameter");
	alpha2897.changes.push_back("- Added VR ConVars: osu_vr_controller_offset_x, osu_vr_controller_offset_y, osu_vr_controller_offset_z");
	alpha2897.changes.push_back("- Added description tooltips to some options");
	alpha2897.changes.push_back("- Added Daycore mod");
	alpha2897.changes.push_back("- Added option \"Higher Quality Sliders\", Options > Graphics > Detail Settings");
	alpha2897.changes.push_back("- Added current key labels to key bindings (Options > Input > Keyboard)");
	alpha2897.changes.push_back("- Added skin support for pause-overlay");
	alpha2897.changes.push_back("- Added preliminary Touchscreen support for Windows 8 and higher (Windows 8.1, Windows 10, etc.)");
	alpha2897.changes.push_back("- Added option \"Use skin's sound samples\", Options > Skin > Skin");
	alpha2897.changes.push_back("- Added 2 tablet options, \"Ignore Sensitivity\" and \"Windows Ink Workaround\" for problematic tablets/drivers");
	alpha2897.changes.push_back("- VR: Added slider sliding vibrations / haptic feedback (Options > Virtual Reality > Haptic Feedback)");
	alpha2897.changes.push_back("- VR: Added layout lock checkbox (avoid accidentally messing up your layout, especially for Oculus players)");
	alpha2897.changes.push_back("- Windows: Added hacky \"Borderless Windowed Fullscreen\" mode (Options > Graphics > Layout)");
	alpha2897.changes.push_back("- Windows: Switched to osu!'s old 2009 BASS audio library dlls to fix some slightly desynced mp3s");
	alpha2897.changes.push_back("- Windows: Stream music files instead of loading them entirely (necessary for above; faster load times)");
	alpha2897.changes.push_back("- Remember selected sorting type in songbrowser");
	alpha2897.changes.push_back("- Remember volume settings if changed outside options");
	alpha2897.changes.push_back("- Minor performance improvements (ignore empty transparent skin images)");
	alpha2897.changes.push_back("- Round stars to 1 decimal place for intelligent search (e.g. \"stars=6\")");
	alpha2897.changes.push_back("- Smoother snaking sliders");
	alpha2897.changes.push_back("- Switched to osu!lazer slider body fadeout style for snaking out sliders (aka shrinking sliders)");
	alpha2897.changes.push_back("- Renamed and moved \"Shrinking Sliders\" to \"Snaking out sliders\" (Options > Graphics > Detail Settings)");
	alpha2897.changes.push_back("- The drawn playfield border now also correctly reflects osu_playfield_stretch_x/y");
	alpha2897.changes.push_back("- Lowered minimum Mouse Sensitivity slider value from 0.4 to 0.1 (Options > Input > Mouse)");
	alpha2897.changes.push_back("- Made \"play-skip\" skin element animatable");
	alpha2897.changes.push_back("- Switched slider bezier curve generator to official algorithm (fixes Aspire loading \"freezes\", mostly)");
	alpha2897.changes.push_back("- More stuff in ranking screen tooltip (stars, speed, CS/AR/OD/HP)");
	alpha2897.changes.push_back("- Round pp up from 0.5 (727.49 => 727; 727.5 => 728)");
	alpha2897.changes.push_back("- Fixed ranking screen tooltip rounding CS/AR/OD/HP >= 10 incorrectly");
	alpha2897.changes.push_back("- Fixed sample set number being used for setting the sample type (hitsounds)");
	alpha2897.changes.push_back("- Fixed invisible selection-*.png skin images with 0 or very small size scaling the *-over.png way too big (Mods + Random)");
	alpha2897.changes.push_back("- Fixed minor visual inaccuracies (approach circle fade-in duration, hidden slider body fading behaviour)");
	alpha2897.changes.push_back("- Fixed skin versions lower than 2.2 drawing thumbnails in songbrowser, even though they shouldn't");
	alpha2897.changes.push_back("- Fixed statistics overlay layout in combination with the target practice mod");
	alpha2897.changes.push_back("- Fixed osu_mod_wobble2 sliders");
	alpha2897.changes.push_back("- Fixed timingpoint sample type and sample volume inaccuracies per-hitobject (osu_timingpoints_force, osu_timingpoints_offset)");
	alpha2897.changes.push_back("- Fixed missing sliderbreaks when the slider duration is shorter than the miss timing window of the sliderstartcircle");
	alpha2897.changes.push_back("- Fixed very long beatmaps not working (mostly random marathons with big MP3s, more than ~15 minutes)");
	alpha2897.changes.push_back("- Fixed slider vertexbuffers for ConVars: osu_playfield_rotation, osu_playfield_stretch_x, osu_playfield_stretch_y");
	alpha2897.changes.push_back("- Fixed main menu crashing on some aspire timingpoints due to division by zero");
	alpha2897.changes.push_back("- Fixed missing cursor texture on \"Click on the orange cursor to continue play!\"");
	alpha2897.changes.push_back("- Fixed incorrect hitcircle number skin overlaps");
	alpha2897.changes.push_back("- Fixed strange sliderstartcircle hitboxes on some 2007 maps");
	alpha2897.changes.push_back("- Fixed stacks not being recalculated when changing CS Override while playing");
	alpha2897.changes.push_back("- Fixed sliderbody size not updating instantly when changing CS Override slider with arrow keys");
	alpha2897.changes.push_back("- VR: Fixed SteamVR SuperSampling causing too large submission textures and breaking everything");
	alpha2897.changes.push_back("- Linux: Fixed number hotkeys not working in mod selector");
	changelogs.push_back(alpha2897);

	CHANGELOG alpha289;
	alpha289.title = "28.9 (16.04.2017)";
	alpha289.changes.push_back("- Thanks to Francesco149 for letting me use his implementation of Tom94's pp algorithm! (https://github.com/Francesco149/oppai/)");
	alpha289.changes.push_back("- Added pp to ranking/results screen");
	alpha289.changes.push_back("- Added live pp counter to statistics overlay (Options > HUD > \"Draw Stats: pp\")");
	alpha289.changes.push_back("- Added new experimental mods \"Full Alternate\", \"No 50s\" and \"No 100s no 50s\" (thanks to Jason Foley for the last two, JTF195 on github)");
	alpha289.changes.push_back("- Clamped Speed/BPM override sliders to the minimum possible 0.05x multiplier (no more negative zero)");
	alpha289.changes.push_back("- Unclamped visual AR/CS/HP/OD values in mod selection screen (e.g. negative OD due to EZHT)");
	alpha289.changes.push_back("- Fixed Auto clicking circles too early directly after loading finished (at time 0)");
	alpha289.changes.push_back("- Fixed ALT+TAB and general focus loss while starting a beatmap causing it to stop with a D rank and 100% acc");
	alpha289.changes.push_back("- Fixed \"CursorCentre: 0\" skin.ini parameter not working as expected");
	alpha289.changes.push_back("- Fixed performance optimization partially breaking 2B/Aspire beatmaps (invisible sliders)");
	alpha289.changes.push_back("- Fixed accuracy calculation dividing by zero if beatmap starts with a slider");
	alpha289.changes.push_back("- Fixed letterboxing and non-native resolutions not being consistent after restarting");
	changelogs.push_back(alpha289);

	CHANGELOG alpha288;
	alpha288.title = "28.8 (07.04.2017)";
	alpha288.changes.push_back("- Major performance optimizations");
	alpha288.changes.push_back("- Switched score calculation to 64-bit. The maximum score is now 18446744073709551615");
	alpha288.changes.push_back("- Fixed spinning spinners after dying in VR");
	alpha288.changes.push_back("- Fixed Auto failing impossible spinners");
	changelogs.push_back(alpha288);

	CHANGELOG alpha287;
	alpha287.title = "28.7 (05.04.2017)";
	alpha287.changes.push_back("- Added \"OS TabletPC Support\" (Options > Input > Tablet)");
	alpha287.changes.push_back("- Fixed being able to spin spinners with relax while paused");
	changelogs.push_back(alpha287);

	CHANGELOG alpha286;
	alpha286.title = "28.5 and 28.6 (03.04.2017)";
	alpha286.changes.push_back("- Added support for animated skins");
	alpha286.changes.push_back("- Added active mods to ranking/results screen");
	alpha286.changes.push_back("- Added avg error to Unstable Rate tooltip");
	alpha286.changes.push_back("- Added support for HitCirclePrefix in skin.ini");
	alpha286.changes.push_back("- Converted hardcoded hitwindow timing values into ConVars (osu_hitwindow_...)");
	alpha286.changes.push_back("- Fixed random vertexbuffer corruptions with shrinking sliders enabled (white flashes/blocks/triangles/lines, distorted slider bodies)");
	alpha286.changes.push_back("- Fixed animated skin elements of the default skin incorrectly overriding user skins");
	changelogs.push_back(alpha286);

	CHANGELOG alpha284;
	alpha284.title = "28.4 (28.03.2017)";
	alpha284.changes.push_back("- Added preliminary primitive sorting options to songbrowser (Artist, BPM, Creator, Date Added, Difficulty, Length, Title)");
	alpha284.changes.push_back("- Slightly improved right-click absolute scrolling range in songbrowser");
	alpha284.changes.push_back("- Fixed negative slider durations causing early gameovers (Aspire...)");
	alpha284.changes.push_back("- Fixed incorrect music lengths while searching (-1 from db caused 2^32-1 due to unsigned integer conversion)");
	changelogs.push_back(alpha284);

	CHANGELOG alpha283;
	alpha283.title = "28.3 (25.03.2017)";
	alpha283.changes.push_back("- Added VR head model to spectator mode");
	alpha283.changes.push_back("- Added ConVars vr_head_rendermodel_name, vr_head_rendermodel_scale, vr_head_rendermodel_brightness");
	alpha283.changes.push_back("- Added option to draw 300s (bottom of Options, last one)");
	alpha283.changes.push_back("- Made slider parsing & curve generation more robust against abuse, long loading times are not fixed though (Aspire...)");
	alpha283.changes.push_back("- Fixed experimental mod \"Minimize\" not working with CS Override as expected");
	alpha283.changes.push_back("- Fixed seeking/scrubbing not working properly while early waiting (green progressbar)");
	changelogs.push_back(alpha283);

	CHANGELOG alpha282;
	alpha282.title = "28.2 (22.03.2017)";
	alpha282.changes.push_back("- Fixed being able to continue spinning spinners while paused");
	alpha282.changes.push_back("- Fixed random invisible songbuttons in songbrowser on ultrawide resolutions (e.g. 21:9)");
	changelogs.push_back(alpha282);

	CHANGELOG alpha27VR;
	alpha27VR.title = "27 + 28 (Steam VR Alpha, 20.03.2017)";
	alpha27VR.changes.push_back("- Added Virtual Reality Support (OpenVR/SteamVR)");
	alpha27VR.changes.push_back("- VertexBuffer'd sliders for a ~400% fps increase (worst case) in static gameplay");
	alpha27VR.changes.push_back("- Dynamic mods like Wobble or Minimize are NOT buffered, and will perform the same as before");
	alpha27VR.changes.push_back("- Added audio output device selection (Options > Audio > Devices)");
	alpha27VR.changes.push_back("- Added VR matrix reset button (resets playfield & screen position/rotation)");
	alpha27VR.changes.push_back("- Added VR health drain & failing (experimental, but better than nothing)");
	alpha27VR.changes.push_back("- Added VR controller distance warning colorization (toggleable in Options)");
	alpha27VR.changes.push_back("");
	alpha27VR.changes.push_back("- Apply online offset automatically if available (was ignored before)");
	alpha27VR.changes.push_back("- Force minimize on focus loss (~alt-tab) if in fullscreen mode");
	alpha27VR.changes.push_back("");
	alpha27VR.changes.push_back("- Fixed beatmaps not loading if they have illegal spaces at the end of their directory name");
	alpha27VR.changes.push_back("- Improved SongBrowser SongButton layout on low resolutions and rectangular aspect ratios");
	alpha27VR.changes.push_back("- Added experimental mouse_fakelag ConVar");
	alpha27VR.changes.push_back("- Added osu_hud_hiterrorbar_width_percent, ..._height_percent, ..._bar_width_scale, ..._bar_height_scale ConVars");
	changelogs.push_back(alpha27VR);

	CHANGELOG alpha27;
	alpha27.title = "27 (Alpha, 25.12.2016)";
	alpha27.changes.push_back("- Added note blocking/locking (enabled by default!)");
	alpha27.changes.push_back("- Added \"Note Blocking\" toggle (Options > Gameplay > General)");
	alpha27.changes.push_back("- Added background fadein/fadeout during Breaks");
	alpha27.changes.push_back("- Added \"FadeIn Background during Breaks\" toggle (Options > Gameplay > General)");
	alpha27.changes.push_back("- Added \"Use osu!next Slider Style\" (Options > Skin > Skin)");
	alpha27.changes.push_back("- Added \"Show Approach Circle on First Hidden Object\" (Options > Gameplay > General)");
	alpha27.changes.push_back("- Added \"Use combo color as tint for slider border\" (Options > Skin > Skin)");
	alpha27.changes.push_back("- Separated \"Slider Opacity\" and \"SliderBody Opacity\" (Options > Skin > Skin)");
	alpha27.changes.push_back("- Added \"Boss Key (Minimize)\" keybind (Options > Input > Keyboard > Keys - Universal)");
	alpha27.changes.push_back("- Added \"Scrubbing\" keybind, defaults to SHIFT (Options > Input > Keyboard > Keys - In-Game)");
	alpha27.changes.push_back("- Added \"Draw Scrubbing Timeline\" (Options > Gameplay > HUD)");
	alpha27.changes.push_back("- Added \"Quick Save\" + \"Quick Load\" keybinds, default to F6 + F7 (Options > Input > Keyboard > Keys - In-Game)");
	alpha27.changes.push_back("- Added arrow key support to override sliders (left/right with a 0.1 stepsize)");
	alpha27.changes.push_back("");
	alpha27.changes.push_back("- Updated slider startcircle & endcircle fadeout with hidden, to match recent osu changes");
	alpha27.changes.push_back("- Changed hitobject sorting to draw simultaneous objects on top of active objects (circles during sliders, 2B etc.)");
	alpha27.changes.push_back("- Made scrubbing rebindable, added minimalistic scrubbing timeline UI (also shows quicksaves)");
	alpha27.changes.push_back("- While the timeline is visible, left click scrubs, and right click sets the quicksave time (in addition to the keybind)");
	alpha27.changes.push_back("- Improved scrubbing to only seek within the playable hitobject time (instead of the whole song)");
	alpha27.changes.push_back("- Inverted ALT key behaviour for override sliders (default stepsize is now 0.1, smaller stepsize of 0.01 while ALT is held)");
	alpha27.changes.push_back("");
	alpha27.changes.push_back("- Fixed incorrect opsu circle radius scaling within some aspect ratio ranges (CS was too high/difficult by as much as x0.2)");
	alpha27.changes.push_back("- Fixed reverse arrows counting towards accuracy");
	alpha27.changes.push_back("- Fixed jerky spinner spin sound");
	changelogs.push_back(alpha27);

	CHANGELOG alpha26;
	alpha26.title = "26 (Alpha, 27.11.2016)";
	alpha26.changes.push_back("- Added partial collection.db support (read-only!)");
	alpha26.changes.push_back("- Collections will atm contain all diffs of a beatmap, even if only 1 diff has been added to the collection in osu!");
	alpha26.changes.push_back("- Partial diffs of beatmaps in collections will be tinted orange");
	alpha26.changes.push_back("- Empty collections will not be visible");
	alpha26.changes.push_back("- Searching in collections will search all beatmaps and throw you back to \"No Grouping\"");
	alpha26.changes.push_back("- Added \"By Difficulty\" sorting (displays all beatmaps, sorted by their contained highest star rating diff)");
	alpha26.changes.push_back("- Added \"Unlimited FPS\" option (Options > Graphics > Renderer)");
	alpha26.changes.push_back("- Added slider preview to options (Options > Skin > Skin)");
	alpha26.changes.push_back("- Added \"SliderBorder Size\" slider (Options > Skin > Skin)");
	alpha26.changes.push_back("- Added \"Use slidergradient.png\" option (Options > Skin > Skin)");
	alpha26.changes.push_back("");
	alpha26.changes.push_back("- Improved thumbnail loading (scheduled loads of beatmap background image paths which become invisible before the load is finished will no longer trigger a useless image load upon finishing)");
	alpha26.changes.push_back("- This means that if you scroll with a higher velocity than 1 beatmap per 0.1 seconds per window height, thumbnails will load quicker");
	alpha26.changes.push_back("");
	alpha26.changes.push_back("- Fixed Stacking once and for all");
	alpha26.changes.push_back("- Fixed slider repeats giving too many points (they were counted as full circle hits additionally, instead of just 30 points)");
	alpha26.changes.push_back("- Score calculation is still not 100% accurate (slidertick placement)");
	alpha26.changes.push_back("- Fixed only the last few seconds of a song getting replayed upon returning to the songbrowser");
	alpha26.changes.push_back("- Fixed random beatmap selection only selecting up to the 32767th available beatmap");
	alpha26.changes.push_back("- Fixed Notes Per Second not scaling correctly with the Speed Multiplier");
	alpha26.changes.push_back("");
	alpha26.changes.push_back("- Fixed slider parser (hitSound/sampleType parameter for every slider circle including repeats)");
	alpha26.changes.push_back("- Fixed key1/key2 getting released incorrectly when mixing mouse buttons and keyboard keys while playing");
	alpha26.changes.push_back("- Fixed OsuUISongBrowserButton animations getting stuck when switching tabs (Grouping, Searching, Collections)");
	alpha26.changes.push_back("- Fixed incorrect sliderball image scaling");
	alpha26.changes.push_back("");
	alpha26.changes.push_back("- Fixed mouse buttons very rarely blocking keyboard keys (with mouse buttons disabled), getting m_bMouseKey1Down/m_bMouseKey2Down stuck");
	alpha26.changes.push_back("- Moved \"Shrinking Sliders\" option from \"Bullshit\" to \"Hitobjects\" (Options > Gameplay > Hitobjects)");
	changelogs.push_back(alpha26);

	CHANGELOG alpha25;
	alpha25.title = "25 (Alpha, 20.09.2016)";
	alpha25.changes.push_back("- Changed HitWindowMiss calculation from (500 - OD*10) ms to a constant 400 ms (clicks within this delta can count as misses)");
	alpha25.changes.push_back("");
	alpha25.changes.push_back("- Added osu!.db database support (read-only! nothing is modified) (Options > osu!folder)");
	alpha25.changes.push_back("- Added partial Score + Ranking Screen (no local scores yet)");
	alpha25.changes.push_back("- Added \"stars\" and \"length\" search parameters (only available if osu!.db support is enabled)");
	alpha25.changes.push_back("- Added star display to songbrowser (only available if osu!.db support is enabled)");
	alpha25.changes.push_back("- Added multi state mod button support, added Nightcore + Sudden Death + Perfect mods");
	alpha25.changes.push_back("- Added Mod Select hotkeys (Options > Keyboard > Keys > Mod Select)");
	alpha25.changes.push_back("- Added SliderBody Opacity slider (Options > Skin > Skin)");
	alpha25.changes.push_back("- Added \"Disable Mouse Buttons\" keybind (Options > Input > Keyboard)");
	alpha25.changes.push_back("- Added \"Save Screenshot\" keybind");
	alpha25.changes.push_back("- Added \"Player Name\" textbox to options");
	alpha25.changes.push_back("- Added new experimental mod \"Random\"");
	alpha25.changes.push_back("");
	alpha25.changes.push_back("- Added osu_draw_reverse_order command for experimental purposes (EZ reading, 1 to activate)");
	alpha25.changes.push_back("- Added osu_stacking command for experimental purposes (0 to disable stacks, 1 to enable)");
	alpha25.changes.push_back("- Added osu_playfield_stretch_x/y commands for experimental purposes (0.0 = osu, 1.0 = fullscreen, -2.0 = squish etc.)");
	alpha25.changes.push_back("");
	alpha25.changes.push_back("- Hitobject fadeout durations are now scaled with the speed multiplier (DT/HT/etc.)");
	alpha25.changes.push_back("- Wobble mod is now compensated for speed. Added osu_mod_wobble_frequency and osu_mod_wobble_rotation_speed commands");
	alpha25.changes.push_back("");
	alpha25.changes.push_back("- Fixed missing ApproachCircle in options skin preview from previous version");
	alpha25.changes.push_back("- Fixed alt-tab while m_bIsWaiting causing invisible clickable pause menu");
	alpha25.changes.push_back("- Fixed \"Disable Mouse Wheel in Play Mode\" also being active while not playing");
	changelogs.push_back(alpha25);
	*/

	CHANGELOG padding;
	padding.title = "... (older changelogs are available on GitHub or Steam)";
	padding.changes.push_back("");
	padding.changes.push_back("https://github.com/McKay42/McOsu");
	padding.changes.push_back("https://github.com/McKay42/McEngine");
	padding.changes.push_back("https://store.steampowered.com/news/app/607260");
	padding.changes.push_back("");
	padding.changes.push_back("");
	padding.changes.push_back("");
	padding.changes.push_back("");
	padding.changes.push_back("");
	changelogs.push_back(padding);

	for (int i=0; i<changelogs.size(); i++)
	{
		CHANGELOG_UI changelog;

		// title label
		changelog.title = new CBaseUILabel(0, 0, 0, 0, "", changelogs[i].title);
		if (i == 0)
			changelog.title->setTextColor(0xff00ff00);
		else
			changelog.title->setTextColor(0xff888888);

		changelog.title->setSizeToContent();
		changelog.title->setDrawBackground(false);
		changelog.title->setDrawFrame(false);

		m_scrollView->getContainer()->addBaseUIElement(changelog.title);

		// changes
		for (int c=0; c<changelogs[i].changes.size(); c++)
		{
			class CustomCBaseUILabel : public CBaseUIButton
			{
			public:
				CustomCBaseUILabel(UString text) : CBaseUIButton(0, 0, 0, 0, "", text) {;}

				virtual void draw(Graphics *g)
				{
					if (m_bVisible && isMouseInside())
					{
						g->setColor(0x3fffffff);

						const int margin = 0;
						const int marginX = margin + 10;
						g->fillRect(m_vPos.x - marginX, m_vPos.y - margin, m_vSize.x + marginX*2, m_vSize.y + margin*2);
					}

					if (!m_bVisible) return;

					g->setColor(m_textColor);
					g->pushTransform();
					{
						g->translate((int)(m_vPos.x + m_vSize.x/2.0f - m_fStringWidth/2.0f), (int)(m_vPos.y + m_vSize.y/2.0f + m_fStringHeight/2.0f));
						g->drawString(m_font, m_sText);
					}
					g->popTransform();
				}
			};

			CBaseUIButton *change = new CustomCBaseUILabel(changelogs[i].changes[c]);
			change->setClickCallback(fastdelegate::MakeDelegate(this, &OsuChangelog::onChangeClicked));

			if (i > 0)
				change->setTextColor(0xff888888);

			change->setSizeToContent();
			change->setDrawBackground(false);
			change->setDrawFrame(false);

			changelog.changes.push_back(change);

			m_scrollView->getContainer()->addBaseUIElement(change);
		}

		m_changelogs.push_back(changelog);
	}
}

OsuChangelog::~OsuChangelog()
{
	m_changelogs.clear();

	SAFE_DELETE(m_container);
}

void OsuChangelog::draw(Graphics *g)
{
	if (!m_bVisible) return;

	m_container->draw(g);

	OsuScreenBackable::draw(g);
}

void OsuChangelog::update()
{
	OsuScreenBackable::update();
	if (!m_bVisible) return;

	// HACKHACK:
	if (m_osu->getHUD()->isVolumeOverlayBusy() || m_osu->getOptionsMenu()->isMouseInside())
		engine->getMouse()->resetWheelDelta();

	m_container->update();

	if (m_backButton->isMouseInside())
		m_container->stealFocus();

	if (m_osu->getHUD()->isVolumeOverlayBusy() || m_osu->getOptionsMenu()->isMouseInside())
	{
		stealFocus();
		m_container->stealFocus();
	}
}

void OsuChangelog::setVisible(bool visible)
{
	OsuScreenBackable::setVisible(visible);

	if (m_bVisible)
		updateLayout();
}

void OsuChangelog::updateLayout()
{
	OsuScreenBackable::updateLayout();

	const float dpiScale = Osu::getUIScale(m_osu);

	m_container->setSize(m_osu->getScreenSize() + Vector2(2, 2));
	m_scrollView->setSize(m_osu->getScreenSize() + Vector2(2, 2));

	float yCounter = 0;
	for (const CHANGELOG_UI &changelog : m_changelogs)
	{
		changelog.title->onResized(); // HACKHACK: framework, setSizeToContent() does not update string metrics
		changelog.title->setSizeToContent();

		yCounter += changelog.title->getSize().y;
		changelog.title->setRelPos(15 * dpiScale, yCounter);
		///yCounter += 10 * dpiScale;

		for (CBaseUIButton *change : changelog.changes)
		{
			change->onResized(); // HACKHACK: framework, setSizeToContent() does not update string metrics
			change->setSizeToContent();
			change->setSizeY(change->getSize().y*2.0f);
			yCounter += change->getSize().y/* + 13 * dpiScale*/;
			change->setRelPos(35 * dpiScale, yCounter);
		}

		// gap to previous version
		yCounter += 65 * dpiScale;
	}

	m_scrollView->setScrollSizeToContent(15 * dpiScale);
}

void OsuChangelog::onBack()
{
	engine->getSound()->play(m_osu->getSkin()->getMenuClick());

	m_osu->toggleChangelog();
}

void OsuChangelog::onChangeClicked(CBaseUIButton *button)
{
	const UString changeTextMaybeContainingClickableURL = button->getText();

	const int maybeURLBeginIndex = changeTextMaybeContainingClickableURL.find("http");
	if (maybeURLBeginIndex != -1 && changeTextMaybeContainingClickableURL.find("://") != -1)
	{
		UString url = changeTextMaybeContainingClickableURL.substr(maybeURLBeginIndex);
		if (url.length() > 0 && url[url.length() - 1] == L')')
			url = url.substr(0, url.length() - 1);

		debugLog("url = %s\n", url.toUtf8());

		m_osu->getNotificationOverlay()->addNotification("Opening browser, please wait ...", 0xffffffff, false, 0.75f);
		env->openURLInDefaultBrowser(url);
	}
}
