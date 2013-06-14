/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * EdgeTV.cpp :
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

#include "EdgeTV.h"

#define  LOG_TAG "EdgeTV"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#if 0
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define  LOGI(...)
#endif

//static const int CONFIG_VER = 1;
static const char* EFFECT_NAME  = "EdgeTV";
static const char* EFFECT_TITLE = "EdgeTV";
static const char* FUNC_LIST[]  = {
		NULL
};

//---------------------------------------------------------------------
// INITIALIZE
//---------------------------------------------------------------------
void EdgeTV::intialize(bool reset)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	super::intialize(reset);

	// Clear data
	map_width          = 0;
	map_height         = 0;
	video_width_margin = 0;
	map                = NULL;

	// Set default parameters (no clear)
	if (reset) {
		if (readConfig() != CONFIG_SUCCESS) {
		}
	} else {
		writeConfig();
	}
}

//---------------------------------------------------------------------
// APIs
//---------------------------------------------------------------------
// Constructor
EdgeTV::EdgeTV(void)
: map_width(0)
, map_height(0)
, video_width_margin(0)
, map(NULL)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
}

// Destructor
EdgeTV::~EdgeTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	stop();
}

// Return "effect name"
const char* EdgeTV::name(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_NAME;
}

// Return "effect title"
const char* EdgeTV::title(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_TITLE;
}

// Return "function label list(NULL TERMINATED)"
const char** EdgeTV::funcs(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return FUNC_LIST;
}

// Initialize
int EdgeTV::start(Utils* utils, int width, int height)
{
	LOGI("%s(L=%d): w=%d, h=%d", __func__, __LINE__, width, height);
	if (super::start(utils, width, height) != 0) {
		return -1;
	}

	map_width  = video_width  / 4;
	map_height = video_height / 4;
	video_width_margin = video_width - map_width * 4;

	// Allocate memory & setup
	map = new RGB32[map_width * map_height * 2];
	if (map == NULL) {
		return -1;
	}
	memset(map, 0, map_width * map_height * 2 * PIXEL_SIZE);

	return 0;
}

// Finalize
int EdgeTV::stop(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	// Free memory
	if (map != NULL) {
		delete[] map;
	}

	//
	return super::stop();
}

// Convert
int EdgeTV::draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	RGB32* src_rgb = mUtils->yuv_YUVtoRGB(src_yuv);

	memset(dst_rgb, 0, video_area * PIXEL_SIZE);

	src_rgb += video_width*4+4;
	dst_rgb += video_width*4+4;
	for (int y=1; y<map_height-1; y++) {
		for (int x=1; x<map_width-1; x++) {
			RGB32 p = *src_rgb;
			RGB32 q = *(src_rgb - 4);

			/* difference between the current pixel and right neighbor. */
			int r = ((int)(p & 0xff0000) - (int)(q & 0xff0000))>>16;
			int g = ((int)(p & 0x00ff00) - (int)(q & 0x00ff00))>>8;
			int b = ((int)(p & 0x0000ff) - (int)(q & 0x0000ff));
			r *= r; /* Multiply itself and divide it with 16, instead of */
			g *= g; /* using abs(). */
			b *= b;
			r = r>>5; /* To lack the lower bit for saturated addition,  */
			g = g>>5; /* devide the value with 32, instead of 16. It is */
			b = b>>4; /* same as `v2 &= 0xfefeff' */
			if (r>127) r = 127;
			if (g>127) g = 127;
			if (b>255) b = 255;
			RGB32 v2 = (r<<17)|(g<<9)|b;

			/* difference between the current pixel and upper neighbor. */
			q = *(src_rgb - video_width*4);
			r = ((int)(p & 0xff0000) - (int)(q & 0xff0000))>>16;
			g = ((int)(p & 0x00ff00) - (int)(q & 0x00ff00))>>8;
			b = ((int)(p & 0x0000ff) - (int)(q & 0x0000ff));
			r *= r;
			g *= g;
			b *= b;
			r = r>>5;
			g = g>>5;
			b = b>>4;
			if (r>127) r = 127;
			if (g>127) g = 127;
			if (b>255) b = 255;
			RGB32 v3 = (r<<17)|(g<<9)|b;

			RGB32 v0 = map[(y-1)*map_width*2+x*2];
			RGB32 v1 = map[y*map_width*2+(x-1)*2+1];
			map[y*map_width*2+x*2] = v2;
			map[y*map_width*2+x*2+1] = v3;
			r = v0 + v1;
			g = r & 0x01010100;
			dst_rgb[0] = r | (g - (g>>8));
			r = v0 + v3;
			g = r & 0x01010100;
			dst_rgb[1] = r | (g - (g>>8));
			dst_rgb[2] = v3;
			dst_rgb[3] = v3;
			r = v2 + v1;
			g = r & 0x01010100;
			dst_rgb[video_width] = r | (g - (g>>8));
			r = v2 + v3;
			g = r & 0x01010100;
			dst_rgb[video_width+1  ] = r | (g - (g>>8));
			dst_rgb[video_width+2  ] = v3;
			dst_rgb[video_width+3  ] = v3;
			dst_rgb[video_width*2  ] = v2;
			dst_rgb[video_width*2+1] = v2;
			dst_rgb[video_width*3  ] = v2;
			dst_rgb[video_width*3+1] = v2;

			src_rgb += 4;
			dst_rgb += 4;
		}
		src_rgb += video_width*3+8+video_width_margin;
		dst_rgb += video_width*3+8+video_width_margin;
	}

	return 0;
}

// Key functions
int EdgeTV::event(int key_code)
{
	LOGI("%s(L=%d): k=%d", __func__, __LINE__, key_code);
	return 0;
}

// Touch action
int EdgeTV::touch(int action, int x, int y)
{
	LOGI("%s(L=%d): action=%d, x=%d, y=%d", __func__, __LINE__, action, x, y);
	return 0;
}
