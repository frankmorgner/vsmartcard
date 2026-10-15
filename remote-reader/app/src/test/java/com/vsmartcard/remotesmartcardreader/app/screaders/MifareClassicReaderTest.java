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
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

public class MifareClassicReaderTest {

    private static class MockMifareClassicTransceiver implements MifareClassicReader.Transceiver {
        boolean closed = false;
        int timeout = 0;
        int type = MifareClassic.TYPE_CLASSIC;
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
        public int getType() {
            return type;
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

        // 5. MIFARE Plus 4K: NN = 00 37, TCK = 5C
        mock.type = MifareClassic.TYPE_PLUS;
        mock.size = MifareClassic.SIZE_4K;
        MifareClassicReader readerPlus4K = new MifareClassicReader(mock, mock.uid, null);
        byte[] atrPlus4K = readerPlus4K.getATR();
        assertEquals((byte) 0x00, atrPlus4K[13]);
        assertEquals((byte) 0x37, atrPlus4K[14]);
        assertEquals((byte) 0x5C, atrPlus4K[19]);
    }

    @Test
    public void testGetDataUid() throws IOException {
        MockMifareClassicTransceiver mock = new MockMifareClassicTransceiver();
        MifareClassicReader reader = new MifareClassicReader(mock, mock.uid, null);

        // FF CA 00 00 00 -> 4-byte UID + 90 00 (5 bytes APDU)
        byte[] resp5 = reader.transmit(new byte[]{(byte) 0xFF, (byte) 0xCA, 0x00, 0x00, 0x00});
        assertArrayEquals(new byte[]{0x01, 0x02, 0x03, 0x04, (byte) 0x90, 0x00}, resp5);

        // FF CA 00 00 -> 4-byte UID + 90 00 (4 bytes short APDU)
        byte[] resp4 = reader.transmit(new byte[]{(byte) 0xFF, (byte) 0xCA, 0x00, 0x00});
        assertArrayEquals(new byte[]{0x01, 0x02, 0x03, 0x04, (byte) 0x90, 0x00}, resp4);

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

        // PC/SC Part 3 Section 3.2.2.1.4: P1 bit 5 = 1 (Non-volatile memory) -> 69 87 (Not available)
        byte[] loadNonVolatile = reader.transmit(new byte[]{
                (byte) 0xFF, (byte) 0x82, 0x20, 0x01, 0x06,
                (byte) 0xA0, (byte) 0xA1, (byte) 0xA2, (byte) 0xA3, (byte) 0xA4, (byte) 0xA5
        });
        assertArrayEquals(new byte[]{(byte) 0x69, (byte) 0x87}, loadNonVolatile);

        // Invalid key length (Lc = 5) -> 69 89
        byte[] invalidLc = reader.transmit(new byte[]{
                (byte) 0xFF, (byte) 0x82, 0x00, 0x01, 0x05,
                (byte) 0xA0, (byte) 0xA1, (byte) 0xA2, (byte) 0xA3, (byte) 0xA4
        });
        assertArrayEquals(new byte[]{(byte) 0x69, (byte) 0x89}, invalidLc);

        // Strict APDU length check: trailing bytes -> 67 00
        byte[] trailingGarbage = reader.transmit(new byte[]{
                (byte) 0xFF, (byte) 0x82, 0x00, 0x01, 0x06,
                (byte) 0xA0, (byte) 0xA1, (byte) 0xA2, (byte) 0xA3, (byte) 0xA4, (byte) 0xA5, 0x00
        });
        assertArrayEquals(new byte[]{(byte) 0x67, 0x00}, trailingGarbage);

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

        // Read Binary: FF B0 00 04 10 (5 bytes)
        byte[] readResp = reader.transmit(new byte[]{(byte) 0xFF, (byte) 0xB0, 0x00, 0x04, 0x10});
        byte[] expected = Arrays.copyOf(payload, 18);
        expected[16] = (byte) 0x90;
        expected[17] = (byte) 0x00;
        assertArrayEquals(expected, readResp);

        // Read Binary: FF B0 00 04 (4 bytes short APDU defaults to Le=16)
        byte[] readResp4 = reader.transmit(new byte[]{(byte) 0xFF, (byte) 0xB0, 0x00, 0x04});
        assertArrayEquals(expected, readResp4);

        // Update Binary with trailing garbage -> 67 00
        byte[] updateGarbage = new byte[5 + 16 + 1];
        System.arraycopy(updateApdu, 0, updateGarbage, 0, updateApdu.length);
        assertArrayEquals(new byte[]{(byte) 0x67, 0x00}, reader.transmit(updateGarbage));
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

        // 4. Copy/Restore Block 5 to Block 6 (same sector 1): FF D7 00 05 02 03 06
        assertArrayEquals(
                new byte[]{(byte) 0x90, 0x00},
                reader.transmit(new byte[]{(byte) 0xFF, (byte) 0xD7, 0x00, 0x05, 0x02, 0x03, 0x06})
        );
        assertArrayEquals(
                new byte[]{0x00, 0x00, 0x00, 0x7D, (byte) 0x90, 0x00},
                reader.transmit(new byte[]{(byte) 0xFF, (byte) 0xB1, 0x00, 0x06, 0x04})
        );

        // 5. Cross-sector Restore (Block 5 in Sector 1 to Block 8 in Sector 2) -> 63 00 per ACR122U Section 5.5.3
        assertArrayEquals(
                new byte[]{(byte) 0x63, 0x00},
                reader.transmit(new byte[]{(byte) 0xFF, (byte) 0xD7, 0x00, 0x05, 0x02, 0x03, 0x08})
        );

        // 6. Store negative value (-4 = 0xFFFFFFFC) into Block 4: FF D7 00 04 05 00 FF FF FF FC
        assertArrayEquals(
                new byte[]{(byte) 0x90, 0x00},
                reader.transmit(new byte[]{
                        (byte) 0xFF, (byte) 0xD7, 0x00, 0x04, 0x05,
                        0x00, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFC
                })
        );
        // Read back negative value block: should return FF FF FF FC + 90 00
        assertArrayEquals(
                new byte[]{(byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFC, (byte) 0x90, 0x00},
                reader.transmit(new byte[]{(byte) 0xFF, (byte) 0xB1, 0x00, 0x04, 0x04})
        );

        // 7. Increment with negative operand -> 63 00 (rejected to protect Android API)
        assertArrayEquals(
                new byte[]{(byte) 0x63, 0x00},
                reader.transmit(new byte[]{
                        (byte) 0xFF, (byte) 0xD7, 0x00, 0x05, 0x05,
                        0x01, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFE
                })
        );

        // 8. Decrement with negative operand -> 63 00
        assertArrayEquals(
                new byte[]{(byte) 0x63, 0x00},
                reader.transmit(new byte[]{
                        (byte) 0xFF, (byte) 0xD7, 0x00, 0x05, 0x05,
                        0x02, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFE
                })
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

        // 5. Native Raw 2-byte Restore (C2 04) per NXP MF1S50YYX_V1 Sec 12.4 Table 26, then Transfer to Block 5
        assertArrayEquals(new byte[]{0x00}, reader.transmit(new byte[]{(byte) 0xC2, 0x04}));
        assertArrayEquals(new byte[]{0x0A}, reader.transmit(new byte[]{(byte) 0xB0, 0x05}));
        assertArrayEquals(
                new byte[]{0x00, 0x00, 0x00, 0x0F, (byte) 0x90, 0x00},
                reader.transmit(new byte[]{(byte) 0xFF, (byte) 0xB1, 0x00, 0x05, 0x04})
        );

        // 6. Native 6-byte Restore (C2 04 00 00 00 00) per libfreefare
        assertArrayEquals(new byte[]{0x00}, reader.transmit(new byte[]{(byte) 0xC2, 0x04, 0x00, 0x00, 0x00, 0x00}));
        assertArrayEquals(new byte[]{0x0A}, reader.transmit(new byte[]{(byte) 0xB0, 0x06}));
        assertArrayEquals(
                new byte[]{0x00, 0x00, 0x00, 0x0F, (byte) 0x90, 0x00},
                reader.transmit(new byte[]{(byte) 0xFF, (byte) 0xB1, 0x00, 0x06, 0x04})
        );

        reader.eject();
        assertTrue(mock.closed);
    }

    @Test
    public void testClassic4kUpperSectors() throws IOException {
        MockMifareClassicTransceiver mock = new MockMifareClassicTransceiver();
        mock.size = MifareClassic.SIZE_4K;
        mock.blockCount = 256;
        MifareClassicReader reader = new MifareClassicReader(mock, mock.uid, null);

        // Verify sector calculations for 4K 16-block sectors (sectors 32..39)
        assertEquals(32, mock.blockToSector(128));
        assertEquals(32, mock.blockToSector(143));
        assertEquals(33, mock.blockToSector(144));
        assertEquals(39, mock.blockToSector(240));
        assertEquals(39, mock.blockToSector(255));

        // Test isSectorTrailer helper
        assertTrue(MifareClassicReader.isSectorTrailer(3));
        assertTrue(MifareClassicReader.isSectorTrailer(127));
        assertTrue(MifareClassicReader.isSectorTrailer(143));
        assertTrue(MifareClassicReader.isSectorTrailer(255));
        assertFalse(MifareClassicReader.isSectorTrailer(0));
        assertFalse(MifareClassicReader.isSectorTrailer(128));
        assertFalse(MifareClassicReader.isSectorTrailer(254));

        // Authenticate block 128 (Sector 32)
        byte[] auth32 = reader.transmit(new byte[]{
                (byte) 0xFF, (byte) 0x86, 0x00, 0x00, 0x05,
                0x01, 0x00, (byte) 0x80, 0x60, 0x00
        });
        assertArrayEquals(new byte[]{(byte) 0x90, 0x00}, auth32);
        assertEquals(32, mock.lastAuthSector);

        // Update Binary on block 128
        byte[] payload = new byte[16];
        Arrays.fill(payload, (byte) 0x4B);
        byte[] updateApdu = new byte[5 + 16];
        updateApdu[0] = (byte) 0xFF;
        updateApdu[1] = (byte) 0xD6;
        updateApdu[2] = 0x00;
        updateApdu[3] = (byte) 0x80;
        updateApdu[4] = 0x10;
        System.arraycopy(payload, 0, updateApdu, 5, 16);
        assertArrayEquals(new byte[]{(byte) 0x90, 0x00}, reader.transmit(updateApdu));

        // Read Binary from block 128
        byte[] readResp = reader.transmit(new byte[]{(byte) 0xFF, (byte) 0xB0, 0x00, (byte) 0x80, 0x10});
        byte[] expected = Arrays.copyOf(payload, 18);
        expected[16] = (byte) 0x90;
        expected[17] = (byte) 0x00;
        assertArrayEquals(expected, readResp);
    }

    @Test
    public void testSectorTrailerAndBlockZeroValueBlockProtection() throws IOException {
        MockMifareClassicTransceiver mock = new MockMifareClassicTransceiver();
        MifareClassicReader reader = new MifareClassicReader(mock, mock.uid, null);

        // 1. Value Block Store on Manufacturer Block 0 -> 69 86 (Command not allowed)
        assertArrayEquals(
                new byte[]{(byte) 0x69, (byte) 0x86},
                reader.transmit(new byte[]{
                        (byte) 0xFF, (byte) 0xD7, 0x00, 0x00, 0x05,
                        0x00, 0x00, 0x00, 0x00, 0x01
                })
        );

        // 2. Value Block Store on Sector Trailer 3 -> 69 86
        assertArrayEquals(
                new byte[]{(byte) 0x69, (byte) 0x86},
                reader.transmit(new byte[]{
                        (byte) 0xFF, (byte) 0xD7, 0x00, 0x03, 0x05,
                        0x00, 0x00, 0x00, 0x00, 0x01
                })
        );

        // 3. Restore with Sector Trailer 7 as target block -> 69 86
        assertArrayEquals(
                new byte[]{(byte) 0x69, (byte) 0x86},
                reader.transmit(new byte[]{(byte) 0xFF, (byte) 0xD7, 0x00, 0x05, 0x02, 0x03, 0x07})
        );

        // 4. Read Value Block on Sector Trailer 3 -> 69 86
        assertArrayEquals(
                new byte[]{(byte) 0x69, (byte) 0x86},
                reader.transmit(new byte[]{(byte) 0xFF, (byte) 0xB1, 0x00, 0x03, 0x04})
        );
    }

    @Test
    public void testDirectTransmitValidAndInvalidP1P2() throws IOException {
        MockMifareClassicTransceiver mock = new MockMifareClassicTransceiver();
        MifareClassicReader reader = new MifareClassicReader(mock, mock.uid, null);

        // Valid Direct Transmit: FF 00 00 00 02 30 04 -> returns rawResp + 90 00
        byte[] resp = reader.transmit(new byte[]{(byte) 0xFF, 0x00, 0x00, 0x00, 0x02, 0x30, 0x04});
        assertArrayEquals(new byte[]{0x30, 0x04}, mock.lastTransceived);
        assertArrayEquals(new byte[]{0x0A, (byte) 0x90, 0x00}, resp);

        // Invalid P1 -> 6A 86
        assertArrayEquals(
                new byte[]{(byte) 0x6A, (byte) 0x86},
                reader.transmit(new byte[]{(byte) 0xFF, 0x00, 0x01, 0x00, 0x02, 0x30, 0x04})
        );

        // Invalid P2 -> 6A 86
        assertArrayEquals(
                new byte[]{(byte) 0x6A, (byte) 0x86},
                reader.transmit(new byte[]{(byte) 0xFF, 0x00, 0x00, 0x01, 0x02, 0x30, 0x04})
        );

        // Null response from transceive -> returns 63 00
        mock.nextTransceiveResponse = null;
        assertArrayEquals(
                new byte[]{(byte) 0x63, 0x00},
                reader.transmit(new byte[]{(byte) 0xFF, 0x00, 0x00, 0x00, 0x02, 0x30, 0x04})
        );
    }

    @Test
    public void testValueBlockOperationParameterValidations() throws IOException {
        MockMifareClassicTransceiver mock = new MockMifareClassicTransceiver();
        MifareClassicReader reader = new MifareClassicReader(mock, mock.uid, null);

        // Invalid P1 in Value Block Operation -> 6A 86
        assertArrayEquals(
                new byte[]{(byte) 0x6A, (byte) 0x86},
                reader.transmit(new byte[]{
                        (byte) 0xFF, (byte) 0xD7, 0x01, 0x05, 0x05,
                        0x00, 0x00, 0x00, 0x00, 0x01
                })
        );

        // Invalid subcode for restore (04 instead of 03) -> 6A 81
        assertArrayEquals(
                new byte[]{(byte) 0x6A, (byte) 0x81},
                reader.transmit(new byte[]{(byte) 0xFF, (byte) 0xD7, 0x00, 0x05, 0x02, 0x04, 0x06})
        );

        // Read Value Block trailing garbage -> 67 00
        assertArrayEquals(
                new byte[]{(byte) 0x67, 0x00},
                reader.transmit(new byte[]{(byte) 0xFF, (byte) 0xB1, 0x00, 0x05, 0x04, (byte) 0xFF})
        );
    }

    @Test
    public void testOutOfBoundsBlockIndices() throws IOException {
        MockMifareClassicTransceiver mock = new MockMifareClassicTransceiver();
        mock.blockCount = 64; // 1K card has 64 blocks
        MifareClassicReader reader = new MifareClassicReader(mock, mock.uid, null);

        // Read block 64 -> 6A 82 (Addressed block does not exist)
        assertArrayEquals(
                new byte[]{(byte) 0x6A, (byte) 0x82},
                reader.transmit(new byte[]{(byte) 0xFF, (byte) 0xB0, 0x00, 0x40, 0x10})
        );

        // Update block 64 -> 6A 82
        byte[] payload = new byte[16];
        byte[] updateApdu = new byte[5 + 16];
        updateApdu[0] = (byte) 0xFF;
        updateApdu[1] = (byte) 0xD6;
        updateApdu[2] = 0x00;
        updateApdu[3] = 0x40;
        updateApdu[4] = 0x10;
        System.arraycopy(payload, 0, updateApdu, 5, 16);
        assertArrayEquals(new byte[]{(byte) 0x6A, (byte) 0x82}, reader.transmit(updateApdu));

        // Authenticate block 64 -> 65 81 (Memory failure / block does not exist)
        assertArrayEquals(
                new byte[]{(byte) 0x65, (byte) 0x81},
                reader.transmit(new byte[]{
                        (byte) 0xFF, (byte) 0x86, 0x00, 0x00, 0x05,
                        0x01, 0x00, 0x40, 0x60, 0x00
                })
        );
    }

    @Test
    public void testNativeAuthenticateLengthsAndUidValidation() throws IOException {
        MockMifareClassicTransceiver mock = new MockMifareClassicTransceiver();
        MifareClassicReader reader = new MifareClassicReader(mock, mock.uid, null);

        // 8-byte valid format (no UID): 60 04 FF FF FF FF FF FF
        byte[] auth8 = reader.transmit(new byte[]{
                0x60, 0x04,
                (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF
        });
        assertArrayEquals(new byte[]{0x00}, auth8);

        // 12-byte valid format with matching UID: 60 04 FF..FF 01 02 03 04
        byte[] auth12 = reader.transmit(new byte[]{
                0x60, 0x04,
                (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF,
                0x01, 0x02, 0x03, 0x04
        });
        assertArrayEquals(new byte[]{0x00}, auth12);

        // 12-byte format with MISMATCHED UID (05 06 07 08) -> returns 63 00
        byte[] auth12Mismatch = reader.transmit(new byte[]{
                0x60, 0x04,
                (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF,
                0x05, 0x06, 0x07, 0x08
        });
        assertArrayEquals(new byte[]{(byte) 0x63, 0x00}, auth12Mismatch);

        // Invalid lengths: 7 bytes, 9 bytes, 13 bytes -> return 67 00
        assertArrayEquals(new byte[]{(byte) 0x67, 0x00}, reader.transmit(new byte[]{
                0x60, 0x04, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF
        }));
        assertArrayEquals(new byte[]{(byte) 0x67, 0x00}, reader.transmit(new byte[]{
                0x60, 0x04, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, 0x01
        }));
        assertArrayEquals(new byte[]{(byte) 0x67, 0x00}, reader.transmit(new byte[]{
                0x60, 0x04, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF,
                0x01, 0x02, 0x03, 0x04, 0x05
        }));
    }

    @Test
    public void testNativeAuthenticate7ByteUid() throws IOException {
        MockMifareClassicTransceiver mock = new MockMifareClassicTransceiver();
        byte[] uid7 = new byte[]{0x04, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
        MifareClassicReader reader = new MifareClassicReader(mock, uid7, null);

        // libfreefare passes last 4 bytes: 33 44 55 66
        byte[] authValid = reader.transmit(new byte[]{
                0x60, 0x04,
                (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF,
                0x33, 0x44, 0x55, 0x66
        });
        assertArrayEquals(new byte[]{0x00}, authValid);

        // Passing first 4 bytes (04 11 22 33) must fail (63 00)
        byte[] authFirst4 = reader.transmit(new byte[]{
                0x60, 0x04,
                (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF,
                0x04, 0x11, 0x22, 0x33
        });
        assertArrayEquals(new byte[]{(byte) 0x63, 0x00}, authFirst4);
    }

    @Test
    public void testUpdateBinaryStrictSingleBlock() throws IOException {
        MockMifareClassicTransceiver mock = new MockMifareClassicTransceiver();
        MifareClassicReader reader = new MifareClassicReader(mock, mock.uid, null);

        // Lc = 32 (multi-block write) -> 67 00 (Wrong length)
        byte[] multiBlock = new byte[5 + 32];
        multiBlock[0] = (byte) 0xFF;
        multiBlock[1] = (byte) 0xD6;
        multiBlock[2] = 0x00;
        multiBlock[3] = 0x04;
        multiBlock[4] = 0x20;
        assertArrayEquals(new byte[]{(byte) 0x67, 0x00}, reader.transmit(multiBlock));

        // Lc = 15 or 17 -> 67 00
        byte[] shortBlock = new byte[5 + 15];
        shortBlock[0] = (byte) 0xFF;
        shortBlock[1] = (byte) 0xD6;
        shortBlock[2] = 0x00;
        shortBlock[3] = 0x04;
        shortBlock[4] = 0x0F;
        assertArrayEquals(new byte[]{(byte) 0x67, 0x00}, reader.transmit(shortBlock));
    }

    @Test
    public void testReadBinaryAcrossSectorBoundariesReturns6300() throws IOException {
        MockMifareClassicTransceiver mock = new MockMifareClassicTransceiver();
        MifareClassicReader reader = new MifareClassicReader(mock, mock.uid, null);

        // Read 32 bytes starting at block 3 (Sector 0 Block 3 + Sector 1 Block 4) -> 63 00
        byte[] crossSector = reader.transmit(new byte[]{(byte) 0xFF, (byte) 0xB0, 0x00, 0x03, 0x20});
        assertArrayEquals(new byte[]{(byte) 0x63, 0x00}, crossSector);

        // Read 32 bytes within Sector 0 (blocks 0 and 1) -> 90 00
        byte[] sameSector = reader.transmit(new byte[]{(byte) 0xFF, (byte) 0xB0, 0x00, 0x00, 0x20});
        assertEquals((byte) 0x90, sameSector[32]);
        assertEquals((byte) 0x00, sameSector[33]);
    }

    @Test
    public void testAtr2kCardFallbackForNonPlusTypes() throws IOException {
        MockMifareClassicTransceiver mock = new MockMifareClassicTransceiver();
        mock.size = MifareClassic.SIZE_2K;

        // TYPE_CLASSIC on 2K clone card -> CARD_NAME_MIFARE_PLUS_2K (0x0036)
        mock.type = MifareClassic.TYPE_CLASSIC;
        MifareClassicReader readerClassic2k = new MifareClassicReader(mock, mock.uid, null);
        assertEquals((byte) 0x00, readerClassic2k.getATR()[13]);
        assertEquals((byte) 0x36, readerClassic2k.getATR()[14]);

        // TYPE_UNKNOWN on 2K clone card -> CARD_NAME_MIFARE_PLUS_2K (0x0036)
        mock.type = MifareClassic.TYPE_UNKNOWN;
        MifareClassicReader readerUnknown2k = new MifareClassicReader(mock, mock.uid, null);
        assertEquals((byte) 0x00, readerUnknown2k.getATR()[13]);
        assertEquals((byte) 0x36, readerUnknown2k.getATR()[14]);
    }
}
