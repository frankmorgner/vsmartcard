# Portuguese Electronic Identity Cards

Basic support for PTEID cards is added.
The cards cannot be used with the standard PKI and tools as the objects are signed by a self generated CA.
The Object structure, permissions and certificates tries to follow the same structure as original cards.

This is useful for development purposes, especially when using the PKCS11 interface.
Some objects may be inconsistent as they were copied from a development card. Further developments will focus on properly emulating PTEID cards.

