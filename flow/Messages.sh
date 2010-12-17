#! /bin/sh
$EXTRACTRC `find . -name \*.ui -o -name \*.rc` >> rc.cpp || exit 11
perl -n flow-extract-i18n-desc.pl `find . -name desc -o -name \*.sml` >> rc.cpp
$EXTRACTRC --tag-group=none --tag=name --context=Stencils `find . -name \*.shape` >> rc.cpp
$XGETTEXT `find . -name \*.cpp -not -name \*.moc.\*` src/part/FlowAboutData.h -o $podir/flow.pot

