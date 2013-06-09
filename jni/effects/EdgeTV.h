/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * EdgeTV.h :
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
 * EdgeTV - detects edge and display it in good old computer way.
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 * The idea of EdgeTV is taken from Adrian Likins's effector script for GIMP,
 * `Predator effect.'
 *
 * The algorithm of the original script pixelizes the image at first, then
 * it adopts the edge detection filter to the image. It also adopts MaxRGB
 * filter to the image. This is not used in EdgeTV.
 * This code is highly optimized and employs many fake algorithms. For example,
 * it devides a value with 16 instead of using sqrt() in line 132-134. It is
 * too hard for me to write detailed comment in this code in English.
 */

#ifndef __EDGETV__
#define __EDGETV__

#include "BaseEffecTV.h"

class EdgeTV : public BaseEffecTV {
	typedef BaseEffecTV super;

protected:
	int map_width;
	int map_height;
	int video_width_margin;
	RGB32* map;

	virtual void intialize(bool reset);
	virtual int readConfig() { return CONFIG_ERROR; };
	virtual int writeConfig() { return CONFIG_ERROR; };

public:
	EdgeTV(void);
	virtual ~EdgeTV(void);
	virtual const char* name(void);
	virtual const char* title(void);
	virtual const char** funcs(void);
	virtual int start(Utils* utils, int width, int height);
	virtual int stop(void);
	virtual int draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg);
	virtual int event(int key_code);
	virtual int touch(int action, int x, int y);
};

#endif // __EDGETV__
