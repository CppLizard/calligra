/*
 *  Copyright (c) 2007 Boudewijn Rempt boud@valdyas.org
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "kis_kra_saver_test.h"

#include <qtest_kde.h>

#include <QBitArray>

#include <KisDocument.h>
#include <KoDocumentInfo.h>
#include <KoShapeContainer.h>
#include <KoPathShape.h>

#include "filter/kis_filter_registry.h"
#include "filter/kis_filter_configuration.h"
#include "filter/kis_filter.h"
#include "KisDocument.h"
#include "kis_image.h"
#include "kis_pixel_selection.h"
#include "kis_group_layer.h"
#include "kis_paint_layer.h"
#include "kis_clone_layer.h"
#include "kis_adjustment_layer.h"
#include "kis_shape_layer.h"
#include "kis_filter_mask.h"
#include "kis_transparency_mask.h"
#include "kis_selection_mask.h"
#include "kis_selection.h"
#include "kis_fill_painter.h"
#include "kis_shape_selection.h"
#include "util.h"
#include "testutil.h"

#include "kis_transform_mask_params_interface.h"


void KisKraSaverTest::testRoundTrip()
{
    KisDocument* doc = createCompleteDocument();
    KoColor bgColor(Qt::red, doc->image()->colorSpace());
    doc->image()->setDefaultProjectionColor(bgColor);
    doc->saveNativeFormat("roundtriptest.kra");
    QStringList list;
    KisCountVisitor cv1(list, KoProperties());
    doc->image()->rootLayer()->accept(cv1);

    KisDocument *doc2 = KisPart::instance()->createDocument();

    doc2->loadNativeFormat("roundtriptest.kra");

    KisCountVisitor cv2(list, KoProperties());
    doc2->image()->rootLayer()->accept(cv2);
    QCOMPARE(cv1.count(), cv2.count());

    // check whether the BG color is saved correctly
    QCOMPARE(doc2->image()->defaultProjectionColor(), bgColor);

    // test round trip of a transform mask
    KisNodeSP tnode =
        TestUtil::findNode(doc2->image()->rootLayer(), "testTransformMask");
    QVERIFY(tnode);
    KisTransformMask *tmask = dynamic_cast<KisTransformMask*>(tnode.data());
    QVERIFY(tmask);
    KisDumbTransformMaskParams *params = dynamic_cast<KisDumbTransformMaskParams*>(tmask->transformParams().data());
    QVERIFY(params);
    QTransform t = params->testingGetTransform();
    QCOMPARE(t, createTestingTransform());


    delete doc2;
    delete doc;



}

void KisKraSaverTest::testSaveEmpty()
{
    KisDocument* doc = createEmptyDocument();
    doc->saveNativeFormat("emptytest.kra");
    QStringList list;
    KisCountVisitor cv1(list, KoProperties());
    doc->image()->rootLayer()->accept(cv1);

    KisDocument *doc2 = KisPart::instance()->createDocument();
    doc2->loadNativeFormat("emptytest.kra");

    KisCountVisitor cv2(list, KoProperties());
    doc2->image()->rootLayer()->accept(cv2);
    QCOMPARE(cv1.count(), cv2.count());

    delete doc2;
    delete doc;
}
QTEST_KDEMAIN(KisKraSaverTest, GUI)
#include "kis_kra_saver_test.moc"
