//============================================================
//
//	scale.c - scaling effects framework code
//
//============================================================


// MAME headers
#include "driver.h"
#include "rc.h"

// scale2x
#ifdef __GNUC__
 #include "scale/scale2x.h"
#else
 #define __restrict__
 #include "scale/scale2x_vc.h"
#endif
#include "scale/scale3x.h"
#include "scale/hlq.h"

// defines
#define SCALE_EFFECT_NONE			0
#define SCALE_EFFECT_SCALE2X		1
#define SCALE_EFFECT_SCALE2X3		2
#ifdef USE_4X_SCALE
#define SCALE_EFFECT_SCALE2X4		3
#endif /* USE_4X_SCALE */
#define SCALE_EFFECT_SCALE3X		4
#define SCALE_EFFECT_SUPERSCALE		5
#define SCALE_EFFECT_SUPERSCALE75	6
#define SCALE_EFFECT_2XSAI			7
#define SCALE_EFFECT_SUPER2XSAI		8
#define SCALE_EFFECT_SUPEREAGLE		9
#define SCALE_EFFECT_EAGLE			10
#define SCALE_EFFECT_HQ2X			11
#define SCALE_EFFECT_HQ2X3			12
#define SCALE_EFFECT_HQ2X4			13
#define SCALE_EFFECT_LQ2X			14
#define SCALE_EFFECT_LQ2X3			15
#define SCALE_EFFECT_LQ2X4			16
#define SCALE_EFFECT_HQ3X			17
#define SCALE_EFFECT_LQ3X			18
#ifdef USE_4X_SCALE
#define SCALE_EFFECT_HQ4X			19
#define SCALE_EFFECT_LQ4X			20
#endif /* USE_4X_SCALE */


//============================================================
//	GLOBAL VARIABLES
//============================================================

struct { int effect; int xsize; int ysize; const char *name; } scale_effect;



//============================================================
//	IMPORTS
//============================================================

extern UINT32 asmblit_cpuid_features(void);



//============================================================
//	LOCAL VARIABLES
//============================================================

static int use_mmx;

static UINT8 *scale_buffer;

static int previous_depth;
static int previous_pitch;
static int previous_height;



//============================================================
//	prototypes
//============================================================

// functions from scale2x
static int scale_perform_scale2x(UINT8 *src, UINT8 *dst, int src_pitch, int dst_pitch, int width, int height, int depth);
static void (*scale_scale2x_line_16)(UINT16 *dst0, UINT16 *dst1, const UINT16 *src0, const UINT16 *src1, const UINT16 *src2, unsigned count);
static void (*scale_scale2x_line_32)(UINT32 *dst0, UINT32 *dst1, const UINT32 *src0, const UINT32 *src1, const UINT32 *src2, unsigned count);

static int scale_perform_scale2x3(UINT8 *src, UINT8 *dst, int src_pitch, int dst_pitch, int width, int height, int depth);
static void (*scale_scale2x3_line_16)(UINT16 *dst0, UINT16 *dst1, UINT16 *dst2, const UINT16 *src0, const UINT16 *src1, const UINT16 *src2, unsigned count);
static void (*scale_scale2x3_line_32)(UINT32 *dst0, UINT32 *dst1, UINT32 *dst2, const UINT32 *src0, const UINT32 *src1, const UINT32 *src2, unsigned count);

#ifdef USE_4X_SCALE
static int scale_perform_scale2x4(UINT8 *src, UINT8 *dst, int src_pitch, int dst_pitch, int width, int height, int depth);
static void (*scale_scale2x4_line_16)(UINT16 *dst0, UINT16 *dst1, UINT16 *dst2, UINT16 *dst3, const UINT16 *src0, const UINT16 *src1, const UINT16 *src2, unsigned count);
static void (*scale_scale2x4_line_32)(UINT32 *dst0, UINT32 *dst1, UINT32 *dst2, UINT32 *dst3, const UINT32 *src0, const UINT32 *src1, const UINT32 *src2, unsigned count);
#endif /* USE_4X_SCALE */

static int scale_perform_scale3x(UINT8 *src, UINT8 *dst, int src_pitch, int dst_pitch, int width, int height, int depth);

// functions from superscale.asm
void superscale_line(UINT16 *src0, UINT16 *src1, UINT16 *src2, UINT16 *dst, UINT32 width, UINT64 *mask);
void superscale_line_75(UINT16 *src0, UINT16 *src1, UINT16 *src2, UINT16 *dst, UINT32 width, UINT64 *mask);

static void (*superscale_line_func)(UINT16 *src0, UINT16 *src1, UINT16 *src2, UINT16 *dst, UINT32 width, UINT64 *mask);

static int scale_perform_superscale(UINT8 *src, UINT8 *dst, int src_pitch, int dst_pitch, int width, int height, int depth);

// functions from eagle
void _eagle_mmx16(unsigned long *lb, unsigned long *lb2, short width,	unsigned long *screen_address1, unsigned long *screen_address2);

static int scale_perform_eagle(UINT8 *src, UINT8 *dst, int src_pitch, int dst_pitch, int width, int height, int depth);

// functions from 2xsaimmx.asm
void  _2xSaISuperEagleLine(UINT8 *srcPtr, UINT8 *deltaPtr, UINT32 srcPitch, UINT32 width, UINT8 *dstPtr, UINT32 dstPitch, UINT16 dstBlah);
void  _2xSaILine(UINT8* srcPtr, UINT8* deltaPtr, UINT32 srcPitch, UINT32 width, UINT8* dstPtr, UINT32 dstPitch, UINT16 dstBlah);
void  _2xSaISuper2xSaILine(UINT8 *srcPtr, UINT8 *deltaPtr, UINT32 srcPitch, UINT32 width, UINT8 *dstPtr, UINT32 dstPitch, UINT16 dstBlah);
void  Init_2xSaIMMX(UINT32 BitFormat);

static void (*scale_2xsai_line)(UINT8 *srcPtr, UINT8 *deltaPtr, UINT32 srcPitch, UINT32 width, UINT8 *dstPtr, UINT32 dstPitch, UINT16 dstBlah);
static int scale_perform_2xsai(UINT8 *src, UINT8 *dst, int src_pitch, int dst_pitch, int width, int height, int depth);
static void scale_2xsai_flush_buffer(int src_pitch, int height);

// functions from hq2x/hq3x/hq4x/lq2x/lq3x/lq4x
static int scale_perform_hlq2x(UINT8 *src, UINT8 *dst, int src_pitch, int dst_pitch, int width, int height, int depth);
static void (*scale_hlq2x_15_def)(UINT16* dst0, UINT16* dst1, const UINT16* src0, const UINT16* src1, const UINT16* src2, unsigned count);
static void (*scale_hlq2x_16_def)(UINT16* dst0, UINT16* dst1, const UINT16* src0, const UINT16* src1, const UINT16* src2, unsigned count);
static void (*scale_hlq2x_32_def)(UINT32* dst0, UINT32* dst1, const UINT32* src0, const UINT32* src1, const UINT32* src2, unsigned count);

static int scale_perform_hlq2x3(UINT8 *src, UINT8 *dst, int src_pitch, int dst_pitch, int width, int height, int depth);
static void (*scale_hlq2x3_16_def)(UINT16* dst0, UINT16* dst1, UINT16* dst2, const UINT16* src0, const UINT16* src1, const UINT16* src2, unsigned count);
static void (*scale_hlq2x3_32_def)(UINT32* dst0, UINT32* dst1, UINT32* dst2, const UINT32* src0, const UINT32* src1, const UINT32* src2, unsigned count);

static int scale_perform_hlq2x4(UINT8 *src, UINT8 *dst, int src_pitch, int dst_pitch, int width, int height, int depth);
static void (*scale_hlq2x4_16_def)(UINT16* dst0, UINT16* dst1, UINT16* dst2, UINT16* dst3, const UINT16* src0, const UINT16* src1, const UINT16* src2, unsigned count);
static void (*scale_hlq2x4_32_def)(UINT32* dst0, UINT32* dst1, UINT32* dst2, UINT32* dst3, const UINT32* src0, const UINT32* src1, const UINT32* src2, unsigned count);

static int scale_perform_hlq3x(UINT8 *src, UINT8 *dst, int src_pitch, int dst_pitch, int width, int height, int depth);
static void (*scale_hlq3x_15_def)(UINT16* dst0, UINT16* dst1, UINT16* dst2, const UINT16* src0, const UINT16* src1, const UINT16* src2, unsigned count);
static void (*scale_hlq3x_16_def)(UINT16* dst0, UINT16* dst1, UINT16* dst2, const UINT16* src0, const UINT16* src1, const UINT16* src2, unsigned count);
static void (*scale_hlq3x_32_def)(UINT32* dst0, UINT32* dst1, UINT32* dst2, const UINT32* src0, const UINT32* src1, const UINT32* src2, unsigned count);
#ifdef USE_4X_SCALE
static int scale_perform_hlq4x(UINT8 *src, UINT8 *dst, int src_pitch, int dst_pitch, int width, int height, int depth);
static void (*scale_hlq4x_15_def)(UINT16* dst0, UINT16* dst1, UINT16* dst2, UINT16* dst3, const UINT16* src0, const UINT16* src1, const UINT16* src2, unsigned count);
static void (*scale_hlq4x_16_def)(UINT16* dst0, UINT16* dst1, UINT16* dst2, UINT16* dst3, const UINT16* src0, const UINT16* src1, const UINT16* src2, unsigned count);
static void (*scale_hlq4x_32_def)(UINT32* dst0, UINT32* dst1, UINT32* dst2, UINT32* dst3, const UINT32* src0, const UINT32* src1, const UINT32* src2, unsigned count);
#endif /* USE_4X_SCALE */

//============================================================
//	scale_parse
//============================================================

int scale_decode(struct rc_option *option, const char *arg, int priority)
{
	if (!strcmp(arg, "none"))
	{
		scale_effect.effect = SCALE_EFFECT_NONE;
	}
	// scale2x/scale3x: use Andrea Mazzoleni's scale2x/scale3x
	else if (!strcmp(arg, "scale2x"))
	{
		scale_effect.effect = SCALE_EFFECT_SCALE2X;
	}
	else if (!strcmp(arg, "scale2x3"))
	{
		scale_effect.effect = SCALE_EFFECT_SCALE2X3;
	}
#ifdef USE_4X_SCALE
	else if (!strcmp(arg, "scale2x4"))
	{
		scale_effect.effect = SCALE_EFFECT_SCALE2X4;
	}
#endif /* USE_4X_SCALE */
	else if (!strcmp(arg, "scale3x"))
	{
		scale_effect.effect = SCALE_EFFECT_SCALE3X;
	}
	// eagle: use Dirk Stevens' Eagle FX
	else if (!strcmp(arg, "eagle"))
	{
		scale_effect.effect = SCALE_EFFECT_EAGLE;
	}
	// superscale/superscale75: use Elsemi's SuperScale
	else if (!strcmp(arg, "superscale"))
	{
		scale_effect.effect = SCALE_EFFECT_SUPERSCALE;
	}
	else if (!strcmp(arg, "superscale75"))
	{
		scale_effect.effect = SCALE_EFFECT_SUPERSCALE75;
	}
	// 2xsai/super2xsai/supereagle: use Derek Liauw Kie Fa's 2xSaI package
	else if (!strcmp(arg, "2xsai"))
	{
		scale_effect.effect = SCALE_EFFECT_2XSAI;
	}
	else if (!strcmp(arg, "super2xsai"))
	{
		scale_effect.effect = SCALE_EFFECT_SUPER2XSAI;
	}
	else if (!strcmp(arg, "supereagle"))
	{
		scale_effect.effect = SCALE_EFFECT_SUPEREAGLE;
	}
	// hq2x/hq3x/hq4x/lq2x/lq3x/lq4x: made by Maxim Stepin
	else if (!strcmp(arg, "hq2x"))
	{
		scale_effect.effect = SCALE_EFFECT_HQ2X;
	}
	else if (!strcmp(arg, "hq2x3"))
	{
		scale_effect.effect = SCALE_EFFECT_HQ2X3;
	}
	else if (!strcmp(arg, "hq2x4"))
	{
		scale_effect.effect = SCALE_EFFECT_HQ2X4;
	}
	else if (!strcmp(arg, "lq2x"))
	{
		scale_effect.effect = SCALE_EFFECT_LQ2X;
	}
	else if (!strcmp(arg, "lq2x3"))
	{
		scale_effect.effect = SCALE_EFFECT_LQ2X3;
	}
	else if (!strcmp(arg, "lq2x4"))
	{
		scale_effect.effect = SCALE_EFFECT_LQ2X4;
	}
	else if (!strcmp(arg, "hq3x"))
	{
		scale_effect.effect = SCALE_EFFECT_HQ3X;
	}
	else if (!strcmp(arg, "lq3x"))
	{
		scale_effect.effect = SCALE_EFFECT_LQ3X;
	}
#ifdef USE_4X_SCALE
	else if (!strcmp(arg, "hq4x"))
	{
		scale_effect.effect = SCALE_EFFECT_HQ4X;
	}
	else if (!strcmp(arg, "lq4x"))
	{
		scale_effect.effect = SCALE_EFFECT_LQ4X;
	}
#endif /* USE_4X_SCALE */
	else
		return -1;

	option->priority = priority;
	return 0;
}

//============================================================
//	scale_exit
//============================================================

int scale_exit(void)
{
	free(scale_buffer);
	scale_buffer = NULL;

	return 0;
}

//============================================================
//	scale_init
//============================================================

int scale_init(void)
{
	static char name[64];

	UINT32 features = asmblit_cpuid_features();
	use_mmx = (features & (1 << 23));

	scale_effect.xsize = scale_effect.ysize = 1;
	sprintf(name, "none");
	scale_effect.name = name;

	previous_depth = previous_pitch = previous_height = 0;
	scale_buffer = NULL;

	switch (scale_effect.effect)
	{
		case SCALE_EFFECT_NONE:
		{
			break;
		}
		case SCALE_EFFECT_SCALE2X:
		{
			sprintf(name, "Scale2x (%s)", use_mmx ? "mmx optimised" : "non-mmx version");
			scale_effect.xsize = scale_effect.ysize = 2;
			break;
		}
		case SCALE_EFFECT_SCALE2X3:
		{
			sprintf(name, "Scale2x3 (%s)", use_mmx ? "mmx optimised" : "non-mmx version");
			scale_effect.xsize = 2;
			scale_effect.ysize = 3;
			break;
		}
#ifdef USE_4X_SCALE
		case SCALE_EFFECT_SCALE2X4:
		{
			sprintf(name, "Scale2x4 (%s)", use_mmx ? "mmx optimised" : "non-mmx version");
			scale_effect.xsize = 2;
			scale_effect.ysize = 4;
			break;
		}
#endif /* USE_4X_SCALE */
		case SCALE_EFFECT_SCALE3X:
		{
			sprintf(name, "Scale3x (non-mmx version)");
			scale_effect.xsize = scale_effect.ysize = 3;
			break;
		}
		case SCALE_EFFECT_EAGLE:
		{
			sprintf(name, "Eagle (mmx optimised)");

			if (!use_mmx)
				return 1;

			scale_effect.xsize = scale_effect.ysize = 2;
			break;
		}

		case SCALE_EFFECT_SUPERSCALE:
		case SCALE_EFFECT_SUPERSCALE75:
		{
			if (scale_effect.effect == SCALE_EFFECT_SUPERSCALE)
			{
				sprintf(name, "SuperScale (mmx optimised)");
				superscale_line_func = superscale_line;
			}
			else
			{
				sprintf(name, "SuperScale75%% (mmx optimised)");
				superscale_line_func = superscale_line_75;
			}

			if (!use_mmx)
				return 1;

			scale_effect.xsize = scale_effect.ysize = 2;
			break;
		}

		case SCALE_EFFECT_2XSAI:
		case SCALE_EFFECT_SUPER2XSAI:
		case SCALE_EFFECT_SUPEREAGLE:
		{
			if (scale_effect.effect == SCALE_EFFECT_2XSAI)
			{
				sprintf(name, "2xSaI (mmx optimised)");
				scale_2xsai_line = _2xSaILine;
			}
			else if (scale_effect.effect == SCALE_EFFECT_SUPER2XSAI)
			{
				sprintf(name, "Super 2xSaI (mmx optimised)");
				scale_2xsai_line = _2xSaISuper2xSaILine;
			}
			else
			{
				sprintf(name, "Super Eagle (mmx optimised)");
				scale_2xsai_line = _2xSaISuperEagleLine;
			}

			if (!use_mmx)
				return 1;

			scale_effect.xsize = scale_effect.ysize = 2;
			break;
		}
		case SCALE_EFFECT_HQ2X:
		{
#ifdef USE_MMX_INTERP_SCALE
			if (use_mmx)
				sprintf(name, "HQ2x (mmx version)");
			else
#endif /* USE_MMX_INTERP_SCALE */
				sprintf(name, "HQ2x (non-mmx version)");

			scale_effect.xsize = scale_effect.ysize = 2;
			scale_hlq2x_15_def = hq2x_15_def;
			scale_hlq2x_16_def = hq2x_16_def;
			scale_hlq2x_32_def = hq2x_32_def;
			break;
		}
		case SCALE_EFFECT_HQ2X3:
		{
#ifdef USE_MMX_INTERP_SCALE
			if (use_mmx)
				sprintf(name, "HQ2x3 (mmx version)");
			else
#endif /* USE_MMX_INTERP_SCALE */
				sprintf(name, "HQ2x3 (non-mmx version)");

			scale_effect.xsize = 2;
			scale_effect.ysize = 3;
			scale_hlq2x3_16_def = hq2x3_16_def;
			scale_hlq2x3_32_def = hq2x3_32_def;
			break;
		}
		case SCALE_EFFECT_HQ2X4:
		{
#ifdef USE_MMX_INTERP_SCALE
			if (use_mmx)
				sprintf(name, "HQ2x4 (mmx version)");
			else
#endif /* USE_MMX_INTERP_SCALE */
				sprintf(name, "HQ2x4 (non-mmx version)");

			scale_effect.xsize = 2;
			scale_effect.ysize = 4;
			scale_hlq2x4_16_def = hq2x4_16_def;
			scale_hlq2x4_32_def = hq2x4_32_def;
			break;
		}
		case SCALE_EFFECT_LQ2X:
		{
#ifdef USE_MMX_INTERP_SCALE
			if (use_mmx)
				sprintf(name, "LQ2x (mmx version)");
			else
#endif /* USE_MMX_INTERP_SCALE */
				sprintf(name, "LQ2x (non-mmx version)");

			scale_effect.xsize = scale_effect.ysize = 2;
			scale_hlq2x_15_def = lq2x_15_def;
			scale_hlq2x_16_def = lq2x_16_def;
			scale_hlq2x_32_def = lq2x_32_def;
			break;
		}
		case SCALE_EFFECT_LQ2X3:
		{
#ifdef USE_MMX_INTERP_SCALE
			if (use_mmx)
				sprintf(name, "LQ2x3 (mmx version)");
			else
#endif /* USE_MMX_INTERP_SCALE */
				sprintf(name, "LQ2x3 (non-mmx version)");

			scale_effect.xsize = 2;
			scale_effect.ysize = 3;
			scale_hlq2x3_16_def = lq2x3_16_def;
			scale_hlq2x3_32_def = lq2x3_32_def;
			break;
		}
		case SCALE_EFFECT_LQ2X4:
		{
#ifdef USE_MMX_INTERP_SCALE
			if (use_mmx)
				sprintf(name, "LQ2x4 (mmx version)");
			else
#endif /* USE_MMX_INTERP_SCALE */
				sprintf(name, "LQ2x4 (non-mmx version)");

			scale_effect.xsize = 2;
			scale_effect.ysize = 4;
			scale_hlq2x4_16_def = lq2x4_16_def;
			scale_hlq2x4_32_def = lq2x4_32_def;
			break;
		}
		case SCALE_EFFECT_HQ3X:
		{
#ifdef USE_MMX_INTERP_SCALE
			if (use_mmx)
				sprintf(name, "HQ3x (mmx version)");
			else
#endif /* USE_MMX_INTERP_SCALE */
				sprintf(name, "HQ3x (non-mmx version)");

			scale_effect.xsize = scale_effect.ysize = 3;
			scale_hlq3x_15_def = hq3x_15_def;
			scale_hlq3x_16_def = hq3x_16_def;
			scale_hlq3x_32_def = hq3x_32_def;
			break;
		}
		case SCALE_EFFECT_LQ3X:
		{
#ifdef USE_MMX_INTERP_SCALE
			if (use_mmx)
				sprintf(name, "LQ3x (mmx version)");
			else
#endif /* USE_MMX_INTERP_SCALE */
				sprintf(name, "LQ3x (non-mmx version)");

			scale_effect.xsize = scale_effect.ysize = 3;
			scale_hlq3x_15_def = lq3x_15_def;
			scale_hlq3x_16_def = lq3x_16_def;
			scale_hlq3x_32_def = lq3x_32_def;
			break;
		}
#ifdef USE_4X_SCALE
		case SCALE_EFFECT_HQ4X:
		{
#ifdef USE_MMX_INTERP_SCALE
			if (use_mmx)
				sprintf(name, "HQ4x (mmx version)");
			else
#endif /* USE_MMX_INTERP_SCALE */
				sprintf(name, "HQ4x (non-mmx version)");

			scale_effect.xsize = scale_effect.ysize = 4;
			scale_hlq4x_15_def = hq4x_15_def;
			scale_hlq4x_16_def = hq4x_16_def;
			scale_hlq4x_32_def = hq4x_32_def;
			break;
		}
		case SCALE_EFFECT_LQ4X:
		{
#ifdef USE_MMX_INTERP_SCALE
			if (use_mmx)
				sprintf(name, "LQ4x (mmx version)");
			else
#endif /* USE_MMX_INTERP_SCALE */
				sprintf(name, "LQ4x (non-mmx version)");

			scale_effect.xsize = scale_effect.ysize = 4;
			scale_hlq4x_15_def = lq4x_15_def;
			scale_hlq4x_16_def = lq4x_16_def;
			scale_hlq4x_32_def = lq4x_32_def;
			break;
		}
#endif /* USE_4X_SCALE */
		default:
		{
			return 1;
		}
	}

	return 0;
}



//============================================================
//	scale_check
//============================================================

int scale_check(int depth)
{
	switch (scale_effect.effect)
	{
		case SCALE_EFFECT_NONE:
			return 0;

		case SCALE_EFFECT_SCALE2X:
		case SCALE_EFFECT_SCALE2X3:
#ifdef USE_4X_SCALE
		case SCALE_EFFECT_SCALE2X4:
#endif /* USE_4X_SCALE */
		case SCALE_EFFECT_SCALE3X:
			if (depth == 16 || depth == 32)
				return 0;
			else
				return 1;

		case SCALE_EFFECT_SUPERSCALE:
		case SCALE_EFFECT_SUPERSCALE75:
			if (use_mmx && depth == 16)
				return 0;
			else
				return 1;

		case SCALE_EFFECT_EAGLE:
			if (use_mmx && depth == 16)
				return 0;
			else
				return 1;

		case SCALE_EFFECT_2XSAI:
		case SCALE_EFFECT_SUPER2XSAI:
		case SCALE_EFFECT_SUPEREAGLE:
			if (use_mmx && (depth == 15 || depth == 16))
				return 0;
			else
				return 1;

		case SCALE_EFFECT_HQ2X:
		case SCALE_EFFECT_HQ2X3:
		case SCALE_EFFECT_HQ2X4:
		case SCALE_EFFECT_LQ2X:
		case SCALE_EFFECT_LQ2X3:
		case SCALE_EFFECT_LQ2X4:
		case SCALE_EFFECT_HQ3X:
		case SCALE_EFFECT_LQ3X:
#ifdef USE_4X_SCALE
		case SCALE_EFFECT_HQ4X:
		case SCALE_EFFECT_LQ4X:
#endif /* USE_4X_SCALE */
#ifdef USE_MMX_INTERP_SCALE
			if (!use_mmx)
				return 1;
#endif /* USE_MMX_INTERP_SCALE */

			if (depth == 15 || depth == 16 || depth == 32)
				return 0;
			else
				return 1;

		default:
			return 1;
	}
}



//============================================================
//	scale_perform_scale
//============================================================

int scale_perform_scale(UINT8 *src, UINT8 *dst, int src_pitch, int dst_pitch, int width, int height, int depth, int update)
{
	switch (scale_effect.effect)
	{
		case SCALE_EFFECT_NONE:
			return 0;
		case SCALE_EFFECT_SCALE2X:
			return scale_perform_scale2x(src, dst, src_pitch, dst_pitch, width, height, depth);
		case SCALE_EFFECT_SCALE2X3:
			return scale_perform_scale2x3(src, dst, src_pitch, dst_pitch, width, height, depth);
#ifdef USE_4X_SCALE
		case SCALE_EFFECT_SCALE2X4:
			return scale_perform_scale2x4(src, dst, src_pitch, dst_pitch, width, height, depth);
#endif /* USE_4X_SCALE */
		case SCALE_EFFECT_SCALE3X:
			return scale_perform_scale3x(src, dst, src_pitch, dst_pitch, width, height, depth);
		case SCALE_EFFECT_SUPERSCALE:
		case SCALE_EFFECT_SUPERSCALE75:
			return scale_perform_superscale(src, dst, src_pitch, dst_pitch, width, height, depth);
		case SCALE_EFFECT_EAGLE:
			return scale_perform_eagle(src, dst, src_pitch, dst_pitch, width, height, depth);
		case SCALE_EFFECT_2XSAI:
		case SCALE_EFFECT_SUPER2XSAI:
		case SCALE_EFFECT_SUPEREAGLE:
			if (update)
				scale_2xsai_flush_buffer(src_pitch, height);
			return scale_perform_2xsai(src, dst, src_pitch, dst_pitch, width, height, depth);
		case SCALE_EFFECT_HQ2X:
		case SCALE_EFFECT_LQ2X:
			return scale_perform_hlq2x(src, dst, src_pitch, dst_pitch, width, height, depth);
		case SCALE_EFFECT_HQ2X3:
		case SCALE_EFFECT_LQ2X3:
			return scale_perform_hlq2x3(src, dst, src_pitch, dst_pitch, width, height, depth);
		case SCALE_EFFECT_HQ2X4:
		case SCALE_EFFECT_LQ2X4:
			return scale_perform_hlq2x4(src, dst, src_pitch, dst_pitch, width, height, depth);
		case SCALE_EFFECT_HQ3X:
		case SCALE_EFFECT_LQ3X:
			return scale_perform_hlq3x(src, dst, src_pitch, dst_pitch, width, height, depth);
#ifdef USE_4X_SCALE
		case SCALE_EFFECT_HQ4X:
		case SCALE_EFFECT_LQ4X:
			return scale_perform_hlq4x(src, dst, src_pitch, dst_pitch, width, height, depth);
#endif /* USE_4X_SCALE */
		default:
			return 1;
	}
}


//============================================================
//	scale_emms
//============================================================

static inline void scale_emms(void)
{
	if (use_mmx)
	{
#ifdef __GNUC__
		__asm__ __volatile__ (
			"emms\n"
		);
#else
		__asm {
			emms;
		}
#endif
	}
}

//============================================================
//	scale_perform_scale2x
//============================================================

static int scale_perform_scale2x(UINT8 *src, UINT8 *dst, int src_pitch, int dst_pitch, int width, int height, int depth)
{
	int y;
	UINT8 *src_prev = src;
	UINT8 *src_curr = src;
	UINT8 *src_next = src + src_pitch;

	if (depth != 16 && depth != 32)
		return 1;

	if (previous_depth != depth)
	{
		if (use_mmx)
		{
			scale_scale2x_line_16 = scale2x_16_mmx;
			scale_scale2x_line_32 = scale2x_32_mmx;
		}
		else
		{
			scale_scale2x_line_16 = scale2x_16_def;
			scale_scale2x_line_32 = scale2x_32_def;
		}

		previous_depth = depth;
	}

	if (depth == 16)
//		scale_scale2x_line_16((UINT16 *)dst, (UINT16 *)(dst + dst_pitch), (UINT16 *)src_prev, (UINT16 *)src_curr, (UINT16 *)src_next, width);
		scale2x_16_def((UINT16 *)dst, (UINT16 *)(dst + dst_pitch), (UINT16 *)src_prev, (UINT16 *)src_curr, (UINT16 *)src_next, width);
	else
//		scale_scale2x_line_32((UINT32 *)dst, (UINT32 *)(dst + dst_pitch), (UINT32 *)src_prev, (UINT32 *)src_curr, (UINT32 *)src_next, width);
		scale2x_32_def((UINT32 *)dst, (UINT32 *)(dst + dst_pitch), (UINT32 *)src_prev, (UINT32 *)src_curr, (UINT32 *)src_next, width);

	if (depth == 16)
	{
		for (y = 2; y < height; y++)
		{
			dst += 2 * dst_pitch;
			src_prev = src_curr;
			src_curr = src_next;
			src_next += src_pitch;
			scale_scale2x_line_16((UINT16 *)dst, (UINT16 *)(dst + dst_pitch), (UINT16 *)src_prev, (UINT16 *)src_curr, (UINT16 *)src_next, width);
		}
	}
	else
	{
		for (y = 2; y < height; y++)
		{
			dst += 2 * dst_pitch;
			src_prev = src_curr;
			src_curr = src_next;
			src_next += src_pitch;
			scale_scale2x_line_32((UINT32 *)dst, (UINT32 *)(dst + dst_pitch), (UINT32 *)src_prev, (UINT32 *)src_curr, (UINT32 *)src_next, width);
		}
	}

	dst += 2 * dst_pitch;
	src_prev = src_curr;
	src_curr = src_next;
	if (depth == 16)
		scale_scale2x_line_16((UINT16 *)dst, (UINT16 *)(dst + dst_pitch), (UINT16 *)src_prev, (UINT16 *)src_curr, (UINT16 *)src_next, width);
	else
		scale_scale2x_line_32((UINT32 *)dst, (UINT32 *)(dst + dst_pitch), (UINT32 *)src_prev, (UINT32 *)src_curr, (UINT32 *)src_next, width);

	scale_emms();

	return 0;
}



//============================================================
//	scale_perform_scale2x3
//============================================================

static int scale_perform_scale2x3(UINT8 *src, UINT8 *dst, int src_pitch, int dst_pitch, int width, int height, int depth)
{
	int y;
	UINT8 *src_prev = src;
	UINT8 *src_curr = src;
	UINT8 *src_next = src + src_pitch;

	if (depth != 16 && depth != 32)
		return 1;

	if (previous_depth != depth)
	{
		if (use_mmx)
		{
			scale_scale2x3_line_16 = scale2x3_16_mmx;
			scale_scale2x3_line_32 = scale2x3_32_mmx;
		}
		else
		{
			scale_scale2x3_line_16 = scale2x3_16_def;
			scale_scale2x3_line_32 = scale2x3_32_def;
		}

		previous_depth = depth;
	}

	if (depth == 16)
//		scale_scale2x3_line_16((UINT16 *)dst, (UINT16 *)(dst + dst_pitch), (UINT16 *)(dst + 2 * dst_pitch), (UINT16 *)src_prev, (UINT16 *)src_curr, (UINT16 *)src_next, width);
		scale2x3_16_def((UINT16 *)dst, (UINT16 *)(dst + dst_pitch), (UINT16 *)(dst + 2 * dst_pitch), (UINT16 *)src_prev, (UINT16 *)src_curr, (UINT16 *)src_next, width);
	else
//		scale_scale2x3_line_32((UINT32 *)dst, (UINT32 *)(dst + dst_pitch), (UINT32 *)(dst + 2 * dst_pitch), (UINT32 *)src_prev, (UINT32 *)src_curr, (UINT32 *)src_next, width);
		scale2x3_32_def((UINT32 *)dst, (UINT32 *)(dst + dst_pitch), (UINT32 *)(dst + 2 * dst_pitch), (UINT32 *)src_prev, (UINT32 *)src_curr, (UINT32 *)src_next, width);

	if (depth == 16)
	{
		for (y = 2; y < height; y++)
		{
			dst += 3 * dst_pitch;
			src_prev = src_curr;
			src_curr = src_next;
			src_next += src_pitch;
			scale_scale2x3_line_16((UINT16 *)dst, (UINT16 *)(dst + dst_pitch), (UINT16 *)(dst + 2 * dst_pitch), (UINT16 *)src_prev, (UINT16 *)src_curr, (UINT16 *)src_next, width);
		}
	}
	else
	{
		for (y = 2; y < height; y++)
		{
			dst += 3 * dst_pitch;
			src_prev = src_curr;
			src_curr = src_next;
			src_next += src_pitch;
			scale_scale2x3_line_32((UINT32 *)dst, (UINT32 *)(dst + dst_pitch), (UINT32 *)(dst + 2 * dst_pitch), (UINT32 *)src_prev, (UINT32 *)src_curr, (UINT32 *)src_next, width);
		}
	}

	dst += 3 * dst_pitch;
	src_prev = src_curr;
	src_curr = src_next;
	if (depth == 16)
		scale_scale2x3_line_16((UINT16 *)dst, (UINT16 *)(dst + dst_pitch), (UINT16 *)(dst + 2 * dst_pitch), (UINT16 *)src_prev, (UINT16 *)src_curr, (UINT16 *)src_next, width);
	else
		scale_scale2x3_line_32((UINT32 *)dst, (UINT32 *)(dst + dst_pitch), (UINT32 *)(dst + 2 * dst_pitch), (UINT32 *)src_prev, (UINT32 *)src_curr, (UINT32 *)src_next, width);

	scale_emms();

	return 0;
}


#ifdef USE_4X_SCALE
//============================================================
//	scale_perform_scale2x4
//============================================================

static int scale_perform_scale2x4(UINT8 *src, UINT8 *dst, int src_pitch, int dst_pitch, int width, int height, int depth)
{
	int y;
	UINT8 *src_prev = src;
	UINT8 *src_curr = src;
	UINT8 *src_next = src + src_pitch;

	if (depth != 16 && depth != 32)
		return 1;

	if (previous_depth != depth)
	{
		if (use_mmx)
		{
			scale_scale2x4_line_16 = scale2x4_16_mmx;
			scale_scale2x4_line_32 = scale2x4_32_mmx;
		}
		else
		{
			scale_scale2x4_line_16 = scale2x4_16_def;
			scale_scale2x4_line_32 = scale2x4_32_def;
		}

		previous_depth = depth;
	}

	if (depth == 16)
//		scale_scale2x4_line_16((UINT16 *)dst, (UINT16 *)(dst + dst_pitch), (UINT16 *)(dst + 2 * dst_pitch), (UINT16 *)(dst + 3 * dst_pitch), (UINT16 *)src_prev, (UINT16 *)src_curr, (UINT16 *)src_next, width);
		scale2x4_16_def((UINT16 *)dst, (UINT16 *)(dst + dst_pitch), (UINT16 *)(dst + 2 * dst_pitch), (UINT16 *)(dst + 3 * dst_pitch), (UINT16 *)src_prev, (UINT16 *)src_curr, (UINT16 *)src_next, width);
	else
//		scale_scale2x4_line_32((UINT32 *)dst, (UINT32 *)(dst + dst_pitch), (UINT32 *)(dst + 2 * dst_pitch), (UINT32 *)(dst + 3 * dst_pitch), (UINT32 *)src_prev, (UINT32 *)src_curr, (UINT32 *)src_next, width);
		scale2x4_32_def((UINT32 *)dst, (UINT32 *)(dst + dst_pitch), (UINT32 *)(dst + 2 * dst_pitch), (UINT32 *)(dst + 3 * dst_pitch), (UINT32 *)src_prev, (UINT32 *)src_curr, (UINT32 *)src_next, width);

	if (depth == 16)
	{
		for (y = 2; y < height; y++)
		{
			dst += 4 * dst_pitch;
			src_prev = src_curr;
			src_curr = src_next;
			src_next += src_pitch;
			scale_scale2x4_line_16((UINT16 *)dst, (UINT16 *)(dst + dst_pitch), (UINT16 *)(dst + 2 * dst_pitch), (UINT16 *)(dst + 3 * dst_pitch), (UINT16 *)src_prev, (UINT16 *)src_curr, (UINT16 *)src_next, width);
		}
	}
	else
	{
		for (y = 2; y < height; y++)
		{
			dst += 4 * dst_pitch;
			src_prev = src_curr;
			src_curr = src_next;
			src_next += src_pitch;
			scale_scale2x4_line_32((UINT32 *)dst, (UINT32 *)(dst + dst_pitch), (UINT32 *)(dst + 2 * dst_pitch), (UINT32 *)(dst + 3 * dst_pitch), (UINT32 *)src_prev, (UINT32 *)src_curr, (UINT32 *)src_next, width);
		}
	}

	dst += 4 * dst_pitch;
	src_prev = src_curr;
	src_curr = src_next;
	if (depth == 16)
		scale_scale2x4_line_16((UINT16 *)dst, (UINT16 *)(dst + dst_pitch), (UINT16 *)(dst + 2 * dst_pitch), (UINT16 *)(dst + 3 * dst_pitch), (UINT16 *)src_prev, (UINT16 *)src_curr, (UINT16 *)src_next, width);
	else
		scale_scale2x4_line_32((UINT32 *)dst, (UINT32 *)(dst + dst_pitch), (UINT32 *)(dst + 2 * dst_pitch), (UINT32 *)(dst + 3 * dst_pitch), (UINT32 *)src_prev, (UINT32 *)src_curr, (UINT32 *)src_next, width);

	scale_emms();

	return 0;
}
#endif /* USE_4X_SCALE */


//============================================================
//	scale_perform_scale3x
//============================================================

static int scale_perform_scale3x(UINT8 *src, UINT8 *dst, int src_pitch, int dst_pitch, int width, int height, int depth)
{
	int y;
	UINT8 *src_prev = src;
	UINT8 *src_curr = src;
	UINT8 *src_next = src + src_pitch;

	if (depth == 16)
		scale3x_16_def((UINT16 *)dst, (UINT16 *)(dst + dst_pitch), (UINT16 *)(dst + 2 * dst_pitch), (UINT16 *)src_prev, (UINT16 *)src_curr, (UINT16 *)src_next, width);
	else
		scale3x_32_def((UINT32 *)dst, (UINT32 *)(dst + dst_pitch), (UINT32 *)(dst + 2 * dst_pitch), (UINT32 *)src_prev, (UINT32 *)src_curr, (UINT32 *)src_next, width);

	if (depth == 16)
	{
		for (y = 3; y < height; y++)
		{
			dst	+= 3 * dst_pitch;
			src_prev = src_curr;
			src_curr = src_next;
			src_next += src_pitch;
			scale3x_16_def((UINT16 *)dst, (UINT16 *)(dst + dst_pitch), (UINT16 *)(dst + 2 * dst_pitch), (UINT16 *)src_prev, (UINT16 *)src_curr, (UINT16 *)src_next, width);
		}
	}
	else
	{
		for (y = 3; y < height; y++)
		{
			dst += 3 * dst_pitch;
			src_prev = src_curr;
			src_curr = src_next;
			src_next += src_pitch;
			scale3x_32_def((UINT32 *)dst, (UINT32 *)(dst + dst_pitch), (UINT32 *)(dst + 2 * dst_pitch), (UINT32 *)src_prev, (UINT32 *)src_curr, (UINT32 *)src_next, width);
		}
	}

	dst += 3 * dst_pitch;
	src_prev = src_curr;
	src_curr = src_next;
	if (depth == 16)
		scale3x_16_def((UINT16 *)dst, (UINT16 *)(dst + dst_pitch), (UINT16 *)(dst + 2 * dst_pitch), (UINT16 *)src_prev, (UINT16 *)src_curr, (UINT16 *)src_next, width);
	else
		scale3x_32_def((UINT32 *)dst, (UINT32 *)(dst + dst_pitch), (UINT32 *)(dst + 2 * dst_pitch), (UINT32 *)src_prev, (UINT32 *)src_curr, (UINT32 *)src_next, width);

	return 0;
}



//============================================================
//	scale_perform_superscale
//============================================================

static int scale_perform_superscale(UINT8 *src, UINT8 *dst, int src_pitch, int dst_pitch, int width, int height, int depth)
{
	UINT32 srcNextline = src_pitch >> 1;
	UINT16 *dst0=(UINT16 *)dst;
	UINT16 *dst1=(UINT16 *)(dst+dst_pitch);
	UINT16 *src0=(UINT16 *)(src-src_pitch);  //don't worry, there is extra space :)
	UINT16 *src1=(UINT16 *)src;
	UINT16 *src2=(UINT16 *)(src+src_pitch);
	UINT64 mask = 0x7BEF7BEF7BEF7BEFLL;
	int i;

	for (i = 0; i < height; i++)
	{
		superscale_line(src0, src1, src2, dst0, width, &mask);
		superscale_line_func(src2, src1, src0, dst1, width, &mask);

		src0 = src1;
		src1 = src2;
		src2 += srcNextline;

		dst0 += dst_pitch;
		dst1 += dst_pitch;
	}
	scale_emms();

	return 0;
}



//============================================================
//	scale_perform_eagle
//============================================================

static int scale_perform_eagle(UINT8 *src, UINT8 *dst, int src_pitch, int dst_pitch, int width, int height, int depth)
{
	int y;

	if (depth != 16 || !use_mmx)
		return 1;

	width *= 2;

	_eagle_mmx16((unsigned long *)src, (unsigned long *)src, width, (unsigned long *)dst, (unsigned long *)dst);
	dst += dst_pitch - 2;
	for (y = 0; y < height; y++, src += src_pitch, dst += 2 * dst_pitch)
		_eagle_mmx16((unsigned long *)src, (unsigned long *)(src + src_pitch), width, (unsigned long *)dst, (unsigned long *)(dst + dst_pitch));

	scale_emms();

	return 0;
}



//============================================================
//	scale_2xsai_flush_buffer
//============================================================

static void scale_2xsai_flush_buffer(int src_pitch, int height)
{
	free(scale_buffer);
	scale_buffer = malloc(src_pitch * (height + 3));
	if (scale_buffer)
		memset(scale_buffer, 0xAA, src_pitch * (height + 3));
}

//============================================================
//	scale_perform_2xsai
//============================================================

static int scale_perform_2xsai(UINT8 *src, UINT8 *dst, int src_pitch, int dst_pitch, int width, int height, int depth)
{
	int y;
	UINT8 *buf;

	if ((depth != 15 && depth != 16) || !use_mmx)
		return 1;

	// see if we need to (re-)initialise
	if (previous_depth != depth || previous_pitch != src_pitch || previous_height != height)
	{
		Init_2xSaIMMX(depth == 15 ? 555 : 565);

		scale_2xsai_flush_buffer(src_pitch, height);

		previous_depth = depth;
		previous_pitch = src_pitch;
		previous_height = height;
	}

	if (scale_buffer == NULL)
		return 1;

	buf = scale_buffer;

	// treat 2xSaI differently from the others to better align the blitters with scanlines
	if (scale_effect.effect == SCALE_EFFECT_2XSAI) {
		for (y = 1; y < height; y++, src += src_pitch, buf += src_pitch, dst += 2 * dst_pitch)
			scale_2xsai_line(src, buf, src_pitch, width - 1, dst, dst_pitch, 0);
		scale_2xsai_line(src, buf, 0, width - 1, dst, dst_pitch, 0);
	}
	else
	{
		scale_2xsai_line(src, buf, src_pitch, width - 1, dst, dst_pitch, 0);
		dst += dst_pitch;
		for (y = 0; y < height; y++, src += src_pitch, buf += src_pitch, dst += 2 * dst_pitch)
			scale_2xsai_line(src, buf, src_pitch, width - 1, dst, dst_pitch, 0);
	}

	return 0;
}



//============================================================
//	scale_perform_hlq2x
//============================================================

static int scale_perform_hlq2x(UINT8 *src, UINT8 *dst, int src_pitch, int dst_pitch, int width, int height, int depth)
{
	int y;
	UINT8 *src_prev = src;
	UINT8 *src_curr = src;
	UINT8 *src_next = src + src_pitch;

	interp_init();

	if (depth == 15)
		scale_hlq2x_15_def((UINT16 *)dst, (UINT16 *)(dst + dst_pitch), (UINT16 *)src_prev, (UINT16 *)src_curr, (UINT16 *)src_next, width);
	else if (depth == 16)
		scale_hlq2x_16_def((UINT16 *)dst, (UINT16 *)(dst + dst_pitch), (UINT16 *)src_prev, (UINT16 *)src_curr, (UINT16 *)src_next, width);
	else
		scale_hlq2x_32_def((UINT32 *)dst, (UINT32 *)(dst + dst_pitch), (UINT32 *)src_prev, (UINT32 *)src_curr, (UINT32 *)src_next, width);

	if (depth == 15 || depth == 16)
	{
		for (y = 2; y < height; y++)
		{
			dst	+= 2 * dst_pitch;
			src_prev = src_curr;
			src_curr = src_next;
			src_next += src_pitch;

			if (depth == 15)
				scale_hlq2x_15_def((UINT16 *)dst, (UINT16 *)(dst + dst_pitch), (UINT16 *)src_prev, (UINT16 *)src_curr, (UINT16 *)src_next, width);
			else
				scale_hlq2x_16_def((UINT16 *)dst, (UINT16 *)(dst + dst_pitch), (UINT16 *)src_prev, (UINT16 *)src_curr, (UINT16 *)src_next, width);
		}
	}
	else
	{
		for (y = 2; y < height; y++)
		{
			dst += 2 * dst_pitch;
			src_prev = src_curr;
			src_curr = src_next;
			src_next += src_pitch;

			scale_hlq2x_32_def((UINT32 *)dst, (UINT32 *)(dst + dst_pitch), (UINT32 *)src_prev, (UINT32 *)src_curr, (UINT32 *)src_next, width);
		}
	}

	dst += 2 * dst_pitch;
	src_prev = src_curr;
	src_curr = src_next;

	if (depth == 15)
		scale_hlq2x_15_def((UINT16 *)dst, (UINT16 *)(dst + dst_pitch), (UINT16 *)src_prev, (UINT16 *)src_curr, (UINT16 *)src_next, width);
	else if (depth == 16)
		scale_hlq2x_16_def((UINT16 *)dst, (UINT16 *)(dst + dst_pitch), (UINT16 *)src_prev, (UINT16 *)src_curr, (UINT16 *)src_next, width);
	else
		scale_hlq2x_32_def((UINT32 *)dst, (UINT32 *)(dst + dst_pitch), (UINT32 *)src_prev, (UINT32 *)src_curr, (UINT32 *)src_next, width);

#ifdef USE_MMX_INTERP_SCALE
	scale_emms();
#endif /* USE_MMX_INTERP_SCALE */

	return 0;
}

//============================================================
//	scale_perform_hlq2x3
//============================================================

static int scale_perform_hlq2x3(UINT8 *src, UINT8 *dst, int src_pitch, int dst_pitch, int width, int height, int depth)
{
	int y;
	UINT8 *src_prev = src;
	UINT8 *src_curr = src;
	UINT8 *src_next = src + src_pitch;

	interp_init();

	if (depth == 15 || depth == 16)
		scale_hlq2x3_16_def((UINT16 *)dst, (UINT16 *)(dst + dst_pitch), (UINT16 *)(dst + 2 * dst_pitch), (UINT16 *)src_prev, (UINT16 *)src_curr, (UINT16 *)src_next, width);
	else
		scale_hlq2x3_32_def((UINT32 *)dst, (UINT32 *)(dst + dst_pitch), (UINT32 *)(dst + 2 * dst_pitch), (UINT32 *)src_prev, (UINT32 *)src_curr, (UINT32 *)src_next, width);

	if (depth == 15 || depth == 16)
	{
		for (y = 2; y < height; y++)
		{
			dst	+= 3 * dst_pitch;
			src_prev = src_curr;
			src_curr = src_next;
			src_next += src_pitch;
			scale_hlq2x3_16_def((UINT16 *)dst, (UINT16 *)(dst + dst_pitch), (UINT16 *)(dst + 2 * dst_pitch), (UINT16 *)src_prev, (UINT16 *)src_curr, (UINT16 *)src_next, width);
		}
	}
	else
	{
		for (y = 2; y < height; y++)
		{
			dst += 3 * dst_pitch;
			src_prev = src_curr;
			src_curr = src_next;
			src_next += src_pitch;
			scale_hlq2x3_32_def((UINT32 *)dst, (UINT32 *)(dst + dst_pitch), (UINT32 *)(dst + 2 * dst_pitch), (UINT32 *)src_prev, (UINT32 *)src_curr, (UINT32 *)src_next, width);
		}
	}

	dst += 3 * dst_pitch;
	src_prev = src_curr;
	src_curr = src_next;
	if (depth == 15 || depth == 16)
		scale_hlq2x3_16_def((UINT16 *)dst, (UINT16 *)(dst + dst_pitch), (UINT16 *)(dst + 2 * dst_pitch), (UINT16 *)src_prev, (UINT16 *)src_curr, (UINT16 *)src_next, width);
	else
		scale_hlq2x3_32_def((UINT32 *)dst, (UINT32 *)(dst + dst_pitch), (UINT32 *)(dst + 2 * dst_pitch), (UINT32 *)src_prev, (UINT32 *)src_curr, (UINT32 *)src_next, width);

#ifdef USE_MMX_INTERP_SCALE
	scale_emms();
#endif /* USE_MMX_INTERP_SCALE */

	return 0;
}

//============================================================
//	scale_perform_hlq2x4
//============================================================

static int scale_perform_hlq2x4(UINT8 *src, UINT8 *dst, int src_pitch, int dst_pitch, int width, int height, int depth)
{
	int y;
	UINT8 *src_prev = src;
	UINT8 *src_curr = src;
	UINT8 *src_next = src + src_pitch;

	interp_init();

	if (depth == 15 || depth == 16)
		scale_hlq2x4_16_def((UINT16 *)dst, (UINT16 *)(dst + dst_pitch), (UINT16 *)(dst + 2 * dst_pitch), (UINT16 *)(dst + 3 * dst_pitch), (UINT16 *)src_prev, (UINT16 *)src_curr, (UINT16 *)src_next, width);
	else
		scale_hlq2x4_32_def((UINT32 *)dst, (UINT32 *)(dst + dst_pitch), (UINT32 *)(dst + 2 * dst_pitch), (UINT32 *)(dst + 3 * dst_pitch), (UINT32 *)src_prev, (UINT32 *)src_curr, (UINT32 *)src_next, width);

	if (depth == 15 || depth == 16)
	{
		for (y = 2; y < height; y++)
		{
			dst	+= 4 * dst_pitch;
			src_prev = src_curr;
			src_curr = src_next;
			src_next += src_pitch;
			scale_hlq2x4_16_def((UINT16 *)dst, (UINT16 *)(dst + dst_pitch), (UINT16 *)(dst + 2 * dst_pitch), (UINT16 *)(dst + 3 * dst_pitch), (UINT16 *)src_prev, (UINT16 *)src_curr, (UINT16 *)src_next, width);
		}
	}
	else
	{
		for (y = 2; y < height; y++)
		{
			dst += 4 * dst_pitch;
			src_prev = src_curr;
			src_curr = src_next;
			src_next += src_pitch;
			scale_hlq2x4_32_def((UINT32 *)dst, (UINT32 *)(dst + dst_pitch), (UINT32 *)(dst + 2 * dst_pitch), (UINT32 *)(dst + 3 * dst_pitch), (UINT32 *)src_prev, (UINT32 *)src_curr, (UINT32 *)src_next, width);
		}
	}

	dst += 4 * dst_pitch;
	src_prev = src_curr;
	src_curr = src_next;
	if (depth == 15 || depth == 16)
		scale_hlq2x4_16_def((UINT16 *)dst, (UINT16 *)(dst + dst_pitch), (UINT16 *)(dst + 2 * dst_pitch), (UINT16 *)(dst + 3 * dst_pitch), (UINT16 *)src_prev, (UINT16 *)src_curr, (UINT16 *)src_next, width);
	else
		scale_hlq2x4_32_def((UINT32 *)dst, (UINT32 *)(dst + dst_pitch), (UINT32 *)(dst + 2 * dst_pitch), (UINT32 *)(dst + 3 * dst_pitch), (UINT32 *)src_prev, (UINT32 *)src_curr, (UINT32 *)src_next, width);

#ifdef USE_MMX_INTERP_SCALE
	scale_emms();
#endif /* USE_MMX_INTERP_SCALE */

	return 0;
}

//============================================================
//	scale_perform_hlq3x
//============================================================

static int scale_perform_hlq3x(UINT8 *src, UINT8 *dst, int src_pitch, int dst_pitch, int width, int height, int depth)
{
	int y;
	UINT8 *src_prev = src;
	UINT8 *src_curr = src;
	UINT8 *src_next = src + src_pitch;

	interp_init();

	if (depth == 15)
		scale_hlq3x_15_def((UINT16 *)dst, (UINT16 *)(dst + dst_pitch), (UINT16 *)(dst + 2 * dst_pitch), (UINT16 *)src_prev, (UINT16 *)src_curr, (UINT16 *)src_next, width);
	else if (depth == 16)
		scale_hlq3x_16_def((UINT16 *)dst, (UINT16 *)(dst + dst_pitch), (UINT16 *)(dst + 2 * dst_pitch), (UINT16 *)src_prev, (UINT16 *)src_curr, (UINT16 *)src_next, width);
	else
		scale_hlq3x_32_def((UINT32 *)dst, (UINT32 *)(dst + dst_pitch), (UINT32 *)(dst + 2 * dst_pitch), (UINT32 *)src_prev, (UINT32 *)src_curr, (UINT32 *)src_next, width);

	if (depth == 15 || depth == 16)
	{
		for (y = 3; y < height; y++)
		{
			dst += 3 * dst_pitch;
			src_prev = src_curr;
			src_curr = src_next;
			src_next += src_pitch;

			if (depth == 15)
				scale_hlq3x_15_def((UINT16 *)dst, (UINT16 *)(dst + dst_pitch), (UINT16 *)(dst + 2 * dst_pitch), (UINT16 *)src_prev, (UINT16 *)src_curr, (UINT16 *)src_next, width);
			else
				scale_hlq3x_16_def((UINT16 *)dst, (UINT16 *)(dst + dst_pitch), (UINT16 *)(dst + 2 * dst_pitch), (UINT16 *)src_prev, (UINT16 *)src_curr, (UINT16 *)src_next, width);
		}
	}
	else
	{
		for (y = 3; y < height; y++)
		{
			dst += 3 * dst_pitch;
			src_prev = src_curr;
			src_curr = src_next;
			src_next += src_pitch;

			scale_hlq3x_32_def((UINT32 *)dst, (UINT32 *)(dst + dst_pitch), (UINT32 *)(dst + 2 * dst_pitch), (UINT32 *)src_prev, (UINT32 *)src_curr, (UINT32 *)src_next, width);
		}
	}

	dst += 3 * dst_pitch;
	src_prev = src_curr;
	src_curr = src_next;

	if (depth == 15)
		scale_hlq3x_15_def((UINT16 *)dst, (UINT16 *)(dst + dst_pitch), (UINT16 *)(dst + 2 * dst_pitch), (UINT16 *)src_prev, (UINT16 *)src_curr, (UINT16 *)src_next, width);
	if (depth == 16)
		scale_hlq3x_16_def((UINT16 *)dst, (UINT16 *)(dst + dst_pitch), (UINT16 *)(dst + 2 * dst_pitch), (UINT16 *)src_prev, (UINT16 *)src_curr, (UINT16 *)src_next, width);
	else
		scale_hlq3x_32_def((UINT32 *)dst, (UINT32 *)(dst + dst_pitch), (UINT32 *)(dst + 2 * dst_pitch), (UINT32 *)src_prev, (UINT32 *)src_curr, (UINT32 *)src_next, width);

#ifdef USE_MMX_INTERP_SCALE
	scale_emms();
#endif /* USE_MMX_INTERP_SCALE */

	return 0;
}

#ifdef USE_4X_SCALE
//============================================================
//	scale_perform_hlq4x
//============================================================

static int scale_perform_hlq4x(UINT8 *src, UINT8 *dst, int src_pitch, int dst_pitch, int width, int height, int depth)
{
	int y;
	UINT8 *src_prev = src;
	UINT8 *src_curr = src;
	UINT8 *src_next = src + src_pitch;

	interp_init();

	if (depth == 15)
		scale_hlq4x_15_def((UINT16 *)dst, (UINT16 *)(dst + dst_pitch), (UINT16 *)(dst + 2 * dst_pitch), (UINT16 *)(dst + 3 * dst_pitch), (UINT16 *)src_prev, (UINT16 *)src_curr, (UINT16 *)src_next, width);
	else if (depth == 16)
		scale_hlq4x_16_def((UINT16 *)dst, (UINT16 *)(dst + dst_pitch), (UINT16 *)(dst + 2 * dst_pitch), (UINT16 *)(dst + 3 * dst_pitch), (UINT16 *)src_prev, (UINT16 *)src_curr, (UINT16 *)src_next, width);
	else
		scale_hlq4x_32_def((UINT32 *)dst, (UINT32 *)(dst + dst_pitch), (UINT32 *)(dst + 2 * dst_pitch), (UINT32 *)(dst + 3 * dst_pitch), (UINT32 *)src_prev, (UINT32 *)src_curr, (UINT32 *)src_next, width);

	if (depth == 15 || depth == 16)
	{
		for (y = 4; y < height; y++)
		{
			dst += 4 * dst_pitch;
			src_prev = src_curr;
			src_curr = src_next;
			src_next += src_pitch;

			if (depth == 15)
				scale_hlq4x_15_def((UINT16 *)dst, (UINT16 *)(dst + dst_pitch), (UINT16 *)(dst + 2 * dst_pitch), (UINT16 *)(dst + 3 * dst_pitch), (UINT16 *)src_prev, (UINT16 *)src_curr, (UINT16 *)src_next, width);
			else
				scale_hlq4x_16_def((UINT16 *)dst, (UINT16 *)(dst + dst_pitch), (UINT16 *)(dst + 2 * dst_pitch), (UINT16 *)(dst + 3 * dst_pitch), (UINT16 *)src_prev, (UINT16 *)src_curr, (UINT16 *)src_next, width);
		}
	}
	else
	{
		for (y = 4; y < height; y++)
		{
			dst += 4 * dst_pitch;
			src_prev = src_curr;
			src_curr = src_next;
			src_next += src_pitch;

			scale_hlq4x_32_def((UINT32 *)dst, (UINT32 *)(dst + dst_pitch), (UINT32 *)(dst + 2 * dst_pitch), (UINT32 *)(dst + 3 * dst_pitch), (UINT32 *)src_prev, (UINT32 *)src_curr, (UINT32 *)src_next, width);
		}
	}

	dst += 4 * dst_pitch;
	src_prev = src_curr;
	src_curr = src_next;

	if (depth == 15)
		scale_hlq4x_15_def((UINT16 *)dst, (UINT16 *)(dst + dst_pitch), (UINT16 *)(dst + 2 * dst_pitch), (UINT16 *)(dst + 3 * dst_pitch), (UINT16 *)src_prev, (UINT16 *)src_curr, (UINT16 *)src_next, width);
	else if (depth == 16)
		scale_hlq4x_16_def((UINT16 *)dst, (UINT16 *)(dst + dst_pitch), (UINT16 *)(dst + 2 * dst_pitch), (UINT16 *)(dst + 3 * dst_pitch), (UINT16 *)src_prev, (UINT16 *)src_curr, (UINT16 *)src_next, width);
	else
		scale_hlq4x_32_def((UINT32 *)dst, (UINT32 *)(dst + dst_pitch), (UINT32 *)(dst + 2 * dst_pitch), (UINT32 *)(dst + 3 * dst_pitch), (UINT32 *)src_prev, (UINT32 *)src_curr, (UINT32 *)src_next, width);

#ifdef USE_MMX_INTERP_SCALE
	scale_emms();
#endif /* USE_MMX_INTERP_SCALE */

	return 0;
}
#endif /* USE_4X_SCALE */
