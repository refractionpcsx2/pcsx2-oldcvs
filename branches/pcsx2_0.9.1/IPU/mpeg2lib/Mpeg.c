/*
 * Mpeg.c
 * Copyright (C) 2000-2002 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 * Modified by Florin for PCSX2 emu
 *
 * This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 * See http://libmpeg2.sourceforge.net/ for updates.
 *
 * mpeg2dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * mpeg2dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "Mpeg.h"
#include "Vlc.h"

#define BigEndian _byteswap_ulong

extern void (* mpeg2_idct_copy) (s16 * block, u8* dest, int stride);
/* JayteeMaster: changed dest to 16 bit signed */
extern void (* mpeg2_idct_add) (int last, s16 * block,
				/*u8*/s16* dest, int stride);

extern int IPU0dma(const void* pMem, int size);

/* JayteeMaster: remove static attribute */
/*static */int non_linear_quantizer_scale [] = {
     0,  1,  2,  3,  4,  5,   6,   7,
     8, 10, 12, 14, 16, 18,  20,  22,
    24, 28, 32, 36, 40, 44,  48,  52,
    56, 64, 72, 80, 88, 96, 104, 112
};

extern tIPU_BP g_BP;

/* Bitstream and buffer needs to be realocated inorder for sucessful
   reading of the old data. Here the old data stored in the 2nd slot
   of the internal buffer is copied to 1st slot, and the new data read
   into 1st slot is copied to the 2nd slot. Which will later be copied
   back to the 1st slot when 128bits have been read.
*/
extern void ReorderBitstream();

int get_macroblock_modes (decoder_t * const decoder)
{
#define bit_buf (decoder->bitstream_buf)
#define bits (decoder->bitstream_bits)
#define bit_ptr (decoder->bitstream_ptr)
    int macroblock_modes;
    const MBtab * tab;

    switch (decoder->coding_type) {
    case I_TYPE:

	macroblock_modes = UBITS (bit_buf, 2);

	if( macroblock_modes == 0 )
		return 0; // error

	tab = MB_I + (macroblock_modes>>1);
	DUMPBITS (bit_buf, bits, tab->len);
	macroblock_modes = tab->modes;

	if ((! (decoder->frame_pred_frame_dct)) &&
	    (decoder->picture_structure == FRAME_PICTURE)) {
	    macroblock_modes |= UBITS (bit_buf, 1) * DCT_TYPE_INTERLACED;
	    DUMPBITS (bit_buf, bits, 1);
	}

	return macroblock_modes;

    case P_TYPE:

	macroblock_modes = UBITS (bit_buf, 6);

	if( macroblock_modes == 0 )
		return 0; // error

	tab = MB_P + (macroblock_modes>>1);
	DUMPBITS (bit_buf, bits, tab->len);
	macroblock_modes = tab->modes;

	if (decoder->picture_structure != FRAME_PICTURE) {
	    if (macroblock_modes & MACROBLOCK_MOTION_FORWARD) {
		macroblock_modes |= UBITS (bit_buf, 2) * MOTION_TYPE_BASE;
		DUMPBITS (bit_buf, bits, 2);
	    }
	    return macroblock_modes;
	} else if (decoder->frame_pred_frame_dct) {
	    if (macroblock_modes & MACROBLOCK_MOTION_FORWARD)
		macroblock_modes |= MC_FRAME;
	    return macroblock_modes;
	} else {
	    if (macroblock_modes & MACROBLOCK_MOTION_FORWARD) {
		macroblock_modes |= UBITS (bit_buf, 2) * MOTION_TYPE_BASE;
		DUMPBITS (bit_buf, bits, 2);
	    }
	    if (macroblock_modes & (MACROBLOCK_INTRA | MACROBLOCK_PATTERN)) {
		macroblock_modes |= UBITS (bit_buf, 1) * DCT_TYPE_INTERLACED;
		DUMPBITS (bit_buf, bits, 1);
	    }
	    return macroblock_modes;
	}

    case B_TYPE:

	macroblock_modes = UBITS (bit_buf, 6);

	if( macroblock_modes == 0 )
		return 0; // error
	tab = MB_B + macroblock_modes;
	DUMPBITS (bit_buf, bits, tab->len);
	macroblock_modes = tab->modes;

	if (decoder->picture_structure != FRAME_PICTURE) {
	    if (! (macroblock_modes & MACROBLOCK_INTRA)) {
		macroblock_modes |= UBITS (bit_buf, 2) * MOTION_TYPE_BASE;
		DUMPBITS (bit_buf, bits, 2);
	    }
	    return macroblock_modes;
	} else if (decoder->frame_pred_frame_dct) {
	    /* if (! (macroblock_modes & MACROBLOCK_INTRA)) */
	    macroblock_modes |= MC_FRAME;
	    return macroblock_modes;
	} else {
	    if (macroblock_modes & MACROBLOCK_INTRA)
		goto intra;
	    macroblock_modes |= UBITS (bit_buf, 2) * MOTION_TYPE_BASE;
	    DUMPBITS (bit_buf, bits, 2);
	    if (macroblock_modes & (MACROBLOCK_INTRA | MACROBLOCK_PATTERN)) {
	    intra:
		macroblock_modes |= UBITS (bit_buf, 1) * DCT_TYPE_INTERLACED;
		DUMPBITS (bit_buf, bits, 1);
	    }
	    return macroblock_modes;
	}

    case D_TYPE:

	macroblock_modes = UBITS (bit_buf, 1);
	if( macroblock_modes == 0 )
		return 0; // error
	DUMPBITS (bit_buf, bits, 1);
	return MACROBLOCK_INTRA;

    default:
	return 0;
    }
#undef bit_buf
#undef bits
#undef bit_ptr
}

static int get_quantizer_scale (decoder_t * const decoder)
{
    int quantizer_scale_code;

    quantizer_scale_code = UBITS (decoder->bitstream_buf, 5);
    DUMPBITS (decoder->bitstream_buf, decoder->bitstream_bits, 5);

    if (decoder->q_scale_type) return non_linear_quantizer_scale [quantizer_scale_code];
    else return quantizer_scale_code << 1;
}

static int get_coded_block_pattern (decoder_t * const decoder)
{
    const CBPtab * tab;

    NEEDBITS (decoder->bitstream_buf, decoder->bitstream_bits, decoder->bitstream_ptr);

    if (decoder->bitstream_buf >= 0x20000000) {
		tab = CBP_7 + (UBITS (decoder->bitstream_buf, 7) - 16);
		DUMPBITS (decoder->bitstream_buf, decoder->bitstream_bits, tab->len);
		return tab->cbp;
	}

	tab = CBP_9 + UBITS (decoder->bitstream_buf, 9);
	DUMPBITS (decoder->bitstream_buf, decoder->bitstream_bits, tab->len);
	return tab->cbp;
}

static int get_luma_dc_dct_diff (decoder_t * const decoder)
{
#define bit_buf (decoder->bitstream_buf)
#define bits (decoder->bitstream_bits)
#define bit_ptr (decoder->bitstream_ptr)

    const DCtab * tab;
    int size;
    int dc_diff;

    if (bit_buf < 0xf8000000) {
		tab = DC_lum_5 + UBITS (bit_buf, 5);
		size = tab->size;
		if (size) {
			bits += tab->len + size;
			bit_buf <<= tab->len;
			dc_diff =
			UBITS (bit_buf, size) - UBITS (SBITS (~bit_buf, 1), size);
			bit_buf <<= size;
			return dc_diff;
		} else {
			DUMPBITS (bit_buf, bits, 3);
			return 0;
		}
    } 
	
	tab = DC_long + (UBITS (bit_buf, 9) - 0x1e0);
	size = tab->size;
	DUMPBITS (bit_buf, bits, tab->len);
	NEEDBITS (bit_buf, bits, bit_ptr);
	dc_diff = UBITS (bit_buf, size) - UBITS (SBITS (~bit_buf, 1), size);
	DUMPBITS (bit_buf, bits, size);
	return dc_diff;
#undef bit_buf
#undef bits
#undef bit_ptr
}

static int get_chroma_dc_dct_diff (decoder_t * const decoder)
{
#define bit_buf (decoder->bitstream_buf)
#define bits (decoder->bitstream_bits)
#define bit_ptr (decoder->bitstream_ptr)

    const DCtab * tab;
    int size;
    int dc_diff;

    if (bit_buf < 0xf8000000) {
		tab = DC_chrom_5 + UBITS (bit_buf, 5);
		size = tab->size;
		if (size) {
			bits += tab->len + size;
			bit_buf <<= tab->len;
			dc_diff =
			UBITS (bit_buf, size) - UBITS (SBITS (~bit_buf, 1), size);
			bit_buf <<= size;
			return dc_diff;
		} else {
			DUMPBITS (bit_buf, bits, 2);
			return 0;
		}
    }
	
	tab = DC_long + (UBITS (bit_buf, 10) - 0x3e0);
	size = tab->size;
	DUMPBITS (bit_buf, bits, tab->len + 1);
	NEEDBITS (bit_buf, bits, bit_ptr);
	dc_diff = UBITS (bit_buf, size) - UBITS (SBITS (~bit_buf, 1), size);
	DUMPBITS (bit_buf, bits, size);
	return dc_diff;
#undef bit_buf
#undef bits
#undef bit_ptr
}

#define SATURATE(val)					\
do {							\
    if (((u32)(val + 2048) > 4095))	\
	val = SBITS (val, 1) ^ 2047;			\
} while (0)

static void get_intra_block_B14 (decoder_t * const decoder)
{
    int i;
    int j;
    int val;
    const u8 * scan = decoder->scan;
    const u8 * quant_matrix = decoder->intra_quantizer_matrix;
    int quantizer_scale = decoder->quantizer_scale;
    int mismatch;
    const DCTtab * tab;
    u32 bit_buf;
	u8 * bit_ptr;
    int bits;
    s16 * dest;

    dest = decoder->DCTblock;
    i = 0;
    mismatch = ~dest[0];

    bit_buf = decoder->bitstream_buf;
    bits = decoder->bitstream_bits;
	bit_ptr = decoder->bitstream_ptr;

    NEEDBITS (bit_buf, bits, bit_ptr);

    while (1) {
	if (bit_buf >= 0x28000000) {

	    tab = DCT_B14AC_5 + (UBITS (bit_buf, 5) - 5);

	    i += tab->run;
	    if (i >= 64) break;	/* end of block */

	normal_code:
	    j = scan[i];
	    bit_buf <<= tab->len;
	    bits += tab->len + 1;
		/* JayteeMaster: 10 points! Replaced quant_matrix[j] by quant_matrix[i] as should be */
	    val = (tab->level * quantizer_scale * quant_matrix[i]) >> 4;

	    /* if (bitstream_get (1)) val = -val; */
	    val = (val ^ SBITS (bit_buf, 1)) - SBITS (bit_buf, 1);

	    SATURATE (val);
	    dest[j] = val;
	    mismatch ^= val;

	    bit_buf <<= 1;
	    NEEDBITS (bit_buf, bits, bit_ptr);

	    continue;

	} else if (bit_buf >= 0x04000000) {

	    tab = DCT_B14_8 + (UBITS (bit_buf, 8) - 4);

	    i += tab->run;
	    if (i < 64) goto normal_code;

	    /* escape code */

	    i += UBITS (bit_buf << 6, 6) - 64;
	    if (i >= 64) break;	/* illegal, check needed to avoid buffer overflow */

	    j = scan[i];

	    DUMPBITS (bit_buf, bits, 12);
	    NEEDBITS (bit_buf, bits, bit_ptr);
		/* JayteeMaster: 10 points! Replaced quant_matrix[j] by quant_matrix[i] as should be */
	    val = (SBITS (bit_buf, 12) * quantizer_scale * quant_matrix[i]) / 16;

	    SATURATE (val);
	    dest[j] = val;
	    mismatch ^= val;

	    DUMPBITS (bit_buf, bits, 12);
	    NEEDBITS (bit_buf, bits, bit_ptr);

	    continue;

	} else if (bit_buf >= 0x02000000) {
	    tab = DCT_B14_10 + (UBITS (bit_buf, 10) - 8);
	    i += tab->run;
	    if (i < 64) goto normal_code;
	} else if (bit_buf >= 0x00800000) {
	    tab = DCT_13 + (UBITS (bit_buf, 13) - 16);
	    i += tab->run;
	    if (i < 64) goto normal_code;
	} else if (bit_buf >= 0x00200000) {
	    tab = DCT_15 + (UBITS (bit_buf, 15) - 16);
	    i += tab->run;
	    if (i < 64) goto normal_code;
	} else {
	    tab = DCT_16 + UBITS (bit_buf, 16);
	    bit_buf <<= 16;
	    GETWORD(&bit_buf,bits+16);
	    i += tab->run;
	    if (i < 64) goto normal_code;
	}
	break;	/* illegal, check needed to avoid buffer overflow */
    }
    dest[63] ^= mismatch & 1;
	if( (bit_buf>>30) != 0x2 )
		ipuRegs->ctrl.ECD = 1;
    DUMPBITS (bit_buf, bits, 2);	/* dump end of block code */
    decoder->bitstream_buf = bit_buf;
    decoder->bitstream_bits = bits;
}

static void get_intra_block_B15 (decoder_t * const decoder)
{
    int i;
    int j;
    int val;
    const u8 * scan = decoder->scan;
    const u8 * quant_matrix = decoder->intra_quantizer_matrix;
    int quantizer_scale = decoder->quantizer_scale;
    int mismatch;
    const DCTtab * tab;
    u32 bit_buf;
	u8 * bit_ptr;
    int bits;
    s16 * dest;

    dest = decoder->DCTblock;
    i = 0;
    mismatch = ~dest[0];

    bit_buf = decoder->bitstream_buf;
    bits = decoder->bitstream_bits;
	bit_ptr = decoder->bitstream_ptr;

    NEEDBITS (bit_buf, bits, bit_ptr);

    while (1) {
		if (bit_buf >= 0x04000000) {

			tab = DCT_B15_8 + (UBITS (bit_buf, 8) - 4);

			i += tab->run;
			if (i < 64) {
				normal_code:
					j = scan[i];
					bit_buf <<= tab->len;
					bits += tab->len + 1;
					/* JayteeMaster: 10 points! Replaced quant_matrix[j] by quant_matrix[i] as should be */
					val = (tab->level * quantizer_scale * quant_matrix[i]) >> 4;

					/* if (bitstream_get (1)) val = -val; */
					val = (val ^ SBITS (bit_buf, 1)) - SBITS (bit_buf, 1);

					SATURATE (val);
					dest[j] = val;
					mismatch ^= val;

					bit_buf <<= 1;
					NEEDBITS (bit_buf, bits, bit_ptr);

					continue;

			} else {
					/* end of block. I commented out this code because if we */
					/* dont exit here we will still exit at the later test :) */
					/* if (i >= 128) break;	*/	/* end of block */
					/* escape code */

					i += UBITS (bit_buf << 6, 6) - 64;
					if (i >= 64)  break;	/* illegal, check against buffer overflow */

					j = scan[i];

					DUMPBITS (bit_buf, bits, 12);
					NEEDBITS (bit_buf, bits, bit_ptr);
					/* JayteeMaster: 10 points! Replaced quant_matrix[j] by quant_matrix[i] as should be */
					val = (SBITS (bit_buf, 12) * quantizer_scale * quant_matrix[i]) / 16;

					SATURATE (val);
					dest[j] = val;
					mismatch ^= val;

					DUMPBITS (bit_buf, bits, 12);
					NEEDBITS (bit_buf, bits, bit_ptr);

					continue;
			}
		} else if (bit_buf >= 0x02000000) {
			tab = DCT_B15_10 + (UBITS (bit_buf, 10) - 8);
			i += tab->run;
			if (i < 64) goto normal_code;
		} else if (bit_buf >= 0x00800000) {
			tab = DCT_13 + (UBITS (bit_buf, 13) - 16);
			i += tab->run;
			if (i < 64) goto normal_code;
		} else if (bit_buf >= 0x00200000) {
			tab = DCT_15 + (UBITS (bit_buf, 15) - 16);
			i += tab->run;
			if (i < 64) goto normal_code;
		} else {
			tab = DCT_16 + UBITS (bit_buf, 16);
			bit_buf <<= 16;
			GETWORD(&bit_buf,bits+16);
			i += tab->run;
			if (i < 64) goto normal_code;
		}
		break;	/* illegal, check needed to avoid buffer overflow */
    }
    dest[63] ^= mismatch & 1;
	if( (bit_buf>>28) != 0x6 )
		ipuRegs->ctrl.ECD = 1;
    DUMPBITS (bit_buf, bits, 4);	/* dump end of block code */
    decoder->bitstream_buf = bit_buf;
    decoder->bitstream_bits = bits;
}

static int get_non_intra_block (decoder_t * const decoder)
{
#define bit_buf (decoder->bitstream_buf)
#define bits (decoder->bitstream_bits)
#define bit_ptr (decoder->bitstream_ptr)
    int i;
    int j;
    int val;
    const u8 * scan = decoder->scan;
    const u8 * quant_matrix = decoder->non_intra_quantizer_matrix;
    int quantizer_scale = decoder->quantizer_scale;
    int mismatch;
    const DCTtab * tab;
    s16 * dest;

    i = -1;
    mismatch = 1;
    dest = decoder->DCTblock;


    NEEDBITS (bit_buf, bits, bit_ptr);
    if (bit_buf >= 0x28000000) {
	tab = DCT_B14DC_5 + (UBITS (bit_buf, 5) - 5);
	goto entry_1;
    } else
	goto entry_2;

    while (1) {
	if (bit_buf >= 0x28000000) {

	    tab = DCT_B14AC_5 + (UBITS (bit_buf, 5) - 5);

	entry_1:
	    i += tab->run;
	    if (i >= 64)
		break;	/* end of block */

	normal_code:
	    j = scan[i];
	    bit_buf <<= tab->len;
	    bits += tab->len + 1;
		/* JayteeMaster: 10 points! Replaced quant_matrix[j] by quant_matrix[i] as should be */
	    val = ((2*tab->level+1) * quantizer_scale * quant_matrix[i]) >> 5;

	    /* if (bitstream_get (1)) val = -val; */
	    val = (val ^ SBITS (bit_buf, 1)) - SBITS (bit_buf, 1);

	    SATURATE (val);
	    dest[j] = val;
	    mismatch ^= val;

	    bit_buf <<= 1;
	    NEEDBITS (bit_buf, bits, bit_ptr);

	    continue;

	}

    entry_2:
	if (bit_buf >= 0x04000000) {

	    tab = DCT_B14_8 + (UBITS (bit_buf, 8) - 4);

	    i += tab->run;
	    if (i < 64)
		goto normal_code;

	    /* escape code */

	    i += UBITS (bit_buf << 6, 6) - 64;
	    if (i >= 64)
		break;	/* illegal, check needed to avoid buffer overflow */

	    j = scan[i];

	    DUMPBITS (bit_buf, bits, 12);
	    NEEDBITS (bit_buf, bits, bit_ptr);
	    val = 2 * (SBITS (bit_buf, 12) + SBITS (bit_buf, 1)) + 1;
		/* JayteeMaster: 10 points! Replaced quant_matrix[j] by quant_matrix[i] as should be */
	    val = (val * quantizer_scale * quant_matrix[i]) / 32;

	    SATURATE (val);
	    dest[j] = val;
	    mismatch ^= val;

	    DUMPBITS (bit_buf, bits, 12);
	    NEEDBITS (bit_buf, bits, bit_ptr);

	    continue;

	} else if (bit_buf >= 0x02000000) {
	    tab = DCT_B14_10 + (UBITS (bit_buf, 10) - 8);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else if (bit_buf >= 0x00800000) {
	    tab = DCT_13 + (UBITS (bit_buf, 13) - 16);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else if (bit_buf >= 0x00200000) {
	    tab = DCT_15 + (UBITS (bit_buf, 15) - 16);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else {
	    tab = DCT_16 + UBITS (bit_buf, 16);
	    bit_buf <<= 16;
	    GETWORD(&bit_buf,bits+16);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	}
	break;	/* illegal, check needed to avoid buffer overflow */
    }
    dest[63] ^= mismatch & 1;
	if( (bit_buf>>30) != 0x2 )
		ipuRegs->ctrl.ECD = 1;

    DUMPBITS (bit_buf, bits, 2);	/* dump end of block code */
    decoder->bitstream_buf = bit_buf;
    decoder->bitstream_bits = bits;
    return i;
#undef bit_buf
#undef bits
#undef bit_ptr
}

static void get_mpeg1_intra_block (decoder_t * const decoder)
{
    int i;
    int j;
    int val;
    const u8 * scan = decoder->scan;
    const u8 * quant_matrix = decoder->intra_quantizer_matrix;
    int quantizer_scale = decoder->quantizer_scale;
    const DCTtab * tab;
    u32 bit_buf;
    int bits;
	u8 * bit_ptr;
    s16 * dest;

    i = 0;
    dest = decoder->DCTblock;

    bit_buf = decoder->bitstream_buf;
    bits = decoder->bitstream_bits;
	bit_ptr = decoder->bitstream_ptr;
    
	NEEDBITS (bit_buf, bits, bit_ptr);

    while (1) {
	if (bit_buf >= 0x28000000) {

	    tab = DCT_B14AC_5 + (UBITS (bit_buf, 5) - 5);

	    i += tab->run;
	    if (i >= 64)
		break;	/* end of block */

	normal_code:
	    j = scan[i];
	    bit_buf <<= tab->len;
	    bits += tab->len + 1;
		/* JayteeMaster: 10 points! Replaced quant_matrix[j] by quant_matrix[i] as should be */
	    val = (tab->level * quantizer_scale * quant_matrix[i]) >> 4;

	    /* oddification */
	    val = (val - 1) | 1;

	    /* if (bitstream_get (1)) val = -val; */
	    val = (val ^ SBITS (bit_buf, 1)) - SBITS (bit_buf, 1);

	    SATURATE (val);
	    dest[j] = val;

	    bit_buf <<= 1;
	    NEEDBITS (bit_buf, bits, bit_ptr);

	    continue;

	} else if (bit_buf >= 0x04000000) {

	    tab = DCT_B14_8 + (UBITS (bit_buf, 8) - 4);

	    i += tab->run;
	    if (i < 64)
		goto normal_code;

	    /* escape code */

	    i += UBITS (bit_buf << 6, 6) - 64;
	    if (i >= 64)
		break;	/* illegal, check needed to avoid buffer overflow */

	    j = scan[i];

	    DUMPBITS (bit_buf, bits, 12);
	    NEEDBITS (bit_buf, bits, bit_ptr);
	    val = SBITS (bit_buf, 8);
	    if (! (val & 0x7f)) {
		DUMPBITS (bit_buf, bits, 8);
		val = UBITS (bit_buf, 8) + 2 * val;
	    }
		/* JayteeMaster: 10 points! Replaced quant_matrix[j] by quant_matrix[i] as should be */
	    val = (val * quantizer_scale * quant_matrix[i]) >> 4;

	    /* oddification */
	    val = (val + ~SBITS (val, 1)) | 1;

	    SATURATE (val);
	    dest[j] = val;

	    DUMPBITS (bit_buf, bits, 8);
	    NEEDBITS (bit_buf, bits, bit_ptr);

	    continue;

	} else if (bit_buf >= 0x02000000) {
	    tab = DCT_B14_10 + (UBITS (bit_buf, 10) - 8);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else if (bit_buf >= 0x00800000) {
	    tab = DCT_13 + (UBITS (bit_buf, 13) - 16);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else if (bit_buf >= 0x00200000) {
	    tab = DCT_15 + (UBITS (bit_buf, 15) - 16);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else {
	    tab = DCT_16 + UBITS (bit_buf, 16);
	    bit_buf <<= 16;
	    GETWORD(&bit_buf,bits+16);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	}
	break;	/* illegal, check needed to avoid buffer overflow */
    }
	if( (bit_buf>>30) != 0x2 )
		ipuRegs->ctrl.ECD = 1;

    DUMPBITS (bit_buf, bits, 2);	/* dump end of block code */
    decoder->bitstream_buf = bit_buf;
    decoder->bitstream_bits = bits;
}

static int get_mpeg1_non_intra_block (decoder_t * const decoder)
{
    int i;
    int j;
    int val;
    const u8 * scan = decoder->scan;
    const u8 * quant_matrix = decoder->non_intra_quantizer_matrix;
    int quantizer_scale = decoder->quantizer_scale;
    const DCTtab * tab;
    u32 bit_buf;
    int bits;
	u8 * bit_ptr;
    s16 * dest;

    i = -1;
    dest = decoder->DCTblock;

    bit_buf = decoder->bitstream_buf;
    bits = decoder->bitstream_bits;
	bit_ptr = decoder->bitstream_ptr;

    NEEDBITS (bit_buf, bits, bit_ptr);
    if (bit_buf >= 0x28000000) {
	tab = DCT_B14DC_5 + (UBITS (bit_buf, 5) - 5);
	goto entry_1;
    } else
	goto entry_2;

    while (1) {
	if (bit_buf >= 0x28000000) {

	    tab = DCT_B14AC_5 + (UBITS (bit_buf, 5) - 5);

	entry_1:
	    i += tab->run;
	    if (i >= 64)
		break;	/* end of block */

	normal_code:
	    j = scan[i];
	    bit_buf <<= tab->len;
	    bits += tab->len + 1;
		/* JayteeMaster: 10 points! Replaced quant_matrix[j] by quant_matrix[i] as should be */
	    val = ((2*tab->level+1) * quantizer_scale * quant_matrix[i]) >> 5;

	    /* oddification */
	    val = (val - 1) | 1;

	    /* if (bitstream_get (1)) val = -val; */
	    val = (val ^ SBITS (bit_buf, 1)) - SBITS (bit_buf, 1);

	    SATURATE (val);
	    dest[j] = val;

	    bit_buf <<= 1;
	    NEEDBITS (bit_buf, bits, bit_ptr);

	    continue;

	}

    entry_2:
	if (bit_buf >= 0x04000000) {

	    tab = DCT_B14_8 + (UBITS (bit_buf, 8) - 4);

	    i += tab->run;
	    if (i < 64)
		goto normal_code;

	    /* escape code */

	    i += UBITS (bit_buf << 6, 6) - 64;
	    if (i >= 64)
		break;	/* illegal, check needed to avoid buffer overflow */

	    j = scan[i];

	    DUMPBITS (bit_buf, bits, 12);
	    NEEDBITS (bit_buf, bits, bit_ptr);
	    val = SBITS (bit_buf, 8);
	    if (! (val & 0x7f)) {
		DUMPBITS (bit_buf, bits, 8);
		val = UBITS (bit_buf, 8) + 2 * val;
	    }
	    val = 2 * (val + SBITS (val, 1)) + 1;
		/* JayteeMaster: 10 points! Replaced quant_matrix[j] by quant_matrix[i] as should be */
	    val = (val * quantizer_scale * quant_matrix[i]) / 32;

	    /* oddification */
	    val = (val + ~SBITS (val, 1)) | 1;

	    SATURATE (val);
	    dest[j] = val;

	    DUMPBITS (bit_buf, bits, 8);
	    NEEDBITS (bit_buf, bits, bit_ptr);

	    continue;

	} else if (bit_buf >= 0x02000000) {
	    tab = DCT_B14_10 + (UBITS (bit_buf, 10) - 8);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else if (bit_buf >= 0x00800000) {
	    tab = DCT_13 + (UBITS (bit_buf, 13) - 16);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else if (bit_buf >= 0x00200000) {
	    tab = DCT_15 + (UBITS (bit_buf, 15) - 16);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else {
	    tab = DCT_16 + UBITS (bit_buf, 16);
	    bit_buf <<= 16;
		GETWORD(&bit_buf,bits+16);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	}
	break;	/* illegal, check needed to avoid buffer overflow */
    }
	if( (bit_buf>>30) != 0x2 )
		ipuRegs->ctrl.ECD = 1;

    DUMPBITS (bit_buf, bits, 2);	/* dump end of block code */
    decoder->bitstream_buf = bit_buf;
    decoder->bitstream_bits = bits;
    return i;
}

static void slice_intra_DCT (decoder_t * const decoder, const int cc,
				    u8 * const dest, const int stride)
{
    NEEDBITS (decoder->bitstream_buf, decoder->bitstream_bits, decoder->bitstream_ptr);
    /* Get the intra DC coefficient and inverse quantize it */
    if (cc == 0) decoder->dc_dct_pred[0] += get_luma_dc_dct_diff (decoder);
    else decoder->dc_dct_pred[cc] += get_chroma_dc_dct_diff (decoder);
    decoder->DCTblock[0] = decoder->dc_dct_pred[cc] << (3 - decoder->intra_dc_precision);

    if (decoder->mpeg1) get_mpeg1_intra_block (decoder);
	else if (decoder->intra_vlc_format){
		get_intra_block_B15 (decoder);
	}else{
		get_intra_block_B14 (decoder);
	}

    mpeg2_idct_copy (decoder->DCTblock, dest, stride);
}

/* JayteeMaster: changed dest to 16 bit signed */
static void slice_non_intra_DCT (decoder_t * const decoder,
					/*u8*/s16 * const dest, const int stride){
    int last;
	memset(decoder->DCTblock,0,sizeof(decoder->DCTblock));
    if (decoder->mpeg1) last = get_mpeg1_non_intra_block (decoder);
    else last = get_non_intra_block (decoder);

    mpeg2_idct_add (last, decoder->DCTblock, dest, stride);
}

extern int coded_block_pattern;
extern u8 FillInternalBuffer(u32 * pointer, u32 advance);
extern decoder_t g_decoder;
extern int g_nIPU0Data; // or 0x80000000 whenever transferring
extern u8* g_pIPU0Pointer;

void mpeg2sliceIDEC(void* pdone)
{
	u32 read;
	decoder_t * decoder = &g_decoder;

	*(int*)pdone = 0;
	bitstream_init(decoder);

	decoder->dc_dct_pred[0] = 
	decoder->dc_dct_pred[1] =	
	decoder->dc_dct_pred[2] = 128 << decoder->intra_dc_precision;

	decoder->mbc=0;

	if (UBITS (decoder->bitstream_buf, 2) == 0)
	{
		ipuRegs->ctrl.SCD = 0;
	}else{
		while (1) {
			int DCT_offset, DCT_stride;
			int mba_inc;
			const MBAtab * mba;
			
			NEEDBITS (decoder->bitstream_buf, decoder->bitstream_bits, decoder->bitstream_ptr);
			decoder->macroblock_modes = get_macroblock_modes (decoder);
			
			/* maybe integrate MACROBLOCK_QUANT test into get_macroblock_modes ? */
			if (decoder->macroblock_modes & MACROBLOCK_QUANT)//only IDEC
				decoder->quantizer_scale = get_quantizer_scale (decoder);

			if (decoder->macroblock_modes & DCT_TYPE_INTERLACED) {
				DCT_offset = decoder->stride;
				DCT_stride = decoder->stride * 2;
			} else {
				DCT_offset = decoder->stride * 8;
				DCT_stride = decoder->stride;
			}

			if (decoder->macroblock_modes & MACROBLOCK_INTRA) {
				decoder->coded_block_pattern = 0x3F;//all 6 blocks

				slice_intra_DCT (decoder, 0, (u8*)decoder->mb8->Y, DCT_stride);
				slice_intra_DCT (decoder, 0, (u8*)decoder->mb8->Y + 8, DCT_stride);
				slice_intra_DCT (decoder, 0, (u8*)decoder->mb8->Y + DCT_offset, DCT_stride);
				slice_intra_DCT (decoder, 0, (u8*)decoder->mb8->Y + DCT_offset + 8, DCT_stride);
				slice_intra_DCT (decoder, 1, (u8*)decoder->mb8->Cb, decoder->stride>>1);
				slice_intra_DCT (decoder, 2, (u8*)decoder->mb8->Cr, decoder->stride>>1);
			}

			// Send The MacroBlock via DmaIpuFrom
			if (decoder->ofm==0){
				ipu_csc(decoder->mb8, decoder->rgb32, decoder->sgn);

				g_nIPU0Data = 64;
				g_pIPU0Pointer = (u8*)decoder->rgb32;
				while(g_nIPU0Data > 0) {
					read = IPU0dma(g_pIPU0Pointer,g_nIPU0Data);
					if( read == 0 )
						co_resume();
					else {
						g_pIPU0Pointer += read*16;
						g_nIPU0Data -= read;
					}
				}
			}
			else{
				ipu_dither(decoder->mb8, decoder->rgb16, decoder->dte);

				g_nIPU0Data = 32;
				g_pIPU0Pointer = (u8*)decoder->rgb16;
				while(g_nIPU0Data > 0) {
					read = IPU0dma(g_pIPU0Pointer,g_nIPU0Data);
					if( read == 0 )
						co_resume();
					else {
						g_pIPU0Pointer += read*16;
						g_nIPU0Data -= read;
					}
				}
 			}
			decoder->mbc++;
			
			NEEDBITS (decoder->bitstream_buf, decoder->bitstream_bits, decoder->bitstream_ptr);

			mba_inc = 0;
			while (1) {
				if (decoder->bitstream_buf >= 0x10000000) {
					mba = MBA_5 + (UBITS (decoder->bitstream_buf, 5) - 2);
					break;
				} else if (decoder->bitstream_buf >= 0x03000000) {
					mba = MBA_11 + (UBITS (decoder->bitstream_buf, 11) - 24);
					break;
				} else switch (UBITS (decoder->bitstream_buf, 11)) {
						case 8:		/* macroblock_escape */
							mba_inc += 33;
						/* pass through */
						case 15:	/* macroblock_stuffing (MPEG1 only) */
							DUMPBITS (decoder->bitstream_buf, decoder->bitstream_bits, 11);
							NEEDBITS (decoder->bitstream_buf, decoder->bitstream_bits, decoder->bitstream_ptr);
							continue;
						default:	/* end of slice/frame, or error? */
						{
							int i;
							ipuRegs->ctrl.SCD = 1;
							ipuRegs->ctrl.ECD=0;
							coded_block_pattern=decoder->coded_block_pattern;

							for (i=0; i<2; i++) {
								u8 byte;
								while(!getBits8(&byte, 0))
									co_resume();
								if (byte == 0) break;
								g_BP.BP+= 8;
							}
							g_BP.BP-=32;//bitstream_init takes 32 bits

							while(!FillInternalBuffer(&g_BP.BP,1))
								co_resume();
							while(!getBits32((u8*)&ipuRegs->top, 0))
								co_resume();
							ipuRegs->top = BigEndian(ipuRegs->top);
							*(int*)pdone = 1;
							co_exit();
						}
				}
			}
			DUMPBITS (decoder->bitstream_buf, decoder->bitstream_bits, mba->len);
			mba_inc += mba->mba;
		
			if (mba_inc) {
				decoder->dc_dct_pred[0] = decoder->dc_dct_pred[1] =
				decoder->dc_dct_pred[2] = 128 << decoder->intra_dc_precision;
				do {
					decoder->mbc++;
				} while (--mba_inc);
			}
		}
	}

	ipuRegs->ctrl.ECD=!ipuRegs->ctrl.SCD;

	coded_block_pattern=decoder->coded_block_pattern;

	g_BP.BP-=32;//bitstream_init takes 32 bits

	while(!FillInternalBuffer(&g_BP.BP,1))
		co_resume();
	while(!getBits32((u8*)&ipuRegs->top, 0))
		co_resume();
	ipuRegs->top = BigEndian(ipuRegs->top);
	*(int*)pdone = 1;
	co_exit();
}

void mpeg2_slice(void* pdone)
{
	int DCT_offset, DCT_stride;
	u8 bit8=0;
	u32 fp = g_BP.FP;
	u32 bp;
	decoder_t * decoder = &g_decoder;
	u32 size = 0;

	*(int*)pdone = 0;
	ipuRegs->ctrl.ECD = 0;

	bitstream_init (decoder);
	
	if (decoder->dcr)
		decoder->dc_dct_pred[0] = decoder->dc_dct_pred[1] =
		decoder->dc_dct_pred[2] = 128 << decoder->intra_dc_precision;

	NEEDBITS (decoder->bitstream_buf, decoder->bitstream_bits, decoder->bitstream_ptr);
	if (decoder->macroblock_modes & DCT_TYPE_INTERLACED) {
		DCT_offset = decoder->stride;
		DCT_stride = decoder->stride * 2;
	} else {
		DCT_offset = decoder->stride * 8;
		DCT_stride = decoder->stride;
	}
	if (decoder->macroblock_modes & MACROBLOCK_INTRA) {
		decoder->coded_block_pattern = 0x3F;//all 6 blocks
		slice_intra_DCT (decoder, 0, (u8*)decoder->mb8->Y, DCT_stride);
		slice_intra_DCT (decoder, 0, (u8*)decoder->mb8->Y + 8, DCT_stride);
		slice_intra_DCT (decoder, 0, (u8*)decoder->mb8->Y + DCT_offset, DCT_stride);
		slice_intra_DCT (decoder, 0, (u8*)decoder->mb8->Y + DCT_offset + 8, DCT_stride);
		slice_intra_DCT (decoder, 1, (u8*)decoder->mb8->Cb, decoder->stride>>1);
		slice_intra_DCT (decoder, 2, (u8*)decoder->mb8->Cr, decoder->stride>>1);
		ipu_copy(decoder->mb8,decoder->mb16);
	} else {
		if (decoder->macroblock_modes & MACROBLOCK_PATTERN) {
			decoder->coded_block_pattern = get_coded_block_pattern (decoder);
			/* JayteeMaster: changed from mb8 to mb16 and from u8 to s16 */
	        if (decoder->coded_block_pattern & 0x20) slice_non_intra_DCT (decoder, (s16*)decoder->mb16->Y, DCT_stride);
			if (decoder->coded_block_pattern & 0x10) slice_non_intra_DCT (decoder, (s16*)decoder->mb16->Y + 8, DCT_stride);
			if (decoder->coded_block_pattern & 0x08) slice_non_intra_DCT (decoder, (s16*)decoder->mb16->Y + DCT_offset,	 DCT_stride);
			if (decoder->coded_block_pattern & 0x04) slice_non_intra_DCT (decoder, (s16*)decoder->mb16->Y + DCT_offset + 8, DCT_stride);
			if (decoder->coded_block_pattern & 0x2)  slice_non_intra_DCT (decoder, (s16*)decoder->mb16->Cb, decoder->stride>>1);
			if (decoder->coded_block_pattern & 0x1)  slice_non_intra_DCT (decoder, (s16*)decoder->mb16->Cr, decoder->stride>>1);
	
		}
	}

	//Send The MacroBlock via DmaIpuFrom
    size = 0;	// Reset

	ipuRegs->ctrl.SCD=0;
	coded_block_pattern=decoder->coded_block_pattern;

	//FillInternalBuffer(&g_BP.BP, 1);

	bp = g_BP.BP;
	g_BP.BP+=decoder->bitstream_bits-16;

	// BP goes from 0 to 128, so negative values mean to read old buffer
	// so we minus from 128 to get the correct BP
	if((int)g_BP.BP < 0) {
		g_BP.BP = 128 + (int)g_BP.BP;

		// After BP is positioned correctly, we need to reload the old buffer
		// so that reading may continue properly
		ReorderBitstream();
	}

	g_nIPU0Data = 48;
	g_pIPU0Pointer = (u8*)decoder->mb16;
	while(g_nIPU0Data > 0) {
		size = IPU0dma(g_pIPU0Pointer,g_nIPU0Data);
		if( size == 0 )
			co_resume();
		else {
			g_pIPU0Pointer += size*16;
			g_nIPU0Data -= size;
		}
	}

	IPU_LOG("BDEC %x, %d\n",g_BP.BP,g_BP.FP);

	while( !getBits8((u8*)&bit8, 0) )
		co_resume();
	if (bit8==0) ipuRegs->ctrl.SCD = 1;
	
	while(!getBits32((u8*)&ipuRegs->top, 0))
		co_resume();
	ipuRegs->top = BigEndian(ipuRegs->top);

	*(int*)pdone = 1;
	co_exit();
}

int get_motion_delta (decoder_t * const decoder,
				    const int f_code)
{
#define bit_buf (decoder->bitstream_buf)
#define bits (decoder->bitstream_bits)
#define bit_ptr (decoder->bitstream_ptr)

    int delta;
    int sign;
    const MVtab * tab;

    if ( (bit_buf & 0x80000000) ) {
		DUMPBITS (bit_buf, bits, 1);
		return 0x00010000;
    } else if ( (bit_buf & 0xf0000000) || ((bit_buf & 0xfc000000)==0x0c000000) ) {

		tab = MV_4 + UBITS (bit_buf, 4);
		delta = (tab->delta << f_code) + 1;
		bits += tab->len + f_code + 1;
		bit_buf <<= tab->len;

		sign = SBITS (bit_buf, 1);
		bit_buf <<= 1;

		if (f_code)
			delta += UBITS (bit_buf, f_code);
		bit_buf <<= f_code;

		return (delta ^ sign) - sign;

    } else {

	tab = MV_10 + UBITS (bit_buf, 10);
	delta = (tab->delta << f_code) + 1;
	bits += tab->len + 1;
	bit_buf <<= tab->len;

	sign = SBITS (bit_buf, 1);
	bit_buf <<= 1;

	if (f_code) {
	    NEEDBITS (bit_buf, bits, bit_ptr);
	    delta += UBITS (bit_buf, f_code);
	    DUMPBITS (bit_buf, bits, f_code);
	}

	return (delta ^ sign) - sign;

    }
#undef bit_buf
#undef bits
#undef bit_ptr
}

int get_dmv (decoder_t * const decoder)
{
#define bit_buf (decoder->bitstream_buf)
#define bits (decoder->bitstream_bits)
#define bit_ptr (decoder->bitstream_ptr)

    const DMVtab * tab;

    tab = DMV_2 + UBITS (bit_buf, 2);
    DUMPBITS (bit_buf, bits, tab->len);
    return tab->dmv;
#undef bit_buf
#undef bits
#undef bit_ptr
}

int get_macroblock_address_increment(decoder_t * const decoder){
	const MBAtab *mba;

	if (decoder->bitstream_buf >= 0x10000000)
		mba = MBA_5 + (UBITS (decoder->bitstream_buf, 5) - 2);
	else if (decoder->bitstream_buf >= 0x03000000)
		mba = MBA_11 + (UBITS (decoder->bitstream_buf, 11) - 24);
	else switch (UBITS (decoder->bitstream_buf, 11)) {
		case 8:		/* macroblock_escape */
			DUMPBITS (decoder->bitstream_buf, decoder->bitstream_bits, 11);
			return 0x23;
		case 15:	/* macroblock_stuffing (MPEG1 only) */
			if (decoder->mpeg1){
				DUMPBITS (decoder->bitstream_buf, decoder->bitstream_bits, 11);
				return 0x22;
			}
		default:
			return 0;//error
	}

	DUMPBITS (decoder->bitstream_buf, decoder->bitstream_bits, mba->len);
	return mba->mba + 1;
}
