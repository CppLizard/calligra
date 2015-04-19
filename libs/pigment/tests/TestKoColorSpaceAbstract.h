#ifndef TESTKOCOLORSPACEABSTRACT_H
#define TESTKOCOLORSPACEABSTRACT_H

#include <QtTest>

class TestKoColorSpaceAbstract : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testMixColorsOpU8();
    void testMixColorsOpF32();
    void testMixColorsOpU8NoAlpha();
};

#endif
