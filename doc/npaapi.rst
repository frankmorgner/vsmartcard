.. highlight:: c

****************
nPA API Overview
****************

The nPA library includes a generic implementation for Secure Messaging (SM),
which might also be used in conjunction with other cards. It is implemented to
be close to ISO 7816-8 focusing only on encoding rather than implementing
everything that is needed to get a secure channel. All cryptographic work is
done by call back functions, which should be appropriatly set for the specific
card.

Using the German identity card (neuer Personalausweis, nPA) requires user
authentication via entry of the PIN. Transmitting the PIN in plaintext is
risky, since it would be transmitted over the air and could be snooped. That's
why the PACE keyagreement is used to verify the PIN and establish an SM channel
to the nPA. :npa:`EstablishPACEChannel` does exactly that and if everything
went fine, it initializes :npa:`sm_ctx` for use of the SM channel. Now
:npa:`sm_transmit_apdu` can be used to securely transmit arbitrary APDUs to the
card. You could for example change your PIN or even continue the Extended
Access Control (EAC) with Terminal Authentication (TA) and Chit Authenitcation
(CA).

Please consider the following overview to the API as incomplete. The `Doxygen
documentation <_static/doxygen-npa/modules.html>`_ should be used as programmer's
reference since it is more detailed.

=====================
Secure Messaging (SM)
=====================

The complete documentation can be found `here
<_static/doxygen-npa/group__sm.html>`_.

.. doxygenstruct:: sm_ctx
.. doxygenfunction:: sm_transmit_apdu
.. doxygenfunction:: sm_ctx_clear_free

==============================================================
Interface to German identity card (neuer Personalausweis, nPA)
==============================================================

The complete documentation can be found `here
<_static/doxygen-npa/group__npa.html>`_.

.. doxygenstruct:: establish_pace_channel_input
.. doxygenstruct:: establish_pace_channel_output
.. doxygenfunction:: EstablishPACEChannel
.. doxygenfunction:: npa_reset_retry_counter
.. doxygendefine:: npa_change_pin
.. doxygendefine:: npa_unblock_pin

=======
Example
=======

The following example are fragments of the npa-tool, which uses libnpa to acces
the nPA with and without SM enabled. First set up the environment:

    .. literalinclude:: ../npa/src/npa-tool.c
        :lines: 49-74,198-212

Connect to a reader and the nPA:

    .. literalinclude:: ../npa/src/npa-tool.c
        :lines: 331-341

Now we try to change the PIN. Therefor we need to establish a SM channel with
PACE. You could set your PIN with `pin = "123456"` or just leave it out to be
asked for it. The same applies to the new PIN `newpin`.

    .. literalinclude:: ../npa/src/npa-tool.c
        :lines: 484-501

Imagine you want to transmit additional APDUs in the established SM channel.
Declare the APDU to something like::

    const unsigned char buf[] = {0x00, 0xA4, 0x00, 0x0C, 0x02, 0x3F, 0x00};
    size_t apdulen = sizeof buf;`
    sc_apdu_t apdu;

Now parse and transmit the APDU with SM:

    .. literalinclude:: ../npa/src/npa-tool.c
        :lines: 171-173,175-183,185

Free up memory and wipe it if necessary (e.g. for keys stored in :npa:`sm_ctx`)

    .. literalinclude:: ../npa/src/npa-tool.c
        :lines: 563-

.. @author Frank Morgner <morgner@informatik.hu-berlin.de>
