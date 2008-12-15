/***************************************************************************

    Cinematronics vector hardware

    driver by Aaron Giles

    Special thanks to Neil Bradley, Zonn Moore, and Jeff Mitchell of the
    Retrocade Alliance

    Games supported:
        * Space Wars
        * Barrier
        * Star Hawk
        * Star Castle
        * Tailgunner
        * Rip Off
        * Speed Freak
        * Sundance
        * Warrior
        * Armor Attack
        * Solar Quest
        * Demon
        * War of the Worlds
        * Boxing Bugs
        * QB-3

    To do:
        * look into bad sample latency

***************************************************************************/

#include "driver.h"
#include "video/vector.h"
#include "cpu/ccpu/ccpu.h"
#include "cinemat.h"
#include "rendlay.h"

#include "armora.lh"
#include "starcas.lh"
#include "solarq.lh"

#define MASTER_CLOCK			XTAL_19_923MHz



static UINT16 *rambase;

static UINT8 coin_detected;
static UINT8 coin_last_reset;

static UINT8 mux_select;



/*************************************
 *
 *  General machine init
 *
 *************************************/

static MACHINE_START( cinemat )
{
	state_save_register_global(machine, coin_detected);
	state_save_register_global(machine, coin_last_reset);
	state_save_register_global(machine, mux_select);
}


MACHINE_RESET( cinemat )
{
	/* reset the coin states */
	coin_detected = 0;
	coin_last_reset = 0;

	/* reset mux select */
	mux_select = 0;
}



/*************************************
 *
 *  General input handlers
 *
 *************************************/

static READ8_HANDLER( inputs_r )
{
	return (input_port_read(space->machine, "INPUTS") >> offset) & 1;
}


static READ8_HANDLER( switches_r )
{
	static const UINT8 switch_shuffle[8] = { 2,5,4,3,0,1,6,7 };
	return (input_port_read(space->machine, "SWITCHES") >> switch_shuffle[offset]) & 1;
}



/*************************************
 *
 *  Coin handlers
 *
 *************************************/

static INPUT_CHANGED( coin_inserted )
{
	/* on the falling edge of a new coin, set the coin_detected flag */
	if (newval == 0)
		coin_detected = 1;
}


static READ8_HANDLER( coin_input_r )
{
	return !coin_detected;
}



/*************************************
 *
 *  General output handlers
 *
 *************************************/

static WRITE8_HANDLER( coin_reset_w )
{
	/* on the rising edge of a coin reset, clear the coin_detected flag */
	if (coin_last_reset != data && data != 0)
		coin_detected = 0;
	coin_last_reset = data;
}


static WRITE8_HANDLER( mux_select_w )
{
	mux_select = data;
	cinemat_sound_control_w(space, 0x07, data);
}



/*************************************
 *
 *  Joystick inputs
 *
 *************************************/

static UINT8 joystick_read(const device_config *device)
{
	if (mame_get_phase(device->machine) != MAME_PHASE_RUNNING)
		return 0;
	else
	{
		int xval = (INT16)(cpu_get_reg(device, CCPU_X) << 4) >> 4;
		return (input_port_read_safe(device->machine, mux_select ? "ANALOGX" : "ANALOGY", 0) - xval) < 0x800;
	}
}



/*************************************
 *
 *  Speed Freak inputs
 *
 *************************************/

static READ8_HANDLER( speedfrk_wheel_r )
{
	static const UINT8 speedfrk_steer[] = {0xe, 0x6, 0x2, 0x0, 0x3, 0x7, 0xf};
	int delta_wheel;

    /* the shift register is cleared once per 'frame' */
    delta_wheel = (INT8)input_port_read(space->machine, "WHEEL") / 8;
    if (delta_wheel > 3)
        delta_wheel = 3;
    else if (delta_wheel < -3)
        delta_wheel = -3;

    return (speedfrk_steer[delta_wheel + 3] >> offset) & 1;
}


static READ8_HANDLER( speedfrk_gear_r )
{
	static int gear = 0x0e;
    int gearval = input_port_read(space->machine, "GEAR");

	/* check the fake gear input port and determine the bit settings for the gear */
	if ((gearval & 0x0f) != 0x0f)
        gear = gearval & 0x0f;

	/* add the start key into the mix -- note that it overlaps 4th gear */
	if (!(input_port_read(space->machine, "INPUTS") & 0x80))
        gear &= ~0x08;

	return (gear >> offset) & 1;
}



/*************************************
 *
 *  Sundance inputs
 *
 *************************************/

static const struct
{
	const char *portname;
	UINT16 bitmask;
} sundance_port_map[16] =
{
	{ "PAD1", 0x155 },	/* bit  0 is set if P1 1,3,5,7,9 is pressed */
	{ NULL, 0 },
	{ NULL, 0 },
	{ NULL, 0 },

	{ NULL, 0 },
	{ NULL, 0 },
	{ NULL, 0 },
	{ NULL, 0 },

	{ "PAD2", 0x1a1 },	/* bit  8 is set if P2 1,6,8,9 is pressed */
	{ "PAD1", 0x1a1 },	/* bit  9 is set if P1 1,6,8,9 is pressed */
	{ "PAD2", 0x155 },	/* bit 10 is set if P2 1,3,5,7,9 is pressed */
	{ NULL, 0 },

	{ "PAD1", 0x093 },	/* bit 12 is set if P1 1,2,5,8 is pressed */
	{ "PAD2", 0x093 },	/* bit 13 is set if P2 1,2,5,8 is pressed */
	{ "PAD1", 0x048 },	/* bit 14 is set if P1 4,8 is pressed */
	{ "PAD2", 0x048 },	/* bit 15 is set if P2 4,8 is pressed */
};


static READ8_HANDLER( sundance_inputs_r )
{
	/* handle special keys first */
	if (sundance_port_map[offset].portname)
		return (input_port_read(space->machine, sundance_port_map[offset].portname) & sundance_port_map[offset].bitmask) ? 0 : 1;
	else
		return (input_port_read(space->machine, "INPUTS") >> offset) & 1;
}



/*************************************
 *
 *  Boxing Bugs inputs
 *
 *************************************/

static READ8_HANDLER( boxingb_dial_r )
{
	int value = input_port_read(space->machine, "DIAL");
	if (!mux_select) offset += 4;
	return (value >> offset) & 1;
}



/*************************************
 *
 *  QB3 inputs & RAM banking
 *
 *************************************/

static READ8_HANDLER( qb3_frame_r )
{
	attotime next_update = video_screen_get_time_until_update(space->machine->primary_screen);
	attotime frame_period = video_screen_get_frame_period(space->machine->primary_screen);
	int percent = next_update.attoseconds / (frame_period.attoseconds / 100);

	/* note this is just an approximation... */
	return (percent >= 10);
}


static WRITE8_HANDLER( qb3_ram_bank_w )
{
	memory_set_bank(space->machine, 1, cpu_get_reg(space->machine->cpu[0], CCPU_P) & 3);
}



/*************************************
 *
 *  Main CPU memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( program_map_4k, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xfff)
	AM_RANGE(0x0000, 0x0fff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( program_map_8k, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0x3fff)
	AM_RANGE(0x0000, 0x0fff) AM_MIRROR(0x1000) AM_ROM
	AM_RANGE(0x2000, 0x2fff) AM_MIRROR(0x1000) AM_ROM AM_REGION("main", 0x1000)
ADDRESS_MAP_END

static ADDRESS_MAP_START( program_map_16k, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0x3fff)
	AM_RANGE(0x0000, 0x3fff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( program_map_32k, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0x7fff)
	AM_RANGE(0x0000, 0x7fff) AM_ROM
ADDRESS_MAP_END


static ADDRESS_MAP_START( data_map, ADDRESS_SPACE_DATA, 16 )
	AM_RANGE(0x0000, 0x00ff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( data_map_qb3, ADDRESS_SPACE_DATA, 16 )
	AM_RANGE(0x0000, 0x03ff) AM_RAMBANK(1) AM_BASE(&rambase)
ADDRESS_MAP_END


static ADDRESS_MAP_START( io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x0f) AM_READ(inputs_r)
	AM_RANGE(0x10, 0x16) AM_READ(switches_r)
	AM_RANGE(0x17, 0x17) AM_READ(coin_input_r)

	AM_RANGE(0x05, 0x05) AM_WRITE(coin_reset_w)
	AM_RANGE(0x06, 0x06) AM_WRITE(cinemat_vector_control_w)
	AM_RANGE(0x00, 0x07) AM_WRITE(cinemat_sound_control_w)
ADDRESS_MAP_END



/*************************************
 *
 *  Port definitions
 *
 *************************************/

static INPUT_PORTS_START( spacewar )
	PORT_START("INPUTS")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Option 3") PORT_CODE(KEYCODE_3_PAD)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Option 8") PORT_CODE(KEYCODE_8_PAD)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Option 4") PORT_CODE(KEYCODE_4_PAD)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Option 9") PORT_CODE(KEYCODE_9_PAD)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Option 1") PORT_CODE(KEYCODE_1_PAD)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Option 6") PORT_CODE(KEYCODE_6_PAD)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Option 2") PORT_CODE(KEYCODE_2_PAD)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Option 7") PORT_CODE(KEYCODE_7_PAD)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_PLAYER(1)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Option 5") PORT_CODE(KEYCODE_5_PAD)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Option 0") PORT_CODE(KEYCODE_0_PAD)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_PLAYER(1)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)

	PORT_START("SWITCHES")
	PORT_DIPNAME( 0x03, 0x00,  "Time" )
	PORT_DIPSETTING(    0x03, "0:45/coin" )
	PORT_DIPSETTING(    0x00, "1:00/coin" )
	PORT_DIPSETTING( 	0x01, "1:30/coin" )
	PORT_DIPSETTING( 	0x02, "2:00/coin" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_CHANGED(coin_inserted, 0)
INPUT_PORTS_END


static INPUT_PORTS_START( barrier )
	PORT_START("INPUTS")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Skill A") PORT_CODE(KEYCODE_A)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Skill B") PORT_CODE(KEYCODE_B)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Skill C") PORT_CODE(KEYCODE_C)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_PLAYER(1)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_PLAYER(1)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_PLAYER(1)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_PLAYER(2)

	PORT_START("SWITCHES")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_CHANGED(coin_inserted, 0)
INPUT_PORTS_END


static INPUT_PORTS_START( speedfrk )
	PORT_START("INPUTS")
	PORT_BIT( 0x000f, IP_ACTIVE_LOW, IPT_SPECIAL ) /* steering wheel, fake below */
	PORT_BIT( 0x0070, IP_ACTIVE_LOW, IPT_SPECIAL ) /* gear shift, fake below */
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1) /* gas */
	PORT_BIT( 0xfe00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("SWITCHES")
	PORT_DIPNAME( 0x03, 0x02, "Extra Time" )
	PORT_DIPSETTING(    0x00, "69" )
	PORT_DIPSETTING(    0x01, "99" )
	PORT_DIPSETTING(    0x02, "129" )
	PORT_DIPSETTING(    0x03, "159" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_CHANGED(coin_inserted, 0)

	PORT_START("WHEEL")
	PORT_BIT( 0xff, 0x00, IPT_DIAL ) PORT_SENSITIVITY(100) PORT_KEYDELTA(10) PORT_RESET

	PORT_START("GEAR")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_NAME("1st gear") PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_NAME("2nd gear") PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_NAME("3rd gear") PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_NAME("4th gear") PORT_PLAYER(2)
INPUT_PORTS_END


/* TODO: 4way or 8way stick? */
static INPUT_PORTS_START( starhawk )
	PORT_START("INPUTS")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x00c0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)

	PORT_START("SWITCHES")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Game_Time ) )
	PORT_DIPSETTING(	0x03, "2:00/4:00" )
	PORT_DIPSETTING(	0x01, "1:30/3:00" )
	PORT_DIPSETTING(	0x02, "1:00/2:00" )
	PORT_DIPSETTING(	0x00, "0:45/1:30" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_DIPNAME( 0x40,	0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_CHANGED(coin_inserted, 0)
INPUT_PORTS_END


static INPUT_PORTS_START( sundance )
	PORT_START("INPUTS")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_SPECIAL ) /* P1 Pad */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("3 Suns") PORT_CODE(KEYCODE_STOP)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Toggle Grid") PORT_CODE(KEYCODE_G)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("4 Suns") PORT_CODE(KEYCODE_SLASH)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_SPECIAL ) /* P2 Pad */
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_SPECIAL ) /* P1 Pad */
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_SPECIAL ) /* P2 Pad */
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("2 Suns") PORT_CODE(KEYCODE_COMMA)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_SPECIAL ) /* P1 Pad */
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_SPECIAL ) /* P2 Pad */
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_SPECIAL ) /* P1 Pad */
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_SPECIAL ) /* P2 Pad */

	PORT_START("SWITCHES")
	PORT_DIPNAME( 0x03, 0x02, "Time" )
	PORT_DIPSETTING(	0x00, "0:45/coin" )
	PORT_DIPSETTING(	0x02, "1:00/coin" )
	PORT_DIPSETTING(	0x01, "1:30/coin" )
	PORT_DIPSETTING(	0x03, "2:00/coin" )
	PORT_DIPNAME( 0x04,	0x00, DEF_STR( Language ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Japanese ) )
	PORT_DIPSETTING(	0x00, DEF_STR( English ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) ) /* supposedly coinage, doesn't work */
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_CHANGED(coin_inserted, 0)

	PORT_START("PAD1")
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P1 Pad 1") PORT_CODE(KEYCODE_7_PAD) PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P1 Pad 2") PORT_CODE(KEYCODE_8_PAD) PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P1 Pad 3") PORT_CODE(KEYCODE_9_PAD) PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P1 Pad 4") PORT_CODE(KEYCODE_4_PAD) PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P1 Pad 5") PORT_CODE(KEYCODE_5_PAD) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P1 Pad 6") PORT_CODE(KEYCODE_6_PAD) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P1 Pad 7") PORT_CODE(KEYCODE_1_PAD) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P1 Pad 8") PORT_CODE(KEYCODE_2_PAD) PORT_PLAYER(1)
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P1 Pad 9") PORT_CODE(KEYCODE_3_PAD) PORT_PLAYER(1)

	PORT_START("PAD2")
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P2 Pad 1") PORT_CODE(KEYCODE_Q) PORT_PLAYER(2)
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P2 Pad 2") PORT_CODE(KEYCODE_W) PORT_PLAYER(2)
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P2 Pad 3") PORT_CODE(KEYCODE_E) PORT_PLAYER(2)
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P2 Pad 4") PORT_CODE(KEYCODE_A) PORT_PLAYER(2)
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P2 Pad 5") PORT_CODE(KEYCODE_S) PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P2 Pad 6") PORT_CODE(KEYCODE_D) PORT_PLAYER(2)
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P2 Pad 7") PORT_CODE(KEYCODE_Z) PORT_PLAYER(2)
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P2 Pad 8") PORT_CODE(KEYCODE_X) PORT_PLAYER(2)
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("P2 Pad 9") PORT_CODE(KEYCODE_C) PORT_PLAYER(2)
INPUT_PORTS_END


static INPUT_PORTS_START( tailg )
	PORT_START("INPUTS")
	PORT_BIT( 0x001f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("SWITCHES")
	PORT_DIPNAME( 0x23, 0x23, "Shield Points" )
	PORT_DIPSETTING(	0x00, "15" )
	PORT_DIPSETTING(	0x02, "20" )
	PORT_DIPSETTING(	0x01, "30" )
	PORT_DIPSETTING(	0x03, "40" )
	PORT_DIPSETTING(	0x20, "50" )
	PORT_DIPSETTING(	0x22, "60" )
	PORT_DIPSETTING(	0x21, "70" )
	PORT_DIPSETTING(	0x23, "80" )
	PORT_DIPNAME( 0x04,	0x04, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_CHANGED(coin_inserted, 0)

	PORT_START("ANALOGX")
	PORT_BIT( 0xfff, 0x800, IPT_AD_STICK_X ) PORT_MINMAX(0x200,0xe00) PORT_SENSITIVITY(100) PORT_KEYDELTA(50)

	PORT_START("ANALOGY")
	PORT_BIT( 0xfff, 0x800, IPT_AD_STICK_Y ) PORT_MINMAX(0x200,0xe00) PORT_SENSITIVITY(100) PORT_KEYDELTA(50)
INPUT_PORTS_END


static INPUT_PORTS_START( warrior )
	PORT_START("INPUTS")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )  PORT_PLAYER(2)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )    PORT_PLAYER(2)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )  PORT_PLAYER(2)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 )        PORT_PLAYER(2)
	PORT_BIT( 0x00e0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )  PORT_PLAYER(1)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )    PORT_PLAYER(1)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )  PORT_PLAYER(1)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 )        PORT_PLAYER(1)
	PORT_BIT( 0xe000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("SWITCHES")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_HIGH )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_CHANGED(coin_inserted, 0)
INPUT_PORTS_END


static INPUT_PORTS_START( armora )
	PORT_START("INPUTS")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0fc0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_PLAYER(1)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_PLAYER(1)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)

	PORT_START("SWITCHES")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_3C ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x40, IP_ACTIVE_HIGH )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_CHANGED(coin_inserted, 0)
INPUT_PORTS_END


static INPUT_PORTS_START( ripoff )
	PORT_START("INPUTS")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0fc0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_PLAYER(1)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_PLAYER(1)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)

	PORT_START("SWITCHES")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x01, "4" )
	PORT_DIPSETTING(	0x03, "8" )
	PORT_DIPSETTING(	0x00, "12" )
	PORT_DIPSETTING(	0x02, "16" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x0c, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x08, DEF_STR( 2C_3C ) )
	PORT_DIPNAME( 0x10,	0x10, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20,	0x00, "Scores" )
	PORT_DIPSETTING(	0x00, "Individual" )
	PORT_DIPSETTING(	0x20, "Combined" )
	PORT_SERVICE( 0x40,	IP_ACTIVE_LOW )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_CHANGED(coin_inserted, 0)
INPUT_PORTS_END


static INPUT_PORTS_START( starcas )
	PORT_START("INPUTS")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0038, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xe000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("SWITCHES")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_3C ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x40, IP_ACTIVE_HIGH )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_CHANGED(coin_inserted, 0)
INPUT_PORTS_END


static INPUT_PORTS_START( solarq )
	PORT_START("INPUTS")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1) /* nova */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1) /* fire */
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1) /* thrust */
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1) /* hyperspace */
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_PLAYER(1)
	PORT_BIT( 0xffc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("SWITCHES")
	PORT_DIPNAME( 0x05, 0x05, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(	0x05, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 2C_3C ) )
	PORT_DIPNAME( 0x02,	0x02, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x02, "25 captures" )
	PORT_DIPSETTING(	0x00, "40 captures" )
	PORT_DIPNAME( 0x18, 0x10, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x18, "2" )
	PORT_DIPSETTING(	0x08, "3" )
	PORT_DIPSETTING(	0x10, "4" )
	PORT_DIPSETTING(	0x00, "5" )
	PORT_DIPNAME( 0x20,	0x20, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x40,	IP_ACTIVE_HIGH )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_CHANGED(coin_inserted, 0)
INPUT_PORTS_END


static INPUT_PORTS_START( boxingb )
	PORT_START("INPUTS")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW,  IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW,  IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW,  IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW,  IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW,  IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW,  IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0fc0, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT( 0xf000, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* dial */

	PORT_START("SWITCHES")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(	0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 2C_3C ) )
	PORT_DIPNAME( 0x04,	0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x04, "3" )
	PORT_DIPSETTING(	0x00, "5" )
	PORT_DIPNAME( 0x08,	0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x00, "30,000" )
	PORT_DIPSETTING(	0x08, "50,000" )
	PORT_DIPNAME( 0x10,	0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20,	0x20, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x40,	IP_ACTIVE_LOW )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_CHANGED(coin_inserted, 0)

	PORT_START("DIAL")
	PORT_BIT( 0xff, 0x00, IPT_DIAL ) PORT_REVERSE PORT_SENSITIVITY(100) PORT_KEYDELTA(5)
INPUT_PORTS_END


static INPUT_PORTS_START( wotw )
	PORT_START("INPUTS")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0038, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xe000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("SWITCHES")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x40, IP_ACTIVE_LOW )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_CHANGED(coin_inserted, 0)
INPUT_PORTS_END


static INPUT_PORTS_START( demon )
	PORT_START("INPUTS")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* also mapped to Button 3, player 2 */

	PORT_START("SWITCHES")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_3C ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3")
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x08, "5" )
	PORT_DIPSETTING(    0x0c, "6" )
	PORT_DIPNAME( 0x30, 0x30, "Starting Difficulty" )
	PORT_DIPSETTING(    0x30, "1" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x00, "10" )
/*  PORT_DIPSETTING(    0x20, "1" )*/
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_CHANGED(coin_inserted, 0)
INPUT_PORTS_END


static INPUT_PORTS_START( qb3 )
	PORT_START("INPUTS")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW,  IPT_JOYSTICKLEFT_UP )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW,  IPT_JOYSTICKLEFT_DOWN )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW,  IPT_JOYSTICKRIGHT_LEFT )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW,  IPT_JOYSTICKRIGHT_UP )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW,  IPT_JOYSTICKRIGHT_RIGHT )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW,  IPT_JOYSTICKRIGHT_DOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW,  IPT_START2 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW,  IPT_BUTTON4 )					// read at $1a5; if 0 add 8 to $25
	PORT_DIPNAME( 0x0200, 0x0200, "Debug" )
	PORT_DIPSETTING(	  0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( On ) )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW,  IPT_BUTTON2 )					// read at $c7; jmp to $3AF1 if 0
	PORT_BIT( 0x0800, IP_ACTIVE_LOW,  IPT_JOYSTICKLEFT_RIGHT )
	PORT_DIPNAME( 0x1000, 0x1000, "Infinite Lives" )
	PORT_DIPSETTING(	  0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( On ) )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW,  IPT_JOYSTICKLEFT_LEFT )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW,  IPT_BUTTON1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW,  IPT_SPECIAL )

	PORT_START("SWITCHES")
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x00, "2" )
	PORT_DIPSETTING(	0x02, "3" )
	PORT_DIPSETTING(	0x01, "4" )
	PORT_DIPSETTING(	0x03, "5" )
	PORT_DIPNAME( 0x04,	0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08,	0x08, DEF_STR( Free_Play ) )	// read at $244, $2c1
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10,	0x00, DEF_STR( Unknown ) )	// read at $27d
	PORT_DIPSETTING(	0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20,	0x20, DEF_STR( Unknown ) )	 // read at $282
	PORT_DIPSETTING(	0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x40,	IP_ACTIVE_LOW )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_CHANGED(coin_inserted, 0)
INPUT_PORTS_END



/*************************************
 *
 *  CPU configurations
 *
 *************************************/

static const ccpu_config config_nojmi =
{
	joystick_read,
	cinemat_vector_callback
};

static const ccpu_config config_jmi =
{
	NULL,
	cinemat_vector_callback
};



/*************************************
 *
 *  Core machine drivers
 *
 *************************************/

static MACHINE_DRIVER_START( cinemat_nojmi_4k )

	/* basic machine hardware */
	MDRV_CPU_ADD("main", CCPU, MASTER_CLOCK/4)
	MDRV_CPU_CONFIG(config_nojmi)
	MDRV_CPU_PROGRAM_MAP(program_map_4k,0)
	MDRV_CPU_DATA_MAP(data_map,0)
	MDRV_CPU_IO_MAP(io_map,0)

	MDRV_MACHINE_START(cinemat)
	MDRV_MACHINE_RESET(cinemat)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_ALWAYS_UPDATE)

	MDRV_SCREEN_ADD("main", VECTOR)
	MDRV_SCREEN_REFRESH_RATE(MASTER_CLOCK/4/16/16/16/16/2)
	MDRV_SCREEN_SIZE(1024, 768)
	MDRV_SCREEN_VISIBLE_AREA(0, 1023, 0, 767)

	MDRV_VIDEO_START(cinemat_bilevel)
	MDRV_VIDEO_UPDATE(cinemat)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( cinemat_jmi_4k )
	MDRV_IMPORT_FROM(cinemat_nojmi_4k)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_CONFIG(config_jmi)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( cinemat_nojmi_8k )
	MDRV_IMPORT_FROM(cinemat_nojmi_4k)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(program_map_8k,0)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( cinemat_jmi_8k )
	MDRV_IMPORT_FROM(cinemat_jmi_4k)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(program_map_8k,0)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( cinemat_jmi_16k )
	MDRV_IMPORT_FROM(cinemat_jmi_4k)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(program_map_16k,0)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( cinemat_jmi_32k )
	MDRV_IMPORT_FROM(cinemat_jmi_4k)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(program_map_32k,0)
MACHINE_DRIVER_END




/*************************************
 *
 *  Game-specific machine drivers
 *
 *************************************/

static MACHINE_DRIVER_START( spacewar )
	MDRV_IMPORT_FROM(cinemat_nojmi_4k)
	MDRV_IMPORT_FROM(spacewar_sound)
	MDRV_VIDEO_UPDATE(spacewar)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( barrier )
	MDRV_IMPORT_FROM(cinemat_jmi_4k)
	MDRV_IMPORT_FROM(barrier_sound)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( speedfrk )
	MDRV_IMPORT_FROM(cinemat_nojmi_8k)
	MDRV_IMPORT_FROM(speedfrk_sound)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( starhawk )
	MDRV_IMPORT_FROM(cinemat_jmi_4k)
	MDRV_IMPORT_FROM(starhawk_sound)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( sundance )
	MDRV_IMPORT_FROM(cinemat_jmi_8k)
	MDRV_IMPORT_FROM(sundance_sound)
	MDRV_VIDEO_START(cinemat_16level)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( tailg )
	MDRV_IMPORT_FROM(cinemat_nojmi_8k)
	MDRV_IMPORT_FROM(tailg_sound)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( warrior )
	MDRV_IMPORT_FROM(cinemat_jmi_8k)
	MDRV_IMPORT_FROM(warrior_sound)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( armora )
	MDRV_IMPORT_FROM(cinemat_jmi_16k)
	MDRV_IMPORT_FROM(armora_sound)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( ripoff )
	MDRV_IMPORT_FROM(cinemat_jmi_8k)
	MDRV_IMPORT_FROM(ripoff_sound)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( starcas )
	MDRV_IMPORT_FROM(cinemat_jmi_8k)
	MDRV_IMPORT_FROM(starcas_sound)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( solarq )
	MDRV_IMPORT_FROM(cinemat_jmi_16k)
	MDRV_IMPORT_FROM(solarq_sound)
	MDRV_VIDEO_START(cinemat_64level)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( boxingb )
	MDRV_IMPORT_FROM(cinemat_jmi_32k)
	MDRV_IMPORT_FROM(boxingb_sound)
	MDRV_SCREEN_MODIFY("main")
	MDRV_SCREEN_VISIBLE_AREA(0, 1024, 0, 788)
	MDRV_VIDEO_START(cinemat_color)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( wotw )
	MDRV_IMPORT_FROM(cinemat_jmi_16k)
	MDRV_SCREEN_MODIFY("main")
	MDRV_SCREEN_VISIBLE_AREA(0, 1120, 0, 767)
	MDRV_IMPORT_FROM(wotw_sound)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( wotwc )
	MDRV_IMPORT_FROM(cinemat_jmi_16k)
	MDRV_IMPORT_FROM(wotwc_sound)
	MDRV_VIDEO_START(cinemat_color)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( demon )
	MDRV_IMPORT_FROM(cinemat_jmi_16k)
	MDRV_IMPORT_FROM(demon_sound)
	MDRV_SCREEN_MODIFY("main")
	MDRV_SCREEN_VISIBLE_AREA(0, 1024, 0, 805)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( qb3 )
	MDRV_IMPORT_FROM(cinemat_jmi_32k)
	MDRV_IMPORT_FROM(qb3_sound)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_DATA_MAP(data_map_qb3,0)
	MDRV_SCREEN_MODIFY("main")
	MDRV_SCREEN_VISIBLE_AREA(0, 1120, 0, 780)
	MDRV_VIDEO_START(cinemat_qb3color)
MACHINE_DRIVER_END




/*************************************
 *
 *  ROM definitions
 *
 *************************************/

#define CCPU_PROMS \
	ROM_REGION( 0x1a0, "proms", ROMREGION_DISPOSE ) \
	ROM_LOAD("prom.f14", 0x000, 0x100, CRC(9edbf536) SHA1(036ad8a231284e05f44b1106d38fc0c7e041b6e8) ) \
	ROM_LOAD("prom.e14", 0x100, 0x020, CRC(29dbfb87) SHA1(d8c40ab010b2ea30f29b2c443819e2b69f376c04) ) \
	ROM_LOAD("prom.d14", 0x120, 0x020, CRC(9a05afbf) SHA1(5d806a42424942ba5ef0b70a1d629315b37f931b) ) \
	ROM_LOAD("prom.c14", 0x140, 0x020, CRC(07492cda) SHA1(32df9148797c23f70db47b840139c40e046dd710) ) \
	ROM_LOAD("prom.j14", 0x160, 0x020, CRC(a481ca71) SHA1(ce145d61686f600cc16b77febfd5c783bf8c13b0) ) \
	ROM_LOAD("prom.e8",  0x180, 0x020, CRC(791ec9e1) SHA1(6f7fcce4aa3be9020595235568381588adaab88e) ) \


ROM_START( spacewar )
	ROM_REGION( 0x1000, "main", 0 )
	ROM_LOAD16_BYTE( "spacewar.1l", 0x0000, 0x0800, CRC(edf0fd53) SHA1(a543d8b95bc77ec061c6b10161a6f3e07401e251) )
	ROM_LOAD16_BYTE( "spacewar.2r", 0x0001, 0x0800, CRC(4f21328b) SHA1(8889f1a9353d6bb1e1078829c1ba77557853739b) )

	CCPU_PROMS
ROM_END


ROM_START( barrier )
	ROM_REGION( 0x1000, "main", 0 )
	ROM_LOAD16_BYTE( "barrier.t7", 0x0000, 0x0800, CRC(7c3d68c8) SHA1(1138029552b73e94522b3b48096befc057d603c7) )
	ROM_LOAD16_BYTE( "barrier.p7", 0x0001, 0x0800, CRC(aec142b5) SHA1(b268936b82e072f38f1f1dd54e0bc88bcdf19925) )

	CCPU_PROMS
ROM_END


ROM_START( speedfrk )
	ROM_REGION( 0x2000, "main", 0 )
	ROM_LOAD16_BYTE( "speedfrk.t7", 0x0000, 0x0800, CRC(3552c03f) SHA1(c233dd064195b336556d7405b51065389b228c78) )
	ROM_LOAD16_BYTE( "speedfrk.p7", 0x0001, 0x0800, CRC(4b90cdec) SHA1(69e2312acdc22ef52236b1c4dfee9f51fcdcaa52) )
	ROM_LOAD16_BYTE( "speedfrk.u7", 0x1000, 0x0800, CRC(616c7cf9) SHA1(3c5bf59a09d85261f69e4b9d499cb7a93d79fb57) )
	ROM_LOAD16_BYTE( "speedfrk.r7", 0x1001, 0x0800, CRC(fbe90d63) SHA1(e42b17133464ae48c90263bba01a7d041e938a05) )

	CCPU_PROMS
ROM_END


ROM_START( starhawk )
	ROM_REGION( 0x1000, "main", 0 )
	ROM_LOAD16_BYTE( "u7", 0x0000, 0x0800, CRC(376e6c5c) SHA1(7d9530ed2e75464578b541f61408ba64ee9d2a95) )
	ROM_LOAD16_BYTE( "r7", 0x0001, 0x0800, CRC(bb71144f) SHA1(79591cd3ef8df78ec26e158f7e82ca0dcd72260d) )

	CCPU_PROMS
ROM_END


ROM_START( sundance )
	ROM_REGION( 0x2000, "main", 0 )
	ROM_LOAD16_BYTE( "sundance.t7", 0x0000, 0x0800, CRC(d5b9cb19) SHA1(72dca386b48a582186898c32123d61b4fd58632e) )
	ROM_LOAD16_BYTE( "sundance.p7", 0x0001, 0x0800, CRC(445c4f20) SHA1(972d0b0613f154ee3347206cae05ee8c36796f84) )
	ROM_LOAD16_BYTE( "sundance.u7", 0x1000, 0x0800, CRC(67887d48) SHA1(be225dbd3508fad2711286834880065a4fc0a2fc) )
	ROM_LOAD16_BYTE( "sundance.r7", 0x1001, 0x0800, CRC(10b77ebd) SHA1(3d43bd47c498d5ea74a7322f8d25dbc0c0187534) )

	CCPU_PROMS
ROM_END


ROM_START( tailg )
	ROM_REGION( 0x2000, "main", 0 )
	ROM_LOAD16_BYTE( "tgunner.t70", 0x0000, 0x0800, CRC(21ec9a04) SHA1(b442f34360d1d4769e7bca73a2d79ce97d335460) )
	ROM_LOAD16_BYTE( "tgunner.p70", 0x0001, 0x0800, CRC(8d7410b3) SHA1(59ead49bd229a873f15334d0999c872d3d6581d4) )
	ROM_LOAD16_BYTE( "tgunner.t71", 0x1000, 0x0800, CRC(2c954ab6) SHA1(9edf189a19b50a9abf458d4ef8ba25b53934385e) )
	ROM_LOAD16_BYTE( "tgunner.p71", 0x1001, 0x0800, CRC(8e2c8494) SHA1(65e461ec4938f9895e5ac31442193e06c8731dc1) )

	CCPU_PROMS
ROM_END


ROM_START( warrior )
	ROM_REGION( 0x2000, "main", 0 )
	ROM_LOAD16_BYTE( "warrior.t7", 0x0000, 0x0800, CRC(ac3646f9) SHA1(515c3acb638fad27fa57f6b438c8ec0b5b76f319) )
	ROM_LOAD16_BYTE( "warrior.p7", 0x0001, 0x0800, CRC(517d3021) SHA1(0483dcaf92c336a07d2c535823348ee886567e85) )
	ROM_LOAD16_BYTE( "warrior.u7", 0x1000, 0x0800, CRC(2e39340f) SHA1(4b3cfb3674dd2a668d4d65e28cb37d7ad20f118d) )
	ROM_LOAD16_BYTE( "warrior.r7", 0x1001, 0x0800, CRC(8e91b502) SHA1(27614c3a8613f49187039cfb05ee96303caf72ba) )

	CCPU_PROMS
ROM_END


ROM_START( armora )
	ROM_REGION( 0x4000, "main", 0 )
	ROM_LOAD16_BYTE( "ar414le.t6", 0x0000, 0x1000, CRC(d7e71f84) SHA1(0b29278a6a698f07eae597bc0a8650e91eaabffa) )
	ROM_LOAD16_BYTE( "ar414lo.p6", 0x0001, 0x1000, CRC(df1c2370) SHA1(b74834d1a591a741892ec41269a831d3590ff766) )
	ROM_LOAD16_BYTE( "ar414ue.u6", 0x2000, 0x1000, CRC(b0276118) SHA1(88f33cb2f46a89819c85f810c7cff812e918391e) )
	ROM_LOAD16_BYTE( "ar414uo.r6", 0x2001, 0x1000, CRC(229d779f) SHA1(0cbdd83eb224146944049346f30d9c72d3ad5f52) )

	CCPU_PROMS
ROM_END

ROM_START( armorap )
	ROM_REGION( 0x4000, "main", 0 )
	ROM_LOAD16_BYTE( "ar414le.t6", 0x0000, 0x1000, CRC(d7e71f84) SHA1(0b29278a6a698f07eae597bc0a8650e91eaabffa) )
	ROM_LOAD16_BYTE( "ar414lo.p6", 0x0001, 0x1000, CRC(df1c2370) SHA1(b74834d1a591a741892ec41269a831d3590ff766) )
	ROM_LOAD16_BYTE( "armorp.u7",  0x2000, 0x1000, CRC(4a86bd8a) SHA1(36647805c40688588dde81c7cbf4fe356b0974fc) )
	ROM_LOAD16_BYTE( "armorp.r7",  0x2001, 0x1000, CRC(d2dd4eae) SHA1(09afaeb0b8f88edb17e42bd2d754af0ae53e609a) )

	CCPU_PROMS
ROM_END

ROM_START( armorar )
	ROM_REGION( 0x4000, "main", 0 )
	ROM_LOAD16_BYTE( "armorr.t7", 0x0000, 0x0800, CRC(256d1ed9) SHA1(8c101356c3fe93f2f49d5dc9d739f3b37cdb98b5) )
	ROM_RELOAD(                   0x1000, 0x0800 )
	ROM_LOAD16_BYTE( "armorr.p7", 0x0001, 0x0800, CRC(bf75c158) SHA1(4d52630ae0ea2ad16bb5f577ad6d21f52e2f0a3c) )
	ROM_RELOAD(                   0x1001, 0x0800 )
	ROM_LOAD16_BYTE( "armorr.u7", 0x2000, 0x0800, CRC(ba68331d) SHA1(871c3f5b6c2845f270e3a272fdb07aed8b527641) )
	ROM_RELOAD(                   0x3000, 0x0800 )
	ROM_LOAD16_BYTE( "armorr.r7", 0x2001, 0x0800, CRC(fa14c0b3) SHA1(37b233f0dac51eaf7d325628a6cced9367b6b6cb) )
	ROM_RELOAD(                   0x3001, 0x0800 )

	CCPU_PROMS
ROM_END


ROM_START( ripoff )
	ROM_REGION( 0x2000, "main", 0 )
	ROM_LOAD16_BYTE( "ripoff.t7", 0x0000, 0x0800, CRC(40c2c5b8) SHA1(bc1f3b540475c9868443a72790a959b1f36b93c6) )
	ROM_LOAD16_BYTE( "ripoff.p7", 0x0001, 0x0800, CRC(a9208afb) SHA1(ea362494855be27a07014832b01e65c1645385d0) )
	ROM_LOAD16_BYTE( "ripoff.u7", 0x1000, 0x0800, CRC(29c13701) SHA1(5e7672deffac1fa8f289686a5527adf7e51eb0bb) )
	ROM_LOAD16_BYTE( "ripoff.r7", 0x1001, 0x0800, CRC(150bd4c8) SHA1(e1e2f0dfec4f53d8ff67b0e990514c304f496b3a) )

	CCPU_PROMS
ROM_END


ROM_START( starcas )
	ROM_REGION( 0x2000, "main", 0 )
	ROM_LOAD16_BYTE( "starcas3.t7", 0x0000, 0x0800, CRC(b5838b5d) SHA1(6ac30be55514cba55180c85af69072b5056d1d4c) )
	ROM_LOAD16_BYTE( "starcas3.p7", 0x0001, 0x0800, CRC(f6bc2f4d) SHA1(ef6f01556b154cfb3e37b2a99d6ea6292e5ec844) )
	ROM_LOAD16_BYTE( "starcas3.u7", 0x1000, 0x0800, CRC(188cd97c) SHA1(c021e93a01e9c65013073de551a8c24fd1a68bde) )
	ROM_LOAD16_BYTE( "starcas3.r7", 0x1001, 0x0800, CRC(c367b69d) SHA1(98354d34ceb03e080b1846611d533be7bdff01cc) )

	CCPU_PROMS
ROM_END

ROM_START( starcasp )
	ROM_REGION( 0x2000, "main", 0 )
	ROM_LOAD16_BYTE( "starcasp.t7", 0x0000, 0x0800, CRC(d2c551a2) SHA1(90b5e1c6988839b812028f1baaea16420c011c08) )
	ROM_LOAD16_BYTE( "starcasp.p7", 0x0001, 0x0800, CRC(baa4e422) SHA1(9035ac675fcbbb93ae3f658339fdfaef47796dab) )
	ROM_LOAD16_BYTE( "starcasp.u7", 0x1000, 0x0800, CRC(26941991) SHA1(4417f2f3e437c1f39ff389362467928f57045d74) )
	ROM_LOAD16_BYTE( "starcasp.r7", 0x1001, 0x0800, CRC(5dd151e5) SHA1(f3b0e2bd3121ac0649938eb2f676d171bcc7d4dd) )

	CCPU_PROMS
ROM_END

ROM_START( starcas1 )
	ROM_REGION( 0x2000, "main", 0 )
	ROM_LOAD16_BYTE( "starcast.t7", 0x0000, 0x0800, CRC(65d0a225) SHA1(e1fbee5ff42dd040ab2e90bbe2189fcb76d6167e) )
	ROM_LOAD16_BYTE( "starcast.p7", 0x0001, 0x0800, CRC(d8f58d9a) SHA1(abba459431dcacc75099b0d340b957be71b89cfd) )
	ROM_LOAD16_BYTE( "starcast.u7", 0x1000, 0x0800, CRC(d4f35b82) SHA1(cd4561ce8e1d0554ac1a8925bbf46d2c676a3b80) )
	ROM_LOAD16_BYTE( "starcast.r7", 0x1001, 0x0800, CRC(9fd3de54) SHA1(17195a490b190e68660829850ff9d702ca1939bb) )

	CCPU_PROMS
ROM_END

ROM_START( starcase )
	ROM_REGION( 0x2000, "main", 0 )
	ROM_LOAD16_BYTE( "starcast.t7", 0x0000, 0x0800, CRC(65d0a225) SHA1(e1fbee5ff42dd040ab2e90bbe2189fcb76d6167e) )
	ROM_LOAD16_BYTE( "starcast.p7", 0x0001, 0x0800, CRC(d8f58d9a) SHA1(abba459431dcacc75099b0d340b957be71b89cfd) )
	ROM_LOAD16_BYTE( "starcast.u7", 0x1000, 0x0800, CRC(d4f35b82) SHA1(cd4561ce8e1d0554ac1a8925bbf46d2c676a3b80) )
	ROM_LOAD16_BYTE( "mottoeis.r7", 0x1001, 0x0800, CRC(a2c1ed52) SHA1(ed9743f44ee98c9e7c2a6819ec681af7c7a97fc9) )

	CCPU_PROMS
ROM_END

ROM_START( stellcas )
	ROM_REGION( 0x2000, "main", 0 )
	ROM_LOAD16_BYTE( "starcast.t7", 0x0000, 0x0800, CRC(65d0a225) SHA1(e1fbee5ff42dd040ab2e90bbe2189fcb76d6167e) )
	ROM_LOAD16_BYTE( "starcast.p7", 0x0001, 0x0800, CRC(d8f58d9a) SHA1(abba459431dcacc75099b0d340b957be71b89cfd) )
	ROM_LOAD16_BYTE( "elttron.u7",  0x1000, 0x0800, CRC(d5b44050) SHA1(a5dd6050ab1a3b0275a229845bc5e9524e2da69c) )
	ROM_LOAD16_BYTE( "elttron.r7",  0x1001, 0x0800, CRC(6f1f261e) SHA1(a22a52af12a5cfbb9031fdd12c9c78db28f28ff1) )

	CCPU_PROMS
ROM_END

ROM_START( spaceftr )
	ROM_REGION( 0x2000, "main", 0 )
	ROM_LOAD16_BYTE( "fortrest7.7t", 0x0000, 0x0800, CRC(65d0a225) SHA1(e1fbee5ff42dd040ab2e90bbe2189fcb76d6167e) )
	ROM_LOAD16_BYTE( "fortresp7.7p", 0x0001, 0x0800, BAD_DUMP CRC(1a32aed6) SHA1(89c16a33288265e06e6fbd8426ba4ee9d81c221f) )
	ROM_LOAD16_BYTE( "fortresu7.7u", 0x1000, 0x0800, CRC(13b0287c) SHA1(366a23fd10684975bd5ee190e5227e47a0298ad5) )
	ROM_LOAD16_BYTE( "fortresr7.7r", 0x1001, 0x0800, CRC(a2c1ed52) SHA1(ed9743f44ee98c9e7c2a6819ec681af7c7a97fc9) )

	CCPU_PROMS
ROM_END


ROM_START( solarq )
	ROM_REGION( 0x4000, "main", 0 )
	ROM_LOAD16_BYTE( "solar.6t", 0x0000, 0x1000, CRC(1f3c5333) SHA1(58d847b5f009a0363ae116768b22d0bcfb3d60a4) )
	ROM_LOAD16_BYTE( "solar.6p", 0x0001, 0x1000, CRC(d6c16bcc) SHA1(6953bdc698da060d37f6bc33a810ba44595b1257) )
	ROM_LOAD16_BYTE( "solar.6u", 0x2000, 0x1000, CRC(a5970e5c) SHA1(9ac07924ca86d003964022cffdd6a0436dde5624) )
	ROM_LOAD16_BYTE( "solar.6r", 0x2001, 0x1000, CRC(b763fff2) SHA1(af1fd978e46a4aee3048e6e36c409821d986f7ee) )

	CCPU_PROMS
ROM_END


ROM_START( boxingb )
	ROM_REGION( 0x8000, "main", 0 )
	ROM_LOAD16_BYTE( "u1a", 0x0000, 0x1000, CRC(d3115b0f) SHA1(9448e7ac1cdb5c7e0739623151be230ab630c4ea) )
	ROM_LOAD16_BYTE( "u1b", 0x0001, 0x1000, CRC(3a44268d) SHA1(876ebe942ded787cfe357563a33d3e26a1483c5a) )
	ROM_LOAD16_BYTE( "u2a", 0x2000, 0x1000, CRC(c97a9cbb) SHA1(8bdeb9ee6b24c0a4554bbf4532a43481a0360019) )
	ROM_LOAD16_BYTE( "u2b", 0x2001, 0x1000, CRC(98d34ff5) SHA1(6767a02a99a01712383300f9acb96cdeffbc9c69) )
	ROM_LOAD16_BYTE( "u3a", 0x4000, 0x1000, CRC(5bb3269b) SHA1(a9dbc91b1455760f10bad0d2ccf540e040a00d4e) )
	ROM_LOAD16_BYTE( "u3b", 0x4001, 0x1000, CRC(85bf83ad) SHA1(9229042e39c53fae56dc93f8996bf3a3fcd35cb8) )
	ROM_LOAD16_BYTE( "u4a", 0x6000, 0x1000, CRC(25b51799) SHA1(46465fe62907ae66a0ce730581e4e9ba330d4369) )
	ROM_LOAD16_BYTE( "u4b", 0x6001, 0x1000, CRC(7f41de6a) SHA1(d01dffad3cb6e76c535a034ea0277dce5801c5f1) )

	CCPU_PROMS
ROM_END


ROM_START( wotw )
	ROM_REGION( 0x4000, "main", 0 )
	ROM_LOAD16_BYTE( "wow_le.t7", 0x0000, 0x1000, CRC(b16440f9) SHA1(9656a26814736f8ff73575063b5ebbb2e8aa7dd0) )
	ROM_LOAD16_BYTE( "wow_lo.p7", 0x0001, 0x1000, CRC(bfdf4a5a) SHA1(db4eceb68e17020d0a597ba105ec3b91ce48b7c1) )
	ROM_LOAD16_BYTE( "wow_ue.u7", 0x2000, 0x1000, CRC(9b5cea48) SHA1(c2bc002e550a0d36e713d07f6aefa79c70b8e284) )
	ROM_LOAD16_BYTE( "wow_uo.r7", 0x2001, 0x1000, CRC(c9d3c866) SHA1(57a47bf06838fe562981321249fe5ae585316f22) )

	CCPU_PROMS
ROM_END

ROM_START( wotwc )
	ROM_REGION( 0x4000, "main", 0 )
	ROM_LOAD16_BYTE( "wow_le.t7", 0x0000, 0x1000, CRC(b16440f9) SHA1(9656a26814736f8ff73575063b5ebbb2e8aa7dd0) )
	ROM_LOAD16_BYTE( "wow_lo.p7", 0x0001, 0x1000, CRC(bfdf4a5a) SHA1(db4eceb68e17020d0a597ba105ec3b91ce48b7c1) )
	ROM_LOAD16_BYTE( "wow_ue.u7", 0x2000, 0x1000, CRC(9b5cea48) SHA1(c2bc002e550a0d36e713d07f6aefa79c70b8e284) )
	ROM_LOAD16_BYTE( "wow_uo.r7", 0x2001, 0x1000, CRC(c9d3c866) SHA1(57a47bf06838fe562981321249fe5ae585316f22) )

	CCPU_PROMS
ROM_END


ROM_START( demon )
	ROM_REGION( 0x4000, "main", 0 )
	ROM_LOAD16_BYTE( "demon.7t",  0x0000, 0x1000, CRC(866596c1) SHA1(65202dcd5c6bf6c11fe76a89682a1505b1870cc9) )
	ROM_LOAD16_BYTE( "demon.7p",  0x0001, 0x1000, CRC(1109e2f1) SHA1(c779b6af1ca09e2e295fc9a0e221ddf283b683ed) )
	ROM_LOAD16_BYTE( "demon.7u",  0x2000, 0x1000, CRC(d447a3c3) SHA1(32f6fb01231aa4f3d93e32d639a89f0cf9624a71) )
	ROM_LOAD16_BYTE( "demon.7r",  0x2001, 0x1000, CRC(64b515f0) SHA1(2dd9a6d784ec1baf31e8c6797ddfdc1423c69470) )

	ROM_REGION( 0x10000, "audio", 0 )
	ROM_LOAD( "demon.snd", 0x0000, 0x1000, CRC(1e2cc262) SHA1(2aae537574ac69c92a3c6400b971e994de88d915) )

	CCPU_PROMS
ROM_END


ROM_START( qb3 )
	ROM_REGION( 0x8000, "main", 0 )
	ROM_LOAD16_BYTE( "qb3_le_t7.bin",  0x0000, 0x2000, CRC(adaaee4c) SHA1(35c6bbb50646a3ddec12f115fcf3f2283e15b0a0) )
	ROM_LOAD16_BYTE( "qb3_lo_p7.bin",  0x0001, 0x2000, CRC(72f6199f) SHA1(ae8f81f218940cfc3aef8f82dfe8cc14220770ce) )
	ROM_LOAD16_BYTE( "qb3_ue_u7.bin",  0x4000, 0x2000, CRC(050a996d) SHA1(bf29236112746b5925b29fb231f152a4bde3f4f9) )
	ROM_LOAD16_BYTE( "qb3_uo_r7.bin",  0x4001, 0x2000, CRC(33fa77a2) SHA1(27a6853f8c2614a2abd7bfb9a62c357797312068) )

	ROM_REGION( 0x10000, "audio", 0 )
	ROM_LOAD( "qb3_snd_u12.bin", 0x0000, 0x1000, CRC(f86663de) SHA1(29c7e75ba22be00d59fc8de5de6d94fcee287a09) )
	ROM_LOAD( "qb3_snd_u11.bin", 0x1000, 0x1000, CRC(32ed58fc) SHA1(483a19f0d540d7d348fce4274fba254ee95bc8d6) )

	CCPU_PROMS
ROM_END



/*************************************
 *
 *  Driver initialization
 *
 *************************************/

static DRIVER_INIT( speedfrk )
{
	memory_install_read8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_IO), 0x00, 0x03, 0, 0, speedfrk_wheel_r);
	memory_install_read8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_IO), 0x04, 0x06, 0, 0, speedfrk_gear_r);
}


static DRIVER_INIT( sundance )
{
	memory_install_read8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_IO), 0x00, 0x0f, 0, 0, sundance_inputs_r);
}


static DRIVER_INIT( tailg )
{
	memory_install_write8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_IO), 0x07, 0x07, 0, 0, mux_select_w);
}


static DRIVER_INIT( boxingb )
{
	memory_install_read8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_IO), 0x0c, 0x0f, 0, 0, boxingb_dial_r);
	memory_install_write8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_IO), 0x07, 0x07, 0, 0, mux_select_w);
}


static DRIVER_INIT( qb3 )
{
	memory_install_read8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_IO), 0x0f, 0x0f, 0, 0, qb3_frame_r);
	memory_install_write8_handler(cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_IO), 0x00, 0x00, 0, 0, qb3_ram_bank_w);

	memory_configure_bank(machine, 1, 0, 4, rambase, 0x100*2);
}



/*************************************
 *
 *  Game drivers
 *
 *************************************/

GAME( 1977, spacewar, 0,       spacewar, spacewar, 0,        ORIENTATION_FLIP_Y,   "Cinematronics", "Space Wars", GAME_SUPPORTS_SAVE )
GAME( 1979, barrier,  0,       barrier,  barrier,  0,        ORIENTATION_FLIP_X ^ ROT270, "Vectorbeam", "Barrier", GAME_SUPPORTS_SAVE )
GAME( 1979, speedfrk, 0,       speedfrk, speedfrk, speedfrk, ORIENTATION_FLIP_Y,   "Vectorbeam", "Speed Freak", GAME_NO_SOUND )
GAME( 1979, starhawk, 0,       starhawk, starhawk, 0,        ORIENTATION_FLIP_Y,   "Cinematronics", "Star Hawk", GAME_SUPPORTS_SAVE )
GAMEL(1979, sundance, 0,       sundance, sundance, sundance, ORIENTATION_FLIP_X ^ ROT270, "Cinematronics", "Sundance", GAME_SUPPORTS_SAVE, layout_voffff20 )
GAMEL(1979, tailg,    0,       tailg,    tailg,    tailg,    ORIENTATION_FLIP_Y,   "Cinematronics", "Tailgunner", GAME_SUPPORTS_SAVE, layout_ho20ffff )
GAME( 1979, warrior,  0,       warrior,  warrior,  0,        ORIENTATION_FLIP_Y,   "Vectorbeam", "Warrior", GAME_SUPPORTS_SAVE )
GAMEL(1980, armora,   0,       armora,   armora,   0,        ORIENTATION_FLIP_Y,   "Cinematronics", "Armor Attack", GAME_SUPPORTS_SAVE, layout_armora )
GAMEL(1980, armorap,  armora,  armora,   armora,   0,        ORIENTATION_FLIP_Y,   "Cinematronics", "Armor Attack (prototype)", GAME_SUPPORTS_SAVE, layout_armora )
GAMEL(1980, armorar,  armora,  armora,   armora,   0,        ORIENTATION_FLIP_Y,   "Cinematronics (Rock-ola license)", "Armor Attack (Rock-ola)", GAME_SUPPORTS_SAVE, layout_armora )
GAME( 1980, ripoff,   0,       ripoff,   ripoff,   0,        ORIENTATION_FLIP_Y,   "Cinematronics", "Rip Off", GAME_SUPPORTS_SAVE )
GAMEL(1980, starcas,  0,       starcas,  starcas,  0,        ORIENTATION_FLIP_Y,   "Cinematronics", "Star Castle (version 3)", GAME_SUPPORTS_SAVE, layout_starcas )
GAMEL(1980, starcas1, starcas, starcas,  starcas,  0,        ORIENTATION_FLIP_Y,   "Cinematronics", "Star Castle (older)", GAME_SUPPORTS_SAVE, layout_starcas )
GAMEL(1980, starcasp, starcas, starcas,  starcas,  0,        ORIENTATION_FLIP_Y,   "Cinematronics", "Star Castle (prototype)", GAME_SUPPORTS_SAVE, layout_starcas )
GAMEL(1980, starcase, starcas, starcas,  starcas,  0,        ORIENTATION_FLIP_Y,   "Cinematronics (Mottoeis license)", "Star Castle (Mottoeis)", GAME_SUPPORTS_SAVE, layout_starcas )
GAMEL(1980, stellcas, starcas, starcas,  starcas,  0,        ORIENTATION_FLIP_Y,   "bootleg", "Stellar Castle (Elettronolo)", GAME_SUPPORTS_SAVE, layout_starcas )
GAMEL(1981, spaceftr, starcas, starcas,  starcas,  0,        ORIENTATION_FLIP_Y,   "Zaccaria", "Space Fortress (Zaccaria)", GAME_SUPPORTS_SAVE | GAME_NOT_WORKING, layout_starcas )
GAMEL(1981, solarq,   0,       solarq,   solarq,   0,        ORIENTATION_FLIP_Y ^ ORIENTATION_FLIP_X, "Cinematronics", "Solar Quest", GAME_SUPPORTS_SAVE, layout_solarq )
GAME( 1981, boxingb,  0,       boxingb,  boxingb,  boxingb,  ORIENTATION_FLIP_Y,   "Cinematronics", "Boxing Bugs", GAME_SUPPORTS_SAVE )
GAME( 1981, wotw,     0,       wotw,     wotw,     0,        ORIENTATION_FLIP_Y,   "Cinematronics", "War of the Worlds", GAME_SUPPORTS_SAVE )
GAME( 1981, wotwc,    wotw,    wotwc,    wotw,     0,        ORIENTATION_FLIP_Y,   "Cinematronics", "War of the Worlds (color)", GAME_SUPPORTS_SAVE )
GAME( 1982, demon,    0,       demon,    demon,    0,        ORIENTATION_FLIP_Y,   "Rock-ola", "Demon", GAME_SUPPORTS_SAVE )
GAME( 1982, qb3,      0,       qb3,      qb3,      qb3,      ORIENTATION_FLIP_Y,   "Rock-ola", "QB-3 (prototype)", GAME_IMPERFECT_GRAPHICS | GAME_SUPPORTS_SAVE )
