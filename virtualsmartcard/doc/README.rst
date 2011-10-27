.. highlight:: sh

.. _PBKDF2: https://www.dlitz.net/software/python-pbkdf2/
.. _PCSC-lite: http://pcsclite.alioth.debian.org/
.. _PCSC-lite: http://pcsclite.alioth.debian.org/
.. _PIP: http://www.pythonware.com/products/pil/
.. _PyCrypto: http://pycrypto.org/
.. _Python: http://www.python.org/
.. _cyberflex-shell: https://github.com/henryk/cyberflex-shell
.. _pyscard: http://pyscard.sourceforge.net/


******************
Virtual Smart Card
******************

:Authors:
    - Frank Morgner <morgner@informatik.hu-berlin.de>
    - Dominik Oepen <oepen@informatik.hu-berlin.de>
:License:
    GPL version 3
:Tested Platforms:
    - Linux (Debian, Ubuntu, OpenMoko)
    - Windows (only virtual smart card, not the virtual reader)

Welcome to virtualsmartcard. The purpose of virtualsmartcard is to emulate a
smart card and make it accessible through PCSC. Currently the virtual smart
card supports almost all commands of ISO-7816 including secure messaging.
Besides a plain ISO-7816 smart card it is also possible to emulate a German
ePass (only basic access control) and a rudimentary Cryptoflex smart card. The
virtual smart card (vpicc) can be accessed through the virtual smart card
reader (vpcd) which is a driver for ``pcscd`` of PCSC-Lite_.

By default the vicc communicates with the vpcd through a socket on localhost
port 35963. The file ``utils.py`` was taken from Henryk Plötz's
cyberflex-shell_.


------------
Installation
------------

virtualsmartcard uses the GNU Build System to compile and install. If you are
unfamiliar with it, please have a look at the file ``INSTALL``. If you have a
look around and can not find it, you are probably working bleeding edge in the
repository.  Run the following command in the virtualsmartcard directory to get
the missing standard auxiliary files::
    
    autoreconf -i

Depending on your usage of the vpicc you might or might not need
the following:

- Python_
- pyscard_
- PyCrypto_
- PBKDF2_
- PIP_

The vpcd has the following dependencies:

- PCSC-Lite_


------------------------
Running virtualsmartcard
------------------------

First you need to make sure that pcscd loads the vpcd. You might need to run
``update-reader.conf`` to update pcscd's configuration file. Then ``pcscd -f
-d`` should say something like ``Attempting startup of Virtual PCD``

Now you can run ``vicc`` which connects to the virtual reader. The
command ``vicc --help`` gives an overview about the command line
options.

You should now be able to access the vpicc through the system's
PC/SC API via vpcd/pcscd. You can use the opensc-explorer or pcsc_scan to test
that.


Question
---------

For questions, please use http://sourceforge.net/projects/vsmartcard/support
