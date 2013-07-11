/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * BlueScreenTV.h :
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
 * BlueScreenTV - blue sceen effect: changes scene background
 * Copyright (C) 2005-2006 Nicolas Argyrou
 *
 * s: take 4-frames background snapshot
 * d: take delayed background snapshot after 3 seconds
 * space: get 4-frames blue screen background and preset tolerance to 30
 * b: get 24-frames blue screen background and preset tolerance to 20
 * c: decrease tolerance by 1 (0-255)
 * v: increase tolerance by 1 (0-255)
 *
 */

/*
 * Developper's notes:
 * The above filter computes color difference between the current frame
 * the pre-defined bluesceen background, and replaces differences less than
 * the threshold level by another background image.
 * Most webcams do not have a very clean image and the threshold is not enough
 * to avoid noise, so the bluescreen is recorded for 4 or 24 frames and the
 * minimum and maximum colors are saved (this method is better than averaging).
 * To avoid noise the replacement background is averaged over 4 frames. It
 * can be taken after a 3 seconds delay to be able to shoot the screen with
 * a webcam.
 * The color difference algorithm is quite different from the other algorithms
 * included in the effectv package. It uses a max(diff(rgv)) formulae with
 * anitaliasing like high quality photo editors do. Moreover it uses it twice
 * for the maximum and minimum blue screen.
 * To have even less noise a fast blur routine blurs the current frame
 * so that noisy lonely pixels are diluted. This blurring routine may be
 * overriden at compilation time by commenting out the "#define USE_BLUR" line.
 * The "#define PROFILING" line in the source code may be uncommented to try
 * other algorithm optimisations, although a lot has been done to allow a
 * maximum speed on 32bits+ processors.
 */

#ifndef __BLUESCREENTV__
#define __BLUESCREENTV__

#include "BaseEffecTV.h"

class BlueScreenTV : public BaseEffecTV {
	typedef BaseEffecTV super;

protected:
	int show_info;
	int threshold;
	int threshold2;
	int bgSetCnt;
	time_t bgSetCntTime;
	int bcSetCnt;
	RGB32* bgimage;
	RGB32* bgimageTmp;
	RGB32* bluescreen_min;
	RGB32* bluescreen_max;

	virtual void intialize(bool reset);
	virtual int readConfig();
	virtual int writeConfig();

public:
	BlueScreenTV(void);
	virtual ~BlueScreenTV(void);
	virtual const char* name(void);
	virtual const char* title(void);
	virtual const char** funcs(void);
	virtual int start(Utils* utils, int width, int height);
	virtual int stop(void);
	virtual int draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg);
	virtual const char* event(int key_code);
	virtual const char* touch(int action, int x, int y);

protected:
	int setBackground(RGB32* src);
	int setBlueScreen(RGB32* src);
};

#endif // __BLUESCREENTV__
