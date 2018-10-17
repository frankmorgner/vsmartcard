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
import android.content.Intent;
import android.content.SharedPreferences;
import android.nfc.NfcAdapter;
import android.nfc.Tag;
import android.nfc.tech.IsoDep;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.WindowManager;

import com.vsmartcard.remotesmartcardreader.app.Hex;

import java.io.IOException;

public class NFCReader implements SCReader {

    private final IsoDep card;
    private Activity activity;

    private NFCReader(IsoDep sc, Activity activity) throws IOException {
        this.card = sc;
        sc.connect();
        SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity);
        int timeout = Integer.parseInt(SP.getString("timeout", "500"));
        card.setTimeout(timeout);
        com.example.android.common.logger.Log.i(getClass().getName(), "Timeout set to " + Integer.toString(timeout));
        this.activity = activity;
        avoidScreenTimeout();
    }

    private void avoidScreenTimeout() {
        if (activity != null) {
            final Runnable run = new Runnable() {
                public void run() {
                    // avoid tag loss due to screen timeout
                    activity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
                    Log.i(this.getClass().getName(), "Disabled screen timeout");
                }
            };
            final Handler handler = new Handler(Looper.getMainLooper());
            handler.post(run);
        }
    }

    private void resetScreenTimeout() {
        if (activity != null) {
            final Runnable run = new Runnable() {
                public void run() {
                    // reset screen properties
                    activity.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
                    Log.i(this.getClass().getName(), "Activated screen timeout");
                }
            };
            final Handler handler = new Handler(Looper.getMainLooper());
            handler.post(run);
        }
    }

    @Override
    public void eject() throws IOException {
        card.close();
        resetScreenTimeout();
    }

    @Override
    public void powerOn() {
        /* should already be connected... */
    }

    private static final byte[] SELECT_MF = {(byte) 0x00, (byte) 0xa4, (byte) 0x00, (byte) 0x0C};
    private void selectMF() throws IOException {
        byte[] response = card.transceive(SELECT_MF);
        if (response.length == 2 && response[0] == (byte) 0x90 && response[1] == (byte) 0x00) {
            Log.d(this.getClass().getName(), "Resetting the card by selecting the MF results in " + Hex.getHexString(response));
        }
    }

    @Override
    public void powerOff() throws IOException {
        selectMF();
    }

    @Override
    public void reset() throws IOException {
        selectMF();
    }

    /* calculation based on https://code.google.com/p/ifdnfc/source/browse/src/atr.c */
    @Override
    public byte[] getATR() {
        // get historical bytes for 14443-A
        byte[] historicalBytes = card.getHistoricalBytes();
        if (historicalBytes == null) {
            // get historical bytes for 14443-B
            historicalBytes = card.getHiLayerResponse();
        }
        if (historicalBytes == null) {
            historicalBytes = new byte[0];
        }

        /* copy historical bytes if available */
        byte[] atr = new byte[4+historicalBytes.length+1];
        atr[0] = (byte) 0x3b;
        atr[1] = (byte) (0x80+historicalBytes.length);
        atr[2] = (byte) 0x80;
        atr[3] = (byte) 0x01;
        System.arraycopy(historicalBytes, 0, atr, 4, historicalBytes.length);

        /* calculate TCK */
        byte tck = atr[1];
        for (int idx = 2; idx < atr.length; idx++) {
            tck ^= atr[idx];
        }
        atr[atr.length-1] = tck;

        return atr;
    }

    @Override
    public byte[] transmit(byte[] apdu) throws IOException {
        return card.transceive(apdu);
    }

    public static NFCReader get(Intent intent, Activity activity) {
        NFCReader nfcReader = null;

        if (intent.getExtras() != null) {
            Tag tag = intent.getParcelableExtra(NfcAdapter.EXTRA_TAG);
            if (tag != null) {
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
                    /* Disconnect from the tag here. The reader mode will capture the tag again */
                    try {
                        IsoDep.get(tag).close();
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                } else {
                    nfcReader = NFCReader.get(tag, activity);
                }
                intent.removeExtra(NfcAdapter.EXTRA_TAG);
            }
        }

        return nfcReader;
    }

    public static NFCReader get(Tag tag, Activity activity) {
        NFCReader nfcReader = null;
        try {
            nfcReader = new NFCReader(IsoDep.get(tag), activity);
        } catch (IOException e) {
            e.printStackTrace();
        }
        return nfcReader;
    }
}