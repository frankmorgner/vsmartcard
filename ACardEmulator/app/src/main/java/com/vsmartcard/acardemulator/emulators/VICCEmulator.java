package com.vsmartcard.acardemulator.emulators;

import android.os.StrictMode;
import android.util.Log;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetAddress;
import java.net.Socket;

public class VICCEmulator implements Emulator {
    public static final int DEFAULT_PORT = 35963;
    public static final String DEFAULT_HOSTNAME = "10.0.2.2";

    private static Socket socket;
    private static InputStream inputStream;
    private static OutputStream outputStream;

    public VICCEmulator(String hostname, int port) {
        if  (socket == null) {
            try {
                if (android.os.Build.VERSION.SDK_INT > 9) {
                    StrictMode.ThreadPolicy policy = new StrictMode.ThreadPolicy.Builder().permitAll().build();
                    StrictMode.setThreadPolicy(policy);
                }
                vpcdConnect(hostname, port);
                sendPowerOn();
            } catch (IOException e) {
                e.printStackTrace();
                vpcdDisconnect();
            }
        }
    }

    @Override
    public void deactivate() {
        try {
            sendPowerOff();
        } catch (IOException e) {
            e.printStackTrace();
            vpcdDisconnect();
        }
    }

    @Override
    public void destroy() {
        vpcdDisconnect();
    }

    @Override
    public byte[] process(byte[] commandAPDU) {
        try {
            sendToVPCD(commandAPDU);
            return receiveFromVPCD();
        } catch (IOException e) {
            e.printStackTrace();
        }
        return new byte[] {0x6D, 0x00};
    }

    private void sendPowerOff() throws IOException {
        Log.d("", "Power Off");
        sendToVPCD(new byte[] {0x00});
    }
    private void sendPowerOn() throws IOException {
        Log.d("", "Power On");
        sendToVPCD(new byte[] {0x01});
    }
    private void sendReset() throws IOException {
        Log.d("", "Reset");
        sendToVPCD(new byte[] {0x02});
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

    private void vpcdConnect(String hostname, int port) throws IOException {
        Log.d("", "Connecting to " + hostname + ":" + Integer.toString(port));
        socket = new Socket(InetAddress.getByName(hostname), port);
        outputStream = socket.getOutputStream();
        inputStream = socket.getInputStream();
    }

    private void vpcdDisconnect() {
        Log.d("", "Disconnecting");
        if  (socket != null) {
            try {
                socket.close();
            } catch (IOException e) {
                // ignore
            } finally {
                socket = null;
            }
        }
    }
}
