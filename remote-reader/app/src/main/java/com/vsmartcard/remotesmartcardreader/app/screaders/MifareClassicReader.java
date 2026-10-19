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
import android.nfc.tech.MifareClassic;
import android.preference.PreferenceManager;
import android.util.Log;

import com.vsmartcard.remotesmartcardreader.app.Hex;

import java.io.IOException;
import java.util.Arrays;

/**
 * SCReader implementation for MIFARE Classic (1K, 4K, Mini, 2K) contactless storage cards.
 *
 * Implements PC/SC Part 3 Storage Card commands, ACS ACR122U T=CL emulation commands,
 * and native NXP MIFARE Classic frames (for tools such as libfreefare):
 * - ATR Generation: PC/SC Part 3 Rev 2.01.09 Section 3.1.3.2.3.2 (Table 3-6),
 *   PC/SC Part 3 Supplemental Document Section 2.1.2.2 (Table 3), and ACS ACR122U API V2.04 Section 3.1.1 (Table 2)
 * - GET DATA (FF CA 00 00): PC/SC Part 3 Section 3.2.2.1.3 & ACS ACR122U API V2.04 Section 4.1
 * - LOAD KEYS (FF 82): PC/SC Part 3 Section 3.2.2.1.4 & ACS ACR122U API V2.04 Section 5.1
 * - GENERAL AUTHENTICATE (FF 86) & obsolete AUTHENTICATE (FF 88):
 *   PC/SC Part 3 Section 3.2.2.1.5-3.2.2.1.6 & ACS ACR122U API V2.04 Section 5.2
 * - READ BINARY (FF B0): PC/SC Part 3 Section 3.2.2.1.8 & ACS ACR122U API V2.04 Section 5.3
 * - UPDATE BINARY (FF D6): PC/SC Part 3 Section 3.2.2.1.9 & ACS ACR122U API V2.04 Section 5.4
 * - VALUE BLOCK OPERATION / RESTORE (FF D7) & READ VALUE BLOCK (FF B1):
 *   ACS ACR122U API V2.04 Section 5.5.1-5.5.3 & NXP MF1S50YYX_V1 Rev. 3.2 Section 8.6.2.1, 12.4-12.5
 * - DIRECT TRANSMIT (FF 00 00 00): ACS ACR122U API V2.04 Section 6.1
 * - Native MIFARE Classic frames (60/61, 30, A0, C0, C1, C2, B0):
 *   NXP MF1S50YYX_V1 Rev. 3.2 Section 9.1 (Table 9), Section 12.1-12.5 & libfreefare (mifare_classic.c)
 */
public class MifareClassicReader extends NFCReader {

    public interface Transceiver {
        boolean authenticateSectorWithKeyA(int sectorIndex, byte[] key) throws IOException;
        boolean authenticateSectorWithKeyB(int sectorIndex, byte[] key) throws IOException;
        byte[] readBlock(int blockIndex) throws IOException;
        void writeBlock(int blockIndex, byte[] data) throws IOException;
        void increment(int blockIndex, int value) throws IOException;
        void decrement(int blockIndex, int value) throws IOException;
        void restore(int blockIndex) throws IOException;
        void transfer(int blockIndex) throws IOException;
        int blockToSector(int blockIndex);
        int getBlockCount();
        int getType();
        int getSize();
        byte[] transceive(byte[] data) throws IOException;
        void close() throws IOException;
        void setTimeout(int timeout);
    }

    /** Number of volatile key storage slots in the emulated IFD */
    public static final int MAX_KEY_SLOTS = 32;

    /** MIFARE Classic key length (6 bytes, NXP MF1S50YYX_V1 Section 8.6.3) */
    public static final int KEY_LENGTH = 6;

    /** MIFARE Classic block size (16 bytes, NXP MF1S50YYX_V1 Section 8.6.2) */
    public static final int BLOCK_SIZE = 16;

    /** Native MIFARE Classic command codes (NXP MF1S50YYX_V1 Section 9.1 Table 9) */
    public static final byte CMD_AUTH_KEY_A = (byte) 0x60;
    public static final byte CMD_AUTH_KEY_B = (byte) 0x61;
    public static final byte CMD_READ = (byte) 0x30;
    public static final byte CMD_WRITE = (byte) 0xA0;
    public static final byte CMD_TRANSFER = (byte) 0xB0;
    public static final byte CMD_DECREMENT = (byte) 0xC0;
    public static final byte CMD_INCREMENT = (byte) 0xC1;
    public static final byte CMD_RESTORE = (byte) 0xC2;

    /** Native MIFARE Classic ACK value (NXP MF1S50YYX_V1 Section 9.3 Table 10) */
    public static final byte MC_ACK = (byte) 0x0A;

    /** PC/SC Part 3 Supplemental Section 2.1.2.2 Card Name identifiers (NN) */
    public static final int CARD_NAME_MIFARE_1K = 0x0001;
    public static final int CARD_NAME_MIFARE_4K = 0x0002;
    public static final int CARD_NAME_MIFARE_MINI = 0x0026;
    public static final int CARD_NAME_MIFARE_PLUS_2K = 0x0036;
    public static final int CARD_NAME_MIFARE_PLUS_4K = 0x0037;

    private final Transceiver transceiver;
    private final byte[] uid;
    private final byte[][] keyStore = new byte[MAX_KEY_SLOTS][KEY_LENGTH];

    public MifareClassicReader(final MifareClassic sc, Activity activity) throws IOException {
        this(createTransceiver(sc), extractTagId(sc), activity);
    }

    private static Transceiver createTransceiver(final MifareClassic sc) throws IOException {
        if (sc == null) {
            throw new IOException("MifareClassic tag is null");
        }
        sc.connect();
        return new Transceiver() {
            @Override
            public boolean authenticateSectorWithKeyA(int sectorIndex, byte[] key) throws IOException {
                return sc.authenticateSectorWithKeyA(sectorIndex, key);
            }

            @Override
            public boolean authenticateSectorWithKeyB(int sectorIndex, byte[] key) throws IOException {
                return sc.authenticateSectorWithKeyB(sectorIndex, key);
            }

            @Override
            public byte[] readBlock(int blockIndex) throws IOException {
                return sc.readBlock(blockIndex);
            }

            @Override
            public void writeBlock(int blockIndex, byte[] data) throws IOException {
                sc.writeBlock(blockIndex, data);
            }

            @Override
            public void increment(int blockIndex, int value) throws IOException {
                sc.increment(blockIndex, value);
            }

            @Override
            public void decrement(int blockIndex, int value) throws IOException {
                sc.decrement(blockIndex, value);
            }

            @Override
            public void restore(int blockIndex) throws IOException {
                sc.restore(blockIndex);
            }

            @Override
            public void transfer(int blockIndex) throws IOException {
                sc.transfer(blockIndex);
            }

            @Override
            public int blockToSector(int blockIndex) {
                return sc.blockToSector(blockIndex);
            }

            @Override
            public int getBlockCount() {
                return sc.getBlockCount();
            }

            @Override
            public int getType() {
                return sc.getType();
            }

            @Override
            public int getSize() {
                return sc.getSize();
            }

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

    private static byte[] extractTagId(MifareClassic sc) {
        if (sc == null) return null;
        Tag tag = sc.getTag();
        return tag != null ? tag.getId() : null;
    }

    public MifareClassicReader(Transceiver transceiver, byte[] tagId, Activity activity) throws IOException {
        super(activity);
        if (transceiver == null) {
            throw new IOException("Transceiver is null");
        }
        this.transceiver = transceiver;
        this.uid = tagId != null ? tagId.clone() : new byte[0];

        int timeout = 500;
        if (activity != null) {
            SharedPreferences SP = PreferenceManager.getDefaultSharedPreferences(activity);
            timeout = Integer.parseInt(SP.getString("timeout", "500"));
        }
        transceiver.setTimeout(timeout);
        com.example.android.common.logger.Log.i(getClass().getName(), "Timeout set to " + timeout);

        resetKeys();

        com.example.android.common.logger.Log.i(getClass().getName(),
                String.format("MifareClassic connected. UID=%s, Size=%d bytes",
                        Hex.getHexString(this.uid), transceiver.getSize()));
    }

    private void resetKeys() {
        // Implementation convenience default: initialize all volatile reader key slots (0x00..0x1F)
        // to FF FF FF FF FF FF. NXP MF1S50YYX_V1 Section 8.6.3 specifies that all sector trailer
        // keys on physical card EEPROM are set to FFFF FFFF FFFFh at chip delivery.
        // Initializing the emulator's volatile key slots with this value matches ACS ACR122U
        // EEPROM default behavior (ACR122U API Section 5.1) and allows immediate authentication
        // against factory-fresh cards without an upfront LOAD KEYS (FF 82) APDU.
        // Note: PC/SC Part 3 Section 3.2.2.1.4 does not mandate specific initial key slot values.
        for (byte[] slot : keyStore) {
            Arrays.fill(slot, (byte) 0xFF);
        }
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
        // PC/SC Part 3 Section 3.1.3.3: For ISO 14443-3 storage cards, power down needs no RF action
    }

    @Override
    public void reset() throws IOException {
        // Preserve loaded keys across warm reset; sector re-authentication is handled by client APDUs
    }

    /**
     * Generates a 20-byte PC/SC Part 3 Contactless Storage Card ATR.
     *
     * Format (PC/SC Part 3 Section 3.1.3.2.3.2 Table 3-6, PC/SC Part 3 Supplemental Section 2.1.2,
     * and ACS ACR122U API V2.04 Section 3.1.1 Table 2):
     * 3B 8F 80 01 80 4F 0C A0 00 00 03 06 03 [C0] [C1] 00 00 00 00 [TCK]
     */
    @Override
    public byte[] getATR() {
        int cardName;
        int size = transceiver.getSize();
        int type = transceiver.getType();
        if (type == MifareClassic.TYPE_PLUS) {
            cardName = (size == MifareClassic.SIZE_4K) ? CARD_NAME_MIFARE_PLUS_4K : CARD_NAME_MIFARE_PLUS_2K;
        } else if (size == MifareClassic.SIZE_4K) {
            cardName = CARD_NAME_MIFARE_4K;
        } else if (size == MifareClassic.SIZE_MINI) {
            cardName = CARD_NAME_MIFARE_MINI;
        } else if (size == MifareClassic.SIZE_2K) {
            // NXP never manufactured a native Classic 2K chip; 2K memory in Classic framing is
            // exclusively MIFARE Plus (MF1PLUS60) in Security Level 1 (SAK 0x10).
            // If Android returns TYPE_CLASSIC or TYPE_UNKNOWN for a 2K card (e.g. non-standard clone
            // chips or custom SAK), we still map to CARD_NAME_MIFARE_PLUS_2K (0x0036) because
            // PC/SC Part 3 Supplemental Table 3 does not define any "MIFARE Classic 2K" identifier.
            cardName = CARD_NAME_MIFARE_PLUS_2K;
        } else {
            cardName = CARD_NAME_MIFARE_1K;
        }
        return buildStorageCardAtr((byte) 0x03, cardName);
    }

    static byte[] buildStorageCardAtr(byte standardByte, int cardName) {
        byte[] atr = new byte[]{
                (byte) 0x3B, // TS: Initial Header
                (byte) 0x8F, // T0: TD1 follows, 15 historical bytes
                (byte) 0x80, // TD1: TD2 follows, T=0
                (byte) 0x01, // TD2: T=1
                (byte) 0x80, // T1: Category indicator (COMPACT-TLV)
                (byte) 0x4F, // Application Identifier Presence Indicator
                (byte) 0x0C, // Length = 12 bytes
                (byte) 0xA0, (byte) 0x00, (byte) 0x00, (byte) 0x03, (byte) 0x06, // PC/SC RID
                standardByte, // SS: Standard (0x03 = ISO 14443A Part 3)
                (byte) ((cardName >> 8) & 0xFF), // C0 (NN MSB)
                (byte) (cardName & 0xFF),        // C1 (NN LSB)
                (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, // RFU (4 bytes)
                (byte) 0x00  // TCK placeholder
        };
        byte tck = atr[1];
        for (int i = 2; i < atr.length - 1; i++) {
            tck ^= atr[i];
        }
        atr[atr.length - 1] = tck;
        return atr;
    }

    @Override
    public byte[] transmit(byte[] apdu) throws IOException {
        if (apdu == null || apdu.length == 0) {
            return new byte[]{(byte) 0x67, 0x00};
        }

        // 1. PC/SC & ACS ACR122U CLA=0xFF commands
        if (apdu[0] == (byte) 0xFF) {
            if (apdu.length < 4) {
                return new byte[]{(byte) 0x67, 0x00};
            }
            byte ins = apdu[1];

            // Direct Transmit: FF 00 00 00 Lc [Payload] (ACS ACR122U API V2.04 Section 6.1)
            if (ins == 0x00) {
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

            // GET DATA: FF CA P1 P2 [Le] (PC/SC Part 3 Section 3.2.2.1.3 & ACR122U Section 4.1)
            if (ins == (byte) 0xCA) {
                if (apdu.length != 4 && apdu.length != 5) {
                    return new byte[]{(byte) 0x67, 0x00};
                }
                if (apdu[3] != 0x00) {
                    return new byte[]{(byte) 0x6A, (byte) 0x86};
                }
                if (apdu[2] == 0x00) {
                    int le = (apdu.length >= 5) ? (apdu[4] & 0xFF) : 0;
                    return IsoDepReader.formatGetDataResponse(uid, le);
                }
                return new byte[]{(byte) 0x6A, (byte) 0x81}; // Function not supported (e.g. P1=0x01 ATS on ISO 14443-3 card)
            }

            // LOAD AUTHENTICATION KEYS: FF 82 [KeyStructure] [KeyNumber] 06 [6-byte Key]
            // PC/SC Part 3 Section 3.2.2.1.4 & ACS ACR122U API V2.04 Section 5.1
            if (ins == (byte) 0x82) {
                return handleLoadKeys(apdu);
            }

            // GENERAL AUTHENTICATE: FF 86 00 00 05 [01 AddrMSB AddrLSB KeyType KeyNumber]
            // PC/SC Part 3 Section 3.2.2.1.6 & ACS ACR122U API V2.04 Section 5.2
            if (ins == (byte) 0x86) {
                return handleGeneralAuthenticate(apdu);
            }

            // Obsolete AUTHENTICATE: FF 88 [AddrMSB] [BlockNumber] [KeyType] [KeyNumber]
            // PC/SC Part 3 Section 3.2.2.1.5 & ACS ACR122U API V2.04 Section 5.2
            if (ins == (byte) 0x88) {
                return handleLegacyAuthenticate(apdu);
            }

            // READ BINARY: FF B0 [AddrMSB] [BlockNumber] [Le]
            // PC/SC Part 3 Section 3.2.2.1.8 & ACS ACR122U API V2.04 Section 5.3
            if (ins == (byte) 0xB0) {
                return handleReadBinary(apdu);
            }

            // UPDATE BINARY: FF D6 [AddrMSB] [BlockNumber] [Lc] [Data]
            // PC/SC Part 3 Section 3.2.2.1.9 & ACS ACR122U API V2.04 Section 5.4
            if (ins == (byte) 0xD6) {
                return handleUpdateBinary(apdu);
            }

            // VALUE BLOCK OPERATION / RESTORE VALUE BLOCK: FF D7 00 [Block] [Lc] ...
            // ACS ACR122U API V2.04 Section 5.5.1 & Section 5.5.3
            if (ins == (byte) 0xD7) {
                return handleValueBlockOperation(apdu);
            }

            // READ VALUE BLOCK: FF B1 00 [Block] [Le]
            // ACS ACR122U API V2.04 Section 5.5.2
            if (ins == (byte) 0xB1) {
                return handleReadValueBlock(apdu);
            }

            return new byte[]{(byte) 0x6D, 0x00}; // Instruction code not supported
        }

        // 2. ISO 7816-4 probing (e.g. SELECT MF: 00 A4 00 0C)
        if (apdu.length >= 2 && apdu[0] == 0x00 && apdu[1] == (byte) 0xA4) {
            return new byte[]{(byte) 0x90, 0x00};
        }

        // 3. Native MIFARE Classic command frames (NXP MF1S50YYX_V1 Section 9.1 Table 9 & libfreefare)
        return handleNativeMifareCommand(apdu);
    }

    private byte[] handleLoadKeys(byte[] apdu) {
        if (apdu.length < 5) {
            return new byte[]{(byte) 0x67, 0x00};
        }
        int p1 = apdu[2] & 0xFF;
        int keyNumber = apdu[3] & 0xFF;
        int lc = apdu[4] & 0xFF;

        // PC/SC Part 3 Section 3.2.2.1.4 Table 3-11 & Table 3-12:
        // b7=1: Reader Key (not supported -> 69 83)
        if ((p1 & 0x80) != 0) {
            return new byte[]{(byte) 0x69, (byte) 0x83};
        }
        // b6=1: Secured/encrypted transmission (not supported -> 69 85)
        if ((p1 & 0x40) != 0) {
            return new byte[]{(byte) 0x69, (byte) 0x85};
        }
        // b5=1: Non-volatile memory (not available -> 69 87)
        if ((p1 & 0x20) != 0) {
            return new byte[]{(byte) 0x69, (byte) 0x87};
        }
        if (keyNumber >= MAX_KEY_SLOTS) {
            return new byte[]{(byte) 0x69, (byte) 0x88}; // Key number not valid
        }
        if (lc != KEY_LENGTH) {
            return new byte[]{(byte) 0x69, (byte) 0x89}; // Key length is not correct
        }
        if (apdu.length != 5 + KEY_LENGTH) {
            return new byte[]{(byte) 0x67, 0x00};
        }

        System.arraycopy(apdu, 5, keyStore[keyNumber], 0, KEY_LENGTH);
        return new byte[]{(byte) 0x90, 0x00};
    }

    private byte[] handleGeneralAuthenticate(byte[] apdu) {
        if (apdu[2] != 0x00 || apdu[3] != 0x00) {
            return new byte[]{(byte) 0x6A, (byte) 0x86};
        }
        if (apdu.length != 10 || (apdu[4] & 0xFF) != 5) {
            return new byte[]{(byte) 0x67, 0x00};
        }
        int version = apdu[5] & 0xFF;
        if (version != 0x01) {
            return new byte[]{(byte) 0x6A, (byte) 0x80};
        }
        int blockNumber = ((apdu[6] & 0xFF) << 8) | (apdu[7] & 0xFF);
        int keyType = apdu[8] & 0xFF;
        int keyNumber = apdu[9] & 0xFF;
        return authenticateBlock(blockNumber, keyType, keyNumber);
    }

    private byte[] handleLegacyAuthenticate(byte[] apdu) {
        if (apdu.length != 6) {
            return new byte[]{(byte) 0x67, 0x00};
        }
        int blockNumber = ((apdu[2] & 0xFF) << 8) | (apdu[3] & 0xFF);
        int keyType = apdu[4] & 0xFF;
        int keyNumber = apdu[5] & 0xFF;
        return authenticateBlock(blockNumber, keyType, keyNumber);
    }

    private byte[] authenticateBlock(int blockNumber, int keyType, int keyNumber) {
        // PC/SC Part 3 Section 3.2.2.1.6 Table 3-14 error codes & ACR122U Section 5.2
        if (keyType != (CMD_AUTH_KEY_A & 0xFF) && keyType != (CMD_AUTH_KEY_B & 0xFF)) {
            return new byte[]{(byte) 0x69, (byte) 0x86}; // Key type not known
        }
        if (keyNumber < 0 || keyNumber >= MAX_KEY_SLOTS) {
            return new byte[]{(byte) 0x69, (byte) 0x88}; // Key number not valid
        }
        if (blockNumber < 0 || blockNumber >= transceiver.getBlockCount()) {
            return new byte[]{(byte) 0x65, (byte) 0x81}; // Memory failure / block does not exist
        }

        int sector = transceiver.blockToSector(blockNumber);
        byte[] key = keyStore[keyNumber];
        try {
            boolean authenticated;
            if (keyType == (CMD_AUTH_KEY_A & 0xFF)) {
                authenticated = transceiver.authenticateSectorWithKeyA(sector, key);
            } else {
                authenticated = transceiver.authenticateSectorWithKeyB(sector, key);
            }
            if (authenticated) {
                return new byte[]{(byte) 0x90, 0x00};
            }
        } catch (IOException e) {
            Log.w(getClass().getName(), "Authentication failed for sector " + sector + ": " + e.getMessage());
        }
        return new byte[]{(byte) 0x63, 0x00};
    }

    private byte[] handleReadBinary(byte[] apdu) {
        if (apdu.length != 4 && apdu.length != 5) {
            return new byte[]{(byte) 0x67, 0x00};
        }
        int blockNumber = ((apdu[2] & 0xFF) << 8) | (apdu[3] & 0xFF);
        int le = (apdu.length >= 5) ? (apdu[4] & 0xFF) : BLOCK_SIZE;
        if (le == 0) {
            le = BLOCK_SIZE;
        }
        if (blockNumber < 0 || blockNumber >= transceiver.getBlockCount()) {
            return new byte[]{(byte) 0x6A, (byte) 0x82}; // Addressed block does not exist
        }
        int numBlocks = (le + BLOCK_SIZE - 1) / BLOCK_SIZE;
        if (blockNumber + numBlocks > transceiver.getBlockCount()) {
            return new byte[]{(byte) 0x6A, (byte) 0x82};
        }
        // MIFARE Classic authentication is sector-bound (NXP MF1S50YYX_V1 §8.6); reading across sectors cannot succeed
        if (transceiver.blockToSector(blockNumber) != transceiver.blockToSector(blockNumber + numBlocks - 1)) {
            return new byte[]{(byte) 0x63, 0x00};
        }

        byte[] buffer = new byte[numBlocks * BLOCK_SIZE];
        try {
            for (int i = 0; i < numBlocks; i++) {
                byte[] block = transceiver.readBlock(blockNumber + i);
                if (block == null || block.length < BLOCK_SIZE) {
                    return new byte[]{(byte) 0x63, 0x00};
                }
                System.arraycopy(block, 0, buffer, i * BLOCK_SIZE, BLOCK_SIZE);
            }
        } catch (IOException e) {
            Log.w(getClass().getName(), "readBlock failed at block " + blockNumber + ": " + e.getMessage());
            return new byte[]{(byte) 0x63, 0x00};
        }

        byte[] result = new byte[le + 2];
        System.arraycopy(buffer, 0, result, 0, le);
        result[le] = (byte) 0x90;
        result[le + 1] = (byte) 0x00;
        return result;
    }

    private byte[] handleUpdateBinary(byte[] apdu) {
        if (apdu.length < 5) {
            return new byte[]{(byte) 0x67, 0x00};
        }
        int blockNumber = ((apdu[2] & 0xFF) << 8) | (apdu[3] & 0xFF);
        int lc = apdu[4] & 0xFF;
        // PC/SC Part 3 Section 3.2.2.1.9 & ACS ACR122U API V2.04 Section 5.4:
        // Lc must be exactly 10h (16 bytes, 1 block) for MIFARE Classic.
        // Multi-block writes are disallowed to prevent partial writes across sector boundaries.
        if (lc != BLOCK_SIZE || apdu.length != 5 + BLOCK_SIZE) {
            return new byte[]{(byte) 0x67, 0x00};
        }
        if (blockNumber < 0 || blockNumber >= transceiver.getBlockCount()) {
            return new byte[]{(byte) 0x6A, (byte) 0x82};
        }

        byte[] blockData = new byte[BLOCK_SIZE];
        System.arraycopy(apdu, 5, blockData, 0, BLOCK_SIZE);
        try {
            transceiver.writeBlock(blockNumber, blockData);
            return new byte[]{(byte) 0x90, 0x00};
        } catch (IOException e) {
            Log.w(getClass().getName(), "writeBlock failed at block " + blockNumber + ": " + e.getMessage());
            return new byte[]{(byte) 0x63, 0x00};
        }
    }

    private byte[] handleValueBlockOperation(byte[] apdu) {
        if (apdu.length < 5) {
            return new byte[]{(byte) 0x67, 0x00};
        }
        if (apdu[2] != 0x00) {
            return new byte[]{(byte) 0x6A, (byte) 0x86};
        }
        int blockNumber = apdu[3] & 0xFF;
        int lc = apdu[4] & 0xFF;
        if (blockNumber >= transceiver.getBlockCount()) {
            return new byte[]{(byte) 0x6A, (byte) 0x82};
        }
        // ACR122U Section 5.5.1: Value block operations apply to data blocks only (not Manufacturer block 0 or Sector Trailers)
        if (blockNumber == 0 || isSectorTrailer(blockNumber)) {
            return new byte[]{(byte) 0x69, (byte) 0x86}; // Command not allowed
        }

        // ACS ACR122U API V2.04 Section 5.5.3: Restore Value Block
        // Format: FF D7 00 [SourceBlock] 02 03 [TargetBlock] (7 bytes)
        if (lc == 0x02 && apdu.length == 7) {
            if (apdu[5] != 0x03) {
                return new byte[]{(byte) 0x6A, (byte) 0x81}; // Function not supported
            }
            int targetBlock = apdu[6] & 0xFF;
            if (targetBlock >= transceiver.getBlockCount()) {
                return new byte[]{(byte) 0x6A, (byte) 0x82};
            }
            if (targetBlock == 0 || isSectorTrailer(targetBlock)) {
                return new byte[]{(byte) 0x69, (byte) 0x86}; // Command not allowed
            }
            if (transceiver.blockToSector(blockNumber) != transceiver.blockToSector(targetBlock)) {
                // ACR122U Section 5.5.3: Source and target blocks must reside in the same sector
                return new byte[]{(byte) 0x63, 0x00};
            }
            try {
                transceiver.restore(blockNumber);
                transceiver.transfer(targetBlock);
                return new byte[]{(byte) 0x90, 0x00};
            } catch (IOException | IllegalArgumentException e) {
                Log.w(getClass().getName(), "Restore value block failed: " + e.getMessage());
                return new byte[]{(byte) 0x63, 0x00};
            }
        }

        // ACS ACR122U API V2.04 Section 5.5.1: Value Block Operation
        // Format: FF D7 00 [Block] 05 [VB_OP] [VB_Value (4 bytes MSB..LSB)] (10 bytes)
        if (lc == 0x05 && apdu.length == 10) {
            int vbOp = apdu[5] & 0xFF;
            int value = ((apdu[6] & 0xFF) << 24)
                    | ((apdu[7] & 0xFF) << 16)
                    | ((apdu[8] & 0xFF) << 8)
                    | (apdu[9] & 0xFF);
            try {
                if (vbOp == 0x00) {
                    // Store value into block (formats block as a MIFARE Value Block)
                    byte[] valueBlock = encodeValueBlock(value, (byte) blockNumber);
                    transceiver.writeBlock(blockNumber, valueBlock);
                    return new byte[]{(byte) 0x90, 0x00};
                } else if (vbOp == 0x01) {
                    // Android's MifareClassic.increment() requires non-negative operand (throws IllegalArgumentException if negative)
                    if (value < 0) {
                        return new byte[]{(byte) 0x63, 0x00};
                    }
                    transceiver.increment(blockNumber, value);
                    transceiver.transfer(blockNumber);
                    return new byte[]{(byte) 0x90, 0x00};
                } else if (vbOp == 0x02) {
                    // Android's MifareClassic.decrement() requires non-negative operand (throws IllegalArgumentException if negative)
                    if (value < 0) {
                        return new byte[]{(byte) 0x63, 0x00};
                    }
                    transceiver.decrement(blockNumber, value);
                    transceiver.transfer(blockNumber);
                    return new byte[]{(byte) 0x90, 0x00};
                }
            } catch (IOException | IllegalArgumentException e) {
                Log.w(getClass().getName(), "Value block operation failed: " + e.getMessage());
                return new byte[]{(byte) 0x63, 0x00};
            }
            return new byte[]{(byte) 0x6A, (byte) 0x81};
        }

        return new byte[]{(byte) 0x67, 0x00};
    }

    private byte[] handleReadValueBlock(byte[] apdu) {
        if (apdu.length != 4 && apdu.length != 5) {
            return new byte[]{(byte) 0x67, 0x00};
        }
        if (apdu[2] != 0x00) {
            return new byte[]{(byte) 0x6A, (byte) 0x86};
        }
        int blockNumber = apdu[3] & 0xFF;
        if (blockNumber >= transceiver.getBlockCount()) {
            return new byte[]{(byte) 0x6A, (byte) 0x82};
        }
        if (blockNumber == 0 || isSectorTrailer(blockNumber)) {
            return new byte[]{(byte) 0x69, (byte) 0x86};
        }
        int le = (apdu.length >= 5) ? (apdu[4] & 0xFF) : 4;
        if (le != 0 && le != 4) {
            return new byte[]{(byte) 0x6C, 0x04};
        }

        try {
            byte[] block = transceiver.readBlock(blockNumber);
            Integer value = decodeValueBlock(block);
            if (value == null) {
                return new byte[]{(byte) 0x63, 0x00};
            }
            // ACS ACR122U API V2.04 Section 5.5.2: Returns 4-byte signed value {MSB .. LSB} + 90 00
            return new byte[]{
                    (byte) ((value >> 24) & 0xFF),
                    (byte) ((value >> 16) & 0xFF),
                    (byte) ((value >> 8) & 0xFF),
                    (byte) (value & 0xFF),
                    (byte) 0x90,
                    (byte) 0x00
            };
        } catch (IOException e) {
            Log.w(getClass().getName(), "Read value block failed: " + e.getMessage());
            return new byte[]{(byte) 0x63, 0x00};
        }
    }

    /**
     * Encodes a 32-bit signed integer and 1-byte block address into the 16-byte MIFARE Value Block
     * format defined in NXP MF1S50YYX_V1 Section 8.6.2.1 (Table 4, Figure 8):
     * Bytes 0..3:   value (Little-Endian)
     * Bytes 4..7:   ~value (Bitwise inverted, Little-Endian)
     * Bytes 8..11:  value (Little-Endian)
     * Bytes 12..15: adr, ~adr, adr, ~adr
     */
    public static byte[] encodeValueBlock(int value, byte adr) {
        int inv = ~value;
        byte invAdr = (byte) ~adr;
        return new byte[]{
                (byte) (value & 0xFF),
                (byte) ((value >> 8) & 0xFF),
                (byte) ((value >> 16) & 0xFF),
                (byte) ((value >> 24) & 0xFF),
                (byte) (inv & 0xFF),
                (byte) ((inv >> 8) & 0xFF),
                (byte) ((inv >> 16) & 0xFF),
                (byte) ((inv >> 24) & 0xFF),
                (byte) (value & 0xFF),
                (byte) ((value >> 8) & 0xFF),
                (byte) ((value >> 16) & 0xFF),
                (byte) ((value >> 24) & 0xFF),
                adr,
                invAdr,
                adr,
                invAdr
        };
    }

    /**
     * Validates and decodes a 16-byte MIFARE Value Block per NXP MF1S50YYX_V1 Section 8.6.2.1.
     * Returns null if the block does not conform to the value block integrity rules.
     */
    public static Integer decodeValueBlock(byte[] block) {
        if (block == null || block.length < BLOCK_SIZE) {
            return null;
        }
        int v0 = (block[0] & 0xFF) | ((block[1] & 0xFF) << 8) | ((block[2] & 0xFF) << 16) | ((block[3] & 0xFF) << 24);
        int v1 = (block[4] & 0xFF) | ((block[5] & 0xFF) << 8) | ((block[6] & 0xFF) << 16) | ((block[7] & 0xFF) << 24);
        int v2 = (block[8] & 0xFF) | ((block[9] & 0xFF) << 8) | ((block[10] & 0xFF) << 16) | ((block[11] & 0xFF) << 24);
        if (v0 != ~v1 || v0 != v2) {
            return null;
        }
        byte adr = block[12];
        byte invAdr = (byte) ~adr;
        if (block[13] != invAdr || block[14] != adr || block[15] != invAdr) {
            return null;
        }
        return v0;
    }

    /**
     * Checks whether a block is a Sector Trailer (contains Key A, Access Bits, Key B).
     * NXP MF1S50YYX_V1 Section 8.6:
     * - Sectors 0..31: 4 blocks each (trailers: 3, 7, 11, ..., 127).
     * - Sectors 32..39 (4K cards): 16 blocks each (trailers: 143, 159, ..., 255).
     */
    public static boolean isSectorTrailer(int blockIndex) {
        if (blockIndex < 32 * 4) {
            return (blockIndex % 4) == 3;
        }
        return ((blockIndex - 32 * 4) % 16) == 15;
    }

    /**
     * Handles native MIFARE Classic frames (NXP MF1S50YYX_V1 Section 9.1 Table 9 & libfreefare mifare_classic.c).
     */
    private byte[] handleNativeMifareCommand(byte[] cmd) {
        byte op = cmd[0];

        // Native Authenticate:
        // - 8 bytes:  [60h/61h] [Block] [Key (6 bytes)]
        // - 12 bytes: [60h/61h] [Block] [Key (6 bytes)] [UID (4 bytes)] (libfreefare / PN532 format)
        if (op == CMD_AUTH_KEY_A || op == CMD_AUTH_KEY_B) {
            if (cmd.length != 8 && cmd.length != 12) {
                return new byte[]{(byte) 0x67, 0x00}; // Wrong length
            }
            int blockNumber = cmd[1] & 0xFF;
            if (blockNumber >= transceiver.getBlockCount()) {
                return new byte[]{(byte) 0x6A, (byte) 0x82};
            }
            // If 4-byte UID is supplied, validate it against detected card UID if available.
            // Per NXP AN10927 Section 4.2 & libfreefare mifare_classic.c, the 4-byte UID used in
            // Crypto1 authentication corresponds to the last 4 bytes of the card UID
            // (uid[0..3] for 4-byte UIDs, or uid[3..6] for 7-byte UIDs).
            if (cmd.length == 12 && uid != null && uid.length >= 4) {
                int uidOffset = uid.length - 4;
                for (int i = 0; i < 4; i++) {
                    if (cmd[8 + i] != uid[uidOffset + i]) {
                        Log.w(getClass().getName(), "Native auth UID mismatch: expected "
                                + Hex.getHexString(Arrays.copyOfRange(uid, uidOffset, uidOffset + 4))
                                + " but got " + Hex.getHexString(Arrays.copyOfRange(cmd, 8, 12)));
                        return new byte[]{(byte) 0x63, 0x00}; // Operation failed
                    }
                }
            }
            byte[] key = new byte[KEY_LENGTH];
            System.arraycopy(cmd, 2, key, 0, KEY_LENGTH);
            int sector = transceiver.blockToSector(blockNumber);
            try {
                boolean ok = (op == CMD_AUTH_KEY_A)
                        ? transceiver.authenticateSectorWithKeyA(sector, key)
                        : transceiver.authenticateSectorWithKeyB(sector, key);
                if (ok) {
                    return new byte[]{0x00};
                }
            } catch (IOException e) {
                Log.w(getClass().getName(), "Native authentication failed: " + e.getMessage());
            }
            return new byte[]{(byte) 0x63, 0x00};
        }

        // Native Read: 30h [Block] (2 bytes) -> returns 16-byte block data
        if (op == CMD_READ && cmd.length == 2) {
            int blockNumber = cmd[1] & 0xFF;
            if (blockNumber >= transceiver.getBlockCount()) {
                return new byte[]{(byte) 0x6A, (byte) 0x82};
            }
            try {
                return transceiver.readBlock(blockNumber);
            } catch (IOException e) {
                Log.w(getClass().getName(), "Native read failed: " + e.getMessage());
                return new byte[]{(byte) 0x63, 0x00};
            }
        }

        // Native Write: A0h [Block] [16-byte Data] (18 bytes)
        if (op == CMD_WRITE && cmd.length == 2 + BLOCK_SIZE) {
            int blockNumber = cmd[1] & 0xFF;
            if (blockNumber >= transceiver.getBlockCount()) {
                return new byte[]{(byte) 0x6A, (byte) 0x82};
            }
            byte[] blockData = new byte[BLOCK_SIZE];
            System.arraycopy(cmd, 2, blockData, 0, BLOCK_SIZE);
            try {
                transceiver.writeBlock(blockNumber, blockData);
                // Return 0x00 for libfreefare C-level success evaluation: ((BUFFER_SIZE(res) == 0) ? 0 : res[0])
                return new byte[]{0x00};
            } catch (IOException e) {
                Log.w(getClass().getName(), "Native write failed: " + e.getMessage());
                return new byte[]{(byte) 0x63, 0x00};
            }
        }

        // Native Increment / Decrement: [C1h/C0h] [Block] [4-byte LE value] (6 bytes)
        // Native Restore: C2h [Block] (2 bytes raw per NXP MF1S50YYX_V1 Sec 12.4 Table 26, or 6 bytes with dummy operand in libfreefare)
        if (((op == CMD_INCREMENT || op == CMD_DECREMENT) && cmd.length == 6)
                || (op == CMD_RESTORE && (cmd.length == 2 || cmd.length == 6))) {
            int blockNumber = cmd[1] & 0xFF;
            if (blockNumber >= transceiver.getBlockCount()) {
                return new byte[]{(byte) 0x6A, (byte) 0x82};
            }
            int val = (cmd.length >= 6)
                    ? ((cmd[2] & 0xFF) | ((cmd[3] & 0xFF) << 8) | ((cmd[4] & 0xFF) << 16) | ((cmd[5] & 0xFF) << 24))
                    : 0;
            try {
                if (op == CMD_INCREMENT) {
                    if (val < 0) {
                        return new byte[]{(byte) 0x63, 0x00};
                    }
                    transceiver.increment(blockNumber, val);
                } else if (op == CMD_DECREMENT) {
                    if (val < 0) {
                        return new byte[]{(byte) 0x63, 0x00};
                    }
                    transceiver.decrement(blockNumber, val);
                } else {
                    transceiver.restore(blockNumber);
                }
                // Return 0x00 for libfreefare C-level success evaluation: ((BUFFER_SIZE(res) == 0) ? 0 : res[0])
                return new byte[]{0x00};
            } catch (IOException | IllegalArgumentException e) {
                Log.w(getClass().getName(), "Native value operation failed: " + e.getMessage());
                return new byte[]{(byte) 0x63, 0x00};
            }
        }

        // Native Transfer: B0h [Block] (2 bytes) -> returns MC_ACK (0x0A)
        if (op == CMD_TRANSFER && cmd.length == 2) {
            int blockNumber = cmd[1] & 0xFF;
            if (blockNumber >= transceiver.getBlockCount()) {
                return new byte[]{(byte) 0x6A, (byte) 0x82};
            }
            try {
                transceiver.transfer(blockNumber);
                return new byte[]{MC_ACK};
            } catch (IOException e) {
                Log.w(getClass().getName(), "Native transfer failed: " + e.getMessage());
                return new byte[]{(byte) 0x63, 0x00};
            }
        }

        return new byte[]{(byte) 0x6D, 0x00};
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

    public byte[] getUid() {
        return uid.clone();
    }

    public byte[] getLoadedKey(int keyNumber) {
        if (keyNumber < 0 || keyNumber >= MAX_KEY_SLOTS) {
            return null;
        }
        return keyStore[keyNumber].clone();
    }
}
