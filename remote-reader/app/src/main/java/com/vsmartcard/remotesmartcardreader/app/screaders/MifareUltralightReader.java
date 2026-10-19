/*
 * Copyright (C) 2026 Frank Morgner
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
import android.nfc.tech.MifareUltralight;
import android.preference.PreferenceManager;
import android.util.Log;

import com.vsmartcard.remotesmartcardreader.app.Hex;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.Arrays;

/**
 * PC/SC and native frame reader implementation for MIFARE Ultralight and MIFARE Ultralight C cards.
 *
 * <p>Specification references:
 * <ul>
 *   <li>PC/SC Workgroup Part 3 Rev 2.01.09: <i>Requirements for PC-Connected Interface Devices</i>,
 *       Section 3.1.3.2.3.2 (Table 3-6, Pages 21-22: Contactless Storage Cards ATR),
 *       Section 3.2.2.1.3 (Tables 3-8a, 3-8b, 3-9, Pages 26-27: Get Data Command),
 *       Section 3.2.2.1.8 (Tables 3-18a, 3-18b, 3-19, Page 31: Read Binary Command),
 *       Section 3.2.2.1.9 (Tables 3-20a, 3-20b, 3-21, Page 32: Update Binary Command).</li>
 *   <li>PC/SC Workgroup Part 3 Supplemental Document Rev 2.01.09,
 *       Section 2.1.2.2 (Table 3, Pages 2-4: NN - Bytes For Card Name:
 *       {@code 00 03} = MIFARE Ultralight, {@code 00 3A} = MIFARE Ultralight C).</li>
 *   <li>Advanced Card Systems (ACS) <i>ACR122U Application Programming Interface V2.04</i>,
 *       Section 3.1.1 (Table 2, Pages 8-9: ATR format for ISO 14443 Part 3 PICCs),
 *       Section 4.1 (Page 11: Get Data),
 *       Section 5.2 (Table 6, Page 15: MIFARE Ultralight Memory Map),
 *       Section 5.3 (Page 16: Read Binary Blocks),
 *       Section 5.4 (Page 17: Update Binary Blocks),
 *       Section 6.1 (Page 21: Direct Transmit).</li>
 *   <li>NXP Semiconductors <i>MF0ICU1 MIFARE Ultralight Contactless Single-Ticket IC Product Data Sheet</i>
 *       (Doc. No. 028639, Rev. 3.9, 2014-07-23),
 *       Section 7.5 (Table 5, Page 10: Memory organization),
 *       Section 7.6.5 (Table 10, Page 16: READ {@code 30h}),
 *       Section 7.6.7 (Table 12, Page 17: WRITE {@code A2h}),
 *       Section 7.6.8 (Table 13, Page 17: COMPATIBILITY WRITE {@code A0h}).</li>
 *   <li>{@code libfreefare} PC/SC backend ({@code mifare_ultralight.c} in
 *       {@code frankmorgner/libfreefare}), which exchanges raw MIFARE Ultralight command frames
 *       ({@code 30}, {@code A2}) after identifying the card via its PC/SC Storage Card ATR and
 *       {@code FF CA 00 00 00}.</li>
 * </ul>
 */
public class MifareUltralightReader extends NFCReader {

    public interface Transceiver {
        void connect() throws IOException;
        void close() throws IOException;
        void setTimeout(int timeout);
        int getTimeout();
        int getType();
        byte[] getUid();
        byte[] readPages(int pageOffset) throws IOException;
        void writePage(int pageOffset, byte[] data) throws IOException;
        byte[] transceive(byte[] data) throws IOException;
    }

    private static final class AndroidMifareUltralightTransceiver implements Transceiver {
        private final MifareUltralight ultralight;

        AndroidMifareUltralightTransceiver(MifareUltralight ultralight) {
            this.ultralight = ultralight;
        }

        @Override
        public void connect() throws IOException {
            ultralight.connect();
        }

        @Override
        public void close() throws IOException {
            ultralight.close();
        }

        @Override
        public void setTimeout(int timeout) {
            ultralight.setTimeout(timeout);
        }

        @Override
        public int getTimeout() {
            return ultralight.getTimeout();
        }

        @Override
        public int getType() {
            return ultralight.getType();
        }

        @Override
        public byte[] getUid() {
            return ultralight.getTag() != null ? ultralight.getTag().getId() : null;
        }

        @Override
        public byte[] readPages(int pageOffset) throws IOException {
            return ultralight.readPages(pageOffset);
        }

        @Override
        public void writePage(int pageOffset, byte[] data) throws IOException {
            ultralight.writePage(pageOffset, data);
        }

        @Override
        public byte[] transceive(byte[] data) throws IOException {
            return ultralight.transceive(data);
        }
    }

    private static final int TIMEOUT = 500;

    /**
     * NXP MF0ICU1 Rev. 3.9 Section 7.5 (Table 5, Page 10): 1 page = 4 bytes;
     * Section 7.6.5 (Table 10, Page 16): {@code readPages()} reads 4 pages = 16 bytes.
     */
    private static final int PAGE_SIZE = 4;
    private static final int READ_PAGES_COUNT = 4;
    private static final int READ_CHUNK_SIZE = PAGE_SIZE * READ_PAGES_COUNT;

    // PC/SC Part 3 Rev 2.01.09 & ACS ACR122U V2.04 CLA=0xFF instructions
    private static final byte CLA_PCSC = (byte) 0xFF;
    private static final byte INS_DIRECT_TRANSMIT = (byte) 0x00;
    private static final byte INS_GET_DATA = (byte) 0xCA;
    private static final byte INS_READ_BINARY = (byte) 0xB0;
    private static final byte INS_UPDATE_BINARY = (byte) 0xD6;

    // Native MIFARE Ultralight commands (NXP MF0ICU1 Rev. 3.9 Section 7.6.5 Table 10, Section 7.6.7 Table 12, Section 7.6.8 Table 13)
    private static final byte CMD_READ = (byte) 0x30;
    private static final byte CMD_WRITE = (byte) 0xA2;
    private static final byte CMD_COMPATIBILITY_WRITE = (byte) 0xA0;

    // PC/SC Part 3 Supplemental Rev 2.01.09 Section 2.1.2.2 (Table 3) Registered Card Names
    private static final int CARD_NAME_ULTRALIGHT = 0x0003;
    private static final int CARD_NAME_ULTRALIGHT_C = 0x003A;

    // ISO/IEC 7816-4 / PC/SC Part 3 Status Words
    private static final byte[] SW_SUCCESS = new byte[]{(byte) 0x90, (byte) 0x00};
    private static final byte[] SW_EOF_BEFORE_LE = new byte[]{(byte) 0x62, (byte) 0x82};
    private static final byte[] SW_OPERATION_FAILED = new byte[]{(byte) 0x63, (byte) 0x00};
    private static final byte[] SW_WRONG_LENGTH = new byte[]{(byte) 0x67, (byte) 0x00};
    private static final byte[] SW_FUNC_NOT_SUPPORTED = new byte[]{(byte) 0x6A, (byte) 0x81};
    private static final byte[] SW_INCORRECT_P1P2 = new byte[]{(byte) 0x6A, (byte) 0x86};
    private static final byte[] SW_INS_NOT_SUPPORTED = new byte[]{(byte) 0x6D, (byte) 0x00};

    private final Transceiver card;

    public MifareUltralightReader(MifareUltralight sc, Activity activity) throws IOException {
        this(new AndroidMifareUltralightTransceiver(sc), activity);
    }

    MifareUltralightReader(Transceiver transceiver, Activity activity) throws IOException {
        super(activity);
        this.card = transceiver;
        this.card.connect();
        int timeout = TIMEOUT;
        if (activity != null) {
            SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(activity);
            timeout = Integer.parseInt(sp.getString("timeout", String.valueOf(TIMEOUT)));
        }
        this.card.setTimeout(timeout);
    }

    @Override
    public void eject() throws IOException {
        Log.i(this.getClass().getName(), "Disconnect MIFARE Ultralight card");
        try {
            card.close();
        } finally {
            super.eject();
        }
    }

    @Override
    public void powerOff() throws IOException {
        // PC/SC Part 3 Section 3.1.3.3: For ISO 14443-3 storage cards, power down needs no RF action
    }

    @Override
    public void reset() throws IOException {
        // No state to reset on MIFARE Ultralight warm reset
    }

    /**
     * Constructs the 20-byte PC/SC Part 3 Rev 2.01.09 Section 3.1.3.2.3.2 Contactless Storage Card ATR.
     *
     * <p>Structure (PC/SC Part 3 Table 3-6, Pages 21-22 &amp; ACS ACR122U V2.04 Section 3.1.1 Table 2, Pages 8-9):
     * {@code 3B 8F 80 01 80 4F 0C A0 00 00 03 06 03 [NN_MSB] [NN_LSB] 00 00 00 00 [TCK]}
     * <ul>
     *   <li>{@code SS = 0x03}: ISO 14443A, Part 3 (PC/SC Part 3 Supplemental Section 2.1.2.1 Table 2)</li>
     *   <li>{@code NN = 0x0003}: MIFARE Ultralight ({@link MifareUltralight#TYPE_ULTRALIGHT})</li>
     *   <li>{@code NN = 0x003A}: MIFARE Ultralight C ({@link MifareUltralight#TYPE_ULTRALIGHT_C})</li>
     * </ul>
     */
    @Override
    public byte[] getATR() {
        int cardName = (card.getType() == MifareUltralight.TYPE_ULTRALIGHT_C)
                ? CARD_NAME_ULTRALIGHT_C
                : CARD_NAME_ULTRALIGHT;

        byte[] atr = new byte[]{
                (byte) 0x3B, (byte) 0x8F, (byte) 0x80, (byte) 0x01,
                (byte) 0x80, (byte) 0x4F, (byte) 0x0C, (byte) 0xA0,
                (byte) 0x00, (byte) 0x00, (byte) 0x03, (byte) 0x06,
                (byte) 0x03,
                (byte) ((cardName >> 8) & 0xFF),
                (byte) (cardName & 0xFF),
                (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00,
                (byte) 0x00
        };
        byte tck = 0;
        for (int i = 1; i < atr.length - 1; i++) {
            tck ^= atr[i];
        }
        atr[atr.length - 1] = tck;
        Log.i(this.getClass().getName(), "ATR: " + Hex.getHexString(atr));
        return atr;
    }

    @Override
    public byte[] transmit(byte[] apdu) throws IOException {
        if (apdu == null || apdu.length == 0) {
            return SW_WRONG_LENGTH;
        }

        if (apdu[0] == CLA_PCSC) {
            return handlePcscCommand(apdu);
        }

        // ISO 7816-4 probing (e.g. SELECT MF: 00 A4 00 0C)
        if (apdu.length >= 2 && apdu[0] == 0x00 && apdu[1] == (byte) 0xA4) {
            return SW_SUCCESS;
        }

        return handleNativeCommand(apdu);
    }

    private byte[] handlePcscCommand(byte[] apdu) {
        if (apdu.length < 4) {
            return SW_WRONG_LENGTH;
        }
        byte ins = apdu[1];
        switch (ins) {
            case INS_GET_DATA:
                return handleGetData(apdu);
            case INS_READ_BINARY:
                return handleReadBinary(apdu);
            case INS_UPDATE_BINARY:
                return handleUpdateBinary(apdu);
            case INS_DIRECT_TRANSMIT:
                return handleDirectTransmit(apdu);
            default:
                return SW_INS_NOT_SUPPORTED;
        }
    }

    /**
     * PC/SC Part 3 Rev 2.01.09 Section 3.2.2.1.3 (Tables 3-8a, 3-8b, 3-9, Pages 26-27)
     * &amp; ACS ACR122U V2.04 Section 4.1 (Page 11):
     * {@code FF CA P1 P2 [Le]} - Get Data (UID for {@code P1=0x00, P2=0x00}).
     */
    private byte[] handleGetData(byte[] apdu) {
        if (apdu.length != 4 && apdu.length != 5) {
            return SW_WRONG_LENGTH;
        }
        int p1 = apdu[2] & 0xFF;
        int p2 = apdu[3] & 0xFF;
        int le = (apdu.length >= 5) ? (apdu[4] & 0xFF) : 0;

        if (p1 == 0x00 && p2 == 0x00) {
            byte[] uid = card.getUid();
            if (uid == null || uid.length == 0) {
                return SW_OPERATION_FAILED;
            }
            return buildGetDataResponse(uid, le);
        }
        if (p1 == 0x01 && p2 == 0x00) {
            // Storage cards do not have ISO 14443-4 ATS historical bytes (PC/SC Part 3 Table 3-9)
            return SW_FUNC_NOT_SUPPORTED;
        }
        return SW_INCORRECT_P1P2;
    }

    private byte[] buildGetDataResponse(byte[] data, int le) {
        if (le == 0 || le == data.length) {
            return appendStatusWord(data, SW_SUCCESS);
        }
        if (le < data.length) {
            return new byte[]{(byte) 0x6C, (byte) (data.length & 0xFF)};
        }
        byte[] padded = Arrays.copyOf(data, le);
        return appendStatusWord(padded, SW_EOF_BEFORE_LE);
    }

    /**
     * PC/SC Part 3 Rev 2.01.09 Section 3.2.2.1.8 (Tables 3-18a, 3-18b, 3-19, Page 31)
     * &amp; ACS ACR122U V2.04 Section 5.3 (Page 16):
     * {@code FF B0 00 [Page] [Le]} - Read Binary Blocks.
     *
     * <p>NXP MF0ICU1 Rev. 3.9 Section 7.6.5 (Table 10, Page 16): {@code readPages(pageOffset)}
     * returns 4 pages (16 bytes) starting from {@code pageOffset}. Standard single-page reads
     * specify {@code Le = 04h}, while 4-page reads specify {@code Le = 10h} (16 bytes) or {@code Le = 00h}.
     */
    private byte[] handleReadBinary(byte[] apdu) {
        if (apdu.length != 4 && apdu.length != 5) {
            return SW_WRONG_LENGTH;
        }
        int p1 = apdu[2] & 0xFF;
        int page = apdu[3] & 0xFF;
        int le = (apdu.length >= 5) ? (apdu[4] & 0xFF) : READ_CHUNK_SIZE;

        if (p1 != 0x00) {
            return SW_INCORRECT_P1P2;
        }

        int requestedBytes = (le == 0) ? READ_CHUNK_SIZE : le;
        int requestedPages = (requestedBytes + PAGE_SIZE - 1) / PAGE_SIZE;
        if (page + requestedPages > 256) {
            return SW_INCORRECT_P1P2;
        }
        ByteArrayOutputStream out = new ByteArrayOutputStream(requestedBytes);
        int currentPage = page;

        try {
            while (out.size() < requestedBytes) {
                byte[] chunk = card.readPages(currentPage);
                if (chunk == null || chunk.length == 0) {
                    return SW_OPERATION_FAILED;
                }
                int remaining = requestedBytes - out.size();
                int toCopy = Math.min(remaining, chunk.length);
                out.write(chunk, 0, toCopy);
                currentPage += READ_PAGES_COUNT;
            }
            return appendStatusWord(out.toByteArray(), SW_SUCCESS);
        } catch (IOException | RuntimeException e) {
            Log.w(this.getClass().getName(), "Read Binary failed at page " + page + ": " + e.getMessage());
            return SW_OPERATION_FAILED;
        }
    }

    /**
     * PC/SC Part 3 Rev 2.01.09 Section 3.2.2.1.9 (Tables 3-20a, 3-20b, 3-21, Page 32)
     * &amp; ACS ACR122U V2.04 Section 5.4 (Page 17):
     * {@code FF D6 00 [Page] [Lc] [Data]} - Update Binary Blocks.
     *
     * <p>NXP MF0ICU1 Rev. 3.9 Section 7.6.7 (Table 12, Page 17): Each MIFARE Ultralight page is 4 bytes.
     * Supports single-page writes ({@code Lc = 04h}) and multi-page writes ({@code Lc} multiple of 4).
     */
    private byte[] handleUpdateBinary(byte[] apdu) {
        if (apdu.length < 5) {
            return SW_WRONG_LENGTH;
        }
        int p1 = apdu[2] & 0xFF;
        int page = apdu[3] & 0xFF;
        int lc = apdu[4] & 0xFF;

        if (p1 != 0x00) {
            return SW_INCORRECT_P1P2;
        }
        if (lc == 0 || (lc % PAGE_SIZE) != 0 || apdu.length != 5 + lc) {
            return SW_WRONG_LENGTH;
        }

        int numPages = lc / PAGE_SIZE;
        if (page + numPages > 256) {
            return SW_INCORRECT_P1P2;
        }

        try {
            for (int offset = 0; offset < lc; offset += PAGE_SIZE) {
                byte[] pageData = Arrays.copyOfRange(apdu, 5 + offset, 5 + offset + PAGE_SIZE);
                card.writePage(page + (offset / PAGE_SIZE), pageData);
            }
            return SW_SUCCESS;
        } catch (IOException | RuntimeException e) {
            Log.w(this.getClass().getName(), "Update Binary failed at page " + page + ": " + e.getMessage());
            return SW_OPERATION_FAILED;
        }
    }

    /**
     * ACS ACR122U V2.04 Section 6.1 (Page 21):
     * {@code FF 00 00 00 Lc [Payload]} - Direct Transmit of raw MIFARE Ultralight / NFC-A frames.
     */
    private byte[] handleDirectTransmit(byte[] apdu) {
        if (apdu.length < 5) {
            return SW_WRONG_LENGTH;
        }
        int p1 = apdu[2] & 0xFF;
        int p2 = apdu[3] & 0xFF;
        int lc = apdu[4] & 0xFF;

        if (p1 != 0x00 || p2 != 0x00) {
            return SW_INCORRECT_P1P2;
        }
        if (lc == 0 || apdu.length != 5 + lc) {
            return SW_WRONG_LENGTH;
        }

        byte[] payload = Arrays.copyOfRange(apdu, 5, 5 + lc);
        try {
            byte[] response = card.transceive(payload);
            if (response == null) {
                return SW_OPERATION_FAILED;
            }
            return appendStatusWord(response, SW_SUCCESS);
        } catch (IOException | RuntimeException e) {
            Log.w(this.getClass().getName(), "Direct transmit failed: " + e.getMessage());
            return SW_OPERATION_FAILED;
        }
    }

    /**
     * Native MIFARE Ultralight command frame pass-through (used by {@code libfreefare}'s
     * {@code mifare_ultralight.c} and raw {@code vpcd} clients):
     * <ul>
     *   <li>{@code 30 [Page]} (2 bytes): READ 4 pages (16 bytes) starting at {@code Page}
     *       (NXP MF0ICU1 Rev. 3.9 Section 7.6.5 Table 10, Page 16).</li>
     *   <li>{@code A2 [Page] [4-byte Data]} (6 bytes): WRITE 1 page (4 bytes) to {@code Page}
     *       (NXP MF0ICU1 Rev. 3.9 Section 7.6.7 Table 12, Page 17).</li>
     *   <li>{@code A0 [Page] [16-byte Data]} (18 bytes): COMPATIBILITY WRITE least significant
     *       4 bytes to {@code Page} (NXP MF0ICU1 Rev. 3.9 Section 7.6.8 Table 13, Page 17).</li>
     *   <li>Other frames (e.g. Ultralight C {@code 1Ah} 3DES Auth): forwarded via {@code transceive()}.</li>
     * </ul>
     */
    private byte[] handleNativeCommand(byte[] apdu) {
        byte cmd = apdu[0];
        if (cmd == CMD_READ && apdu.length == 2) {
            int page = apdu[1] & 0xFF;
            try {
                return card.readPages(page);
            } catch (IOException | RuntimeException e) {
                Log.w(this.getClass().getName(), "Native READ failed at page " + page + ": " + e.getMessage());
                return SW_OPERATION_FAILED;
            }
        }
        if (cmd == CMD_WRITE && apdu.length == 2 + PAGE_SIZE) {
            int page = apdu[1] & 0xFF;
            byte[] pageData = Arrays.copyOfRange(apdu, 2, 2 + PAGE_SIZE);
            try {
                card.writePage(page, pageData);
                return SW_SUCCESS;
            } catch (IOException | RuntimeException e) {
                Log.w(this.getClass().getName(), "Native WRITE failed at page " + page + ": " + e.getMessage());
                return SW_OPERATION_FAILED;
            }
        }
        if (cmd == CMD_COMPATIBILITY_WRITE && apdu.length == 2 + READ_CHUNK_SIZE) {
            int page = apdu[1] & 0xFF;
            byte[] pageData = Arrays.copyOfRange(apdu, 2, 2 + PAGE_SIZE);
            try {
                card.writePage(page, pageData);
                return SW_SUCCESS;
            } catch (IOException | RuntimeException e) {
                Log.w(this.getClass().getName(), "Native COMPATIBILITY WRITE failed at page " + page + ": " + e.getMessage());
                return SW_OPERATION_FAILED;
            }
        }
        try {
            byte[] response = card.transceive(apdu);
            if (response == null) {
                return SW_OPERATION_FAILED;
            }
            return response;
        } catch (IOException | RuntimeException e) {
            int timeout = card.getTimeout();
            card.setTimeout(Math.min(timeout * 2, 4000));
            Log.w(this.getClass().getName(), "Native transceive failed: " + e.getMessage());
            return SW_OPERATION_FAILED;
        }
    }

    private static byte[] appendStatusWord(byte[] data, byte[] sw) {
        byte[] result = Arrays.copyOf(data, data.length + sw.length);
        System.arraycopy(sw, 0, result, data.length, sw.length);
        return result;
    }
}
