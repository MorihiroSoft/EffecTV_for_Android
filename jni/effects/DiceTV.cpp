/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * DiceTV.cpp :
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
 * dice.c: a 'dicing' effect
 *  copyright (c) 2001 Sam Mertens.  This code is subject to the provisions of
 *  the GNU Public License.
 *
 * I suppose this looks similar to PuzzleTV, but it's not. The screen is
 * divided into small squares, each of which is rotated either 0, 90, 180 or
 * 270 degrees.  The amount of rotation for each square is chosen at random.
 *
 * Controls:
 *      c   -   shrink the size of the squares, down to 1x1.
 *      v   -   enlarge the size of the squares, up to 32x32.
 *      space - generate a new random rotation map.
 *
 */

#include "DiceTV.h"

#define  LOG_TAG "DiceTV"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#if 0
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define  LOGI(...)
#endif

static const int CONFIG_VER = 1;
static const char* EFFECT_NAME  = "DiceTV";
static const char* EFFECT_TITLE = "DiceTV";
static const char* FUNC_LIST[]  = {
		"Show\ninfo",
		"Half\nsize",
		"Double\nsize",
		NULL
};

#define DEFAULT_CUBE_BITS 4 // = 16[pix.]
#define MAX_CUBE_BITS     5 // = 32[pix.]
#define MIN_CUBE_BITS     0 // =  1[pix.]

//---------------------------------------------------------------------
// INITIALIZE
//---------------------------------------------------------------------
void DiceTV::intialize(bool reset)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	super::intialize(reset);

	// Clear data
	dicemap = NULL;

	// Set default parameters (no clear)
	if (reset) {
		if (readConfig() != CONFIG_SUCCESS) {
			show_info = 1;
			g_cube_bits  = DEFAULT_CUBE_BITS;
			g_cube_size  = 0;
			g_map_width  = 0;
			g_map_height = 0;
		}
	} else {
		writeConfig();
	}
}

int DiceTV::readConfig()
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
			FREAD_1(fp, g_cube_bits) &&
			FREAD_1(fp, g_cube_size) &&
			FREAD_1(fp, g_map_height) &&
			FREAD_1(fp, g_map_width);
	fclose(fp);
	return (rcode ?
			(ver==CONFIG_VER ? CONFIG_SUCCESS : CONFIG_W_VER) :
			CONFIG_E_FREAD);
}

int DiceTV::writeConfig()
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
			FWRITE_1(fp, g_cube_bits) &&
			FWRITE_1(fp, g_cube_size) &&
			FWRITE_1(fp, g_map_height) &&
			FWRITE_1(fp, g_map_width);
	fclose(fp);
	return (rcode ? CONFIG_SUCCESS : CONFIG_E_FWRITE);
}

//---------------------------------------------------------------------
// APIs
//---------------------------------------------------------------------
// Constructor
DiceTV::DiceTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
}

// Destructor
DiceTV::~DiceTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	stop();
}

// Return "effect name"
const char* DiceTV::name(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_NAME;
}

// Return "effect title"
const char* DiceTV::title(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_TITLE;
}

// Return "function label list(NULL TERMINATED)"
const char** DiceTV::funcs(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return FUNC_LIST;
}

// Initialize
int DiceTV::start(Utils* utils, int width, int height)
{
	LOGI("%s(L=%d): w=%d, h=%d", __func__, __LINE__, width, height);
	if (super::start(utils, width, height) != 0) {
		return -1;
	}

	// Allocate memory & setup
	dicemap = new char[video_area];
	if (dicemap == NULL) {
		return -1;
	}

	diceCreateMap();

	return 0;
}

// Finalize
int DiceTV::stop(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	// Free memory
	if (dicemap != NULL) {
		delete[] dicemap;
	}

	//
	return super::stop();
}

// Convert
int DiceTV::draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	RGB32* src_rgb = mUtils->yuv_YUVtoRGB(src_yuv);

	if ((video_width&(g_cube_size-1))>0 || (g_map_height&(g_cube_size-1))>0) {
		memset(dst_rgb, 0, video_area * PIXEL_SIZE);
	}

	for (int map_y=0,map_i=0; map_y<g_map_height; map_y++) {
		for (int map_x=0; map_x<g_map_width; map_x++,map_i++) {
			int base = (map_y << g_cube_bits) * video_width + (map_x << g_cube_bits);
			switch (dicemap[map_i]) {
			case Up:
				for (int dy=0; dy<g_cube_size; dy++) {
					int i = base + dy * video_width;
					for (int dx=0; dx<g_cube_size; dx++,i++) {
						dst_rgb[i] = src_rgb[i];
					}
				}
				break;
			case Left:
				for (int dy=0; dy<g_cube_size; dy++) {
					int i = base + dy * video_width;
					for (int dx=0; dx<g_cube_size; dx++,i++) {
						int di = base + (dx * video_width) + (g_cube_size - dy - 1);
						dst_rgb[di] = src_rgb[i];
					}
				}
				break;
			case Down:
				for (int dy=0; dy<g_cube_size; dy++) {
					int di = base + dy * video_width;
					int i = base + (g_cube_size - dy - 1) * video_width + g_cube_size;
					for (int dx=0; dx<g_cube_size; dx++,di++) {
						i--;
						dst_rgb[di] = src_rgb[i];
					}
				}
				break;
			case Right:
				for (int dy=0; dy<g_cube_size; dy++) {
					int i = base + (dy * video_width);
					for (int dx=0; dx<g_cube_size; dx++,i++) {
						int di = base + dy + (g_cube_size - dx - 1) * video_width;
						dst_rgb[di] = src_rgb[i];
					}
				}
				break;
			default:
				// This should never occur
				break;
			}
		}
	}

	if (show_info) {
		sprintf(dst_msg, "Size: %1d",
				g_cube_size);
	}

	return 0;
}

// Key functions
int DiceTV::event(int key_code)
{
	LOGI("%s(L=%d): k=%d", __func__, __LINE__, key_code);
	switch(key_code) {
	case 0: // Show info
		show_info = 1 - show_info;
		break;
	case 1: // Half size
		if (MIN_CUBE_BITS < g_cube_bits) {
			g_cube_bits--;
			diceCreateMap();
		}
		break;
	case 2: // Double size
		if (MAX_CUBE_BITS > g_cube_bits) {
			g_cube_bits++;
			diceCreateMap();
		}
		break;
	}
	return 0;
}

// Touch action
int DiceTV::touch(int action, int x, int y)
{
	LOGI("%s(L=%d): action=%d, x=%d, y=%d", __func__, __LINE__, action, x, y);
	switch(action) {
	case 0: // Down -> Space
		diceCreateMap();
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
void DiceTV::diceCreateMap(void)
{
	g_map_height = video_height >> g_cube_bits;
	g_map_width  = video_width  >> g_cube_bits;
	g_cube_size  = 1 << g_cube_bits;

	for (int y=0,i=0; y<g_map_height; y++) {
		for (int x=0; x<g_map_width; x++,i++) {
			// dicemap[i] = ((i + y) & 0x3); /* Up, Down, Left or Right */
			dicemap[i] = (mUtils->fastrand() >> 24) & 0x03;
		}
	}
}
