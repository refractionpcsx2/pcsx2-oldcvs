/* 
 *	Copyright (C) 2003-2005 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 *	Special Notes: 
 *
 *	Register definitions and most of the enums originate from sps2dev-0.4.0
 *	Copyright (C) 2002 Terratron Technologies Inc.  All Rights Reserved.
 *
 */

#pragma once

// 

#pragma pack(push, 1)

//
// sps2registers.h
//

enum GS_REG
{
	GS_PMODE	= 0x12000000,
	GS_SMODE1	= 0x12000010,
	GS_SMODE2	= 0x12000020,
	GS_SRFSH	= 0x12000030,
	GS_SYNCH1	= 0x12000040,
	GS_SYNCH2	= 0x12000050,
	GS_SYNCV	= 0x12000060,
	GS_DISPFB1	= 0x12000070,
	GS_DISPLAY1	= 0x12000080,
	GS_DISPFB2	= 0x12000090,
	GS_DISPLAY2	= 0x120000a0,
	GS_EXTBUF	= 0x120000b0,
	GS_EXTDATA	= 0x120000c0,
	GS_EXTWRITE	= 0x120000d0,
	GS_BGCOLOR	= 0x120000e0,
	GS_UNKNOWN	= 0x12000400,
	GS_CSR		= 0x12001000,
	GS_IMR		= 0x12001010,
	GS_BUSDIR	= 0x12001040,
	GS_SIGLBLID	= 0x12001080
};

enum GS_PRIM
{
	GS_POINTLIST		= 0,
	GS_LINELIST			= 1,
	GS_LINESTRIP		= 2,
	GS_TRIANGLELIST		= 3,
	GS_TRIANGLESTRIP	= 4,
	GS_TRIANGLEFAN		= 5,
	GS_SPRITE			= 6,
	GS_INVALID			= 7,
};

enum GIF_REG
{
	GIF_REG_PRIM	= 0x00,
	GIF_REG_RGBA	= 0x01,
	GIF_REG_STQ		= 0x02,
	GIF_REG_UV		= 0x03,
	GIF_REG_XYZF2	= 0x04,
	GIF_REG_XYZ2	= 0x05,
	GIF_REG_TEX0_1	= 0x06,
	GIF_REG_TEX0_2	= 0x07,
	GIF_REG_CLAMP_1	= 0x08,
	GIF_REG_CLAMP_2	= 0x09,
	GIF_REG_FOG		= 0x0a,
	GIF_REG_XYZF3	= 0x0c,
	GIF_REG_XYZ3	= 0x0d,
	GIF_REG_A_D		= 0x0e,
	GIF_REG_NOP		= 0x0f,
};

enum GIF_A_D_REG
{
	GIF_A_D_REG_PRIM		= 0x00,
	GIF_A_D_REG_RGBAQ		= 0x01,
	GIF_A_D_REG_ST			= 0x02,
	GIF_A_D_REG_UV			= 0x03,
	GIF_A_D_REG_XYZF2		= 0x04,
	GIF_A_D_REG_XYZ2		= 0x05,
	GIF_A_D_REG_TEX0_1		= 0x06,
	GIF_A_D_REG_TEX0_2		= 0x07,
	GIF_A_D_REG_CLAMP_1		= 0x08,
	GIF_A_D_REG_CLAMP_2		= 0x09,
	GIF_A_D_REG_FOG			= 0x0a,
	GIF_A_D_REG_XYZF3		= 0x0c,
	GIF_A_D_REG_XYZ3		= 0x0d,
	GIF_A_D_REG_NOP			= 0x0f,
	GIF_A_D_REG_TEX1_1		= 0x14,
	GIF_A_D_REG_TEX1_2		= 0x15,
	GIF_A_D_REG_TEX2_1		= 0x16,
	GIF_A_D_REG_TEX2_2		= 0x17,
	GIF_A_D_REG_XYOFFSET_1	= 0x18,
	GIF_A_D_REG_XYOFFSET_2	= 0x19,
	GIF_A_D_REG_PRMODECONT	= 0x1a,
	GIF_A_D_REG_PRMODE		= 0x1b,
	GIF_A_D_REG_TEXCLUT		= 0x1c,
	GIF_A_D_REG_SCANMSK		= 0x22,
	GIF_A_D_REG_MIPTBP1_1	= 0x34,
	GIF_A_D_REG_MIPTBP1_2	= 0x35,
	GIF_A_D_REG_MIPTBP2_1	= 0x36,
	GIF_A_D_REG_MIPTBP2_2	= 0x37,
	GIF_A_D_REG_TEXA		= 0x3b,
	GIF_A_D_REG_FOGCOL		= 0x3d,
	GIF_A_D_REG_TEXFLUSH	= 0x3f,
	GIF_A_D_REG_SCISSOR_1	= 0x40,
	GIF_A_D_REG_SCISSOR_2	= 0x41,
	GIF_A_D_REG_ALPHA_1		= 0x42,
	GIF_A_D_REG_ALPHA_2		= 0x43,
	GIF_A_D_REG_DIMX		= 0x44,
	GIF_A_D_REG_DTHE		= 0x45,
	GIF_A_D_REG_COLCLAMP	= 0x46,
	GIF_A_D_REG_TEST_1		= 0x47,
	GIF_A_D_REG_TEST_2		= 0x48,
	GIF_A_D_REG_PABE		= 0x49,
	GIF_A_D_REG_FBA_1		= 0x4a,
	GIF_A_D_REG_FBA_2		= 0x4b,
	GIF_A_D_REG_FRAME_1		= 0x4c,
	GIF_A_D_REG_FRAME_2		= 0x4d,
	GIF_A_D_REG_ZBUF_1		= 0x4e,
	GIF_A_D_REG_ZBUF_2		= 0x4f,
	GIF_A_D_REG_BITBLTBUF	= 0x50,
	GIF_A_D_REG_TRXPOS		= 0x51,
	GIF_A_D_REG_TRXREG		= 0x52,
	GIF_A_D_REG_TRXDIR		= 0x53,
	GIF_A_D_REG_HWREG		= 0x54,
	GIF_A_D_REG_SIGNAL		= 0x60,
	GIF_A_D_REG_FINISH		= 0x61,
	GIF_A_D_REG_LABEL		= 0x62,
};

enum GIF_FLG
{
	GIF_FLG_PACKED	= 0,
	GIF_FLG_REGLIST	= 1,
	GIF_FLG_IMAGE	= 2,
	GIF_FLG_IMAGE2	= 3
};

enum PSM
{
	PSM_PSMCT32		= 0,
	PSM_PSMCT24		= 1,
	PSM_PSMCT16		= 2,
	PSM_PSMCT16S	= 10,
	PSM_PSMT8		= 19,
	PSM_PSMT4		= 20,
	PSM_PSMT8H		= 27,
	PSM_PSMT4HL		= 36,
	PSM_PSMT4HH		= 44,
	PSM_PSMZ32		= 48,
	PSM_PSMZ24		= 49,
	PSM_PSMZ16		= 50,
	PSM_PSMZ16S		= 58,
};

//
// sps2regstructs.h
//

#define REG64(name) \
union name			\
{					\
	UINT64 i64;		\
	UINT32 ai32[2];	\
	struct {		\

#define REG128(name)\
union name			\
{					\
	UINT64 ai64[2];	\
	UINT32 ai32[4];	\
	struct {		\

#define REG64_(prefix, name) REG64(prefix##name)
#define REG128_(prefix, name) REG128(prefix##name)

#define REG_END }; };
#define REG_END2 };

#define REG64_SET(name) \
union name			\
{					\
	UINT64 i64;		\
	UINT32 ai32[2];	\

#define REG128_SET(name)\
union name			\
{					\
	UINT64 ai64[2];	\
	UINT32 ai32[4];	\

#define REG_SET_END };

REG64_(GSReg, BGCOLOR)
	UINT32 R:8;
	UINT32 G:8;
	UINT32 B:8;
	UINT32 _PAD1:8;
	UINT32 _PAD2:32;
REG_END

REG64_(GSReg, BUSDIR)
	UINT32 DIR:1;
	UINT32 _PAD1:31;
	UINT32 _PAD2:32;
REG_END

REG64_(GSReg, CSR)
	UINT32 rSIGNAL:1;
	UINT32 rFINISH:1;
	UINT32 rHSINT:1;
	UINT32 rVSINT:1;
	UINT32 rEDWINT:1;
	UINT32 rZERO1:1;
	UINT32 rZERO2:1;
	UINT32 r_PAD1:1;
	UINT32 rFLUSH:1;
	UINT32 rRESET:1;
	UINT32 r_PAD2:2;
	UINT32 rNFIELD:1;
	UINT32 rFIELD:1;
	UINT32 rFIFO:2;
	UINT32 rREV:8;
	UINT32 rID:8;
	UINT32 wSIGNAL:1;
	UINT32 wFINISH:1;
	UINT32 wHSINT:1;
	UINT32 wVSINT:1;
	UINT32 wEDWINT:1;
	UINT32 wZERO1:1;
	UINT32 wZERO2:1;
	UINT32 w_PAD1:1;
	UINT32 wFLUSH:1;
	UINT32 wRESET:1;
	UINT32 w_PAD2:2;
	UINT32 wNFIELD:1;
	UINT32 wFIELD:1;
	UINT32 wFIFO:2;
	UINT32 wREV:8;
	UINT32 wID:8;
REG_END

REG64_(GSReg, DISPFB) // (-1/2)
	UINT32 FBP:9;
	UINT32 FBW:6;
	UINT32 PSM:5;
	UINT32 _PAD:12;
	UINT32 DBX:11;
	UINT32 DBY:11;
	UINT32 _PAD2:10;
REG_END

REG64_(GSReg, DISPLAY) // (-1/2)
	UINT32 DX:12;
	UINT32 DY:11;
	UINT32 MAGH:4;
	UINT32 MAGV:2;
	UINT32 _PAD:3;
	UINT32 DW:12;
	UINT32 DH:11;
	UINT32 _PAD2:9;
REG_END

REG64_(GSReg, EXTBUF)
	UINT32 EXBP:14;
	UINT32 EXBW:6;
	UINT32 FBIN:2;
	UINT32 WFFMD:1;
	UINT32 EMODA:2;
	UINT32 EMODC:2;
	UINT32 _PAD1:5;
	UINT32 WDX:11;
	UINT32 WDY:11;
	UINT32 _PAD2:10;
REG_END

REG64_(GSReg, EXTDATA)
	UINT32 SX:12;
	UINT32 SY:11;
	UINT32 SMPH:4;
	UINT32 SMPV:2;
	UINT32 _PAD1:3;
	UINT32 WW:12;
	UINT32 WH:11;
	UINT32 _PAD2:9;
REG_END

REG64_(GSReg, EXTWRITE)
	UINT32 WRITE:1;
	UINT32 _PAD1:31;
	UINT32 _PAD2:32;
REG_END

REG64_(GSReg, IMR)
	UINT32 _PAD1:8;
	UINT32 SIGMSK:1;
	UINT32 FINISHMSK:1;
	UINT32 HSMSK:1;
	UINT32 VSMSK:1;
	UINT32 EDWMSK:1;
	UINT32 _PAD2:19;
	UINT32 _PAD3:32;
REG_END

REG64_(GSReg, PMODE)
	UINT32 EN1:1;
	UINT32 EN2:1;
	UINT32 CRTMD:3;
	UINT32 MMOD:1;
	UINT32 AMOD:1;
	UINT32 SLBG:1;
	UINT32 ALP:8;
	UINT32 _PAD:16;
	UINT32 _PAD1:32;
REG_END

REG64_(GSReg, SIGLBLID)
	UINT32 SIGID:32;
	UINT32 LBLID:32;
REG_END

REG64_(GSReg, SMODE1)
	UINT32 RC:3;
	UINT32 LC:7;
	UINT32 T1248:2;
	UINT32 SLCK:1;
	UINT32 CMOD:2;
	UINT32 EX:1;
	UINT32 PRST:1;
	UINT32 SINT:1;
	UINT32 XPCK:1;
	UINT32 PCK2:2;
	UINT32 SPML:4;
	UINT32 GCONT:1;
	UINT32 PHS:1;
	UINT32 PVS:1;
	UINT32 PEHS:1;
	UINT32 PEVS:1;
	UINT32 CLKSEL:2;
	UINT32 NVCK:1;
	UINT32 SLCK2:1;
	UINT32 VCKSEL:2;
	UINT32 VHP:1;
	UINT32 _PAD1:27;
REG_END

REG64_(GSReg, SMODE2)
	UINT32 INT:1;
	UINT32 FFMD:1;
	UINT32 DPMS:2;
	UINT32 _PAD2:28;
	UINT32 _PAD3:32;
REG_END

REG64_SET(GSReg)
	GSRegBGCOLOR	BGCOLOR;
	GSRegBUSDIR		BUSDIR;
	GSRegCSR		CSR;
	GSRegDISPFB		DISPFB;
	GSRegDISPLAY	DISPLAY;
	GSRegEXTBUF		EXTBUF;
	GSRegEXTDATA	EXTDATA;
	GSRegEXTWRITE	EXTWRITE;
	GSRegIMR		IMR;
	GSRegPMODE		PMODE;
	GSRegSIGLBLID	SIGLBLID;
	GSRegSMODE1		SMODE1; 
	GSRegSMODE2		SMODE2; 
REG_SET_END

//
// sps2tags.h
//

#define SET_GIF_REG(gifTag, iRegNo, uiValue) \
	{((GIFTag*)&gifTag)->ai64[1] |= (((uiValue) & 0xf) << ((iRegNo) << 2));}

#ifdef _M_AMD64
#define GET_GIF_REG(tag, reg) \
	(((tag).ai64[1] >> ((reg) << 2)) & 0xf)
#else
#define GET_GIF_REG(tag, reg) \
	(((tag).ai32[2 + ((reg) >> 3)] >> (((reg) & 7) << 2)) & 0xf)
#endif

//
// GIFTag

REG128(GIFTag)
	UINT32 NLOOP:15;
	UINT32 EOP:1;
	UINT32 _PAD1:16;
	UINT32 _PAD2:14;
	UINT32 PRE:1;
	UINT32 PRIM:11;
	UINT32 FLG:2; // enum GIF_FLG
	UINT32 NREG:4;
	UINT64 REGS:64;
REG_END

// GIFReg

REG64_(GIFReg, ALPHA)
	UINT32 A:2;
	UINT32 B:2;
	UINT32 C:2;
	UINT32 D:2;
	UINT32 _PAD1:24;
	UINT32 FIX:8;
	UINT32 _PAD2:24;
REG_END

REG64_(GIFReg, BITBLTBUF)
	UINT32 SBP:14;
	UINT32 _PAD1:2;
	UINT32 SBW:6;
	UINT32 _PAD2:2;
	UINT32 SPSM:6;
	UINT32 _PAD3:2;
	UINT32 DBP:14;
	UINT32 _PAD4:2;
	UINT32 DBW:6;
	UINT32 _PAD5:2;
	UINT32 DPSM:6;
	UINT32 _PAD6:2;
REG_END

REG64_(GIFReg, CLAMP)
	UINT64 WMS:2;
	UINT64 WMT:2;
	UINT64 MINU:10;
	UINT64 MAXU:10;
	UINT64 MINV:10;
	UINT64 MAXV:10;
	UINT64 _PAD:20;
REG_END

REG64_(GIFReg, COLCLAMP)
	UINT32 CLAMP:1;
	UINT32 _PAD1:31;
	UINT32 _PAD2:32;
REG_END

REG64_(GIFReg, DIMX)
	UINT32 DM00:3;
	UINT32 _PAD00:1;
	UINT32 DM01:3;
	UINT32 _PAD01:1;
	UINT32 DM02:3;
	UINT32 _PAD02:1;
	UINT32 DM03:3;
	UINT32 _PAD03:1;

	UINT32 DM10:3;
	UINT32 _PAD10:1;
	UINT32 DM11:3;
	UINT32 _PAD11:1;
	UINT32 DM12:3;
	UINT32 _PAD12:1;
	UINT32 DM13:3;
	UINT32 _PAD13:1;

	UINT32 DM20:3;
	UINT32 _PAD20:1;
	UINT32 DM21:3;
	UINT32 _PAD21:1;
	UINT32 DM22:3;
	UINT32 _PAD22:1;
	UINT32 DM23:3;
	UINT32 _PAD23:1;

	UINT32 DM30:3;
	UINT32 _PAD30:1;
	UINT32 DM31:3;
	UINT32 _PAD31:1;
	UINT32 DM32:3;
	UINT32 _PAD32:1;
	UINT32 DM33:3;
	UINT32 _PAD33:1;
REG_END

REG64_(GIFReg, DTHE)
	UINT32 DTHE:1;
	UINT32 _PAD1:31;
	UINT32 _PAD2:32;
REG_END

REG64_(GIFReg, FBA)
	UINT32 FBA:1;
	UINT32 _PAD1:31;
	UINT32 _PAD2:32;
REG_END

REG64_(GIFReg, FINISH)
	UINT32 _PAD1:32;
	UINT32 _PAD2:32;
REG_END

REG64_(GIFReg, FOG)
	UINT32 _PAD1:32;
	UINT32 _PAD2:24;
	UINT32 F:8;
REG_END

REG64_(GIFReg, FOGCOL)
	UINT32 FCR:8;
	UINT32 FCG:8;
	UINT32 FCB:8;
	UINT32 _PAD1:8;
	UINT32 _PAD2:32;
REG_END

REG64_(GIFReg, FRAME)
	UINT32 FBP:9;
	UINT32 _PAD1:7;
	UINT32 FBW:6;
	UINT32 _PAD2:2;
	UINT32 PSM:6;
	UINT32 _PAD3:2;
	UINT32 FBMSK:32;
REG_END2
	UINT32 Block() {return FBP<<5;}
REG_END2

REG64_(GIFReg, HWREG)
	UINT32 DATA_LOWER:32;
	UINT32 DATA_UPPER:32;
REG_END

REG64_(GIFReg, LABEL)
	UINT32 ID:32;
	UINT32 IDMSK:32;
REG_END

REG64_(GIFReg, MIPTBP1)
	UINT64 TBP1:14;
	UINT64 TBW1:6;
	UINT64 TBP2:14;
	UINT64 TBW2:6;
	UINT64 TBP3:14;
	UINT64 TBW3:6;
	UINT64 _PAD:4;
REG_END

REG64_(GIFReg, MIPTBP2)
	UINT64 TBP4:14;
	UINT64 TBW4:6;
	UINT64 TBP5:14;
	UINT64 TBW5:6;
	UINT64 TBP6:14;
	UINT64 TBW6:6;
	UINT64 _PAD:4;
REG_END

REG64_(GIFReg, NOP)
	UINT32 _PAD1:32;
	UINT32 _PAD2:32;
REG_END

REG64_(GIFReg, PABE)
	UINT32 PABE:1;
	UINT32 _PAD1:31;
	UINT32 _PAD2:32;
REG_END

REG64_(GIFReg, PRIM)
	UINT32 PRIM:3;
	UINT32 IIP:1;
	UINT32 TME:1;
	UINT32 FGE:1;
	UINT32 ABE:1;
	UINT32 AA1:1;
	UINT32 FST:1;
	UINT32 CTXT:1;
	UINT32 FIX:1;
	UINT32 _PAD1:21;
	UINT32 _PAD2:32;
REG_END

REG64_(GIFReg, PRMODE)
	UINT32 _PRIM:3;
	UINT32 IIP:1;
	UINT32 TME:1;
	UINT32 FGE:1;
	UINT32 ABE:1;
	UINT32 AA1:1;
	UINT32 FST:1;
	UINT32 CTXT:1;
	UINT32 FIX:1;
	UINT32 _PAD2:21;
	UINT32 _PAD3:32;
REG_END

REG64_(GIFReg, PRMODECONT)
	UINT32 AC:1;
	UINT32 _PAD1:31;
	UINT32 _PAD2:32;
REG_END

REG64_(GIFReg, RGBAQ)
	UINT32 R:8;
	UINT32 G:8;
	UINT32 B:8;
	UINT32 A:8;
	float Q;
REG_END

REG64_(GIFReg, SCANMSK)
	UINT32 MSK:2;
	UINT32 _PAD1:30;
	UINT32 _PAD2:32;
REG_END

REG64_(GIFReg, SCISSOR)
	UINT32 SCAX0:11;
	UINT32 _PAD1:5;
	UINT32 SCAX1:11;
	UINT32 _PAD2:5;
	UINT32 SCAY0:11;
	UINT32 _PAD3:5;
	UINT32 SCAY1:11;
	UINT32 _PAD4:5;
REG_END

REG64_(GIFReg, SIGNAL)
	UINT32 ID:32;
	UINT32 IDMSK:32;
REG_END

REG64_(GIFReg, ST)
	float S;
	float T;
REG_END

REG64_(GIFReg, TEST)
	UINT32 ATE:1;
	UINT32 ATST:3;
	UINT32 AREF:8;
	UINT32 AFAIL:2;
	UINT32 DATE:1;
	UINT32 DATM:1;
	UINT32 ZTE:1;
	UINT32 ZTST:2;
	UINT32 _PAD1:13;
	UINT32 _PAD2:32;
REG_END

REG64_(GIFReg, TEX0)
	UINT64 TBP0:14;
	UINT64 TBW:6;
	UINT64 PSM:6;
	UINT64 TW:4;
	UINT64 TH:4;
	UINT64 TCC:1;
	UINT64 TFX:2;
	UINT64 CBP:14;
	UINT64 CPSM:4;
	UINT64 CSM:1;
	UINT64 CSA:5;
	UINT64 CLD:3;
REG_END

REG64_(GIFReg, TEX1)
	UINT32 LCM:1;
	UINT32 _PAD1:1;
	UINT32 MXL:3;
	UINT32 MMAG:1;
	UINT32 MMIN:3;
	UINT32 MTBA:1;
	UINT32 _PAD2:9;
	UINT32 L:2;
	UINT32 _PAD3:11;
	UINT32 K:12;
	UINT32 _PAD4:20;
REG_END

REG64_(GIFReg, TEX2)
	UINT32 _PAD1:20;
	UINT32 PSM:6;
	UINT32 _PAD2:6;
	UINT32 _PAD3:5;
	UINT32 CBP:14;
	UINT32 CPSM:4;
	UINT32 CSM:1;
	UINT32 CSA:5;
	UINT32 CLD:3;
REG_END

REG64_(GIFReg, TEXA)
	UINT32 TA0:8;
	UINT32 _PAD1:7;
	UINT32 AEM:1;
	UINT32 _PAD2:16;
	UINT32 TA1:8;
	UINT32 _PAD3:24;
REG_END

REG64_(GIFReg, TEXCLUT)
	UINT32 CBW:6;
	UINT32 COU:6;
	UINT32 COV:10;
	UINT32 _PAD1:10;
	UINT32 _PAD2:32;
REG_END

REG64_(GIFReg, TEXFLUSH)
	UINT32 _PAD1:32;
	UINT32 _PAD2:32;
REG_END

REG64_(GIFReg, TRXDIR)
	UINT32 XDIR:2;
	UINT32 _PAD1:30;
	UINT32 _PAD2:32;
REG_END

REG64_(GIFReg, TRXPOS)
	UINT32 SSAX:11;
	UINT32 _PAD1:5;
	UINT32 SSAY:11;
	UINT32 _PAD2:5;
	UINT32 DSAX:11;
	UINT32 _PAD3:5;
	UINT32 DSAY:11;
	UINT32 DIR:2;
	UINT32 _PAD4:3;
REG_END

REG64_(GIFReg, TRXREG)
	UINT32 RRW:12;
	UINT32 _PAD1:20;
	UINT32 RRH:12;
	UINT32 _PAD2:20;
REG_END

REG64_(GIFReg, UV)
	UINT32 U:14;
	UINT32 _PAD1:2;
	UINT32 V:14;
	UINT32 _PAD2:2;
	UINT32 _PAD3:32;
REG_END

REG64_(GIFReg, XYOFFSET)
	UINT32 OFX:16;
	UINT32 _PAD1:16;
	UINT32 OFY:16;
	UINT32 _PAD2:16;
REG_END

REG64_(GIFReg, XYZ)
	UINT32 X:16;
	UINT32 Y:16;
	UINT32 Z:32;
REG_END

REG64_(GIFReg, XYZF)
	UINT32 X:16;
	UINT32 Y:16;
	UINT32 Z:24;
	UINT32 F:8;
REG_END

REG64_(GIFReg, ZBUF)
	UINT32 ZBP:9;
	UINT32 _PAD1:15;
	// UINT32 PSM:4;
	// UINT32 _PAD2:4;
	UINT32 PSM:6;
	UINT32 _PAD2:2;
	UINT32 ZMSK:1;
	UINT32 _PAD3:31;
REG_END

REG64_SET(GIFReg)
	GIFRegALPHA			ALPHA;
	GIFRegBITBLTBUF		BITBLTBUF;
	GIFRegCLAMP			CLAMP;
	GIFRegCOLCLAMP		COLCLAMP;
	GIFRegDIMX			DIMX;
	GIFRegDTHE			DTHE;
	GIFRegFBA			FBA;
	GIFRegFINISH		FINISH;
	GIFRegFOG			FOG;
	GIFRegFOGCOL		FOGCOL;
	GIFRegFRAME			FRAME;
	GIFRegHWREG			HWREG;
	GIFRegLABEL			LABEL;
	GIFRegMIPTBP1		MIPTBP1;
	GIFRegMIPTBP2		MIPTBP2;
	GIFRegNOP			NOP;
	GIFRegPABE			PABE;
	GIFRegPRIM			PRIM;
	GIFRegPRMODE		PRMODE;
	GIFRegPRMODECONT	PRMODECONT;
	GIFRegRGBAQ			RGBAQ;
	GIFRegSCANMSK		SCANMSK;
	GIFRegSCISSOR		SCISSOR;
	GIFRegSIGNAL		SIGNAL;
	GIFRegST			ST;
	GIFRegTEST			TEST;
	GIFRegTEX0			TEX0;
	GIFRegTEX1			TEX1;
	GIFRegTEX2			TEX2;
	GIFRegTEXA			TEXA;
	GIFRegTEXCLUT		TEXCLUT;
	GIFRegTEXFLUSH		TEXFLUSH;
	GIFRegTRXDIR		TRXDIR;
	GIFRegTRXPOS		TRXPOS;
	GIFRegTRXREG		TRXREG;
	GIFRegUV			UV;
	GIFRegXYOFFSET		XYOFFSET;
	GIFRegXYZ			XYZ;
	GIFRegXYZF			XYZF;
	GIFRegZBUF			ZBUF;
REG_SET_END

// GIFPacked

REG128_(GIFPacked, PRIM)
	UINT32 PRIM:11;
	UINT32 _PAD1:21;
	UINT32 _PAD2:32;
	UINT32 _PAD3:32;
	UINT32 _PAD4:32;
REG_END

REG128_(GIFPacked, RGBA)
	UINT32 R:8;
	UINT32 _PAD1:24;
	UINT32 G:8;
	UINT32 _PAD2:24;
	UINT32 B:8;
	UINT32 _PAD3:24;
	UINT32 A:8;
	UINT32 _PAD4:24;
REG_END

REG128_(GIFPacked, STQ)
	float S;
	float T;
	float Q;
	UINT32 _PAD1:32;
REG_END

REG128_(GIFPacked, UV)
	UINT32 U:14;
	UINT32 _PAD1:18;
	UINT32 V:14;
	UINT32 _PAD2:18;
	UINT32 _PAD3:32;
	UINT32 _PAD4:32;
REG_END

REG128_(GIFPacked, XYZF2)
	UINT32 X:16;
	UINT32 _PAD1:16;
	UINT32 Y:16;
	UINT32 _PAD2:16;
	UINT32 _PAD3:4;
	UINT32 Z:24;
	UINT32 _PAD4:4;
	UINT32 _PAD5:4;
	UINT32 F:8;
	UINT32 _PAD6:3;
	UINT32 ADC:1;
	UINT32 _PAD7:16;
REG_END

REG128_(GIFPacked, XYZ2)
	UINT32 X:16;
	UINT32 _PAD1:16;
	UINT32 Y:16;
	UINT32 _PAD2:16;
	UINT32 Z:32;
	UINT32 _PAD3:15;
	UINT32 ADC:1;
	UINT32 _PAD4:16;
REG_END

REG128_(GIFPacked, FOG)
	UINT32 _PAD1:32;
	UINT32 _PAD2:32;
	UINT32 _PAD3:32;
	UINT32 _PAD4:4;
	UINT32 F:8;
	UINT32 _PAD5:20;
REG_END

REG128_(GIFPacked, A_D)
	UINT64 DATA:64;
	UINT32 ADDR:8; // enum GIF_A_D_REG
	UINT32 _PAD1:24;
	UINT32 _PAD2:32;
REG_END

REG128_(GIFPacked, NOP)
	UINT32 _PAD1:32;
	UINT32 _PAD2:32;
	UINT32 _PAD3:32;
	UINT32 _PAD4:32;
REG_END

REG128_SET(GIFPackedReg)
	GIFReg			r;
	GIFPackedPRIM	PRIM;
	GIFPackedRGBA	RGBA;
	GIFPackedSTQ	STQ;
	GIFPackedUV		UV;
	GIFPackedXYZF2	XYZF2;
	GIFPackedXYZ2	XYZ2;
	GIFPackedFOG	FOG;
	GIFPackedA_D	A_D;
	GIFPackedNOP	NOP;
REG_SET_END

#pragma pack(pop)

enum {KEYPRESS=1, KEYRELEASE=2};
struct keyEvent {UINT32 key, event;};

enum {FREEZE_LOAD=0, FREEZE_SAVE=1, FREEZE_SIZE=2};
struct freezeData {int size; BYTE* data;};

enum stateType {ST_WRITE, ST_TRANSFER, ST_VSYNC}; 
