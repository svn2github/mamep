
typedef struct _ajax_state ajax_state;
struct _ajax_state
{
	/* memory pointers */
//  UINT8 *    paletteram;    // currently this uses generic palette handling

	/* video-related */
	int        layer_colorbase[3], sprite_colorbase, zoom_colorbase;
	UINT8      priority;

	/* misc */
	int        firq_enable;

	/* devices */
	running_device *maincpu;
	running_device *audiocpu;
	running_device *subcpu;
	running_device *k007232_1;
	running_device *k007232_2;
	running_device *k052109;
	running_device *k051960;
	running_device *k051316;
};


/*----------- defined in machine/ajax.c -----------*/

WRITE8_HANDLER( ajax_bankswitch_2_w );
READ8_HANDLER( ajax_ls138_f10_r );
WRITE8_HANDLER( ajax_ls138_f10_w );
MACHINE_START( ajax );
MACHINE_RESET( ajax );
INTERRUPT_GEN( ajax_interrupt );

/*----------- defined in video/ajax.c -----------*/

VIDEO_START( ajax );
VIDEO_UPDATE( ajax );

extern void ajax_tile_callback(running_machine *machine, int layer,int bank,int *code,int *color,int *flags,int *priority);
extern void ajax_sprite_callback(running_machine *machine, int *code,int *color,int *priority,int *shadow);
extern void ajax_zoom_callback(running_machine *machine, int *code,int *color,int *flags);
