/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * EffectView.java : View for effect
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

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Paint.FontMetrics;
import android.util.AttributeSet;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceHolder.Callback;
import android.view.SurfaceView;

class EffectView extends SurfaceView implements
Callback,
Runnable
{
	private static final boolean DEBUG = false;
	private static final String TAG = "EffectView";

	//---------------------------------------------------------------------
	// CONSTANTS
	//---------------------------------------------------------------------
	private static final float FONT_SIZE = 16.0f;
	private static final float TEXT_X = 4.0f;
	private static final float TEXT_Y = 4.0f; // + font_h

	//---------------------------------------------------------------------
	// MEMBERS
	//---------------------------------------------------------------------
	private boolean mIsResumed = false;
	private boolean mIsCreated = false;

	private Thread mThread = null;

	private SurfaceHolder mHolder = null;
	private int mSurfaceW = 0;
	private int mSurfaceH = 0;

	private int      mImageW   = 0;
	private int      mImageH   = 0;
	private int[][]  mImagePix = null;
	private char[][] mImageMsg = null;
	private int[]    mImageIdx = null;

	//---------------------------------------------------------------------
	// PUBLIC MRTHODS
	//---------------------------------------------------------------------
	public EffectView(Context context) {
		super(context);
		init();
	}

	public EffectView(Context context, AttributeSet attrs) {
		super(context, attrs);
		init();
	}

	public EffectView(Context context, AttributeSet attrs, int defStyle) {
		super(context, attrs, defStyle);
		init();
	}

	@Override
	public void surfaceCreated(SurfaceHolder holder) {
		if (DEBUG) Log.d(TAG, "surfaceCreated()");
		mIsCreated = true;
		startThread();
	}

	@Override
	public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
		if (DEBUG) Log.d(TAG, "surfaceChanged(): surface w="+width+", h="+height);
		mHolder = holder;
		mSurfaceW = width;
		mSurfaceH = height;
	}

	@Override
	public void surfaceDestroyed(SurfaceHolder holder) {
		if (DEBUG) Log.d(TAG, "surfaceDestroyed()");
		mIsCreated = false;
		stopThread();
	}

	public void setup(int image_w, int image_h, int[][] image_pix, char[][] image_msg, int[] image_idx) {
		if (DEBUG) Log.d(TAG, "setup(): image w="+image_w+", h="+image_h);
		mImageW   = image_w;
		mImageH   = image_h;
		mImagePix = image_pix;
		mImageMsg = image_msg;
		mImageIdx = image_idx;
	}

	public void resume() {
		if (DEBUG) Log.d(TAG, "resume()");
		mIsResumed = true;
		startThread();
	}

	public void pause() {
		if (DEBUG) Log.d(TAG, "pause()");
		mIsResumed = false;
		stopThread();
	}

	public void run() {
		if (DEBUG) Log.d(TAG, "run()");
		Canvas canvas;
		Paint paint = new Paint();

		paint.setAntiAlias(true);
		paint.setTextSize(FONT_SIZE);
		final FontMetrics fm = paint.getFontMetrics();
		final float font_h = -fm.ascent;

		while(mThread != null){
			if (mHolder == null || mSurfaceW < 1 || mSurfaceH < 1) {
				continue;
			}
			int idx = 0;
			try {
				synchronized (mImageIdx) {
					mImageIdx.wait();
					idx = mImageIdx[0];
				}
			} catch (InterruptedException e) {
				e.printStackTrace();
				throw new RuntimeException();
			}
			if (DEBUG) Log.d(TAG, "run() - notify: idx="+idx);

			canvas = mHolder.lockCanvas();
			if (canvas == null) {
				continue;
			}

			canvas.drawColor(Color.GRAY);

			float s;
			if (mSurfaceW * mImageH < mSurfaceH * mImageW) {
				s = (float)mSurfaceW / (float)mImageW;
			} else {
				s = (float)mSurfaceH / (float)mImageH;
			}
			canvas.scale(s, s);
			canvas.drawBitmap(mImagePix[idx], 0, mImageW, 0, 0, mImageW, mImageH, false, paint);

			int len = 0;
			while(mImageMsg[idx][len] != 0) len++;
			if (len > 0) {
				paint.setStrokeWidth(2.0f);
				paint.setColor(Color.BLACK);
				paint.setStyle(Paint.Style.STROKE);
				canvas.drawText(mImageMsg[idx], 0, len, TEXT_X, TEXT_Y+font_h, paint);
				paint.setStrokeWidth(0.0f);
				paint.setColor(Color.WHITE);
				paint.setStyle(Paint.Style.FILL);
				canvas.drawText(mImageMsg[idx], 0, len, TEXT_X, TEXT_Y+font_h, paint);
			}

			mHolder.unlockCanvasAndPost(canvas);
		}
	}

	//---------------------------------------------------------------------
	// PRIVATE METHODS
	//---------------------------------------------------------------------
	private void init() {
		if (DEBUG) Log.d(TAG, "init()");
		getHolder().addCallback(this);
	}

	private void startThread() {
		if (DEBUG) Log.d(TAG, "startThread()");
		if (mIsCreated && mIsResumed && mThread == null) {
			mThread = new Thread(this);
			mThread.start();
		}
	}

	private void stopThread() {
		if (DEBUG) Log.d(TAG, "stopThread()");
		if (mThread != null) {
			mThread = null;
		}
	}
}
