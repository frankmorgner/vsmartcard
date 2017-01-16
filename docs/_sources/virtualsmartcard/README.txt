.. highlight:: sh

.. |vpicc| replace:: :abbr:`vpicc (virtual smart card)`
.. |vpcd| replace:: :abbr:`vpcd (virtual smart card reader)`
.. |BAC| replace:: :abbr:`BAC (Basic Access Control)`
.. |PACE| replace:: :abbr:`PACE (Password Authenticated Connection Establishment)`
.. |TA| replace:: :abbr:`TA (Terminal Authenticatation)`
.. |CA| replace:: :abbr:`CA (Chip Authentication)`
.. |EAC| replace:: :abbr:`EAC (Extended Access Control)`

.. _vicc:

################################################################################
Virtual Smart Card
################################################################################

.. sidebar:: Smart card emulator written in Python

  :License:
      GPL version 3
  :Tested Platforms:
      - Windows
      - macOS
      - Linux (Debian, Ubuntu, OpenMoko)

Virtual Smart Card emulates a smart card and makes it accessible through PC/SC.
Currently the Virtual Smart Card supports the following types of smart cards:

- Generic ISO-7816 smart card including secure messaging
- German electronic identity card (nPA) with complete support for |EAC|
  (|PACE|, |TA|, |CA|)
- Electronic passport (ePass/MRTD) with support for |BAC|
- Cryptoflex smart card (incomplete)
      
The |vpcd| is a smart card reader driver for PCSC-Lite_ and the windows smart
card service. It allows smart card applications to access the |vpicc| through
the PC/SC API.  By default |vpcd| opens slots for communication with multiple
|vpicc|'s on localhost on port 35963 and port 35964. But the |vpicc| does not
need to run on the same machine as the |vpcd|, they can connect over the
internet for example.

Although the Virtual Smart Card is a software emulator, you can use
:ref:`pcsc-relay` to make it accessible to an external contact-less smart card
reader.

The file :file:`utils.py` was taken from Henryk Plötz's cyberflex-shell_.

.. tikz:: Virtual Smart Card used with PCSC-Lite or WinSCard
    :stringsubst:
    :libs: arrows, calc, fit, patterns, plotmarks, shapes.geometric, shapes.misc, shapes.symbols, shapes.arrows, shapes.callouts, shapes.multipart, shapes.gates.logic.US, shapes.gates.logic.IEC, er, automata, backgrounds, chains, topaths, trees, petri, mindmap, matrix, calendar, folding, fadings, through, positioning, scopes, decorations.fractals, decorations.shapes, decorations.text, decorations.pathmorphing, decorations.pathreplacing, decorations.footprints, decorations.markings, shadows
 
    \input{$wd/bilder/tikzstyles.tex}
	\node (pcsclite)
    [klein, aktivbox, inner xsep=.7cm, text width=3cm]
	{PC/SC Framework\\\
    (\texttt{pcscd} or \texttt{SCardSvr})
	};
    \node (sca) [aktivbox, klein, left=of pcsclite] {Smart Card\\Application};
    \node (vpcd) [box, at=(pcsclite.east), xshift=-.3cm] {\texttt{vpcd}};
	\node (vicc) [aktivbox, right=2cm of pcsclite] {\texttt{vicc}};

    \begin{pgfonlayer}{background}
        \path[linie]
        (sca) edge (pcsclite)
        (vpcd) edge node {\includegraphics[width=1.2cm]{$wd/bilder/simplecloud.pdf}} (vicc)
        ;
    \end{pgfonlayer}

.. versionadded:: 0.7
    The Virtual Smart Card optionally brings its own standalone implementation of
    PC/SC. This allows accessing |vpicc| without PCSC-Lite. Our PC/SC
    implementation acts as replacement for ``libpcsclite`` which can lead to
    problems when used in parallel with PCSC-Lite.

.. tikz:: Virtual Smart Card used with its own PC/SC implementation
    :stringsubst:
    :libs: arrows, calc, fit, patterns, plotmarks, shapes.geometric, shapes.misc, shapes.symbols, shapes.arrows, shapes.callouts, shapes.multipart, shapes.gates.logic.US, shapes.gates.logic.IEC, er, automata, backgrounds, chains, topaths, trees, petri, mindmap, matrix, calendar, folding, fadings, through, positioning, scopes, decorations.fractals, decorations.shapes, decorations.text, decorations.pathmorphing, decorations.pathreplacing, decorations.footprints, decorations.markings, shadows
 
    \input{$wd/bilder/tikzstyles.tex}
	\node (pcsclite)
    [klein, box, text width=4cm]
	{Virtual Smart Card's\\
    PC/SC Framework
	};
    \node (sca) [aktivbox, klein, left=of pcsclite] {Smart Card\\Application};
	\node (vicc) [aktivbox, right=2cm of pcsclite] {\texttt{vicc}};

    \begin{pgfonlayer}{background}
        \path[linie]
        (sca) edge (pcsclite)
        (pcsclite) edge node {\includegraphics[width=1.5cm]{$wd/bilder/simplecloud.pdf}} (vicc)
        ;
    \end{pgfonlayer}

On Android, where a traditional PC/SC framework is not available, you can use
our framework to make your real contact-less smart accessible through PKCS#11.
For example, an email signing application can use the PKCS#11 interface of
OpenSC, which is linked against our PC/SC implementation. Then an Android App
(e.g. :ref:`remote-reader`) can connect as |vpicc| delegating all requests and
responses via NFC to a contact-less smart card that signs the mail.

Depending on your usage of the |vpicc| you may need to install the following:

- Python_
- pyscard_ (relaying a local smart card with :option:`--type=relay`)
- PyCrypto_, PBKDF2_, PIL_, readline_ or PyReadline_ (emulation of electronic
  passport with :option:`--type=ePass`)
- OpenPACE_ (emulation of German identity card with :option:`--type=nPA`)
- libqrencode_ (to print a QR code on the command line for `vpcd-config`; an
  URL will be printed if libqrencode is not available)


.. include:: download.txt


.. _vicc_install:

.. include:: autotools.txt


================================================================================
Building and installing |vpcd| on Mac OS X
================================================================================

Mac OS X 10.9 and earlier is using PCSC-Lite as smart card service which allows
using the standard routine for :ref:`installation on Unix<vicc_install>`.

Mac OS X 10.10 (and later) ships with a proprietary implementation of the PC/SC
layer instead of with PCSC-Lite. As far as we know, this means that smart card
readers must be USB devices instead of directly allowing a more generic type of
reader. To make |vpcd| work we simply configure it to pretend being a USB smart
card reader with an :file:`Ìnfo.plist`::

    ./configure --prefix=/ --enable-infoplist
    make
    make install

================================================================================
Building and installing |vpcd| on Windows
================================================================================

.. versionadded:: 0.7
    We implemented |vpcd| as user mode device driver for Windows so that
    |vpicc| can directly be used in Windows' smart card applications that use
    PC/SC.

For the Windows integration we extended `Fabio Ottavi's UMDF Driver for a
Virtual Smart Card Reader`_ with a |vpcd| interface. To build |vpcd| for
Windows we use `Windows Driver Kit 10 and Visual Studio 2015`_. The vpcd
installer requires the `WiX Toolset 3.10`_. If you choose
to download the `Windows binaries`_, you may directly jump to step 4.

1. Clone the git repository and make sure it is initialized with all
   submodules::

    git clone https://github.com/frankmorgner/vsmartcard.git
    cd vsmartcard
    git submodule update --init --recursive

2. In Visual Studio open |vpcd|'s solution
   :file:`virtualsmartcard\\win32\\BixVReader.sln` and  ensure with the
   configuration manager, that the project is built for your platform (i.e.
   ``x64`` or ``x82``).

3. If you can successfully :guilabel:`Build the solution`, you can find
   the installer (:file:`BixVReaderInstaller.msi`) in
   :file:`virtualsmartcard\\win32\\BixVReaderInstaller\\bin\\*Release`

4. To install |vpcd|, double click :file:`BixVReaderInstaller.msi`. Since we
   are currently not signing the Installer, this will yield a warning about an
   unverified driver software publisher on Windows 8 and later. Click
   :guilabel:`Install this driver software anyway`.

For debugging |vpcd| and building the driver with an older version of Visual
Studio or WDK please see `Fabio Ottavi's UMDF Driver for a Virtual Smart Card
Reader`_ for details.

All of Fabio's card connectors are still available, but inactive by default
(see `Configuring vpcd on Windows`_ below).


********************************************************************************
Using the Virtual Smart Card
********************************************************************************

The protocol between |vpcd| and |vpicc| as well as details on extending |vpicc|
with a different card emulator are covered in :ref:`virtualsmartcard-api`. Here
we will focus on configuring and running the provided modules.

.. _vicc_config:

================================================================================
Configuring |vpcd| on Unix
================================================================================

The configuration file of |vpcd| is usually placed into
:file:`/etc/reader.conf.d/`. For older versions of PCSC-Lite you
need to run :command:`update-reader.conf` to update :command:`pcscd`'s main
configuration file. The PC/SC daemon should read it and load the
|vpcd| on startup. In debug mode :command:`pcscd -f -d` should say something
like "Attempting startup of Virtual PCD" when loading |vpcd|.

By default, |vpcd| opens a socket for |vpicc| and waits for incoming
connections.  The port to open should be specified in ``CHANNELID`` and
``DEVICENAME``:

.. literalinclude:: vpcd_example.conf
    :emphasize-lines: 2,4

If the first part of the ``DEVICENAME`` is different from ``/dev/null``, |vpcd|
will use this string as a hostname for connecting to a waiting |vpicc|. |vpicc|
needs to be started with :option:`--reversed` in this case.

================================================================================
Configuring |vpcd| on Mac OS X
================================================================================

Mac OS X 10.9 and earlier is using PCSC-Lite as smart card service which allows
using the standard routine for :ref:`configuration on Unix<vicc_config>`.

On Mac OS X 10.10 you should have configured the generation of
:file:`Info.plist` at compile time. Now do the following for registering |vpcd|
as USB device:

1. Choose an USB device (e.g. mass storage, phone, mouse, ...), which will be
   used to start |vpcd|. Plug it into the computer.

2. Run the following command to get the device's product and vendor ID::

    system_profiler SPUSBDataType

3. Change :file:`/usr/libexec/SmartCardServices/drivers/ifd-vpcd.bundle/Info.plist`
   to match your product and vendor ID:

.. literalinclude:: Info.plist
    :emphasize-lines: 34,39

Note that ``ifdFriendlyName`` can be used in the same way as ``DEVICENAME``
:ref:`described above<vicc_config>`.

4. Restart the PC/SC service::

    sudo killall -SIGKILL -m .*com.apple.ifdreader

Now, every time you plug in your USB device |vpcd| will be started. It will be
stopped when you unplug the device.

================================================================================
Configuring |vpcd| on Windows
================================================================================

The configuration file :file:`BixVReader.ini` of |vpcd| is installed to
:file:`C:\\Windows` (:envvar:`%SystemRoot%`). The user mode device driver
framework (:command:`WUDFHost.exe`) should read it automatically and load the
|vpcd| on startup. The Windows Device Manager :command:`mmc devmgmt.msc` should
list the :guilabel:`Bix Virtual Smart Card Reader`.

|vpcd| opens a socket for |vpicc| and waits for incoming connections. The port
to open should be specified in ``TCP_PORT``:

.. literalinclude:: ../../virtualsmartcard/win32/BixVReader/BixVReader.ini
    :emphasize-lines: 8

Currently it is not possible to configure the Windows driver to connect to an
|vpicc| running with :option:`--reversed`.

================================================================================
Running |vpicc|
================================================================================

The compiled `Windows binaries`_ of |vpicc| include OpenPACE. The other
dependencies listed above need to be installed seperately. You can start the
|vpicc| via :command:`python.exe vicc.py`. On all other systems an executable
script :command:`vicc` is installed using the autotools.

The |vpicc| option :option:`--help` gives an overview about the command line
switches:

.. program-output:: vicc --help

.. versionadded:: 0.7
    We implemented :command:`vpcd-config` which tries to guess the local IP
    address and outputs |vpcd|'s configuration. |vpicc|'s options should be
    chosen accordingly (:option:`--hostname` and :option:`--port`)
    :command:`vpcd-config` also prints a QR code for configuration of the
    :ref:`remote-reader`.

When |vpcd| and |vpicc| are connected you should be able to access the card
through the PC/SC API. You can use the :command:`opensc-explorer` or
:command:`pcsc_scan` for testing. In Virtual Smart Card's root directory we also
provide scripts for testing with npa-tool_ and PCSC-Lite's smart card
reader driver tester.


.. include:: questions.txt


********************
Notes and References
********************

.. target-notes::

.. _cyberflex-shell: https://github.com/henryk/cyberflex-shell
.. _PCSC-lite: http://pcsclite.alioth.debian.org/
.. _Python: http://www.python.org/
.. _pyscard: http://pyscard.sourceforge.net/
.. _PyCrypto: http://pycrypto.org/
.. _PBKDF2: https://www.dlitz.net/software/python-pbkdf2/
.. _readline: https://docs.python.org/3.3/library/readline.html
.. _PyReadline: https://pypi.python.org/pypi/pyreadline
.. _PIL: http://www.pythonware.com/products/pil/
.. _OpenPACE: https://github.com/frankmorgner/openpace
.. _libqrencode: https://fukuchi.org/works/qrencode/
.. _`Fabio Ottavi's UMDF Driver for a Virtual Smart Card Reader`: http://www.codeproject.com/Articles/134010/An-UMDF-Driver-for-a-Virtual-Smart-Card-Reader
.. _`Windows Driver Kit 10 and Visual Studio 2015`: https://msdn.microsoft.com/en-us/library/windows/hardware/ff557573
.. _`WiX Toolset 3.10`: https://wixtoolset.org/releases/v3.10/stable
.. _`Windows binaries`: https://github.com/frankmorgner/vsmartcard/releases/download/virtualsmartcard-0.7/virtualsmartcard-0.7_win32.zip
.. _npa-tool: https://github.com/frankmorgner/OpenSC
