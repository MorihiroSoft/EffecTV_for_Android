/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * EnergyTV.h :
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
 * Lightning Effect
 * Author: sazzzzz's (http://wonderfl.net/c/tcGz)
 */

#ifndef __ENERGYTV__
#define __ENERGYTV__

#include "BaseEffecTV.h"

class EnergyTV : public BaseEffecTV {
	typedef BaseEffecTV super;

protected:
	union Box {
		struct {
			int idx;
			int val;
		} sort;
		struct {
			int l; // x-1
			int t; // y-1
			int r; // x+1
			int b; // y+1
			float weight;
		} draw;
	};

	int show_info;
	int mode;
	int color;
	unsigned char marker_vu[2];
	int threshold;

	int vw2;
	int vh2;
	int va2;
	bool get_buffer;
	bool pause;
	RGB32* palettes;
	RGB32* palette;
	unsigned char* blur_buffer;
	unsigned char* blur_buffer_a;
	unsigned char* blur_buffer_b;
	YUV* img_buffer;
	int* blob_buffer;
	int* label_table;
	int marker_box_num;
	Box* marker_box;
	int marker_center[2];

	virtual void intialize(bool reset);
	virtual int readConfig();
	virtual int writeConfig();

public:
	EnergyTV(void);
	virtual ~EnergyTV(void);
	virtual const char* name(void);
	virtual const char* title(void);
	virtual const char** funcs(void);
	virtual int start(Utils* utils, int width, int height);
	virtual int stop(void);
	virtual int draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg);
	virtual const char* event(int key_code);
	virtual const char* touch(int action, int x, int y);

protected:
	void makePalette(void);
	void setPalette(int val);
	void blob(YUV* src_yuv);
	void screen(YUV* src_yuv, RGB32* dst_rgb);
	void lightning(void);
	float normalrandf();
	void line(const float x0, const float y0, const float x1, const float y1, const int thick);
	float calLineDist(const float x, const float y, const float x0, const float y0, const float x1, const float y1);
	void blur(void);
	void blend(RGB32* dst_rgb);
};

#endif // __ENERGYTV__
