/* This file is part of the KDE project
 * Copyright (C) 2006, 2009 Thomas Zander <zander@kde.org>
 * Copyright (C) 2007 Sebastian Sauer <mail@dipe.org>
 * Copyright (C) 2007 Pierre Ducroquet <pinaraf@gmail.com>
 * Copyright (C) 2008 Girish Ramakrishnan <girish@forwardbias.in>
 * Copyright (C) 2009 KO GmbH <cbo@kogmbh.com>
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

#include "KoStyleManager.h"
#include "KoParagraphStyle.h"
#include "KoCharacterStyle.h"
#include "KoListStyle.h"
#include "KoListLevelProperties.h"
#include "KoTableStyle.h"
#include "KoTableColumnStyle.h"
#include "KoTableRowStyle.h"
#include "KoTableCellStyle.h"
#include "KoSectionStyle.h"
#include "ChangeFollower.h"
#include "KoTextDocument.h"

#include <KoGenStyle.h>
#include <KoGenStyles.h>
#include <KoShapeSavingContext.h>

#include <QTimer>
#include <QUrl>
#include <QTextLayout>
#include <QTextBlock>
#include <QTextCursor>
#include <QPixmap>
#include <QMap>
#include <QBuffer>
#include <kdebug.h>
#include <klocale.h>

class KoStyleManager::Private
{
public:
    Private() : updateTriggered(false), defaultCharacterStyle(0), defaultParagraphStyle(0), defaultListStyle(0), outlineStyle(0)
    {
    }
    ~Private() {
        qDeleteAll(automaticListStyles);
    }
    static int s_stylesNumber; // For giving out unique numbers to the styles for referencing

    QHash<int, KoCharacterStyle*> charStyles;
    QHash<int, KoParagraphStyle*> paragStyles;
    QHash<int, KoListStyle*> listStyles;
    QHash<int, KoListStyle *> automaticListStyles;
    QHash<int, KoTableStyle *> tableStyles;
    QHash<int, KoTableColumnStyle *> tableColumnStyles;
    QHash<int, KoTableRowStyle *> tableRowStyles;
    QHash<int, KoTableCellStyle *> tableCellStyles;
    QHash<int, KoSectionStyle *> sectionStyles;
    QHash<int, KoParagraphStyle *> unusedParagraphStyles;
    QList<ChangeFollower*> documentUpdaterProxies;

    bool updateTriggered;
    QList<int> updateQueue;

    KoCharacterStyle *defaultCharacterStyle;
    KoParagraphStyle *defaultParagraphStyle;
    KoListStyle *defaultListStyle;
    KoListStyle *outlineStyle;
    QList<int> defaultToCEntriesStyleId;
    KoOdfNotesConfiguration *footNotesConfiguration;
    KoOdfNotesConfiguration *endNotesConfiguration;
    KoOdfBibliographyConfiguration *bibliographyConfiguration;
};

// static
int KoStyleManager::Private::s_stylesNumber = 100;

KoStyleManager::KoStyleManager(QObject *parent)
        : QObject(parent), d(new Private())
{
    d->defaultCharacterStyle = new KoCharacterStyle(this);
    d->defaultCharacterStyle->setName(i18n("Default"));

    add(d->defaultCharacterStyle);

    d->defaultParagraphStyle = new KoParagraphStyle(this);
    d->defaultParagraphStyle->setName(i18n("Default"));

    add(d->defaultParagraphStyle);

    //TODO: also use the defaultstyles.xml mechanism. see KoOdfLoadingContext and KoTextSharedLoadingData
    d->defaultListStyle = new KoListStyle(this);
    KoListLevelProperties llp;
    llp.setLevel(1);
    llp.setStartValue(1);
    llp.setStyle(KoListStyle::DecimalItem);
    llp.setListItemSuffix(".");
    d->defaultListStyle->setLevelProperties(llp);

    //default styles for ToCs
    int maxOutLineLevel = 10;
    for (int outlineLevel = 1; outlineLevel <= maxOutLineLevel; outlineLevel++) {
        KoParagraphStyle *style = new KoParagraphStyle();
        style->setName("Contents " + QString::number(outlineLevel));
        style->setLeftMargin(QTextLength(QTextLength::FixedLength, (outlineLevel - 1) * 8));
        add(style);
        d->defaultToCEntriesStyleId.append(style->styleId());
    }

    d->footNotesConfiguration = 0;
    d->endNotesConfiguration = 0;
    d->bibliographyConfiguration = 0;
}

KoStyleManager::~KoStyleManager()
{
    delete d;
}

void KoStyleManager::saveOdfDefaultStyles(KoShapeSavingContext &context)
{
    KoGenStyle pstyle(KoGenStyle::ParagraphStyle, "paragraph");
    pstyle.setDefaultStyle(true);
    d->defaultParagraphStyle->saveOdf(pstyle, context);
    if (!pstyle.isEmpty()) {
        context.mainStyles().insert(pstyle);
    }

    KoGenStyle tstyle(KoGenStyle::TextStyle, "text");
    tstyle.setDefaultStyle(true);
    d->defaultCharacterStyle->saveOdf(tstyle);
    if (!tstyle.isEmpty()) {
        context.mainStyles().insert(tstyle);
    }

}

void KoStyleManager::saveOdf(KoShapeSavingContext &context)
{
    saveOdfDefaultStyles(context);

    // don't save character styles that are already saved as part of a paragraph style
    QHash<KoParagraphStyle*, QString> savedNames;
    foreach(KoParagraphStyle *paragraphStyle, d->paragStyles) {
        if (paragraphStyle == d->defaultParagraphStyle)
            continue;

        QString name(QString(QUrl::toPercentEncoding(paragraphStyle->name(), "", " ")).replace('%', '_'));
        if (name.isEmpty()) {
            name = 'P';
        }

        KoGenStyle style(KoGenStyle::ParagraphStyle, "paragraph");
        paragraphStyle->saveOdf(style, context);
        QString newName = context.mainStyles().insert(style, name, KoGenStyles::DontAddNumberToName);
        savedNames.insert(paragraphStyle, newName);
    }

    foreach(KoParagraphStyle *p, d->paragStyles) {
        if (p->nextStyle() > 0 && savedNames.contains(p) && paragraphStyle(p->nextStyle())) {
            KoParagraphStyle *next = paragraphStyle(p->nextStyle());
            if (next == p) // this is the default
                continue;
            context.mainStyles().insertStyleRelation(savedNames.value(p), savedNames.value(next), "style:next-style-name");
        }
    }

    foreach(KoCharacterStyle *characterStyle, d->charStyles) {
        if (characterStyle == d->defaultCharacterStyle)
            continue;

        QString name(QString(QUrl::toPercentEncoding(characterStyle->name(), "", " ")).replace('%', '_'));
        if (name.isEmpty()) {
            name = 'T';
        }

        KoGenStyle style(KoGenStyle::ParagraphStyle, "text");
        characterStyle->saveOdf(style);
        context.mainStyles().insert(style, name, KoGenStyles::DontAddNumberToName);
    }

    foreach(KoListStyle *listStyle, d->listStyles) {
        if (listStyle == d->defaultListStyle)
            continue;
        QString name(QString(QUrl::toPercentEncoding(listStyle->name(), "", " ")).replace('%', '_'));
        if (name.isEmpty())
            name = 'L';

        KoGenStyle style(KoGenStyle::ListStyle);
        listStyle->saveOdf(style, context);
        context.mainStyles().insert(style, name, KoGenStyles::DontAddNumberToName);
    }

    foreach(KoTableStyle *tableStyle, d->tableStyles) {
        QString name(QString(QUrl::toPercentEncoding(tableStyle->name(), "", " ")).replace('%', '_'));
        if (name.isEmpty())
            name = 'T'; //TODO is this correct?

        KoGenStyle style(KoGenStyle::TableStyle);
        tableStyle->saveOdf(style);
        context.mainStyles().insert(style, name, KoGenStyles::DontAddNumberToName);
    }

    foreach(KoTableColumnStyle *tableColumnStyle, d->tableColumnStyles) {
        QString name(QString(QUrl::toPercentEncoding(tableColumnStyle->name(), "", " ")).replace('%', '_'));
        if (name.isEmpty())
            name = 'T'; //TODO is this correct?

        KoGenStyle style(KoGenStyle::TableColumnStyle);
        tableColumnStyle->saveOdf(style);
        context.mainStyles().insert(style, name, KoGenStyles::DontAddNumberToName);
    }

    foreach(KoTableRowStyle *tableRowStyle, d->tableRowStyles) {
        QString name(QString(QUrl::toPercentEncoding(tableRowStyle->name(), "", " ")).replace('%', '_'));
        if (name.isEmpty())
            name = 'T'; //TODO is this correct?

        KoGenStyle style(KoGenStyle::TableRowStyle);
        tableRowStyle->saveOdf(style);
        context.mainStyles().insert(style, name, KoGenStyles::DontAddNumberToName);
    }

    foreach(KoTableCellStyle *tableCellStyle, d->tableCellStyles) {
        QString name(QString(QUrl::toPercentEncoding(tableCellStyle->name(), "", " ")).replace('%', '_'));
        if (name.isEmpty())
            name = "T."; //TODO is this correct?

        KoGenStyle style(KoGenStyle::TableCellStyle);
        tableCellStyle->saveOdf(style, context);
        context.mainStyles().insert(style, name, KoGenStyles::DontAddNumberToName);
    }

    foreach(KoSectionStyle *sectionStyle, d->sectionStyles) {
        QString name(QString(QUrl::toPercentEncoding(sectionStyle->name(), "", " ")).replace('%', '_'));
        if (name.isEmpty())
            name = "T."; //TODO is this correct?

        KoGenStyle style(KoGenStyle::SectionStyle);
        sectionStyle->saveOdf(style);
        context.mainStyles().insert(style, name, KoGenStyles::DontAddNumberToName);
    }

    //save note configuration in styles.xml
    if (d->footNotesConfiguration) {
        QBuffer xmlBufferFootNote;
        KoXmlWriter *xmlWriter = new KoXmlWriter(&xmlBufferFootNote);
        d->footNotesConfiguration->saveOdf(xmlWriter);
        context.mainStyles().insertRawOdfStyles(KoGenStyles::DocumentStyles, xmlBufferFootNote.data());
    }

    if (d->endNotesConfiguration) {
        QBuffer xmlBufferEndNote;
        KoXmlWriter *xmlWriter = new KoXmlWriter(&xmlBufferEndNote);
        d->endNotesConfiguration->saveOdf(xmlWriter);
        context.mainStyles().insertRawOdfStyles(KoGenStyles::DocumentStyles, xmlBufferEndNote.data());
    }

    if (d->bibliographyConfiguration) {
        QBuffer xmlBufferBib;
        KoXmlWriter *xmlWriter = new KoXmlWriter(&xmlBufferBib);
        d->bibliographyConfiguration->saveOdf(xmlWriter);
        context.mainStyles().insertRawOdfStyles(KoGenStyles::DocumentStyles, xmlBufferBib.data());
    }

    if (d->outlineStyle) {
        QString name(QString(QUrl::toPercentEncoding(d->outlineStyle->name(), "", " ")).replace('%', '_'));
        if (name.isEmpty())
            name = 'O';

        KoGenStyle style(KoGenStyle::OutlineLevelStyle);
        d->outlineStyle->saveOdf(style, context);
        context.mainStyles().insert(style, name, KoGenStyles::DontAddNumberToName);
    }
}

void KoStyleManager::add(KoCharacterStyle *style)
{
    if (d->charStyles.key(style, -1) != -1)
        return;
    style->setParent(this);
    style->setStyleId(d->s_stylesNumber);
    d->charStyles.insert(d->s_stylesNumber++, style);

    emit styleAdded(style);
}

void KoStyleManager::add(KoParagraphStyle *style)
{
    if (d->paragStyles.key(style, -1) != -1)
        return;
    style->setParent(this);
    style->setStyleId(d->s_stylesNumber);
    d->paragStyles.insert(d->s_stylesNumber++, style);

    if (style->listStyle() && style->listStyle()->styleId() == 0)
        add(style->listStyle());
    KoParagraphStyle *root = style;
    while (root->parentStyle()) {
        root = root->parentStyle();
        if (root->styleId() == 0)
            add(root);
    }

    emit styleAdded(style);
}

void KoStyleManager::add(KoListStyle *style)
{
    if (d->listStyles.key(style, -1) != -1)
        return;
    style->setParent(this);
    style->setStyleId(d->s_stylesNumber);
    d->listStyles.insert(d->s_stylesNumber++, style);
    emit styleAdded(style);
}

void KoStyleManager::addAutomaticListStyle(KoListStyle *style)
{
    if (d->automaticListStyles.key(style, -1) != -1)
        return;
    style->setStyleId(d->s_stylesNumber);
    d->automaticListStyles.insert(d->s_stylesNumber++, style);
}

void KoStyleManager::add(KoTableStyle *style)
{
    if (d->tableStyles.key(style, -1) != -1)
        return;
    style->setParent(this);
    style->setStyleId(d->s_stylesNumber);
    d->tableStyles.insert(d->s_stylesNumber++, style);
    emit styleAdded(style);
}

void KoStyleManager::add(KoTableColumnStyle *style)
{
    if (d->tableColumnStyles.key(style, -1) != -1)
        return;
    style->setStyleId(d->s_stylesNumber);
    d->tableColumnStyles.insert(d->s_stylesNumber++, style);
    emit styleAdded(style);
}

void KoStyleManager::add(KoTableRowStyle *style)
{
    if (d->tableRowStyles.key(style, -1) != -1)
        return;
    style->setStyleId(d->s_stylesNumber);
    d->tableRowStyles.insert(d->s_stylesNumber++, style);
    emit styleAdded(style);
}

void KoStyleManager::add(KoTableCellStyle *style)
{
    if (d->tableCellStyles.key(style, -1) != -1)
        return;
    style->setParent(this);
    style->setStyleId(d->s_stylesNumber);
    d->tableCellStyles.insert(d->s_stylesNumber++, style);
    emit styleAdded(style);
}

void KoStyleManager::add(KoSectionStyle *style)
{
    if (d->sectionStyles.key(style, -1) != -1)
        return;
    style->setParent(this);
    style->setStyleId(d->s_stylesNumber);
    d->sectionStyles.insert(d->s_stylesNumber++, style);
    emit styleAdded(style);
}

void KoStyleManager::setNotesConfiguration(KoOdfNotesConfiguration *notesConfiguration)
{
    if (notesConfiguration->noteClass() == KoOdfNotesConfiguration::Footnote) {
        d->footNotesConfiguration = notesConfiguration;
    } else if (notesConfiguration->noteClass() == KoOdfNotesConfiguration::Endnote) {
        d->endNotesConfiguration = notesConfiguration;
    }
}

void KoStyleManager::setBibliographyConfiguration(KoOdfBibliographyConfiguration *bibliographyConfiguration)
{
    d->bibliographyConfiguration = bibliographyConfiguration;
}

void KoStyleManager::remove(KoCharacterStyle *style)
{
    if (d->charStyles.remove(style->styleId()))
        emit styleRemoved(style);
}

void KoStyleManager::remove(KoParagraphStyle *style)
{
    if (d->paragStyles.remove(style->styleId()))
        emit styleRemoved(style);
}

void KoStyleManager::remove(KoListStyle *style)
{
    if (d->listStyles.remove(style->styleId()))
        emit styleRemoved(style);
}

void KoStyleManager::remove(KoTableStyle *style)
{
    if (d->tableStyles.remove(style->styleId()))
        emit styleRemoved(style);
}

void KoStyleManager::remove(KoTableColumnStyle *style)
{
    if (d->tableColumnStyles.remove(style->styleId()))
        emit styleRemoved(style);
}

void KoStyleManager::remove(KoTableRowStyle *style)
{
    if (d->tableRowStyles.remove(style->styleId()))
        emit styleRemoved(style);
}

void KoStyleManager::remove(KoTableCellStyle *style)
{
    if (d->tableCellStyles.remove(style->styleId()))
        emit styleRemoved(style);
}

void KoStyleManager::remove(KoSectionStyle *style)
{
    if (d->sectionStyles.remove(style->styleId()))
        emit styleRemoved(style);
}

void KoStyleManager::remove(ChangeFollower *cf)
{
    d->documentUpdaterProxies.removeAll(cf);
}

void KoStyleManager::alteredStyle(const KoParagraphStyle *style)
{
    Q_ASSERT(style);
    int id = style->styleId();
    if (id <= 0) {
        kWarning(32500) << "alteredStyle received from a non registered style!";
        return;
    }
    if (! d->updateQueue.contains(id))
        d->updateQueue.append(id);
    requestFireUpdate();

    // check if anyone that uses 'style' as a parent needs to be flagged as changed as well.
    foreach(KoParagraphStyle *ps, d->paragStyles) {
        if (ps->parentStyle() == style)
            alteredStyle(ps);
    }
}

void KoStyleManager::alteredStyle(const KoCharacterStyle *style)
{
    Q_ASSERT(style);
    int id = style->styleId();
    if (id <= 0) {
        kWarning(32500) << "alteredStyle received from a non registered style!";
        return;
    }
    if (! d->updateQueue.contains(id))
        d->updateQueue.append(id);
    requestFireUpdate();
}

void KoStyleManager::alteredStyle(const KoListStyle *style)
{
    Q_ASSERT(style);
    int id = style->styleId();
    if (id <= 0) {
        kWarning(32500) << "alteredStyle received from a non registered style!";
        return;
    }
    if (!d->updateQueue.contains(id))
        d->updateQueue.append(id);
    requestFireUpdate();
}

void KoStyleManager::alteredStyle(const KoTableStyle *style)
{
    Q_ASSERT(style);
    int id = style->styleId();
    if (id <= 0) {
        kWarning(32500) << "alteredStyle received from a non registered style!";
        return;
    }
    if (!d->updateQueue.contains(id))
        d->updateQueue.append(id);
    requestFireUpdate();
}

void KoStyleManager::alteredStyle(const KoTableColumnStyle *style)
{
    Q_ASSERT(style);
    int id = style->styleId();
    if (id <= 0) {
        kWarning(32500) << "alteredStyle received from a non registered style!";
        return;
    }
    if (!d->updateQueue.contains(id))
        d->updateQueue.append(id);
    requestFireUpdate();
}

void KoStyleManager::alteredStyle(const KoTableRowStyle *style)
{
    Q_ASSERT(style);
    int id = style->styleId();
    if (id <= 0) {
        kWarning(32500) << "alteredStyle received from a non registered style!";
        return;
    }
    if (!d->updateQueue.contains(id))
        d->updateQueue.append(id);
    requestFireUpdate();
}

void KoStyleManager::alteredStyle(const KoTableCellStyle *style)
{
    Q_ASSERT(style);
    int id = style->styleId();
    if (id <= 0) {
        kWarning(32500) << "alteredStyle received from a non registered style!";
        return;
    }
    if (!d->updateQueue.contains(id))
        d->updateQueue.append(id);
    requestFireUpdate();
}

void KoStyleManager::alteredStyle(const KoSectionStyle *style)
{
    Q_ASSERT(style);
    int id = style->styleId();
    if (id <= 0) {
        kWarning(32500) << "alteredStyle received from a non registered style!";
        return;
    }
    if (!d->updateQueue.contains(id))
        d->updateQueue.append(id);
    requestFireUpdate();
}

void KoStyleManager::updateAlteredStyles()
{
    foreach(ChangeFollower *cf, d->documentUpdaterProxies) {
        cf->processUpdates(d->updateQueue);
    }
    d->updateQueue.clear();
    d->updateTriggered = false;
}

void KoStyleManager::requestFireUpdate()
{
    if (d->updateTriggered)
        return;
    QTimer::singleShot(0, this, SLOT(updateAlteredStyles()));
    d->updateTriggered = true;
}

void KoStyleManager::add(QTextDocument *document)
{
    foreach(ChangeFollower *cf, d->documentUpdaterProxies) {
        if (cf->document() == document) {
            return; // already present.
        }
    }
    ChangeFollower *cf = new ChangeFollower(document, this);
    d->documentUpdaterProxies.append(cf);
}

void KoStyleManager::remove(QTextDocument *document)
{
    foreach(ChangeFollower *cf, d->documentUpdaterProxies) {
        if (cf->document() == document) {
            d->documentUpdaterProxies.removeAll(cf);
            return;
        }
    }
}

KoCharacterStyle *KoStyleManager::characterStyle(int id) const
{
    return d->charStyles.value(id);
}

KoParagraphStyle *KoStyleManager::paragraphStyle(int id) const
{
    return d->paragStyles.value(id);
}

KoListStyle *KoStyleManager::listStyle(int id) const
{
    return d->listStyles.value(id);
}

KoListStyle *KoStyleManager::listStyle(int id, bool *automatic) const
{
    if (KoListStyle *style = listStyle(id)) {
        *automatic = false;
        return style;
    }

    KoListStyle *style = d->automaticListStyles.value(id);

    if (style) {
        *automatic = true;
    }
    else {
        // *automatic is unchanged
    }
    return style;
}

KoTableStyle *KoStyleManager::tableStyle(int id) const
{
    return d->tableStyles.value(id);
}

KoTableColumnStyle *KoStyleManager::tableColumnStyle(int id) const
{
    return d->tableColumnStyles.value(id);
}

KoTableRowStyle *KoStyleManager::tableRowStyle(int id) const
{
    return d->tableRowStyles.value(id);
}

KoTableCellStyle *KoStyleManager::tableCellStyle(int id) const
{
    return d->tableCellStyles.value(id);
}

KoSectionStyle *KoStyleManager::sectionStyle(int id) const
{
    return d->sectionStyles.value(id);
}

KoCharacterStyle *KoStyleManager::characterStyle(const QString &name) const
{
    foreach(KoCharacterStyle *style, d->charStyles) {
        if (style->name() == name)
            return style;
    }
    return 0;
}

KoParagraphStyle *KoStyleManager::paragraphStyle(const QString &name) const
{
    foreach(KoParagraphStyle *style, d->paragStyles) {
        if (style->name() == name)
            return style;
    }
    return 0;
}

KoListStyle *KoStyleManager::listStyle(const QString &name) const
{
    foreach(KoListStyle *style, d->listStyles) {
        if (style->name() == name)
            return style;
    }
    return 0;
}

KoTableStyle *KoStyleManager::tableStyle(const QString &name) const
{
    foreach(KoTableStyle *style, d->tableStyles) {
        if (style->name() == name)
            return style;
    }
    return 0;
}

KoTableColumnStyle *KoStyleManager::tableColumnStyle(const QString &name) const
{
    foreach(KoTableColumnStyle *style, d->tableColumnStyles) {
        if (style->name() == name)
            return style;
    }
    return 0;
}

KoTableRowStyle *KoStyleManager::tableRowStyle(const QString &name) const
{
    foreach(KoTableRowStyle *style, d->tableRowStyles) {
        if (style->name() == name)
            return style;
    }
    return 0;
}

KoTableCellStyle *KoStyleManager::tableCellStyle(const QString &name) const
{
    foreach(KoTableCellStyle *style, d->tableCellStyles) {
        if (style->name() == name)
            return style;
    }
    return 0;
}

KoSectionStyle *KoStyleManager::sectionStyle(const QString &name) const
{
    foreach(KoSectionStyle *style, d->sectionStyles) {
        if (style->name() == name)
            return style;
    }
    return 0;
}

KoOdfNotesConfiguration *KoStyleManager::notesConfiguration(KoOdfNotesConfiguration::NoteClass noteClass) const
{
    if (noteClass == KoOdfNotesConfiguration::Endnote) {
        if (!d->endNotesConfiguration) {
            d->endNotesConfiguration = new KoOdfNotesConfiguration();
            d->endNotesConfiguration->setNoteClass(noteClass);
            KoOdfNumberDefinition *numFormat = new KoOdfNumberDefinition();
            numFormat->setFormatSpecification(KoOdfNumberDefinition::RomanLowerCase);
            d->endNotesConfiguration->setNumberFormat(*numFormat);
        }
        return d->endNotesConfiguration;
    } else if (noteClass == KoOdfNotesConfiguration::Footnote) {
        if (!d->footNotesConfiguration) {
            d->footNotesConfiguration = new KoOdfNotesConfiguration();
            d->footNotesConfiguration->setNoteClass(noteClass);
            KoOdfNumberDefinition *numFormat = new KoOdfNumberDefinition();
            numFormat->setFormatSpecification(KoOdfNumberDefinition::Numeric);
            d->footNotesConfiguration->setNumberFormat(*numFormat);
        }
        return d->footNotesConfiguration;
    } else {
        return 0;
    }
}

KoOdfBibliographyConfiguration *KoStyleManager::bibliographyConfiguration() const
{
    return d->bibliographyConfiguration;
}

KoCharacterStyle *KoStyleManager::defaultCharacterStyle() const
{
    return d->defaultCharacterStyle;
}

KoParagraphStyle *KoStyleManager::defaultParagraphStyle() const
{
    return d->defaultParagraphStyle;
}

KoListStyle *KoStyleManager::defaultListStyle() const
{
    return d->defaultListStyle;
}

void KoStyleManager::setOutlineStyle(KoListStyle* listStyle)
{
    if (d->outlineStyle && d->outlineStyle->parent() == this)
        delete d->outlineStyle;
    listStyle->setParent(this);
    d->outlineStyle = listStyle;
}

KoListStyle *KoStyleManager::outlineStyle() const
{
    return d->outlineStyle;
}

QList<KoCharacterStyle*> KoStyleManager::characterStyles() const
{
    return d->charStyles.values();
}

QList<KoParagraphStyle*> KoStyleManager::paragraphStyles() const
{
    return d->paragStyles.values();
}

QList<KoListStyle*> KoStyleManager::listStyles() const
{
    return d->listStyles.values();
}

QList<KoTableStyle*> KoStyleManager::tableStyles() const
{
    return d->tableStyles.values();
}

QList<KoTableColumnStyle*> KoStyleManager::tableColumnStyles() const
{
    return d->tableColumnStyles.values();
}

QList<KoTableRowStyle*> KoStyleManager::tableRowStyles() const
{
    return d->tableRowStyles.values();
}

QList<KoTableCellStyle*> KoStyleManager::tableCellStyles() const
{
    return d->tableCellStyles.values();
}

QList<KoSectionStyle*> KoStyleManager::sectionStyles() const
{
    return d->sectionStyles.values();
}

KoParagraphStyle *KoStyleManager::defaultTableOfContentsEntryStyle(int outlineLevel)
{
    KoParagraphStyle *style = paragraphStyle(d->defaultToCEntriesStyleId.at(outlineLevel - 1));
    return style;
}

KoParagraphStyle *KoStyleManager::defaultTableOfcontentsTitleStyle()
{
    return defaultParagraphStyle();
}

void KoStyleManager::addUnusedStyle(KoParagraphStyle *style)
{
    if (d->unusedParagraphStyles.key(style, -1) != -1)
        return;
    style->setParent(this);
    style->setStyleId(d->s_stylesNumber);
    d->unusedParagraphStyles.insert(d->s_stylesNumber++, style);

    KoParagraphStyle *root = style;
    while (root->parentStyle()) {
        root = root->parentStyle();
        if (root->styleId() == 0)
            addUnusedStyle(root);
    }
    if (root != d->defaultParagraphStyle && root->parentStyle() == 0)
        root->setParentStyle(d->defaultParagraphStyle);
}

void KoStyleManager::moveToUsedStyles(int id)
{
    if (d->paragStyles.contains(id))
        return;

    KoParagraphStyle *style = d->unusedParagraphStyles.value(id);
    d->unusedParagraphStyles.remove(id);

    d->paragStyles.insert(style->styleId(), style);

    if (style->listStyle() && style->listStyle()->styleId() == 0)
        add(style->listStyle());
    KoParagraphStyle *root = style;
    while (root->parentStyle()) {
        root = root->parentStyle();
        if (d->paragStyles.contains(id) == false)
            moveToUsedStyles(root->styleId());
    }

    if (root != d->defaultParagraphStyle && root->parentStyle() == 0)
        root->setParentStyle(d->defaultParagraphStyle);

    emit styleAdded(style);
}

KoParagraphStyle *KoStyleManager::unusedStyle(int id)
{
    return d->unusedParagraphStyles.value(id);
}

#include <KoStyleManager.moc>
