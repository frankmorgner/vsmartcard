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

.. doxygenfile:: sm.h

==============================================================
Interface to German identity card (neuer Personalausweis, nPA)
==============================================================

.. doxygenfile:: npa.h

.. @author Frank Morgner <morgner@informatik.hu-berlin.de>
