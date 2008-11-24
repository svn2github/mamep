/*************************************************************************

  Volfied C-Chip Protection
  =========================

  The C-Chip (Taito TC0030CMD) is an unidentified mask programmed
  microcontroller of some sort with 64 pins.  It probably has
  about 2k of ROM and 8k of RAM.

  Cheat:
    volfied:0:100191:00:001:Complete level with 99.9% Now!
    volfied:0:100192:99:501:Complete level with 99.9% Now! (2/3)
    volfied:0:100193:09:501:Complete level with 99.9% Now! (3/3)

*************************************************************************/

#include "driver.h"
#include "includes/cchip.h"

static UINT8 current_bank = 0;
static UINT8 current_flag = 0;
static UINT8 cc_port = 0;
static UINT8 current_cmd=0;
static UINT8* cchip_ram=0;

static const UINT16 palette_data_01[0x50] =
{
	0x0000, 0xde7b, 0xde03, 0x5e01, 0x5e02, 0xc07b, 0x0000, 0xde7b,
	0x0058, 0x4079, 0x407a, 0x407b, 0xd47b, 0x0000, 0x0000, 0x0000,
	0x0000, 0x104a, 0xce41, 0x8c39, 0x5252, 0xd662, 0x4a31, 0x0000,
	0x1e00, 0x1000, 0x9e01, 0x1e02, 0xde02, 0x0000, 0x0000, 0x0000,
	0x0000, 0xde7b, 0xde03, 0x5e01, 0x5e02, 0xc07b, 0x0000, 0xde7b,
	0x0058, 0x4079, 0x407a, 0x407b, 0xd47b, 0x0000, 0x0000, 0x0000,
	0x0000, 0x104a, 0xce41, 0x8c39, 0x5252, 0xd662, 0x4a31, 0x0000,
	0x1e00, 0x1000, 0x9e01, 0x1e02, 0xde02, 0x0000, 0x0000, 0x0000,
	0x0000, 0xd62a, 0x1002, 0xce01, 0x5a3b, 0xde7b, 0x4a31, 0x0000,
	0x1e00, 0x1000, 0x9e01, 0x1e02, 0xde02, 0x0038, 0x0e38, 0x0000
};

static const UINT16 palette_data_02[0x50] =
{
	0x0000, 0xde7b, 0xde03, 0x5e01, 0x5e02, 0xc07b, 0x0000, 0xde7b,
	0x0058, 0x4079, 0x407a, 0x407b, 0xd47b, 0x0000, 0x0000, 0x0000,
	0x0000, 0x4008, 0x0029, 0xc641, 0x4c52, 0x5473, 0xde7b, 0x1863,
	0x524a, 0xce39, 0x0821, 0x9c01, 0x1200, 0x8001, 0xc002, 0xce39,
	0x0000, 0xde7b, 0xde03, 0x5e01, 0x5e02, 0xc07b, 0x0000, 0xde7b,
	0x0058, 0x4079, 0x407a, 0x407b, 0xd47b, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x4a29, 0xce39, 0xde7b, 0x4001, 0x4002, 0xc003,
	0x9e01, 0x1e00, 0x0078, 0x0e00, 0x5401, 0x0040, 0xde03, 0x1600,
	0x0000, 0x4208, 0x0c39, 0xd061, 0x547a, 0x1472, 0xde7b, 0xde7b,
	0x187b, 0x947a, 0x0821, 0x9e79, 0x1040, 0x8079, 0xc07a, 0x0000
};

static const UINT16 palette_data_03[0x50] =
{
	0x0000, 0xde7b, 0xde03, 0x5e01, 0x5e02, 0xc07b, 0x0000, 0xde7b,
	0x0058, 0x4079, 0x407a, 0x407b, 0xd47b, 0x0000, 0x0000, 0x0000,
	0x0000, 0xc038, 0x4049, 0xc059, 0x406a, 0xc07a, 0x4208, 0x0821,
	0x8c31, 0x1042, 0x9c73, 0x1e03, 0x1a02, 0x0c00, 0x1860, 0x1e78,
	0x0000, 0xde7b, 0xde03, 0x5e01, 0x5e02, 0xc07b, 0x0000, 0xde7b,
	0x0058, 0x4079, 0x407a, 0x407b, 0xd47b, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x4a29, 0xce39, 0xde7b, 0x4001, 0x4002, 0xc003,
	0x9e01, 0x1e00, 0x0078, 0x0e00, 0x5401, 0x0040, 0xde03, 0x1600,
	0x0000, 0xc001, 0x4002, 0x8002, 0xc002, 0xc002, 0x0001, 0xc001,
	0x9201, 0xc002, 0xc003, 0x0003, 0x8002, 0x4001, 0xc002, 0x4003
};

static const UINT16 palette_data_04[0x50] =
{
	0x0000, 0xde7b, 0xde03, 0x5e01, 0x5e02, 0xc07b, 0x0000, 0xde7b,
	0x0058, 0x4079, 0x407a, 0x407b, 0xd47b, 0x0000, 0x0000, 0x0000,
	0x0000, 0x1042, 0xce39, 0x8c31, 0x524a, 0xd65a, 0x4a29, 0x0000,
	0x1e00, 0x1000, 0x8c21, 0xce29, 0x0039, 0x0038, 0x0e38, 0x0038,
	0x0000, 0xde7b, 0xde03, 0x5e01, 0x5e02, 0xc07b, 0x0000, 0xde7b,
	0x0058, 0x4079, 0x407a, 0x407b, 0xd47b, 0x0000, 0x0000, 0x0000,
	0x0000, 0xde7b, 0x1e00, 0xc003, 0x1042, 0xde03, 0x0000, 0xd65a,
	0xce39, 0x8c31, 0x4a29, 0x0078, 0xc07b, 0x1e02, 0x1e78, 0xc003,
	0x0000, 0x1002, 0xce01, 0x8c01, 0x5202, 0xd602, 0x4a01, 0x0000,
	0x1e00, 0x1000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
};

static const UINT16 palette_data_05[0x50] =
{
	0x0000, 0xde7b, 0xde03, 0x5e01, 0x5e02, 0xc07b, 0x0000, 0xde7b,
	0x0058, 0x4079, 0x407a, 0x407b, 0xd47b, 0x0000, 0x0000, 0x0000,
	0x0000, 0x1200, 0x1600, 0x1a00, 0x9e01, 0x8021, 0xc029, 0x0032,
	0x803a, 0x4208, 0x0821, 0x1042, 0xd65a, 0x9c73, 0xde03, 0x5c02,
	0x0000, 0xde7b, 0xde03, 0x5e01, 0x5e02, 0xc07b, 0x0000, 0xde7b,
	0x0058, 0x4079, 0x407a, 0x407b, 0xd47b, 0x0000, 0x0000, 0x0000,
	0x0000, 0xde7b, 0x1e00, 0xc003, 0x1042, 0xde03, 0x0000, 0xd65a,
	0xce39, 0x8c31, 0x4a29, 0x0078, 0xc07b, 0x1e02, 0x1e78, 0xc003,
	0x0000, 0x5202, 0xd602, 0x5a03, 0xde03, 0x8021, 0xc029, 0x0032,
	0x803a, 0x4208, 0x0821, 0x1042, 0xd65a, 0x9c73, 0xde03, 0x5c02
};

static const UINT16 palette_data_06[0x50] =
{
	0x0000, 0xde7b, 0xde03, 0x5e01, 0x5e02, 0xc07b, 0x0000, 0xde7b,
	0x0058, 0x4079, 0x407a, 0x407b, 0xd47b, 0x0000, 0x0000, 0x0000,
	0x0000, 0x9e52, 0x9028, 0x9428, 0x9828, 0x9e28, 0x4208, 0xde7b,
	0xde03, 0x9c02, 0xc03a, 0x0063, 0x586b, 0x9252, 0x8a31, 0x5e31,
	0x0000, 0xde7b, 0xde03, 0x5e01, 0x5e02, 0xc07b, 0x0000, 0xde7b,
	0x0058, 0x4079, 0x407a, 0x407b, 0xd47b, 0x0000, 0x0000, 0x0000,
	0x0000, 0xde7b, 0x1e00, 0xc003, 0x1042, 0xde03, 0x0000, 0xd65a,
	0xce39, 0x8c31, 0x4a29, 0x0078, 0xc07b, 0x1e02, 0x1e78, 0xc003,
	0x0263, 0x9e52, 0x8058, 0x0879, 0x8c79, 0x107a, 0x4208, 0xde7b,
	0xde01, 0x1e01, 0xc03a, 0x0063, 0x586b, 0x9252, 0x8a31, 0x527a
};

static const UINT16 palette_data_07[0x50] =
{
	0x0000, 0xde7b, 0xde03, 0x5e01, 0x5e02, 0xc07b, 0x0000, 0xde7b,
	0x0058, 0x4079, 0x407a, 0x407b, 0xd47b, 0x0000, 0x0000, 0x0000,
	0x0000, 0xc038, 0x4049, 0xc059, 0x406a, 0xc07a, 0x4208, 0x0821,
	0x8c31, 0x1042, 0x9c73, 0x1e03, 0x1a02, 0x0c00, 0x1860, 0x1e78,
	0x0000, 0xde7b, 0xde03, 0x5e01, 0x5e02, 0xc07b, 0x0000, 0xde7b,
	0x0058, 0x4079, 0x407a, 0x407b, 0xd47b, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x4a29, 0xce39, 0xde7b, 0x4001, 0x4002, 0xc003,
	0x9e01, 0x1e00, 0x0078, 0x0e00, 0x5401, 0x0040, 0xde03, 0x1600,
	0x0000, 0x8001, 0x0002, 0x8002, 0x0003, 0x8003, 0x4208, 0x0821,
	0x8c31, 0x1042, 0x9c73, 0x1e00, 0x5c02, 0x0c00, 0x1860, 0x1e78
};

static const UINT16 palette_data_08[0x50] =
{
	0x0000, 0xde7b, 0xde03, 0x5e01, 0x5e02, 0xc07b, 0x0000, 0xde7b,
	0x0058, 0x4079, 0x407a, 0x407b, 0xd47b, 0x0000, 0x0000, 0x0000,
	0x0000, 0x1042, 0xce39, 0x8c31, 0x524a, 0xd65a, 0x4a29, 0x0000,
	0x1e00, 0x1000, 0x9e01, 0x5e02, 0x5e03, 0x0038, 0x0e38, 0x0000,
	0x0000, 0xde7b, 0xde03, 0x5e01, 0x5e02, 0xc07b, 0x0000, 0xde7b,
	0x0058, 0x4079, 0x407a, 0x407b, 0xd47b, 0x0000, 0x0000, 0x0000,
	0x0000, 0xde7b, 0x1e00, 0xc003, 0x1042, 0xde03, 0x0000, 0xd65a,
	0xce39, 0x8c31, 0x4a29, 0x0078, 0xc07b, 0x1e02, 0x1e78, 0xc003,
	0x0000, 0x5202, 0x1002, 0xce19, 0x9432, 0x1843, 0x8c11, 0x0000,
	0x1e00, 0x1000, 0x9e01, 0x5e02, 0x5e03, 0x0038, 0x0e38, 0x0000
};

static const UINT16 palette_data_09[0x50] =
{
	0x0000, 0xde7b, 0xde03, 0x5e01, 0x5e02, 0xc07b, 0x0000, 0xde7b,
	0x0058, 0x4079, 0x407a, 0x407b, 0xd47b, 0x0000, 0x0000, 0x0000,
	0x0000, 0x1048, 0x1250, 0x1458, 0x1660, 0xd418, 0x9e02, 0xc203,
	0x4208, 0x4a29, 0x8c31, 0x1042, 0x1e78, 0x166b, 0x0c38, 0x1868,
	0x0000, 0xde7b, 0xde03, 0x5e01, 0x5e02, 0xc07b, 0x0000, 0xde7b,
	0x0058, 0x4079, 0x407a, 0x407b, 0xd47b, 0x0000, 0x0000, 0x0000,
	0x0000, 0xde7b, 0x1e00, 0xc003, 0x1042, 0xde03, 0x0000, 0xd65a,
	0xce39, 0x8c31, 0x4a29, 0x0078, 0xc07b, 0x1e02, 0x1e78, 0xc003,
	0x0000, 0x1600, 0x1a21, 0x5c29, 0xde39, 0xd418, 0x9e02, 0xc203,
	0x4208, 0x4a29, 0x8c31, 0x1042, 0x1e42, 0x186b, 0x9210, 0x9e31
};

static const UINT16 palette_data_0a[0x50] =
{
	0x0000, 0xde7b, 0xde03, 0x5e01, 0x5e02, 0xc07b, 0x0000, 0xde7b,
	0x0058, 0x4079, 0x407a, 0x407b, 0xd47b, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0038, 0x4a29, 0xce39, 0x9452, 0x9218, 0xde7b,
	0xc001, 0xc003, 0xde03, 0x1403, 0xcc01, 0x4a01, 0x0668, 0x4672,
	0x0000, 0xde7b, 0xde03, 0x5e01, 0x5e02, 0xc07b, 0x0000, 0xde7b,
	0x0058, 0x4079, 0x407a, 0x407b, 0xd47b, 0x0000, 0x0000, 0x0000,
	0x0000, 0xde7b, 0x1e00, 0xc003, 0x1042, 0xde03, 0x0000, 0xd65a,
	0xce39, 0x8c31, 0x4a29, 0x0078, 0xc07b, 0x1e02, 0x1e78, 0xc003,
	0x0000, 0x0000, 0x0038, 0x4a29, 0x5401, 0x9c02, 0x9218, 0xde7b,
	0x0003, 0xc003, 0x5e02, 0xde01, 0x5201, 0xd200, 0x0668, 0x4672
};

static const UINT16 palette_data_0b[0x50] =
{
	0x0000, 0xde7b, 0xde03, 0x5e01, 0x5e02, 0xc07b, 0x0000, 0xde7b,
	0x0058, 0x4079, 0x407a, 0x407b, 0xd47b, 0x0000, 0x0000, 0x0000,
	0x0050, 0x8001, 0xc001, 0x0002, 0xc002, 0xd043, 0x9c73, 0x524a,
	0xce39, 0x8c31, 0x4208, 0xde03, 0x9c02, 0x1e60, 0x1a00, 0x1000,
	0x0000, 0xde7b, 0xde03, 0x5e01, 0x5e02, 0xc07b, 0x0000, 0xde7b,
	0x0058, 0x4079, 0x407a, 0x407b, 0xd47b, 0x0000, 0x0000, 0x0000,
	0x0000, 0xde7b, 0x1e00, 0xc003, 0x1042, 0xde03, 0x0000, 0xd65a,
	0xce39, 0x8c31, 0x4a29, 0x0078, 0xc07b, 0x1e02, 0x1e78, 0xc003,
	0x0000, 0x8c01, 0xce01, 0x1002, 0xd62a, 0xde4b, 0x9c73, 0x5202,
	0xce01, 0x8c01, 0x4208, 0xde03, 0x9c02, 0x1e60, 0x1a00, 0x1000
};

static const UINT16 palette_data_0c[0x50] =
{
	0x0000, 0xde7b, 0xde03, 0x5e01, 0x5e02, 0xc07b, 0x0000, 0xde7b,
	0x0058, 0x4079, 0x407a, 0x407b, 0xd47b, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0038, 0x4a29, 0xce39, 0x9452, 0x9218, 0x9e52,
	0xc001, 0xc003, 0x1e00, 0x1400, 0x0c00, 0x4a01, 0x0668, 0x4672,
	0x0000, 0xde7b, 0xde03, 0x5e01, 0x5e02, 0xc07b, 0x0000, 0xde7b,
	0x0058, 0x4079, 0x407a, 0x407b, 0xd47b, 0x0000, 0x0000, 0x0000,
	0x0000, 0xde7b, 0x1e00, 0xc003, 0x1042, 0xde03, 0x0000, 0xd65a,
	0xce39, 0x8c31, 0x4a29, 0x0078, 0xc07b, 0x1e02, 0x1e78, 0xc003,
	0x0000, 0x0000, 0x0038, 0x4a29, 0xce39, 0x9452, 0x9218, 0xde7b,
	0xc001, 0xc003, 0xde03, 0x1403, 0xcc01, 0x4a01, 0x0668, 0x4672
};

static const UINT16 palette_data_0d[0x50] =
{
	0x0000, 0xde7b, 0xde03, 0x5e01, 0x5e02, 0xc07b, 0x0000, 0xde7b,
	0x0058, 0x4079, 0x407a, 0x407b, 0xd47b, 0x0000, 0x0000, 0x0000,
	0x0078, 0x4208, 0x1052, 0x9462, 0x1873, 0x5a73, 0xde7b, 0x1863,
	0x524a, 0xce39, 0x0821, 0x1600, 0x1000, 0xd201, 0xde03, 0x0a42,
	0x0000, 0xde7b, 0xde03, 0x5e01, 0x5e02, 0xc07b, 0x0000, 0xde7b,
	0x0058, 0x4079, 0x407a, 0x407b, 0xd47b, 0x0000, 0x0000, 0x0000,
	0x0078, 0x4208, 0x1052, 0x9462, 0x1873, 0x5a73, 0xde7b, 0x1863,
	0x524a, 0xce39, 0x0821, 0x1600, 0x1000, 0xd201, 0xde03, 0x0a42,
	0x0000, 0x4208, 0x5029, 0x9431, 0xd839, 0x5a4a, 0x9e52, 0x5862,
	0xde4b, 0x8e39, 0x0821, 0x1600, 0x1000, 0xd201, 0x1e00, 0x0a42
};

static const UINT16 palette_data_0e[0x50] =
{
	0x0000, 0xde7b, 0xde03, 0x5e01, 0x5e02, 0xc07b, 0x0000, 0xde7b,
	0x0058, 0x4079, 0x407a, 0x407b, 0xd47b, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0e01, 0x5001, 0x9201, 0xd401, 0x1602, 0x1200, 0x1600,
	0x4208, 0x0821, 0x8c31, 0x1042, 0x5a6b, 0x8001, 0x0002, 0x9a02,
	0x0000, 0xde7b, 0xde03, 0x5e01, 0x5e02, 0xc07b, 0x0000, 0xde7b,
	0x0058, 0x4079, 0x407a, 0x407b, 0xd47b, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x4a29, 0xce39, 0xde7b, 0x4001, 0x4002, 0xc003,
	0x9e01, 0x1e00, 0x0078, 0x0e00, 0x5401, 0x0040, 0xde03, 0x1600,
	0x0000, 0x8a21, 0x0a32, 0x4c3a, 0x8e4a, 0x504b, 0xd203, 0xc003,
	0x4208, 0x0821, 0x8c31, 0x1042, 0x5a6b, 0x8001, 0x0002, 0x545b
};

static const UINT16 palette_data_0f[0x50] =
{
	0x0000, 0xde7b, 0xde03, 0x5e01, 0x5e02, 0xc07b, 0x0000, 0xde7b,
	0x0058, 0x4079, 0x407a, 0x407b, 0xd47b, 0x0000, 0x0000, 0x0000,
	0x0000, 0xc038, 0x4049, 0xc059, 0x406a, 0xc07a, 0x0000, 0x0821,
	0x9c31, 0x1042, 0x9c73, 0x1e02, 0x1a02, 0x0c00, 0x4002, 0xc001,
	0x0000, 0xde7b, 0xde03, 0x5e01, 0x5e02, 0xc07b, 0x0000, 0xde7b,
	0x0058, 0x4079, 0x407a, 0x407b, 0xd47b, 0x0000, 0x0000, 0x0000,
	0x0000, 0xde7b, 0x1e00, 0xc003, 0x1042, 0xde03, 0x0000, 0xd65a,
	0xce39, 0x8c31, 0x4a29, 0x0078, 0xc07b, 0x1e02, 0x1e78, 0xc003,
	0x0000, 0xce00, 0x5201, 0xd601, 0x5a02, 0xde02, 0x0000, 0x0821,
	0x8c31, 0x1042, 0x9c73, 0x1e03, 0x1a02, 0x0c00, 0x9e01, 0x0e00
};

static const UINT16 palette_data_10[0x50] =
{
	0x0000, 0xde7b, 0xde03, 0x5e01, 0x5e02, 0xc07b, 0x0000, 0xde7b,
	0x0058, 0x4079, 0x407a, 0x407b, 0xd47b, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0601, 0x8a09, 0x0e1a, 0x922a, 0x163b, 0xde7b, 0xd65a,
	0xce39, 0x0821, 0x0000, 0x0c00, 0x5208, 0x1a02, 0x9e03, 0xce39,
	0x0000, 0xde7b, 0xde03, 0x5e01, 0x5e02, 0xc07b, 0x0000, 0xde7b,
	0x0058, 0x4079, 0x407a, 0x407b, 0xd47b, 0x0000, 0x0000, 0x0000,
	0x0000, 0x1400, 0x8002, 0x0068, 0x0000, 0x5e01, 0x5e02, 0x1e03,
	0xde03, 0xce39, 0xce39, 0xce39, 0xce39, 0xce39, 0xce39, 0xce39,
	0x0078, 0x4208, 0x1052, 0x9462, 0x1873, 0x5a73, 0xde7b, 0x1863,
	0x524a, 0xce39, 0x0821, 0x1600, 0x1000, 0xd201, 0xde03, 0x0a42
};

static const UINT16 palette_data_11[0x50] =
{
	0x0000, 0x4a29, 0x8c31, 0xce39, 0x1042, 0x524a, 0x9452, 0xd65a,
	0x1863, 0x0000, 0xde39, 0xde7b, 0xc001, 0x8002, 0x1800, 0x1e00,
	0x0000, 0xde7b, 0x1e00, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x1e00, 0x1e00, 0x1e00, 0x1e00, 0x1e00, 0x1e00, 0x1e00, 0x1e00,
	0x1e00, 0x1e00, 0x1e00, 0x1e00, 0x1e00, 0x1e00, 0x1e00, 0x1e00,
	0xde03, 0xde03, 0xde03, 0xde03, 0xde03, 0xde03, 0xde03, 0xde03,
	0xde03, 0xde03, 0xde03, 0xde03, 0xde03, 0xde03, 0xde03, 0xde03,
	0xde03, 0x0e00, 0x9e4a, 0x0000, 0x1042, 0xde7b, 0x9452, 0x4a29,
	0xce39, 0x1c02, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
};

static const UINT16 *const palette_data_lookup[] =
{
	0,
	palette_data_01,
	palette_data_02,
	palette_data_03,
	palette_data_04,
	palette_data_05,
	palette_data_06,
	palette_data_07,
	palette_data_08,
	palette_data_09,
	palette_data_0a,
	palette_data_0b,
	palette_data_0c,
	palette_data_0d,
	palette_data_0e,
	palette_data_0f,
	palette_data_10,
	palette_data_11
};


static TIMER_CALLBACK( volfied_timer_callback )
{
	// Palette commands - palette data written to bank 0: $10 - $af
	if (current_cmd>=0x1 && current_cmd<0x12)
	{
		const UINT16* palette_data=palette_data_lookup[current_cmd];
		int i=0;
		for (i=0; i<0x50; i++)
		{
			cchip_ram[0x10 + i*2 + 0]=palette_data[i]>>8;
			cchip_ram[0x10 + i*2 + 1]=palette_data[i]&0xff;
		}
	}

	// Unknown command - result written to bank 0: $23
	if (current_cmd>=0x81 && current_cmd<0x92)
	{
		switch (current_cmd)
		{
		case 0x81: cchip_ram[0x23]=0xf; break;
		case 0x82: cchip_ram[0x23]=0x1; break;
		case 0x83: cchip_ram[0x23]=0x6; break;
		case 0x84: cchip_ram[0x23]=0xf; break;
		case 0x85: cchip_ram[0x23]=0x9; break;
		case 0x86: cchip_ram[0x23]=0x6; break;
		case 0x87: cchip_ram[0x23]=0x6; break;
		case 0x88: cchip_ram[0x23]=0xf; break;
		case 0x89: cchip_ram[0x23]=0x8; break;
		case 0x8a: cchip_ram[0x23]=0x1; break;
		case 0x8b: cchip_ram[0x23]=0xa; break;
		case 0x8c: cchip_ram[0x23]=0x1; break;
		case 0x8d: cchip_ram[0x23]=0x1; break;
		case 0x8e: cchip_ram[0x23]=0x8; break;
		case 0x8f: cchip_ram[0x23]=0x6; break;
		case 0x90: cchip_ram[0x23]=0xa; break;
		case 0x91: cchip_ram[0x23]=0x0; break;
		}
	}

	current_cmd=0;
}

/*************************************
 *
 * Writes to C-Chip - Important Bits
 *
 *************************************/

WRITE16_HANDLER( volfied_cchip_ctrl_w )
{
	/* value 2 is written here */
}

WRITE16_HANDLER( volfied_cchip_bank_w )
{
	current_bank = data & 7;
}

WRITE16_HANDLER( volfied_cchip_ram_w )
{
	cchip_ram[(current_bank * 0x400) + offset]=data;

//  if (offset!=0x8)
//      logerror("%08x:  volfied c write %04x %04x\n", cpu_get_pc(space->cpu), offset,data);

	if (current_bank == 0)
	{
		if (offset == 0x008)
		{
			cc_port = data;

			coin_lockout_w(1, data & 0x80);
			coin_lockout_w(0, data & 0x40);
			coin_counter_w(1, data & 0x20);
			coin_counter_w(0, data & 0x10);
		}

		if (offset == 0x3fe)
		{
			/*******************
            (This table stored in ROM at $146a8)
            (Level number stored at $100198.b, from $100118.b, from $100098.b)
            (Level number at $b34 stored to $100098.b)

            round 01 => data $0A
            round 02 => data $01
            round 03 => data $03
            round 04 => data $08
            round 05 => data $05
            round 06 => data $04
            round 07 => data $0B
            round 08 => data $09
            round 09 => data $07
            round 10 => data $06
            round 11 => data $0E
            round 12 => data $0D
            round 13 => data $02
            round 14 => data $0C
            round 15 => data $0F
            round 16 => data $10
            final    => data $11

            ********************/

			current_cmd = data;

			// Palette request cmd - verified to take around 122242 68000 cycles to complete
			if (current_cmd >= 0x1 && current_cmd < 0x12)
			{
				timer_set(ATTOTIME_IN_CYCLES(122242,0), NULL, 0, volfied_timer_callback);
			}
			// Unknown cmd - verified to take around 105500 68000 cycles to complete
			else if (current_cmd >= 0x81 && current_cmd < 0x92)
			{
				timer_set(ATTOTIME_IN_CYCLES(105500,0), NULL, 0, volfied_timer_callback);
			}
			else
			{
				logerror("unknown cchip cmd %02x\n", data);
				current_cmd=0;
			}
		}

		// Some kind of timer command
		if (offset == 0x3ff)
		{
			current_flag = data;
		}
	}
}


/*************************************
 *
 * Reads from C-Chip
 *
 *************************************/

READ16_HANDLER( volfied_cchip_ctrl_r )
{
	/*
        Bit 2 = Error signal
        Bit 0 = Ready signal
    */
	return 0x01; /* Return 0x05 for C-Chip error */
}

READ16_HANDLER( volfied_cchip_ram_r )
{
	/* Check for input ports */
	if (current_bank == 0)
	{
		switch (offset)
		{
		case 0x03: return input_port_read(space->machine, "F00007");    /* STARTn + SERVICE1 */
		case 0x04: return input_port_read(space->machine, "F00009");    /* COINn */
		case 0x05: return input_port_read(space->machine, "F0000B");    /* Player controls + TILT */
		case 0x06: return input_port_read(space->machine, "F0000D");    /* Player controls (cocktail) */
		case 0x08: return cc_port;
		}
	}

//  if (cpu_get_pc(space->cpu)!=0x15ca8 && cpu_get_pc(space->cpu)!=0x15cd8 && cpu_get_pc(space->cpu)!=0x15cde)
//      logerror("%08x:  volfied c read %04x (bank %04x)\n", cpu_get_pc(space->cpu), offset, current_bank);

	/* Unknown */
	if (current_bank == 2 && offset == 0x005)
	{
		/* Not fully understood - Game writes:
            0001a0c2:  volfied c write 0005 00aa
            0001a0ca:  volfied c write 0006 0055
            0001a0d2:  volfied c write 0004 0065

            Then expects 0x7c to replace the 0xaa some time later.
        */
		return 0x7c;                /* makes worm in round 1 appear */
	}

	/* Unknown - some kind of timer */
	if (current_bank == 0 && offset == 0x3ff)
	{
		return 2 * current_flag;    /* fixes freeze after shield runs out */
	}

	/* Current command status */
	if (current_bank == 0 && offset == 0x3fe)
	{
		return current_cmd;
	}

	return cchip_ram[(current_bank * 0x400) + offset];
}


/*************************************
 *
 * C-Chip State Saving
 *
 *************************************/

void volfied_cchip_init(void)
{
	cchip_ram=auto_malloc(0x400 * 8);

	state_save_register_global(current_bank);
	state_save_register_global(current_cmd);
	state_save_register_global(current_flag);
	state_save_register_global(cc_port);
	state_save_register_global_pointer(cchip_ram, 0x400 * 8);
}
