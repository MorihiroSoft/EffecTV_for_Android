/*
 * EffecTV for Android
 * Copyright (C) 2013 Morihiro Soft
 *
 * DirSelectionDialog.java :
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

import java.io.File;
import java.io.IOException;
import java.util.Comparator;

import android.app.AlertDialog;
import android.content.Context;
import android.os.Environment;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.Toast;

public class DirSelectionDialog implements
AdapterView.OnItemClickListener
{
	//---------------------------------------------------------------------
	// CONSTANTS
	//---------------------------------------------------------------------
	private static final String KEY_SLASH  = "/";
	private static final String KEY_DOT    = "."; // Skip
	private static final String KEY_SELECT = "[Select this]";
	private static final String KEY_PARENT = ".." + KEY_SLASH;

	//---------------------------------------------------------------------
	// INTERFACE
	//---------------------------------------------------------------------
	interface OnDirSelectListener {
		abstract void onDirSelected(final String dir);
	}

	//---------------------------------------------------------------------
	// MEMBERS
	//---------------------------------------------------------------------
	private Context             mContext  = null;
	private OnDirSelectListener mListener = null;
	private String              mPath     = null;
	private DirAdapter          mAdapter  = null;
	private AlertDialog         mDialog   = null;
	private ListView            mListView = null;

	//---------------------------------------------------------------------
	// PUBLIC METHODS
	//---------------------------------------------------------------------
	public DirSelectionDialog(Context context, OnDirSelectListener listener) {
		if (context == null || listener == null) {
			throw new IllegalArgumentException("context="+context+", listener="+listener);
		}
		mContext  = context;
		mListener = listener;
	}

	public void show(String path) {
		mAdapter = new DirAdapter(mContext, android.R.layout.simple_list_item_1);
		mPath = mAdapter.setPath(path!=null ? path :
			Environment.getExternalStorageDirectory().getAbsolutePath());
		if (mPath == null) {
			throw new IllegalArgumentException("path="+path);
		}

		mDialog = new AlertDialog.Builder(mContext)
		.setTitle(mPath)
		.setAdapter(mAdapter, null)
		.create();

		mListView = mDialog.getListView();
		mListView.setOnItemClickListener(this);
		mListView.setScrollbarFadingEnabled(false);

		mDialog.setCanceledOnTouchOutside(true);
		mDialog.show();
	}

	@Override
	public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
		final String item = mAdapter.getItem(position);
		if (KEY_SELECT.equals(item)) {
			mListener.onDirSelected(mPath);
			mDialog.dismiss();
			mDialog = null;
		} else {
			String new_path = mAdapter.setPath(mPath + item);
			if (new_path != null) {
				mPath = new_path;
				mDialog.setTitle(mPath);
				mListView.setSelection(0);
			}
		}
	}

	//---------------------------------------------------------------------
	// Adapter
	//---------------------------------------------------------------------
	public class DirAdapter extends ArrayAdapter<String> {
		public DirAdapter(Context context, int textViewResourceId) {
			super(context, textViewResourceId);
		}

		public String setPath(String path) {
			// Check path
			File d = new File(path);
			if (!d.exists() || !d.isDirectory()) {
				Toast.makeText(getContext(), "Not directory",
						Toast.LENGTH_LONG).show();
				return null;
			}
			if (!d.canRead()) {
				Toast.makeText(getContext(), "Cannot read directory",
						Toast.LENGTH_LONG).show();
				return null;
			}

			// Format path name
			try {
				path = d.getCanonicalPath() + KEY_SLASH;
			} catch (IOException e) {
				e.printStackTrace();
			}

			// Update directory list
			this.clear();
			for (File f : d.listFiles()) {
				final String name = f.getName() + KEY_SLASH;
				if (f.isDirectory() && !name.startsWith(KEY_DOT)) {
					this.add(name);
				}
			}
			this.sort(new Comparator<String>() {
				@Override
				public int compare(String arg0, String arg1) {
					if (arg0 != null && arg1 != null) {
						return arg0.compareTo(arg1);
					} else {
						return 0;
					}
				}
			});
			this.insert(KEY_PARENT, 0);
			if (d.canWrite()) {
				this.insert(KEY_SELECT, 0);
			}
			this.notifyDataSetChanged();

			//
			return path;
		}
	}
}
