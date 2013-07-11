/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * MatrixTV.h :
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
 * matrixTV - A Matrix Like effect.
 * This plugin for EffectTV is under GNU General Public License
 * See the "COPYING" that should be shiped with this source code
 * Copyright (C) 2001-2003 Monniez Christophe
 * d-fence@swing.be
 *
 * 2003/12/24 Kentaro Fukuchi
 * - Completely rewrote but based on Monniez's idea.
 * - Uses edge detection, not only G value of each pixel.
 * - Added 4x4 font includes number, alphabet and Japanese Katakana characters.
 */

#ifndef __MATRIXTV__
#define __MATRIXTV__

#include "BaseEffecTV.h"

class MatrixTV : public BaseEffecTV {
	typedef BaseEffecTV super;

protected:
	struct Blip {
		int mode;
		int y;
		int timer;
		int speed;
	};

	int show_info;
	int mode;
	int pause;
	int mapW;
	int mapH;
	unsigned char* cmap;
	unsigned char* vmap;
	unsigned char* img;
	unsigned char* font;
	RGB32* palette;
	Blip* blips;

	virtual void intialize(bool reset);
	virtual int readConfig();
	virtual int writeConfig();

public:
	MatrixTV(void);
	virtual ~MatrixTV(void);
	virtual const char* name(void);
	virtual const char* title(void);
	virtual const char** funcs(void);
	virtual int start(Utils* utils, int width, int height);
	virtual int stop(void);
	virtual int draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg);
	virtual const char* event(int key_code);
	virtual const char* touch(int action, int x, int y);

protected:
	RGB32 green(unsigned int v);
	int setPalette(void);
	int setPattern(void);
	void drawChar(RGB32* dst, unsigned char c, unsigned char v);
	void createImg(RGB32* src);
	void updateCharMap(void);

	void darkenColumn(int);
	void blipNone(int x);
	void blipFall(int x);
	void blipStop(int x);
	void blipSlide(int x);
};

#endif // __MATRIXTV__
