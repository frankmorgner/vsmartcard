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

.. sidebar:: Summary

    Smart card emulator written in Python

  :Authors:
      - `Frank Morgner <morgner@informatik.hu-berlin.de>`_
      - `Dominik Oepen <oepen@informatik.hu-berlin.de>`_
  :License:
      GPL version 3
  :Tested Platforms:
      - Linux (Debian, Ubuntu, OpenMoko)
      - Windows

Virtual Smart Card emulates a smart card and makes it accessible through PC/SC.
Currently the Virtual Smart Card supports the following types of smart cards:

- Generic ISO-7816 smart card including secure messaging
- German electronic identity card (nPA) with complete support for |EAC| (|PACE|, |TA|, |CA|)
- German electronic passport (ePass) with complete support for |BAC|
- Cryptoflex smart card (incomplete)
      
The |vpcd| is a smart card reader driver for PCSC-Lite_ and the windows smart
card service. It allows smart card applications to access the |vpicc| through
the PC/SC API.  By default |vpcd| opens slots for communication with multiple
|vpicc|'s on localhost from port 35963 to port 35972. But the |vpicc| does not
need to run on the same machine as the |vpcd|, they can connect over the
internet for example.

Although the Virtual Smart Card is a software emulator, you can use
:ref:`pcsc-relay` to make it accessible to an external contact-less smart card
reader.

The file :file:`utils.py` was taken from Henryk Plötz's cyberflex-shell_.

.. tikz:: Virtual Smart Card used with PCSC-Lite or WinSCard
    :stringsubst:
    :libs: arrows, calc, fit, patterns, plotmarks, shapes.geometric, shapes.misc, shapes.symbols, shapes.arrows, shapes.callouts, shapes.multipart, shapes.gates.logic.US, shapes.gates.logic.IEC, er, automata, backgrounds, chains, topaths, trees, petri, mindmap, matrix, calendar, folding, fadings, through, positioning, scopes, decorations.fractals, decorations.shapes, decorations.text, decorations.pathmorphing, decorations.pathreplacing, decorations.footprints, decorations.markings, shadows
 
    \input{%(wd)s/bilder/tikzstyles.tex}
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
        (vpcd) edge node {\includegraphics[width=1.2cm]{%(wd)s/bilder/simplecloud.pdf}} (vicc)
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
 
    \input{%(wd)s/bilder/tikzstyles.tex}
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
        (pcsclite) edge node {\includegraphics[width=1.5cm]{%(wd)s/bilder/simplecloud.pdf}} (vicc)
        ;
    \end{pgfonlayer}


.. include:: download.txt


.. _vicc_install:

.. include:: autotools.txt

Depending on your usage of the |vpicc| you might or might not need
the following:

- Python_
- pyscard_
- PyCrypto_
- PBKDF2_
- PIP_
- OpenPACE_ (nPA emulation)


================================================================================
Building and installing |vpcd| on Windows
================================================================================

.. versionadded:: 0.7
    We implemented |vpcd| as user mode device driver for Windows so that
    |vpicc| can directly be used in Windows' smart card applications that use
    PC/SC.

For the Windows integration we extended `Fabio Ottavi's UMDF Driver for a
Virtual Smart Card Reader`_ with a |vpcd| interface. To build |vpcd| for
Windows we use `Windows Driver Kit 8.1 and Visual Studio 2013`_:


1. In Visual Studio select :menuselection:`File --> Open --> Convert
   Sources/Dirs...` and choose the vpcd's :file:`sources` either in the
   :file:`WinXP` [#footnote1]_ or :file:`Win7` folder.

   When successfully imported, ensure with the configuration manager, that both
   of the created projects are built for the same platform (x64 or Win32).

2. If you can successfully :guilabel:`Build the solution`, you can find the
   install package in :file:`BixVReader-package`. It contains `BixVReader.inf`
   and the required libraries, especially `BixVReader.dll` and
   `WudfUpdate_01009.dll` [#footnote2]_.

3. Copy :file:`win32\\BixVReader\\BixVReader.ini` into the :envvar:`%SystemRoot%`
   directory.

4. In a console with administrator rights go to :file:`BixVReader-package` and
   install the driver::

   "C:\Program Files\Windows Kits\8.1\Tools\x86\devcon.exe" install BixVReader.inf root\BixVirtualReader
   
   You can adjust the path to ``devcon.exe`` with your version of the WDK and
   your target architecture (e.g., use ``...\x64\devcon.exe`` for a 64 bit
   driver).

   For Win7 and older, code signing is optional and will yield a warning during
   installation when missing. Simply click continue to install the driver anyway.

   To activate the WDK test signing, use VS build-in Driver Signing settings.
   Right click :guilabel:`BixVReader-package` :menuselection:`Properties -->
   Driver Signing --> Sign Mode --> Test Sign`.  Import the WDKTestCert
   certificate :file:`BixVReader-package.cer` into your windows keystore (e.g.
   on local computer) and then install the driver. See
   `Microsoft's Kernel-Mode Code Signing Walkthrough`_ for
   details.

For debugging |vpcd| and building the driver with an older version of Visual
Studio or WDK please see `Fabio Ottavi's UMDF Driver for a Virtual Smart Card
Reader`_ for details.


********************************************************************************
Using the Virtual Smart Card
********************************************************************************

The protocol between |vpcd| and |vpicc| as well as details on extending |vpicc|
with a different card emulator are covered in :ref:`virtualsmartcard-api`. Here
we will focus on configuring and running the provided modules.

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
Configuring |vpcd| on Windows
================================================================================

The configuration file `BixVReader.ini` of |vpcd| is usually placed into
:file:`C:\\Windows` (:envvar:`SystemRoot`). The user mode device driver
framework (:command:`WUDFHost.exe`) should read it automatically and load the
|vpcd| on startup. The Windows Device Manager :command:`mmc devmgmt.msc` should
list the :guilabel:`Bix Virtual Smart Card Reader`.

|vpcd| opens a socket for |vpicc| and waits for incoming
connections. The port to open should be specified in ``TCP_PORT``:

.. literalinclude:: BixVReader.ini
    :emphasize-lines: 8

Currently it is not possible to configure the Windows driver to connect to an
|vpicc| running with :option:`--reversed`.

================================================================================
Running |vpicc|
================================================================================

The command :command:`vicc --help` gives an overview about the command line
options of |vpicc|.

.. program-output:: vicc --help

.. versionadded:: 0.7
    We implemented :command:`vpcd-config` which tries to guess the local IP
    address and outputs |vpcd|'s configuration. |vpicc|'s options should be
    chosen accordingly (:option:`--hostname` and :option:`--port`)
    :command:`vpcd-config` also prints a QR code for configuration of the
    :ref:`remote-reader`.

On Windows you can start |vpicc| with :command:`python.exe src/vpicc/vicc.in`
or :command:`python.exe vicc`. Note emulating the German ID card
(:option:`--type nPA`) when running |vpicc| on Windows is currently not
supported, because OpenPACE's Python bindings are untested for Windows.
However, it is possible to connect an emulated ID card running on Linux to a
|vpcd| running on Windows.

When |vpcd| and |vpicc| are connected you should be able to access the card
through the PC/SC API. You can use the :command:`opensc-explorer` or
:command:`pcsc_scan` for testing. In Virtual Smart Card's root directory we also
provide scripts for testing with :ref:`libnpa` and PCSC-Lite's smart card
reader driver tester.


.. include:: questions.txt


********************
Notes and References
********************

.. target-notes::

.. [#footnote1] With VS 2013 and WDK 8.1 no Windows XP driver can be build. You need to use an older version of VS with WDK 7.1.0.
.. [#footnote2] Note that WudfUpdate_01009.dll for 32 bit will be around 1795 KB and for 64 bit around 2102 KB big.

.. _cyberflex-shell: https://github.com/henryk/cyberflex-shell
.. _PCSC-lite: http://pcsclite.alioth.debian.org/
.. _Python: http://www.python.org/
.. _pyscard: http://pyscard.sourceforge.net/
.. _PyCrypto: http://pycrypto.org/
.. _PBKDF2: https://www.dlitz.net/software/python-pbkdf2/
.. _PIP: http://www.pythonware.com/products/pil/
.. _OpenPACE: https://github.com/frankmorgner/openpace
.. _`Fabio Ottavi's UMDF Driver for a Virtual Smart Card Reader`: http://www.codeproject.com/Articles/134010/An-UMDF-Driver-for-a-Virtual-Smart-Card-Reader
.. _`Windows Driver Kit 8.1 and Visual Studio 2013`: http://msdn.microsoft.com/en-us/windows/hardware/hh852365.aspx
.. _`Microsoft's Kernel-Mode Code Signing Walkthrough`: http://msdn.microsoft.com/en-us/library/windows/hardware/dn653569%28v=vs.85%29.aspx
