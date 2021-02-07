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

import android.os.AsyncTask;

import androidx.annotation.Nullable;

import com.example.android.common.logger.Log;
import com.vsmartcard.remotesmartcardreader.app.screaders.SCReader;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Inet4Address;
import java.net.InetAddress;
import java.net.InterfaceAddress;
import java.net.NetworkInterface;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketTimeoutException;
import java.util.Enumeration;
import java.util.LinkedList;
import java.util.List;
import java.util.concurrent.TimeUnit;

class VPCDWorker extends AsyncTask<VPCDWorker.VPCDWorkerParams, Void, Void> {

    public static class VPCDWorkerParams {
        final String hostname;
        final int port;
        final SCReader reader;
        final boolean listen;
        VPCDWorkerParams(String hostname, int port, SCReader reader, boolean listen) {
            this.hostname = hostname;
            this.port = port;
            this.reader = reader;
            this.listen = listen;
        }
    }

    public static final int DEFAULT_PORT = 35963;
    // default URI when used in emulator
    public static final String DEFAULT_HOSTNAME = "10.0.2.2";
    public static final boolean DEFAULT_LISTEN = false;

    private SCReader reader;
    private ServerSocket listenSocket;
    private Socket socket;
    private InputStream inputStream;
    private OutputStream outputStream;

    @Override
    protected void onCancelled () {
        try {
            if (socket != null)
                // interrupt all blocking socket communication
                socket.close();
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
    public Void doInBackground(VPCDWorkerParams... params) {
        try {
            reader = params[0].reader;
            vpcdConnection(params[0]);

            while (!isCancelled()) {
                vpcdAccept();
                byte[] out = null;
                byte[] in = receiveFromVPCD();
                if (in == null) {
                    if (listenSocket == null) {
                        Log.i(this.getClass().getName(), "End of stream, finishing");
                        break;
                    } else {
                        Log.i(this.getClass().getName(), "End of stream, closing connection");
                        vpcdCloseClient();
                        continue; // back to accept
                    }
                }

                if (in.length == VPCD_CTRL_LEN) {
                    switch (in[0]) {
                        case VPCD_CTRL_OFF:
                            reader.powerOff();
                            Log.i(this.getClass().getName(), "Powered down the card (cold reset)");
                            break;
                        case VPCD_CTRL_ON:
                            reader.powerOn();
                            Log.i(this.getClass().getName(), "Powered up the card with ATR " + Hex.getHexString(reader.getATR()));
                            break;
                        case VPCD_CTRL_RESET:
                            reader.reset();
                            Log.i(this.getClass().getName(), "Resetted the card (warm reset)");
                            break;
                        case VPCD_CTRL_ATR:
                            out = reader.getATR();
                            break;
                        default:
                            throw new IOException("Unhandled command from VPCD.");
                    }
                } else {
                    Log.i(this.getClass().getName(), "C-APDU: " + Hex.getHexString(in));
                    out = reader.transmit(in);
                    Log.i(this.getClass().getName(), "R-APDU: " + Hex.getHexString(out));
                }
                if (out != null) {
                    sendToVPCD(out);
                }
            }
        } catch (Exception e) {
            if (!isCancelled()) {
                e.printStackTrace();
                Log.i(this.getClass().getName(), "ERROR: " + e.getMessage());
            }
        }
        try {
            vpcdDisconnect();
        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }

    @Nullable
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

    private void vpcdConnection(VPCDWorkerParams params) throws IOException {
        if (params.listen){
            vpcdListen(params.port);
            return;
        }

        Log.i(this.getClass().getName(), "Connecting to " + params.hostname + ":" + Integer.toString(params.port) + "...");
        vpcdConnect(params.hostname, params.port);
        Log.i(this.getClass().getName(), "Connected to VPCD");
    }

    private void vpcdListen(int port) throws IOException {
        listenSocket = new ServerSocket(port);

        final List<String> ifaceAddresses = new LinkedList<>();
        final Enumeration<NetworkInterface> ifaces = NetworkInterface.getNetworkInterfaces();
        while(ifaces.hasMoreElements()){
            final NetworkInterface iface = ifaces.nextElement();
            if (!iface.isUp() || iface.isLoopback() || iface.isVirtual()) {
                continue;
            }
            for(InterfaceAddress addr : iface.getInterfaceAddresses()){
                final InetAddress inetAddr = addr.getAddress();
                ifaceAddresses.add(inetAddr.getHostAddress());
            }
        }

        Log.i(this.getClass().getName(), "Listening on port " + port + ". Local addresses: " + join(", ", ifaceAddresses));
    }

    private void vpcdAccept() throws IOException {
        if(listenSocket == null){
            return;
        }

        if (socket != null){
            return;  // Already accepted, only one client allowed
        }

        Log.i(this.getClass().getName(),"Waiting for connections...");
        while(!isCancelled()) {
            listenSocket.setSoTimeout(1000);
            try {
                socket = listenSocket.accept();
            } catch (SocketTimeoutException ignored){}
            if (socket != null){
                break;
            }
        }

        Log.i(this.getClass().getName(),"Connected, " + socket.getInetAddress());
        listenSocket.setSoTimeout(0);
        outputStream = socket.getOutputStream();
        inputStream = socket.getInputStream();
    }

    private void vpcdCloseClient(){
        try {
            outputStream.close();
        } catch (IOException ignored) { }
        try {
            inputStream.close();
        } catch (IOException ignored) { }
        try {
            socket.close();
        } catch (IOException ignored) { }
        outputStream = null;
        inputStream = null;
        socket = null;
    }

    private void vpcdConnect(String hostname, int port) throws IOException {
        listenSocket = null;
        socket = new Socket(InetAddress.getByName(hostname), port);
        outputStream = socket.getOutputStream();
        inputStream = socket.getInputStream();
    }

    private void vpcdDisconnect() throws IOException {
        if (reader != null) {
            reader.eject();
        }
        if  (socket != null) {
            socket.close();
            Log.i(this.getClass().getName(), "Disconnected from VPCD");
        }
        if  (listenSocket != null) {
            Log.i(this.getClass().getName(), "Closing listening socket");
            listenSocket.close();
        }
    }

    /**
     * Usage of API level 24+ would allow streams(), join can be removed.
     */
    private static String join(String separator, List<String> input) {
        if (input == null || input.size() <= 0) return "";
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < input.size(); i++) {
            sb.append(input.get(i));
            if (i != input.size() - 1) {
                sb.append(separator);
            }
        }
        return sb.toString();

    }
}