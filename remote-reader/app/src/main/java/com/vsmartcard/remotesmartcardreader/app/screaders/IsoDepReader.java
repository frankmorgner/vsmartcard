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
import android.nfc.Tag;
import android.nfc.tech.IsoDep;
import android.nfc.tech.NfcB;
import android.preference.PreferenceManager;
import android.util.Log;

import com.vsmartcard.remotesmartcardreader.app.Hex;

import java.io.IOException;

/**
 * SCReader implementation for ISO/IEC 14443-4 (ISO-DEP) Type A and Type B smart cards,
 * including MIFARE DESFire and MIFARE Plus (in ISO/IEC 14443-4 mode).
 *
 * References:
 * - PC/SC Workgroup Part 3 Rev 2.01.09, Section 3.1.3.2.3.1 (Table 3-5: Contactless Smart Cards ATR)
 * - PC/SC Workgroup Part 3 Rev 2.01.09, Section 3.2.2.1.3 (Tables 3-8a, 3-8b, 3-9: GET DATA Command)
 * - ACS ACR122U API V2.04, Section 3.1.2 (Table 3: ATR format for ISO 14443 Part 4 PICCs)
 * - ACS ACR122U API V2.04, Section 4.1 (Get Data: FF CA 00 00 for UID, FF CA 01 00 for ATS)
 * - ACS ACR122U API V2.04, Section 6.1 (Direct Transmit: FF 00 00 00)
 * - ACS ACR122U API V2.04, Section 7.2 (MIFARE DESFire ISO 7816-4 Wrapping and Native Mode)
 */
public class IsoDepReader extends NFCReader {

    public interface Transceiver {
        byte[] transceive(byte[] data) throws IOException;
        void close() throws IOException;
        void setTimeout(int timeout);
        byte[] getHistoricalBytes();
        byte[] getTypeBHistoricalBytes();
        byte[] getUid();
    }

    private final IsoDep card;
    private final Transceiver transceiver;

    public IsoDepReader(final IsoDep sc, Activity activity) throws IOException {
        this(createTransceiver(sc), sc, activity);
    }

    IsoDepReader(Transceiver transceiver, Activity activity) throws IOException {
        this(transceiver, null, activity);
    }

    private IsoDepReader(Transceiver transceiver, IsoDep sc, Activity activity) throws IOException {
        super(activity);
        if (transceiver == null) {
            throw new IOException("IsoDep transceiver is null");
        }
        this.card = sc;
        this.transceiver = transceiver;
        int timeout = 500;
        if (activity != null) {
            SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity);
            timeout = Integer.parseInt(SP.getString("timeout", "500"));
        }
        this.transceiver.setTimeout(timeout);
        com.example.android.common.logger.Log.i(getClass().getName(), "Timeout set to " + timeout);
    }

    private static Transceiver createTransceiver(final IsoDep sc) throws IOException {
        if (sc == null) {
            throw new IOException("IsoDep is null");
        }
        sc.connect();
        return new Transceiver() {
            @Override
            public byte[] transceive(byte[] data) throws IOException {
                return sc.transceive(data);
            }

            @Override
            public void close() throws IOException {
                sc.close();
            }

            @Override
            public void setTimeout(int timeout) {
                sc.setTimeout(timeout);
            }

            @Override
            public byte[] getHistoricalBytes() {
                return sc.getHistoricalBytes();
            }

            @Override
            public byte[] getTypeBHistoricalBytes() {
                return extractTypeBHistoricalBytes(sc.getTag());
            }

            @Override
            public byte[] getUid() {
                Tag tag = sc.getTag();
                return tag != null ? tag.getId() : null;
            }
        };
    }

    @Override
    public void eject() throws IOException {
        try {
            transceiver.close();
        } finally {
            super.eject();
        }
    }

    private static final byte[] SELECT_MF = {(byte) 0x00, (byte) 0xa4, (byte) 0x00, (byte) 0x0C};
    private void selectMF() throws IOException {
        byte[] response = transceiver.transceive(SELECT_MF);
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
        byte[] historicalBytes = transceiver.getHistoricalBytes();
        if (historicalBytes == null) {
            // for 14443 Type B, use Application Data + Protocol Info + MBLI
            historicalBytes = transceiver.getTypeBHistoricalBytes();
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
        if (apdu == null || apdu.length == 0) {
            return new byte[]{(byte) 0x67, 0x00};
        }

        // GET DATA: FF CA P1 P2 [Le]
        // PC/SC Part 3 Section 3.2.2.1.3 ("can be used for all kinds of contactless cards")
        // and ACS ACR122U API V2.04 Section 4.1:
        // - P1=0x00, P2=0x00: UID is returned
        // - P1=0x01, P2=0x00: Historical bytes from the ATS without CRC are returned
        if (apdu.length >= 2 && apdu[0] == (byte) 0xFF && apdu[1] == (byte) 0xCA) {
            if (apdu.length != 4 && apdu.length != 5) {
                return new byte[]{(byte) 0x67, 0x00};
            }
            if (apdu[3] != 0x00) {
                return new byte[]{(byte) 0x6A, (byte) 0x86}; // Incorrect P1/P2 parameters
            }
            byte p1 = apdu[2];
            int le = (apdu.length >= 5) ? (apdu[4] & 0xFF) : 0;
            byte[] targetData;
            if (p1 == 0x00) {
                targetData = transceiver.getUid();
                if (targetData == null) {
                    return new byte[]{(byte) 0x6A, (byte) 0x81};
                }
            } else if (p1 == 0x01) {
                targetData = transceiver.getHistoricalBytes();
                if (targetData == null) {
                    targetData = transceiver.getTypeBHistoricalBytes();
                }
                if (targetData == null) {
                    targetData = new byte[0];
                }
            } else {
                return new byte[]{(byte) 0x6A, (byte) 0x81}; // Function not supported
            }
            return formatGetDataResponse(targetData, le);
        }

        // Direct Transmit: ACS ACR122U API V2.04 Section 6.1 (FF 00 00 00 Lc [Payload])
        if (apdu.length >= 4 && apdu[0] == (byte) 0xFF && apdu[1] == 0x00) {
            if (apdu[2] != 0x00 || apdu[3] != 0x00) {
                return new byte[]{(byte) 0x6A, (byte) 0x86};
            }
            if (apdu.length < 5) {
                return new byte[]{(byte) 0x67, 0x00};
            }
            int lc = apdu[4] & 0xFF;
            if (lc == 0 || apdu.length != 5 + lc) {
                return new byte[]{(byte) 0x67, 0x00};
            }
            byte[] rawCmd = new byte[lc];
            System.arraycopy(apdu, 5, rawCmd, 0, lc);
            try {
                byte[] rawResp = transceiver.transceive(rawCmd);
                if (rawResp == null) {
                    return new byte[]{(byte) 0x63, 0x00};
                }
                return wrapStatus(rawResp, (byte) 0x90, (byte) 0x00);
            } catch (IOException e) {
                Log.w(getClass().getName(), "Direct transmit failed: " + e.getMessage());
                return new byte[]{(byte) 0x63, 0x00};
            }
        }

        try {
            byte[] response = transceiver.transceive(apdu);
            if (response == null) {
                return new byte[]{(byte) 0x6F, 0x00};
            }
            // ACS ACR122U API V2.04 Section 7.2 (Page 33):
            // "In DESFire Native Mode, the status code [90 00h] will not be added to the response
            // if the response length is greater than 1. If the response length is less than 2,
            // the status code [90 00h] will be added in order to meet the requirement of PC/SC.
            // The minimum response length is 2."
            if (response.length < 2) {
                return wrapStatus(response, (byte) 0x90, (byte) 0x00);
            }
            return response;
        } catch (IOException e) {
            Log.w(getClass().getName(), "transceive failed: " + e.getMessage());
            return new byte[]{(byte) 0x6F, 0x00};
        }
    }

    static byte[] formatGetDataResponse(byte[] data, int le) {
        if (le == 0 || le == data.length) {
            return wrapStatus(data, (byte) 0x90, (byte) 0x00);
        }
        if (le < data.length) {
            // PC/SC Part 3 Section 3.2.2.1.3 Table 3-9:
            // '6C' 'xx': Wrong length (wrong number Le; 'XX' encodes the exact number)
            return new byte[]{(byte) 0x6C, (byte) (data.length & 0xFF)};
        }
        // PC/SC Part 3 Section 3.2.2.1.3 & Table 3-9:
        // Pad with zero-value bytes up to Le and return warning '62' '82'
        byte[] padded = new byte[le + 2];
        System.arraycopy(data, 0, padded, 0, data.length);
        padded[le] = (byte) 0x62;
        padded[le + 1] = (byte) 0x82;
        return padded;
    }

    private static byte[] wrapStatus(byte[] data, byte sw1, byte sw2) {
        int len = data != null ? data.length : 0;
        byte[] result = new byte[len + 2];
        if (len > 0) {
            System.arraycopy(data, 0, result, 0, len);
        }
        result[len] = sw1;
        result[len + 1] = sw2;
        return result;
    }

    public byte[] getTypeBHistoricalBytes() {
        if (card == null) {
            return transceiver.getTypeBHistoricalBytes();
        }
        return extractTypeBHistoricalBytes(card.getTag());
    }

    private static byte[] extractTypeBHistoricalBytes(Tag tag) {
        if (tag == null)
            return null;
        NfcB nfcB = NfcB.get(tag);
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
