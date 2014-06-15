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

package com.vsmartcard.remotesmartcardreader.app;

import android.os.Handler;

import com.vsmartcard.remotesmartcardreader.app.screaders.SCReader;

import java.io.Closeable;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetAddress;
import java.net.Socket;

public class VPCDWorker implements Runnable, Closeable {
    private int port;
    private String hostname;
    private SCReader reader;
    private Socket socket;
    private InputStream inputStream;
    private OutputStream outputStream;
    private boolean doRun;
    private MessageSender messageSender;

    public VPCDWorker(String hostname, int port, SCReader reader, Handler handler) {
        this.hostname = hostname;
        this.port = port;
        this.reader = reader;
        this.messageSender = new MessageSender(handler);
    }

    public void close() {
        doRun = false;
        try {
            vpcdDisconnect();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private static final int VPCD_CTRL_LEN = 1;
    private static final byte VPCD_CTRL_OFF = 0;
    private static final byte VPCD_CTRL_ON = 1;
    private static final byte VPCD_CTRL_RESET = 2;
    private static final byte VPCD_CTRL_ATR = 4;

    @Override
    public void run() {
        android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_BACKGROUND);
        doRun = true;

        try {
            vpcdConnect();
            messageSender.sendConnected(hostname+":"+Integer.toString(port));

            while (doRun) {
                byte[] in = receiveFromVPCD();
                if (in == null) {
                    doRun = false;
                    break;
                }
                byte[] out = null;

                if (in.length == VPCD_CTRL_LEN) {
                    switch (in[0]) {
                        case VPCD_CTRL_OFF:
                            reader.powerOff();
                            messageSender.sendOff();
                            break;
                        case VPCD_CTRL_ON:
                            reader.powerOn();
                            messageSender.sendOn();
                            break;
                        case VPCD_CTRL_RESET:
                            reader.reset();
                            messageSender.sendReset();
                            break;
                        case VPCD_CTRL_ATR:
                            out = reader.getATR();
                            //messageSender.sendATR(Hex.getHexString(out));
                            break;
                        default:
                            throw new IOException("Unhandled command from VPCD.");
                    }
                } else {
                    messageSender.sendCAPDU(Hex.getHexString(in));
                    out = reader.transmit(in);
                    messageSender.sendRAPDU(Hex.getHexString(out));
                }
                if (out != null) {
                    sendToVPCD(out);
                }
            }
        } catch (Exception e) {
            if (doRun) {
                e.printStackTrace();
                messageSender.sendError(e.getMessage());
            } else {
                messageSender.sendDisconnected("");
            }
        }
    }

    private byte[] receiveFromVPCD() throws IOException {
        /* convert length from network byte order.
        Note that Java always uses network byte order internally. */
        int length1 = inputStream.read();
        int length2 = inputStream.read();
        if (length1 == -1 || length2 == -1) {
            // EOF
            return null;
        }
        int length = (length1 << 8) + length2;

        byte[] data = new byte[length];

        int offset = 0;
        while (length > 0) {
            int read = inputStream.read(data, offset, length);
            if (read == -1) {
                // EOF
                return null;
            }
            offset += read;
            length -= read;
        }

        return data;
    }

    private void sendToVPCD(byte[] data) throws IOException {
        /* convert length to network byte order.
        Note that Java always uses network byte order internally. */
        byte[] length = new byte[2];
        length[0] = (byte) (data.length >> 8);
        length[1] = (byte) (data.length & 0xff);
        outputStream.write(length);

        outputStream.write(data, 0, data.length);

        outputStream.flush();
    }

    private void vpcdConnect() throws IOException {
        socket = new Socket(InetAddress.getByName(hostname), port);
        outputStream = socket.getOutputStream();
        inputStream = socket.getInputStream();
    }

    private void vpcdDisconnect() throws IOException {
        if  (socket != null) {
            socket.close();
        }
    }
}
