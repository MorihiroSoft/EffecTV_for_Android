/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * LensTV.h :
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
 * lensTV - old skool Demo lens Effect
 * Code taken from "The Demo Effects Colletion" 0.0.4
 * http://www.paassen.tmfweb.nl/retrodemo.html
 *
 *
 * Ported to EffecTV BSB by Buddy Smith
 * Modified from BSB for EffecTV 0.3.x by Ed Tannenbaaum
 * ET added interactive control via mouse as follows....
 * Spacebar toggles interactive mode (off by default)
 * In interactive mode:
 *   Mouse with no buttons pressed moves magnifier
 *   Left button and y movement controls size of magnifier
 *   Right Button and y movement controls magnification.
 *
 * This works best in Fullscreen mode due to mouse trapping
 *
 * You can now read the fine print in the TV advertisements!
 */

#ifndef __LensTV__
#define __LensTV__

#include "BaseEffecTV.h"

class LensTV : public BaseEffecTV {
	typedef BaseEffecTV super;

protected:
	int show_info;
	int mode;
	int lens_x;
	int lens_y;
	int lens_xd;
	int lens_yd;
	int lens_size;
	int lens_crvr;
	int* lens;

	virtual void intialize(bool reset);
	virtual int readConfig();
	virtual int writeConfig();

public:
	LensTV(void);
	virtual ~LensTV(void);
	virtual const char* name(void);
	virtual const char* title(void);
	virtual const char** funcs(void);
	virtual int start(Utils* utils, int width, int height);
	virtual int stop(void);
	virtual int draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg);
	virtual const char* event(int key_code);
	virtual const char* touch(int action, int x, int y);

protected:
	int init(void);
	void clipmag(void);
	void apply_lens(int ox, int oy, RGB32* src,RGB32* dst);
};

#endif // __LensTV__
