/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * MatrixTV.cpp :
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

#include "MatrixTV.h"

#define  LOG_TAG "MatrixTV"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#if 0
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#else
#define  LOGI(...)
#endif

static const int CONFIG_VER = 1;
static const char* EFFECT_NAME  = "MatrixTV";
static const char* EFFECT_TITLE = "Matrix\nTV";
static const char* FUNC_LIST[]  = {
		"Show\ninfo",
		"Letters\nonly",
		"Blending\ninput",
		NULL
};
static const char* MODE_LIST[] = {
		"Letters only",
		"Blending input",
};

#include "MatrixTV_font.h"
#define CHARNUM 80
#define FONT_W 4
#define FONT_H 4
#define FONT_DEPTH 4

#define MODE_NONE 0
#define MODE_FALL 1
#define MODE_STOP 2
#define MODE_SLID 3

#define WHITE (0.45f)

//---------------------------------------------------------------------
// INITIALIZE
//---------------------------------------------------------------------
void MatrixTV::intialize(bool reset)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	super::intialize(reset);

	// Clear data
	pause   = 0;
	mapW    = 0;
	mapH    = 0;
	cmap    = NULL;
	vmap    = NULL;
	img     = NULL;
	font    = NULL;
	palette = NULL;
	blips   = NULL;

	// Set default parameters (no clear)
	if (reset) {
		if (readConfig() != CONFIG_SUCCESS) {
			show_info = 1;
			mode = 0;
		}
	} else {
		writeConfig();
	}
}

int MatrixTV::readConfig()
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
			FREAD_1(fp, mode);
	fclose(fp);
	return (rcode ?
			(ver==CONFIG_VER ? CONFIG_SUCCESS : CONFIG_W_VER) :
			CONFIG_E_FREAD);
}

int MatrixTV::writeConfig()
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
			FWRITE_1(fp, mode);
	fclose(fp);
	return (rcode ? CONFIG_SUCCESS : CONFIG_E_FWRITE);
}

//---------------------------------------------------------------------
// APIs
//---------------------------------------------------------------------
// Constructor
MatrixTV::MatrixTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
}

// Destructor
MatrixTV::~MatrixTV(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	stop();
}

// Return "effect name"
const char* MatrixTV::name(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_NAME;
}

// Return "effect title"
const char* MatrixTV::title(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return EFFECT_TITLE;
}

// Return "function label list(NULL TERMINATED)"
const char** MatrixTV::funcs(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	return FUNC_LIST;
}

// Initialize
int MatrixTV::start(Utils* utils, int width, int height)
{
	LOGI("%s(L=%d): w=%d, h=%d", __func__, __LINE__, width, height);
	if (super::start(utils, width, height) != 0) {
		return -1;
	}

	mapW = video_width / FONT_W;
	mapH = video_height / FONT_H;

	// Allocate memory & setup
	cmap  = new unsigned char[mapW * mapH];
	vmap  = new unsigned char [mapW * mapH];
	img   = new unsigned char [mapW * mapH];
	blips = new Blip[mapW];
	if (cmap == NULL || vmap == NULL || img == NULL || blips == NULL) {
		return -1;
	}
	memset(cmap, CHARNUM - 1, mapW * mapH * sizeof(unsigned char));
	memset(vmap, 0, mapW * mapH * sizeof(unsigned char));
	memset(blips, 0, mapW * sizeof(Blip));

	if (setPattern() != 0) {
		return -1;
	}

	if (setPalette() != 0) {
		return -1;
	}

	return 0;
}

// Finalize
int MatrixTV::stop(void)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	// Free memory
	if (cmap != NULL) {
		delete[] cmap;
	}
	if (vmap != NULL) {
		delete[] vmap;
	}
	if (img != NULL) {
		delete[] img;
	}
	if (font != NULL) {
		delete[] font;
	}
	if (palette != NULL) {
		delete[] palette;
	}
	if (blips != NULL) {
		delete[] blips;
	}

	//
	return super::stop();
}

// Convert
int MatrixTV::draw(YUV* src_yuv, RGB32* dst_rgb, char* dst_msg)
{
	LOGI("%s(L=%d)", __func__, __LINE__);
	RGB32* src_rgb = mUtils->yuv_YUVtoRGB(src_yuv);

	if (pause == 0) {
		updateCharMap();
		createImg(src_rgb);
	}
	unsigned char* c = cmap;
	unsigned char* v = vmap;
	unsigned char* r = img;

	RGB32* p = src_rgb;
	RGB32* q = dst_rgb;

	for (int y=mapH; y>0; y--) {
		RGB32* q2 = q;
		for (int x=mapW; x>0; x--) {
			drawChar(q2, *c, (unsigned int)(*r|*v));
			c++;
			v++;
			r++;
			q2 += FONT_W;
		}
		q += video_width * FONT_H;
	}

	if (mode == 1) {
		p = src_rgb;
		q = dst_rgb;
		for (int i=video_area; i>0; i--) {
			RGB32 a = *q;
			RGB32 b = (*p++ & 0xfefeff) >> 1;
			*q++ = a | b;
		}
	}

	if (show_info) {
		sprintf(dst_msg, "Mode: %s (Touch screen to clear)",
				MODE_LIST[mode]);
	}

	return 0;
}

// Key functions
int MatrixTV::event(int key_code)
{
	LOGI("%s(L=%d): k=%d", __func__, __LINE__, key_code);
	switch(key_code) {
	case 0: // Show info
		show_info = 1 - show_info;
		break;
	case 1: // Letters only
	case 2: // Blending input
		mode = key_code - 1;
		break;
	}
	return 0;
}

// Touch action
int MatrixTV::touch(int action, int x, int y)
{
	LOGI("%s(L=%d): action=%d, x=%d, y=%d", __func__, __LINE__, action, x, y);
	switch(action) {
	case 0: // Down -> Space
		memset(cmap, CHARNUM - 1, mapW * mapH * sizeof(unsigned char));
		memset(vmap, 0, mapW * mapH * sizeof(unsigned char));
		memset(blips, 0, mapW * sizeof(Blip));
		pause = 1;
		break;
	case 1: // Move
		break;
	case 2: // Up -> Space
		pause = 0;
		break;
	}
	return 0;
}

//---------------------------------------------------------------------
// LOCAL METHODs
//---------------------------------------------------------------------
//
RGB32 MatrixTV::green(unsigned int v)
{
	if(v < 256) {
		return ((int)(v*WHITE)<<16)|(v<<8)|(int)(v*WHITE);
	}

	unsigned int w = v - (int)(256*WHITE);
	if(w > 255) w = 255;
	return (w << 16) + 0xff00 + w;
}

//
int MatrixTV::setPalette(void)
{
	palette = new RGB32[256 * FONT_DEPTH];
	if (palette == NULL) {
		return -1;
	}

	for (int i=0; i<256; i++) {
		palette[i*FONT_DEPTH  ] = 0;
		palette[i*FONT_DEPTH+1] = green(0x44 * i / 170);
		palette[i*FONT_DEPTH+2] = green(0x99 * i / 170);
		palette[i*FONT_DEPTH+3] = green(0xff * i / 170);
	}

	return 0;
}

//
int MatrixTV::setPattern(void)
{
	font = new unsigned char[CHARNUM * FONT_W * FONT_H];
	if (font == NULL) {
		return -1;
	}

	/* FIXME: This code is highly depends on the structure of bundled */
	/*        matrixFont.xpm. */
	for (int l=0; l<32; l++) {
		const char* p  = MatrixTV_font[5 + l];
		int cy = l /4;
		int y  = l % 4;
		for (int c=0; c<40; c++) {
			int cx = c / 4;
			int x  = c % 4;
			unsigned char v;
			switch(*p) {
			case ' ':
				v = 0;
				break;
			case '.':
				v = 1;
				break;
			case 'o':
				v = 2;
				break;
			case 'O':
			default:
				v = 3;
				break;
			}
			font[(cy * 10 + cx) * FONT_W * FONT_H + y * FONT_W + x] = v;
			p++;
		}
	}

	return 0;
}

/* Create edge-enhanced image data from the input */
void MatrixTV::createImg(RGB32* src)
{
	unsigned char* q = img;
	for (int y=0; y<mapH; y++) {
		RGB32* p = src;
		for (int x=0; x<mapW; x++) {
			RGB32 pc = *p;                                // center
			RGB32 pr = *(p + FONT_W - 1);                 // right
			RGB32 pb = *(p + video_width * (FONT_H - 1)); // below

			int r = (int)(pc & 0xff0000) >> 15;
			int g = (int)(pc & 0x00ff00) >> 7;
			int b = (int)(pc & 0x0000ff) * 2;

			unsigned int val = (r + 2*g + b) >> 5; // val < 64

			r -= (int)(pr & 0xff0000)>>16;
			g -= (int)(pr & 0x00ff00)>>8;
			b -= (int)(pr & 0x0000ff);
			r -= (int)(pb & 0xff0000)>>16;
			g -= (int)(pb & 0x00ff00)>>8;
			b -= (int)(pb & 0x0000ff);

			val += (r * r + g * g + b * b)>>5;

			if (val > 160) val = 160; // want not to make blip from the edge.
			*q = (unsigned char)val;

			p += FONT_W;
			q++;
		}
		src += video_width * FONT_H;
	}
}

//
void MatrixTV::drawChar(RGB32* dst, unsigned char c, unsigned char v)
{
	if (v == 255) { // sticky characters
		v = 160;
	}

	RGB32*         p = &palette[(int)v * FONT_DEPTH];
	unsigned char* f = &font[(int)c * FONT_W * FONT_H];
	for (int y=0; y<FONT_H; y++) {
		for (int x=0; x<FONT_W; x++) {
			*dst++ = p[*f];
			f++;
		}
		dst += video_width - FONT_W;
	}
}

//
void MatrixTV::updateCharMap(void)
{
	for (int x=0; x<mapW; x++) {
		darkenColumn(x);
		switch(blips[x].mode) {
		default:
		case MODE_NONE:
			blipNone(x);
			break;
		case MODE_FALL:
			blipFall(x);
			break;
		case MODE_STOP:
			blipStop(x);
			break;
		case MODE_SLID:
			blipSlide(x);
			break;
		}
	}
}

//
void MatrixTV::darkenColumn(int x)
{
	unsigned char* p = vmap + x;
	for(int y=0; y<mapH; y++) {
		int v = *p;
		if (v < 255) {
			v *= 0.9;
			*p = v;
		}
		p += mapW;
	}
}

//
void MatrixTV::blipNone(int x)
{
	// This is a test code to reuse a randome number for multi purpose. :-P
	// Of course it isn't good code because fastrand() doesn't generate ideal
	// randome numbers.
	unsigned int r = mUtils->fastrand();

	if ((r & 0xf0) == 0xf0) {
		blips[x].mode = MODE_FALL;
		blips[x].y = 0;
		blips[x].speed = (r >> 30) + 1;
		blips[x].timer = 0;
	} else if ((r & 0x0f000) ==  0x0f000) {
		blips[x].mode = MODE_SLID;
		blips[x].timer = (r >> 28) + 15;
		blips[x].speed = ((r >> 24) & 3) + 2;
	}
}

//
void MatrixTV::blipFall(int x)
{
	int            y = blips[x].y;
	unsigned char *p = vmap + x + y * mapW;
	unsigned char *c = cmap + x + y * mapW;

	for (int i=blips[x].speed; i>0; i--) {
		if (blips[x].timer > 0) {
			*p = 255;
		} else {
			*p = 254 - i * 10;
		}
		*c = mUtils->fastrand() % CHARNUM;
		p += mapW;
		c += mapW;
		y++;
		if (y >= mapH) break;
	}
	if (blips[x].timer > 0) {
		blips[x].timer--;
	}

	if (y >= mapH) {
		blips[x].mode = MODE_NONE;
		y = 0; // ADD by Morihiro
	}

	blips[x].y = y;

	if (blips[x].timer == 0) {
		unsigned int r = mUtils->fastrand();
		if ((r & 0x3f00) == 0x3f00) {
			blips[x].timer = (r >> 28) + 8;
		} else if (blips[x].speed > 1 && (r & 0x7f) == 0x7f) {
			blips[x].mode = MODE_STOP;
			blips[x].timer = (r >> 26) + 30;
		}
	}
}

//
void MatrixTV::blipStop(int x)
{
	int y = blips[x].y;
	vmap[x + y * mapW] = 254;
	cmap[x + y * mapW] = mUtils->fastrand() % CHARNUM;

	blips[x].timer--;

	if (blips[x].timer < 0) {
		blips[x].mode = MODE_FALL;
	}
}

//
void MatrixTV::blipSlide(int x)
{
	blips[x].timer--;
	if (blips[x].timer < 0) {
		blips[x].mode = MODE_NONE;
	}

	unsigned char* p = cmap + x + mapW * (mapH - 1);
	int dy = mapW * blips[x].speed;

	for (int y=mapH - blips[x].speed; y>0; y--) {
		*p = *(p - dy);
		p -= mapW;
	}
	for (int y=blips[x].speed; y>0; y--) {
		*p = mUtils->fastrand() % CHARNUM;
		p -= mapW;
	}
}
