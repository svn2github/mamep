/*************************************************************************

    Tumble Pop

*************************************************************************/

class tumblep_state : public driver_data_t
{
public:
	static driver_data_t *alloc(running_machine &machine) { return auto_alloc_clear(&machine, tumblep_state(machine)); }

	tumblep_state(running_machine &machine)
		: driver_data_t(machine) { }

	/* memory pointers */
	UINT16 *  pf1_rowscroll;
	UINT16 *  pf2_rowscroll;
	UINT16 *  spriteram;
//  UINT16 *  paletteram;    // currently this uses generic palette handling (in deco16ic.c)
	size_t    spriteram_size;

	/* devices */
	running_device *maincpu;
	running_device *audiocpu;
	running_device *deco16ic;
};



/*----------- defined in video/tumblep.c -----------*/

VIDEO_UPDATE( tumblep );
