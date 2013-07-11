/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * EdgeBlurTV.cpp :
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
 * EdgeBlurTV - Get difference, and make blur.
 * Copyright (C) 2005-2006 FUKUCHI Kentaro
 *
 */

#include "EdgeBlurTV.h"

#define  LOG_TAG "EdgeBlurTV"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#if 0
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define  LOGI(...)
#endif

static const int CONFIG_VER = 1;
static const char* EFFECT_NAME  = "EdgeBlurTV";
static const char* EFFECT_TITLE = "EdgeBlur\nTV";
static const char* FUNC_LIST[]  = {
		"Show\ninfo",
		"Red",
		"Green",
		"Blue",
		"White",
		"Threshold\n+",
		"Threshold\n-",
		NULL
};

#define MAX_BLUR 31
#define COLORS 4

#define MAX_THRESHOLD 255
#define MIN_THRESHOLD 5
#define DEF_THRESHOLD 100
#define DLT_THRESHOLD 5

//---------------------------------------------------------------------
// INITIALIZE
//---------------------------------------------------------------------
void EdgeBlurTV::intialize(bool reset)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	super::intialize(reset);

	// Clear data
	blurFrame = 0;
	blur[0]   = NULL;
	blur[1]   = NULL;
	palette   = NULL;
	palettes  = NULL;

	// Set default parameters (no clear)
	if (reset) {
		if (readConfig() != CONFIG_SUCCESS) {
			show_info = 1;
			threshold = DEF_THRESHOLD;
		}
	} else {
		writeConfig();
	}
}

int EdgeBlurTV::readConfig()
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
			FREAD_1(fp, threshold);
	fclose(fp);
	return (rcode ?
			(ver==CONFIG_VER ? CONFIG_SUCCESS : CONFIG_W_VER) :
			CONFIG_E_FREAD);
}

int EdgeBlurTV::writeConfig()
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
			FWRITE_1(fp, threshold);
	fclose(fp);
	return (rcode ? CONFIG_SUCCESS : CONFIG_E_FWRITE);
}

//---------------------------------------------------------------------
// APIs
//---------------------------------------------------------------------
// Constructor
EdgeBlurTV::EdgeBlurTV(void)
: show_info(0)
, threshold(0)
, blurFrame(0)
, blur()
, palette(NULL)
, palettes(NULL)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
}

// Destructor
EdgeBlurTV::~EdgeBlurTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	stop();
}

// Return "effect name"
const char* EdgeBlurTV::name(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_NAME;
}

// Return "effect title"
const char* EdgeBlurTV::title(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_TITLE;
}

// Return "function label list(NULL TERMINATED)"
const char** EdgeBlurTV::funcs(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return FUNC_LIST;
}

// Initialize
int EdgeBlurTV::start(Utils* utils, int width, int height)
{
	LOGI("%s(L=%d): w=%d, h=%d", __func__, __LINE__, width, height);
	if (super::start(utils, width, height) != 0) {
		return -1;
	}

	// Allocate memory & setup
	blur[0]  = new int[video_area];
	blur[1]  = new int[video_area];
	palettes = new RGB32[(MAX_BLUR + 1) * COLORS];
	if (blur[0] == NULL || blur[1] == NULL || palettes == NULL) {
		return -1;
	}
	memset(blur[0], 0, video_area * sizeof(int));
	memset(blur[1], 0, video_area * sizeof(int));

	makePalette();
	palette = palettes;

	mUtils->image_set_threshold_yuv_y(threshold);

	return 0;
}

// Finalize
int EdgeBlurTV::stop(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	// Free memory
	if (blur[0] != NULL) {
		delete[] blur[0];
	}
	if (blur[1] != NULL) {
		delete[] blur[1];
	}
	if (palettes != NULL) {
		delete[] palettes;
	}

	//
	return super::stop();
}

// Convert
int EdgeBlurTV::draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	RGB32* src_rgb = mUtils->yuv_YUVtoRGB(src_yuv);
	unsigned char* diff = mUtils->image_edge(src_rgb);

	memset(dst_rgb, 0, video_area * PIXEL_SIZE);

	int*           p0 = blur[blurFrame    ] + video_width + 1;
	int*           p1 = blur[blurFrame ^ 1] + video_width + 1;
	RGB32*         q  = dst_rgb + video_width + 1;
	unsigned char* r  = diff + video_width + 1;

	for (int y=video_height-2; y>0; y--) {
		for (int x=video_width-2; x>0; x--) {
			int v;
			if (*r > 64) {
				v = MAX_BLUR;
			} else {
				v = *(p0 - video_width) + *(p0 - 1) + *(p0 + 1) + *(p0 + video_width);
				if(v > 3) v -= 3;
				v = v >> 2;
			}
			*q  = palette[v];
			*p1 = v;

			p0++;
			p1++;
			q++;
			r++;
		}
		p0 += 2;
		p1 += 2;
		q  += 2;
		r  += 2;
	}

	blurFrame ^= 1;

	if (show_info) {
		sprintf(dst_msg, "Threshold: %1d",
				threshold);
	}

	return 0;
}

// Key functions
const char* EdgeBlurTV::event(int key_code)
{
	LOGI("%s(L=%d): k=%d", __func__, __LINE__, key_code);
	switch(key_code) {
	case 0: // Show info
		show_info = 1 - show_info;
		break;
	case 1: // Red
		palette = &palettes[(MAX_BLUR + 1)*2];
		break;
	case 2: // Green
		palette = &palettes[(MAX_BLUR + 1)];
		break;
	case 3: // Blue
		palette = &palettes[0];
		break;
	case 4: // White
		palette = &palettes[(MAX_BLUR + 1)*3];
		break;
	case 5: // Threshold +
		threshold += DLT_THRESHOLD;
		if (threshold > MAX_THRESHOLD) {
			threshold = MAX_THRESHOLD;
		}
		mUtils->image_set_threshold_yuv_y(threshold);
		break;
	case 6: // Threshold -
		threshold -= DLT_THRESHOLD;
		if (threshold < MIN_THRESHOLD) {
			threshold = MIN_THRESHOLD;
		}
		mUtils->image_set_threshold_yuv_y(threshold);
		break;
	}
	return NULL;
}

// Touch action
const char* EdgeBlurTV::touch(int action, int x, int y)
{
	LOGI("%s(L=%d): action=%d, x=%d, y=%d", __func__, __LINE__, action, x, y);
	return NULL;
}

//---------------------------------------------------------------------
// LOCAL METHODs
//---------------------------------------------------------------------
//
void EdgeBlurTV::makePalette(void)
{
	for (int i=0; i<=MAX_BLUR; i++) {
		int v = 255 * i / MAX_BLUR;
		palettes[i                     ] = v;
		palettes[(MAX_BLUR + 1)     + i] = v <<  8;
		palettes[(MAX_BLUR + 1) * 2 + i] = v << 16;
		palettes[(MAX_BLUR + 1) * 3 + i] = v * 0x10101;
	}
}
