/*********************************************************************

    ui.c

    Functions used to handle MAME's user interface.

    Copyright Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

*********************************************************************/

#include "driver.h"
#include "osdepend.h"
#include "video/vector.h"
#include "machine/laserdsc.h"
#include "profiler.h"
#include "cheat.h"
#ifdef CMD_LIST
#include "cmddata.h"
#endif /* CMD_LIST */
#ifdef USE_SHOW_TIME
#include <time.h>
#endif /* USE_SHOW_TIME */
#include "render.h"
#include "rendfont.h"
#include "ui.h"
#include "uiinput.h"
#include "uimenu.h"
#include "uigfx.h"

#ifdef MAMEMESS
#define MESS
#include "mslegacy.h"
#include "messopts.h"
#endif /* MAMEMESS */

#ifdef MESS
#include "mess.h"
#include "uimess.h"
#include "inputx.h"
#endif /* MESS */

#include <ctype.h>



/***************************************************************************
    CONSTANTS
***************************************************************************/

enum
{
	INPUT_TYPE_DIGITAL = 0,
	INPUT_TYPE_ANALOG = 1,
	INPUT_TYPE_ANALOG_DEC = 2,
	INPUT_TYPE_ANALOG_INC = 3
};

enum
{
	ANALOG_ITEM_KEYSPEED = 0,
	ANALOG_ITEM_CENTERSPEED,
	ANALOG_ITEM_REVERSE,
	ANALOG_ITEM_SENSITIVITY,
	ANALOG_ITEM_COUNT
};

enum
{
	LOADSAVE_NONE,
	LOADSAVE_LOAD,
	LOADSAVE_SAVE
};

//mamep: to render as fixed-width font
enum
{
	CHAR_WIDTH_HALFWIDTH = 0,
	CHAR_WIDTH_FULLWIDTH,
	CHAR_WIDTH_UNKNOWN
};



/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

static rgb_t uifont_colortable[MAX_COLORTABLE];
static render_texture *bgtexture;
static bitmap_t *bgbitmap;

static rgb_t ui_bgcolor;

/* font for rendering */
static render_font *ui_font;

float ui_font_height;

static int multiline_text_box_visible_lines;
static int multiline_text_box_target_lines;

//mamep: to render as fixed-width font
static int draw_text_fixed_mode;
static int draw_text_scroll_offset;

static int message_window_scroll;

static int auto_pause;
static int scroll_reset;

/* current UI handler */
static UINT32 (*ui_handler_callback)(running_machine *, UINT32);
static UINT32 ui_handler_param;

/* flag to track single stepping */
static int single_step;

/* FPS counter display */
static int showfps;
static osd_ticks_t showfps_end;

/* profiler display */
static int show_profiler;

/* popup text display */
static osd_ticks_t popup_text_end;

/* messagebox buffer */
static astring *messagebox_text;
static rgb_t messagebox_backcolor;

/* slider info */
static slider_state *slider_list;
static slider_state *slider_current;

static int display_rescale_message;
static int allow_rescale;

#ifdef UI_COLOR_DISPLAY
static int ui_transparency;
#endif /* UI_COLOR_DISPLAY */

#ifdef USE_SHOW_TIME
static int show_time = 0;
static int Show_Time_Position;
static void display_time(void);
#endif /* USE_SHOW_TIME */

#ifdef USE_SHOW_INPUT_LOG
static void display_input_log(void);
#endif /* USE_SHOW_INPUT_LOG */


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

static void ui_exit(running_machine *machine);
static int rescale_notifier(running_machine *machine, int width, int height);

/* text generators */
static astring *disclaimer_string(running_machine *machine, astring *buffer);
static astring *warnings_string(running_machine *machine, astring *buffer);

/* UI handlers */
static UINT32 handler_messagebox(running_machine *machine, UINT32 state);
static UINT32 handler_messagebox_ok(running_machine *machine, UINT32 state);
static UINT32 handler_messagebox_anykey(running_machine *machine, UINT32 state);
static UINT32 handler_ingame(running_machine *machine, UINT32 state);
static UINT32 handler_load_save(running_machine *machine, UINT32 state);
static UINT32 handler_confirm_quit(running_machine *machine, UINT32 state);

/* slider controls */
static slider_state *slider_alloc(const char *title, INT32 minval, INT32 defval, INT32 maxval, INT32 incval, slider_update update, void *arg);
static slider_state *slider_init(running_machine *machine);
static INT32 slider_volume(running_machine *machine, void *arg, astring *string, INT32 newval);
static INT32 slider_mixervol(running_machine *machine, void *arg, astring *string, INT32 newval);
static INT32 slider_adjuster(running_machine *machine, void *arg, astring *string, INT32 newval);
static INT32 slider_overclock(running_machine *machine, void *arg, astring *string, INT32 newval);
static INT32 slider_refresh(running_machine *machine, void *arg, astring *string, INT32 newval);
static INT32 slider_brightness(running_machine *machine, void *arg, astring *string, INT32 newval);
static INT32 slider_contrast(running_machine *machine, void *arg, astring *string, INT32 newval);
static INT32 slider_gamma(running_machine *machine, void *arg, astring *string, INT32 newval);
static INT32 slider_xscale(running_machine *machine, void *arg, astring *string, INT32 newval);
static INT32 slider_yscale(running_machine *machine, void *arg, astring *string, INT32 newval);
static INT32 slider_xoffset(running_machine *machine, void *arg, astring *string, INT32 newval);
static INT32 slider_yoffset(running_machine *machine, void *arg, astring *string, INT32 newval);
static INT32 slider_overxscale(running_machine *machine, void *arg, astring *string, INT32 newval);
static INT32 slider_overyscale(running_machine *machine, void *arg, astring *string, INT32 newval);
static INT32 slider_overxoffset(running_machine *machine, void *arg, astring *string, INT32 newval);
static INT32 slider_overyoffset(running_machine *machine, void *arg, astring *string, INT32 newval);
static INT32 slider_flicker(running_machine *machine, void *arg, astring *string, INT32 newval);
static INT32 slider_beam(running_machine *machine, void *arg, astring *string, INT32 newval);
static char *slider_get_screen_desc(const device_config *screen);
static char *slider_get_laserdisc_desc(const device_config *screen);
#ifdef MAME_DEBUG
static INT32 slider_crossscale(running_machine *machine, void *arg, astring *string, INT32 newval);
static INT32 slider_crossoffset(running_machine *machine, void *arg, astring *string, INT32 newval);
#endif

static void build_bgtexture(running_machine *machine);
static void free_bgtexture(running_machine *machine);

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

/*-------------------------------------------------
    ui_set_handler - set a callback/parameter
    pair for the current UI handler
-------------------------------------------------*/

INLINE UINT32 ui_set_handler(UINT32 (*callback)(running_machine *, UINT32), UINT32 param)
{
	ui_handler_callback = callback;
	ui_handler_param = param;
	return param;
}


rgb_t ui_get_rgb_color(rgb_t color)
{
	if (color < MAX_COLORTABLE)
		return uifont_colortable[color];

	return color;
}


/*-------------------------------------------------
    is_breakable_char - is a given unicode
    character a possible line break?
-------------------------------------------------*/

INLINE int is_breakable_char(unicode_char ch)
{
	/* regular spaces and hyphens are breakable */
	if (ch == ' ' || ch == '-')
		return TRUE;

	/* In the following character sets, any character is breakable:
        Hiragana (3040-309F)
        Katakana (30A0-30FF)
        Bopomofo (3100-312F)
        Hangul Compatibility Jamo (3130-318F)
        Kanbun (3190-319F)
        Bopomofo Extended (31A0-31BF)
        CJK Strokes (31C0-31EF)
        Katakana Phonetic Extensions (31F0-31FF)
        Enclosed CJK Letters and Months (3200-32FF)
        CJK Compatibility (3300-33FF)
        CJK Unified Ideographs Extension A (3400-4DBF)
        Yijing Hexagram Symbols (4DC0-4DFF)
        CJK Unified Ideographs (4E00-9FFF) */
	if (ch >= 0x3040 && ch <= 0x9fff)
		return TRUE;

	/* Hangul Syllables (AC00-D7AF) are breakable */
	if (ch >= 0xac00 && ch <= 0xd7af)
		return TRUE;

	/* CJK Compatibility Ideographs (F900-FAFF) are breakable */
	if (ch >= 0xf900 && ch <= 0xfaff)
		return TRUE;

	return FALSE;
}


//mamep: check fullwidth character.
//mame core does not support surrogate pairs U+10000-U+10FFFF
INLINE int is_fullwidth_char(unicode_char uchar)
{
	switch (uchar)
	{
	// Chars in Latin-1 Supplement
	// font width depends on your font
	case 0x00a7:
	case 0x00a8:
	case 0x00b0:
	case 0x00b1:
	case 0x00b4:
	case 0x00b6:
	case 0x00d7:
	case 0x00f7:
		return CHAR_WIDTH_UNKNOWN;
	}

	// Greek and Coptic
	// font width depends on your font
	if (uchar >= 0x0370 && uchar <= 0x03ff)
		return CHAR_WIDTH_UNKNOWN;

	// Cyrillic
	// font width depends on your font
	if (uchar >= 0x0400 && uchar <= 0x04ff)
		return CHAR_WIDTH_UNKNOWN;

	if (uchar < 0x1000)
		return CHAR_WIDTH_HALFWIDTH;

	//	Halfwidth CJK Chars
	if (uchar >= 0xff61 && uchar <= 0xffdc)
		return CHAR_WIDTH_HALFWIDTH;

	//	Halfwidth Symbols Variants
	if (uchar >= 0xffe8 && uchar <= 0xffee)
		return CHAR_WIDTH_HALFWIDTH;

	return CHAR_WIDTH_FULLWIDTH;
}



/***************************************************************************
    CORE IMPLEMENTATION
***************************************************************************/

#ifdef UI_COLOR_DISPLAY
//============================================================
//	setup_palette
//============================================================

static void setup_palette(void)
{
	static struct
	{
		const char *name;
		int color;
		UINT8 defval[3];
	} palette_decode_table[] =
	{
//		{ OPTION_FONT_BLANK,         FONT_COLOR_BLANK,         { 0,0,0 } },
//		{ OPTION_FONT_NORMAL,        FONT_COLOR_NORMAL,        { 255,255,255 } },
//		{ OPTION_FONT_SPECIAL,       FONT_COLOR_SPECIAL,       { 247,203,0 } },
		{ OPTION_SYSTEM_BACKGROUND,  SYSTEM_COLOR_BACKGROUND,  { 16,16,48 } },
		{ OPTION_BUTTON_RED,         BUTTON_COLOR_RED,         { 255,64,64 } },
		{ OPTION_BUTTON_YELLOW,      BUTTON_COLOR_YELLOW,      { 255,238,0 } },
		{ OPTION_BUTTON_GREEN,       BUTTON_COLOR_GREEN,       { 0,255,64 } },
		{ OPTION_BUTTON_BLUE,        BUTTON_COLOR_BLUE,        { 0,170,255 } },
		{ OPTION_BUTTON_PURPLE,      BUTTON_COLOR_PURPLE,      { 170,0,255 } },
		{ OPTION_BUTTON_PINK,        BUTTON_COLOR_PINK,        { 255,0,170 } },
		{ OPTION_BUTTON_AQUA,        BUTTON_COLOR_AQUA,        { 0,255,204 } },
		{ OPTION_BUTTON_SILVER,      BUTTON_COLOR_SILVER,      { 255,0,255 } },
		{ OPTION_BUTTON_NAVY,        BUTTON_COLOR_NAVY,        { 255,160,0 } },
		{ OPTION_BUTTON_LIME,        BUTTON_COLOR_LIME,        { 190,190,190 } },
		{ OPTION_CURSOR,             CURSOR_COLOR,             { 60,120,240 } },
		{ NULL }
	};

	int i;

	ui_transparency = 255;

#ifdef TRANS_UI
	ui_transparency = options_get_int(mame_options(), OPTION_UI_TRANSPARENCY);
	if (ui_transparency < 0 || ui_transparency > 255)
	{
		mame_printf_error(_("Illegal value for %s = %s\n"), OPTION_UI_TRANSPARENCY, options_get_string(mame_options(), OPTION_UI_TRANSPARENCY));
		ui_transparency = 224;
	}
#endif /* TRANS_UI */

	for (i = 0; palette_decode_table[i].name; i++)
	{
		const char *value = options_get_string(mame_options(), palette_decode_table[i].name);
		int col = palette_decode_table[i].color;
		int r = palette_decode_table[i].defval[0];
		int g = palette_decode_table[i].defval[1];
		int b = palette_decode_table[i].defval[2];
		int rate;

		if (value)
		{
			int pal[3];

			if (sscanf(value, "%d,%d,%d", &pal[0], &pal[1], &pal[2]) != 3 ||
				pal[0] < 0 || pal[0] >= 256 ||
				pal[1] < 0 || pal[1] >= 256 ||
				pal[2] < 0 || pal[2] >= 256 )
			{
				mame_printf_error(_("error: invalid value for palette: %s\n"), value);
				continue;
			}

			r = pal[0];
			g = pal[1];
			b = pal[2];
		}

		rate = 0xff;
#ifdef TRANS_UI
		if (col == UI_FILLCOLOR)
			rate = ui_transparency;
		else
		if (col == CURSOR_COLOR)
		{
			rate = ui_transparency / 2;
			if (rate < 128)
				rate = 128; //cursor should be visible
		}
#endif /* TRANS_UI */

		uifont_colortable[col] = MAKE_ARGB(rate, r, g, b);
	}
}
#endif /* UI_COLOR_DISPLAY */


/*-------------------------------------------------
    ui_init - set up the user interface
-------------------------------------------------*/

int ui_init(running_machine *machine)
{
	/* make sure we clean up after ourselves */
	add_exit_callback(machine, ui_exit);

#ifdef MAMEMESS
	/* load the localization file */
	uistring_init();
#endif /* MAMEMESS */

#ifdef UI_COLOR_DISPLAY
	setup_palette();
#endif /* UI_COLOR_DISPLAY */
	build_bgtexture(machine);
	ui_bgcolor = UI_FILLCOLOR;

	/* allocate the font and messagebox string */
	ui_font = render_font_alloc("ui.bdf");
	messagebox_text = astring_alloc();

	/* initialize the other UI bits */
	ui_menu_init(machine);
	ui_gfx_init(machine);

#ifdef CMD_LIST
	datafile_init(mame_options());
#endif /* CMD_LIST */

	/* reset globals */
	single_step = FALSE;
	ui_set_handler(handler_messagebox, 0);

	/* request callbacks when the renderer does resizing */
	render_set_rescale_notify(machine, rescale_notifier);
	allow_rescale = 0;
	display_rescale_message = FALSE;
	return 0;
}


/*-------------------------------------------------
    ui_exit - clean up ourselves on exit
-------------------------------------------------*/

static void ui_exit(running_machine *machine)
{
#ifdef CMD_LIST
	datafile_exit();
#endif /* CMD_LIST */

	/* free the font */
	if (ui_font != NULL)
		render_font_free(ui_font);
	ui_font = NULL;

	/* free the messagebox string */
	if (messagebox_text != NULL)
		astring_free(messagebox_text);
	messagebox_text = NULL;
}


/*-------------------------------------------------
    rescale_notifier - notifier to trigger a
    rescale message during long operations
-------------------------------------------------*/

static int rescale_notifier(running_machine *machine, int width, int height)
{
	/* always allow rescaling for a "small" area and for screenless drivers */
	if (width < 500 || height < 500 || machine->primary_screen == NULL)
		return TRUE;

	/* if we've currently disallowed rescaling, turn on a message next frame */
	if (allow_rescale == 0)
		display_rescale_message = TRUE;

	/* only allow rescaling once we're sure the message is visible */
	return (allow_rescale == 1);
}


/*-------------------------------------------------
    ui_display_startup_screens - display the
    various startup screens
-------------------------------------------------*/

int ui_display_startup_screens(running_machine *machine, int first_time, int show_disclaimer)
{
#ifdef MESS
	const int maxstate = 4;
#else
	const int maxstate = 3;
#endif
	int str = options_get_int(mame_options(), OPTION_SECONDS_TO_RUN);
	int show_gameinfo = !options_get_bool(mame_options(), OPTION_SKIP_GAMEINFO);
	int show_warnings = TRUE;
	int state;

	/* disable everything if we are using -str for 300 or fewer seconds, or if we're the empty driver,
       or if we are debugging */
	if (!first_time || (str > 0 && str < 60*5) || machine->gamedrv == &driver_empty || (machine->debug_flags & DEBUG_FLAG_ENABLED) != 0)
		show_gameinfo = show_warnings = show_disclaimer = FALSE;

	/* initialize the on-screen display system */
	slider_list = slider_current = slider_init(machine);

	auto_pause = FALSE;
	scroll_reset = TRUE;
#ifdef USE_SHOW_TIME
	show_time = 0;
	Show_Time_Position = 0;
#endif /* USE_SHOW_TIME */

	/* loop over states */
	ui_set_handler(handler_ingame, 0);
	for (state = 0; state < maxstate && !mame_is_scheduled_event_pending(machine) && !ui_menu_is_force_game_select(); state++)
	{
		/* default to standard colors */
		messagebox_backcolor = UI_FILLCOLOR;

		/* pick the next state */
		switch (state)
		{
			case 0:
				if (show_disclaimer && astring_len(disclaimer_string(machine, messagebox_text)) > 0)
					ui_set_handler(handler_messagebox_ok, 0);
				break;

			case 1:
				if (show_warnings && astring_len(warnings_string(machine, messagebox_text)) > 0)
				{
					ui_set_handler(handler_messagebox_ok, 0);
					if (machine->gamedrv->flags & (GAME_NOT_WORKING | GAME_UNEMULATED_PROTECTION))
						messagebox_backcolor = UI_REDCOLOR;
				}
				break;

			case 2:
				if ((show_gameinfo 
#ifdef MAMEMESS
				|| has_dummy_image(machine) > 0
#endif /* MAMEMESS */
				) && astring_len(game_info_astring(machine, messagebox_text)) > 0)
					{
						/* append MAME version and ask for select key */
						astring_catprintf(messagebox_text, "\n\t%s %s", APPNAME, build_version);

#ifdef MAMEMESS
						// mamep: prepare a screen for console with dummy image
						if(has_dummy_image(machine) > 0)
						{
							astring_catprintf(messagebox_text, "\n\t%s", _("Please load an image"));
							ui_set_handler(handler_messagebox_anykey, 0);
						}
						else
#endif /* MAMEMESS */
						{
							astring_catprintf(messagebox_text, "\n\t%s", _("Press Select Key/Button"));
							ui_set_handler(handler_messagebox_anykey, 0);
						}
					}
				break;
#ifdef MESS
			case 3:
				break;
#endif
		}

		/* clear the input memory */
		input_code_poll_switches(TRUE);
		while (input_code_poll_switches(FALSE) != INPUT_CODE_INVALID) ;

		/* loop while we have a handler */
		while (ui_handler_callback != handler_ingame && !mame_is_scheduled_event_pending(machine) && !ui_menu_is_force_game_select())
			video_frame_update(machine, FALSE);

		/* clear the handler and force an update */
		ui_set_handler(handler_ingame, 0);
		video_frame_update(machine, FALSE);

		scroll_reset = TRUE;
	}

	/* if we're the empty driver, force the menus on */
	if (ui_menu_is_force_game_select())
		ui_set_handler(ui_menu_ui_handler, 0);

	/* clear the input memory */
	while (input_code_poll_switches(FALSE) != INPUT_CODE_INVALID)
		;

	return 0;
}


/*-------------------------------------------------
    ui_set_startup_text - set the text to display
    at startup
-------------------------------------------------*/

void ui_set_startup_text(running_machine *machine, const char *text, int force)
{
	static osd_ticks_t lastupdatetime = 0;
	osd_ticks_t curtime = osd_ticks();

	/* copy in the new text */
	astring_cpyc(messagebox_text, text);
	messagebox_backcolor = UI_FILLCOLOR;

	/* don't update more than 4 times/second */
	if (force || (curtime - lastupdatetime) > osd_ticks_per_second() / 4)
	{
		lastupdatetime = curtime;
		video_frame_update(machine, FALSE);
	}
}


/*-------------------------------------------------
    ui_update_and_render - update the UI and
    render it; called by video.c
-------------------------------------------------*/

void ui_update_and_render(running_machine *machine)
{
	/* always start clean */
	render_container_empty(render_container_get_ui());

	if (auto_pause)
	{
		auto_pause = 0;
		mame_pause(machine, TRUE);
	}

	/* if we're paused, dim the whole screen */
	if (mame_get_phase(machine) >= MAME_PHASE_RESET && (single_step || mame_is_paused(machine)))
	{
		int alpha = (1.0f - options_get_float(mame_options(), OPTION_PAUSE_BRIGHTNESS)) * 255.0f;
		if (ui_menu_is_force_game_select())
			alpha = 255;
		if (alpha > 255)
			alpha = 255;
		if (alpha >= 0)
			render_ui_add_rect(0.0f, 0.0f, 1.0f, 1.0f, MAKE_ARGB(alpha,0x00,0x00,0x00), PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA));
	}

	/* render any cheat stuff at the bottom */
	cheat_render_text(machine);

	/* call the current UI handler */
	assert(ui_handler_callback != NULL);
	ui_handler_param = (*ui_handler_callback)(machine, ui_handler_param);

	/* display any popup messages */
	if (osd_ticks() < popup_text_end)
		ui_draw_text_box(astring_c(messagebox_text), JUSTIFY_CENTER, 0.5f, 0.9f, messagebox_backcolor);
	else
		popup_text_end = 0;

	/* cancel takes us back to the ingame handler */
	if (ui_handler_param == UI_HANDLER_CANCEL)
		ui_set_handler(handler_ingame, 0);

	/* add a message if we are rescaling */
	if (display_rescale_message)
	{
		display_rescale_message = FALSE;
		if (allow_rescale == 0)
			allow_rescale = 2;
		ui_draw_text_box(_("Updating Artwork..."), JUSTIFY_CENTER, 0.5f, 0.5f, messagebox_backcolor);
	}

	/* decrement the frame counter if it is non-zero */
	else if (allow_rescale != 0)
		allow_rescale--;
}


/*-------------------------------------------------
    ui_get_font - return the UI font
-------------------------------------------------*/

render_font *ui_get_font(void)
{
	return ui_font;
}


/*-------------------------------------------------
    ui_get_line_height - return the current height
    of a line
-------------------------------------------------*/

float ui_get_line_height(void)
{
	INT32 raw_font_pixel_height = render_font_get_pixel_height(ui_font);
	INT32 target_pixel_width, target_pixel_height;
	float one_to_one_line_height;
	float target_aspect;
	float scale_factor;

	/* get info about the UI target */
	render_target_get_bounds(render_get_ui_target(), &target_pixel_width, &target_pixel_height, &target_aspect);

	/* mamep: to avoid division by zero */
	if (target_pixel_height == 0)
		return 0.0f;

	/* compute the font pixel height at the nominal size */
	one_to_one_line_height = (float)raw_font_pixel_height / (float)target_pixel_height;

	/* determine the scale factor */
	scale_factor = UI_TARGET_FONT_HEIGHT / one_to_one_line_height;

	/* if our font is small-ish, do integral scaling */
	if (raw_font_pixel_height < 24)
	{
		/* do we want to scale smaller? only do so if we exceed the threshhold */
		if (scale_factor <= 1.0f)
		{
			if (one_to_one_line_height < UI_MAX_FONT_HEIGHT || raw_font_pixel_height < 12)
				scale_factor = 1.0f;
		}

		/* otherwise, just ensure an integral scale factor */
		else
			scale_factor = floor(scale_factor);
	}

	/* otherwise, just make sure we hit an even number of pixels */
	else
	{
		INT32 height = scale_factor * one_to_one_line_height * (float)target_pixel_height;
		scale_factor = (float)height / (one_to_one_line_height * (float)target_pixel_height);
	}

	return scale_factor * one_to_one_line_height;
}


/*-------------------------------------------------
    ui_get_char_width - return the width of a
    single character
-------------------------------------------------*/

float ui_get_char_width(unicode_char ch)
{
	return render_font_get_char_width(ui_font, ui_get_line_height(), render_get_ui_aspect(), ch);
}

//mamep: to render as fixed-width font
float ui_get_char_width_no_margin(unicode_char ch)
{
	return render_font_get_char_width_no_margin(ui_font, ui_get_line_height(), render_get_ui_aspect(), ch);
}

float ui_get_char_fixed_width(unicode_char uchar, double halfwidth, double fullwidth)
{
	float chwidth;

	switch (is_fullwidth_char(uchar))
	{
	case CHAR_WIDTH_HALFWIDTH:
		return halfwidth;

	case CHAR_WIDTH_UNKNOWN:
		chwidth = ui_get_char_width_no_margin(uchar);
		if (chwidth <= halfwidth)
			return halfwidth;
	}

	return fullwidth;
}



/*-------------------------------------------------
    ui_get_string_width - return the width of a
    character string
-------------------------------------------------*/

float ui_get_string_width(const char *s)
{
	return render_font_get_utf8string_width(ui_font, ui_get_line_height(), render_get_ui_aspect(), s);
}


/*-------------------------------------------------
    ui_draw_box - add primitives to draw
    a box with the given background color
-------------------------------------------------*/

void ui_draw_box(float x0, float y0, float x1, float y1, rgb_t backcolor)
{
#ifdef UI_COLOR_DISPLAY
	if (backcolor == UI_FILLCOLOR)
		render_ui_add_quad(x0, y0, x1, y1, MAKE_ARGB(0xff, 0xff, 0xff, 0xff), bgtexture, PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA));
	else
#endif /* UI_COLOR_DISPLAY */
		render_ui_add_rect(x0, y0, x1, y1, backcolor, PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA));
}


/*-------------------------------------------------
    ui_draw_outlined_box - add primitives to draw
    an outlined box with the given background
    color
-------------------------------------------------*/

void ui_draw_outlined_box(float x0, float y0, float x1, float y1, rgb_t backcolor)
{
	float hw = UI_LINE_WIDTH * 0.5f;

	ui_draw_box(x0, y0, x1, y1, backcolor);
	render_ui_add_line(x0 + hw, y0 + hw, x1 - hw, y0 + hw, UI_LINE_WIDTH, ARGB_WHITE, PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA));
	render_ui_add_line(x1 - hw, y0 + hw, x1 - hw, y1 - hw, UI_LINE_WIDTH, ARGB_WHITE, PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA));
	render_ui_add_line(x1 - hw, y1 - hw, x0 + hw, y1 - hw, UI_LINE_WIDTH, ARGB_WHITE, PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA));
	render_ui_add_line(x0 + hw, y1 - hw, x0 + hw, y0 + hw, UI_LINE_WIDTH, ARGB_WHITE, PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA));
}


/*-------------------------------------------------
    ui_draw_text - simple text renderer
-------------------------------------------------*/

void ui_draw_text(const char *buf, float x, float y)
{
	ui_draw_text_full(buf, x, y, 1.0f - x, JUSTIFY_LEFT, WRAP_WORD, DRAW_OPAQUE, ARGB_WHITE, ARGB_BLACK, NULL, NULL);
}

void ui_draw_text_bk(const char *buf, float x, float y, int col)
{
	ui_draw_text_full(buf, x, y, 1.0f - x, JUSTIFY_LEFT, WRAP_WORD, DRAW_OPAQUE, ARGB_WHITE, col, NULL, NULL);
}


/*-------------------------------------------------
    ui_draw_text_full - full featured text
    renderer with word wrapping, justification,
    and full size computation
-------------------------------------------------*/

void ui_draw_text_full(const char *origs, float x, float y, float origwrapwidth, int justify, int wrap, int draw, rgb_t fgcolor, rgb_t bgcolor, float *totalwidth, float *totalheight)
{
	float lineheight = ui_get_line_height();
	const char *ends = origs + strlen(origs);
	float wrapwidth = origwrapwidth;
	const char *s = origs;
	const char *linestart;
	float cury = y;
	float maxwidth = 0;
	const char *s_temp;
	const char *up_arrow = NULL;
	const char *down_arrow = _("(more)");

	//mamep: control scrolling text
	int curline = 0;

	//mamep: render as fixed-width font
	float fontwidth_halfwidth = 0.0f;
	float fontwidth_fullwidth = 0.0f;

	if (draw_text_fixed_mode)
	{
		int scharcount;
		int len = strlen(origs);
		int n;

		for (n = 0; len > 0; n += scharcount, len -= scharcount)
		{
			unicode_char schar;
			float scharwidth;

			scharcount = uchar_from_utf8(&schar, &origs[n], len);
			if (scharcount == -1)
				break;

			scharwidth = ui_get_char_width_no_margin(schar);
			if (is_fullwidth_char(schar))
			{
				if (fontwidth_fullwidth < scharwidth)
					fontwidth_fullwidth = scharwidth;
			}
			else
			{
				if (fontwidth_halfwidth < scharwidth)
					fontwidth_halfwidth = scharwidth;
			}
		}

		if (fontwidth_fullwidth < fontwidth_halfwidth * 2.0f)
			fontwidth_fullwidth = fontwidth_halfwidth * 2.0f;
		if (fontwidth_halfwidth < fontwidth_fullwidth / 2.0f)
			fontwidth_halfwidth = fontwidth_fullwidth / 2.0f;
	}

	//mamep: check if we are scrolling
	if (draw_text_scroll_offset)
		up_arrow = _("(more)");
	if (draw_text_scroll_offset == multiline_text_box_target_lines - multiline_text_box_visible_lines)
		down_arrow = NULL;

	/* if we don't want wrapping, guarantee a huge wrapwidth */
	if (wrap == WRAP_NEVER)
		wrapwidth = 1000000.0f;
	if (wrapwidth <= 0)
		return;

	/* loop over lines */
	while (*s != 0)
	{
		const char *lastbreak = NULL;
		int line_justify = justify;
		unicode_char schar;
		int scharcount;
		float lastbreak_width = 0;
		float curwidth = 0;
		float curx = x;

		/* get the current character */
		scharcount = uchar_from_utf8(&schar, s, ends - s);
		if (scharcount == -1)
			break;

		/* if the line starts with a tab character, center it regardless */
		if (schar == '\t')
		{
			s += scharcount;
			line_justify = JUSTIFY_CENTER;
		}

		/* remember the starting position of the line */
		linestart = s;

		/* loop while we have characters and are less than the wrapwidth */
		while (*s != 0 && curwidth <= wrapwidth)
		{
			float chwidth;

			/* get the current chcaracter */
			scharcount = uchar_from_utf8(&schar, s, ends - s);
			if (scharcount == -1)
				break;

			/* if we hit a newline, stop immediately */
			if (schar == '\n')
				break;

			//mamep: render as fixed-width font
			if (draw_text_fixed_mode)
				chwidth = ui_get_char_fixed_width(schar, fontwidth_halfwidth, fontwidth_fullwidth);
			else
				/* get the width of this character */
				chwidth = ui_get_char_width(schar);

			/* if we hit a space, remember the location and width *without* the space */
			if (schar == ' ')
			{
				lastbreak = s;
				lastbreak_width = curwidth;
			}

			/* add the width of this character and advance */
			curwidth += chwidth;
			s += scharcount;

			/* if we hit any non-space breakable character, remember the location and width
               *with* the breakable character */
			if (schar != ' ' && is_breakable_char(schar) && curwidth <= wrapwidth)
			{
				lastbreak = s;
				lastbreak_width = curwidth;
			}
		}

		/* if we accumulated too much for the current width, we need to back off */
		if (curwidth > wrapwidth)
		{
			/* if we're word wrapping, back up to the last break if we can */
			if (wrap == WRAP_WORD)
			{
				/* if we hit a break, back up to there with the appropriate width */
				if (lastbreak != NULL)
				{
					s = lastbreak;
					curwidth = lastbreak_width;
				}

				/* if we didn't hit a break, back up one character */
				else if (s > linestart)
				{
					/* get the previous character */
					s = (const char *)utf8_previous_char(s);
					scharcount = uchar_from_utf8(&schar, s, ends - s);
					if (scharcount == -1)
						break;

					//mamep: render as fixed-width font
					if (draw_text_fixed_mode)
						curwidth -= ui_get_char_fixed_width(schar, fontwidth_halfwidth, fontwidth_fullwidth);
					else
						curwidth -= ui_get_char_width(schar);
				}
			}

			/* if we're truncating, make sure we have enough space for the ... */
			else if (wrap == WRAP_TRUNCATE)
			{
				/* add in the width of the ... */
				curwidth += 3.0f * ui_get_char_width('.');

				/* while we are above the wrap width, back up one character */
				while (curwidth > wrapwidth && s > linestart)
				{
					/* get the previous character */
					s = (const char *)utf8_previous_char(s);
					scharcount = uchar_from_utf8(&schar, s, ends - s);
					if (scharcount == -1)
					break;

					curwidth -= ui_get_char_width(schar);
				}
			}
		}

		//mamep: add scrolling arrow
		if (draw != DRAW_NONE
		 && ((curline == 0 && up_arrow)
		 ||  (curline == multiline_text_box_visible_lines - 1 && down_arrow)))
		{
			if (curline == 0)
				linestart = up_arrow;
			else
				linestart = down_arrow;

			curwidth = ui_get_string_width(linestart);
			ends = linestart + strlen(linestart);
			s_temp = ends;
			line_justify = JUSTIFY_CENTER;
		}
		else
			s_temp = s;

		/* align according to the justfication */
		if (line_justify == JUSTIFY_CENTER)
			curx += (origwrapwidth - curwidth) * 0.5f;
		else if (line_justify == JUSTIFY_RIGHT)
			curx += origwrapwidth - curwidth;

		/* track the maximum width of any given line */
		if (curwidth > maxwidth)
			maxwidth = curwidth;

		/* if opaque, add a black box */
		if (draw == DRAW_OPAQUE)
			ui_draw_box(curx, cury, curx + curwidth, cury + lineheight, bgcolor);

		/* loop from the line start and add the characters */

		while (linestart < s_temp)
		{
			/* get the current character */
			unicode_char linechar;
			int linecharcount = uchar_from_utf8(&linechar, linestart, ends - linestart);
			if (linecharcount == -1)
				break;

			//mamep: consume the offset lines
			if (draw_text_scroll_offset == 0 && draw != DRAW_NONE)
			{
				//mamep: render as fixed-width font
				if (draw_text_fixed_mode)
				{
					float width = ui_get_char_fixed_width(linechar, fontwidth_halfwidth, fontwidth_fullwidth);
					float xmargin = (width - ui_get_char_width(linechar)) / 2.0f;

					render_ui_add_char(curx + xmargin, cury, lineheight, render_get_ui_aspect(), fgcolor, ui_font, linechar);
					curx += width;
				}
				else
				{
					render_ui_add_char(curx, cury, lineheight, render_get_ui_aspect(), fgcolor, ui_font, linechar);
					curx += ui_get_char_width(linechar);
				}
			}
			linestart += linecharcount;
		}

		/* append ellipses if needed */
		if (wrap == WRAP_TRUNCATE && *s != 0 && draw != DRAW_NONE)
		{
			render_ui_add_char(curx, cury, lineheight, render_get_ui_aspect(), fgcolor, ui_font, '.');
			curx += ui_get_char_width('.');
			render_ui_add_char(curx, cury, lineheight, render_get_ui_aspect(), fgcolor, ui_font, '.');
			curx += ui_get_char_width('.');
			render_ui_add_char(curx, cury, lineheight, render_get_ui_aspect(), fgcolor, ui_font, '.');
			curx += ui_get_char_width('.');
		}

		/* if we're not word-wrapping, we're done */
		if (wrap != WRAP_WORD)
			break;

		//mamep: text scrolling
		if (draw_text_scroll_offset > 0)
			draw_text_scroll_offset--;
		else
		/* advance by a row */
		{
			cury += lineheight;

			//mamep: skip overflow text
			if (draw != DRAW_NONE && curline == multiline_text_box_visible_lines - 1)
				break;

			//mamep: controll scrolling text
			if (draw_text_scroll_offset == 0)
				curline++;
		}

		/* skip past any spaces at the beginning of the next line */
		scharcount = uchar_from_utf8(&schar, s, ends - s);
		if (scharcount == -1)
			break;

		if (schar == '\n')
			s += scharcount;
		else
			while (*s && (schar < 0x80) && isspace(schar))
			{
				s += scharcount;
				scharcount = uchar_from_utf8(&schar, s, ends - s);
				if (scharcount == -1)
					break;
			}
	}

	/* report the width and height of the resulting space */
	if (totalwidth)
		*totalwidth = maxwidth;
	if (totalheight)
		*totalheight = cury - y;
}


int ui_draw_text_set_fixed_width_mode(int mode)
{
	int mode_save = draw_text_fixed_mode;

	draw_text_fixed_mode = mode;

	return mode_save;
}

void ui_draw_text_full_fixed_width(const char *origs, float x, float y, float wrapwidth, int justify, int wrap, int draw, rgb_t fgcolor, rgb_t bgcolor, float *totalwidth, float *totalheight)
{
	int mode_save = ui_draw_text_set_fixed_width_mode(TRUE);

	ui_draw_text_full(origs, x, y, wrapwidth, justify, wrap, draw, fgcolor, bgcolor, totalwidth, totalheight);
	ui_draw_text_set_fixed_width_mode(mode_save);
}

void ui_draw_text_full_scroll(const char *origs, float x, float y, float wrapwidth, int offset, int justify, int wrap, int draw, rgb_t fgcolor, rgb_t bgcolor, float *totalwidth, float *totalheight)
{
	int offset_save = draw_text_scroll_offset;

	draw_text_scroll_offset = offset;
	ui_draw_text_full(origs, x, y, wrapwidth, justify, wrap, draw, fgcolor, bgcolor, totalwidth, totalheight);

	draw_text_scroll_offset = offset_save;
}


/*-------------------------------------------------
    ui_draw_text_box - draw a multiline text
    message with a box around it
-------------------------------------------------*/

void ui_draw_text_box_scroll(const char *text, int offset, int justify, float xpos, float ypos, rgb_t backcolor)
{
	float target_width, target_height;
	float target_x, target_y;

	/* compute the multi-line target width/height */
	ui_draw_text_full(text, 0, 0, 1.0f - 2.0f * UI_BOX_LR_BORDER,
				justify, WRAP_WORD, DRAW_NONE, ARGB_WHITE, ARGB_BLACK, &target_width, &target_height);

	multiline_text_box_target_lines = (int)(target_height / ui_get_line_height() + 0.5f);
	if (target_height > 1.0f - 2.0f * UI_BOX_TB_BORDER)
		target_height = floor((1.0f - 2.0f * UI_BOX_TB_BORDER) / ui_get_line_height()) * ui_get_line_height();
	multiline_text_box_visible_lines = (int)(target_height / ui_get_line_height() + 0.5f);

	/* determine the target location */
	target_x = xpos - 0.5f * target_width;
	target_y = ypos - 0.5f * target_height;

	/* make sure we stay on-screen */
	if (target_x < UI_BOX_LR_BORDER)
		target_x = UI_BOX_LR_BORDER;
	if (target_x + target_width + UI_BOX_LR_BORDER > 1.0f)
		target_x = 1.0f - UI_BOX_LR_BORDER - target_width;
	if (target_y < UI_BOX_TB_BORDER)
		target_y = UI_BOX_TB_BORDER;
	if (target_y + target_height + UI_BOX_TB_BORDER > 1.0f)
		target_y = 1.0f - UI_BOX_TB_BORDER - target_height;

	/* add a box around that */
	ui_draw_outlined_box(target_x - UI_BOX_LR_BORDER,
					 target_y - UI_BOX_TB_BORDER,
					 target_x + target_width + UI_BOX_LR_BORDER,
					 target_y + target_height + UI_BOX_TB_BORDER, backcolor);
	ui_draw_text_full_scroll(text, target_x, target_y, target_width, offset,
				justify, WRAP_WORD, DRAW_NORMAL, ARGB_WHITE, ARGB_BLACK, NULL, NULL);
}


void ui_draw_text_box(const char *text, int justify, float xpos, float ypos, rgb_t backcolor)
{
	ui_draw_text_box_scroll(text, message_window_scroll, justify, xpos, ypos, backcolor);
}


void ui_draw_text_box_fixed_width(const char *text, int justify, float xpos, float ypos, rgb_t backcolor)
{
	int mode_save = draw_text_fixed_mode;

	draw_text_fixed_mode = 1;
	ui_draw_text_box_scroll(text, message_window_scroll, justify, xpos, ypos, backcolor);

	draw_text_fixed_mode = mode_save;
}

void ui_draw_text_box_reset_scroll(void)
{
	scroll_reset = TRUE;
}


int ui_window_scroll_keys(running_machine *machine)
{
	static int counter = 0;
	static int fast = 6;
	int pan_lines;
	int max_scroll;
	int do_scroll = FALSE;

	max_scroll = multiline_text_box_target_lines - multiline_text_box_visible_lines;
	pan_lines = multiline_text_box_visible_lines - 2;

	if (scroll_reset)
	{
		message_window_scroll = 0;
		scroll_reset = 0;
	}

	/* up backs up by one item */
	if (ui_input_pressed_repeat(machine, IPT_UI_UP, fast))
	{
		message_window_scroll--;
		do_scroll = TRUE;
	}

	/* down advances by one item */
	if (ui_input_pressed_repeat(machine, IPT_UI_DOWN, fast))
	{
		message_window_scroll++;
		do_scroll = TRUE;
	}

	/* pan-up goes to previous page */
	if (ui_input_pressed_repeat(machine, IPT_UI_PAGE_UP,8))
	{
		message_window_scroll -= pan_lines;
		do_scroll = TRUE;
	}

	/* pan-down goes to next page */
	if (ui_input_pressed_repeat(machine, IPT_UI_PAGE_DOWN,8))
	{
		message_window_scroll += pan_lines;
		do_scroll = TRUE;
	}

	/* home goes to the start */
	if (ui_input_pressed(machine, IPT_UI_HOME))
	{
		message_window_scroll = 0;
		do_scroll = TRUE;
	}

	/* end goes to the last */
	if (ui_input_pressed(machine, IPT_UI_END))
	{
		message_window_scroll = max_scroll;
		do_scroll = TRUE;
	}

	if (message_window_scroll < 0)
		message_window_scroll = 0;
	if (message_window_scroll > max_scroll)
		message_window_scroll = max_scroll;

	if (input_type_pressed(machine, IPT_UI_UP,0) || input_type_pressed(machine, IPT_UI_DOWN,0))
	{
		if (++counter == 25)
		{
			fast--;
			if (fast < 1)
				fast = 0;

			counter = 0;
		}
	}
	else
	{
		fast = 6;
		counter = 0;
	}

	if (do_scroll)
		return -1;

	if (ui_input_pressed(machine, IPT_UI_SELECT))
	{
		message_window_scroll = 0;
		return 1;
	}
	if (ui_input_pressed(machine, IPT_UI_CANCEL))
	{
		message_window_scroll = 0;
		return 2;
	}

	return 0;
}


/*-------------------------------------------------
    ui_popup_time - popup a message for a specific
    amount of time
-------------------------------------------------*/

void CLIB_DECL ui_popup_time(int seconds, const char *text, ...)
{
	va_list arg;

	/* extract the text */
	va_start(arg,text);
	astring_vprintf(messagebox_text, text, arg);
	messagebox_backcolor = UI_FILLCOLOR;
	va_end(arg);

	/* set a timer */
	popup_text_end = osd_ticks() + osd_ticks_per_second() * seconds;
}


/*-------------------------------------------------
    ui_show_fps_temp - show the FPS counter for
    a specific period of time
-------------------------------------------------*/

void ui_show_fps_temp(double seconds)
{
	if (!showfps)
		showfps_end = osd_ticks() + seconds * osd_ticks_per_second();
}


/*-------------------------------------------------
    ui_set_show_fps - show/hide the FPS counter
-------------------------------------------------*/

void ui_set_show_fps(int show)
{
	showfps = show;
	if (!show)
	{
		showfps = 0;
		showfps_end = 0;
	}
}


/*-------------------------------------------------
    ui_get_show_fps - return the current FPS
    counter visibility state
-------------------------------------------------*/

int ui_get_show_fps(void)
{
	return showfps || (showfps_end != 0);
}


/*-------------------------------------------------
    ui_set_show_profiler - show/hide the profiler
-------------------------------------------------*/

void ui_set_show_profiler(int show)
{
	if (show)
	{
		show_profiler = TRUE;
		profiler_start();
	}
	else
	{
		show_profiler = FALSE;
		profiler_stop();
	}
}


/*-------------------------------------------------
    ui_get_show_profiler - return the current
    profiler visibility state
-------------------------------------------------*/

int ui_get_show_profiler(void)
{
	return show_profiler;
}


/*-------------------------------------------------
    ui_show_menu - show the menus
-------------------------------------------------*/

void ui_show_menu(void)
{
	ui_set_handler(ui_menu_ui_handler, 0);
}


/*-------------------------------------------------
    ui_is_menu_active - return TRUE if the menu
    UI handler is active
-------------------------------------------------*/

int ui_is_menu_active(void)
{
	return (ui_handler_callback == ui_menu_ui_handler);
}



/***************************************************************************
    TEXT GENERATORS
***************************************************************************/

/*-------------------------------------------------
    disclaimer_string - print the disclaimer
    text to the given buffer
-------------------------------------------------*/

static astring *disclaimer_string(running_machine *machine, astring *string)
{
	astring_cpyc(string, _("Usage of emulators in conjunction with ROMs you don't own is forbidden by copyright law.\n\n"));
	astring_catprintf(string, _("IF YOU ARE NOT LEGALLY ENTITLED TO PLAY \"%s\" ON THIS EMULATOR, PRESS ESC.\n\n"), _LST(machine->gamedrv->description));
	astring_catc(string, _("Otherwise, type OK or move the joystick left then right to continue"));
	return string;
}


/*-------------------------------------------------
    warnings_string - print the warning flags
    text to the given buffer
-------------------------------------------------*/

static astring *warnings_string(running_machine *machine, astring *string)
{
#define WARNING_FLAGS (	GAME_NOT_WORKING | \
						GAME_UNEMULATED_PROTECTION | \
						GAME_WRONG_COLORS | \
						GAME_IMPERFECT_COLORS | \
						GAME_NO_SOUND |  \
						GAME_IMPERFECT_SOUND |  \
						GAME_IMPERFECT_GRAPHICS | \
						GAME_NO_COCKTAIL)
	int i;

	astring_reset(string);

	/* if no warnings, nothing to return */
	if (rom_load_warnings() == 0 && !(machine->gamedrv->flags & WARNING_FLAGS))
		return string;

	/* add a warning if any ROMs were loaded with warnings */
	if (rom_load_warnings() > 0)
	{
		astring_catc(string, _("One or more ROMs/CHDs for this game are incorrect. The game may not run correctly.\n"));
		if (machine->gamedrv->flags & WARNING_FLAGS)
			astring_catc(string, "\n");
	}

	/* if we have at least one warning flag, print the general header */
	if (machine->gamedrv->flags & WARNING_FLAGS)
	{
		astring_catc(string, _("There are known problems with this game\n\n"));

		/* add one line per warning flag */
#ifdef MESS
		if (machine->gamedrv->flags & GAME_COMPUTER)
			astring_catc(string, _("The emulated system is a computer:\n\nThe keyboard emulation may not be 100% accurate.\n"));
#endif
		if (machine->gamedrv->flags & GAME_IMPERFECT_COLORS)
			astring_catc(string, _("The colors aren't 100% accurate.\n"));
		if (machine->gamedrv->flags & GAME_WRONG_COLORS)
			astring_catc(string, _("The colors are completely wrong.\n"));
		if (machine->gamedrv->flags & GAME_IMPERFECT_GRAPHICS)
			astring_catc(string, _("The video emulation isn't 100% accurate.\n"));
		if (machine->gamedrv->flags & GAME_IMPERFECT_SOUND)
			astring_catc(string, _("The sound emulation isn't 100% accurate.\n"));
		if (machine->gamedrv->flags & GAME_NO_SOUND)
			astring_catc(string, _("The game lacks sound.\n"));
		if (machine->gamedrv->flags & GAME_NO_COCKTAIL)
			astring_catc(string, _("Screen flipping in cocktail mode is not supported.\n"));

		/* if there's a NOT WORKING or UNEMULATED PROTECTION warning, make it stronger */
		if (machine->gamedrv->flags & (GAME_NOT_WORKING | GAME_UNEMULATED_PROTECTION))
		{
			const game_driver *maindrv;
			const game_driver *clone_of;
			int foundworking;

			/* add the strings for these warnings */
			if (machine->gamedrv->flags & GAME_NOT_WORKING)
				astring_catc(string, _("THIS GAME DOESN'T WORK. You won't be able to make it work correctly.  Don't bother.\n"));
			if (machine->gamedrv->flags & GAME_UNEMULATED_PROTECTION)
				astring_catc(string, _("The game has protection which isn't fully emulated.\n"));

			/* find the parent of this driver */
			clone_of = driver_get_clone(machine->gamedrv);
			if (clone_of != NULL && !(clone_of->flags & GAME_IS_BIOS_ROOT))
 				maindrv = clone_of;
			else
				maindrv = machine->gamedrv;

			/* scan the driver list for any working clones and add them */
			foundworking = 0;
			for (i = 0; drivers[i] != NULL; i++)
				if (drivers[i] == maindrv || driver_get_clone(drivers[i]) == maindrv)
					if ((drivers[i]->flags & (GAME_NOT_WORKING | GAME_UNEMULATED_PROTECTION)) == 0)
					{
						/* this one works, add a header and display the name of the clone */
						if (foundworking == 0)
							astring_catc(string, _("\n\nThere are working clones of this game. They are:\n\n"));
						astring_catc(string, drivers[i]->name);
						astring_catc(string, "\n");
						foundworking = 1;
					}
		}
	}

	/* add the 'press OK' string */
	astring_catc(string, _("\n\nType OK or move the joystick left then right to continue"));
	return string;
}


#ifdef USE_SHOW_TIME

#define DISPLAY_AMPM 0

static void display_time(void)
{
	char buf[20];
#if DISPLAY_AMPM
	char am_pm[] = "am";
#endif /* DISPLAY_AMPM */
	float width;
	time_t ltime;
	struct tm *today;

	time(&ltime);
	today = localtime(&ltime);

#if DISPLAY_AMPM
	if( today->tm_hour > 12 )
	{
		strcpy( am_pm, "pm" );
		today->tm_hour -= 12;
	}
	if( today->tm_hour == 0 ) /* Adjust if midnight hour. */
		today->tm_hour = 12;
#endif /* DISPLAY_AMPM */

#if DISPLAY_AMPM
	sprintf(buf, "%02d:%02d:%02d %s", today->tm_hour, today->tm_min, today->tm_sec, am_pm);
#else
	sprintf(buf, "%02d:%02d:%02d", today->tm_hour, today->tm_min, today->tm_sec);
#endif /* DISPLAY_AMPM */
	width = ui_get_string_width(buf) + UI_LINE_WIDTH * 2.0f;
	switch(Show_Time_Position)
	{
		case 0:
			ui_draw_text_bk(buf, 1.0f - width, 1.0f - ui_get_line_height(), UI_FILLCOLOR);
			break;

		case 1:
			ui_draw_text_bk(buf, 1.0f - width, 0.0f, UI_FILLCOLOR);
			break;

		case 2:
			ui_draw_text_bk(buf, 0.0f, 0.0f, UI_FILLCOLOR);
			break;

		case 3:
			ui_draw_text_bk(buf, 0.0f, 1.0f - ui_get_line_height(), UI_FILLCOLOR);
			break;
	}
}
#endif /* USE_SHOW_TIME */

#ifdef USE_SHOW_INPUT_LOG
static void display_input_log(void)
{
	double time_now = attotime_to_double(timer_get_time());
	double time_display = attotime_to_double(ATTOTIME_IN_MSEC(1000));
	double time_fadeout = attotime_to_double(ATTOTIME_IN_MSEC(1000));
	float curx;
	int i;

	if (!command_buffer[0].code)
		return;

	// adjust time for load state
	{
		double max = 0.0f;
		int i;

		for (i = 0; command_buffer[i].code; i++)
			if (max < command_buffer[i].time)
				max = command_buffer[i].time;

		if (max > time_now)
		{
			double adjust = max - time_now;

			for (i = 0; command_buffer[i].code; i++)
				command_buffer[i].time -= adjust;
		}
	}

	// find position to start display
	curx = 1.0f - UI_LINE_WIDTH;
	for (i = 0; command_buffer[i].code; i++)
		curx -= ui_get_char_width(command_buffer[i].code);

	for (i = 0; command_buffer[i].code; i++)
	{
		if (curx >= UI_LINE_WIDTH)
			break;

		curx += ui_get_char_width(command_buffer[i].code);
	}

	ui_draw_box(0.0f, 1.0f - ui_get_line_height(), 1.0f, 1.0f, UI_FILLCOLOR);

	for (; command_buffer[i].code; i++)
	{
		double rate = time_now - command_buffer[i].time;

		if (rate < time_display + time_fadeout)
		{
			int level = 255 - ((rate - time_display) / time_fadeout) * 255;
			rgb_t fgcolor;

			if (level > 255)
				level = 255;

			fgcolor = MAKE_ARGB(255, level, level, level);

			render_ui_add_char(curx, 1.0f - ui_get_line_height(), ui_get_line_height(), render_get_ui_aspect(), fgcolor, ui_font, command_buffer[i].code);
		}
		curx += ui_get_char_width(command_buffer[i].code);
	}
}
#endif /* USE_SHOW_INPUT_LOG */



/*-------------------------------------------------
    game_info_astring - populate an allocated
    string with the game info text
-------------------------------------------------*/

astring *game_info_astring(running_machine *machine, astring *string)
{
	int scrcount = video_screen_count(machine->config);
	int cpunum, sndnum;
	int count;

	/* print description, manufacturer, and CPU: */
	astring_printf(string, "%s\n%s %s\n\nCPU:\n", _LST(machine->gamedrv->description), machine->gamedrv->year, _MANUFACT(machine->gamedrv->manufacturer));

	/* loop over all CPUs */
	for (cpunum = 0; cpunum < MAX_CPU && machine->config->cpu[cpunum].type != CPU_DUMMY; cpunum += count)
	{
		cpu_type type = machine->config->cpu[cpunum].type;
		int clock = machine->config->cpu[cpunum].clock;

		/* count how many identical CPUs we have */
		for (count = 1; cpunum + count < MAX_CPU; count++)
			if (machine->config->cpu[cpunum + count].type != type ||
		        machine->config->cpu[cpunum + count].clock != clock)
		    	break;

		/* if more than one, prepend a #x in front of the CPU name */
		if (count > 1)
			astring_catprintf(string, "%d" UTF8_MULTIPLY, count);
		astring_catc(string, cputype_name(type));

		/* display clock in kHz or MHz */
		if (clock >= 1000000)
			astring_catprintf(string, " %d.%06d" UTF8_NBSP "MHz\n", clock / 1000000, clock % 1000000);
		else
			astring_catprintf(string, " %d.%03d" UTF8_NBSP "kHz\n", clock / 1000, clock % 1000);
	}

	/* loop over all sound chips */
	for (sndnum = 0; sndnum < MAX_SOUND && machine->config->sound[sndnum].type != SOUND_DUMMY; sndnum += count)
	{
		sound_type type = machine->config->sound[sndnum].type;
		int clock = sndnum_clock(sndnum);

		/* append the Sound: string */
		if (sndnum == 0)
			astring_catc(string, _("\nSound:\n"));

		/* count how many identical sound chips we have */
		for (count = 1; sndnum + count < MAX_SOUND; count++)
			if (machine->config->sound[sndnum + count].type != type ||
		        sndnum_clock(sndnum + count) != clock)
		    	break;

		/* if more than one, prepend a #x in front of the CPU name */
		if (count > 1)
			astring_catprintf(string, "%d" UTF8_MULTIPLY, count);
		astring_catc(string, sndnum_name(sndnum));

		/* display clock in kHz or MHz */
		if (clock >= 1000000)
			astring_catprintf(string, " %d.%06d" UTF8_NBSP "MHz\n", clock / 1000000, clock % 1000000);
		else if (clock != 0)
			astring_catprintf(string, " %d.%03d" UTF8_NBSP "kHz\n", clock / 1000, clock % 1000);
		else
			astring_catc(string, "\n");
	}

	/* display screen information */
	astring_catc(string, _("\nVideo:\n"));
	if (scrcount == 0)
		astring_catc(string, _("None\n"));
	else
	{
		const device_config *screen;

		for (screen = video_screen_first(machine->config); screen != NULL; screen = video_screen_next(screen))
		{
			const screen_config *scrconfig = screen->inline_config;

			if (scrcount > 1)
				astring_catc(string, slider_get_screen_desc(screen));

			if (scrconfig->type == SCREEN_TYPE_VECTOR)
				astring_catc(string, _("Vector\n"));
			else
			{
				const rectangle *visarea = video_screen_get_visible_area(screen);

				astring_catprintf(string, "%d " UTF8_MULTIPLY " %d (%s) %f" UTF8_NBSP "Hz\n",
						visarea->max_x - visarea->min_x + 1,
						visarea->max_y - visarea->min_y + 1,
						(machine->gamedrv->flags & ORIENTATION_SWAP_XY) ? "V" : "H",
						ATTOSECONDS_TO_HZ(video_screen_get_frame_period(screen).attoseconds));
			}
		}
	}

	return string;
}



/***************************************************************************
    UI HANDLERS
***************************************************************************/

/*-------------------------------------------------
    handler_messagebox - displays the current
    messagebox_text string but handles no input
-------------------------------------------------*/

static UINT32 handler_messagebox(running_machine *machine, UINT32 state)
{
	ui_draw_text_box(astring_c(messagebox_text), JUSTIFY_LEFT, 0.5f, 0.5f, messagebox_backcolor);
	return 0;
}


/*-------------------------------------------------
    handler_messagebox_ok - displays the current
    messagebox_text string and waits for an OK
-------------------------------------------------*/

static UINT32 handler_messagebox_ok(running_machine *machine, UINT32 state)
{
	int res;

	/* draw a standard message window */
	ui_draw_text_box(astring_c(messagebox_text), JUSTIFY_LEFT, 0.5f, 0.5f, messagebox_backcolor);

	res = ui_window_scroll_keys(machine);
	if (res == 0)
	{
		/* an 'O' or left joystick kicks us to the next state */
		if (state == 0 && (input_code_pressed_once(KEYCODE_O) || ui_input_pressed(machine, IPT_UI_LEFT)))
			state++;

		/* a 'K' or right joystick exits the state */
		else if (state == 1 && (input_code_pressed_once(KEYCODE_K) || ui_input_pressed(machine, IPT_UI_RIGHT)))
			state = UI_HANDLER_CANCEL;
	}

	/* if the user cancels, exit out completely */
	if (res == 2)
	{
		mame_schedule_exit(machine);
		state = UI_HANDLER_CANCEL;
	}

	return state;
}


/*-------------------------------------------------
    handler_messagebox_anykey - displays the
    current messagebox_text string and waits for
    any keypress
-------------------------------------------------*/

static UINT32 handler_messagebox_anykey(running_machine *machine, UINT32 state)
{
	int res;

	/* draw a standard message window */
	ui_draw_text_box(astring_c(messagebox_text), JUSTIFY_LEFT, 0.5f, 0.5f, messagebox_backcolor);

	res = ui_window_scroll_keys(machine);
	/* if the user cancels, exit out completely */
	if (res == 2)
	{
		mame_schedule_exit(machine);
		state = UI_HANDLER_CANCEL;
	}

	/* if select key is pressed, just exit */
	if (res == 1 
#ifdef MAMEMESS
	&& has_dummy_image(machine) <= 0
#endif /* MAMEMESS */
	)
		{
			if (input_code_poll_switches(FALSE) != INPUT_CODE_INVALID)
			state = UI_HANDLER_CANCEL;
		}

	return state;
}


/*-------------------------------------------------
    handler_ingame - in-game handler takes care
    of the standard keypresses
-------------------------------------------------*/

static UINT32 handler_ingame(running_machine *machine, UINT32 state)
{
	int is_paused = mame_is_paused(machine);

	/* first draw the FPS counter */
	if (showfps || osd_ticks() < showfps_end)
	{
		ui_draw_text_full_fixed_width(video_get_speed_text(machine), 0.0f, 0.0f, 1.0f,
					JUSTIFY_RIGHT, WRAP_WORD, DRAW_OPAQUE, ARGB_WHITE, ui_bgcolor, NULL, NULL);
	}
	else
		showfps_end = 0;

	/* draw the profiler if visible */
	if (show_profiler)
		ui_draw_text_full(profiler_get_text(machine), 0.0f, 0.0f, 1.0f, JUSTIFY_LEFT, WRAP_WORD, DRAW_OPAQUE, ARGB_WHITE, ui_bgcolor, NULL, NULL);

	/* if we're single-stepping, pause now */
	if (single_step)
	{
		mame_pause(machine, TRUE);
		single_step = FALSE;
	}

// mamep: we want to use both window UI and in-game UI
#if 0 //def MESS
	if (ui_mess_handler_ingame(machine))
		return 0;
#endif /* MESS */

	/* if the user pressed ESC, stop the emulation */
	if (ui_input_pressed(machine, IPT_UI_CANCEL))
		return ui_set_handler(handler_confirm_quit, 0);

	/* turn on menus if requested */
	if (ui_input_pressed(machine, IPT_UI_CONFIGURE))
		return ui_set_handler(ui_menu_ui_handler, 0);

	if (options_get_bool(mame_options(), OPTION_CHEAT) && ui_input_pressed(machine, IPT_UI_CHEAT))
		return ui_set_handler(ui_menu_ui_handler, SHORTCUT_MENU_CHEAT);

#ifdef CMD_LIST
	if (ui_input_pressed(machine, IPT_UI_COMMAND))
		return ui_set_handler(ui_menu_ui_handler, SHORTCUT_MENU_COMMAND);
#endif /* CMD_LIST */

	/* if the on-screen display isn't up and the user has toggled it, turn it on */
//  if ((machine->debug_flags & DEBUG_FLAG_ENABLED) == 0 && ui_input_pressed(machine, IPT_UI_ON_SCREEN_DISPLAY))
//      return ui_set_handler(ui_slider_ui_handler, 0);

	/* handle a reset request */
	if (ui_input_pressed(machine, IPT_UI_RESET_MACHINE))
		mame_schedule_hard_reset(machine);
	if (ui_input_pressed(machine, IPT_UI_SOFT_RESET))
		mame_schedule_soft_reset(machine);

	/* handle a request to display graphics/palette */
	if (ui_input_pressed(machine, IPT_UI_SHOW_GFX))
	{
		if (!is_paused)
			mame_pause(machine, TRUE);
		return ui_set_handler(ui_gfx_ui_handler, is_paused);
	}

	/* handle a save state request */
	if (ui_input_pressed(machine, IPT_UI_SAVE_STATE))
	{
		mame_pause(machine, TRUE);
		return ui_set_handler(handler_load_save, LOADSAVE_SAVE);
	}

	/* handle a load state request */
	if (ui_input_pressed(machine, IPT_UI_LOAD_STATE))
	{
		mame_pause(machine, TRUE);
		return ui_set_handler(handler_load_save, LOADSAVE_LOAD);
	}

	/* handle a save snapshot request */
	if (ui_input_pressed(machine, IPT_UI_SNAPSHOT))
		video_save_active_screen_snapshots(machine);

#ifdef INP_CAPTION
	draw_caption(machine);
#endif /* INP_CAPTION */

	/* toggle pause */
	if (ui_input_pressed(machine, IPT_UI_PAUSE))
	{
		/* with a shift key, it is single step */
		if (is_paused && (input_code_pressed(KEYCODE_LSHIFT) || input_code_pressed(KEYCODE_RSHIFT)))
		{
			single_step = TRUE;
			mame_pause(machine, FALSE);
		}
		else
			mame_pause(machine, !mame_is_paused(machine));
	}


#ifdef USE_SHOW_TIME
	if (ui_input_pressed(machine, IPT_UI_TIME))
	{
		if (show_time)
		{
			Show_Time_Position++;

			if (Show_Time_Position > 3)
			{
				Show_Time_Position = 0;
				show_time = 0;
			}
		}
		else
		{
			Show_Time_Position = 0;
			show_time = 1;
		}
	}

	if (show_time)
		display_time();
#endif /* USE_SHOW_TIME */

#ifdef USE_SHOW_INPUT_LOG
	if (ui_input_pressed(machine, IPT_UI_SHOW_INPUT_LOG))
	{
		show_input_log ^= 1;
		command_buffer[0].code = '\0';
	}

	/* show popup message if input exist any log */
	if (show_input_log)
		display_input_log();
#endif /* USE_SHOW_INPUT_LOG */

	/* toggle movie recording */
	if (ui_input_pressed(machine, IPT_UI_RECORD_MOVIE))
	{
		if (!video_mng_is_movie_active(machine))
		{
			video_mng_begin_recording(machine, NULL);
			popmessage(_("REC START"));
		}
		else
		{
			video_mng_end_recording(machine);
			popmessage(_("REC STOP"));
		}
	}

	/* toggle profiler display */
	if (ui_input_pressed(machine, IPT_UI_SHOW_PROFILER))
		ui_set_show_profiler(!ui_get_show_profiler());

	/* toggle FPS display */
	if (ui_input_pressed(machine, IPT_UI_SHOW_FPS))
		ui_set_show_fps(!ui_get_show_fps());

	/* toggle crosshair display */
	if (ui_input_pressed(machine, IPT_UI_TOGGLE_CROSSHAIR))
		crosshair_toggle(machine);

	/* increment frameskip? */
	if (ui_input_pressed(machine, IPT_UI_FRAMESKIP_INC))
	{
		/* get the current value and increment it */
		int newframeskip = video_get_frameskip() + 1;
		if (newframeskip > MAX_FRAMESKIP)
			newframeskip = -1;
		video_set_frameskip(newframeskip);

		/* display the FPS counter for 2 seconds */
		ui_show_fps_temp(2.0);
	}

	/* decrement frameskip? */
	if (ui_input_pressed(machine, IPT_UI_FRAMESKIP_DEC))
	{
		/* get the current value and decrement it */
		int newframeskip = video_get_frameskip() - 1;
		if (newframeskip < -1)
			newframeskip = MAX_FRAMESKIP;
		video_set_frameskip(newframeskip);

		/* display the FPS counter for 2 seconds */
		ui_show_fps_temp(2.0);
	}

	/* toggle throttle? */
	if (ui_input_pressed(machine, IPT_UI_THROTTLE))
		video_set_throttle(!video_get_throttle());

	/* check for fast forward */
	if (input_type_pressed(machine, IPT_UI_FAST_FORWARD, 0))
	{
		video_set_fastforward(TRUE);
		ui_show_fps_temp(0.5);
	}
	else
		video_set_fastforward(FALSE);

	return 0;
}


/*-------------------------------------------------
    handler_load_save - leads the user through
    specifying a game to save or load
-------------------------------------------------*/

static UINT32 handler_load_save(running_machine *machine, UINT32 state)
{
	char filename[20];
	input_code code;
	char file = 0;

	/* if we're not in the middle of anything, skip */
	if (state == LOADSAVE_NONE)
		return 0;

	/* okay, we're waiting for a key to select a slot; display a message */
	if (state == LOADSAVE_SAVE)
		ui_draw_message_window(_("Select position to save to"));
	else
		ui_draw_message_window(_("Select position to load from"));

	/* check for cancel key */
	if (ui_input_pressed(machine, IPT_UI_CANCEL))
	{
		/* display a popup indicating things were cancelled */
		if (state == LOADSAVE_SAVE)
			popmessage(_("Save cancelled"));
		else
			popmessage(_("Load cancelled"));

		/* reset the state */
		mame_pause(machine, FALSE);
		return UI_HANDLER_CANCEL;
	}

	/* check for A-Z or 0-9 */
	for (code = KEYCODE_A; code <= (input_code)KEYCODE_Z; code++)
		if (input_code_pressed_once(code))
			file = code - KEYCODE_A + 'a';
	if (file == 0)
		for (code = KEYCODE_0; code <= (input_code)KEYCODE_9; code++)
			if (input_code_pressed_once(code))
				file = code - KEYCODE_0 + '0';
	if (file == 0)
		for (code = KEYCODE_0_PAD; code <= (input_code)KEYCODE_9_PAD; code++)
			if (input_code_pressed_once(code))
				file = code - KEYCODE_0_PAD + '0';
	if (file == 0)
		return state;

	/* display a popup indicating that the save will proceed */
	sprintf(filename, "%c", file);
	if (state == LOADSAVE_SAVE)
	{
		popmessage(_("Save to position %c"), file);
		mame_schedule_save(machine, filename);
	}
	else
	{
		popmessage(_("Load from position %c"), file);
		mame_schedule_load(machine, filename);
	}

	/* remove the pause and reset the state */
	mame_pause(machine, FALSE);
	return UI_HANDLER_CANCEL;
}



static UINT32 handler_confirm_quit(running_machine *machine, UINT32 state)
{
	const char *quit_message =
		"Quit the game?\n\n"
		"Press Select key/button to quit,\n"
		"Cancel key/button to continue.";

	if (!options_get_bool(mame_options(), OPTION_CONFIRM_QUIT))
	{
		mame_schedule_exit(machine);
		return ui_set_handler(ui_menu_ui_handler, 0);
	}

	ui_draw_message_window(_(quit_message));

	if (ui_input_pressed(machine, IPT_UI_SELECT))
	{
		mame_schedule_exit(machine);
		return ui_set_handler(ui_menu_ui_handler, 0);
	}

	if (ui_input_pressed(machine, IPT_UI_CANCEL))
	{
		return UI_HANDLER_CANCEL;
	}

	return 0;
}


/***************************************************************************
    SLIDER CONTROLS
***************************************************************************/

/*-------------------------------------------------
    ui_get_slider_list - get the list of sliders
-------------------------------------------------*/

const slider_state *ui_get_slider_list(void)
{
	return slider_list;
}


/*-------------------------------------------------
    slider_alloc - allocate a new slider entry
-------------------------------------------------*/

static slider_state *slider_alloc(const char *title, INT32 minval, INT32 defval, INT32 maxval, INT32 incval, slider_update update, void *arg)
{
	int size = sizeof(slider_state) + strlen(title);
	slider_state *state = auto_malloc(size);
	memset(state, 0, size);

	state->minval = minval;
	state->defval = defval;
	state->maxval = maxval;
	state->incval = incval;
	state->update = update;
	state->arg = arg;
	strcpy(state->description, title);

	return state;
}


/*-------------------------------------------------
    slider_init - initialize the list of slider
    controls
-------------------------------------------------*/

static slider_state *slider_init(running_machine *machine)
{
	const input_field_config *field;
	const input_port_config *port;
	const device_config *device;
	slider_state *listhead = NULL;
	slider_state **tailptr = &listhead;
	astring *string = astring_alloc();
	int numitems, item;

	/* add overall volume */
	*tailptr = slider_alloc(_("Master Volume"), -32, 0, 0, 1, slider_volume, NULL);
	tailptr = &(*tailptr)->next;

	/* add per-channel volume */
	numitems = sound_get_user_gain_count(machine);
	for (item = 0; item < numitems; item++)
	{
		INT32 maxval = 2000;
		INT32 defval = sound_get_default_gain(machine, item) * 1000.0f + 0.5f;

		if (defval > 1000)
			maxval = 2 * defval;

		astring_printf(string, _("%s Volume"), sound_get_user_gain_name(machine, item));
		*tailptr = slider_alloc(astring_c(string), 0, defval, maxval, 20, slider_mixervol, (void *)(FPTR)item);
		tailptr = &(*tailptr)->next;
	}

	/* add analog adjusters */
	for (port = machine->portconfig; port != NULL; port = port->next)
		for (field = port->fieldlist; field != NULL; field = field->next)
			if (field->type == IPT_ADJUSTER)
			{
				void *param = (void *)field;
				*tailptr = slider_alloc(field->name, 0, field->defvalue, 100, 1, slider_adjuster, param);
				tailptr = &(*tailptr)->next;
			}

	/* add CPU overclocking (cheat only) */
	if (options_get_bool(mame_options(), OPTION_CHEAT))
	{
		numitems = cpu_gettotalcpu();
		for (item = 0; item < numitems; item++)
		{
			astring_printf(string, _("Overclock CPU %s"), machine->config->cpu[item].tag);
			//mamep: 4x overclock
			*tailptr = slider_alloc(astring_c(string), 10, 1000, 4000, 50, slider_overclock, (void *)(FPTR)item);
			tailptr = &(*tailptr)->next;
		}
	}

	/* add screen parameters */
	for (device = video_screen_first(machine->config); device != NULL; device = video_screen_next(device))
	{
		const screen_config *scrconfig = device->inline_config;
		int defxscale = floor(scrconfig->xscale * 1000.0f + 0.5f);
		int defyscale = floor(scrconfig->yscale * 1000.0f + 0.5f);
		int defxoffset = floor(scrconfig->xoffset * 1000.0f + 0.5f);
		int defyoffset = floor(scrconfig->yoffset * 1000.0f + 0.5f);
		void *param = (void *)device;

		/* add refresh rate tweaker */
		if (options_get_bool(mame_options(), OPTION_CHEAT))
		{
			astring_printf(string, _("%s Refresh Rate"), slider_get_screen_desc(device));
			*tailptr = slider_alloc(astring_c(string), -10000, 0, 10000, 1000, slider_refresh, param);
			tailptr = &(*tailptr)->next;
		}

		/* add standard brightness/contrast/gamma controls per-screen */
		astring_printf(string, _("%s Brightness"), slider_get_screen_desc(device));
		*tailptr = slider_alloc(astring_c(string), 100, 1000, 2000, 10, slider_brightness, param);
		tailptr = &(*tailptr)->next;
		astring_printf(string, _("%s Contrast"), slider_get_screen_desc(device));
		*tailptr = slider_alloc(astring_c(string), 100, 1000, 2000, 50, slider_contrast, param);
		tailptr = &(*tailptr)->next;
		astring_printf(string, _("%s Gamma"), slider_get_screen_desc(device));
		*tailptr = slider_alloc(astring_c(string), 100, 1000, 3000, 50, slider_gamma, param);
		tailptr = &(*tailptr)->next;

		/* add scale and offset controls per-screen */
		astring_printf(string, _("%s Horiz Stretch"), slider_get_screen_desc(device));
		*tailptr = slider_alloc(astring_c(string), 500, (defxscale == 0) ? 1000 : defxscale, 1500, 2, slider_xscale, param);
		tailptr = &(*tailptr)->next;
		astring_printf(string, _("%s Horiz Position"), slider_get_screen_desc(device));
		*tailptr = slider_alloc(astring_c(string), -500, defxoffset, 500, 2, slider_xoffset, param);
		tailptr = &(*tailptr)->next;
		astring_printf(string, _("%s Vert Stretch"), slider_get_screen_desc(device));
		*tailptr = slider_alloc(astring_c(string), 500, (defyscale == 0) ? 1000 : defyscale, 1500, 2, slider_yscale, param);
		tailptr = &(*tailptr)->next;
		astring_printf(string, _("%s Vert Position"), slider_get_screen_desc(device));
		*tailptr = slider_alloc(astring_c(string), -500, defyoffset, 500, 2, slider_yoffset, param);
		tailptr = &(*tailptr)->next;
	}

	for (device = device_list_first(machine->config->devicelist, LASERDISC); device != NULL; device = device_list_next(device, LASERDISC))
	{
		const laserdisc_config *config = device->inline_config;
		if (config->overupdate != NULL)
		{
			int defxscale = floor(config->overscalex * 1000.0f + 0.5f);
			int defyscale = floor(config->overscaley * 1000.0f + 0.5f);
			int defxoffset = floor(config->overposx * 1000.0f + 0.5f);
			int defyoffset = floor(config->overposy * 1000.0f + 0.5f);
			void *param = (void *)device;

			/* add scale and offset controls per-overlay */
			astring_printf(string, _("%s Horiz Stretch"), slider_get_laserdisc_desc(device));
			*tailptr = slider_alloc(astring_c(string), 500, (defxscale == 0) ? 1000 : defxscale, 1500, 2, slider_overxscale, param);
			tailptr = &(*tailptr)->next;
			astring_printf(string, _("%s Horiz Position"), slider_get_laserdisc_desc(device));
			*tailptr = slider_alloc(astring_c(string), -500, defxoffset, 500, 2, slider_overxoffset, param);
			tailptr = &(*tailptr)->next;
			astring_printf(string, _("%s Vert Stretch"), slider_get_laserdisc_desc(device));
			*tailptr = slider_alloc(astring_c(string), 500, (defyscale == 0) ? 1000 : defyscale, 1500, 2, slider_overyscale, param);
			tailptr = &(*tailptr)->next;
			astring_printf(string, _("%s Vert Position"), slider_get_laserdisc_desc(device));
			*tailptr = slider_alloc(astring_c(string), -500, defyoffset, 500, 2, slider_overyoffset, param);
			tailptr = &(*tailptr)->next;
		}
	}

	for (device = video_screen_first(machine->config); device != NULL; device = video_screen_next(device))
	{
		const screen_config *scrconfig = device->inline_config;
		if (scrconfig->type == SCREEN_TYPE_VECTOR)
		{
			/* add flicker control */
			*tailptr = slider_alloc(_("Vector Flicker"), 0, 0, 1000, 10, slider_flicker, NULL);
			tailptr = &(*tailptr)->next;
			*tailptr = slider_alloc(_("Beam Width"), 10, 100, 1000, 10, slider_beam, NULL);
			tailptr = &(*tailptr)->next;
			break;
		}
	}

#ifdef MAME_DEBUG
	/* add crosshair adjusters */
	for (port = machine->portconfig; port != NULL; port = port->next)
		for (field = port->fieldlist; field != NULL; field = field->next)
			if (field->crossaxis != CROSSHAIR_AXIS_NONE && field->player == 0)
			{
				void *param = (void *)field;
				astring_printf(string, _("Crosshair Scale %s"), (field->crossaxis == CROSSHAIR_AXIS_X) ? "X" : "Y");
				*tailptr = slider_alloc(astring_c(string), -3000, 1000, 3000, 100, slider_crossscale, param);
				tailptr = &(*tailptr)->next;
				astring_printf(string, _("Crosshair Offset %s"), (field->crossaxis == CROSSHAIR_AXIS_X) ? "X" : "Y");
				*tailptr = slider_alloc(astring_c(string), -3000, 0, 3000, 100, slider_crossoffset, param);
				tailptr = &(*tailptr)->next;
			}
#endif

	astring_free(string);
	return listhead;
}


/*-------------------------------------------------
    slider_volume - global volume slider callback
-------------------------------------------------*/

static INT32 slider_volume(running_machine *machine, void *arg, astring *string, INT32 newval)
	{
	if (newval != SLIDER_NOCHANGE)
		sound_set_attenuation(newval);
	if (string != NULL)
		astring_printf(string, "%3ddB", sound_get_attenuation());
	return sound_get_attenuation();
}


/*-------------------------------------------------
    slider_mixervol - single channel volume
    slider callback
-------------------------------------------------*/

static INT32 slider_mixervol(running_machine *machine, void *arg, astring *string, INT32 newval)
{
	int which = (FPTR)arg;
	if (newval != SLIDER_NOCHANGE)
		sound_set_user_gain(machine, which, (float)newval * 0.001f);
	if (string != NULL)
		astring_printf(string, "%4.2f", sound_get_user_gain(machine, which));
	return floor(sound_get_user_gain(machine, which) * 1000.0f + 0.5f);
}


/*-------------------------------------------------
    slider_adjuster - analog adjuster slider
    callback
-------------------------------------------------*/

static INT32 slider_adjuster(running_machine *machine, void *arg, astring *string, INT32 newval)
{
	const input_field_config *field = arg;
	input_field_user_settings settings;

	input_field_get_user_settings(field, &settings);
	if (newval != SLIDER_NOCHANGE)
	{
		settings.value = newval;
		input_field_set_user_settings(field, &settings);
	}
	if (string != NULL)
		astring_printf(string, "%d%%", settings.value);
	return settings.value;
}


/*-------------------------------------------------
    slider_overclock - CPU overclocker slider
    callback
-------------------------------------------------*/

static INT32 slider_overclock(running_machine *machine, void *arg, astring *string, INT32 newval)
{
	int which = (FPTR)arg;
	if (newval != SLIDER_NOCHANGE)
		cpunum_set_clockscale(machine, which, (float)newval * 0.001f);
	if (string != NULL)
		astring_printf(string, "%3.0f%%", floor(cpunum_get_clockscale(which) * 100.0f + 0.5f));
	return floor(cpunum_get_clockscale(which) * 1000.0f + 0.5f);
}


/*-------------------------------------------------
    slider_refresh - refresh rate slider callback
-------------------------------------------------*/

static INT32 slider_refresh(running_machine *machine, void *arg, astring *string, INT32 newval)
{
	const device_config *screen = arg;
	const screen_config *scrconfig = screen->inline_config;
	double defrefresh = ATTOSECONDS_TO_HZ(scrconfig->refresh);
	double refresh;

	if (newval != SLIDER_NOCHANGE)
	{
		int width = video_screen_get_width(screen);
		int height = video_screen_get_height(screen);
		const rectangle *visarea = video_screen_get_visible_area(screen);

		video_screen_configure(screen, width, height, visarea, HZ_TO_ATTOSECONDS(defrefresh + (double)newval * 0.001));
	}
	if (string != NULL)
		astring_printf(string, "%.3ffps", ATTOSECONDS_TO_HZ(video_screen_get_frame_period(machine->primary_screen).attoseconds));
	refresh = ATTOSECONDS_TO_HZ(video_screen_get_frame_period(machine->primary_screen).attoseconds);
	return floor((refresh - defrefresh) * 1000.0f + 0.5f);
}


/*-------------------------------------------------
    slider_brightness - screen brightness slider
    callback
-------------------------------------------------*/

static INT32 slider_brightness(running_machine *machine, void *arg, astring *string, INT32 newval)
{
	const device_config *screen = arg;
	render_container *container = render_container_get_screen(screen);
	render_container_user_settings settings;

	render_container_get_user_settings(container, &settings);
	if (newval != SLIDER_NOCHANGE)
	{
		settings.brightness = (float)newval * 0.001f;
		render_container_set_user_settings(container, &settings);
	}
	if (string != NULL)
		astring_printf(string, "%.3f", settings.brightness);
	return floor(settings.brightness * 1000.0f + 0.5f);
}


/*-------------------------------------------------
    slider_contrast - screen contrast slider
    callback
-------------------------------------------------*/

static INT32 slider_contrast(running_machine *machine, void *arg, astring *string, INT32 newval)
{
	const device_config *screen = arg;
	render_container *container = render_container_get_screen(screen);
	render_container_user_settings settings;

	render_container_get_user_settings(container, &settings);
	if (newval != SLIDER_NOCHANGE)
	{
		settings.contrast = (float)newval * 0.001f;
		render_container_set_user_settings(container, &settings);
	}
	if (string != NULL)
		astring_printf(string, "%.3f", settings.contrast);
	return floor(settings.contrast * 1000.0f + 0.5f);
}


/*-------------------------------------------------
    slider_gamma - screen gamma slider callback
-------------------------------------------------*/

static INT32 slider_gamma(running_machine *machine, void *arg, astring *string, INT32 newval)
{
	const device_config *screen = arg;
	render_container *container = render_container_get_screen(screen);
	render_container_user_settings settings;

	render_container_get_user_settings(container, &settings);
	if (newval != SLIDER_NOCHANGE)
	{
		settings.gamma = (float)newval * 0.001f;
		render_container_set_user_settings(container, &settings);
	}
	if (string != NULL)
		astring_printf(string, "%.3f", settings.gamma);
	return floor(settings.gamma * 1000.0f + 0.5f);
}


/*-------------------------------------------------
    slider_xscale - screen horizontal scale slider
    callback
-------------------------------------------------*/

static INT32 slider_xscale(running_machine *machine, void *arg, astring *string, INT32 newval)
{
	const device_config *screen = arg;
	render_container *container = render_container_get_screen(screen);
	render_container_user_settings settings;

	render_container_get_user_settings(container, &settings);
	if (newval != SLIDER_NOCHANGE)
	{
		settings.xscale = (float)newval * 0.001f;
		render_container_set_user_settings(container, &settings);
	}
	if (string != NULL)
		astring_printf(string, "%.3f", settings.xscale);
	return floor(settings.xscale * 1000.0f + 0.5f);
}


/*-------------------------------------------------
    slider_yscale - screen vertical scale slider
    callback
-------------------------------------------------*/

static INT32 slider_yscale(running_machine *machine, void *arg, astring *string, INT32 newval)
{
	const device_config *screen = arg;
	render_container *container = render_container_get_screen(screen);
	render_container_user_settings settings;

	render_container_get_user_settings(container, &settings);
	if (newval != SLIDER_NOCHANGE)
	{
		settings.yscale = (float)newval * 0.001f;
		render_container_set_user_settings(container, &settings);
	}
	if (string != NULL)
		astring_printf(string, "%.3f", settings.yscale);
	return floor(settings.yscale * 1000.0f + 0.5f);
}


/*-------------------------------------------------
    slider_xoffset - screen horizontal position
    slider callback
-------------------------------------------------*/

static INT32 slider_xoffset(running_machine *machine, void *arg, astring *string, INT32 newval)
{
	const device_config *screen = arg;
	render_container *container = render_container_get_screen(screen);
	render_container_user_settings settings;

	render_container_get_user_settings(container, &settings);
	if (newval != SLIDER_NOCHANGE)
	{
		settings.xoffset = (float)newval * 0.001f;
		render_container_set_user_settings(container, &settings);
	}
	if (string != NULL)
		astring_printf(string, "%.3f", settings.xoffset);
	return floor(settings.xoffset * 1000.0f + 0.5f);
}


/*-------------------------------------------------
    slider_yoffset - screen vertical position
    slider callback
-------------------------------------------------*/

static INT32 slider_yoffset(running_machine *machine, void *arg, astring *string, INT32 newval)
{
	const device_config *screen = arg;
	render_container *container = render_container_get_screen(screen);
	render_container_user_settings settings;

	render_container_get_user_settings(container, &settings);
	if (newval != SLIDER_NOCHANGE)
	{
		settings.yoffset = (float)newval * 0.001f;
		render_container_set_user_settings(container, &settings);
	}
	if (string != NULL)
		astring_printf(string, "%.3f", settings.yoffset);
	return floor(settings.yoffset * 1000.0f + 0.5f);
}


/*-------------------------------------------------
    slider_overxscale - screen horizontal scale slider
    callback
-------------------------------------------------*/

static INT32 slider_overxscale(running_machine *machine, void *arg, astring *string, INT32 newval)
{
	const device_config *laserdisc = arg;
	laserdisc_config settings;

	laserdisc_get_config(laserdisc, &settings);
	if (newval != SLIDER_NOCHANGE)
	{
		settings.overscalex = (float)newval * 0.001f;
		laserdisc_set_config(laserdisc, &settings);
	}
	if (string != NULL)
		astring_printf(string, "%.3f", settings.overscalex);
	return floor(settings.overscalex * 1000.0f + 0.5f);
}


/*-------------------------------------------------
    slider_overyscale - screen vertical scale slider
    callback
-------------------------------------------------*/

static INT32 slider_overyscale(running_machine *machine, void *arg, astring *string, INT32 newval)
{
	const device_config *laserdisc = arg;
	laserdisc_config settings;

	laserdisc_get_config(laserdisc, &settings);
	if (newval != SLIDER_NOCHANGE)
	{
		settings.overscaley = (float)newval * 0.001f;
		laserdisc_set_config(laserdisc, &settings);
	}
	if (string != NULL)
		astring_printf(string, "%.3f", settings.overscaley);
	return floor(settings.overscaley * 1000.0f + 0.5f);
}


/*-------------------------------------------------
    slider_overxoffset - screen horizontal position
    slider callback
-------------------------------------------------*/

static INT32 slider_overxoffset(running_machine *machine, void *arg, astring *string, INT32 newval)
{
	const device_config *laserdisc = arg;
	laserdisc_config settings;

	laserdisc_get_config(laserdisc, &settings);
	if (newval != SLIDER_NOCHANGE)
	{
		settings.overposx = (float)newval * 0.001f;
		laserdisc_set_config(laserdisc, &settings);
	}
	if (string != NULL)
		astring_printf(string, "%.3f", settings.overposx);
	return floor(settings.overposx * 1000.0f + 0.5f);
}


/*-------------------------------------------------
    slider_overyoffset - screen vertical position
    slider callback
-------------------------------------------------*/

static INT32 slider_overyoffset(running_machine *machine, void *arg, astring *string, INT32 newval)
{
	const device_config *laserdisc = arg;
	laserdisc_config settings;

	laserdisc_get_config(laserdisc, &settings);
	if (newval != SLIDER_NOCHANGE)
	{
		settings.overposy = (float)newval * 0.001f;
		laserdisc_set_config(laserdisc, &settings);
	}
	if (string != NULL)
		astring_printf(string, "%.3f", settings.overposy);
	return floor(settings.overposy * 1000.0f + 0.5f);
}


/*-------------------------------------------------
    slider_flicker - vector flicker slider
    callback
-------------------------------------------------*/

static INT32 slider_flicker(running_machine *machine, void *arg, astring *string, INT32 newval)
	{
	if (newval != SLIDER_NOCHANGE)
		vector_set_flicker((float)newval * 0.1f);
	if (string != NULL)
		astring_printf(string, "%1.2f", vector_get_flicker());
	return floor(vector_get_flicker() * 10.0f + 0.5f);
}


/*-------------------------------------------------
    slider_beam - vector beam width slider
    callback
-------------------------------------------------*/

static INT32 slider_beam(running_machine *machine, void *arg, astring *string, INT32 newval)
	{
	if (newval != SLIDER_NOCHANGE)
		vector_set_beam((float)newval * 0.01f);
	if (string != NULL)
		astring_printf(string, "%1.2f", vector_get_beam());
	return floor(vector_get_beam() * 100.0f + 0.5f);
}


/*-------------------------------------------------
    slider_get_screen_desc - returns the
    description for a given screen
-------------------------------------------------*/

static char *slider_get_screen_desc(const device_config *screen)
{
	int screen_count = video_screen_count(screen->machine->config);
	static char descbuf[256];

	if (screen_count > 1)
		sprintf(descbuf, _("Screen '%s'"), screen->tag);
	else
		strcpy(descbuf, _("Screen"));

	return descbuf;
}


/*-------------------------------------------------
    slider_get_laserdisc_desc - returns the
    description for a given laseridsc
-------------------------------------------------*/

static char *slider_get_laserdisc_desc(const device_config *laserdisc)
{
	int ldcount = device_list_items(laserdisc->machine->config->devicelist, LASERDISC);
	static char descbuf[256];

	if (ldcount > 1)
		sprintf(descbuf, _("Laserdisc '%s'"), laserdisc->tag);
	else
		strcpy(descbuf, _("Laserdisc"));

	return descbuf;
}


/*-------------------------------------------------
    slider_crossscale - crosshair scale slider
    callback
-------------------------------------------------*/

#ifdef MAME_DEBUG
static INT32 slider_crossscale(running_machine *machine, void *arg, astring *string, INT32 newval)
{
	input_field_config *field = arg;

	if (newval != SLIDER_NOCHANGE)
		field->crossscale = (float)newval * 0.001f;
	if (string != NULL)
		astring_printf(string, "%s %s %1.3f", _("Crosshair Scale"), (field->crossaxis == CROSSHAIR_AXIS_X) ? "X" : "Y", (float)newval * 0.001f);
	return floor(field->crossscale * 1000.0f + 0.5f);
}
#endif


/*-------------------------------------------------
    slider_crossoffset - crosshair scale slider
    callback
-------------------------------------------------*/

#ifdef MAME_DEBUG
static INT32 slider_crossoffset(running_machine *machine, void *arg, astring *string, INT32 newval)
{
	input_field_config *field = arg;

	if (newval != SLIDER_NOCHANGE)
		field->crossoffset = (float)newval * 0.001f;
	if (string != NULL)
		astring_printf(string, "%s %s %1.3f", _("Crosshair Offset"), (field->crossaxis == CROSSHAIR_AXIS_X) ? "X" : "Y", (float)newval * 0.001f);
	return field->crossoffset;
}
#endif

void ui_auto_pause(void)
{
	auto_pause = 1;
}

static void build_bgtexture(running_machine *machine)
{
#ifdef UI_COLOR_DISPLAY
	float r = (float)RGB_RED(uifont_colortable[UI_FILLCOLOR]);
	float g = (float)RGB_GREEN(uifont_colortable[UI_FILLCOLOR]);
	float b = (float)RGB_BLUE(uifont_colortable[UI_FILLCOLOR]);
#else /* UI_COLOR_DISPLAY */
	UINT8 r = 0x10;
	UINT8 g = 0x10;
	UINT8 b = 0x30;
#endif /* UI_COLOR_DISPLAY */
	UINT8 a = 0xff;
	int i;

#ifdef TRANS_UI
	a = ui_transparency;
#endif /* TRANS_UI */

	bgbitmap = bitmap_alloc(1, 1024, BITMAP_FORMAT_RGB32);
	if (!bgbitmap)
		fatalerror("build_bgtexture failed");

	for (i = 0; i < bgbitmap->height; i++)
	{
		double gradual = (float)(1024 - i) / 1024.0f + 0.1f;

		if (gradual > 1.0f)
			gradual = 1.0f;
		else if (gradual < 0.1f)
			gradual = 0.1f;

		*BITMAP_ADDR32(bgbitmap, i, 0) = MAKE_ARGB(a, (UINT8)(r * gradual), (UINT8)(g * gradual), (UINT8)(b * gradual));
	}

	bgtexture = render_texture_alloc(render_texture_hq_scale, NULL);
	render_texture_set_bitmap(bgtexture, bgbitmap, NULL, 0, TEXFORMAT_ARGB32);
	add_exit_callback(machine, free_bgtexture);
}

static void free_bgtexture(running_machine *machine)
{
	bitmap_free(bgbitmap);
	bgbitmap = NULL;
	render_texture_free(bgtexture);
	bgtexture = NULL;
}
