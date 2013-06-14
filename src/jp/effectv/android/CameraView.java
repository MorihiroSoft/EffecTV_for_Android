/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * CameraView.java : View for camera preview (Hidden behind EffectView)
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
import android.hardware.Camera;
import android.util.AttributeSet;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.ViewGroup.LayoutParams;

class CameraView extends SurfaceView implements
SurfaceHolder.Callback,
Camera.PreviewCallback
{
	private static final boolean DEBUG = false;
	private static final String TAG = "CameraView";

	//---------------------------------------------------------------------
	// INTERFACE
	//---------------------------------------------------------------------
	interface CameraPreviewBuffer {
		abstract byte[] getBuffer();
		abstract void onPreviewFrame(byte[] data);
	}

	//---------------------------------------------------------------------
	// MEMBERS
	//---------------------------------------------------------------------
	private boolean mIsResumed = false;
	private boolean mIsCreated = false;

	private int                 mCameraId            = 0;
	private int                 mCameraW             = 0;
	private int                 mCameraH             = 0;
	private int[]               mCameraFpsRange      = null;
	private Camera              mCamera              = null;
	private CameraPreviewBuffer mCameraPreviewBuffer = null;
	private boolean             mAutoLock            = false;

	//---------------------------------------------------------------------
	// PUBLIC METHODS
	//---------------------------------------------------------------------
	public CameraView(Context context) {
		super(context);
		init();
	}

	public CameraView(Context context, AttributeSet attrs) {
		super(context, attrs);
		init();
	}

	public CameraView(Context context, AttributeSet attrs, int defStyle) {
		super(context, attrs, defStyle);
		init();
	}

	@Override
	public void surfaceCreated(SurfaceHolder holder) {
		if (DEBUG) Log.d(TAG, "surfaceCreated()");
		mIsCreated = true;
		startCamera();
	}

	@Override
	public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
		if (DEBUG) Log.d(TAG, "surfaceChanged(): w="+width+", h="+height);
		if (mCameraPreviewBuffer == null) {
			throw new IllegalStateException("mCameraPreviewBuffer=null");
		}
		if (mCamera != null) {
			mCamera.stopPreview();

			if (width > mCameraW || height > mCameraH) {
				LayoutParams lparam = getLayoutParams();
				lparam.width  = mCameraW;
				lparam.height = mCameraH;
				setLayoutParams(lparam);
			}

			mCamera.addCallbackBuffer(mCameraPreviewBuffer.getBuffer());
			mCamera.setPreviewCallbackWithBuffer(this);
			mCamera.startPreview();
		}
	}

	@Override
	public void surfaceDestroyed(SurfaceHolder holder) {
		if (DEBUG) Log.d(TAG, "surfaceDestroyed()");
		mIsCreated = false;
		stopCamera();

	}

	@Override
	public void onPreviewFrame(byte[] data, Camera camera) {
		if (DEBUG) Log.d(TAG, "onPreviewFrame()");
		mCameraPreviewBuffer.onPreviewFrame(data);
		camera.addCallbackBuffer(mCameraPreviewBuffer.getBuffer());
	}

	public void setup(int id, int w, int h, int[] fps, CameraPreviewBuffer previewbuffer) {
		if (DEBUG) Log.d(TAG, "setup(): id="+id+", w="+w+", h="+h+", fps=("+fps[0]+", "+fps[1]+")");
		mCameraId            = id;
		mCameraW             = w;
		mCameraH             = h;
		mCameraFpsRange      = fps;
		mCameraPreviewBuffer = previewbuffer;
	}

	public void resume() {
		if (DEBUG) Log.d(TAG, "resume()");
		mIsResumed = true;
		startCamera();
	}

	public void pause() {
		if (DEBUG) Log.d(TAG, "pause()");
		mIsResumed = false;
		setAutoLock(false);
		stopCamera();
	}

	public boolean getAutoLock() {
		return mAutoLock;
	}

	public void setAutoLock(boolean lock) {
		if (DEBUG) Log.d(TAG, "setAutoLock(): lock="+lock);
		mAutoLock = lock;

		Camera.Parameters cp = mCamera.getParameters();
		if (cp.isAutoExposureLockSupported()) {
			cp.setAutoExposureLock(lock);
		}
		if (cp.isAutoWhiteBalanceLockSupported()) {
			cp.setAutoWhiteBalanceLock(lock);
		}
		mCamera.setParameters(cp);
	}

	//---------------------------------------------------------------------
	// PRIVATE METHODS
	//---------------------------------------------------------------------
	private void init() {
		if (DEBUG) Log.d(TAG, "init()");
		getHolder().addCallback(this);

		if (Camera.getNumberOfCameras() < 1) {
			throw new UnsupportedOperationException("No camera");
		}
	}

	private void startCamera() {
		if (DEBUG) Log.d(TAG, "startCamera()");
		if (mCameraPreviewBuffer == null) {
			throw new IllegalStateException("mCameraPreviewBuffer=null");
		}
		if (mIsCreated && mIsResumed && mCamera == null) {
			try {
				mCamera = Camera.open(mCameraId);
				Camera.Parameters cp = mCamera.getParameters();
				cp.setPreviewSize(mCameraW, mCameraH);
				cp.setPreviewFpsRange(
						mCameraFpsRange[Camera.Parameters.PREVIEW_FPS_MIN_INDEX],
						mCameraFpsRange[Camera.Parameters.PREVIEW_FPS_MAX_INDEX]);
				mCamera.setParameters(cp);
				mCamera.addCallbackBuffer(mCameraPreviewBuffer.getBuffer());
				mCamera.setPreviewCallbackWithBuffer(this);
				mCamera.setPreviewDisplay(getHolder());
				mCamera.startPreview();
			} catch (Exception e) {
				e.printStackTrace();
				throw new RuntimeException("setup camera");
			}
		}
	}

	private void stopCamera() {
		if (DEBUG) Log.d(TAG, "stopCamera()");
		if (mCamera != null) {
			mCamera.stopPreview();
			mCamera.setPreviewCallback(null);
			mCamera.release();
			mCamera = null;
		}
	}
}
