/*
 *  Copyright (c) 2004 Boudewijn Rempt <boud@valdyas.org>
 *  Copyright (c) 2005 Bart Coppens <kde@bartcoppens.be>
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
#include "kis_imagepipe_brush.h"
#include "kis_imagepipe_brush_p.h"
#include "kis_brushes_pipe.h"


class KisImageBrushesPipe : public KisBrushesPipe<KisGbrBrush>
{
protected:
    void selectNextBrush(const KisPaintInformation& info) {
        quint32 brushIndex = 0;

        double angle;
        for (int i = 0; i < m_parasite.dim; i++) {
            int index = m_parasite.index[i];
            switch (m_parasite.selection[i]) {
            case KisParasite::Constant: break;
            case KisParasite::Incremental:
                index = (index + 1) % m_parasite.rank[i]; break;
            case KisParasite::Random:
                index = int(float(m_parasite.rank[i]) * KRandom::random() / RAND_MAX); break;
            case KisParasite::Pressure:
                index = static_cast<int>(info.pressure() * (m_parasite.rank[i] - 1) + 0.5); break;
            case KisParasite::Angular:
                // + m_d->PI_2 to be compatible with the gimp
                angle = info.angle() + M_PI_2;
                // We need to be in the [0..2*Pi[ interval so that we can more nicely select it
                if (angle < 0)
                    angle += 2.0 * M_PI;
                else if (angle > 2.0 * M_PI)
                    angle -= 2.0 * M_PI;
                index = static_cast<int>(angle / (2.0 * M_PI) * m_parasite.rank[i]);
                break;
            default:
                warnImage << "This parasite KisParasite::SelectionMode has not been implemented. Reselecting"
                          << " to Constant";
                m_parasite.selection[i] = KisParasite::Constant; // Not incremental, since that assumes rank > 0
                index = 0;
            }
            m_parasite.index[i] = index;
            brushIndex += m_parasite.brushesCount[i] * index;
        }
        brushIndex %= m_brushes.size();

        m_currentBrushIndex = brushIndex;
    }

public:
    using KisBrushesPipe<KisGbrBrush>::addBrush;

    void setParasite(const KisPipeBrushParasite& parasite) {
        m_parasite = parasite;
    }

    const KisPipeBrushParasite& parasite() const {
        return m_parasite;
    }

    void setUseColorAsMask(bool useColorAsMask) {
        foreach (KisGbrBrush *brush, m_brushes) {
            brush->setUseColorAsMask(useColorAsMask);
        }
    }

    void makeMaskImage() {
        foreach (KisGbrBrush *brush, m_brushes) {
            brush->makeMaskImage();
        }
    }

    bool saveToDevice(QIODevice* dev) const {
        foreach (KisGbrBrush *brush, m_brushes) {
            if(!brush->saveToDevice(dev)) {
                return false;
            }
        }
        return true;
    }

private:
    KisPipeBrushParasite m_parasite;
};


struct KisImagePipeBrush::Private
{
public:
    KisImageBrushesPipe brushesPipe;
};

KisImagePipeBrush::KisImagePipeBrush(const QString& filename)
        : KisGbrBrush(filename)
        , m_d(new Private())
{
}

KisImagePipeBrush::KisImagePipeBrush(const QString& name, int w, int h,
                                     QVector< QVector<KisPaintDevice*> > devices,
                                     QVector<KisParasite::SelectionMode > modes)
        : KisGbrBrush("")
        , m_d(new Private())
{
    Q_ASSERT(devices.count() == modes.count());
    Q_ASSERT(devices.count() > 0);
    Q_ASSERT(devices.count() < 2); // XXX Multidimensionals not supported yet, change to MaxDim!

    setName(name);

    KisPipeBrushParasite parasite;

    parasite.dim = devices.count();
    // XXX Change for multidim! :
    parasite.ncells = devices.at(0).count();
    parasite.rank[0] = parasite.ncells; // ### This can masquerade some bugs, be careful here in the future
    parasite.selection[0] = modes.at(0);
    // XXX needsmovement!

    parasite.setBrushesCount();

    m_d->brushesPipe.setParasite(parasite);
    for (int i = 0; i < devices.at(0).count(); i++) {
        m_d->brushesPipe.addBrush(new KisGbrBrush(devices.at(0).at(i), 0, 0, w, h));
    }

    setImage(m_d->brushesPipe.firstBrush()->image());
}

KisImagePipeBrush::KisImagePipeBrush(const KisImagePipeBrush& rhs)
        : KisGbrBrush(rhs),
        m_d(new Private)
{
    *m_d = *(rhs.m_d);
}


KisImagePipeBrush::~KisImagePipeBrush()
{
    delete m_d;
}

bool KisImagePipeBrush::load()
{
    QFile file(filename());
    file.open(QIODevice::ReadOnly);
    QByteArray data = file.readAll();
    file.close();
    return initFromData(data);
}

bool KisImagePipeBrush::initFromData(const QByteArray &data)
{
    // XXX: this doesn't correctly load the image pipe brushes yet.

    // XXX: This stuff is in utf-8, too.
    // The first line contains the name -- this means we look until we arrive at the first newline
    QByteArray line1;

    qint32 i = 0;

    while (data[i] != '\n' && i < data.size()) {
        line1.append(data[i]);
        i++;
    }
    setName(QString::fromUtf8(line1, line1.size()));

    i++; // Skip past the first newline

    // The second line contains the number of brushes, separated by a space from the parasite

    // XXX: This stuff is in utf-8, too.
    QByteArray line2;
    while (data[i] != '\n' && i < data.size()) {
        line2.append(data[i]);
        i++;
    }

    QString paramline = QString::fromUtf8(line2, line2.size());
    qint32 numOfBrushes = paramline.left(paramline.indexOf(' ')).toUInt();
    QString parasiteString = paramline.mid(paramline.indexOf(' ') + 1);
    KisPipeBrushParasite parasite = KisPipeBrushParasite(parasiteString);
    parasite.sanitize();

    m_d->brushesPipe.setParasite(parasite);
    i++; // Skip past the second newline


    for (int brushIndex = 0;
         brushIndex < numOfBrushes && i < data.size(); brushIndex++) {

        KisGbrBrush* brush = new KisGbrBrush(name() + '_' + QString().setNum(brushIndex),
                                             data,
                                             i);

        m_d->brushesPipe.addBrush(brush);
    }

    if (numOfBrushes > 0) {
        setValid(true);
        setSpacing(m_d->brushesPipe.lastBrush()->spacing());
        setWidth(m_d->brushesPipe.firstBrush()->width());
        setHeight(m_d->brushesPipe.firstBrush()->height());
        setImage(m_d->brushesPipe.firstBrush()->image());
    }

    return true;
}

bool KisImagePipeBrush::save()
{
    QFile file(filename());
    file.open(QIODevice::WriteOnly | QIODevice::Truncate);
    bool ok = saveToDevice(&file);
    file.close();
    return ok;
}

bool KisImagePipeBrush::saveToDevice(QIODevice* dev) const
{
    QByteArray utf8Name = name().toUtf8(); // Names in v2 brushes are in UTF-8
    char const* name = utf8Name.data();
    int len = qstrlen(name);

    if (m_d->brushesPipe.parasite().dim != 1) {
        warnImage << "Save to file for pipe brushes with dim != not yet supported!";
        return false;
    }

    // Save this pipe brush: first the header, and then all individual brushes consecutively
    // XXX: this needs some care for when we have > 1 dimension)

    // Gimp Pipe Brush header format: Name\n<number of brushes> <parasite>\n

    // The name\n
    if (dev->write(name, len) == -1)
        return false;

    if (!dev->putChar('\n'))
        return false;

    // Write the parasite (also writes number of brushes)
    if (!m_d->brushesPipe.parasite().saveToDevice(dev))
        return false;

    if (!dev->putChar('\n'))
        return false;

    // <gbr brushes>
    return m_d->brushesPipe.saveToDevice(dev);
}

void KisImagePipeBrush::generateMaskAndApplyMaskOrCreateDab(KisFixedPaintDeviceSP dst, KisBrush::ColoringInformation* coloringInformation,
                                                            double scaleX, double scaleY, double angle, const KisPaintInformation& info,
                                                            double subPixelX , double subPixelY,
                                                            qreal softnessFactor) const
{
    m_d->brushesPipe.generateMaskAndApplyMaskOrCreateDab(dst, coloringInformation, scaleX, scaleY, angle, info, subPixelX, subPixelY, softnessFactor);
}

KisFixedPaintDeviceSP KisImagePipeBrush::paintDevice(const KoColorSpace * colorSpace, double scale, double angle, const KisPaintInformation& info, double subPixelX, double subPixelY) const
{
    return m_d->brushesPipe.paintDevice(colorSpace, scale, angle, info, subPixelX, subPixelY);
}

enumBrushType KisImagePipeBrush::brushType() const
{
    return !hasColor() || useColorAsMask() ? PIPE_MASK : PIPE_IMAGE;
}

bool KisImagePipeBrush::hasColor() const
{
    return m_d->brushesPipe.hasColor();
}

void KisImagePipeBrush::makeMaskImage()
{
    m_d->brushesPipe.makeMaskImage();
    setUseColorAsMask(false);
}

void KisImagePipeBrush::setUseColorAsMask(bool useColorAsMask)
{
    KisGbrBrush::setUseColorAsMask(useColorAsMask);
    m_d->brushesPipe.setUseColorAsMask(useColorAsMask);
}

const KisBoundary* KisImagePipeBrush::boundary() const
{
    KisGbrBrush *brush = m_d->brushesPipe.firstBrush();
    Q_ASSERT(brush);

    return brush->boundary();
}

bool KisImagePipeBrush::canPaintFor(const KisPaintInformation& info)
{
    if (info.movement().isMuchSmallerThan(1) // FIXME the 1 here is completely arbitrary.
            // What is the correct order of magnitude?
            && m_d->brushesPipe.parasite().needsMovement)
        return false;
    return true;
}

KisImagePipeBrush* KisImagePipeBrush::clone() const
{
    return new KisImagePipeBrush(*this);
}

QString KisImagePipeBrush::defaultFileExtension() const
{
    return QString(".gih");
}

qint32 KisImagePipeBrush::maskWidth(double scale, double angle) const
{
    return m_d->brushesPipe.maskWidth(scale, angle);
}

qint32 KisImagePipeBrush::maskHeight(double scale, double angle) const
{
    return m_d->brushesPipe.maskHeight(scale, angle);
}

void KisImagePipeBrush::setAngle(qreal _angle)
{
    KisGbrBrush::setAngle(_angle);
    m_d->brushesPipe.setAngle(_angle);
}

void KisImagePipeBrush::setScale(qreal _scale)
{
    KisGbrBrush::setScale(_scale);
    m_d->brushesPipe.setScale(_scale);
}

void KisImagePipeBrush::setSpacing(double _spacing)
{
    KisGbrBrush::setSpacing(_spacing);
    m_d->brushesPipe.setSpacing(_spacing);
}

void KisImagePipeBrush::setBrushType(enumBrushType type)
{
    Q_UNUSED(type);
    qFatal("FATAL: protected member setBrushType has no meaning for KisImagePipeBrush");
    // brushType() is a finction of hasColor() and useColorAsMask()
}

void KisImagePipeBrush::setHasColor(bool hasColor)
{
    Q_UNUSED(hasColor);
    qFatal("FATAL: protected member setHasColor has no meaning for KisImagePipeBrush");
    // hasColor() is a function of the underlying brushes
}

KisGbrBrush* KisImagePipeBrush::testingGetCurrentBrush() const
{
    return m_d->brushesPipe.currentBrush();
}

QVector<KisGbrBrush*> KisImagePipeBrush::testingGetBrushes() const
{
    return m_d->brushesPipe.testingGetBrushes();
}

void KisImagePipeBrush::testingSelectNextBrush(const KisPaintInformation& info) const
{
    return m_d->brushesPipe.testingSelectNextBrush(info);
}
