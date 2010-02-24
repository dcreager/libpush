# CCAN talloc library

This directory contains the [CCAN talloc
library](http://ccan.ozlabs.org/info/talloc.html), which we use to
simplify the memory allocation of the parser callbacks in libpush.
The CCAN libraries are distributed under the LGPL, whereas libpush is
distributed under a BSD license.  To ensure that our code satisfies
the license requirements of the LGPL, the _ccan/ccan_ directory
contains an unmodified copy of the [talloc
download](http://ccan.ozlabs.org/tarballs/with-deps/talloc.tar.bz2).
The _ccan_ directory contains the extra include files and scons
scripts needed to compile the CCAN libraries using our build system.
It should be possible to upgrade to the latest version of the CCAN
libraries simply by replacing the _ccan/ccan_ directory with the
latest copy from the CCAN website.
