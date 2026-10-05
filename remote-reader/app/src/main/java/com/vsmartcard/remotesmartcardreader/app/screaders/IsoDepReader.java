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
import android.content.SharedPreferences;
import android.nfc.tech.IsoDep;
import android.nfc.tech.NfcB;
import android.preference.PreferenceManager;
import android.util.Log;

import com.vsmartcard.remotesmartcardreader.app.Hex;

import java.io.IOException;

public class IsoDepReader extends NFCReader {

    private final IsoDep card;

    public IsoDepReader(IsoDep sc, Activity activity) throws IOException {
        super(activity);
        if (sc == null) {
            throw new IOException("IsoDep is null");
        }
        this.card = sc;
        sc.connect();
        int timeout = 500;
        if (activity != null) {
            SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity);
            timeout = Integer.parseInt(SP.getString("timeout", "500"));
        }
        card.setTimeout(timeout);
        com.example.android.common.logger.Log.i(getClass().getName(), "Timeout set to " + timeout);
    }

    @Override
    public void eject() throws IOException {
        try {
            card.close();
        } finally {
            super.eject();
        }
    }

    private static final byte[] SELECT_MF = {(byte) 0x00, (byte) 0xa4, (byte) 0x00, (byte) 0x0C};
    private void selectMF() throws IOException {
        byte[] response = card.transceive(SELECT_MF);
        if (response != null && response.length == 2 && response[0] == (byte) 0x90 && response[1] == (byte) 0x00) {
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
        byte[] atr = new byte[4 + historicalBytes.length + 1];
        atr[0] = (byte) 0x3b;
        atr[1] = (byte) (0x80 + historicalBytes.length);
        atr[2] = (byte) 0x80;
        atr[3] = (byte) 0x01;
        System.arraycopy(historicalBytes, 0, atr, 4, historicalBytes.length);

        /* calculate TCK */
        byte tck = atr[1];
        for (int idx = 2; idx < atr.length; idx++) {
            tck ^= atr[idx];
        }
        atr[atr.length - 1] = tck;

        return atr;
    }

    @Override
    public byte[] transmit(byte[] apdu) throws IOException {
        return card.transceive(apdu);
    }

    public byte[] getTypeBHistoricalBytes() {
        NfcB nfcB = NfcB.get(card.getTag());
        if (nfcB == null)
            return null;

        byte[] appData = nfcB.getApplicationData();
        byte[] protocolInfo = nfcB.getProtocolInfo();
        if (appData == null || protocolInfo == null || !(appData.length == 4 && protocolInfo.length == 3))
            return null;

        Byte mbli = translateToMbli(protocolInfo, nfcB.getMaxTransceiveLength());
        if (mbli == null)
            return null;

        byte[] historicalBytes = new byte[8];
        System.arraycopy(appData, 0, historicalBytes, 0, 4);
        System.arraycopy(protocolInfo, 0, historicalBytes, 4, 3);
        historicalBytes[7] = (byte) (mbli << 4);
        return historicalBytes;
    }

    private static final int[] ATQB_FRAME_SIZES = {16, 24, 32, 40, 48, 64, 96, 128, 256};

    public static Byte translateToMbli(byte[] protocolInfo, int maxUnit) {
        if (protocolInfo == null || protocolInfo.length < 2)
            return null;
        // retrieve maximum frame size from protocol info
        int maxFrameSizeCode = (protocolInfo[1] >> (byte) 4) & 0xF;
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
        return (byte) mbli;
    }
}
