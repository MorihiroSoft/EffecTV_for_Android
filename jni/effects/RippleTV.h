/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * RippleTV.h :
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
 * RippleTV - Water ripple effect.
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 */

#ifndef __RIPPLETV__
#define __RIPPLETV__

#include "BaseEffecTV.h"

class RippleTV : public BaseEffecTV {
	typedef BaseEffecTV super;

protected:
	int show_info;
	int mode;
	int threshold;
	int bgIsSet;
	int map_w;
	int map_h;
	int* map;
	int* map1;
	int* map2;
	int* map3;
	signed char* vtable;
	int* sqrtable;

	virtual void intialize(bool reset);
	virtual int readConfig();
	virtual int writeConfig();

public:
	RippleTV(void);
	virtual ~RippleTV(void);
	virtual const char* name(void);
	virtual const char* title(void);
	virtual const char** funcs(void);
	virtual int start(Utils* utils, int width, int height);
	virtual int stop(void);
	virtual int draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg);
	virtual int event(int key_code);
	virtual int touch(int action, int x, int y);

protected:
	void setTable(void);
	int setBackground(YUV* src);
	void motiondetect(YUV* src);
	void drop(int power);
	void raindrop(void);
};

#endif // __RIPPLETV__
