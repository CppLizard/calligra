#! /bin/sh
source ../../calligra_xgettext.sh

$EXTRACTRC `find . -name \*.ui` >> rc.cpp || exit 11
calligra_xgettext VideoShape.pot `find . -name \*.cpp`
rm -f rc.cpp
