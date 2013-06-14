/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * RevTV.cpp :
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
 * revTV based on Rutt-Etra Video Synthesizer 1974?
 *
 * (c)2002 Ed Tannenbaum
 *
 * This effect acts like a waveform monitor on each line.
 * It was originally done by deflecting the electron beam on a monitor using
 * additional electromagnets on the yoke of a b/w CRT. Here it is emulated digitally.

 * Experimental tapes were made with this system by Bill and Louise Etra and Woody and Steina Vasulka

 * The line spacing can be controlled using the 1 and 2 Keys.
 * The gain is controlled using the 3 and 4 keys.
 * The update rate is controlled using the 0 and - keys.
 */

#include "RevTV.h"

#define  LOG_TAG "RevTV"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#if 0
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define  LOGI(...)
#endif

static const int CONFIG_VER = 1;
static const char* EFFECT_NAME  = "RevTV";
static const char* EFFECT_TITLE = "RevTV";
static const char* FUNC_LIST[]  = {
		"Show\ninfo",
		"Space\n+",
		"Space\n-",
		"Factor\n+",
		"Factor\n-",
		NULL
};

//---------------------------------------------------------------------
// INITIALIZE
//---------------------------------------------------------------------
void RevTV::intialize(bool reset)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	super::intialize(reset);

	// Clear data

	// Set default parameters (no clear)
	if (reset) {
		if (readConfig() != CONFIG_SUCCESS) {
			show_info = 1;
			space  = 10;
			factor = 30;
		}
	} else {
		writeConfig();
	}
}

int RevTV::readConfig()
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
			FREAD_1(fp, space) &&
			FREAD_1(fp, factor);
	fclose(fp);
	return (rcode ?
			(ver==CONFIG_VER ? CONFIG_SUCCESS : CONFIG_W_VER) :
			CONFIG_E_FREAD);
}

int RevTV::writeConfig()
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
			FWRITE_1(fp, space) &&
			FWRITE_1(fp, factor);
	fclose(fp);
	return (rcode ? CONFIG_SUCCESS : CONFIG_E_FWRITE);
}

//---------------------------------------------------------------------
// APIs
//---------------------------------------------------------------------
// Constructor
RevTV::RevTV(void)
: show_info(0)
, space(0)
, factor(0)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
}

// Destructor
RevTV::~RevTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	stop();
}

// Return "effect name"
const char* RevTV::name(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_NAME;
}

// Return "effect title"
const char* RevTV::title(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_TITLE;
}

// Return "function label list(NULL TERMINATED)"
const char** RevTV::funcs(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return FUNC_LIST;
}

// Initialize
int RevTV::start(Utils* utils, int width, int height)
{
	LOGI("%s(L=%d): w=%d, h=%d", __func__, __LINE__, width, height);
	if (super::start(utils, width, height) != 0) {
		return -1;
	}

	return 0;
}

// Finalize
int RevTV::stop(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return super::stop();
}

// Convert
int RevTV::draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	memset(dst_rgb, 0, video_area * PIXEL_SIZE);
	vasulka(src_yuv, dst_rgb, 0, 0, 0, 0, video_width, video_height);

	if (show_info) {
		sprintf(dst_msg, "Space: %1d, Factor: %1d",
				space,
				factor);
	}

	return 0;
}

// Key functions
int RevTV::event(int key_code)
{
	LOGI("%s(L=%d): k=%d", __func__, __LINE__, key_code);
	switch(key_code) {
	case 0: // Show info
		show_info = 1 - show_info;
		break;
	case 1: // Space +
		space++;
		if (space > 64) space = 64;
		break;
	case 2: // Space -
		space--;
		if (space < 1) space = 1;
		break;
	case 3: // Factor +
		factor++;
		break;
	case 4: // Factor -
		factor--;
		if (factor < 1) factor = 1;
		break;
	}
	return 0;
}

// Touch action
int RevTV::touch(int action, int x, int y)
{
	LOGI("%s(L=%d): action=%d, x=%d, y=%d", __func__, __LINE__, action, x, y);
	return 0;
}

//---------------------------------------------------------------------
// LOCAL METHODs
//---------------------------------------------------------------------
//
void RevTV::vasulka(YUV* src, RGB32* dst, int srcx, int srcy, int dstx, int dsty, int w, int h)
{
	// draw the offset lines
	RGB32* q = &dst[dstx + dsty * video_width];
	for (int y=srcy; y<h+srcy; y+=space) {
		for (int x=srcx; x<=w+srcx; x++) {
			int v = y - src[x + y * video_width] / factor;
			int offset = x + v * video_width;
			if (offset >= 0 && offset < video_area) {
				q[offset] = 0x00FFFFFF;
			}
		}
	}
}
