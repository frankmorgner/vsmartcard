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

import android.annotation.TargetApi;
import android.app.Activity;
import android.app.PendingIntent;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Intent;
import android.net.Uri;
import android.nfc.NfcAdapter;
import android.nfc.Tag;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.support.annotation.NonNull;
import android.text.Editable;
import android.text.Layout;
import android.text.method.ScrollingMovementMethod;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import com.vsmartcard.remotesmartcardreader.app.screaders.DummyReader;
import com.vsmartcard.remotesmartcardreader.app.screaders.NFCReader;
import com.vsmartcard.remotesmartcardreader.app.screaders.SCReader;

@TargetApi(Build.VERSION_CODES.KITKAT)
public class MainActivity extends Activity implements NfcAdapter.ReaderCallback {

    class VPCDHandler extends Handler {
        VPCDHandler(Looper looper) {
            super(looper);
        }
        @Override
        public void handleMessage(Message message) {
            switch (message.what) {
                case MessageSender.MESSAGE_CONNECTED:
                    textViewVPCDStatus.append(getResources().getString(R.string.status_connected_to)+" "+message.obj+"\n");
                    spinner.setVisibility(View.INVISIBLE);
                    updateLabels();
                    break;
                case MessageSender.MESSAGE_DISCONNECTED:
                    textViewVPCDStatus.append(getResources().getString(R.string.status_disconnected)+"\n");
                    testing = false;
                    updateLabels();
                    break;
                case MessageSender.MESSAGE_ATR:
                    textViewVPCDStatus.append(getResources().getString(R.string.status_atr)+": "+message.obj+"\n");
                    break;
                case MessageSender.MESSAGE_ON:
                    textViewVPCDStatus.append(getResources().getString(R.string.status_on)+"\n");
                    break;
                case MessageSender.MESSAGE_OFF:
                    textViewVPCDStatus.append(getResources().getString(R.string.status_off)+"\n");
                    break;
                case MessageSender.MESSAGE_RESET:
                    textViewVPCDStatus.append(getResources().getString(R.string.status_reset)+"\n");
                    break;
                case MessageSender.MESSAGE_CAPDU:
                    textViewVPCDStatus.append(getResources().getString(R.string.status_capdu)+": "+message.obj+"\n");
                    break;
                case MessageSender.MESSAGE_RAPDU:
                    textViewVPCDStatus.append(getResources().getString(R.string.status_rapdu)+": "+message.obj+"\n");
                    break;
                case MessageSender.MESSAGE_ERROR:
                    if (testing) {
                        textViewVPCDStatus.append(getResources().getString(R.string.status_error)+": "+message.obj+"\n");
                        testing = false;
                        vpcdDisconnect();
                        updateLabels();
                    }
                    break;
                default:
                    textViewVPCDStatus.append(getResources().getString(R.string.status_unknown)+" "+Integer.toString(message.what)+"\n");
                    break;
            }
            Layout layout = textViewVPCDStatus.getLayout();
            if (layout != null) {
                int scrollAmount = layout.getLineTop(textViewVPCDStatus.getLineCount()) - textViewVPCDStatus.getHeight();
                if (scrollAmount > 0)
                    textViewVPCDStatus.scrollTo(0, scrollAmount);
                else
                    textViewVPCDStatus.scrollTo(0, 0);
            }
        }
    }

    private EditText editTextVPCDHost;
    private EditText editTextVPCDPort;
    private TextView textViewVPCDStatus;
    private Button button;
    private ProgressBar spinner;
    private VPCDHandler handler;
    private VPCDWorker vpcdTest;
    private boolean testing;
    private String hostname;
    private String port;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        editTextVPCDHost = (EditText) findViewById(R.id.editTextHostname);
        editTextVPCDPort = (EditText) findViewById(R.id.editTextPort);
        textViewVPCDStatus = (TextView) findViewById(R.id.textViewLog);
        textViewVPCDStatus.setMovementMethod(new ScrollingMovementMethod());
        spinner = (ProgressBar) findViewById(R.id.progressBar);
        button = (Button) findViewById(R.id.buttonDisConnect);

        EditText.OnEditorActionListener vpcdWatcher = new EditText.OnEditorActionListener() {
            @Override
            public boolean onEditorAction(TextView textView, int i, KeyEvent keyEvent) {
                if (i == EditorInfo.IME_ACTION_DONE) {
                    editTextOnEditorDone();
                    return true;
                }
                return false;
            }
        };

        editTextVPCDHost.setOnEditorActionListener(vpcdWatcher);
        editTextVPCDPort.setOnEditorActionListener(vpcdWatcher);

        handler = new VPCDHandler(Looper.getMainLooper());

        PendingIntent.getActivity(this, 0, new Intent(this, this.getClass()).addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP), 0);

        loadSettings();
        updateLabels();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.main, menu);

        return super.onCreateOptionsMenu(menu);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle presses on the action bar items
        switch (item.getItemId()) {
            case R.id.action_copy:
                // Code to Copy the content of Text View to the Clip board.
                ClipboardManager clipboard = (ClipboardManager) getSystemService(CLIPBOARD_SERVICE);
                ClipData clip = ClipData.newPlainText("simple text", textViewVPCDStatus.getText());
                clipboard.setPrimaryClip(clip);
                Toast.makeText(getApplicationContext(), "Log copied to clipboard.",
                        Toast.LENGTH_LONG).show();
                return true;
            case R.id.action_delete:
                textViewVPCDStatus.setText("");
                return true;
            default:
                return super.onOptionsItemSelected(item);
        }
    }

    private void enableReaderMode() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
            Bundle bundle = new Bundle();
            bundle.putInt(NfcAdapter.EXTRA_READER_PRESENCE_CHECK_DELAY, NFCReader.TIMEOUT*10);
            NfcAdapter.getDefaultAdapter(this).enableReaderMode(this, this,
                    NfcAdapter.FLAG_READER_NFC_A | NfcAdapter.FLAG_READER_NFC_B | NfcAdapter.FLAG_READER_SKIP_NDEF_CHECK,
                    bundle);
        } else {
            //NfcAdapter.enableForegroundDispatch(this, mPendingIntent, mFilters, mTechLists);
        }
    }

    @Override
    public void onTagDiscovered(Tag tag) {
        NFCReader nfcReader = NFCReader.get(tag);
        if (nfcReader != null) {
            /* avoid updating UI components since this may end up on a non ui thread */
            vpcdDisconnect();
            testing = true;
            vpcdConnect(nfcReader);
        }
    }

    private void disableReaderMode() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
            NfcAdapter nfc = NfcAdapter.getDefaultAdapter(this);
            if (nfc != null) {
                nfc.disableReaderMode(this);
            }
        }
    }

    private static final String saved_status_key = "textViewVPCDStatus";
    @Override
    public void onRestoreInstanceState(@NonNull Bundle savedInstanceState) {
        super.onRestoreInstanceState(savedInstanceState);
        textViewVPCDStatus.setText(savedInstanceState.getCharSequence(saved_status_key));
    }
    @Override
    public void onSaveInstanceState(Bundle savedInstanceState) {
        savedInstanceState.putCharSequence(saved_status_key, textViewVPCDStatus.getText());
        super.onSaveInstanceState(savedInstanceState);
    }

    @Override
     public void onPause() {
        super.onPause();

        testing = false;
        vpcdDisconnect();
        disableReaderMode();
    }

    public void buttonOnClickDisConnect(View view) {
        if (testing) {
            testing = false;
            vpcdDisconnect();
        } else {
            saveSettings();
            testing = true;
            spinner.setVisibility(View.VISIBLE);
            textViewVPCDStatus.append(getResources().getString(R.string.status_dummy_start)+"\n");
            vpcdConnect(new DummyReader());
        }
        updateLabels();
    }

    private void editTextOnEditorDone() {
        textViewVPCDStatus.append(getResources().getString(R.string.status_dummy_start)+"\n");
        forceConnect(new DummyReader());
    }

    private void vpcdConnect(SCReader scReader) {
        int p = VPCDWorker.DEFAULT_PORT;
        try {
            p = Integer.parseInt(port);
        } catch (NumberFormatException e) {
            textViewVPCDStatus.append(getResources().getString(R.string.status_default_port)+"\n");
        }
        vpcdTest = new VPCDWorker(hostname, p, scReader, handler);
        new Thread(vpcdTest).start();
    }

    private void vpcdDisconnect() {
        if (vpcdTest != null) {
            vpcdTest.close();
            vpcdTest = null;
        }
    }

    private void updateLabels() {
        if (testing) {
            button.setText(getResources().getString(R.string.button_stop_testing));
        } else {
            button.setText(getResources().getString(R.string.button_test_configuration));
            spinner.setVisibility(View.INVISIBLE);
        }
    }

    private void loadSettings() {
        Settings settings = new Settings(this);
        hostname = settings.getVPCDHost();
        port = settings.getVPCDPort();
        editTextVPCDHost.setText(hostname);
        editTextVPCDPort.setText(port);
    }

    private void saveSettings() {
        Editable textHostname = editTextVPCDHost.getText();
        Editable textPort = editTextVPCDPort.getText();
        if (textHostname != null) {
            hostname = textHostname.toString();
        }
        if (textPort != null) {
            port = textPort.toString();
        }
        Settings settings = new Settings(this);
        settings.setVPCDSettings(hostname, port);
    }

    private void forceConnect(SCReader scReader) {
        if (testing) {
            testing = false;
            vpcdDisconnect();
        }
        saveSettings();
        testing = true;
        spinner.setVisibility(View.VISIBLE);
        vpcdConnect(scReader);
        updateLabels();
    }

    @Override
    public void onResume() {
        super.onResume();
        Intent intent = getIntent();
        // Check to see that the Activity started due to a discovered tag
        if (NfcAdapter.ACTION_TECH_DISCOVERED.equals(intent.getAction())) {
            NFCReader nfcReader = NFCReader.get(intent);
            if (nfcReader != null) {
                forceConnect(nfcReader);
            } else {
                super.onNewIntent(intent);
            }
        } else {
            // Check to see that the Activity started due to a configuration URI
            if (Intent.ACTION_VIEW.equals(intent.getAction())) {
                testing = false;
                vpcdDisconnect();
                Uri uri = intent.getData();
                String h = "";
                String p = "";
                if (uri.getScheme().equals(getResources().getString(R.string.scheme_vpcd))) {
                    h = uri.getHost();
                    int _p = uri.getPort();
                    if (_p > 0) {
                        p = Integer.toString(_p);
                    }
                } else {
                    h = uri.getQueryParameter("host");
                    p = uri.getQueryParameter("port");
                }
                editTextVPCDHost.setText(h);
                editTextVPCDPort.setText(p);
                super.onNewIntent(intent);
            }
        }
        enableReaderMode();
    }

    @Override
    public void onNewIntent(Intent intent) {
        // onResume gets called after this to handle the intent
        setIntent(intent);
    }
}