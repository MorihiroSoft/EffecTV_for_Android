/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * SpiralTV.h :
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
 * spiral.c: a 'spiraling' effect (even though it isn't really a spiral)
 *  code originally derived from quark.c; additions and changes are
 *  copyright (c) 2001 Sam Mertens.  This code is subject to the provisions of
 *  the GNU Public License.
 *
 * ---------
 *  2001/04/20
 *      The code looks to be about done.  The extra junk I add to the
 *      title bar looks ugly - I just don't know how much to take off.
 *      The TAB key no longer toggles animation: 'A' does.
 *      A lot of these comments can probably be removed/reduced for the next
 *      stable release.
 * ---------
 *  2001/04/16
 *      I've made quick adjustments to my most recent 'experimental' code so
 *      it'll compile with the latest EffecTV-BSB code.  More proper
 *      intergration will commence shortly.
 *      Animation (more accurately, automatic movement of the wave loci along
 *      a fixed path) is implemented.  I'm dissapointed by the results, though.
 *      The following temporary additions have been made to the user interface
 *      to allow testing of the animation:
 *        The window title now displays the current waveform, plus
 *        several animation parameters.  Those parameters are formatted as
 *        "(%0.2fint,%df, %dd)"; the first number is the size of the interval
 *        between steps along the closed path.  The second number is the
 *        number of frames to wait before each step.  The third number is
 *        a quick and dirty way to control wave amplitude; wave table indices
 *        are bitshifted right by that value.
 *      The TAB key toggles animation on and off; animation is off by
 *      default.
 *      INSERT/DELETE increment and decrement the step interval value,
 *      respectively.
 *
 *      HOME/END increment and decrement the # of frames between each
 *      movement of the waves' centerpoint, respectively.
 *
 *      PAGE UP/PAGE DOWN increment and decrement the amount that wave table
 *      entries are bitshifted by, respectively.
 *
 *  Recent changes in the user interface:
 *  1. Hitting space will now cycle among 8 different wave shapes.
 *      The active waveshape's name is displayed in the titlebar.
 *  2. The mouse can be used to recenter the effect.  Left-clicking
 *      moves the center of the effect (which defaults to the center
 *      of the video image) to the mouse pointer's location.  Any other
 *      mouse button will toggle mouse visibility, so the user can see
 *      where they're clicking.
 *  3. The keys '1','2','3','4' and '0' also move the effect center.
 *      '1' through '4' move the center midway between the middle of the
 *      image and each of the four corners, respectively. '0' returns
 *      the center to its default position.
 *
 *	-Sam Mertens
 */

#ifndef __SPIRALTV__
#define __SPIRALTV__

#include "BaseEffecTV.h"

class SpiralTV : public BaseEffecTV {
	typedef BaseEffecTV super;

protected:
	typedef char WaveEl;

	int show_info;
	int plane;
	int mode;
	int g_focus_x;
	int g_focus_y;
	int g_focus_radius;
	int g_animate_focus;
	int g_focus_counter;
	int g_focus_interval;
	double g_focus_degree;
	double g_focus_increment;
	unsigned int g_depth_shift;
	WaveEl* g_wave_table;
	unsigned int* buffer;
	unsigned int** planetable;
	int* depthmap;

	virtual void intialize(bool reset);
	virtual int readConfig();
	virtual int writeConfig();

public:
	SpiralTV(void);
	virtual ~SpiralTV(void);
	virtual const char* name(void);
	virtual const char* title(void);
	virtual const char** funcs(void);
	virtual int start(Utils* utils, int width, int height);
	virtual int stop(void);
	virtual int draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg);
	virtual const char* event(int key_code);
	virtual const char* touch(int action, int x, int y);

protected:
	int spiralDefineWaves(void);
	void spiralCreateMap(void);
	void spiralMoveFocus(void);
};

#endif // __SPIRALTV__
