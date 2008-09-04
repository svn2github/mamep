/*************************************************************************

    laserdsc.h

    Generic laserdisc support.

    Copyright Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

*************************************************************************/

#pragma once

#ifndef __LASERDSC_H__
#define __LASERDSC_H__

#include "sound/custom.h"



/***************************************************************************
    CONSTANTS
***************************************************************************/

/* types of players supported */
enum
{
	LASERDISC_TYPE_PIONEER_PR7820,			/* Pioneer PR-7820 */
	LASERDISC_TYPE_PIONEER_PR8210,			/* Pioneer PR-8210 / LD-V1100 */
	LASERDISC_TYPE_SIMUTREK_SPECIAL,		/* Pioneer PR-8210 with mods */
	LASERDISC_TYPE_PIONEER_LDV1000,			/* Pioneer LD-V1000 */
	LASERDISC_TYPE_PHILLIPS_22VP932,		/* Phillips 22VP932 (PAL) */
	LASERDISC_TYPE_SONY_LDP1450				/* Sony LDP-1450 */
};

/* laserdisc control lines */
#define LASERDISC_LINE_ENTER		0			/* "ENTER" key/line */
#define LASERDISC_LINE_CONTROL		1			/* "CONTROL" line */
#define LASERDISC_INPUT_LINES		2

/* laserdisc status lines */
#define LASERDISC_LINE_READY		0			/* "READY" line */
#define LASERDISC_LINE_STATUS		1			/* "STATUS STROBE" line */
#define LASERDISC_LINE_COMMAND		2			/* "COMMAND STROBE" line */
#define LASERDISC_LINE_DATA_AVAIL	3			/* data available "line" */
#define LASERDISC_OUTPUT_LINES		4

/* laserdisc field codes */
#define LASERDISC_CODE_WHITE_FLAG	0			/* boolean white flag */
#define LASERDISC_CODE_LINE16		1			/* 24-bit line 16 code */
#define LASERDISC_CODE_LINE17		2			/* 24-bit line 17 code */
#define LASERDISC_CODE_LINE18		3			/* 24-bit line 18 code */
#define LASERDISC_CODE_LINE1718		4			/* 24-bit best of line 17/18 code */

/* device configuration */
enum
{
	LDINFO_INT_TYPE = DEVINFO_INT_DEVICE_SPECIFIC
};



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef void (*laserdisc_audio_func)(const device_config *device, int samplerate, int samples, const INT16 *ch0, const INT16 *ch1);

typedef struct _laserdisc_config laserdisc_config;
struct _laserdisc_config
{
	UINT32					type;
	laserdisc_audio_func	audio;

	/* rendering information */
	const char *			screen;

	/* overlay information */
	video_update_func		overupdate;
	UINT32					overwidth, overheight, overformat;
	rectangle				overclip;
	float					overposx, overposy;
	float					overscalex, overscaley;
};



/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define MDRV_LASERDISC_ADD(_tag, _type) \
	MDRV_DEVICE_ADD(_tag, LASERDISC) \
	MDRV_DEVICE_CONFIG_DATA32(laserdisc_config, type, LASERDISC_TYPE_##_type)

#define MDRV_LASERDISC_AUDIO(_func) \
	MDRV_DEVICE_CONFIG_DATAPTR(laserdisc_config, audio, _func)

#define MDRV_LASERDISC_SCREEN(_tag) \
	MDRV_DEVICE_CONFIG_DATAPTR(laserdisc_config, screen, _tag)

#define MDRV_LASERDISC_OVERLAY(_update, _width, _height, _format) \
	MDRV_DEVICE_CONFIG_DATAPTR(laserdisc_config, overupdate, video_update_##_update) \
	MDRV_DEVICE_CONFIG_DATA32(laserdisc_config, overwidth, _width) \
	MDRV_DEVICE_CONFIG_DATA32(laserdisc_config, overheight, _height) \
	MDRV_DEVICE_CONFIG_DATA32(laserdisc_config, overformat, _format)

#define MDRV_LASERDISC_OVERLAY_CLIP(_minx, _maxx, _miny, _maxy) \
	MDRV_DEVICE_CONFIG_DATA32(laserdisc_config, overclip.min_x, _minx) \
	MDRV_DEVICE_CONFIG_DATA32(laserdisc_config, overclip.max_x, _maxx) \
	MDRV_DEVICE_CONFIG_DATA32(laserdisc_config, overclip.min_y, _miny) \
	MDRV_DEVICE_CONFIG_DATA32(laserdisc_config, overclip.max_y, _maxy)

#define MDRV_LASERDISC_OVERLAY_POSITION(_posx, _posy) \
	MDRV_DEVICE_CONFIG_DATAFP32(laserdisc_config, overposx, _posx, 24) \
	MDRV_DEVICE_CONFIG_DATAFP32(laserdisc_config, overposy, _posy, 24)

#define MDRV_LASERDISC_OVERLAY_SCALE(_scalex, _scaley) \
	MDRV_DEVICE_CONFIG_DATAFP32(laserdisc_config, overscalex, _scalex, 24) \
	MDRV_DEVICE_CONFIG_DATAFP32(laserdisc_config, overscaley, _scaley, 24)

#define MDRV_LASERDISC_REMOVE(_tag, _type) \
	MDRV_DEVICE_REMOVE(_tag, _type)


/* use these to add laserdisc screens with proper video update parameters */
#define MDRV_LASERDISC_SCREEN_ADD_NTSC(_tag, _overlayformat) \
	MDRV_VIDEO_ATTRIBUTES(VIDEO_SELF_RENDER) \
	MDRV_VIDEO_UPDATE(laserdisc) \
	\
	MDRV_SCREEN_ADD(_tag, RASTER) \
	MDRV_SCREEN_FORMAT(_overlayformat) \
	MDRV_SCREEN_RAW_PARAMS(XTAL_14_31818MHz*2, 910, 0, 704, 525, 0, 480) \

/* not correct yet; fix me... */
#define MDRV_LASERDISC_SCREEN_ADD_PAL(_tag, _format) \
	MDRV_VIDEO_ATTRIBUTES(VIDEO_SELF_RENDER) \
	MDRV_VIDEO_UPDATE(laserdisc) \
	\
	MDRV_SCREEN_ADD(_tag, RASTER) \
	MDRV_SCREEN_FORMAT(_format) \
	MDRV_SCREEN_RAW_PARAMS(XTAL_14_31818MHz, 910, 0, 704, 525.0/2, 0, 480/2) \



/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

extern const custom_sound_interface laserdisc_custom_interface;



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/


/* ----- core control and status ----- */

/* call this once per field (i.e., 59.94 times/second for NTSC) */
void laserdisc_vsync(const device_config *device);

/* get a bitmap for the current frame (and the frame number) */
UINT32 laserdisc_get_video(const device_config *device, bitmap_t **bitmap);

/* return the raw philips or white flag codes */
UINT32 laserdisc_get_field_code(const device_config *device, UINT8 code);



/* ----- input and output ----- */

/* write to the parallel data port of the player */
void laserdisc_data_w(const device_config *device, UINT8 data);

/* assert or clear a signal line connected to the player */
void laserdisc_line_w(const device_config *device, UINT8 line, UINT8 newstate);

/* read from the parallel data port of the player */
UINT8 laserdisc_data_r(const device_config *device);

/* read the state of a signal line connected to the player */
UINT8 laserdisc_line_r(const device_config *device, UINT8 line);



/* ----- player specifics ----- */

/* specify the "slow" speed of the Pioneer PR-7820 */
void pr7820_set_slow_speed(const device_config *device, double frame_rate_scaler);

/* control the audio squelch of the Simutrek modified players */
void simutrek_set_audio_squelch(const device_config *device, int state);

/* set the callback */
void simutrek_set_cmd_ack_callback(const device_config *device, void (*callback)(void));



/* ----- video interface ----- */

/* enable/disable the video */
void laserdisc_video_enable(const device_config *device, int enable);

/* enable/disable the overlay */
void laserdisc_overlay_enable(const device_config *device, int enable);

/* video update callback */
VIDEO_UPDATE( laserdisc );



/* ----- configuration ----- */

/* return a copy of the current live configuration settings */
void laserdisc_get_config(const device_config *device, laserdisc_config *config);

/* change the current live configuration settings */
void laserdisc_set_config(const device_config *device, const laserdisc_config *config);



/* ----- device interface ----- */

/* device get info callback */
#define LASERDISC DEVICE_GET_INFO_NAME(laserdisc)
DEVICE_GET_INFO( laserdisc );


#endif 	/* __LASERDSC_H__ */
