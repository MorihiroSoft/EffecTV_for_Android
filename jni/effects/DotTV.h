/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * DotTV.h :
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
 * DotTV: convert gray scale image into a set of dots
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 */

#ifndef __DOTTV__
#define __DOTTV__

#include "BaseEffecTV.h"

class DotTV : public BaseEffecTV {
	typedef BaseEffecTV super;

protected:
	int screen_width;
	int screen_height;
	int screen_scale;

	int mode;
	int dots_width;
	int dots_height;
	int dot_size;
	int dot_hsize;
	RGB32* pattern;
	RGB32* heart_pattern;
	int* sampx;
	int* sampy;

	virtual void intialize(bool reset);
	virtual int readConfig();
	virtual int writeConfig();

public:
	DotTV(void);
	virtual ~DotTV(void);
	virtual const char* name(void);
	virtual const char* title(void);
	virtual const char** funcs(void);
	virtual int start(Utils* utils, int width, int height);
	virtual int stop(void);
	virtual int draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg);
	virtual int event(int key_code);
	virtual int touch(int action, int x, int y);

protected:
	void init_sampxy_table(void);
	void makePattern(void);
	void makeOneHeart(int val, unsigned char* bigheart);
	void makeHeartPattern(void);
	void drawDot(int xx, int yy, int Y, RGB32* dst);
	void drawHeart(int xx, int yy, int Y, RGB32* dst);
};

#endif // __DOTTV__
