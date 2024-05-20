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
import android.nfc.tech.NfcA;
import android.nfc.tech.NfcB;
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
    private final Activity activity;

    private NFCReader(IsoDep sc, Activity activity) throws IOException {
        this.card = sc;
        sc.connect();
        SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity);
        int timeout = Integer.parseInt(SP.getString("timeout", "500"));
        card.setTimeout(timeout);
        com.example.android.common.logger.Log.i(getClass().getName(), "Timeout set to " + timeout);
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

    /* generate the mapped ATR from 14443 data according to PC/SC part 3 section 3.1.3.2.3 */
    @Override
    public byte[] getATR() {
        // for 14443 Type A, use the historical bytes returned as part of the ATS
        byte[] historicalBytes = card.getHistoricalBytes();
        if (historicalBytes == null) {
            // for 14443 Type B, use Application Data + Protocol Info + MBLI
            historicalBytes = getTypeBHistoricalBytes();
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

    public byte[] getTypeBHistoricalBytes() {
        NfcB nfcB = NfcB.get(card.getTag());
        if (nfcB == null)
            return null;

        byte[] appData = nfcB.getApplicationData();
        byte[] protocolInfo = nfcB.getProtocolInfo();
        if (!(appData.length == 4 && protocolInfo.length == 3))
            return null;

        Byte mbli = translateToMbli(protocolInfo, nfcB.getMaxTransceiveLength());
        if (mbli == null)
            return null;

        byte[] historicalBytes = new byte[8];
        System.arraycopy(appData, 0, historicalBytes, 0, 4);
        System.arraycopy(protocolInfo, 0, historicalBytes, 4, 3);
        historicalBytes[7] = (byte)(mbli << 4);
        return historicalBytes;
    }

    private static final int[] ATQB_FRAME_SIZES = { 16, 24, 32, 40, 48, 64, 96, 128, 256 };

    public static Byte translateToMbli(byte[] protocolInfo, int maxUnit) {
        // retrieve maximum frame size from protocol info
        int maxFrameSizeCode = (protocolInfo[1] >> (byte)4) & 0xF;
        if (maxFrameSizeCode >= ATQB_FRAME_SIZES.length)
            return null; // values 9..15 are RFU
        int maxFrameSize = ATQB_FRAME_SIZES[maxFrameSizeCode];

        // there's 3 to 5 bytes of overhead in a buffer.
        int predictedMbl = maxUnit + 5; // (can be up to 2 bytes larger)

        // buffer length must be maxFrameSize * {power of 2}.
        int mbl = Integer.highestOneBit(predictedMbl / maxFrameSize) * maxFrameSize;
        if (predictedMbl - mbl > 2)
            return null;

        // now that we know we have a valid MBL, calculate MBLI from it
        int mbli = Integer.numberOfTrailingZeros(mbl / maxFrameSize) + 1;
        if (mbli > 15)
            return null;
        return (byte)mbli;
    }
}
