#!/bin/sh

set -e
set -x

export GDK_BACKEND=offscreen
export GSK_RENDERER=cairo

# Test with all libs enabled
unset GTKHASH_TEST_LIB
"$@"

for lib in ${HASH_LIBS} ; do
	# Test with one lib enabled
	export GTKHASH_TEST_LIB=${lib}
	"$@"
done