/*
 * yuv2rgb.h
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

#ifndef YUV2RGB_H
#define YUV2RGB_H

#define CONVERT_FRAME 0
#define CONVERT_TOP_FIELD 1
#define CONVERT_BOTTOM_FIELD 2
#define CONVERT_BOTH_FIELDS 3

typedef struct convert_init_s {
    void * id;
    int id_size;
    int buf_size[3];
    void (* start) (void * id, u8 * dest, int flags);
    void (* copy) (void * id, u8 * Y, u8 * Cr, u8 * Cb, unsigned int v_offset);
} convert_init_t;

typedef void convert_t (int width, int height, u32 accel, void * arg,
			convert_init_t * result);

convert_t convert_rgb32;
convert_t convert_rgb24;
convert_t convert_rgb16;
convert_t convert_rgb15;
convert_t convert_bgr32;
convert_t convert_bgr24;
convert_t convert_bgr16;
convert_t convert_bgr15;

#define CONVERT_RGB 0
#define CONVERT_BGR 1
convert_t * convert_rgb (int order, int bpp);

#endif /* YUV2RGB_H */
