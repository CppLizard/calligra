/* This file is part of the KDE project
 * Copyright (C) 2005 David Faure <faure@kde.org>
 * Copyright (C) 2007-2009 Thomas Zander <zander@kde.org>
 * Copyright (C) 2007-2008 Sebastian Sauer <mail@dipe.org>
 * Copyright (C) 2007-2008 Pierre Ducroquet <pinaraf@gmail.com>
 * Copyright (C) 2007-2008 Thorsten Zachmann <zachmann@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "KWOdfWriter.h"
#include "KWDocument.h"
#include <rdf/KoDocumentRdfBase.h>
#include "KWPage.h"

#include <ktemporaryfile.h>

#include "frames/KWTextFrameSet.h"
#include "frames/KWTextFrame.h"
#include <KoXmlWriter.h>
#include <KoOdfWriteStore.h>
#include <KoShapeSavingContext.h>

#include <KoTextShapeData.h>
#include <KoStyleManager.h>
#include <KoParagraphStyle.h>
#include <KoShapeGroup.h>
#include <KoShapeLayer.h>

#include <KoGenChanges.h>
#include <KoTextSharedSavingData.h>

#include <KoStoreDevice.h>

#include <QBuffer>
#include <QTextCursor>
#include <KDebug>

QByteArray KWOdfWriter::serializeHeaderFooter(KoEmbeddedDocumentSaver &embeddedSaver, KoGenStyles &mainStyles, KoGenChanges  &changes, KWTextFrameSet *fs)
{
    QByteArray tag;
    switch (fs->textFrameSetType()) {
    case KWord::OddPagesHeaderTextFrameSet:  tag = "style:header";       break;
    case KWord::EvenPagesHeaderTextFrameSet: tag = "style:header-left";  break;
    case KWord::OddPagesFooterTextFrameSet:  tag = "style:footer";       break;
    case KWord::EvenPagesFooterTextFrameSet: tag = "style:footer-left";  break;
    default: return QByteArray();
    }

    QByteArray content;
    QBuffer buffer(&content);
    buffer.open(QIODevice::WriteOnly);
    KoXmlWriter writer(&buffer);

    KoShapeSavingContext context(writer, mainStyles, embeddedSaver);

    KoTextSharedSavingData *sharedData = new KoTextSharedSavingData;
    sharedData->setGenChanges(changes);
    context.addSharedData(KOTEXT_SHARED_SAVING_ID, sharedData);

    Q_ASSERT(!fs->frames().isEmpty());
    KoTextShapeData *shapedata = qobject_cast<KoTextShapeData *>(fs->frames().first()->shape()->userData());
    Q_ASSERT(shapedata);

    writer.startElement(tag);
    shapedata->saveOdf(context, 0, -1);
    writer.endElement();

    return content;
}

// rename to save pages ?
void KWOdfWriter::saveHeaderFooter(KoEmbeddedDocumentSaver &embeddedSaver, KoGenStyles &mainStyles, KoGenChanges &changes)
{
    //kDebug(32001)<< "START saveHeaderFooter ############################################";
    // first get all the framesets in a nice quick-to-access data structure
    // this avoids iterating till we drop
    QHash<KWPageStyle, QHash<int, KWTextFrameSet*> > data;
    foreach (KWFrameSet *fs, m_document->frameSets()) {
        KWTextFrameSet *tfs = dynamic_cast<KWTextFrameSet*> (fs);
        if (! tfs)
            continue;
        if (! KWord::isAutoGenerated(tfs))
            continue;
        if (tfs->textFrameSetType() == KWord::MainTextFrameSet)
            continue;
        QHash<int, KWTextFrameSet*> set = data.value(tfs->pageStyle());
        set.insert(tfs->textFrameSetType(), tfs);
        Q_ASSERT(tfs->pageStyle().isValid());
        data.insert(tfs->pageStyle(), set);
    }

    // save page styles that don't have a header or footer which will be handled later
    foreach (KWPageStyle pageStyle, m_document->pageManager()->pageStyles()) {
        if (data.contains(pageStyle))
            continue;

        KoGenStyle masterStyle(KoGenStyle::MasterPageStyle);
        KoGenStyle layoutStyle = pageStyle.saveOdf();
        masterStyle.addProperty("style:page-layout-name", mainStyles.insert(layoutStyle, "pm"));
        QString name = mainStyles.insert(masterStyle, pageStyle.name(), KoGenStyles::DontAddNumberToName);
        m_masterPages.insert(pageStyle, name);
    }

    // We need to flush them out ordered as defined in the specs.
    QList<KWord::TextFrameSetType> order;
    order << KWord::OddPagesHeaderTextFrameSet
          << KWord::EvenPagesHeaderTextFrameSet
          << KWord::OddPagesFooterTextFrameSet
          << KWord::EvenPagesFooterTextFrameSet;

    foreach (KWPageStyle pageStyle, data.keys()) {
        KoGenStyle masterStyle(KoGenStyle::MasterPageStyle);
        //masterStyle.setAutoStyleInStylesDotXml(true);
        KoGenStyle layoutStyle = pageStyle.saveOdf();
        masterStyle.addProperty("style:page-layout-name", mainStyles.insert(layoutStyle, "pm"));

        QHash<int, KWTextFrameSet*> headersAndFooters = data.value(pageStyle);
        int index = 0;
        foreach (int type, order) {
            if (! headersAndFooters.contains(type))
                continue;
            KWTextFrameSet *fs = headersAndFooters.value(type);
            Q_ASSERT(fs);
            if (fs->frameCount() == 0) // don't save empty framesets
                continue;

            QByteArray content = serializeHeaderFooter(embeddedSaver, mainStyles, changes, fs);

            if (content.isNull())
                continue;

            masterStyle.addChildElement(QString::number(++index), content);
        }
        // append the headerfooter-style to the main-style
        if (! masterStyle.isEmpty()) {
            QString name = mainStyles.insert(masterStyle, pageStyle.name(), KoGenStyles::DontAddNumberToName);
            m_masterPages.insert(pageStyle, name);
        }
    }

    //foreach (KoGenStyles::NamedStyle s, mainStyles.styles(KoGenStyle::ParagraphAutoStyle))
    //    mainStyles.markStyleForStylesXml(s.name);

    //kDebug(32001) << "END saveHeaderFooter ############################################";
}

KWOdfWriter::KWOdfWriter(KWDocument *document)
        : QObject(),
        m_document(document),
        m_shapeTree(4, 2)
{
}

KWOdfWriter::~KWOdfWriter()
{
}

// 1.6: KWDocument::saveOasisHelper()
bool KWOdfWriter::save(KoOdfWriteStore &odfStore, KoEmbeddedDocumentSaver &embeddedSaver)
{
    //kDebug(32001) << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";

    KoStore *store = odfStore.store();

    if (! store->open("settings.xml")) {
        return false;
    }

    saveOdfSettings(store);

    if (!store->close())
        return false;

    KoXmlWriter *manifestWriter = odfStore.manifestWriter();

    manifestWriter->addManifestEntry("settings.xml", "text/xml");

    KoXmlWriter *contentWriter = odfStore.contentWriter();
    if (!contentWriter)
        return false;

    KTemporaryFile tmpChangeFile;
    tmpChangeFile.open();
    KoXmlWriter *changeWriter = new KoXmlWriter(&tmpChangeFile, 1);
    if (!changeWriter)
        return false;

    KTemporaryFile tmpTextBodyFile;
    tmpTextBodyFile.open();
    KoXmlWriter *tmpBodyWriter = new KoXmlWriter(&tmpTextBodyFile, 1);
    if (!tmpBodyWriter)
        return false;

    KoGenStyles mainStyles;

    KoGenChanges changes;

    // Save the named styles
    KoStyleManager *styleManager = m_document->resourceManager()->resource(KoText::StyleManager).value<KoStyleManager*>();
    styleManager->saveOdf(mainStyles);

    // TODO get the pagestyle for the first page and store that as 'style:default-page-layout'

    // Header and footers save their content into master-styles/master-page, and their
    // styles into the page-layout automatic-style.

    saveHeaderFooter(embeddedSaver, mainStyles, changes);

    KoXmlWriter *bodyWriter = odfStore.bodyWriter();
    bodyWriter->startElement("office:body");
    bodyWriter->startElement("office:text");

    KoShapeSavingContext context(*tmpBodyWriter, mainStyles, embeddedSaver);

    KoTextSharedSavingData *sharedData = new KoTextSharedSavingData;
    sharedData->setGenChanges(changes);
    context.addSharedData(KOTEXT_SHARED_SAVING_ID, sharedData);

    calculateZindexOffsets();

    KWTextFrameSet *mainTextFrame = 0;

    foreach (KWFrameSet *fs, m_document->frameSets()) {
        // For the purpose of saving to ODF we have 3 types of frames.
        //  1) auto-generated frames.  This includes header/footers and the main text FS.
        //  2) frames that are anchored to text.  They have a parent and their parent will save them.
        //  3) frames that are not anchored but freely positioned somewhere on the page.
        //     in ODF terms those frames are page-anchored.

        if (fs->frameCount() == 1 && fs->frames().first()->shape()->parent())
            continue; // is a frame that is anchored to text, don't save those here.

        KWTextFrameSet *tfs = dynamic_cast<KWTextFrameSet*>(fs);
        if (tfs) {
            if (tfs->textFrameSetType() == KWord::MainTextFrameSet) {
                mainTextFrame = tfs;
                continue;
            }
            else if (KWord::isAutoGenerated(tfs)) {
                continue;
            }
        }

        KWFrame *lastNonCopyFrame = 0;
        int counter = 1;
        QSet<QString> uniqueNames;
        foreach (KWFrame *frame, fs->frames()) { // make sure all shapes have names.
            KoShape *shape = frame->shape();
            if (counter++ == 1)
                shape->setName(fs->name());
            else if (shape->name().isEmpty() || uniqueNames.contains(shape->name()))
                shape->setName(QString("%1-%2").arg(fs->name(), QString::number(counter)));
            uniqueNames << shape->name();
        }
        const QList<KWFrame*> frames = fs->frames();
        for (int i = 0; i < frames.count(); ++i) {
            KWFrame *frame = frames.at(i);
            KoShape *shape = frame->shape();
            // frame properties first
            shape->setAdditionalStyleAttribute("fo:margin", QString::number(frame->runAroundDistance()) + "pt");
            shape->setAdditionalStyleAttribute("style:horizontal-pos", "from-left");
            shape->setAdditionalStyleAttribute("style:horizontal-rel", "page");
            shape->setAdditionalStyleAttribute("style:vertical-pos", "from-top");
            shape->setAdditionalStyleAttribute("style:vertical-rel", "page");
            QString value;
            switch (frame->textRunAround()) {
            case KWord::RunAround:
                switch (frame->runAroundSide()) {
                case KWord::BiggestRunAroundSide: value = "biggest"; break;
                case KWord::LeftRunAroundSide: value = "left"; break;
                case KWord::RightRunAroundSide: value = "right"; break;
                case KWord::AutoRunAroundSide: value = "dynamic"; break;
                case KWord::BothRunAroundSide: value = "parallel"; break;
                }
                break;
            case KWord::RunThrough:
                value = "run-through";
                break;
            case KWord::NoRunAround:
                value = "none";
                break;
            }
            shape->setAdditionalStyleAttribute("style:wrap", value);

            switch (frame->frameBehavior()) {
            case KWord::AutoCreateNewFrameBehavior:
                value = "auto-create-new-frame";
                break;
            case KWord::IgnoreContentFrameBehavior:
                value = "clip";
                break;
            case KWord::AutoExtendFrameBehavior:
                // the third case, AutoExtendFrame is handled by min-height
                value.clear();
                KWTextFrame *tf = dynamic_cast<KWTextFrame*>(frame);
                if (tf && tf->minimumFrameHeight() > 1)
                    shape->setAdditionalAttribute("fo:min-height", QString::number(tf->minimumFrameHeight()) + "pt");
                break;
            }
            if (!value.isEmpty())
                shape->setAdditionalStyleAttribute("style:overflow-behavior", value);

            if (frame->frameBehavior() != KWord::IgnoreContentFrameBehavior) {
                switch (frame->newFrameBehavior()) {
                case KWord::ReconnectNewFrame: value = "followup"; break;
                case KWord::NoFollowupFrame: value.clear(); break; // "none" is the default
                case KWord::CopyNewFrame: value = "copy"; break;
                }
                if (!value.isEmpty()) {
                    shape->setAdditionalStyleAttribute("koffice:frame-behavior-on-new-page", value);
                    if (! frame->frameOnBothSheets())
                        shape->setAdditionalAttribute("koffice:frame-copy-to-facing-pages", "true");
                }
            }

            if (frame->isCopy()) {
                Q_ASSERT(lastNonCopyFrame);
                shape->setAdditionalAttribute("draw:copy-of", lastNonCopyFrame->shape()->name());
            } else {
                lastNonCopyFrame = frame;
            }

            // shape properties
            KWPage page = m_document->pageManager()->page(shape);
            const qreal pagePos = page.offsetInDocument();

            const int effectiveZIndex = shape->zIndex() + m_zIndexOffsets.value(page);
            shape->setAdditionalAttribute("draw:z-index", QString::number(effectiveZIndex));
            shape->setAdditionalAttribute("text:anchor-type", "page");
            shape->setAdditionalAttribute("text:anchor-page-number", QString::number(page.pageNumber()));
            context.addShapeOffset(shape, QMatrix(1, 0, 0 , 1, 0, -pagePos));
            shape->saveOdf(context);
            context.removeShapeOffset(shape);
            shape->removeAdditionalAttribute("draw:copy-of");
            shape->removeAdditionalAttribute("draw:z-index");
            shape->removeAdditionalAttribute("fo:min-height");
            shape->removeAdditionalAttribute("koffice:frame-copy-to-facing-pages");
            shape->removeAdditionalAttribute("text:anchor-page-number");
            shape->removeAdditionalAttribute("text:anchor-page-number");
            shape->removeAdditionalAttribute("text:anchor-type");
        }
    }

    if (mainTextFrame) {
        if (! mainTextFrame->frames().isEmpty() && mainTextFrame->frames().first()) {
            KoTextShapeData *shapeData = qobject_cast<KoTextShapeData *>(mainTextFrame->frames().first()->shape()->userData());
            if (shapeData) {
                KWPageManager *pm = m_document->pageManager();
                if (pm->pageCount()) { // make the first page refer to our page master
                    QTextCursor cursor(shapeData->document());
                    QTextBlockFormat tbf;
                    KWPageStyle style = pm->pages().first().pageStyle();
                    tbf.setProperty(KoParagraphStyle::MasterPageName, m_masterPages.value(style));
                    cursor.mergeBlockFormat(tbf);
                }
                shapeData->saveOdf(context);
            }
        }
    }

    //we save the changes before starting the page sequence element because odf validator insist on having <tracked-changes> right after the <office:text> tag
    mainStyles.saveOdfStyles(KoGenStyles::DocumentAutomaticStyles, contentWriter);

    changes.saveOdfChanges(changeWriter);

    delete changeWriter;
    changeWriter = 0;

    tmpChangeFile.close();
    bodyWriter->addCompleteElement(&tmpChangeFile);

    bodyWriter->startElement("text:page-sequence");
    foreach (KWPage page, m_document->pageManager()->pages()) {
        Q_ASSERT(m_masterPages.contains(page.pageStyle()));
        bodyWriter->startElement("text:page");
        bodyWriter->addAttribute("text:master-page-name", m_masterPages.value(page.pageStyle()));
        bodyWriter->endElement(); // text:page
    }
    bodyWriter->endElement(); // text:page-sequence

    delete tmpBodyWriter;
    tmpBodyWriter = 0;

    tmpTextBodyFile.close();
    bodyWriter->addCompleteElement(&tmpTextBodyFile);

    bodyWriter->endElement(); // office:text
    bodyWriter->endElement(); // office:body

    odfStore.closeContentWriter();

    // add manifest line for content.xml
    manifestWriter->addManifestEntry("content.xml", "text/xml");

    // update references to xml:id to be to new xml:id
    // in the external Rdf
    if (KoDocumentRdfBase *rdf = m_document->documentRdfBase()) {
        QMap<QString, QString> m = sharedData->getRdfIdMapping();
        rdf->updateXmlIdReferences(m);
    }
    // save the styles.xml
    if (!mainStyles.saveOdfStylesDotXml(store, manifestWriter))
        return false;

    if (!context.saveDataCenter(store, manifestWriter)) {
        return false;
    }

    return true;
}

bool KWOdfWriter::saveOdfSettings(KoStore *store)
{
    KoStoreDevice settingsDev(store);
    KoXmlWriter *settingsWriter = KoOdfWriteStore::createOasisXmlWriter(&settingsDev, "office:document-settings");

    // add this so that OOo reads guides lines and grid data from ooo:view-settings
    settingsWriter->addAttribute("xmlns:ooo", "http://openoffice.org/2004/office");

    settingsWriter->startElement("office:settings");
    settingsWriter->startElement("config:config-item-set");
    settingsWriter->addAttribute("config:name", "view-settings");

    m_document->saveUnitOdf(settingsWriter);

    settingsWriter->endElement(); // config:config-item-set

    settingsWriter->startElement("config:config-item-set");
    settingsWriter->addAttribute("config:name", "ooo:view-settings");
    settingsWriter->startElement("config:config-item-map-indexed");
    settingsWriter->addAttribute("config:name", "Views");
    settingsWriter->startElement("config:config-item-map-entry");

    m_document->guidesData().saveOdfSettings(*settingsWriter);
    m_document->gridData().saveOdfSettings(*settingsWriter);

    settingsWriter->endElement(); // config:config-item-map-entry
    settingsWriter->endElement(); // config:config-item-map-indexed
    settingsWriter->endElement(); // config:config-item-set

    settingsWriter->endElement(); // office:settings
    settingsWriter->endElement(); // office:document-settings

    settingsWriter->endDocument();

    delete settingsWriter;

    return true;
}

void KWOdfWriter::calculateZindexOffsets()
{
    Q_ASSERT(m_zIndexOffsets.isEmpty()); // call this method only once, please.
    foreach (KWFrameSet *fs, m_document->frameSets()) {
        if (KWord::isAutoGenerated(dynamic_cast<KWTextFrameSet*>(fs)))
            continue;
        foreach (KWFrame *frame, fs->frames()) {
            addShapeToTree(frame->shape());
        }
    }

    foreach (const KWPage &page, m_document->pageManager()->pages()) {
        // TODO handle pageSpread here.
        int minZIndex = 0;
        foreach (KoShape *shape, m_shapeTree.intersects(page.rect()))
            minZIndex = qMin(shape->zIndex(), minZIndex);
        m_zIndexOffsets.insert(page, -minZIndex);
    }
}

void KWOdfWriter::addShapeToTree(KoShape *shape)
{
    if (! dynamic_cast<KoShapeGroup*>(shape) && ! dynamic_cast<KoShapeLayer*>(shape))
        m_shapeTree.insert(shape->boundingRect(), shape);

    // add the children of a KoShapeContainer
    KoShapeContainer *container = dynamic_cast<KoShapeContainer*>(shape);
    if (container) {
        foreach(KoShape *containerShape, container->childShapes()) {
            addShapeToTree(containerShape);
        }
    }
}
