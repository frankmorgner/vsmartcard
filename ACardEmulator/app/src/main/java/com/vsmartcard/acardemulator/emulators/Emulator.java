package com.vsmartcard.acardemulator.emulators;

public interface Emulator {

    public void deactivate();

    public void destroy();

    public byte[] process(byte[] commandAPDU);

}
