#!/bin/sh
echo "Regenerating configure using autoconf..."
autoconf autoconf/configure.in > configure
autoheader autoconf/configure.in
rm -f config.cache
echo "Done."

                        