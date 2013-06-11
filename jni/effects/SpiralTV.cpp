/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * SpiralTV.cpp :
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

#include "SpiralTV.h"

#define  LOG_TAG "SpiralTV"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#if 0
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define  LOGI(...)
#endif

static const int CONFIG_VER = 1;
static const char* EFFECT_NAME  = "SpiralTV";
static const char* EFFECT_TITLE = "Spiral\nTV";
static const char* FUNC_LIST[]  = {
		"Show\ninfo",
		"Type",
		"Depth\n+",
		"Depth\n-",
		"Init.",
		//"Animation",
		//"Speed +",
		//"Speed -",
		//"Interval +",
		//"Interval -",
		NULL
};
static const char* MODE_LIST[] = {
		"Concentric A",
		"Sawtooth Up",
		"Sawtooth Down",
		"Triangle",
		"Sinusoidal",
		"Concentric B",
		"Lens",
		"Flat"
};

#define RADIAN(degree) (degree * M_PI / 180.0)

/* Several values must be powers of 2. The *_POWER predefines keep them so. */

#define PLANE_POWER        (4) // 2 exp 4 = 16
#define WAVE_COUNT_POWER   (3) // 2 exp 3 = 8
#define WAVE_LENGTH_POWER  (9) // 2 exp 9 = 512

#define PLANES             (1 << PLANE_POWER) // 16
#define PLANE_MASK         (PLANES - 1)
#define PLANE_MAX          (PLANES - 1)

#define WAVE_COUNT         (1 << WAVE_COUNT_POWER) // 8
#define WAVE_MASK          (WAVE_COUNT - 1)
#define WAVE_MAX           (WAVE_COUNT - 1)

#define WAVE_LENGTH        (1 << WAVE_LENGTH_POWER) // 512
#define WAVE_LENGTH_MASK   (WAVE_LENGTH - 1)

#define WAVE_CONCENTRIC_A  0
#define WAVE_SAWTOOTH_UP   1
#define WAVE_SAWTOOTH_DOWN 2
#define WAVE_TRIANGLE      3
#define WAVE_SINUS         4
#define WAVE_CONCENTRIC_B  5
#define WAVE_LENS          6
#define WAVE_FLAT          7

/* The *_OFFSET predefines are just precalculations.  There shouldn't normally
 ** be any need to change them.
 */
#define WAVE_CONCENTRIC_A_OFFSET (WAVE_LENGTH * WAVE_CONCENTRIC_A)
#define WAVE_SAW_UP_OFFSET       (WAVE_LENGTH * WAVE_SAWTOOTH_UP)
#define WAVE_SAW_DOWN_OFFSET     (WAVE_LENGTH * WAVE_SAWTOOTH_DOWN)
#define WAVE_TRIANGLE_OFFSET     (WAVE_LENGTH * WAVE_TRIANGLE)
#define WAVE_CONCENTRIC_B_OFFSET (WAVE_LENGTH * WAVE_CONCENTRIC_B)
#define WAVE_LENS_OFFSET         (WAVE_LENGTH * WAVE_LENS)
#define WAVE_SINUS_OFFSET        (WAVE_LENGTH * WAVE_SINUS)
#define WAVE_FLAT_OFFSET         (WAVE_LENGTH * WAVE_FLAT)

#define FOCUS_INTERVAL_PRESET  (30)
#define FOCUS_DEGREE_PRESET    (1.0)
#define FOCUS_INCREMENT_PRESET (90.0)
#define DEPTH_SHIFT_PRESET     (0)

//---------------------------------------------------------------------
// INITIALIZE
//---------------------------------------------------------------------
void SpiralTV::intialize(bool reset)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	super::intialize(reset);

	// Clear data
	plane          = -1;
	g_focus_x      = 0;
	g_focus_y      = 0;
	g_focus_radius = 0;
	g_wave_table   = NULL;
	buffer         = NULL;
	planetable     = NULL;
	depthmap       = NULL;

	// Set default parameters (no clear)
	if (reset) {
		if (readConfig() != CONFIG_SUCCESS) {
			show_info         = 1;
			mode              = 0;
			g_animate_focus   = 0;
			g_focus_counter   = 0;
			g_focus_interval  = FOCUS_INTERVAL_PRESET;
			g_focus_degree    = FOCUS_DEGREE_PRESET;
			g_focus_increment = FOCUS_INCREMENT_PRESET;
			g_depth_shift     = DEPTH_SHIFT_PRESET;
		}
	} else {
		writeConfig();
	}
}

int SpiralTV::readConfig()
{
	LOGI("%s(L=%d): path=%s", __func__, __LINE__, mConfigPath);
	FILE* fp = fopen(mConfigPath, "rb");
	if (fp == NULL) {
		return CONFIG_E_FOPEN;
	}
	int ver;
	bool rcode =
			FREAD_1(fp, ver) &&
			FREAD_1(fp, show_info) &&
			FREAD_1(fp, mode) &&
			FREAD_1(fp, g_animate_focus) &&
			FREAD_1(fp, g_focus_counter) &&
			FREAD_1(fp, g_focus_interval) &&
			FREAD_1(fp, g_focus_degree) &&
			FREAD_1(fp, g_focus_increment) &&
			FREAD_1(fp, g_depth_shift);
	fclose(fp);
	return (rcode ?
			(ver==CONFIG_VER ? CONFIG_SUCCESS : CONFIG_W_VER) :
			CONFIG_E_FREAD);
}

int SpiralTV::writeConfig()
{
	LOGI("%s(L=%d): path=%s", __func__, __LINE__, mConfigPath);
	FILE* fp = fopen(mConfigPath, "wb");
	if (fp == NULL) {
		LOGE("%s(L=%d): fp=NULL", __func__, __LINE__);
		return CONFIG_E_FOPEN;
	}
	bool rcode =
			FWRITE_1(fp, CONFIG_VER) &&
			FWRITE_1(fp, show_info) &&
			FWRITE_1(fp, mode) &&
			FWRITE_1(fp, g_animate_focus) &&
			FWRITE_1(fp, g_focus_counter) &&
			FWRITE_1(fp, g_focus_interval) &&
			FWRITE_1(fp, g_focus_degree) &&
			FWRITE_1(fp, g_focus_increment) &&
			FWRITE_1(fp, g_depth_shift);
	fclose(fp);
	return (rcode ? CONFIG_SUCCESS : CONFIG_E_FWRITE);
}

//---------------------------------------------------------------------
// APIs
//---------------------------------------------------------------------
// Constructor
SpiralTV::SpiralTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
}

// Destructor
SpiralTV::~SpiralTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	stop();
}

// Return "effect name"
const char* SpiralTV::name(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_NAME;
}

// Return "effect title"
const char* SpiralTV::title(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_TITLE;
}

// Return "function label list(NULL TERMINATED)"
const char** SpiralTV::funcs(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return FUNC_LIST;
}

// Initialize
int SpiralTV::start(Utils* utils, int width, int height)
{
	LOGI("%s(L=%d): w=%d, h=%d", __func__, __LINE__, width, height);
	if (super::start(utils, width, height) != 0) {
		return -1;
	}

	g_focus_x = (video_width  / 2);
	g_focus_y = (video_height / 2);
	g_focus_radius = video_width / 2;

	// Allocate memory & setup
	buffer       = new unsigned int[video_area * PLANES];
	planetable   = new unsigned int*[PLANES];
	depthmap     = new int[video_area];
	g_wave_table = new WaveEl[WAVE_COUNT * WAVE_LENGTH];
	if (buffer == NULL || planetable == NULL || depthmap == NULL || g_wave_table == NULL) {
		return -1;
	}
	memset(buffer, 0, video_area * PLANES * PIXEL_SIZE);
	for (int i=0; i<PLANES; i++) {
		planetable[i] = &buffer[video_area * i];
	}

	if (spiralDefineWaves() != 0) {
		return -1;
	}
	spiralCreateMap();

	return 0;
}

// Finalize
int SpiralTV::stop(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	// Free memory
	if (buffer != NULL) {
		delete[] buffer;
	}
	if (planetable != NULL) {
		delete[] planetable;
	}
	if (depthmap != NULL) {
		delete[] depthmap;
	}
	if (g_wave_table != NULL) {
		delete[] g_wave_table;
	}

	//
	return super::stop();
}

// Convert
int SpiralTV::draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	RGB32* src_rgb = mUtils->yuv_YUVtoRGB(src_yuv);

	if (plane < 0) {
		for (int i=0; i<PLANES; i++) {
			memcpy(planetable[i], src_rgb, video_area * PIXEL_SIZE);
		}
		plane = PLANE_MAX;
	} else {
	memcpy(planetable[plane], src_rgb, video_area * PIXEL_SIZE);
	}

	if (g_animate_focus) {
		spiralMoveFocus();
	}

	for (int y=0,i=0; y<video_height; y++) {
		for (int x=0; x<video_width; x++,i++) {
			int cf = (plane + depthmap[i]) & PLANE_MASK;
			dst_rgb[i] = (planetable[cf])[i];
		}
	}

	plane--;
	plane &= PLANE_MASK;

	if (show_info) {
		sprintf(dst_msg, "Mode: %s (Depth: %d)",
				MODE_LIST[mode],
				g_depth_shift);
	}

	return 0;
}

// Key functions
int SpiralTV::event(int key_code)
{
	LOGI("%s(L=%d): k=%d", __func__, __LINE__, key_code);
	switch(key_code) {
	case 0: // Show info
		show_info = 1 - show_info;
		break;
	case 1: // Phase Table
		mode++;
		mode &= WAVE_MASK;
		spiralCreateMap();
		break;
	case 2: // Depth +
		if (g_depth_shift < WAVE_COUNT_POWER) {
			g_depth_shift++;
		}
		spiralCreateMap();
		break;
	case 3: // Depth -
		if (g_depth_shift > 0) {
			g_depth_shift--;
		}
		spiralCreateMap();
		break;
	case 4: // Init.
		g_focus_y = (video_height/2);
		g_focus_x = (video_width /2);
		g_focus_counter   = 0;
		g_focus_interval  = FOCUS_INTERVAL_PRESET;
		g_focus_degree    = FOCUS_DEGREE_PRESET;
		g_focus_increment = FOCUS_INCREMENT_PRESET;
		g_depth_shift     = DEPTH_SHIFT_PRESET;
		spiralCreateMap();
		break;
	case 5: // Animation
		g_animate_focus = 1 - g_animate_focus;
		break;
	case 6: // Speed +
		g_focus_increment *= 1.25;
		break;
	case 7: // Speed -
		g_focus_increment *= 0.80;
		break;
	case 8: // Interval +
		g_focus_interval++;
		if (g_focus_interval > 60) {
			g_focus_interval = 60;
		}
		break;
	case 9: // Interval -
		g_focus_interval--;
		if (g_focus_interval < 1) {
			g_focus_interval = 1;
		}
		break;
	}
	return 0;
}

// Touch action
int SpiralTV::touch(int action, int x, int y)
{
	LOGI("%s(L=%d): action=%d, x=%d, y=%d", __func__, __LINE__, action, x, y);
	switch(action) {
	case 0: // Down
		g_focus_x = x;
		g_focus_y = y;
		spiralCreateMap();
		break;
	case 1: // Move
		break;
	case 2: // Up
		break;
	}
	return 0;
}

//---------------------------------------------------------------------
// LOCAL METHODs
//---------------------------------------------------------------------
//
int SpiralTV::spiralDefineWaves(void)
{
	const int w = ((int)sqrt(video_height * video_height + video_width * video_width));

	g_wave_table = new WaveEl[WAVE_COUNT * WAVE_LENGTH];
	if (g_wave_table == NULL) {
		return -1;
	}

	// This code feels a little like a hack, but at least it contains
	// all like-minded hacks in one place.

	double sinus_val = M_PI / 2.0;
	for (int i=0; i<WAVE_LENGTH; i++) {
		// The 'flat' wave is very easy to compute :)
		g_wave_table[WAVE_FLAT_OFFSET + i] = 0;

		g_wave_table[WAVE_SAW_UP_OFFSET + i] = i & PLANE_MASK;
		g_wave_table[WAVE_SAW_DOWN_OFFSET + i] = PLANE_MAX - (i & PLANE_MASK);
		if (i & PLANES) {
			g_wave_table[WAVE_TRIANGLE_OFFSET + i] = (~i) & PLANE_MASK;
		} else {
			g_wave_table[WAVE_TRIANGLE_OFFSET + i] = i & PLANE_MASK;
		}

		int iw = i / (w/(PLANES*2));

		if (iw & PLANES) {
			g_wave_table[WAVE_CONCENTRIC_A_OFFSET + i] = (~iw) & PLANE_MASK;
		} else {
			g_wave_table[WAVE_CONCENTRIC_A_OFFSET + i] = iw & PLANE_MASK;
		}

		g_wave_table[WAVE_CONCENTRIC_B_OFFSET + i] = (i*PLANES)/w;
		g_wave_table[WAVE_LENS_OFFSET + i] = i >> 3;
		g_wave_table[WAVE_SINUS_OFFSET + i] = ((PLANES/2) + (int)((PLANES/2 - 1) * sin(sinus_val))) & PLANE_MASK;
		sinus_val += M_PI/PLANES;
	}
	return 0;
}

//
void SpiralTV::spiralCreateMap(void)
{
	/*
	 ** The following code generates the default depth map.
	 */
	int wave_offset = mode * WAVE_LENGTH;

	//x_ratio = 320.0f / video_width;
	//y_ratio = 240.0f / video_height;
	float x_ratio = 1.0f;
	float y_ratio = 1.0f;

	for (int y=0,i=0; y<video_height; y++) {
		int rel_y = (g_focus_y - y) * y_ratio;
		int yy = rel_y * rel_y;
		for (int x=0; x<video_width; x++,i++) {
			int rel_x = (g_focus_x - x) * x_ratio;
			int v = ((int)sqrt(yy + rel_x * rel_x)) & WAVE_LENGTH_MASK;
			depthmap[i] = g_wave_table[wave_offset + v] >> g_depth_shift;
		}
	}
}

//
void SpiralTV::spiralMoveFocus(void)
{
	g_focus_counter++;

	//  We'll only switch maps every X frames.
	if (g_focus_interval <= g_focus_counter) {
		g_focus_counter = 0;
		g_focus_x = (g_focus_radius * cos(RADIAN(g_focus_degree    ))) + (video_width /2);
		g_focus_y = (g_focus_radius * sin(RADIAN(g_focus_degree*2.0))) + (video_height/2);
		spiralCreateMap();
		g_focus_degree += g_focus_increment;
		while (g_focus_degree >= 360.0) g_focus_degree -= 360.0;
	}
}
