.. highlight:: sh

.. |vsmartcard| replace:: Virtual Smart Card Architecture

Welcome to the |vsmartcard| documentation!
==========================================================

Welcome to the |vsmartcard|. |vsmartcard| is an umbrella project for various
projects concerned with the emulation of different types of smart card readers
or smart cards themselves.

.. image:: _static/vsmartcard.jpeg
    :alt: |vsmartcard| connects different aspects of smart card handling
    :scale: 50%
    :align: center

Currently the following projects are part of |vsmartcard|: 

.. toctree::
   :titlesonly:

   virtualsmartcard/README
   pcsc-relay/README
   ccid/README
   npa/README

Looking for a feature the included programs do not offer? Want to extend the
functionality of some library? Apart from requesting a new feature, you can
have a look at these programming guides and try yourself:

.. toctree::
   :maxdepth: 1

   virtualsmartcard/api
   npa/api


Example Use Cases
-----------------

Mobile Phone with Virtual Smartcard connected via NFC
*****************************************************

.. image:: _static/nfc-emulator.jpeg
    :alt: Overview
    :width: 320px

.. image:: _static/nfc-emulator-arch.jpeg
    :alt: Architecture
    :width: 320px

Mobile Phone with Virtual Smartcard connected via USB
*****************************************************

.. image:: _static/usb-emulator.jpeg
    :alt: Overview
    :width: 320px

.. image:: _static/usb-emulator-arch.jpeg
    :alt: Architecture
    :width: 320px


========
Download
========

You can download the latest release of the |vsmartcard| `here
<http://sourceforge.net/projects/vsmartcard/files>`_.

Alternatively, you can clone our git repository::

    git clone git://vsmartcard.git.sourceforge.net/gitroot/vsmartcard/vsmartcard
