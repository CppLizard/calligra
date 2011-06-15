#! /bin/sh
$EXTRACTRC --tag-group=calligra functions/*.xml > xml_doc.cpp
$EXTRACTRC chart/*.ui dialogs/*.ui  part/dialogs/*.ui *.kcfg *.rc >> rc.cpp
$XGETTEXT *.cpp chart/*.cpp commands/*.cpp database/*.cpp dialogs/*.cpp functions/*.cpp part/AboutData.h part/*.cpp part/commands/*.cpp part/dialogs/*.cpp ui/*.cpp -o $podir/tables.pot
rm xml_doc.cpp rc.cpp
