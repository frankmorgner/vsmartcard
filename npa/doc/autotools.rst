.. highlight:: sh

=============
Installation
=============

The nPA smart card library uses the GNU Build System to compile and install. If you are
unfamiliar with it, please have a look at :file:`INSTALL`. If you have a look
around and can not find it, you are probably working bleeding edge in the
repository.  Run the following command in :file:`npa` to
get the missing standard auxiliary files::
    
    autoreconf -i

To configure (:command:`configure --help` lists possible options), build and
install the nPA smart card library now do the following::
    
    ./configure
    make
    make install
