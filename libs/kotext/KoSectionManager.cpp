/*
 *  Copyright (c) 2014 Denis Kuplyakov <dener.kup@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
#include "KoSectionManager.h"

#include <KoSection.h>
#include <KoParagraphStyle.h>
#include "KoSectionEnd.h"
#include <KLocalizedString>
#include <KoTextDocument.h>

#include <QHash>
#include <QString>
#include <qstack.h>
#include <QTextBlock>

#include <kdebug.h>

class KoSectionManagerPrivate
{
public:
    KoSectionManagerPrivate(KoSectionManager *parent, QTextDocument *_doc)
        : doc(_doc)
        , valid(false)
        , q_ptr(parent)
    {
        Q_ASSERT(_doc);
    }

    ~KoSectionManagerPrivate()
    {
        QSet<KoSection *>::iterator it = registeredSections.begin();
        for (; it != registeredSections.end(); it++) {
            delete *it; // KoSectionEnd will be deleted in KoSection
        }
    }

    QTextDocument *doc;
    bool valid; //< is current section info is valid
    QSet<KoSection *> registeredSections; //< stores pointer to sections that sometime was registered

    // used to prevent using sectionNames without update
    QHash<QString, KoSection *> &sectionNames()
    {
        Q_Q(KoSectionManager);
        q->update();
        return m_sectionNames;
    }

protected:
    KoSectionManager *q_ptr;

private:
    Q_DISABLE_COPY(KoSectionManagerPrivate)
    Q_DECLARE_PUBLIC(KoSectionManager);

    QHash<QString, KoSection *> m_sectionNames; //< stores name -> pointer reference, for sections that are visible in document now
};

KoSectionManager::KoSectionManager(QTextDocument* doc)
: d_ptr(new KoSectionManagerPrivate(this, doc))
{
    KoTextDocument(doc).setSectionManager(this); //FIXME: setting it back from here looks bad
}

KoSectionManager::~KoSectionManager()
{
    delete d_ptr;
}

KoSection *KoSectionManager::sectionAtPosition(int pos)
{
    Q_D(KoSectionManager);
    update();

    KoSection *result = 0;
    int smallest = INT_MAX; //smallest in size section will be the deepest
    QHash<QString, KoSection *>::iterator it = d->sectionNames().begin();
    for (; it != d->sectionNames().end(); it++) {
        if (it.value()->bounds().first > pos || it.value()->bounds().second < pos) {
            continue;
        }

        if (it.value()->bounds().second - it.value()->bounds().first < smallest) {
            result = it.value();
            smallest = result->bounds().second - result->bounds().first;
        }
    }

    return result;
}

void KoSectionManager::invalidate()
{
    Q_D(KoSectionManager);
    d->valid = false;
}

bool KoSectionManager::isValidNewName(QString name)
{
    Q_D(KoSectionManager);
    return (d->sectionNames().find(name) == d->sectionNames().end());
}

QString KoSectionManager::possibleNewName()
{
    Q_D(KoSectionManager);

    int i = d->registeredSections.count() + 1;
    while (!isValidNewName(QString(i18n("New section %1")).arg(i))) {
        i++;
    }

    return QString(i18n("New section %1")).arg(i);
}

void KoSectionManager::registerSection(KoSection* section)
{
    Q_D(KoSectionManager);
    d->registeredSections.insert(section);
    invalidate();
}

void KoSectionManager::unregisterSection(KoSection* section)
{
    Q_D(KoSectionManager);
    d->registeredSections.remove(section);
    invalidate();
}

QStandardItemModel *KoSectionManager::update(bool needModel)
{
    Q_D(KoSectionManager);
    if (d->valid && !needModel) {
        return 0;
    }
    d->valid = true;
    d->sectionNames().clear();

    QSet<KoSection *>::iterator it = d->registeredSections.begin();
    for (; it != d->registeredSections.end(); it++) {
        (*it)->setBeginPos(-1);
        (*it)->setEndPos(-1);
        (*it)->setLevel(-1);
    }

    QTextBlock block = d->doc->begin();

    QStandardItemModel *model = 0;
    QStack<QStandardItem *> curChain;

    if (needModel) {
        model = new QStandardItemModel();
        curChain.push(model->invisibleRootItem());
    }

    int curLevel = -1;
    do {
        QTextBlockFormat fmt = block.blockFormat();

        if (fmt.hasProperty(KoParagraphStyle::SectionStartings)) {
            QList<QVariant> starts = fmt.property(KoParagraphStyle::SectionStartings).value< QList<QVariant> >();
            foreach (const QVariant &sv, starts) {
                curLevel++;
                KoSection *sec = static_cast<KoSection *>(sv.value<void *>());
                sec->setBeginPos(block.position());
                sec->setLevel(curLevel);

                d->sectionNames()[sec->name()] = sec;

                if (needModel) {
                    QStandardItem *item = new QStandardItem(sec->name());
                    item->setData(qVariantFromValue(static_cast<void *>(sec)), Qt::UserRole + 1);

                    curChain.top()->appendRow(item);

                    curChain.push(item);
                }
            }
        }

        if (fmt.hasProperty(KoParagraphStyle::SectionEndings)) {
            QList<QVariant> ends = fmt.property(KoParagraphStyle::SectionEndings).value< QList<QVariant> >();
            foreach (const QVariant &sv, ends) {
                curLevel--;
                KoSectionEnd *sec = static_cast<KoSectionEnd *>(sv.value<void *>());
                sec->correspondingSection()->setEndPos(block.position() + block.length());

                if (needModel) {
                    curChain.pop();
                }
            }
        }
    } while ((block = block.next()).isValid());

    return model;
}
