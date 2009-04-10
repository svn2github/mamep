/*
	ataridev.h

*/

#ifndef _ATARIDEV_H
#define _ATARIDEV_H

#define ATARI_5200	0
#define ATARI_400	1
#define ATARI_800	2
#define ATARI_600XL 3
#define ATARI_800XL 4

/* defined in ataricrt.c */
MACHINE_START( a400 );
MACHINE_START( a800 );
MACHINE_START( a800xl );
MACHINE_START( a5200 );

DEVICE_IMAGE_LOAD( a800_cart );
DEVICE_IMAGE_UNLOAD( a800_cart );

DEVICE_IMAGE_LOAD( a800xl_cart );
DEVICE_IMAGE_UNLOAD( a800xl_cart );

DEVICE_IMAGE_LOAD( a5200_cart );
DEVICE_IMAGE_UNLOAD( a5200_cart );

/* defined in atarifdc.c */
DEVICE_IMAGE_LOAD( a800_floppy );

#endif /* _ATARIDEV_H */

