/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * MainApplication.java :
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

package jp.effectv.android;

import android.app.Application;
import android.util.Log;

public class MainApplication extends Application
{
	private static final boolean DEBUG = false;
	private static final String TAG = "MainApplication";

	//---------------------------------------------------------------------
	// CONSTANTS
	//---------------------------------------------------------------------
	public static final int DST_MSG_LEN = 256;

	//---------------------------------------------------------------------
	// NATIVE METHODS
	//---------------------------------------------------------------------
	static {
		System.loadLibrary("EffecTV");
	}
	native public static void native_init(String local_path);
	native public static String[] native_names();
	native public static String[] native_titles();
	native public static String[] native_funcs(int effect_type);
	native public static int native_start(boolean f, int w, int h, int fps, int effect_type);
	native public static int native_stop();
	native public static int native_draw(byte[] src_yuv, int[] dst_rgb, char[] dst_msg, int dst_yuv_fmt, byte[] dst_yuv);
	native public static String native_event(int key_code);
	native public static String native_touch(int action, int x, int y);
	native public static int native_cnvavc(String src_path, String dst_path);

	//---------------------------------------------------------------------
	// PUBLIC METHODS
	//---------------------------------------------------------------------
	public MainApplication() {
		super();
		if (DEBUG) Log.d(TAG, "MainApplication()");
	}
}
