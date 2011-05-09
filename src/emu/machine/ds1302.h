/**********************************************************************

    DALLAS DS1302

    RTC + BACKUP RAM

**********************************************************************/

#pragma once

#ifndef __DS1302_H__
#define __DS1302_H__

#include "emu.h"



/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define MCFG_DS1302_ADD(_tag) \
	MCFG_DEVICE_ADD(_tag, DS1302, 0)


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/


// ======================> ds1302_device

class ds1302_device :  public device_t
{
public:
    // construction/destruction
    ds1302_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	void ds1302_dat_w(UINT32 offset, UINT8 data);
	void ds1302_clk_w(UINT32 offset, UINT8 data);
	UINT8 ds1302_read(UINT32 offset);

protected:
    // device-level overrides
    virtual void device_start();
    virtual void device_reset();
    virtual void device_post_load() { }
    virtual void device_clock_changed() { }

private:

	UINT32 m_shift_in;
	UINT8  m_shift_out;
	UINT8  m_icount;
	UINT8  m_last_clk;
	UINT8  m_last_cmd;
	UINT8  m_sram[0x20];
};


// device type definition
extern const device_type DS1302;



/***************************************************************************
    PROTOTYPES
***************************************************************************/

extern WRITE8_DEVICE_HANDLER( ds1302_dat_w );
extern WRITE8_DEVICE_HANDLER( ds1302_clk_w );
extern READ8_DEVICE_HANDLER( ds1302_read );


#endif /* __DS1302_H__ */
