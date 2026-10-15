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

import android.nfc.tech.MifareClassic;

import org.junit.Test;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

public class MifareClassicReaderTest {

    private static class MockMifareClassicTransceiver implements MifareClassicReader.Transceiver {
        boolean closed = false;
        int timeout = 0;
        int size = MifareClassic.SIZE_1K;
        int blockCount = 64;
        byte[] uid = new byte[]{0x01, 0x02, 0x03, 0x04};
        byte[] expectedKeyA = new byte[]{(byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF};
        byte[] expectedKeyB = new byte[]{(byte) 0xA0, (byte) 0xA1, (byte) 0xA2, (byte) 0xA3, (byte) 0xA4, (byte) 0xA5};
        int lastAuthSector = -1;
        final Map<Integer, byte[]> blocks = new HashMap<>();
        int internalTransferReg = 0;
        byte[] lastTransceived = null;
        byte[] nextTransceiveResponse = new byte[]{0x0A};

        @Override
        public void close() throws IOException {
            closed = true;
        }

        @Override
        public void setTimeout(int timeout) {
            this.timeout = timeout;
        }

        @Override
        public int getSize() {
            return size;
        }

        @Override
        public int getBlockCount() {
            return blockCount;
        }

        @Override
        public int blockToSector(int blockIndex) {
            if (blockIndex < 32 * 4) {
                return blockIndex / 4;
            }
            return 32 + (blockIndex - 32 * 4) / 16;
        }

        @Override
        public boolean authenticateSectorWithKeyA(int sectorIndex, byte[] key) throws IOException {
            if (Arrays.equals(expectedKeyA, key)) {
                lastAuthSector = sectorIndex;
                return true;
            }
            return false;
        }

        @Override
        public boolean authenticateSectorWithKeyB(int sectorIndex, byte[] key) throws IOException {
            if (Arrays.equals(expectedKeyB, key)) {
                lastAuthSector = sectorIndex;
                return true;
            }
            return false;
        }

        @Override
        public byte[] readBlock(int blockIndex) throws IOException {
            byte[] data = blocks.get(blockIndex);
            return data != null ? data.clone() : new byte[16];
        }

        @Override
        public void writeBlock(int blockIndex, byte[] data) throws IOException {
            blocks.put(blockIndex, data.clone());
        }

        @Override
        public void increment(int blockIndex, int value) throws IOException {
            internalTransferReg = readValueFromBlock(blockIndex) + value;
        }

        @Override
        public void decrement(int blockIndex, int value) throws IOException {
            internalTransferReg = readValueFromBlock(blockIndex) - value;
        }

        @Override
        public void restore(int blockIndex) throws IOException {
            internalTransferReg = readValueFromBlock(blockIndex);
        }

        @Override
        public void transfer(int blockIndex) throws IOException {
            writeValueToBlock(blockIndex, internalTransferReg);
        }

        @Override
        public byte[] transceive(byte[] data) throws IOException {
            lastTransceived = data != null ? data.clone() : null;
            return nextTransceiveResponse != null ? nextTransceiveResponse.clone() : null;
        }

        private int readValueFromBlock(int blockIndex) {
            byte[] b = blocks.get(blockIndex);
            if (b == null || b.length < 4) {
                return 0;
            }
            return ByteBuffer.wrap(b, 0, 4).order(ByteOrder.LITTLE_ENDIAN).getInt();
        }

        private void writeValueToBlock(int blockIndex, int val) {
            byte[] block = new byte[16];
            int inv = ~val;
            ByteBuffer buf = ByteBuffer.wrap(block).order(ByteOrder.LITTLE_ENDIAN);
            buf.putInt(0, val);
            buf.putInt(4, inv);
            buf.putInt(8, val);
            byte addr = (byte) (blockIndex & 0xFF);
            byte invAddr = (byte) (~addr & 0xFF);
            block[12] = addr;
            block[13] = invAddr;
            block[14] = addr;
            block[15] = invAddr;
            blocks.put(blockIndex, block);
        }
    }

    @Test
    public void testAtrForCardSizes() throws IOException {
        MockMifareClassicTransceiver mock = new MockMifareClassicTransceiver();

        // 1. MIFARE Classic 1K: NN = 00 01, TCK = 6A (ACS ACR122U V2.04 Section 3.1.1 Table 2, Page 8)
        mock.size = MifareClassic.SIZE_1K;
        MifareClassicReader reader1K = new MifareClassicReader(mock, mock.uid, null);
        assertArrayEquals(
                new byte[]{
                        (byte) 0x3B, (byte) 0x8F, (byte) 0x80, (byte) 0x01,
                        (byte) 0x80, (byte) 0x4F, (byte) 0x0C, (byte) 0xA0,
                        (byte) 0x00, (byte) 0x00, (byte) 0x03, (byte) 0x06,
                        (byte) 0x03, (byte) 0x00, (byte) 0x01,
                        (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00,
                        (byte) 0x6A
                },
                reader1K.getATR()
        );

        // 2. MIFARE Classic 4K: NN = 00 02, TCK = 69
        mock.size = MifareClassic.SIZE_4K;
        MifareClassicReader reader4K = new MifareClassicReader(mock, mock.uid, null);
        byte[] atr4K = reader4K.getATR();
        assertEquals((byte) 0x00, atr4K[13]);
        assertEquals((byte) 0x02, atr4K[14]);
        assertEquals((byte) 0x69, atr4K[19]);

        // 3. MIFARE Mini: NN = 00 26
        mock.size = MifareClassic.SIZE_MINI;
        MifareClassicReader readerMini = new MifareClassicReader(mock, mock.uid, null);
        byte[] atrMini = readerMini.getATR();
        assertEquals((byte) 0x00, atrMini[13]);
        assertEquals((byte) 0x26, atrMini[14]);

        // 4. MIFARE Plus 2K: NN = 00 36
        mock.size = MifareClassic.SIZE_2K;
        MifareClassicReader reader2K = new MifareClassicReader(mock, mock.uid, null);
        byte[] atr2K = reader2K.getATR();
        assertEquals((byte) 0x00, atr2K[13]);
        assertEquals((byte) 0x36, atr2K[14]);
    }

    @Test
    public void testGetDataUid() throws IOException {
        MockMifareClassicTransceiver mock = new MockMifareClassicTransceiver();
        MifareClassicReader reader = new MifareClassicReader(mock, mock.uid, null);

        // FF CA 00 00 00 -> 4-byte UID + 90 00
        byte[] resp = reader.transmit(new byte[]{(byte) 0xFF, (byte) 0xCA, 0x00, 0x00, 0x00});
        assertArrayEquals(new byte[]{0x01, 0x02, 0x03, 0x04, (byte) 0x90, 0x00}, resp);

        // P1 = 0x01 (ATS historical bytes on storage card) -> 6A 81 (Function not supported)
        assertArrayEquals(
                new byte[]{(byte) 0x6A, (byte) 0x81},
                reader.transmit(new byte[]{(byte) 0xFF, (byte) 0xCA, 0x01, 0x00, 0x00})
        );
    }

    @Test
    public void testLoadKeysAndGeneralAuthenticateAndLegacyAuthenticate() throws IOException {
        MockMifareClassicTransceiver mock = new MockMifareClassicTransceiver();
        MifareClassicReader reader = new MifareClassicReader(mock, mock.uid, null);

        // Default factory key FF FF FF FF FF FF is pre-loaded in slot 0:
        // General Authenticate (FF 86 00 00 05 01 00 04 60 00) on block 4 (sector 1) with Key A
        byte[] authDefault = reader.transmit(new byte[]{
                (byte) 0xFF, (byte) 0x86, 0x00, 0x00, 0x05,
                0x01, 0x00, 0x04, 0x60, 0x00
        });
        assertArrayEquals(new byte[]{(byte) 0x90, 0x00}, authDefault);
        assertEquals(1, mock.lastAuthSector);

        // Load custom Key B (A0 A1 A2 A3 A4 A5) into slot 1: FF 82 00 01 06 A0 A1 A2 A3 A4 A5
        byte[] loadKeyB = reader.transmit(new byte[]{
                (byte) 0xFF, (byte) 0x82, 0x00, 0x01, 0x06,
                (byte) 0xA0, (byte) 0xA1, (byte) 0xA2, (byte) 0xA3, (byte) 0xA4, (byte) 0xA5
        });
        assertArrayEquals(new byte[]{(byte) 0x90, 0x00}, loadKeyB);

        // Legacy Authenticate (FF 88 00 08 61 01) on block 8 (sector 2) with Key B from slot 1
        byte[] legacyAuth = reader.transmit(new byte[]{
                (byte) 0xFF, (byte) 0x88, 0x00, 0x08, 0x61, 0x01
        });
        assertArrayEquals(new byte[]{(byte) 0x90, 0x00}, legacyAuth);
        assertEquals(2, mock.lastAuthSector);

        // Authenticate with wrong key (slot 0 as Key B) -> 63 00
        byte[] failedAuth = reader.transmit(new byte[]{
                (byte) 0xFF, (byte) 0x88, 0x00, 0x08, 0x61, 0x00
        });
        assertArrayEquals(new byte[]{(byte) 0x63, 0x00}, failedAuth);
    }

    @Test
    public void testReadAndUpdateBinary() throws IOException {
        MockMifareClassicTransceiver mock = new MockMifareClassicTransceiver();
        MifareClassicReader reader = new MifareClassicReader(mock, mock.uid, null);

        byte[] payload = new byte[16];
        for (int i = 0; i < 16; i++) {
            payload[i] = (byte) (0x10 + i);
        }

        // Update Binary: FF D6 00 04 10 [16B]
        byte[] updateApdu = new byte[5 + 16];
        updateApdu[0] = (byte) 0xFF;
        updateApdu[1] = (byte) 0xD6;
        updateApdu[2] = 0x00;
        updateApdu[3] = 0x04;
        updateApdu[4] = 0x10;
        System.arraycopy(payload, 0, updateApdu, 5, 16);

        assertArrayEquals(new byte[]{(byte) 0x90, 0x00}, reader.transmit(updateApdu));

        // Read Binary: FF B0 00 04 10
        byte[] readResp = reader.transmit(new byte[]{(byte) 0xFF, (byte) 0xB0, 0x00, 0x04, 0x10});
        byte[] expected = Arrays.copyOf(payload, 18);
        expected[16] = (byte) 0x90;
        expected[17] = (byte) 0x00;
        assertArrayEquals(expected, readResp);
    }

    @Test
    public void testValueBlockOperations() throws IOException {
        MockMifareClassicTransceiver mock = new MockMifareClassicTransceiver();
        MifareClassicReader reader = new MifareClassicReader(mock, mock.uid, null);

        // 1. Store Value 100 (0x00000064 MSB) into Block 5: FF D7 00 05 05 00 00 00 00 64
        byte[] storeResp = reader.transmit(new byte[]{
                (byte) 0xFF, (byte) 0xD7, 0x00, 0x05, 0x05,
                0x00, 0x00, 0x00, 0x00, 0x64
        });
        assertArrayEquals(new byte[]{(byte) 0x90, 0x00}, storeResp);

        // Verify NXP MF1S50YYX_V1 Section 8.6.2.1 value block structure:
        // Value (LE): 64 00 00 00, ~Value (LE): 9B FF FF FF, Value (LE): 64 00 00 00,
        // Addr: 05 FA 05 FA
        assertArrayEquals(
                new byte[]{
                        0x64, 0x00, 0x00, 0x00,
                        (byte) 0x9B, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF,
                        0x64, 0x00, 0x00, 0x00,
                        0x05, (byte) 0xFA, 0x05, (byte) 0xFA
                },
                mock.blocks.get(5)
        );

        // 2. Increment Block 5 by 25 (0x00000019): FF D7 00 05 05 01 00 00 00 19
        assertArrayEquals(
                new byte[]{(byte) 0x90, 0x00},
                reader.transmit(new byte[]{
                        (byte) 0xFF, (byte) 0xD7, 0x00, 0x05, 0x05,
                        0x01, 0x00, 0x00, 0x00, 0x19
                })
        );

        // 3. Read Value Block 5: FF B1 00 05 04 -> 00 00 00 7D (125) + 90 00
        assertArrayEquals(
                new byte[]{0x00, 0x00, 0x00, 0x7D, (byte) 0x90, 0x00},
                reader.transmit(new byte[]{(byte) 0xFF, (byte) 0xB1, 0x00, 0x05, 0x04})
        );

        // 4. Copy/Restore Block 5 to Block 6: FF D7 00 05 02 03 06
        assertArrayEquals(
                new byte[]{(byte) 0x90, 0x00},
                reader.transmit(new byte[]{(byte) 0xFF, (byte) 0xD7, 0x00, 0x05, 0x02, 0x03, 0x06})
        );
        assertArrayEquals(
                new byte[]{0x00, 0x00, 0x00, 0x7D, (byte) 0x90, 0x00},
                reader.transmit(new byte[]{(byte) 0xFF, (byte) 0xB1, 0x00, 0x06, 0x04})
        );
    }

    @Test
    public void testLibfreefareNativeFrames() throws IOException {
        MockMifareClassicTransceiver mock = new MockMifareClassicTransceiver();
        MifareClassicReader reader = new MifareClassicReader(mock, mock.uid, null);

        // 1. Native Authenticate Key A: 60 [Block=04] [Key=FF..FF] [UID=01 02 03 04] (12 bytes)
        byte[] nativeAuth = reader.transmit(new byte[]{
                0x60, 0x04,
                (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF,
                0x01, 0x02, 0x03, 0x04
        });
        assertArrayEquals(new byte[]{0x00}, nativeAuth);
        assertEquals(1, mock.lastAuthSector);

        // 2. Native Write: A0 [Block=04] [16B] (18 bytes)
        byte[] writeFrame = new byte[18];
        writeFrame[0] = (byte) 0xA0;
        writeFrame[1] = 0x04;
        writeFrame[2] = 0x42;
        byte[] writeResp = reader.transmit(writeFrame);
        assertArrayEquals(new byte[]{0x00}, writeResp);

        // 3. Native Read: 30 [Block=04] (2 bytes) -> 16 bytes
        byte[] readResp = reader.transmit(new byte[]{0x30, 0x04});
        assertEquals(16, readResp.length);
        assertEquals(0x42, readResp[0]);

        // 4. Native Increment + Transfer (C1 [Block=04] [4B LE] then B0 [Block=04] -> 0x0A = MC_OK)
        // Store value 10 first via formatValueBlock
        reader.transmit(new byte[]{
                (byte) 0xFF, (byte) 0xD7, 0x00, 0x04, 0x05,
                0x00, 0x00, 0x00, 0x00, 0x0A
        });
        assertArrayEquals(new byte[]{0x00}, reader.transmit(new byte[]{(byte) 0xC1, 0x04, 0x05, 0x00, 0x00, 0x00}));
        assertArrayEquals(new byte[]{0x0A}, reader.transmit(new byte[]{(byte) 0xB0, 0x04}));

        // Verify value is now 15 (0x0F)
        assertArrayEquals(
                new byte[]{0x00, 0x00, 0x00, 0x0F, (byte) 0x90, 0x00},
                reader.transmit(new byte[]{(byte) 0xFF, (byte) 0xB1, 0x00, 0x04, 0x04})
        );

        reader.eject();
        assertTrue(mock.closed);
    }
}
