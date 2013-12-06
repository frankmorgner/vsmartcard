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
      - Windows (|vpicc| natively, |vpcd| only via Cygwin)

Virtual Smart Card emulates a smart card and makes it accessible through PC/SC.
Currently the Virtual Smart Card supports the following types of smart cards:

- Generic ISO-7816 smart card including secure messaging
- German electronic identity card (nPA) with complete support for |EAC| (|PACE|, |TA|, |CA|)
- German electronic passport (ePass) with complete support for |BAC|
- Cryptoflex smart card (incomplete)
      
The |vpcd| is a smart card driver for PCSC-Lite_. It allows
smart card applications to access the |vpicc| through the PC/SC API.  By
default |vpcd| opens slots for communication with multiple |vpicc|'s on
localhost from port 35963 to port 35972. But the |vpicc| does not need to run
on the same machine as the |vpcd|, they can connect over the internet for
example.

Although the Virtual Smart Card is a software emulator, you can use
:ref:`pcsc-relay` to make it accessible to an external contact-less smart card
reader.

The file :file:`utils.py` was taken from Henryk Plötz's cyberflex-shell_.

.. tikz:: Virtual Smart Card used with PCSC-Lite
    :stringsubst:
    :libs: arrows, calc, fit, patterns, plotmarks, shapes.geometric, shapes.misc, shapes.symbols, shapes.arrows, shapes.callouts, shapes.multipart, shapes.gates.logic.US, shapes.gates.logic.IEC, er, automata, backgrounds, chains, topaths, trees, petri, mindmap, matrix, calendar, folding, fadings, through, positioning, scopes, decorations.fractals, decorations.shapes, decorations.text, decorations.pathmorphing, decorations.pathreplacing, decorations.footprints, decorations.markings, shadows
 
    \input{%(wd)s/bilder/tikzstyles.tex}
	\node (pcsclite)
    [klein, aktivbox, shape=rectangle split, rectangle split parts=3, inner xsep=3em]
	{PCSC-Lite
	\nodepart{second}
	\footnotesize \texttt{libpcsclite}
	\nodepart{third}
    \footnotesize \texttt{pcscd}
	};
    \node (pcsc) [box, kleiner, at=(pcsclite.two west)] {PC/SC};
    \node (sca) [aktivbox, klein, left=of pcsc, align=center] {Smart Card\\Application};
    \node (vpcd) [box, at=(pcsclite.three east)] {\texttt{vpcd}};
	\node (vicc) [aktivbox, right=of vpcd] {\texttt{vicc}};

    \begin{pgfonlayer}{background}
        \path[linie]
        (sca) edge (pcsc)
        (vpcd) edge (vicc)
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
    [aktivbox, shape=rectangle split, rectangle split parts=2, inner xsep=3em]
	{Virtual Smart Card
	\nodepart{second}
	\footnotesize \texttt{libpcsclite} with \texttt{vpcd}
	};
    \node (pcsc) [box, kleiner, at=(pcsclite.two west)] {PC/SC};
    \node (sca) [aktivbox, klein, left=of pcsc, align=center] {Smart Card\\Application};
	\node (vicc) [aktivbox, right=of pcsclite.two east] {\texttt{vicc}};

    \begin{pgfonlayer}{background}
        \path[linie]
        (sca) edge (pcsc)
        (pcsclite.two east) edge (vicc)
        ;
    \end{pgfonlayer}



.. include:: relay-note.txt


.. include:: download.txt


.. include:: autotools.txt

Depending on your usage of the |vpicc| you might or might not need
the following:

- Python_
- pyscard_
- PyCrypto_
- PBKDF2_
- PIP_
- OpenPACE_ (nPA emulation)


********************************************************************************
Running the Virtual Smart Card
********************************************************************************

The configuration file from |vpcd| is usually placed into
:file:`/etc/reader.conf.d/`. The PC/SC daemon should read it and load the
|vpcd| on startup. In debug mode :command:`pcscd -f -d` should say something
like "Attempting startup of Virtual PCD". For older versions of PCSC-Lite you
need to run :command:`update-reader.conf` to update :command:`pcscd`'s main
configuration file.

By default, |vpcd| opens a socket for |vpicc| and waits for incoming
connections.  The port to open should be specified in ``CHANNELID`` and
``DEVICENAME``:

.. literalinclude:: vpcd_example.conf
    :emphasize-lines: 2,4

If the first part of the ``DEVICENAME`` is different from ``/dev/null``, |vpcd|
will use this string as a hostname to connect to a waiting |vpicc|. |vpicc|
needs to be started with the ``--reversed`` flag in this case.

The command :command:`vicc --help` gives an overview about the command line
options of |vpicc|.

.. program-output:: vicc --help

When |vpcd| and |vpicc| are connected you should be able to access the card
through the PC/SC API via :command:`pcscd`. You can use the
:command:`opensc-explorer` or :command:`pcsc_scan` for testing. In
Virtual Smart Card's root directory we also provide scripts for testing with
:ref:`libnpa` and PCSC-Lite's smart card reader driver tester.


================================================================================
Accessing the Virtual Smart Card from Windows applications
================================================================================

Running |vpcd| under Windows is currently not supported, because it implements
a smart card driver specific for PCSC-Lite (:command:`pcscd`). This means, that
although you can run |vpicc| under Windows (for example in relay mode), it
can't be accessed by Windows' smart card applications.

However, there are several more or less complicated paths you can go:

- Run |vpcd|/:command:`pcscd` in Linux and use :ref:`pcsc-relay` to forward the
  |vpicc| via NFC to a contactless reader which is attached to the Windows
  machine. This has been tested tested with a ACR122U (touchatag).
- Run your windows machine as virtual machine in a Linux host. Then run
  |vpcd|/:command:`pcscd` in the Linux host and grab the |vpicc| with the
  :ref:`ccid-emulator`. Now forward the emulated USB device to the Windows VM.
  This is easy, but the :ref:`ccid-emulator` is not tested very well under
  Windows.
- Use the combination above (|vpcd|/:ref:`ccid-emulator` under Linux) on a
  device, that you can put in USB client mode. Then use a USB connector to
  physically connect the Linux machine in client mode to the Windows machine.
- Port the |vpcd| from PCSC-Lite to Windows' PC/SC service. Then, |vpcd| runs
  natively under Windows. Although |vpcd| is relatively simple and should be
  POSIX compliant, this could be a lot of work.
- Run |vpcd|/:command:`pcscd` under Cygwin on the Windows machine. This has
  been reported to work, but it is unclear whether you can use PCSC-Lite as
  PC/SC provider for a native Windows application.


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
.. _PIP: http://www.pythonware.com/products/pil/
.. _OpenPACE: http://openpace.sourceforge.net
