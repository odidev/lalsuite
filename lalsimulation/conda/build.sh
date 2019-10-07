#!/bin/bash

set -e

# when running on gitlab-ci, we are not using a production
# build, so we don't want to use NDEBUG
export CPPFLAGS="${CPPFLAGS} -UNDEBUG"

# only link libraries we actually use
export GSL_LIBS="-L${PREFIX}/lib -lgsl"

./configure \
	--prefix="${PREFIX}" \
	--enable-help2man \
	--enable-swig-iface \
	--disable-swig-octave \
	--disable-swig-python \
	--disable-python \
	--enable-silent-rules
make -j ${CPU_COUNT}
make -j ${CPU_COUNT} check
make install
