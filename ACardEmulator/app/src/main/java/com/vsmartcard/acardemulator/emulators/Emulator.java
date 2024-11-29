package com.vsmartcard.acardemulator.emulators;

public interface Emulator {

    void deactivate();

    void destroy();

    byte[] process(byte[] commandAPDU);

}
