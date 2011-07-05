/*
    Copyright (C) 2011 Pierre Ducroquet <pinaraf@pinaraf.info>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

*/

#include "TestOpenDocumentStyle.h"
#include <KoTableCellStyle.h>
#include <KoTableColumnStyle.h>
#include <KoTableRowStyle.h>
#include <KoTableStyle.h>
#include <KoParagraphStyle.h>
#include <KoXmlReader.h>
#include <KoXmlWriter.h>
#include <KoOdfLoadingContext.h>
#include <KoShapeLoadingContext.h>
#include <KoGenStyle.h>
#include <KoGenStyles.h>
#include <KoXmlNS.h>

#include <KDebug>

#include <QBuffer>
#include <QDomDocument>
#include <QDomElement>

Attribute::Attribute(const QDomElement& element)
    : m_references()
{
    if (element.firstChildElement() != element.lastChildElement()) {
        kFatal() << "We don't handle complex attributes so far";
    }
    QDomElement content = element.firstChildElement();
    if (content.tagName() == "ref") {
        m_references << content.attribute("name");
    }
    m_name = element.attribute("name");
    m_values = listValuesFromNode(element);
}

QString Attribute::name()
{
    return m_name;
}

QStringList Attribute::listValues()
{
    return m_values;
}

QStringList Attribute::listValuesFromNode(const QDomElement &m_node)
{
    QStringList result;
    if (m_references.size() == 0) {
        // Parse the content of the attribute
        QDomElement content = m_node.firstChildElement();
        if (content.tagName() == "choice") {
            QDomElement valueChild = content.firstChildElement();
            do {
                if (valueChild.tagName() == "value") {
                    result << valueChild.text();
                } else if (valueChild.tagName() == "ref") {
                    m_references << valueChild.attribute("name");
                } else if (valueChild.tagName() == "list") {
                    // Parse that sublist
                    if (valueChild.childNodes().length() != 1) {
                        kFatal() << "Unrecognized list element in " << m_name;
                    }
                    QDomElement subElement = valueChild.firstChildElement();
                    if (subElement.nodeName() == "oneOrMore") {
                        // Build a list of each sub item
                        QStringList allowedValues;
                        QDomElement subChoices = subElement.firstChildElement();
                        if (subChoices.nodeName() != "choice") {
                            kFatal() << "Unrecognized oneOrMore element in " << m_name;
                        }
                        QDomElement subValueChild = subChoices.firstChildElement();
                        do {
                            if (subValueChild.nodeName() == "value") {
                                allowedValues << subValueChild.text();
                            } else {
                                kFatal() << "Unrecognized oneOrMore element in " << m_name;
                            }
                            subValueChild = subValueChild.nextSiblingElement();
                        } while (!subValueChild.isNull());
                        QStringList mergedAllowedValues;
                        while (mergedAllowedValues.length() != (pow(allowedValues.length(), allowedValues.length()))) {
                            foreach (QString baseValue, allowedValues) {
                                if (!mergedAllowedValues.contains(baseValue))
                                    mergedAllowedValues << baseValue;
                                foreach (QString knownValue, mergedAllowedValues) {
                                    if ((knownValue == baseValue) || (knownValue.contains(baseValue + " ")) || (knownValue.contains(" " + baseValue))) {
                                        continue;
                                    }
                                    QString builtValue = knownValue + " " + baseValue;
                                    if (!mergedAllowedValues.contains(builtValue))
                                        mergedAllowedValues << builtValue;
                                }
                            }
                        }
                        foreach (QString allowedValue, mergedAllowedValues) {
                            QStringList equivalenceList;
                            equivalenceList << allowedValue;
                            
                            QStringList currentList = allowedValue.split(' ');
                            currentList.sort();
                            
                            foreach (QString otherAllowedValue, mergedAllowedValues) {
                                if (otherAllowedValue == allowedValue)
                                    continue;
                                
                                QStringList otherList = otherAllowedValue.split(' ');
                                otherList.sort();
                                if (otherList == currentList)
                                    equivalenceList << otherAllowedValue;
                            }
                            equivalenceList.sort();
                            if (!m_equivalences.contains(equivalenceList))
                                m_equivalences << equivalenceList;
                        }
                        result << mergedAllowedValues;
                    }
                } else {
                    kFatal() << "Unrecognized choice element in " << m_name << " : " << valueChild.tagName();
                }
                valueChild = valueChild.nextSiblingElement();
            } while (!valueChild.isNull());
        } else {
            kFatal() << "Unhandled attribute value node " << content.tagName();
        }
    }
    foreach (QString reference, m_references) {
        if (reference == "boolean") {
            result << "true" << "false";
        } else if (reference == "positiveLength") {
            result << "42px" << "42pt" << "12cm";
        } else if (reference == "length") {
            result << "42px" << "42pt" << "12cm" << "-5in";
        } else if (reference == "nonNegativeLength") {
            result << "42px" << "42pt" << "12cm" << "0in";
        } else if (reference == "relativeLength") {
            result << "42*";
        } else if (reference == "shadowType") {
            kWarning() << "Not fully supported : shadowType.";
            result << "none";
        } else if (reference == "color") {
            result << "#ABCDEF" << "#0a1234";
        } else if (reference == "positiveInteger") {
            result << "37" << "42";
        } else if (reference == "nonNegativeInteger") {
            result << "0" << "42";
        } else if (reference == "percent") {
            result << "-50%" << "0%" << "100%" << "42%";
        } else if (reference == "borderWidths") {
            result << "42px 42pt 12cm" << "0px 0pt 0cm";
        } else if (reference == "angle") {
            result << "5deg" << "1rad" << "400grad" << "3.14159265rad" << "45";    // OpenDocument 1.1 : no unit == degrees
        } else if (reference == "zeroToHundredPercent") {
            result << "0%" << "10%" << "100%" << "13.37%" << "42.73%";
        } else if (reference == "string") {
            // Now, that sucks !
            kWarning() << "Found a string reference in " << m_name;
            result << "";
        } else {
            kFatal() << "Unhandled reference " << reference << "( in " << m_name << ")";
        }
    }
    return result;
}

bool Attribute::compare(const QString& initialValue, const QString& outputValue)
{
    if (outputValue.isEmpty())
        return false;
    if (initialValue == outputValue)
        return true;
    if (m_references.contains("percent") && initialValue.contains('%'))
        return false;
    
    // -----------   Special cases
    if (m_name == "style:glyph-orientation-vertical")
        if ((initialValue.at(0) == '0') && (outputValue.at(0) == '0'))
            return true;
    if (m_name == "style:writing-mode")
        return KoText::directionFromString(initialValue) == KoText::directionFromString(outputValue);
    // -----------
    
    foreach (QString reference, m_references) {
        if ((reference == "positiveLength") || (reference == "nonNegativeLength") || (reference == "length")) {
            if (qAbs(KoUnit::parseValue(initialValue) - KoUnit::parseValue(outputValue)) < 0.0001)
                return true;
        } else if (reference == "color") {
            if (initialValue.toLower() == outputValue.toLower())
                return true;
        } else if (reference == "angle") {
            return qAbs(KoUnit::parseAngle(initialValue) - KoUnit::parseAngle(outputValue)) < 0.0001;
        }
    }
    if (!m_equivalences.empty()) {
        // Check every equivalence value
        foreach (QStringList equivalenceList, m_equivalences) {
            if (equivalenceList.contains(outputValue)) {
                foreach (QString otherValue, equivalenceList) {
                    if ((otherValue != outputValue) && (compare(initialValue, otherValue)))
                        return true;
                }
            }   
        }
    }
    return false;
}


TestOpenDocumentStyle::TestOpenDocumentStyle()
    : QObject()
{
}

void TestOpenDocumentStyle::initTestCase()
{
    // Parse the relaxng file quickly
    //QString fileName(SPECS_DATA_DIR "OpenDocument-schema-v1.1.rng");
    QString fileName(SPECS_DATA_DIR "OpenDocument-v1.2-cs01-schema.rng");
    kDebug() << fileName;
    QFile specFile(fileName);
    specFile.open(QIODevice::ReadOnly);
    QDomDocument specDocument;
    specDocument.setContent(&specFile);

    int count = 0;
    QDomElement mainElement = specDocument.documentElement();
    QDomNode n = mainElement.firstChild();
    while(!n.isNull()) {
        QDomElement e = n.toElement(); // try to convert the node to an element.
        if (!e.isNull()) {
            count++;
            m_rngRules.insertMulti(e.attribute("name"), e);
        }
        n = n.nextSibling();
    }
}

QList<Attribute*> TestOpenDocumentStyle::listAttributesFromRNGName(const QString& name)
{
    QList<Attribute*> result;
    if (!m_rngRules.contains(name))
        return result;
    QList<QDomElement> elements = m_rngRules.values(name);
    if ((elements.count() == 1) && (elements.first().firstChildElement().tagName() == "interleave"))
    {
        QDomElement root = elements.first().firstChildElement();
        elements.clear();
        elements.append(root);
    }
    foreach (QDomElement element, elements) {
        QDomElement child = element.firstChildElement();
        do {
            if (child.tagName() == "ref") {
                result.append(listAttributesFromRNGName(child.attribute("name")));
            } else if (child.tagName() == "optional") {
                QDomElement optionChild = child.firstChildElement();
                do {
                    if (optionChild.tagName() == "attribute") {
                        result << new Attribute(optionChild);
                    } else {
                        kFatal() << "Unrecognized optional element : " << child.tagName();
                    }
                    optionChild = optionChild.nextSiblingElement();
                } while (!optionChild.isNull());
            } else {
                kFatal() << "Unrecognized element : " << child.tagName();
            }
            child = child.nextSiblingElement();
        } while (!child.isNull());
    }
    return result;
}

QByteArray TestOpenDocumentStyle::generateStyleNodeWithAttribute(const QString& styleFamily, const QString& attributeName, const QString& attributeValue)
{
    QBuffer xmlOutputBuffer;
    KoXmlWriter *xmlWriter = new KoXmlWriter(&xmlOutputBuffer);
    xmlWriter->startDocument("style:style");
    xmlWriter->startElement("style:style");
    xmlWriter->addAttribute("xmlns:style", KoXmlNS::style);
    xmlWriter->addAttribute("xmlns:fo", KoXmlNS::fo);
    xmlWriter->addAttribute("xmlns:table", KoXmlNS::table);
    xmlWriter->addAttribute("xmlns:text", KoXmlNS::text);
    xmlWriter->addAttribute("style:name", "TestStyle");
    xmlWriter->addAttribute("style:family", styleFamily);
    xmlWriter->startElement(("style:" + styleFamily + "-properties").toLatin1());
    xmlWriter->addAttribute(attributeName.toLatin1(), attributeValue);
    xmlWriter->endElement();
    xmlWriter->endElement();
    xmlWriter->endDocument();
    delete(xmlWriter);

    return xmlOutputBuffer.data();
}

QByteArray TestOpenDocumentStyle::generateStyleProperties(const KoGenStyle& genStyle, const QString &styleFamily)
{
    QBuffer xmlOutputBuffer;
    KoXmlWriter *xmlWriter = new KoXmlWriter(&xmlOutputBuffer);

    xmlWriter->startDocument("style:style");
    KoGenStyles genStyles;
    genStyle.writeStyle(xmlWriter, genStyles, "style:style", "SavedTestStyle", ("style:" + styleFamily + "-properties").toLatin1());

    xmlWriter->endDocument();

    delete(xmlWriter);
    return xmlOutputBuffer.data();
}

template<class T>
void loadOdf(T* genStyle, const KoXmlElement *mainElement, KoOdfLoadingContext &loadCtxt)
{
    genStyle->loadOdf(mainElement, loadCtxt);
}

template<>
void loadOdf<KoTableCellStyle>(KoTableCellStyle* genStyle, const KoXmlElement *mainElement, KoOdfLoadingContext &loadCtxt)
{
    KoShapeLoadingContext shapeCtxt(loadCtxt, 0);
    genStyle->loadOdf(mainElement, shapeCtxt);
}

template<>
void loadOdf<KoParagraphStyle>(KoParagraphStyle* genStyle, const KoXmlElement *mainElement, KoOdfLoadingContext &loadCtxt)
{
    KoShapeLoadingContext shapeCtxt(loadCtxt, 0);
    genStyle->loadOdf(mainElement, shapeCtxt);
}

template<class T>
void saveOdf(T* genStyle, KoGenStyle *styleWriter)
{
    genStyle->saveOdf(*styleWriter);
}

template<>
void saveOdf<KoParagraphStyle>(KoParagraphStyle *genStyle, KoGenStyle *styleWriter)
{
    KoGenStyles styles;
    genStyle->saveOdf(*styleWriter, styles);
}

template<class T>
bool TestOpenDocumentStyle::basicTestFunction(KoGenStyle::Type family, const QString &familyName, Attribute *attribute, const QString &value)
{
    T basicStyle;
    T genStyle;
    KoOdfStylesReader stylesReader;
    KoOdfLoadingContext loadCtxt(stylesReader, 0);

    QByteArray xmlOutputData = this->generateStyleNodeWithAttribute(familyName, attribute->name(), value);
    KoXmlDocument *xmlReader = new KoXmlDocument;
    xmlReader->setContent(xmlOutputData, true);
    KoXmlElement mainElement = xmlReader->documentElement();
    loadOdf<T>(&genStyle, &mainElement, loadCtxt);

    // THAT is often poorly implemented
    //QVERIFY(not(genStyle == basicStyle));

    KoGenStyle styleWriter(family, familyName.toLatin1());
    saveOdf<T>(&genStyle, &styleWriter);

    QByteArray generatedXmlOutput = generateStyleProperties(styleWriter, familyName);

    KoXmlDocument *generatedXmlReader = new KoXmlDocument;
    if (!generatedXmlReader->setContent(generatedXmlOutput)) {
        kDebug() << "Output XML seems not to be valid : " << generatedXmlOutput;
        kFatal() << "Unable to set content";
    }

    KoXmlElement root = generatedXmlReader->documentElement();
    KoXmlElement properties = root.firstChild().toElement();
    QString outputPropertyValue = properties.attribute(attribute->name());
    kDebug(32500) << "Comparing " << outputPropertyValue << "obtained for " << value;
    if (properties.attributeNames().count() > 1)
    {
        kWarning(32500) << "Warning : got more than one attribute !";
    }
    return attribute->compare(value, outputPropertyValue);
}

void TestOpenDocumentStyle::testTableColumnStyle_data()
{
    QList<Attribute*> attributes = listAttributesFromRNGName("style-table-column-properties-attlist");
    QTest::addColumn<Attribute*>("attribute");
    QTest::addColumn<QString>("value");
    foreach (Attribute *attribute, attributes) {
        foreach (QString value, attribute->listValues()) {
            QTest::newRow(attribute->name().toLatin1()) << attribute << value;
        }
    }
}

void TestOpenDocumentStyle::testTableColumnStyle()
{
    QFETCH(Attribute*, attribute);
    QFETCH(QString, value);

    QVERIFY(basicTestFunction<KoTableColumnStyle>(KoGenStyle::TableColumnStyle, "table-column", attribute, value));
}

void TestOpenDocumentStyle::testTableStyle_data()
{
    QList<Attribute*> attributes = listAttributesFromRNGName("style-table-properties-attlist");
    QTest::addColumn<Attribute*>("attribute");
    QTest::addColumn<QString>("value");
    foreach (Attribute *attribute, attributes) {
        foreach (QString value, attribute->listValues()) {
            QTest::newRow(attribute->name().toLatin1()) << attribute << value;
        }
    }
}

void TestOpenDocumentStyle::testTableStyle()
{
    QFETCH(Attribute*, attribute);
    QFETCH(QString, value);

    QVERIFY(basicTestFunction<KoTableStyle>(KoGenStyle::TableStyle, "table", attribute, value));
}

void TestOpenDocumentStyle::testTableRowStyle_data()
{
    QList<Attribute*> attributes = listAttributesFromRNGName("style-table-row-properties-attlist");
    QTest::addColumn<Attribute*>("attribute");
    QTest::addColumn<QString>("value");
    foreach (Attribute *attribute, attributes) {
        foreach (QString value, attribute->listValues()) {
            QTest::newRow(attribute->name().toLatin1()) << attribute << value;
        }
    }
}

void TestOpenDocumentStyle::testTableRowStyle()
{
    QFETCH(Attribute*, attribute);
    QFETCH(QString, value);

    QVERIFY(basicTestFunction<KoTableRowStyle>(KoGenStyle::TableRowStyle, "table-row", attribute, value));
}

void TestOpenDocumentStyle::testTableCellStyle_data()
{
    QList<Attribute*> attributes = listAttributesFromRNGName("style-table-cell-properties-attlist");
    QTest::addColumn<Attribute*>("attribute");
    QTest::addColumn<QString>("value");
    foreach (Attribute *attribute, attributes) {
        foreach (QString value, attribute->listValues()) {
            QTest::newRow(attribute->name().toLatin1()) << attribute << value;
        }
    }
}

void TestOpenDocumentStyle::testTableCellStyle()
{
    QFETCH(Attribute*, attribute);
    QFETCH(QString, value);

    QVERIFY(basicTestFunction<KoTableCellStyle>(KoGenStyle::TableCellStyle, "table-cell", attribute, value));
}

void TestOpenDocumentStyle::testParagraphStyle_data()
{
    QList<Attribute*> attributes = listAttributesFromRNGName("style-paragraph-properties-attlist");
    QTest::addColumn<Attribute*>("attribute");
    QTest::addColumn<QString>("value");
    foreach (Attribute *attribute, attributes) {
        foreach (QString value, attribute->listValues()) {
            QTest::newRow(attribute->name().toLatin1()) << attribute << value;
        }
    }
}

void TestOpenDocumentStyle::testParagraphStyle()
{
    QFETCH(Attribute*, attribute);
    QFETCH(QString, value);

    QVERIFY(basicTestFunction<KoParagraphStyle>(KoGenStyle::ParagraphStyle, "paragraph", attribute, value));
}

QTEST_MAIN(TestOpenDocumentStyle)
#include <TestOpenDocumentStyle.moc>
