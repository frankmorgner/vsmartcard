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
import android.nfc.tech.NfcF;
import android.preference.PreferenceManager;
import android.util.Log;

import com.vsmartcard.remotesmartcardreader.app.Hex;

import java.io.IOException;
import java.util.Arrays;

/**
 * SCReader implementation for NFC-F / JIS X 6319-4 (FeliCa) tags.
 *
 * Supports PC/SC Storage Card commands and FeliCa/reader-specific conventions:
 * - GET DATA (FF CA 00 00): PC/SC Part 3 Section 3.2.2.1.3 (UID/IDm)
 * - GET DATA (FF CA 01 00 / 02 00): FeliCa-specific extensions for PMm and System Code
 * - READ BINARY (FF B0): PC/SC Part 3 Section 3.2.2.1.8, mapped by this implementation to FeliCa Read (0x06)
 * - UPDATE BINARY (FF D6): PC/SC Part 3 Section 3.2.2.1.9, mapped by this implementation to FeliCa Write (0x08)
 * - SELECT FILE / SERVICE (FF A4 00 01): Vendor-specific FeliCa/PC/SC convention used by Sony PaSoRi-oriented software
 * - Direct Transmit (FF 00 00 00): ACS ACR122U / ACR1252U Direct Transmit pseudo-APDU convention
 * - Native raw packet framing: JIS X 6319-4 Section 6.2 framing (defensive fallback for VPCD / non-APDU clients)
 */
public class NfcFReader extends NFCReader {

    public interface Transceiver {
        byte[] transceive(byte[] data) throws IOException;
        void close() throws IOException;
        void setTimeout(int timeout);
    }

    /**
     * Standard PC/SC Part 3 Contactless Storage Card ATR for FeliCa (JIS X 6319-4):
     * 3B 8F 80 01 80 4F 0C A0 00 00 03 06 11 00 3B 00 00 00 00 42
     */
    public static final byte[] FELICA_ATR = new byte[]{
            (byte) 0x3B, // Header TS
            (byte) 0x8F, // T0: 15 historical bytes
            (byte) 0x80, // TD1: T=0
            (byte) 0x01, // TD2: T=1
            // Historical Bytes (15 bytes, PC/SC Part 3 Supplemental Document):
            (byte) 0x80, // Category indicator (COMPACT-TLV)
            (byte) 0x4F, (byte) 0x0C, // Tag 4 (Application Identifier), Length 12
            (byte) 0xA0, (byte) 0x00, (byte) 0x00, (byte) 0x03, (byte) 0x06, // PC/SC RID
            (byte) 0x11, // SS: Standard (0x11 = FeliCa)
            (byte) 0x00, (byte) 0x3B, // NN: Card Name (0x003B = FeliCa)
            (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, // RFU (4 bytes)
            (byte) 0x42  // TCK checksum: XOR of bytes 1 to 18 = 0x42
    };

    /** Default service code: Transportation SF history (Suica, PASMO, ICOCA, etc.) */
    public static final int DEFAULT_SERVICE_CODE = 0x090F;

    private final Transceiver transceiver;
    private final byte[] idm = new byte[8];
    private final byte[] pmm = new byte[8];
    private final byte[] systemCode = new byte[2];
    private int selectedServiceCode = DEFAULT_SERVICE_CODE;

    public NfcFReader(final NfcF sc, Activity activity) throws IOException {
        this(createTransceiver(sc),
                extractTagId(sc),
                sc != null ? sc.getManufacturer() : null,
                sc != null ? sc.getSystemCode() : null,
                activity);
    }

    private static Transceiver createTransceiver(final NfcF sc) throws IOException {
        if (sc == null) {
            throw new IOException("NfcF tag is null");
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
        };
    }

    private static byte[] extractTagId(NfcF sc) {
        if (sc == null) return null;
        Tag tag = sc.getTag();
        return tag != null ? tag.getId() : null;
    }

    public NfcFReader(Transceiver transceiver, byte[] tagId, byte[] manufacturer, byte[] systemCode, Activity activity) throws IOException {
        super(activity);
        if (transceiver == null) {
            throw new IOException("Transceiver is null");
        }
        this.transceiver = transceiver;

        int timeout = 500;
        if (activity != null) {
            SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity);
            timeout = Integer.parseInt(SP.getString("timeout", "500"));
        }
        transceiver.setTimeout(timeout);
        com.example.android.common.logger.Log.i(getClass().getName(), "Timeout set to " + timeout);

        if (tagId != null && tagId.length == 8) {
            System.arraycopy(tagId, 0, this.idm, 0, 8);
        }
        if (manufacturer != null && manufacturer.length == 8) {
            System.arraycopy(manufacturer, 0, this.pmm, 0, 8);
        }
        if (systemCode != null && systemCode.length == 2) {
            System.arraycopy(systemCode, 0, this.systemCode, 0, 2);
        } else {
            this.systemCode[0] = (byte) 0x00;
            this.systemCode[1] = (byte) 0x03;
        }

        com.example.android.common.logger.Log.i(getClass().getName(),
                String.format("NfcF connected. IDm=%s, PMm=%s, SystemCode=0x%02X%02X",
                        Hex.getHexString(this.idm), Hex.getHexString(this.pmm),
                        this.systemCode[0] & 0xFF, this.systemCode[1] & 0xFF));
    }

    @Override
    public void eject() throws IOException {
        try {
            transceiver.close();
        } finally {
            super.eject();
        }
    }

    @Override
    public void powerOff() throws IOException {
        selectedServiceCode = DEFAULT_SERVICE_CODE;
    }

    @Override
    public void reset() throws IOException {
        selectedServiceCode = DEFAULT_SERVICE_CODE;
    }

    @Override
    public byte[] getATR() {
        return FELICA_ATR.clone();
    }

    @Override
    public byte[] transmit(byte[] apdu) throws IOException {
        if (apdu == null || apdu.length == 0) {
            return new byte[]{(byte) 0x67, 0x00};
        }

        // Direct Transmit: ACS ACR122U / ACR1252U pseudo-APDU convention (FF 00 00 00 Lc [Payload])
        if (apdu.length >= 4 && apdu[0] == (byte) 0xFF && apdu[1] == 0x00 && apdu[2] == 0x00 && apdu[3] == 0x00) {
            if (apdu.length < 5) {
                return new byte[]{(byte) 0x67, 0x00};
            }
            int lc = apdu[4] & 0xFF;
            if (lc == 0 || apdu.length < 5 + lc) {
                return new byte[]{(byte) 0x67, 0x00};
            }
            byte[] rawCmd = new byte[lc];
            System.arraycopy(apdu, 5, rawCmd, 0, lc);
            try {
                byte[] rawResp = transceiver.transceive(rawCmd);
                updateCachedIds(rawCmd, rawResp);
                return wrapStatus(rawResp, (byte) 0x90, (byte) 0x00);
            } catch (IOException e) {
                Log.w(getClass().getName(), "Transparent transmit failed: " + e.getMessage());
                return new byte[]{(byte) 0x6F, 0x00};
            }
        }

        // Native FeliCa packet framing (JIS X 6319-4 Section 6.2): byte 0 is LEN (information field length)
        // Handled as a defensive fallback for VPCD clients transmitting unencapsulated frames.
        if (apdu.length >= 2 && (apdu[0] & 0xFF) == apdu.length) {
            try {
                byte[] rawResp = transceiver.transceive(apdu);
                updateCachedIds(apdu, rawResp);
                return rawResp;
            } catch (IOException e) {
                Log.w(getClass().getName(), "Raw packet transmit failed: " + e.getMessage());
                return new byte[]{(byte) 0x6F, 0x00};
            }
        }

        // GET DATA: FF CA P1 P2 Le
        // - P1=0x00, P2=0x00: UID (IDm) per PC/SC Part 3 Section 3.2.2.1.3
        // - P1=0x01 (PMm) and P1=0x02 (System Code): FeliCa-specific extensions
        if (apdu.length >= 4 && apdu[0] == (byte) 0xFF && apdu[1] == (byte) 0xCA) {
            if (apdu[3] != 0x00) {
                return new byte[]{(byte) 0x6A, (byte) 0x86}; // Incorrect P1/P2 parameters
            }
            byte p1 = apdu[2];
            if (p1 == 0x00) {
                return wrapStatus(idm, (byte) 0x90, (byte) 0x00);
            } else if (p1 == 0x01) {
                return wrapStatus(pmm, (byte) 0x90, (byte) 0x00);
            } else if (p1 == 0x02) {
                return wrapStatus(systemCode, (byte) 0x90, (byte) 0x00);
            } else {
                return new byte[]{(byte) 0x6A, (byte) 0x81}; // Function not supported
            }
        }

        // SELECT FILE / SERVICE: FF A4 00 01 [Lc] [ServiceCode]
        // Vendor-specific FeliCa/PC/SC convention used by Sony PaSoRi-oriented software (not in PC/SC Part 3).
        // FeliCa cards partition memory by 16-bit Service Codes rather than ISO 7816-4 EF/DF files.
        // This command sets the active service for subsequent READ BINARY (FF B0) and UPDATE BINARY (FF D6).
        if (apdu.length >= 4 && apdu[0] == (byte) 0xFF && apdu[1] == (byte) 0xA4) {
            if (apdu.length < 7 || (apdu[4] & 0xFF) < 2) {
                return new byte[]{(byte) 0x67, 0x00};
            }
            int sc0 = apdu[5] & 0xFF;
            int sc1 = apdu[6] & 0xFF;
            selectedServiceCode = sc0 | (sc1 << 8);
            Log.d(getClass().getName(), String.format("Selected service code: 0x%04X", selectedServiceCode));
            return new byte[]{(byte) 0x90, 0x00};
        }

        // READ BINARY: FF B0 P1 P2 Le (PC/SC Part 3 Section 3.2.2.1.8)
        // Mapped by this implementation to FeliCa Read Without Encryption (0x06).
        if (apdu.length >= 4 && apdu[0] == (byte) 0xFF && apdu[1] == (byte) 0xB0) {
            int blockNumber = (apdu[3] & 0xFF) | ((apdu[2] & 0xFF) << 8);
            int le = (apdu.length >= 5) ? (apdu[4] & 0xFF) : 16;
            if (le == 0) {
                le = 16;
            }
            return readWithoutEncryption(blockNumber, le);
        }

        // UPDATE BINARY: FF D6 P1 P2 Lc [Data] (PC/SC Part 3 Section 3.2.2.1.9)
        // For FeliCa, this implementation writes a 16-byte block using FeliCa Write Without Encryption (0x08).
        if (apdu.length >= 5 && apdu[0] == (byte) 0xFF && apdu[1] == (byte) 0xD6) {
            int blockNumber = (apdu[3] & 0xFF) | ((apdu[2] & 0xFF) << 8);
            int lc = apdu[4] & 0xFF;
            if (lc < 16 || apdu.length < 5 + lc) {
                return new byte[]{(byte) 0x67, 0x00};
            }
            byte[] blockData = new byte[16];
            System.arraycopy(apdu, 5, blockData, 0, 16);
            return writeWithoutEncryption(blockNumber, blockData);
        }

        // ISO 7816-4 probing (e.g. SELECT MF: 00 A4 00 0C)
        if (apdu.length >= 2 && apdu[0] == 0x00 && apdu[1] == (byte) 0xA4) {
            return new byte[]{(byte) 0x90, 0x00};
        }

        return new byte[]{(byte) 0x6D, 0x00}; // Instruction code not supported
    }

    private byte[] readWithoutEncryption(int startBlock, int le) {
        if (startBlock < 0 || startBlock > 0xFF) {
            return new byte[]{(byte) 0x6A, (byte) 0x82}; // File / Block not found
        }
        int numBlocks = Math.max(1, (le + 15) / 16);
        if (numBlocks > 16) {
            numBlocks = 16; // JIS X 6319-4 allows up to 16 blocks per read command
        }
        if (startBlock + numBlocks - 1 > 0xFF) {
            return new byte[]{(byte) 0x6A, (byte) 0x82}; // Block range exceeds 1-byte address space
        }

        // JIS X 6319-4 Section 10.5.1 Read Without Encryption command packet:
        // Length: 14 + 2 * numBlocks
        // Command Code: 0x06
        // IDm: 8 bytes
        // Number of Services: 0x01
        // Service Code: 2 bytes (Little Endian)
        // Number of Blocks: 1 byte
        // Block List: 2 bytes per block (0x80 | 2-byte format, blockNumber)
        byte[] request = new byte[14 + 2 * numBlocks];
        request[0] = (byte) request.length;
        request[1] = 0x06;
        System.arraycopy(idm, 0, request, 2, 8);
        request[10] = 0x01;
        request[11] = (byte) (selectedServiceCode & 0xFF);
        request[12] = (byte) ((selectedServiceCode >> 8) & 0xFF);
        request[13] = (byte) numBlocks;
        for (int i = 0; i < numBlocks; i++) {
            request[14 + 2 * i] = (byte) 0x80; // 2-byte block list element for service 0
            request[14 + 2 * i + 1] = (byte) ((startBlock + i) & 0xFF);
        }

        byte[] response;
        try {
            response = transceiver.transceive(request);
        } catch (IOException e) {
            Log.w(getClass().getName(), "transceive failed during readWithoutEncryption (block " + startBlock + "): " + e.getMessage());
            // Graceful fallback for PC/SC reader tools (e.g., TransiticViewer) probing unwritten history blocks
            byte[] fallback = new byte[le + 2];
            fallback[le] = (byte) 0x90;
            fallback[le + 1] = (byte) 0x00;
            return fallback;
        }

        // Response format:
        // Byte 0: Length (>= 13 bytes)
        // Byte 1: Response Code (0x07)
        // Bytes 2..9: IDm
        // Byte 10: Status Flag 1 (SF1)
        // Byte 11: Status Flag 2 (SF2)
        // Byte 12: Number of Blocks
        // Bytes 13..: 16 * numBlocks bytes block data
        if (response != null && response.length >= 13 && response[1] == 0x07) {
            byte sf1 = response[10];
            byte sf2 = response[11];
            if (sf1 == 0x00 && sf2 == 0x00) {
                int blockDataLen = response.length - 13;
                int returnLen = Math.min(le, blockDataLen);
                byte[] result = new byte[returnLen + 2];
                System.arraycopy(response, 13, result, 0, returnLen);
                result[returnLen] = (byte) 0x90;
                result[returnLen + 1] = (byte) 0x00;
                return result;
            } else {
                Log.d(getClass().getName(), String.format("FeliCa block %d unavailable (SF1=0x%02X, SF2=0x%02X)", startBlock, sf1, sf2));
                byte[] emptyResult = new byte[le + 2];
                emptyResult[le] = (byte) 0x90;
                emptyResult[le + 1] = (byte) 0x00;
                return emptyResult;
            }
        }

        // Fallback for unexpected response structure
        byte[] fallback = new byte[le + 2];
        fallback[le] = (byte) 0x90;
        fallback[le + 1] = (byte) 0x00;
        return fallback;
    }

    private byte[] writeWithoutEncryption(int blockNumber, byte[] blockData) {
        if (blockNumber < 0 || blockNumber > 0xFF) {
            return new byte[]{(byte) 0x6A, (byte) 0x82}; // File / Block not found
        }
        if (blockData == null || blockData.length < 16) {
            return new byte[]{(byte) 0x67, 0x00}; // Wrong length
        }

        // JIS X 6319-4 Section 10.6 Write command packet:
        // Length: 32 bytes (14 + 2 + 16)
        // Command Code: 0x08
        // IDm: 8 bytes
        // Number of Services: 0x01
        // Service Code: 2 bytes
        // Number of Blocks: 0x01
        // Block List: 2 bytes (0x80, blockNumber)
        // Block Data: 16 bytes
        byte[] request = new byte[32];
        request[0] = 32;
        request[1] = 0x08;
        System.arraycopy(idm, 0, request, 2, 8);
        request[10] = 0x01;
        request[11] = (byte) (selectedServiceCode & 0xFF);
        request[12] = (byte) ((selectedServiceCode >> 8) & 0xFF);
        request[13] = 0x01;
        request[14] = (byte) 0x80;
        request[15] = (byte) (blockNumber & 0xFF);
        System.arraycopy(blockData, 0, request, 16, 16);

        byte[] response;
        try {
            response = transceiver.transceive(request);
        } catch (IOException e) {
            Log.w(getClass().getName(), "transceive failed during writeWithoutEncryption: " + e.getMessage());
            return new byte[]{(byte) 0x6F, 0x00};
        }

        if (response != null && response.length >= 12 && response[1] == 0x09) {
            byte sf1 = response[10];
            byte sf2 = response[11];
            if (sf1 == 0x00 && sf2 == 0x00) {
                return new byte[]{(byte) 0x90, 0x00};
            } else {
                return new byte[]{(byte) 0x6F, sf1};
            }
        }
        return new byte[]{(byte) 0x6F, 0x00};
    }

    private void updateCachedIds(byte[] request, byte[] response) {
        if (response != null && response.length >= 18 && response[1] == 0x01) {
            // Polling response: IDm at bytes 2..9, PMm at bytes 10..17
            System.arraycopy(response, 2, this.idm, 0, 8);
            System.arraycopy(response, 10, this.pmm, 0, 8);
            if (request != null && request.length >= 4) {
                this.systemCode[0] = request[2];
                this.systemCode[1] = request[3];
            }
        }
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

    public byte[] getIdm() {
        return idm.clone();
    }

    public byte[] getPmm() {
        return pmm.clone();
    }

    public byte[] getSystemCode() {
        return systemCode.clone();
    }

    public int getSelectedServiceCode() {
        return selectedServiceCode;
    }
}
