/* This file is part of the KDE project
 * Copyright (C) 2006-2010 Thomas Zander <zander@kde.org>
 * Copyright (C) 2008 Thorsten Zachmann <zachmann@kde.org>
 * Copyright (C) 2008 Girish Ramakrishnan <girish@forwardbias.in>
 * Copyright (C) 2008 Roopesh Chander <roop@forwardbias.in>
 * Copyright (C) 2007-2008 Pierre Ducroquet <pinaraf@pinaraf.info>
 * Copyright (C) 2009-2011 KO GmbH <cbo@kogmbh.com>
 * Copyright (C) 2009-2011 Casper Boemann <cbo@boemann.dk>
 * Copyright (C) 2010 Nandita Suri <suri.nandita@gmail.com>
 * Copyright (C) 2010 Ajay Pundhir <ajay.pratap@iiitb.net>
 * Copyright (C) 2011 Lukáš Tvrdý <lukas.tvrdy@ixonos.com>
 * Copyright (C) 2011 Gopalakrishna Bhat A <gopalakbhat@gmail.com>
 * Copyright (C) 2011 Stuart Dickson <stuart@furkinfantasic.net>
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

#include "KoTextLayoutArea.h"

#include "KoTextLayoutEndNotesArea.h"
#include "KoTextLayoutTableArea.h"
#include "TableIterator.h"
#include "ListItemsHelper.h"
#include "RunAroundHelper.h"
#include "KoTextDocumentLayout.h"
#include "FrameIterator.h"

#include <KoParagraphStyle.h>
#include <KoCharacterStyle.h>
#include <KoListStyle.h>
#include <KoStyleManager.h>
#include <KoTextBlockData.h>
#include <KoTextBlockBorderData.h>
#include <KoTextBlockPaintStrategyBase.h>
#include <KoText.h>
#include <KoChangeTracker.h>
#include <KoChangeTrackerElement.h>
#include <KoImageData.h>
#include <KoImageCollection.h>
#include <KoInlineNote.h>
#include <KoInlineNote.h>
#include <KoInlineTextObjectManager.h>
#include <KoTableOfContentsGeneratorInfo.h>

#include <KDebug>

#include <QTextTable>
#include <QTextList>
#include <QStyle>
#include <QFontMetrics>
#include <QTextFragment>
#include <QTextLayout>
#include <QTextCursor>
#include <QTime>

extern int qt_defaultDpiY();

#define DropCapsAdditionalFormattingId 25602902

void KoTextLayoutArea::paint(QPainter *painter, const KoTextDocumentLayout::PaintContext &context)
{
    if (m_startOfArea == 0) // We have not been layouted yet
        return;

    /*
    struct Timer {
        QTime m_time;
        Timer() { m_time.start(); }
        ~Timer() { kDebug() << "elapsed=" << m_time.elapsed(); }
    };
    Timer timer;
    */

    painter->save();
    painter->translate(0, m_verticalAlignOffset);

    painter->setPen(context.textContext.palette.color(QPalette::Text)); // for text that has no color.
    const QRegion clipRegion = painter->clipRegion(); // fetch after painter->translate so the clipRegion is correct
    KoTextBlockBorderData *lastBorder = 0;
    QRectF lastBorderRect;

    QTextFrame::iterator it = m_startOfArea->it;
    QTextFrame::iterator stop = m_endOfArea->it;
    if (!stop.currentBlock().isValid() || m_endOfArea->lineTextStart >= 0) {
        ++stop;
    }

    int tableAreaIndex = 0;
    int blockIndex = 0;
    int tocIndex = 0;
    bool isAtEnd = false;
    for (; it != stop && !isAtEnd; ++it) {
        isAtEnd = it.atEnd();
        QTextBlock block = it.currentBlock();
        QTextTable *table = qobject_cast<QTextTable*>(it.currentFrame());
        QTextFrame *subFrame = it.currentFrame();
        QTextBlockFormat format = block.blockFormat();
        qDebug() << it.currentBlock().isValid() << table;

        if (!block.isValid()) {
            if (lastBorder) { // draw previous block's border
                lastBorder->paint(*painter, lastBorderRect);
                lastBorder = 0;
            }
        }

        if (table) {
            if (tableAreaIndex >= m_tableAreas.size()) {
                continue;
            }
            m_tableAreas[tableAreaIndex]->paint(painter, context);
            ++tableAreaIndex;
            continue;
        } else if (subFrame) {
            if (subFrame->format().intProperty(KoText::SubFrameType) == KoText::EndNotesFrameType) {
                m_endNotesArea->paint(painter, context);
            }
            continue;
        } else {
            if (!block.isValid()) {
                continue;
            }
        }

        if (block.blockFormat().hasProperty(KoParagraphStyle::TableOfContentsDocument)) {
            // Possibly paint the selection of the entire Table of Contents
            // but since it's a secondary document we need to create a fake selection
            QVariant data = block.blockFormat().property(KoParagraphStyle::TableOfContentsDocument);
            QTextDocument *tocDocument = data.value<QTextDocument *>();

            KoTextDocumentLayout::PaintContext tocContext = context;
            tocContext.textContext.selections = QVector<QAbstractTextDocumentLayout::Selection>();

            bool pure = true;
            foreach(const QAbstractTextDocumentLayout::Selection & selection,   context.textContext.selections) {
                if (selection.cursor.selectionStart()  <= block.position()
                    && selection.cursor.selectionEnd() >= block.position()) {
                    painter->fillRect(m_tableOfContentsAreas[tocIndex]->boundingRect(), selection.format.background());
                    if (pure) {
                        tocContext.textContext.selections.append(QAbstractTextDocumentLayout::Selection());
                        tocContext.textContext.selections[0].cursor = QTextCursor(tocDocument);
                        tocContext.textContext.selections[0].cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
                        tocContext.textContext.selections[0].format = selection.format;
                        pure = false;
                    }
                }
            }
            m_tableOfContentsAreas[tocIndex]->paint(painter, tocContext);
            ++tocIndex;
            continue;
        }

        QTextLayout *layout = block.layout();
        KoTextBlockBorderData *border = 0;

        if (blockIndex >= m_blockRects.count())
            break;
        QRectF br = m_blockRects[blockIndex];
        ++blockIndex;

        if (!painter->hasClipping() || clipRegion.intersects(layout->boundingRect().toRect())) {
            KoTextBlockData *blockData = dynamic_cast<KoTextBlockData*>(block.userData());
            KoTextBlockPaintStrategyBase *paintStrategy = 0;
            if (blockData) {
                border = blockData->border();
                paintStrategy = blockData->paintStrategy();
            }

            KoTextBlockPaintStrategyBase dummyPaintStrategy;
            if (paintStrategy == 0) {
                paintStrategy = &dummyPaintStrategy;
            }
            if (!paintStrategy->isVisible()) {
                if (lastBorder) { // draw previous block's border
                    lastBorder->paint(*painter, lastBorderRect);
                    lastBorder = 0;
                }
                continue; // this paragraph shouldn't be shown so just skip it
            }

            // Check and update border drawing code
            if (lastBorder == 0) {
                lastBorderRect = br;
            } else if (lastBorder && lastBorder != border) {
                lastBorder->paint(*painter, lastBorderRect);
                lastBorderRect = br;
            } else if (lastBorder == border) {
                lastBorderRect = lastBorderRect.united(br);
            }
            lastBorder = border;

            painter->save();

            QBrush bg = paintStrategy->background(block.blockFormat().background());
            if (bg != Qt::NoBrush) {
                painter->fillRect(br, bg);
            }

            paintStrategy->applyStrategy(painter);
            painter->save();
            drawListItem(painter, block, context.imageCollection);
            painter->restore();

            QVector<QTextLayout::FormatRange> selections;
            foreach(const QAbstractTextDocumentLayout::Selection & selection, context.textContext.selections) {
                QTextCursor cursor = selection.cursor;
                int begin = cursor.position();
                int end = cursor.anchor();
                if (begin > end)
                    qSwap(begin, end);

                if (end < block.position() || begin > block.position() + block.length())
                    continue; // selection does not intersect this block.
                if (selection.cursor.hasComplexSelection()) {
                    continue; // selections of several table cells are covered by the within drawBorders above.
                }
                if (!m_documentLayout->changeTracker()
                    || m_documentLayout->changeTracker()->displayChanges()
                    || !m_documentLayout->changeTracker()->containsInlineChanges(selection.format)
                    || !m_documentLayout->changeTracker()->elementById(selection.format.property(KoCharacterStyle::ChangeTrackerId).toInt())->isEnabled()
                    || (m_documentLayout->changeTracker()->elementById(selection.format.property(KoCharacterStyle::ChangeTrackerId).toInt())->getChangeType() != KoGenChange::DeleteChange)) {
                    QTextLayout::FormatRange fr;
                    fr.start = begin - block.position();
                    fr.length = end - begin;
                    fr.format = selection.format;
                    selections.append(fr);
                }
            }

            for (QTextBlock::iterator it = block.begin(); !(it.atEnd()); ++it) {
                QTextFragment currentFragment = it.fragment();
                if (currentFragment.isValid()) {
                    QTextCharFormat format = currentFragment.charFormat();
                    int changeId = format.intProperty(KoCharacterStyle::ChangeTrackerId);
                    if (changeId && m_documentLayout->changeTracker() && m_documentLayout->changeTracker()->displayChanges()) {
                        KoChangeTrackerElement *changeElement = m_documentLayout->changeTracker()->elementById(changeId);
                        switch(changeElement->getChangeType()) {
                            case (KoGenChange::InsertChange):
                            format.setBackground(QBrush(m_documentLayout->changeTracker()->getInsertionBgColor()));
                            break;
                            case (KoGenChange::FormatChange):
                            format.setBackground(QBrush(m_documentLayout->changeTracker()->getFormatChangeBgColor()));
                            break;
                            case (KoGenChange::DeleteChange):
                            format.setBackground(QBrush(m_documentLayout->changeTracker()->getDeletionBgColor()));
                            break;
                            case (KoGenChange::UNKNOWN):
                            break;
                        }

                        QTextLayout::FormatRange fr;
                        fr.start = currentFragment.position() - block.position();
                        fr.length = currentFragment.length();
                        fr.format = format;
                        selections.prepend(fr);
                    }
                }
            }

            //We set clip because layout-draw doesn't clip text to it correctly after all
            //and adjust to make sure we don't clip edges of glyphs. The clipping is
            //imprtatnt for paragraph splt acrosse two pages.
            painter->setClipRect(br.adjusted(-2,-2,2,2), Qt::IntersectClip);

            layout->draw(painter, QPointF(0, 0), selections, br);

            decorateParagraph(painter, block);

            painter->restore();
        } else {
            if (lastBorder) {
                lastBorder->paint(*painter, lastBorderRect);
                lastBorder = 0;
            }
        }
    }
    if (lastBorder) {
        lastBorder->paint(*painter, lastBorderRect);
    }

    painter->translate(0, -m_verticalAlignOffset);
    painter->translate(0, bottom() - top() - m_footNotesHeight);
    foreach(KoTextLayoutArea *footerArea, m_footNoteAreas) {
        footerArea->paint(painter, context);
        painter->translate(0, footerArea->bottom());
    }
    painter->restore();
}

void KoTextLayoutArea::drawListItem(QPainter *painter, const QTextBlock &block, KoImageCollection *imageCollection)
{
    KoTextBlockData *data = dynamic_cast<KoTextBlockData*>(block.userData());
    if (data == 0)
        return;

    QTextList *list = block.textList();

    if (list && data->hasCounterData()) {
        QTextListFormat listFormat = list->format();

        if (! data->counterText().isEmpty()) {
            QFont font(data->labelFormat().font(), m_documentLayout->paintDevice());

            KoListStyle::Style listStyle = static_cast<KoListStyle::Style>(listFormat.style());
            QString result = data->counterText();

            QTextLayout layout(result, font, m_documentLayout->paintDevice());

            QList<QTextLayout::FormatRange> layouts;
            QTextLayout::FormatRange format;
            format.start = 0;
            format.length = data->counterText().length();
            format.format = data->labelFormat();

            layouts.append(format);
            layout.setAdditionalFormats(layouts);

            Qt::Alignment align = static_cast<Qt::Alignment>(listFormat.intProperty(KoListStyle::Alignment));

            if (align == 0) {
                align = Qt::AlignLeft;
            }
            else if (align != Qt::AlignLeft) {
                align |= Qt::AlignAbsolute;
            }

            QTextOption option(align);
            option.setTextDirection(block.layout()->textOption().textDirection());

            if (option.textDirection() == Qt::RightToLeft || data->counterText().isRightToLeft()) {
                option.setAlignment(Qt::AlignRight);
            }

            layout.setTextOption(option);
            layout.beginLayout();

            QTextLine line = layout.createLine();
            line.setLineWidth(data->counterWidth());
            layout.endLayout();

            QPointF counterPosition = data->counterPosition();
            if (block.layout()->lineCount() > 0) {
                // if there is text, then baseline align the counter.
                QTextLine firstParagLine = block.layout()->lineAt(0);
                if (KoListStyle::isNumberingStyle(listStyle) || listStyle == KoListStyle::CustomCharItem) {
                    //if numbered list baseline align
                    counterPosition += QPointF(0, firstParagLine.ascent() - layout.lineAt(0).ascent());
                } else {
                     //for unnumbered list center align
                    counterPosition += QPointF(0, (firstParagLine.height() - layout.lineAt(0).height())/2.0);
                }
            }
            layout.draw(painter, counterPosition);
        }

        KoListStyle::Style listStyle = static_cast<KoListStyle::Style>(listFormat.style());
        if (listStyle == KoListStyle::ImageItem && imageCollection) {
            QFontMetricsF fm(data->labelFormat().font(), m_documentLayout->paintDevice());
            qreal x = qMax(qreal(1), data->counterPosition().x());
            qreal width = qMax(listFormat.doubleProperty(KoListStyle::Width), (qreal)1.0);
            qreal height = qMax(listFormat.doubleProperty(KoListStyle::Height), (qreal)1.0);
            qreal y = data->counterPosition().y() + fm.ascent() - fm.xHeight()/2 - height/2; // centered
            qint64 key = listFormat.property(KoListStyle::BulletImageKey).value<qint64>();
            KoImageData idata;
            imageCollection->fillFromKey(idata, key);
            painter->drawPixmap(x, y, width, height, idata.pixmap());
        }
    }
}

/**
 * Draw a line. Typically meant to underline text or similar.
 * @param painter the painter to paint on.
 * @painter color the pen color to for the decoratoin line
 * @param type The type
 * @param style the type of line to draw.
 * @param width The thickness of the line, in pixels (the painter will be prescaled to points coordinate system).
 * @param x1 we are always drawing horizontal lines, this is the start point.
 * @param x2 we are always drawing horizontal lines, this is the end point.
 * @param y the y-offset to paint on.
 */
static void drawDecorationLine(QPainter *painter, const QColor &color, KoCharacterStyle::LineType type, KoCharacterStyle::LineStyle style, qreal width, const qreal x1, const qreal x2, const qreal y)
{
    QPen penBackup = painter->pen();
    QPen pen = painter->pen();
    pen.setColor(color);
    pen.setWidthF(width);
    if (style == KoCharacterStyle::WaveLine) {
        // Ok, try the waves :)
        pen.setStyle(Qt::SolidLine);
        painter->setPen(pen);
        qreal x = x1;
        const qreal halfWaveWidth = 0.5 * width;
        const qreal halfWaveLength = 2 * width;
        const int startAngle = 0 * 16;
        const int middleAngle = 180 * 16;
        const int endAngle = 180 * 16;
        while (x < x2) {
            QRectF rectangle1(x, y, halfWaveLength, 2*halfWaveWidth);
            if (type == KoCharacterStyle::DoubleLine) {
                painter->translate(0, -pen.width());
                painter->drawArc(rectangle1, startAngle, middleAngle);
                painter->translate(0, 2*pen.width());
                painter->drawArc(rectangle1, startAngle, middleAngle);
                painter->translate(0, -pen.width());
            } else {
                painter->drawArc(rectangle1, startAngle, middleAngle);
            }
            if (x + halfWaveLength > x2)
                break;
            QRectF rectangle2(x + halfWaveLength, y, halfWaveLength, 2*halfWaveWidth);
            if (type == KoCharacterStyle::DoubleLine) {
                painter->translate(0, -pen.width());
                painter->drawArc(rectangle2, middleAngle, endAngle);
                painter->translate(0, 2*pen.width());
                painter->drawArc(rectangle2, middleAngle, endAngle);
                painter->translate(0, -pen.width());
            } else {
                painter->drawArc(rectangle2, middleAngle, endAngle);
            }
            x = x + 2 * halfWaveLength;
        }
    } else {
        if (style == KoCharacterStyle::LongDashLine) {
            QVector<qreal> dashes;
            dashes << 12 << 2;
            pen.setDashPattern(dashes);
        } else {
            pen.setStyle((Qt::PenStyle)style);
        }
        painter->setPen(pen);
        if (type == KoCharacterStyle::DoubleLine) {
            painter->translate(0, -pen.width());
            painter->drawLine(QPointF(x1, y), QPointF(x2, y));
            painter->translate(0, 2*pen.width());
            painter->drawLine(QPointF(x1, y), QPointF(x2, y));
            painter->translate(0, -pen.width());
        } else {
            painter->drawLine(QPointF(x1, y), QPointF(x2, y));
        }
    }
    painter->setPen(penBackup);
}

static void drawDecorationText(QPainter *painter, const QTextLine &line, const QColor &color, const QString& decorText, qreal x1, qreal x2)
{
    qreal y = line.position().y();
    QPen oldPen = painter->pen();
    painter->setPen(QPen(color));
    do {
        QRectF br;
        painter->drawText(QRectF(QPointF(x1, y), QPointF(x2, y + line.height())), Qt::AlignLeft | Qt::AlignVCenter, decorText, &br);
        x1 = br.right();
    } while (x1 <= x2);
    painter->setPen(oldPen);
}

static void drawDecorationWords(QPainter *painter, const QTextLine &line, const QString &text, const QColor &color, KoCharacterStyle::LineType type, KoCharacterStyle::LineStyle style, const QString& decorText, qreal width, const qreal y, const int fragmentToLineOffset, const int startOfFragmentInBlock)
{
    qreal wordBeginX = -1;
    int j = line.textStart()+fragmentToLineOffset;
    while (j < line.textLength() + line.textStart() && j-startOfFragmentInBlock<text.size()) {
        if (text[j-startOfFragmentInBlock].isSpace()) {
            if (wordBeginX != -1) {
                if (decorText.isEmpty())
                    drawDecorationLine(painter, color, type, style, width, wordBeginX, line.cursorToX(j), y);
                else
                    drawDecorationText(painter, line, color, decorText, wordBeginX, line.cursorToX(j));
            }
            wordBeginX = -1;
        } else if (wordBeginX == -1) {
            wordBeginX = line.cursorToX(j);
        }
    ++j;
    }
    if (wordBeginX != -1) {
        if (decorText.isEmpty())
            drawDecorationLine(painter, color, type, style, width, wordBeginX, line.cursorToX(j), y);
        else
            drawDecorationText(painter, line, color, decorText, wordBeginX, line.cursorToX(j));
    }
}

static qreal computeWidth(KoCharacterStyle::LineWeight weight, qreal width, const QFont& font)
{
    switch (weight) {
    case KoCharacterStyle::AutoLineWeight:
    case KoCharacterStyle::NormalLineWeight:
    case KoCharacterStyle::MediumLineWeight:
    case KoCharacterStyle::DashLineWeight:
        return QFontMetricsF(font).lineWidth();
    case KoCharacterStyle::BoldLineWeight:
    case KoCharacterStyle::ThickLineWeight:
        return QFontMetricsF(font).lineWidth() * 1.5;
    case KoCharacterStyle::ThinLineWeight:
        return QFontMetricsF(font).lineWidth() * 0.7;
    case KoCharacterStyle::PercentLineWeight:
        return QFontInfo(font).pointSizeF() * width / 100;
    case KoCharacterStyle::LengthLineWeight:
        return width;
    }
    Q_ASSERT(0); // illegal weight passed
    return 0;
}

void KoTextLayoutArea::decorateParagraph(QPainter *painter, const QTextBlock &block)
{
    QTextLayout *layout = block.layout();
    QTextOption textOption = layout->textOption();

    QTextBlockFormat bf = block.blockFormat();
    QVariantList tabList = bf.property(KoParagraphStyle::TabPositions).toList();
    QFont oldFont = painter->font();

    QTextBlock::iterator it;
    int startOfBlock = -1;
    int currentTabStop = 0;
    // loop over text fragments in this paragraph and draw the underline and line through.
    for (it = block.begin(); !it.atEnd(); ++it) {
        QTextFragment currentFragment = it.fragment();
        if (currentFragment.isValid()) {
            QTextCharFormat fmt = currentFragment.charFormat();
            painter->setFont(fmt.font());
            if (startOfBlock == -1)
                startOfBlock = currentFragment.position(); // start of this block w.r.t. the document
            int firstLine = layout->lineForTextPosition(currentFragment.position() - startOfBlock).lineNumber();
            int lastLine = layout->lineForTextPosition(currentFragment.position() + currentFragment.length()
                    - startOfBlock).lineNumber();
            int startOfFragmentInBlock = currentFragment.position() - startOfBlock;
            Q_ASSERT_X(firstLine <= lastLine, __FUNCTION__, QString("Invalid lines first=%1 last=%2").arg(firstLine).arg(lastLine).toLocal8Bit()); // see bug 278682
            for (int i = firstLine ; i <= lastLine ; i++) {
                QTextLine line = layout->lineAt(i);
                if (layout->isValidCursorPosition(currentFragment.position() - startOfBlock)) {
                    int p1 = currentFragment.position() - startOfBlock;
                    if (block.text().at(p1) != QChar::ObjectReplacementCharacter) {
                        Q_ASSERT_X(line.isValid(), __FUNCTION__, QString("Invalid line=%1 first=%2 last=%3").arg(i).arg(firstLine).arg(lastLine).toLocal8Bit()); // see bug 278682
                        int p2 = currentFragment.position() + currentFragment.length() - startOfBlock;
                        int fragmentToLineOffset = qMax(currentFragment.position() - startOfBlock - line.textStart(),0);
                        qreal x1 = line.cursorToX(p1);
                        qreal x2 = line.cursorToX(p2);
                        // Following line was supposed to fix bug 171686 (I cannot reproduce the original problem) but it opens bug 260159. So, deactivated for now.
                        //x2 = qMin(x2, line.naturalTextWidth() + line.cursorToX(line.textStart()));
                        drawStrikeOuts(painter, currentFragment, line, x1, x2, startOfFragmentInBlock, fragmentToLineOffset);
                        drawOverlines(painter, currentFragment, line, x1, x2, startOfFragmentInBlock, fragmentToLineOffset);
                        drawUnderlines(painter, currentFragment, line, x1, x2, startOfFragmentInBlock, fragmentToLineOffset);
                        decorateTabs(painter, tabList, line, currentFragment, startOfBlock, currentTabStop);
                    }
                }
            }
        }
    }
    painter->setFont(oldFont);
}

void KoTextLayoutArea::drawStrikeOuts(QPainter *painter, const QTextFragment &currentFragment, const QTextLine &line, qreal x1, qreal x2, const int startOfFragmentInBlock, const int fragmentToLineOffset) const
{
    QTextCharFormat fmt = currentFragment.charFormat();
    KoCharacterStyle::LineStyle strikeOutStyle = (KoCharacterStyle::LineStyle)
            fmt.intProperty(KoCharacterStyle::StrikeOutStyle);
    KoCharacterStyle::LineType strikeOutType = (KoCharacterStyle::LineType)
            fmt.intProperty(KoCharacterStyle::StrikeOutType);
    if ((strikeOutStyle != KoCharacterStyle::NoLineStyle) &&
            (strikeOutType != KoCharacterStyle::NoLineType)) {
        QTextCharFormat::VerticalAlignment valign = fmt.verticalAlignment();

        QFont font(fmt.font());
        if (valign == QTextCharFormat::AlignSubScript
                || valign == QTextCharFormat::AlignSuperScript)
            font.setPointSize(qRound(font.pointSize() * 2 / 3.));
        QFontMetricsF metrics(font, m_documentLayout->paintDevice());

        qreal y = line.position().y();
        if (valign == QTextCharFormat::AlignSubScript)
            y += line.height() - metrics.descent() - metrics.strikeOutPos();
        else if (valign == QTextCharFormat::AlignSuperScript)
            y += metrics.ascent() - metrics.strikeOutPos();
        else
            y += line.ascent() - metrics.strikeOutPos();

        QColor color = fmt.colorProperty(KoCharacterStyle::StrikeOutColor);
        if (!color.isValid())
            color = fmt.foreground().color();
        KoCharacterStyle::LineMode strikeOutMode =
            (KoCharacterStyle::LineMode) fmt.intProperty(KoCharacterStyle::StrikeOutMode);

        QString strikeOutText = fmt.stringProperty(KoCharacterStyle::StrikeOutText);
        qreal width = 0; // line thickness
        if (strikeOutText.isEmpty()) {
            width = computeWidth(
                        (KoCharacterStyle::LineWeight) fmt.intProperty(KoCharacterStyle::StrikeOutWeight),
                        fmt.doubleProperty(KoCharacterStyle::StrikeOutWidth),
                        font);
        }
        if (valign == QTextCharFormat::AlignSubScript
                || valign == QTextCharFormat::AlignSuperScript) // adjust size.
            width = width * 2 / 3;

        if (strikeOutMode == KoCharacterStyle::SkipWhiteSpaceLineMode) {
            drawDecorationWords(painter, line, currentFragment.text(), color, strikeOutType,
                    strikeOutStyle, strikeOutText, width, y, fragmentToLineOffset,
                    startOfFragmentInBlock);
        } else {
            if (strikeOutText.isEmpty())
                drawDecorationLine(painter, color, strikeOutType, strikeOutStyle, width, x1, x2, y);
            else
                drawDecorationText(painter, line, color, strikeOutText, x1, x2);
        }
    }
}

void KoTextLayoutArea::drawOverlines(QPainter *painter, const QTextFragment &currentFragment, const QTextLine &line, qreal x1, qreal x2, const int startOfFragmentInBlock, const int fragmentToLineOffset) const
{
    QTextCharFormat fmt = currentFragment.charFormat();
    KoCharacterStyle::LineStyle fontOverLineStyle = (KoCharacterStyle::LineStyle) fmt.intProperty(KoCharacterStyle::OverlineStyle);
    KoCharacterStyle::LineType fontOverLineType = (KoCharacterStyle::LineType) fmt.intProperty(KoCharacterStyle::OverlineType);
    if ((fontOverLineStyle != KoCharacterStyle::NoLineStyle) &&
            (fontOverLineType != KoCharacterStyle::NoLineType)) {
        QTextCharFormat::VerticalAlignment valign = fmt.verticalAlignment();

        QFont font(fmt.font());
        if (valign == QTextCharFormat::AlignSubScript
                || valign == QTextCharFormat::AlignSuperScript)
            font.setPointSize(font.pointSize() * 2 / 3);
        QFontMetricsF metrics(font, m_documentLayout->paintDevice());

        qreal y = line.position().y();
        if (valign == QTextCharFormat::AlignSubScript)
            y += line.height() - metrics.descent() - metrics.overlinePos();
        else if (valign == QTextCharFormat::AlignSuperScript)
            y += metrics.ascent() - metrics.overlinePos();
        else
            y += line.ascent() - metrics.overlinePos();

        QColor color = fmt.colorProperty(KoCharacterStyle::OverlineColor);
        if (!color.isValid())
            color = fmt.foreground().color();
        KoCharacterStyle::LineMode overlineMode =
            (KoCharacterStyle::LineMode) fmt.intProperty(KoCharacterStyle::OverlineMode);
        qreal width = computeWidth( // line thickness
                          (KoCharacterStyle::LineWeight) fmt.intProperty(KoCharacterStyle::OverlineWeight),
                          fmt.doubleProperty(KoCharacterStyle::OverlineWidth),
                          font);
        if (valign == QTextCharFormat::AlignSubScript
                || valign == QTextCharFormat::AlignSuperScript) // adjust size.
            width = width * 2 / 3;

        if (overlineMode == KoCharacterStyle::SkipWhiteSpaceLineMode) {
            drawDecorationWords(painter, line, currentFragment.text(), color, fontOverLineType,
                    fontOverLineStyle, QString(), width, y, fragmentToLineOffset, startOfFragmentInBlock);
        } else {
            drawDecorationLine(painter, color, fontOverLineType, fontOverLineStyle, width, x1, x2, y);
        }
    }
}

void KoTextLayoutArea::drawUnderlines(QPainter *painter, const QTextFragment &currentFragment, const QTextLine &line, qreal x1, qreal x2, const int startOfFragmentInBlock, const int fragmentToLineOffset) const
{
    QTextCharFormat fmt = currentFragment.charFormat();
    KoCharacterStyle::LineStyle fontUnderLineStyle = (KoCharacterStyle::LineStyle) fmt.intProperty(KoCharacterStyle::UnderlineStyle);
    KoCharacterStyle::LineType fontUnderLineType = (KoCharacterStyle::LineType) fmt.intProperty(KoCharacterStyle::UnderlineType);
    if ((fontUnderLineStyle != KoCharacterStyle::NoLineStyle) &&
            (fontUnderLineType != KoCharacterStyle::NoLineType)) {
        QTextCharFormat::VerticalAlignment valign = fmt.verticalAlignment();

        QFont font(fmt.font());
        if (valign == QTextCharFormat::AlignSubScript
                || valign == QTextCharFormat::AlignSuperScript)
            font.setPointSize(font.pointSize() * 2 / 3);
        QFontMetricsF metrics(font, m_documentLayout->paintDevice());

        qreal y = line.position().y();
        if (valign == QTextCharFormat::AlignSubScript)
            y += line.height() - metrics.descent() + metrics.underlinePos();
        else if (valign == QTextCharFormat::AlignSuperScript)
            y += metrics.ascent() + metrics.underlinePos();
        else
            y += line.ascent() + metrics.underlinePos();

        QColor color = fmt.underlineColor();
        if (!color.isValid())
            color = fmt.foreground().color();
        KoCharacterStyle::LineMode underlineMode =
            (KoCharacterStyle::LineMode) fmt.intProperty(KoCharacterStyle::UnderlineMode);
        qreal width = computeWidth( // line thickness
                          (KoCharacterStyle::LineWeight) fmt.intProperty(KoCharacterStyle::UnderlineWeight),
                          fmt.doubleProperty(KoCharacterStyle::UnderlineWidth),
                          font);
        if (valign == QTextCharFormat::AlignSubScript
                || valign == QTextCharFormat::AlignSuperScript) // adjust size.
            width = width * 2 / 3;

        if (underlineMode == KoCharacterStyle::SkipWhiteSpaceLineMode) {
            drawDecorationWords(painter, line, currentFragment.text(), color, fontUnderLineType,
                    fontUnderLineStyle, QString(), width, y, fragmentToLineOffset, startOfFragmentInBlock);
        } else {
            drawDecorationLine(painter, color, fontUnderLineType, fontUnderLineStyle, width, x1, x2, y);
        }
    }
}

// Decorate any tabs ('\t's) in 'currentFragment' and laid out in 'line'.
int KoTextLayoutArea::decorateTabs(QPainter *painter, const QVariantList& tabList, const QTextLine &line, const QTextFragment& currentFragment, int startOfBlock, int currentTabStop)
{
    // If a line in the layout represent multiple text fragments, this function will
    // be called multiple times on the same line, with different fragments.
    // Likewise, if a fragment spans two lines, then this function will be called twice
    // on the same fragment, once for each line.
    QString fragText = currentFragment.text();
    int fragmentOffset = currentFragment.position() - startOfBlock;

    QFontMetricsF fm(currentFragment.charFormat().font());
    qreal tabStyleLineMargin = fm.averageCharWidth() / 4; // leave some margin for the tab decoration line

    // currentFragment.position() : start of this fragment w.r.t. the document
    // startOfBlock : start of this block w.r.t. the document
    // fragmentOffset : start of this fragment w.r.t. the block
    // line.textStart() : start of this line w.r.t. the block

    int searchForTabFrom; // search for \t from this point onwards in fragText
    int searchForTabTill; // search for \t till this point in fragText

    if (line.textStart() >= fragmentOffset) { // fragment starts at or before the start of line
        // we are concerned with only that part of the fragment displayed in this line
        searchForTabFrom = line.textStart() - fragmentOffset;
        // It's a new line. So we should look at the first tab-stop properties for the next \t.
        currentTabStop = 0;
    } else { // fragment starts in the middle of the line
        searchForTabFrom = 0;
    }
    if (line.textStart() + line.textLength() > fragmentOffset + currentFragment.length()) {
        // fragment ends before the end of line. need to see only till the end of the fragment.
        searchForTabTill = currentFragment.length();
    } else {
        // line ends before the fragment ends. need to see only till the end of this line.
        // but then, we need to convert the end of line to an index into fragText
        searchForTabTill = line.textLength() + line.textStart() - fragmentOffset;
    }
    for (int i = searchForTabFrom ; i < searchForTabTill; i++) {
        qreal tabStyleLeftLineMargin = tabStyleLineMargin;
        qreal tabStyleRightLineMargin = tabStyleLineMargin;
        if (currentTabStop >= tabList.size()) // no more decorations
            break;
        if (fragText[i] == '\t') {
            // no margin if its adjacent char is also a tab
            if (i > searchForTabFrom && fragText[i-1] == '\t')
                tabStyleLeftLineMargin = 0;
            if (i < (searchForTabTill - 1) && fragText[i+1] == '\t')
                tabStyleRightLineMargin = 0;

            qreal x1 = line.cursorToX(currentFragment.position() - startOfBlock + i);
            qreal x2 = line.cursorToX(currentFragment.position() - startOfBlock + i + 1);

            // find a tab-stop decoration for this tab position
            // for eg., if there's a tab-stop at 1in, but the text before \t already spans 1.2in,
            // we should look at the next tab-stop
            KoText::Tab tab;
            do {
                tab = qvariant_cast<KoText::Tab>(tabList[currentTabStop]);
                currentTabStop++;
                // comparing with x1 should work for all of left/right/center/char tabs
            } while (tab.position <= x1 && currentTabStop < tabList.size());
            if (tab.position <= x1) // no appropriate tab-stop found
                break;

            qreal y = line.position().y() + line.ascent() - 1 ;
            x1 += tabStyleLeftLineMargin;
            x2 -= tabStyleRightLineMargin;
            QColor tabDecorColor = currentFragment.charFormat().foreground().color();
            if (tab.leaderColor.isValid())
                tabDecorColor = tab.leaderColor;
            qreal width = computeWidth(tab.leaderWeight, tab.leaderWidth, painter->font());
            if (x1 < x2) {
                if (tab.leaderText.isEmpty())
                    drawDecorationLine(painter, tabDecorColor, tab.leaderType, tab.leaderStyle, width, x1, x2, y);
                else
                    drawDecorationText(painter, line, tabDecorColor, tab.leaderText, x1, x2);
            }
        }
    }
    return currentTabStop;
}
