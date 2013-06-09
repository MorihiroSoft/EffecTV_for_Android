/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * ShagadelicTV.h :
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/*
 * EffecTV - Realtime Digital Video Effector
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * ShagadelicTV - makes you shagadelic! Yeah baby yeah!
 * Copyright (C) 2001-2006 FUKUCHI Kentaro
 *
 * Inspired by Adrian Likin's script for the GIMP.
 */

#ifndef __SHAGADELICTV__
#define __SHAGADELICTV__

#include "BaseEffecTV.h"

class ShagadelicTV : public BaseEffecTV {
	typedef BaseEffecTV super;

protected:
	int show_info;
	unsigned char phase;
	int rx, ry;
	int bx, by;
	int rvx, rvy;
	int bvx, bvy;
	int mask, mask0;
	signed char* ripple;
	signed char* spiral;

	virtual void intialize(bool reset);
	virtual int readConfig();
	virtual int writeConfig();

public:
	ShagadelicTV(void);
	virtual ~ShagadelicTV(void);
	virtual const char* name(void);
	virtual const char* title(void);
	virtual const char** funcs(void);
	virtual int start(Utils* utils, int width, int height);
	virtual int stop(void);
	virtual int draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg);
	virtual int event(int key_code);
	virtual int touch(int action, int x, int y);
};

#endif // __SHAGADELICTV__
