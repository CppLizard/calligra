/*  This file is part of the KDE project

    Copyright (c) 2011 Sven Langkamp <sven.langkamp@gmail.com>
    Copyright (C) 2011 Srikanth Tiyyagura <srikanth.tulasiram@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "KoResourceTagStore.h"

#include <QDebug>
#include <QStringList>
#include <kstandarddirs.h>
#include <QFile>
#include <QDir>
#include <KoResourceServer.h>

KoResourceTagStore::KoResourceTagStore(KoResourceServerBase *resourceServer, const QString& resourceType, const QString& extensions)
    : m_serverExtensions(extensions)
    , m_resourceServer(resourceServer)
{
    m_tagsXMLFile =  KStandardDirs::locateLocal("data", "krita/tags/" + resourceType + "_tags.xml");
}

KoResourceTagStore::~KoResourceTagStore()
{
    serializeTags();
}

QStringList KoResourceTagStore::assignedTagsList(KoResource* resource) const
{
    return m_resourceToTag.values(resource);
}

void KoResourceTagStore::removeResource(const KoResource *resource)
{
    m_resourceToTag.remove(resource);
}

QStringList KoResourceTagStore::tagNamesList() const
{
    return m_tagList.uniqueKeys();
}

void KoResourceTagStore::addTag(KoResource* resource, const QString& tag)
{
    if (!resource) {
        m_tagList.insert(tag, 0);
        return;
    }
    if (m_resourceToTag.contains(resource, tag)) {
        return;
    }
    m_resourceToTag.insert(resource, tag);
    if (m_tagList.contains(tag)) {
        m_tagList[tag]++;
    } else {
        m_tagList.insert(tag, 1);
    }
}

void KoResourceTagStore::delTag(KoResource* resource, const QString& tag)
{
    int res = m_resourceToTag.remove(resource, tag);

    if (res > 0) { // decrease the usecount for this tag
        if (m_tagList.contains(tag)) {
            if (m_tagList[tag] > 0) {
                m_tagList[tag]--;
            }
        }
    }
}

void KoResourceTagStore::delTag(const QString& tag)
{
    Q_ASSERT(!m_resourceToTag.values().contains(tag));
    m_tagList.remove(tag);
}

QStringList KoResourceTagStore::searchTag(const QString& lineEditText)
{
    QStringList tagsList = lineEditText.split(QRegExp("[,]\\s*"), QString::SkipEmptyParts);
    if (tagsList.isEmpty()) {
        return QStringList();
    }

    QSet<const KoResource*> resources;

    foreach (QString tag, tagsList) {
        foreach (const KoResource *res, m_resourceToTag.keys(tag)) {
            resources << res;
        }
    }
    QStringList filenames;
    foreach (const KoResource *res, resources) {
        filenames << adjustedFileName(res->shortFilename());
    }

    return removeAdjustedFileNames(filenames);
}

void KoResourceTagStore::loadTags()
{
    readXMLFile(m_tagsXMLFile);
}

void KoResourceTagStore::writeXMLFile(const QString &tagstore)
{
    QFile f(tagstore);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        kWarning() << "Cannot write meta information to '" << tagstore << "'.";
        return;
    }
    QDomDocument doc;
    QDomElement root;

    QDomDocument docTemp("tags");
    doc = docTemp;
    doc.appendChild(doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\""));
    root = doc.createElement("tags");
    doc.appendChild(root);


    foreach (const KoResource *res, m_resourceToTag.uniqueKeys()) {
        QDomElement resourceEl = doc.createElement("resource");
        resourceEl.setAttribute("identifier", res->filename().replace(QDir::homePath(), QString("~")));
        resourceEl.setAttribute("md5", QString(res->md5().toBase64()));

        foreach (const QString &tag, m_resourceToTag.values(res)) {
            QDomElement tagEl = doc.createElement("tag");
            tagEl.appendChild(doc.createTextNode(tag));
            resourceEl.appendChild(tagEl);
        }
        root.appendChild(resourceEl);

    }

    // Now write empty tags
    foreach (const QString &tag, m_tagList.uniqueKeys())  {
        if (m_tagList[tag] == 0) {
            QDomElement resourceEl = doc.createElement("resource");
            resourceEl.setAttribute("identifier", "dummy");
            QDomElement tagEl = doc.createElement("tag");
            tagEl.appendChild(doc.createTextNode(tag));
            resourceEl.appendChild(tagEl);
             root.appendChild(resourceEl);
        }
    }

    QTextStream metastream(&f);
    metastream << doc.toString();

    f.close();

}

void KoResourceTagStore::readXMLFile(const QString &tagstore)
{

    QString inputFile;

    if (QFile::exists(tagstore)) {
        inputFile = tagstore;
    } else {
        inputFile = KStandardDirs::locateLocal("data", "krita/tags.xml");
    }

    QFile f(inputFile);
    if (!f.open(QIODevice::ReadOnly)) {
        return;
    }

    QDomDocument doc;
    if (!doc.setContent(&f)) {
        kWarning() << "The file could not be parsed.";
        return;
    }

    QDomElement root = doc.documentElement();
    if (root.tagName() != "tags") {
        kWarning() << "The file doesn't seem to be of interest.";
        return;
    }

    QDomNodeList resourceNodesList = root.childNodes();

    QByteArray resourceMD5;
    QString fileName;

    for (int i = 0; i < resourceNodesList.count(); i++) {
        QDomElement element = resourceNodesList.at(i).toElement();
        if (element.tagName() == "resource") {

            KoResource *resByMd5 = 0;
            KoResource *resByFileName = 0;

            if (element.hasAttribute("md5")) {
                resourceMD5 = QByteArray::fromBase64(element.attribute("md5").toLatin1());
                resByMd5 = m_resourceServer->byMd5(resourceMD5);
            }

            if (element.hasAttribute("identifier")) {
                fileName = element.attribute("identifier");
                QFileInfo fi(fileName);
                resByFileName = m_resourceServer->byFileName(fi.fileName());
            }

            if (fileName == "dummy" || isServerResource(fileName)) {

                QDomNodeList tagNodesList = resourceNodesList.at(i).childNodes();

                for (int j = 0; j < tagNodesList.count() ; j++) {

                    QDomElement tagEl = tagNodesList.at(j).toElement();

                    if (fileName != "dummy") {
                        QFileInfo fi(fileName);
                        KoResource *res = m_resourceServer->byFileName(fi.fileName());
                        addTag(res, tagEl.text());
                    } else {
                        addTag(0, tagEl.text());
                    }
                }
            }
            else {
                if (!resByMd5 && !resByFileName) {
                    // no resource found, hopeless...
                    continue;
                }

                KoResource *res = 0;

                if (resByMd5 && resByFileName && (resByMd5 != resByFileName)) {
                    kWarning() << "MD5sum and filename point to different resources -- was the resource renamed? We go with md5";
                    res = resByMd5;
                }
                else if (!resByMd5 && resByFileName) {
                    // We didn't find the resource by md5, but did find it by filename, so take that one
                    res = resByFileName;
                }
                else {
                    res = resByMd5;
                }
                if (res) {
                    QDomNodeList tagNodesList = resourceNodesList.at(i).childNodes();
                    for (int j = 0; j < tagNodesList.count() ; j++) {
                        QDomElement tagEl = tagNodesList.at(j).toElement();
                        addTag(res, tagEl.text());
                    }
                }
                else {
                    kWarning() << "Could not find resource for md5:" << resByMd5 << "or filename" << fileName;
                }
            }
        }
    }
}

bool KoResourceTagStore::isServerResource(const QString &resourceName) const
{
    bool removeChild = false;
    QStringList extensionsList = m_serverExtensions.split(':');
    foreach (QString extension, extensionsList) {
        if (resourceName.contains(extension.remove('*'))) {
            removeChild = true;
            break;
        }
    }
    return removeChild;
}

QString KoResourceTagStore::adjustedFileName(const QString &fileName) const
{
    if (!isServerResource(fileName)) {
        return fileName + "-krita" + m_serverExtensions.split(':').takeFirst().remove('*');
    }
    return fileName;
}

QStringList KoResourceTagStore::removeAdjustedFileNames(QStringList fileNamesList)
{
    foreach (const QString & fileName, fileNamesList) {
        if (fileName.contains("-krita")) {
            fileNamesList.append(fileName.split("-krita").takeFirst());
            fileNamesList.removeAll(fileName);
        }
    }
    return fileNamesList;
}

void KoResourceTagStore::serializeTags()
{
    writeXMLFile(m_tagsXMLFile);
}
