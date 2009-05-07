/***************************************************************************

    Ninja Gaiden / Tecmo Knights Video Hardware

***************************************************************************/

#include "driver.h"

UINT16 *gaiden_videoram,*gaiden_videoram2,*gaiden_videoram3;
int gaiden_sprite_sizey;
//int raiga_alpha;

static tilemap *text_layer,*foreground,*background;
static bitmap_t *sprite_bitmap, *tile_bitmap_bg, *tile_bitmap_fg;

/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static TILE_GET_INFO( get_bg_tile_info )
{
	UINT16 *videoram1 = &gaiden_videoram3[0x0800];
	UINT16 *videoram2 = gaiden_videoram3;
	SET_TILE_INFO(
			1,
			videoram1[tile_index] & 0x0fff,
			(videoram2[tile_index] & 0xf0) >> 4,
			0);
}

static TILE_GET_INFO( get_fg_tile_info )
{
	UINT16 *videoram1 = &gaiden_videoram2[0x0800];
	UINT16 *videoram2 = gaiden_videoram2;
	SET_TILE_INFO(
			2,
			videoram1[tile_index] & 0x0fff,
			(videoram2[tile_index] & 0xf0) >> 4,
			0);
}

static TILE_GET_INFO( get_fg_tile_info_raiga )
{
	UINT16 *videoram1 = &gaiden_videoram2[0x0800];
	UINT16 *videoram2 = gaiden_videoram2;

	/* bit 3 controls blending */
	tileinfo->category = (videoram2[tile_index] & 0x08) >> 3;

	SET_TILE_INFO(
			2,
			videoram1[tile_index] & 0x0fff,
			((videoram2[tile_index] & 0xf0) >> 4) | (tileinfo->category ? 0x80 : 0x00),
			0);
}

static TILE_GET_INFO( get_tx_tile_info )
{
	UINT16 *videoram1 = &gaiden_videoram[0x0400];
	UINT16 *videoram2 = gaiden_videoram;
	SET_TILE_INFO(
			0,
			videoram1[tile_index] & 0x07ff,
			(videoram2[tile_index] & 0xf0) >> 4,
			0);
}


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

VIDEO_START( gaiden )
{
	/* set up tile layers */
	background = tilemap_create(machine, get_bg_tile_info, tilemap_scan_rows,  16, 16, 64, 32);
	foreground = tilemap_create(machine, get_fg_tile_info, tilemap_scan_rows,  16, 16, 64, 32);
	text_layer = tilemap_create(machine, get_tx_tile_info, tilemap_scan_rows,   8,  8, 32, 32);

	tilemap_set_transparent_pen(background, 0);
	tilemap_set_transparent_pen(foreground, 0);
	tilemap_set_transparent_pen(text_layer, 0);
}

VIDEO_START( raiga )
{
	int width = video_screen_get_width(machine->primary_screen);
	int height = video_screen_get_height(machine->primary_screen);

	/* set up tile layers */
	tile_bitmap_bg = auto_bitmap_alloc(machine, width, height, BITMAP_FORMAT_INDEXED16);
	tile_bitmap_fg = auto_bitmap_alloc(machine, width, height, BITMAP_FORMAT_INDEXED16);

	background = tilemap_create(machine, get_bg_tile_info,	   tilemap_scan_rows,16,16,64,32);
	foreground = tilemap_create(machine, get_fg_tile_info_raiga,tilemap_scan_rows,16,16,64,32);
	text_layer = tilemap_create(machine, get_tx_tile_info,	   tilemap_scan_rows, 8, 8,32,32);

	tilemap_set_transparent_pen(background,0);
	tilemap_set_transparent_pen(foreground,0);
	tilemap_set_transparent_pen(text_layer,0);

	/* set up sprites */
	sprite_bitmap = auto_bitmap_alloc(machine, width, height, BITMAP_FORMAT_INDEXED16);
}

VIDEO_START( drgnbowl )
{
	/* set up tile layers */
	background = tilemap_create(machine, get_bg_tile_info, tilemap_scan_rows,       16, 16, 64, 32);
	foreground = tilemap_create(machine, get_fg_tile_info, tilemap_scan_rows,  16, 16, 64, 32);
	text_layer = tilemap_create(machine, get_tx_tile_info, tilemap_scan_rows,   8,  8, 32, 32);

	tilemap_set_transparent_pen(foreground, 15);
	tilemap_set_transparent_pen(text_layer, 15);

	tilemap_set_scrolldx(background, -248, 248);
	tilemap_set_scrolldx(foreground, -252, 252);
}



/***************************************************************************

  Memory handlers

***************************************************************************/

WRITE16_HANDLER( gaiden_txscrollx_w )
{
	static UINT16 scroll;
	COMBINE_DATA(&scroll);
	tilemap_set_scrollx(text_layer, 0, scroll);
}

WRITE16_HANDLER( gaiden_txscrolly_w )
{
	static UINT16 scroll;
	COMBINE_DATA(&scroll);
	tilemap_set_scrolly(text_layer, 0, scroll);
}

WRITE16_HANDLER( gaiden_fgscrollx_w )
{
	static UINT16 scroll;
	COMBINE_DATA(&scroll);
	tilemap_set_scrollx(foreground, 0, scroll);
}

WRITE16_HANDLER( gaiden_fgscrolly_w )
{
	static UINT16 scroll;
	COMBINE_DATA(&scroll);
	tilemap_set_scrolly(foreground, 0, scroll);
}

WRITE16_HANDLER( gaiden_bgscrollx_w )
{
	static UINT16 scroll;
	COMBINE_DATA(&scroll);
	tilemap_set_scrollx(background, 0, scroll);
}

WRITE16_HANDLER( gaiden_bgscrolly_w )
{
	static UINT16 scroll;
	COMBINE_DATA(&scroll);
	tilemap_set_scrolly(background, 0, scroll);
}

WRITE16_HANDLER( gaiden_videoram3_w )
{
	COMBINE_DATA(&gaiden_videoram3[offset]);
	tilemap_mark_tile_dirty(background,offset & 0x07ff);
}

WRITE16_HANDLER( gaiden_flip_w )
{
	if (ACCESSING_BITS_0_7)
		flip_screen_set(space->machine, data & 1);
}


READ16_HANDLER( gaiden_videoram3_r )
{
   return gaiden_videoram3[offset];
}

WRITE16_HANDLER( gaiden_videoram2_w )
{
	COMBINE_DATA(&gaiden_videoram2[offset]);
	tilemap_mark_tile_dirty(foreground,offset & 0x07ff);
}

READ16_HANDLER( gaiden_videoram2_r )
{
   return gaiden_videoram2[offset];
}

WRITE16_HANDLER( gaiden_videoram_w )
{
	COMBINE_DATA(&gaiden_videoram[offset]);
	tilemap_mark_tile_dirty(text_layer,offset & 0x03ff);
}



/***************************************************************************

  Display refresh

***************************************************************************/

/* mix & blend the paletted 16-bit tile and sprite bitmaps into an RGB 32-bit bitmap */

/* the source data is 3 16-bit indexed bitmaps, we use them to determine which 2 colours
   to blend into the final 32-bit rgb bitmaps, this is currently broken (due to zsolt's core
   changes?) it appears that the sprite drawing is no longer putting the correct raw data
   in the bitmaps? */
static void blendbitmaps(running_machine *machine,
		bitmap_t *dest,bitmap_t *src1,bitmap_t *src2,bitmap_t *src3,
		int sx,int sy,const rectangle *cliprect)
{
	int y,x;
	const pen_t *paldata = machine->pens;

	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
	{
		UINT32 *dd  = BITMAP_ADDR32(dest, y, 0);
		UINT16 *sd1 = BITMAP_ADDR16(src1, y, 0);
		UINT16 *sd2 = BITMAP_ADDR16(src2, y, 0);
		UINT16 *sd3 = BITMAP_ADDR16(src3, y, 0);

		for (x = cliprect->min_x; x <= cliprect->max_x; x++)
		{
			if (sd3[x])
			{
				if (sd2[x])
					dd[x] = paldata[sd2[x] | 0x0400] | paldata[sd3[x]];
				else
					dd[x] = paldata[sd1[x] | 0x0400] | paldata[sd3[x]];
			}
			else
			{
				if (sd2[x])
				{
					if (sd2[x] & 0x800)
						dd[x] = paldata[sd1[x] | 0x0400] | paldata[sd2[x]];
					else
						dd[x] = paldata[sd2[x]];
				}
				else
					dd[x] = paldata[sd1[x]];
			}
		}
	}
}

/* sprite format:
 *
 *  word        bit                 usage
 * --------+-fedcba9876543210-+----------------
 *    0    | ---------------x | flip x
 *         | --------------x- | flip y
 *         | -------------x-- | enable
 *         | ----------x----- | blend
 *         | --------xx------ | sprite-tile priority
 *    1    | xxxxxxxxxxxxxxxx | number
 *    2    | --------xxxx---- | palette
 *         | --------------xx | size: 8x8, 16x16, 32x32, 64x64
 *    3    | xxxxxxxxxxxxxxxx | y position
 *    4    | xxxxxxxxxxxxxxxx | x position
 *    5,6,7|                  | unused
 */

#define NUM_SPRITES 256

static void gaiden_draw_sprites(running_machine *machine, bitmap_t *bitmap, const rectangle *cliprect)
{
	static const UINT8 layout[8][8] =
	{
		{ 0, 1, 4, 5,16,17,20,21},
		{ 2, 3, 6, 7,18,19,22,23},
		{ 8, 9,12,13,24,25,28,29},
		{10,11,14,15,26,27,30,31},
		{32,33,36,37,48,49,52,53},
		{34,35,38,39,50,51,54,55},
		{40,41,44,45,56,57,60,61},
		{42,43,46,47,58,59,62,63}
	};

	const gfx_element *gfx = machine->gfx[3];
	const UINT16 *source = (NUM_SPRITES - 1) * 8 + spriteram16;
	int count = NUM_SPRITES;

	/* draw all sprites from front to back */
	while (count--)
	{
		UINT32 attributes = source[0];
		UINT32 priority_mask;
		int col,row;

		if (attributes & 0x04)
		{
			UINT32 priority = (attributes >> 6) & 3;
			UINT32 flipx = (attributes & 1);
			UINT32 flipy = (attributes & 2);

			UINT32 color = source[2];
			UINT32 sizex = 1 << ((color >> 0) & 3);						/* 1,2,4,8 */
			UINT32 sizey = 1 << ((color >> gaiden_sprite_sizey) & 3);	/* 1,2,4,8 */

			/* raiga needs something like this */
			UINT32 number = (source[1] & (sizex > 2 ? 0x7ff8 : 0x7ffc));

			int ypos = source[3] & 0x01ff;
			int xpos = source[4] & 0x01ff;

			if ((attributes & 0x20) && (video_screen_get_frame_number(machine->primary_screen) & 1))
				goto skip_sprite;

			color = (color >> 4) & 0x0f;

			/* wraparound */
			if (xpos >= 256)
				xpos -= 512;
			if (ypos >= 256)
				ypos -= 512;

			if (flip_screen_get(machine))
			{
				flipx = !flipx;
				flipy = !flipy;

				xpos = 256 - (8 * sizex) - xpos;
				ypos = 256 - (8 * sizey) - ypos;

				if (xpos <= -256)
					xpos += 512;
				if (ypos <= -256)
					ypos += 512;
			}

			/* bg: 1; fg:2; text: 4 */
			switch( priority )
			{
				default:
				case 0x0: priority_mask = 0;					break;
				case 0x1: priority_mask = 0xf0;					break;	/* obscured by text layer */
				case 0x2: priority_mask = 0xf0 | 0xcc;			break;	/* obscured by foreground */
				case 0x3: priority_mask = 0xf0 | 0xcc | 0xaa;	break;	/* obscured by bg and fg  */
			}

			for (row = 0; row < sizey; row++)
			{
				for (col = 0; col < sizex; col++)
				{
					int sx = xpos + 8 * (flipx ? (sizex - 1 - col) : col);
					int sy = ypos + 8 * (flipy ? (sizey - 1 - row) : row);

					pdrawgfx_transpen_raw(bitmap, cliprect, gfx,
						number + layout[row][col],
						gfx->color_base + color * gfx->color_granularity,
						flipx, flipy,
						sx, sy,
						priority_bitmap, priority_mask, 0);
				}
			}
		}
skip_sprite:
		source -= 8;
	}
}


static void raiga_draw_sprites(running_machine *machine, bitmap_t *bitmap_bg, bitmap_t *bitmap_fg, bitmap_t *bitmap_sp, const rectangle *cliprect)
{
	static const UINT8 layout[8][8] =
	{
		{ 0, 1, 4, 5,16,17,20,21},
		{ 2, 3, 6, 7,18,19,22,23},
		{ 8, 9,12,13,24,25,28,29},
		{10,11,14,15,26,27,30,31},
		{32,33,36,37,48,49,52,53},
		{34,35,38,39,50,51,54,55},
		{40,41,44,45,56,57,60,61},
		{42,43,46,47,58,59,62,63}
	};

	const gfx_element *gfx = machine->gfx[3];
	const UINT16 *source = (NUM_SPRITES - 1) * 8 + spriteram16;
	int count = NUM_SPRITES;

	/* draw all sprites from front to back */
	while (count--)
	{
		UINT32 attributes = source[0];
		UINT32 priority_mask;
		int col,row;

		if (attributes & 0x04)
		{
			UINT32 priority = (attributes >> 6) & 3;
			UINT32 flipx = (attributes & 1);
			UINT32 flipy = (attributes & 2);

			UINT32 color = source[2];
			UINT32 sizex = 1 << ((color >> 0) & 3);						/* 1,2,4,8 */
			UINT32 sizey = 1 << ((color >> gaiden_sprite_sizey) & 3);	/* 1,2,4,8 */

			/* raiga needs something like this */
			UINT32 number = (source[1] & (sizex > 2 ? 0x7ff8 : 0x7ffc));

			int ypos = source[3] & 0x01ff;
			int xpos = source[4] & 0x01ff;

			color = (color >> 4) & 0x0f;

			/* wraparound */
			if (xpos >= 256)
				xpos -= 512;
			if (ypos >= 256)
				ypos -= 512;

			if (flip_screen_get(machine))
			{
				flipx = !flipx;
				flipy = !flipy;

				xpos = 256 - (8 * sizex) - xpos;
				ypos = 256 - (8 * sizey) - ypos;

				if (xpos <= -256)
					xpos += 512;
				if (ypos <= -256)
					ypos += 512;
			}

			/* bg: 1; fg:2; text: 4 */
			switch( priority )
			{
				default:
				case 0x0: priority_mask = 0;					break;
				case 0x1: priority_mask = 0xf0;					break;	/* obscured by text layer */
				case 0x2: priority_mask = 0xf0 | 0xcc;			break;	/* obscured by foreground */
				case 0x3: priority_mask = 0xf0 | 0xcc | 0xaa;	break;	/* obscured by bg and fg  */
			}

			/* blending */
			if (attributes & 0x20)
			{
				color |= 0x80;

				for (row = 0; row < sizey; row++)
				{
					for (col = 0; col < sizex; col++)
					{
						int sx = xpos + 8 * (flipx ? (sizex - 1 - col) : col);
						int sy = ypos + 8 * (flipy ? (sizey - 1 - row) : row);

						pdrawgfx_transpen_raw(bitmap_sp, cliprect, gfx,
							number + layout[row][col],
							gfx->color_base + color * gfx->color_granularity,
							flipx, flipy,
							sx, sy,
							priority_bitmap, priority_mask, 0);
					}
				}
			}
			else
			{
				bitmap_t *bitmap = (priority >= 2) ? bitmap_bg : bitmap_fg;

				for (row = 0; row < sizey; row++)
				{
					for (col = 0; col < sizex; col++)
					{
						int sx = xpos + 8 * (flipx ? (sizex - 1 - col) : col);
						int sy = ypos + 8 * (flipy ? (sizey - 1 - row) : row);

						pdrawgfx_transpen_raw(bitmap, cliprect, gfx,
							number + layout[row][col],
							gfx->color_base + color * gfx->color_granularity,
							flipx, flipy,
							sx, sy,
							priority_bitmap, priority_mask, 0);
					}
				}
			}
		}

		source -= 8;
	}
}


/* sprite format:
 *
 *  word        bit                 usage
 * --------+-fedcba9876543210-+----------------
 *    0    | --------xxxxxxxx | sprite code (lower bits)
 *         | ---xxxxx-------- | unused ?
 *    1    | --------xxxxxxxx | y position
 *         | ------x--------- | unused ?
 *    2    | --------xxxxxxxx | x position
 *         | -------x-------- | unused ?
 *    3    | -----------xxxxx | sprite code (upper bits)
 *         | ----------x----- | sprite-tile priority
 *         | ---------x------ | flip x
 *         | --------x------- | flip y
 * 0x400   |-------------xxxx | color
 *         |---------x------- | x position (high bit)
 */

static void drgnbowl_draw_sprites(running_machine *machine, bitmap_t *bitmap, const rectangle *cliprect)
{
	int i, code, color, x, y, flipx, flipy, priority_mask;

	for( i = 0; i < 0x800/2; i += 4 )
	{
		code = (spriteram16[i + 0] & 0xff) | ((spriteram16[i + 3] & 0x1f) << 8);
		y = 256 - (spriteram16[i + 1] & 0xff) - 12;
		x = spriteram16[i + 2] & 0xff;
		color = (spriteram16[(0x800/2) + i] & 0x0f);
		flipx = spriteram16[i + 3] & 0x40;
		flipy = spriteram16[i + 3] & 0x80;

		if(spriteram16[(0x800/2) + i] & 0x80)
			x -= 256;

		x += 256;

		if(spriteram16[i + 3] & 0x20)
			priority_mask = 0xf0 | 0xcc; /* obscured by foreground */
		else
			priority_mask = 0;

		pdrawgfx_transpen_raw(bitmap,cliprect,machine->gfx[3],
				code,
				machine->gfx[3]->color_base + color * machine->gfx[3]->color_granularity,
				flipx,flipy,x,y,
				priority_bitmap, priority_mask,15);

		/* wrap x*/
		pdrawgfx_transpen_raw(bitmap,cliprect,machine->gfx[3],
				code,
				machine->gfx[3]->color_base + color * machine->gfx[3]->color_granularity,
				flipx,flipy,x-512,y,
				priority_bitmap, priority_mask,15);

	}
}

VIDEO_UPDATE( gaiden )
{
	bitmap_fill(priority_bitmap, cliprect, 0);
	bitmap_fill(bitmap, cliprect, 0x200);

	tilemap_draw(bitmap, cliprect, background, 0, 1);
	tilemap_draw(bitmap, cliprect, foreground, 0, 2);
	tilemap_draw(bitmap, cliprect, text_layer, 0, 4);

	gaiden_draw_sprites(screen->machine, bitmap, cliprect);
	return 0;
}

VIDEO_UPDATE( raiga )
{
	bitmap_fill(priority_bitmap,    cliprect, 0);

	bitmap_fill(tile_bitmap_bg, cliprect, 0x200);
	bitmap_fill(tile_bitmap_fg,     cliprect, 0);
	bitmap_fill(sprite_bitmap,      cliprect, 0);

	/* draw tilemaps into a 16-bit bitmap */
	tilemap_draw(tile_bitmap_bg, cliprect,background, 0, 1);
	tilemap_draw(tile_bitmap_fg, cliprect,foreground, 0, 2);
	/* draw the blended tiles at a lower priority
       so sprites covered by them will still be drawn */
	tilemap_draw(tile_bitmap_fg, cliprect,foreground, 1, 0);
	tilemap_draw(tile_bitmap_fg, cliprect,text_layer, 0, 4);

	/* draw sprites into a 16-bit bitmap */
	raiga_draw_sprites(screen->machine, tile_bitmap_bg, tile_bitmap_fg, sprite_bitmap, cliprect);

	/* mix & blend the tilemaps and sprites into a 32-bit bitmap */
	blendbitmaps(screen->machine, bitmap, tile_bitmap_bg, tile_bitmap_fg, sprite_bitmap, 0, 0, cliprect);
	return 0;
}

VIDEO_UPDATE( drgnbowl )
{
	bitmap_fill(priority_bitmap, cliprect, 0);

	tilemap_draw(bitmap, cliprect, background, 0, 1);
	tilemap_draw(bitmap, cliprect, foreground, 0, 2);
	tilemap_draw(bitmap, cliprect, text_layer, 0, 4);
	drgnbowl_draw_sprites(screen->machine, bitmap, cliprect);
	return 0;
}
