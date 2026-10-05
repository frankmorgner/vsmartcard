/*
 * Copyright (C) 2014 Frank Morgner
 *
 * This file is part of RemoteSmartCardReader.
 *
 * RemoteSmartCardReader is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * RemoteSmartCardReader is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * RemoteSmartCardReader.  If not, see <http://www.gnu.org/licenses/>.
 */

package com.vsmartcard.remotesmartcardreader.app.screaders;

import android.app.Activity;
import android.nfc.Tag;
import android.nfc.tech.IsoDep;
import android.nfc.tech.NfcF;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.WindowManager;

import java.io.IOException;

public abstract class NFCReader implements SCReader {

    protected final Activity activity;

    protected NFCReader(Activity activity) {
        this.activity = activity;
        avoidScreenTimeout();
    }

    protected void avoidScreenTimeout() {
        if (activity != null) {
            final Runnable run = new Runnable() {
                public void run() {
                    // avoid tag loss due to screen timeout
                    activity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
                    Log.i(getClass().getName(), "Disabled screen timeout");
                }
            };
            final Handler handler = new Handler(Looper.getMainLooper());
            handler.post(run);
        }
    }

    protected void resetScreenTimeout() {
        if (activity != null) {
            final Runnable run = new Runnable() {
                public void run() {
                    // reset screen properties
                    activity.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
                    Log.i(getClass().getName(), "Activated screen timeout");
                }
            };
            final Handler handler = new Handler(Looper.getMainLooper());
            handler.post(run);
        }
    }

    @Override
    public void eject() throws IOException {
        resetScreenTimeout();
    }

    @Override
    public void powerOn() throws IOException {
        /* should already be connected */
    }

    public static NFCReader get(Tag tag, Activity activity) {
        if (tag == null) {
            return null;
        }

        IsoDep isoDep = IsoDep.get(tag);
        if (isoDep != null) {
            try {
                return new IsoDepReader(isoDep, activity);
            } catch (IOException e) {
                Log.e(NFCReader.class.getName(), "Error connecting to IsoDep tag: " + e.getMessage(), e);
                com.example.android.common.logger.Log.e(NFCReader.class.getName(), "Error connecting to IsoDep tag: " + e.getMessage());
            }
        }

        NfcF nfcF = NfcF.get(tag);
        if (nfcF != null) {
            try {
                return new NfcFReader(nfcF, activity);
            } catch (IOException e) {
                Log.e(NFCReader.class.getName(), "Error connecting to NfcF tag: " + e.getMessage(), e);
                com.example.android.common.logger.Log.e(NFCReader.class.getName(), "Error connecting to FeliCa tag: " + e.getMessage());
            }
        }

        com.example.android.common.logger.Log.e(NFCReader.class.getName(), "Tag does not support ISO-DEP (ISO 14443-4) or NFC-F (JIS 6319-4)");
        return null;
    }

    public static Byte translateToMbli(byte[] protocolInfo, int maxUnit) {
        return IsoDepReader.translateToMbli(protocolInfo, maxUnit);
    }
}
