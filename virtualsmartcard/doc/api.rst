.. |vpicc| replace:: :abbr:`vpicc (virtual smart card)`
.. |vpcd| replace:: :abbr:`vpcd (virtual smart card reader)`

*****************************
Creating a virtual smart card
*****************************

============= ==================== ============= =============
|vpcd|                             |vpicc|
---------------------------------- ---------------------------
Length        Command              Length        Response
============= ==================== ============= =============
``0x00 0x01`` ``0x00`` (Power Off)               (No Response)
``0x00 0x01`` ``0x01`` (Power On)                (No Response)
``0x00 0x01`` ``0x02`` (Reset)                   (No Response)
``0x00 0x01`` ``0x04`` (Get ATR)   ``0xXX 0xXX`` (ATR)
``0xXX 0xXX`` (APDU)               ``0xXX 0xXX`` (R-APDU)
============= ==================== ============= =============

.. toctree::

   api/virtualsmartcard
