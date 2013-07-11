/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * DotTV.cpp :
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
 * DotTV: convert gray scale image into a set of dots
 * Copyright (C) 2001-2002 FUKUCHI Kentaro
 *
 */

#include "DotTV.h"

#define  LOG_TAG "DotTV"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#if 0
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define  LOGI(...)
#endif

static const int CONFIG_VER = 1;
static const char* EFFECT_NAME  = "DotTV";
static const char* EFFECT_TITLE = "DotTV";
static const char* FUNC_LIST[]  = {
		NULL
};

#include "DotTV_half_heart.h"
#define DOTDEPTH 5
#define DOTMAX (1<<DOTDEPTH)
//#define SFACT 4
#define SFACT 1

//---------------------------------------------------------------------
// INITIALIZE
//---------------------------------------------------------------------
void DotTV::intialize(bool reset)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	super::intialize(reset);

	// Clear data
	screen_width  = 0;
	screen_height = 0;
	screen_scale  = 0;
	dots_width    = 0;
	dots_height   = 0;
	dot_size      = 0;
	dot_hsize     = 0;
	pattern       = NULL;
	heart_pattern = NULL;
	sampx         = NULL;
	sampy         = NULL;

	// Set default parameters (no clear)
	if (reset) {
		if (readConfig() != CONFIG_SUCCESS) {
			mode = 0;
		}
	} else {
		writeConfig();
	}
}

int DotTV::readConfig()
{
	LOGI("%s(L=%d): path=%s", __func__, __LINE__, mConfigPath);
	FILE* fp = fopen(mConfigPath, "rb");
	if (fp == NULL) {
		return CONFIG_E_FOPEN;
	}
	int ver;
	bool rcode =
			FREAD_1(fp, ver) &&
			FREAD_1(fp, mode);
	fclose(fp);
	return (rcode ?
			(ver==CONFIG_VER ? CONFIG_SUCCESS : CONFIG_W_VER) :
			CONFIG_E_FREAD);
}

int DotTV::writeConfig()
{
	LOGI("%s(L=%d): path=%s", __func__, __LINE__, mConfigPath);
	FILE* fp = fopen(mConfigPath, "wb");
	if (fp == NULL) {
		LOGE("%s(L=%d): fp=NULL", __func__, __LINE__);
		return CONFIG_E_FOPEN;
	}
	bool rcode =
			FWRITE_1(fp, CONFIG_VER) &&
			FWRITE_1(fp, mode);
	fclose(fp);
	return (rcode ? CONFIG_SUCCESS : CONFIG_E_FWRITE);
}

//---------------------------------------------------------------------
// APIs
//---------------------------------------------------------------------
// Constructor
DotTV::DotTV(void)
: screen_width(0)
, screen_height(0)
, screen_scale(0)
, mode(0)
, dots_width(0)
, dots_height(0)
, dot_size(0)
, dot_hsize(0)
, pattern(NULL)
, heart_pattern(NULL)
, sampx(NULL)
, sampy(NULL)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
}

// Destructor
DotTV::~DotTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	stop();
}

// Return "effect name"
const char* DotTV::name(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_NAME;
}

// Return "effect title"
const char* DotTV::title(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_TITLE;
}

// Return "function label list(NULL TERMINATED)"
const char** DotTV::funcs(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return FUNC_LIST;
}

// Initialize
int DotTV::start(Utils* utils, int width, int height)
{
	LOGI("%s(L=%d): w=%d, h=%d", __func__, __LINE__, width, height);
	if (super::start(utils, width, height) != 0) {
		return -1;
	}

	screen_width  = video_width;
	screen_height = video_height;
	screen_scale  = 2;

	//
	double scale;
	if (screen_scale > 0) {
		scale = screen_scale;
	} else {
		scale = (double)screen_width / video_width;
		if (scale > (double)screen_height / video_height) {
			scale = (double)screen_height / video_height;
		}
	}
	dot_size = 8 * scale;
	dot_size = dot_size & 0xfe;
	dot_hsize = dot_size / 2;
	dots_width = screen_width / dot_size;
	dots_height = screen_height / dot_size;

	// Allocate memory & setup
	pattern       = new RGB32[DOTMAX * dot_hsize * dot_hsize];
	heart_pattern = new RGB32[DOTMAX * dot_hsize * dot_size];
	sampx         = new int[video_width];
	sampy         = new int[video_height];
	if (pattern == NULL || heart_pattern == NULL || sampx == NULL || sampy == NULL) {
		return -1;
	}

	makePattern();
	makeHeartPattern();
	init_sampxy_table();

	return 0;
}

// Finalize
int DotTV::stop(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	// Free memory
	if (pattern != NULL) {
		delete[] pattern;
	}
	if (heart_pattern != NULL) {
		delete[] heart_pattern;
	}
	if (sampx != NULL) {
		delete[] sampx;
	}
	if (sampy != NULL) {
		delete[] sampy;
	}

	//
	return super::stop();
}

// Convert
int DotTV::draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	memset(dst_rgb, 0, video_area * PIXEL_SIZE);
	if (mode) {
		for (int y=0; y<dots_height; y++) {
			int sy = sampy[y];
			for (int x=0; x<dots_width; x++) {
				int sx = sampx[x];
				int Y = (src_yuv[sy*video_width+sx] & 0xFF);
				// Y = 16~235 -> 0~255
				Y = (Y<16 ? 0 : (Y>235 ? 255 : (((Y-16)*255/235)&0xFF)));
				drawHeart(x, y, Y, dst_rgb);
			}
		}
	} else {
		for (int y=0; y<dots_height; y++) {
			int sy = sampy[y];
			for (int x=0; x<dots_width; x++) {
				int sx = sampx[x];
				int Y = (src_yuv[sy*video_width+sx] & 0xFF);
				// Y = 16~235 -> 0~255
				Y = (Y<16 ? 0 : (Y>235 ? 255 : (((Y-16)*255/235)&0xFF)));
				drawDot(x, y, Y, dst_rgb);
			}
		}
	}
	return 0;
}

// Key functions
const char* DotTV::event(int key_code)
{
	LOGI("%s(L=%d): k=%d", __func__, __LINE__, key_code);
	return NULL;
}

// Touch action
const char* DotTV::touch(int action, int x, int y)
{
	LOGI("%s(L=%d): action=%d, x=%d, y=%d", __func__, __LINE__, action, x, y);
	switch(action) {
	case 0: // Down -> Space
		mode ^= 1;
		break;
	case 1: // Move
		break;
	case 2: // Up -> Space
		break;
	}
	return NULL;
}

//---------------------------------------------------------------------
// LOCAL METHODs
//---------------------------------------------------------------------
//
void DotTV::init_sampxy_table(void)
{
	int j;

	j = dot_hsize;
	for (int i=0; i<dots_width; i++) {
		sampx[i] = j * video_width / screen_width;
		j += dot_size;
	}

	j = dot_hsize;
	for (int i=0; i<dots_height; i++) {
		sampy[i] = j * video_height / screen_height;
		j += dot_size;
	}
}

//
void DotTV::makePattern(void)
{
	for (int i=0; i<DOTMAX; i++) {
		/* Generated pattern is a quadrant of a disk. */
		RGB32* pat = pattern + (i+1) * dot_hsize * dot_hsize - 1;
		double r = dot_hsize * (0.2 + 0.8 * i / DOTMAX);
		r *= r;
		for (int y=0; y<dot_hsize; y++) {
			for (int x=0; x<dot_hsize; x++) {
				int c = 0;
				for (int u=0; u<4; u++) {
					double p = (double)u / 4.0 + y;
					p *= p;
					for (int v=0; v<4; v++) {
						double q = (double)v / 4.0 + x;
						if (p + q * q < r) {
							c++;
						}
					}
				}
				c = (c>15 ? 15 : c);
				*pat-- = c<<20 | c<<12 | c<<4;
				//*pat-- = ((c<<(16+4)) | (c<<(8+4)) | (c<<(0+4)));
				/* The upper left part of a disk is needed, but generated pattern is a bottom
				 * right part. So I spin the pattern. */
			}
		}
	}
}

//
void DotTV::makeOneHeart(int val, unsigned char* bigheart)
{
	int f1x, f1y;
	int f2x, f2y;
	double s1x, s1y;
	double s2x, s2y;
	double d1x, d1y;
	double d2x, d2y;
	double sum, hsum;
	double w, h;

	RGB32* pat = heart_pattern + val * dot_size * dot_hsize;

	s2y = (double)(-dot_hsize) / dot_size
			* (31.9 + (double)(DOTMAX - val) / SFACT) + 31.9;
	f2y = (int)s2y;
	for (int y=0; y<dot_size; y++) {
		s1y = s2y;
		f1y = f2y;
		s2y = (double)(y + 1 - dot_hsize) / dot_size
				* (31.9 + (double)(DOTMAX - val) / SFACT) + 31.9;
		f2y = (int)s2y;
		d1y = 1.0 - (s1y - (double)f1y);
		d2y = s2y - (double)f2y;
		h   = s2y - s1y;

		s2x = (double)(-dot_hsize) / dot_size
				* (31.9 + (double)(DOTMAX - val) / SFACT) + 31.9;
		f2x = (int)s2x;
		for (int x=0; x<dot_hsize; x++) {
			s1x = s2x;
			f1x = f2x;
			s2x = (double)(x + 1 - dot_hsize) / dot_size
					* (31.9 + (double)(DOTMAX - val) / SFACT) + 31.9;
			f2x = (int)s2x;
			d1x = 1.0 - (s1x - (double)f1x);
			d2x = s2x - (double)f2x;
			w   = s2x - s1x;

			sum = 0.0;
			for (int yy=f1y; yy<=f2y; yy++) {
				hsum = d1x * bigheart[yy * 32 + f1x];
				for (int xx=f1x+1; xx<f2x; xx++) {
					hsum += bigheart[yy * 32 + xx];
				}
				hsum += d2x * bigheart[yy * 32 + f2x];

				if (yy == f1y) {
					sum += hsum * d1y;
				} else if (yy == f2y) {
					sum += hsum * d2y;
				} else {
					sum += hsum;
				}
			}
			RGB32 c = (RGB32)(sum / w / h);
			*pat++ = (c<0 ? 0 : (c>255 ? 255 : c)) << 16;
		}
	}
}

//
void DotTV::makeHeartPattern(void)
{
	unsigned char* bigheart = new unsigned char[64 * 32];
	memset(bigheart, 0, 64 * 32 * sizeof(unsigned char));

	for (int y=0; y<32; y++) {
		for (int x=0; x<16; x++) {
			bigheart[(y + 16) * 32 + x + 16] = DotTV_half_heart[y * 16 + x];
		}
	}

	for (int i=0; i<DOTMAX; i++) {
		makeOneHeart(i, bigheart);
	}

	delete[] bigheart;
}

//
void DotTV::drawDot(int xx, int yy, int Y, RGB32* dst)
{
	Y = (Y >> (8 - DOTDEPTH));

	RGB32* p = pattern + Y * dot_hsize * dot_hsize;
	RGB32* q = dst + yy * dot_size * screen_width + xx * dot_size;

	// TOP
	for (int y=0; y<dot_hsize; y++) {
		// LEFT
		for (int x=0; x<dot_hsize; x++) {
			*q++ = *p++;
		}
		// RIGHT
		p -= 2;
		for (int x=0; x<dot_hsize-1; x++) {
			*q++ = *p--;
		}
		p += dot_hsize + 1;
		q += screen_width - dot_size + 1;
	}
	// BOTTOM
	p -= dot_hsize * 2;
	for (int y=0; y<dot_hsize-1; y++) {
		// LEFT
		for (int x=0; x<dot_hsize; x++) {
			*q++ = *p++;
		}
		// RIGHT
		p -= 2;
		for (int x=0; x<dot_hsize-1; x++) {
			*q++ = *p--;
		}
		p += -dot_hsize + 1;
		q += screen_width - dot_size + 1;
	}
}

//
void DotTV::drawHeart(int xx, int yy, int Y, RGB32* dst)
{
	Y = (Y >> (8 - DOTDEPTH));

	RGB32* p = heart_pattern + Y * dot_size * dot_hsize;
	RGB32* q = dst + yy * dot_size * screen_width + xx * dot_size;

	for(int y=0; y<dot_size; y++) {
		// LEFT
		for (int x=0; x<dot_hsize; x++) {
			*q++ = *p++;
		}
		// RIGHT
		p--;
		for (int x=0; x<dot_hsize; x++) {
			*q++ = *p--;
		}
		p += dot_hsize + 1;
		q += screen_width - dot_size;
	}
}
