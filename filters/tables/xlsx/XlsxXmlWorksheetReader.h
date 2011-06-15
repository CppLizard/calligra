/*
 * This file is part of Office 2007 Filters for Calligra
 *
 * Copyright (C) 2010 Sebastian Sauer <sebsauer@kdab.com>
 * Copyright (C) 2009-2010 Nokia Corporation and/or its subsidiary(-ies).
 *
 * Contact: Suresh Chande suresh.chande@nokia.com
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#ifndef XLSXXMLWORKSHEETREADER_H
#define XLSXXMLWORKSHEETREADER_H

#include <MsooXmlThemesReader.h>
#include <MsooXmlCommonReader.h>

#include <KoGenStyle.h>
#include <styles/KoCharacterStyle.h>

class XlsxXmlWorksheetReaderContext;
class XlsxXmlDocumentReaderContext;
class XlsxComments;
class XlsxStyles;
class XlsxImport;
class Sheet;

//! A class reading MSOOXML XLSX markup - xl/worksheets/sheet*.xml part.
class XlsxXmlWorksheetReader : public MSOOXML::MsooXmlCommonReader
{
public:
    explicit XlsxXmlWorksheetReader(KoOdfWriters *writers);

    virtual ~XlsxXmlWorksheetReader();

    //! Reads/parses the XML.
    virtual KoFilter::ConversionStatus read(MSOOXML::MsooXmlReaderContext* context = 0);

protected:
    KoFilter::ConversionStatus readInternal();
    KoFilter::ConversionStatus read_chartsheet();
    KoFilter::ConversionStatus read_worksheet();
    KoFilter::ConversionStatus read_dialogsheet();
    KoFilter::ConversionStatus read_sheetHelper(const QString& type);
    KoFilter::ConversionStatus read_sheetFormatPr();
    KoFilter::ConversionStatus read_cols();
    KoFilter::ConversionStatus read_col();
    KoFilter::ConversionStatus read_sheetData();
    KoFilter::ConversionStatus read_conditionalFormatting();
    KoFilter::ConversionStatus read_cfRule();
    KoFilter::ConversionStatus read_formula();
    KoFilter::ConversionStatus read_row();
    KoFilter::ConversionStatus read_c();
    KoFilter::ConversionStatus read_f();
    KoFilter::ConversionStatus read_v();
    KoFilter::ConversionStatus read_mergeCell();
    KoFilter::ConversionStatus read_mergeCells();
    KoFilter::ConversionStatus read_drawing();
    KoFilter::ConversionStatus read_legacyDrawing();
    KoFilter::ConversionStatus read_hyperlink();
    KoFilter::ConversionStatus read_hyperlinks();
    KoFilter::ConversionStatus read_picture();
    KoFilter::ConversionStatus read_autoFilter();
    KoFilter::ConversionStatus read_filterColumn();
    KoFilter::ConversionStatus read_filters();
    KoFilter::ConversionStatus read_filter();
    KoFilter::ConversionStatus read_customFilters();
    KoFilter::ConversionStatus read_customFilter();
    KoFilter::ConversionStatus read_oleObjects();
    KoFilter::ConversionStatus read_oleObject();
    KoFilter::ConversionStatus read_tableParts();
    KoFilter::ConversionStatus read_tablePart();

    XlsxXmlWorksheetReaderContext* m_context;

    KoGenStyle m_tableStyle;
    //! for table:table-column
    int m_columnCount;
    //! for table:table-row
    int m_currentRow;
    //! for table:table-cell
    int m_currentColumn;
    //! Used in read_f() and read_v()
    QString m_value;

    // for optimization
    static const char* officeValue;
    static const char* officeDateValue;
    static const char* officeTimeValue;
    static const char* officeStringValue;
    static const char* officeBooleanValue;

private:
    void init();

    void showWarningAboutWorksheetSize();
    void saveColumnStyle(const QString& widthString);
    QString processRowStyle(const QString& heightString = QString());
    void appendTableColumns(int columns, const QString& width = QString());
    void appendTableCells(int cells);
    //! Saves annotation element (comments) for cell specified by @a col and @a row it there is any annotation defined.
    void saveAnnotation(int col, int row);

    typedef QPair<int, QMap<QString, QString> > Condition;
    QList<Condition> m_conditionalIndices;
    QMap<QString, QList<Condition> > m_conditionalStyles;

    QString m_formula;

#include <MsooXmlCommonReaderMethods.h>
#include <MsooXmlCommonReaderDrawingMLMethods.h>

    class Private;
    Private* const d;
};

class XlsxXmlWorksheetReaderContext : public MSOOXML::MsooXmlReaderContext
{
public:
    //! Creates the context object.
    XlsxXmlWorksheetReaderContext(
        uint _worksheetNumber,
        uint _numberOfWorkSheets,
        const QString& _worksheetName,
        const QString& _state,
        const QString _path, const QString _file,
        MSOOXML::DrawingMLTheme*& _themes,
        const QVector<QString>& _sharedStrings,
        const XlsxComments& _comments,
        const XlsxStyles& _styles,
        MSOOXML::MsooXmlRelationships& _relationships,
        XlsxImport* _import,
        QMap<QString, QString> _oleReplacements,
        QMap<QString, QString> _oleBeginFrames);

    virtual ~XlsxXmlWorksheetReaderContext();

    Sheet* sheet;
    const uint worksheetNumber;
    const uint numberOfWorkSheets;
    const QString worksheetName;
    QString state;
    MSOOXML::DrawingMLTheme* themes;
    const QVector<QString> *sharedStrings;
    const XlsxComments* comments;
    const XlsxStyles* styles;

    XlsxImport* import;
    const QString path;
    const QString file;

    QMap<QString, QString> oleReplacements;
    QMap<QString, QString> oleFrameBegins;

    struct AutoFilterCondition {
        QString field;
        QString value;
        QString opField;
    };

    struct AutoFilter {
        QString type; // empty, -and, -or
        QString area;
        QString field;
        QVector<AutoFilterCondition> filterConditions;
    };

    QVector<XlsxXmlWorksheetReaderContext::AutoFilter> autoFilters;

    AutoFilterCondition currentFilterCondition;

    bool firstRoundOfReading;

    QList<QMap<QString, QString> > conditionalStyleForPosition(const QString& positionLetter, int positionNumber);

    QList<QPair<QString, QMap<QString, QString> > >conditionalStyles;
};

#endif
