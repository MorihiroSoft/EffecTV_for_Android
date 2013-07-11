/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * SparkTV.h :
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
 * SparkTV - spark effect.
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 */

#ifndef __SPARKTV__
#define __SPARKTV__

#include "BaseEffecTV.h"

class SparkTV : public BaseEffecTV {
	typedef BaseEffecTV super;

protected:
	struct shortvec {
		int x1;
		int y1;
		int x2;
		int y2;
	};

	int show_info;
	int mode;
	int threshold;
	int bgIsSet;
	int sparks_head;
	shortvec* sparks;
	int* sparks_life;
	int* px;
	int* py;
	int* pp;

	virtual void intialize(bool reset);
	virtual int readConfig();
	virtual int writeConfig();

public:
	SparkTV(void);
	virtual ~SparkTV(void);
	virtual const char* name(void);
	virtual const char* title(void);
	virtual const char** funcs(void);
	virtual int start(Utils* utils, int width, int height);
	virtual int stop(void);
	virtual int draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg);
	virtual const char* event(int key_code);
	virtual const char* touch(int action, int x, int y);

protected:
	int setBackground(YUV* src);
	int shortvec_length2(shortvec sv);
	void draw_sparkline_dx(int x, int y, int dx, int dy, RGB32* dst, int width, int height);
	void draw_sparkline_dy(int x, int y, int dx, int dy, RGB32* dst, int width, int height);
	void draw_sparkline(int x1, int y1, int x2, int y2, RGB32* dst, int width, int height);
	void break_line(int a, int b, int width, int height);
	void draw_spark(shortvec sv, RGB32* dst, int width, int height);
	shortvec scanline_dx(int dir, int y1, int y2, unsigned char* diff);
	shortvec scanline_dy(int dir, int x1, int x2, unsigned char* diff);
	shortvec detectEdgePoints(unsigned char* diff);
};

#endif // __SPARKTV__
