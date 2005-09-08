/*
 *  Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
 *  Copyright (c) 2004 Boudewijn Rempt <boud@valdyas.org>
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
#include <qrect.h>
#include <qwmatrix.h>
#include <qimage.h>

#include <qapplication.h>
#include <kcommand.h>
#include <klocale.h>
#include <kdebug.h>

#include <koStore.h>

#include "kis_global.h"
#include "kis_types.h"
#include "kis_painter.h"
#include "kis_fill_painter.h"
#include "kis_undo_adapter.h"
#include "tiles/kis_iterator.h"
#include "kis_iterators_pixel.h"
#include "kis_iteratorpixeltrait.h"
#include "kis_scale_visitor.h"
#include "kis_rotate_visitor.h"
#include "kis_transform_visitor.h"
#include "kis_profile.h"
#include "kis_canvas_controller.h"
#include "kis_color.h"
#include "kis_integer_maths.h"
#include "kis_colorspace_registry.h"

#include "kis_paint_device.h"
#include "kis_paint_device_iface.h"
#include "kis_paint_device_impl.h"

namespace {

    class KisPaintDeviceImplCommand : public KNamedCommand {
        typedef KNamedCommand super;

    public:
        KisPaintDeviceImplCommand(const QString& name, KisPaintDeviceImplSP paintDevice);
        virtual ~KisPaintDeviceImplCommand() {}

        virtual void execute() = 0;
        virtual void unexecute() = 0;

    protected:
        void setUndo(bool undo);
        void notifyPropertyChanged();

        KisPaintDeviceImplSP m_paintDevice;
    };

    KisPaintDeviceImplCommand::KisPaintDeviceImplCommand(const QString& name, KisPaintDeviceImplSP paintDevice) :
        super(name), m_paintDevice(paintDevice)
    {
    }

    void KisPaintDeviceImplCommand::setUndo(bool undo)
    {
        if (m_paintDevice -> undoAdapter()) {
            m_paintDevice -> undoAdapter() -> setUndo(undo);
        }
    }

    void KisPaintDeviceImplCommand::notifyPropertyChanged()
    {
        if (m_paintDevice -> image()) {
            m_paintDevice -> image() -> notifyLayersChanged();
        }
    }

    class KisPaintDeviceImplVisibilityCommand : public KisPaintDeviceImplCommand {
        typedef KisPaintDeviceImplCommand super;

    public:
        KisPaintDeviceImplVisibilityCommand(KisPaintDeviceImplSP paintDevice, bool oldVisibility, bool newVisibility);

        virtual void execute();
        virtual void unexecute();

    private:
        bool m_oldVisibility;
        bool m_newVisibility;
    };

    KisPaintDeviceImplVisibilityCommand::KisPaintDeviceImplVisibilityCommand(KisPaintDeviceImplSP paintDevice, bool oldVisibility, bool newVisibility) :
        super(i18n("Layer Visibility"), paintDevice)
    {
        m_oldVisibility = oldVisibility;
        m_newVisibility = newVisibility;
    }

    void KisPaintDeviceImplVisibilityCommand::execute()
    {
        setUndo(false);
        m_paintDevice -> setVisible(m_newVisibility);
        notifyPropertyChanged();
        setUndo(true);
    }

    void KisPaintDeviceImplVisibilityCommand::unexecute()
    {
        setUndo(false);
        m_paintDevice -> setVisible(m_oldVisibility);
        notifyPropertyChanged();
        setUndo(true);
    }

    class KisPaintDeviceImplCompositeOpCommand : public KisPaintDeviceImplCommand {
        typedef KisPaintDeviceImplCommand super;

    public:
        KisPaintDeviceImplCompositeOpCommand(KisPaintDeviceImplSP paintDevice, const KisCompositeOp& oldCompositeOp, const KisCompositeOp& newCompositeOp);

        virtual void execute();
        virtual void unexecute();

    private:
        KisCompositeOp m_oldCompositeOp;
        KisCompositeOp m_newCompositeOp;
    };

    KisPaintDeviceImplCompositeOpCommand::KisPaintDeviceImplCompositeOpCommand(KisPaintDeviceImplSP paintDevice, const KisCompositeOp& oldCompositeOp,
                                       const KisCompositeOp& newCompositeOp) :
        super(i18n("Layer Composite Mode"), paintDevice)
    {
        m_oldCompositeOp = oldCompositeOp;
        m_newCompositeOp = newCompositeOp;
    }

    void KisPaintDeviceImplCompositeOpCommand::execute()
    {
        setUndo(false);
        m_paintDevice -> setCompositeOp(m_newCompositeOp);
        notifyPropertyChanged();
        setUndo(true);
    }

    void KisPaintDeviceImplCompositeOpCommand::unexecute()
    {
        setUndo(false);
        m_paintDevice -> setCompositeOp(m_oldCompositeOp);
        notifyPropertyChanged();
        setUndo(true);
    }

    class MoveCommand : public KNamedCommand {
        typedef KNamedCommand super;

    public:
        MoveCommand(KisPaintDeviceImplSP device, const QPoint& oldpos, const QPoint& newpos);
        virtual ~MoveCommand();

        virtual void execute();
        virtual void unexecute();

    private:
        void moveTo(const QPoint& pos);
        void undoOff();
        void undoOn();

    private:
        KisPaintDeviceImplSP m_device;
        QPoint m_oldPos;
        QPoint m_newPos;
    };

    MoveCommand::MoveCommand(KisPaintDeviceImplSP device, const QPoint& oldpos, const QPoint& newpos) :
        super(i18n("Moved Layer"))
    {
        m_device = device;
        m_oldPos = oldpos;
        m_newPos = newpos;
    }

    MoveCommand::~MoveCommand()
    {
    }

    void MoveCommand::undoOff()
    {
        if (m_device -> undoAdapter()) {
            m_device -> undoAdapter() -> setUndo(false);
        }
    }

    void MoveCommand::undoOn()
    {
        if (m_device -> undoAdapter()) {
            m_device -> undoAdapter() -> setUndo(true);
        }
    }

    void MoveCommand::execute()
    {
        undoOff();
        moveTo(m_newPos);
        undoOn();
    }

    void MoveCommand::unexecute()
    {
        undoOff();
        moveTo(m_oldPos);
        undoOn();
    }

    void MoveCommand::moveTo(const QPoint& pos)
    {
        m_device -> move(pos.x(), pos.y());

        if (m_device -> image()) {
            m_device -> image() -> notify();
        }
    }

    class KisConvertLayerTypeCmd : public KNamedCommand {
        typedef KNamedCommand super;

    public:
        KisConvertLayerTypeCmd(KisUndoAdapter *adapter, KisPaintDeviceImplSP paintDevice,
                       KisDataManagerSP beforeData, KisAbstractColorSpace * beforeColorSpace, KisProfileSP beforeProfile,
                       KisDataManagerSP afterData, KisAbstractColorSpace * afterColorSpace, KisProfileSP afterProfile
                       ) : super(i18n("Convert Layer Type"))
            {
                m_adapter = adapter;
                m_paintDevice = paintDevice;
                m_beforeData = beforeData;
                m_beforeColorSpace = beforeColorSpace;
                m_beforeProfile = beforeProfile;
                m_afterData = afterData;
                m_afterColorSpace = afterColorSpace;
                m_afterProfile = afterProfile;
            }

        virtual ~KisConvertLayerTypeCmd()
            {
            }

    public:
        virtual void execute()
            {
                m_adapter -> setUndo(false);

                m_paintDevice -> setData(m_afterData, m_afterColorSpace, m_afterProfile);

                m_adapter -> setUndo(true);
                if (m_paintDevice -> image()) {
                    m_paintDevice -> image() -> notify();
                    m_paintDevice -> image() -> notifyLayersChanged();
                }
            }

        virtual void unexecute()
            {
                m_adapter -> setUndo(false);

                m_paintDevice -> setData(m_beforeData, m_beforeColorSpace, m_beforeProfile);

                m_adapter -> setUndo(true);
                if (m_paintDevice -> image()) {
                    m_paintDevice -> image() -> notify();
                    m_paintDevice -> image() -> notifyLayersChanged();
                }
            }

    private:
        KisUndoAdapter *m_adapter;

        KisPaintDeviceImplSP m_paintDevice;

        KisDataManagerSP m_beforeData;
        KisAbstractColorSpace * m_beforeColorSpace;
        KisProfileSP m_beforeProfile;

        KisDataManagerSP m_afterData;
        KisAbstractColorSpace * m_afterColorSpace;
        KisProfileSP m_afterProfile;
    };

}

KisPaintDeviceImpl::KisPaintDeviceImpl(KisAbstractColorSpace * colorSpace, const QString& name) :
    KShared()
{
    if (colorSpace == 0) {
        kdDebug() << "Cannot create paint device without colorstrategy!\n";
        return;
    }

    m_dcop = 0;
    Q_ASSERT(colorSpace != 0);
    Q_ASSERT(name.isEmpty() == false);
    m_x = 0;
    m_y = 0;

    m_pixelSize = colorSpace -> pixelSize();
    m_nChannels = colorSpace -> nChannels();

    Q_UINT8* defPixel = new Q_UINT8 [ m_pixelSize ];
    colorSpace -> fromQColor(Qt::black, OPACITY_TRANSPARENT, defPixel);

    m_datamanager = new KisDataManager(m_pixelSize, defPixel);
        delete [] defPixel;

    Q_CHECK_PTR(m_datamanager);
    m_extentIsValid = true;

    m_visible = true;
    m_owner = 0;
    m_name = name;

    m_compositeOp = COMPOSITE_OVER;

    m_colorSpace = colorSpace;

    m_hasSelection = false;
    m_selection = 0;
    m_profile = 0;
}

KisPaintDeviceImpl::KisPaintDeviceImpl(KisImage *img, KisAbstractColorSpace * colorSpace, const QString& name) :
    KShared()
{
    m_dcop = 0;
    Q_ASSERT(name.isEmpty() == false);

        m_x = 0;
        m_y = 0;

    m_visible = true;

    m_name = name;
    m_compositeOp = COMPOSITE_OVER;
    m_hasSelection = false;
    m_selection = 0;
    m_profile = 0;

    m_owner = img;

    if (img != 0 && colorSpace == 0) {
        m_colorSpace = img -> colorSpace();
    }
    else {
        m_colorSpace = colorSpace;
    }

    if (img != 0 && m_colorSpace == img -> colorSpace()) {
            m_profile = m_owner -> profile();
    }

    m_pixelSize = m_colorSpace -> pixelSize();
    m_nChannels = m_colorSpace -> nChannels();

    Q_UINT8* defPixel = new Q_UINT8[ m_pixelSize ];
    colorSpace -> fromQColor(Qt::black, OPACITY_TRANSPARENT, defPixel);

    m_datamanager = new KisDataManager(m_pixelSize, defPixel);
        delete [] defPixel;
    Q_CHECK_PTR(m_datamanager);
    m_extentIsValid = true;
}

KisPaintDeviceImpl::KisPaintDeviceImpl(const KisPaintDeviceImpl& rhs) : QObject(), KShared(rhs), KisPaintDevice()
{
        if (this != &rhs) {
                m_owner = 0;
        m_dcop = rhs.m_dcop;
                if (rhs.m_datamanager) {
            m_datamanager = new KisDataManager(*rhs.m_datamanager);
            Q_CHECK_PTR(m_datamanager);
        }
        m_extentIsValid = rhs.m_extentIsValid;
                m_visible = rhs.m_visible;
                m_x = rhs.m_x;
                m_y = rhs.m_y;
                m_name = rhs.m_name;
                m_compositeOp = rhs.m_compositeOp;
        m_colorSpace = rhs.m_colorSpace;
        m_hasSelection = false;
        m_selection = 0;
        m_profile = rhs.m_profile;
        m_pixelSize = rhs.m_pixelSize;
        m_nChannels = rhs.m_nChannels;
        }
}

KisPaintDeviceImpl::~KisPaintDeviceImpl()
{
    delete m_dcop;
}

DCOPObject *KisPaintDeviceImpl::dcopObject()
{
    if (!m_dcop) {
        m_dcop = new KisPaintDeviceImplIface(this);
        Q_CHECK_PTR(m_dcop);
    }
    return m_dcop;
}


void KisPaintDeviceImpl::move(Q_INT32 x, Q_INT32 y)
{
        m_x = x;
        m_y = y;
    if(m_selection)
    {
        m_selection->setX(x);
        m_selection->setY(y);
    }

        emit positionChanged(this);
}

void KisPaintDeviceImpl::move(const QPoint& pt)
{
        move(pt.x(), pt.y());
}

KNamedCommand * KisPaintDeviceImpl::moveCommand(Q_INT32 x, Q_INT32 y)
{
    KNamedCommand * cmd = new MoveCommand(this, QPoint(m_x, m_y), QPoint(x, y));
    Q_CHECK_PTR(cmd);
    cmd -> execute();
    return cmd;
}


QString KisPaintDeviceImpl::name() const
{
        return m_name;
}

void KisPaintDeviceImpl::setName(const QString& name)
{
        if (!name.isEmpty())
                m_name = name;
}


void KisPaintDeviceImpl::extent(Q_INT32 &x, Q_INT32 &y, Q_INT32 &w, Q_INT32 &h) const
{
    m_datamanager -> extent(x, y, w, h);
    x += m_x;
    y += m_y;
}

QRect KisPaintDeviceImpl::extent() const
{
    Q_INT32 x, y, w, h;
    extent(x, y, w, h);
    return QRect(x, y, w, h);
}

bool KisPaintDeviceImpl::extentIsValid() const
{
    return m_extentIsValid;
}

void KisPaintDeviceImpl::setExtentIsValid(bool isValid)
{
    m_extentIsValid = isValid;
}

void KisPaintDeviceImpl::exactBounds(Q_INT32 &x, Q_INT32 &y, Q_INT32 &w, Q_INT32 &h)
{
    QRect r = exactBounds();
    x = r.x();
    y = r.y();
    w = r.width();
    h = r.height();
}

QRect KisPaintDeviceImpl::exactBounds()
{
    Q_INT32 x, y, w, h, boundX, boundY, boundW, boundH;
    extent(x, y, w, h);

    extent(boundX, boundY, boundW, boundH);
    Q_UINT8 * emptyPixel = new Q_UINT8[m_pixelSize];
    Q_CHECK_PTR(emptyPixel);

    memset(emptyPixel, 0, m_pixelSize);

    bool found = false;

    for (Q_INT32 y2 = y; y2 < y + h ; ++y2) {
        KisHLineIterator it = createHLineIterator(x, y2, w, false);
        while (!it.isDone() && found == false) {
            if (memcmp(it.rawData(), emptyPixel, m_pixelSize) != 0) {
                boundY = y2;
                found = true;
                break;
            }
            ++it;
        }
        if (found) break;
    }

    found = false;

    for (Q_INT32 y2 = y + h; y2 > y ; --y2) {
        KisHLineIterator it = createHLineIterator(x, y2, w, false);
        while (!it.isDone() && found == false) {
            if (memcmp(it.rawData(), emptyPixel, m_pixelSize) != 0) {
                boundH = y2 - boundY + 1;
                found = true;
                break;
            }
            ++it;
        }
        if (found) break;
    }
    found = false;

    for (Q_INT32 x2 = x; x2 < x + w ; ++x2) {
        KisVLineIterator it = createVLineIterator(x2, y, h, false);
        while (!it.isDone() && found == false) {
            if (memcmp(it.rawData(), emptyPixel, m_pixelSize) != 0) {
                boundX = x2;
                found = true;
                break;
            }
            ++it;
        }
        if (found) break;
    }

    found = false;

    // Loog for right edge )
    for (Q_INT32 x2 = x + w; x2 > x ; --x2) {
        KisVLineIterator it = createVLineIterator(x2, y, h, false);
        while (!it.isDone() && found == false) {
            if (memcmp(it.rawData(), emptyPixel, m_pixelSize) != 0) {
                boundW = x2 - boundX + 1;
                found = true;
                break;
            }
            ++it;
        }
        if (found) break;
    }

    delete [] emptyPixel;
    return QRect(boundX, boundY, boundW, boundH);
}

void KisPaintDeviceImpl::accept(KisScaleVisitor& visitor)
{
        visitor.visitKisPaintDeviceImpl(this);
}

void KisPaintDeviceImpl::accept(KisRotateVisitor& visitor)
{
        visitor.visitKisPaintDeviceImpl(this);
}

void KisPaintDeviceImpl::scale(double xscale, double yscale, KisProgressDisplayInterface * progress, KisFilterStrategy *filterStrategy)
{
        KisScaleVisitor visitor;
        accept(visitor);
        visitor.scale(xscale, yscale, progress, filterStrategy);
}

void KisPaintDeviceImpl::rotate(double angle, bool rotateAboutImageCentre, KisProgressDisplayInterface * progress)
{
        KisRotateVisitor visitor;
        accept(visitor);
        visitor.rotate(angle, rotateAboutImageCentre, progress);
}

void KisPaintDeviceImpl::shear(double angleX, double angleY, KisProgressDisplayInterface * progress)
{
        KisRotateVisitor visitor;
        accept(visitor);
        visitor.shear(angleX, angleY, progress);
}

void KisPaintDeviceImpl::mirrorX()
{
    QRect r;
    if (hasSelection()) {
        r = selection() -> exactBounds();
    }
    else {
        r = exactBounds();
    }

    for (Q_INT32 y = r.top(); y <= r.bottom(); ++y) {
        KisHLineIteratorPixel srcIt = createHLineIterator(r.x(), y, r.width(), false);
        KisHLineIteratorPixel dstIt = createHLineIterator(r.x(), y, r.width(), true);

        dstIt += r.width() - 1;

        while (!srcIt.isDone()) {
            if (srcIt.isSelected()) {
                memcpy(dstIt.rawData(), srcIt.oldRawData(), m_pixelSize);
            }
            ++srcIt;
            --dstIt;

        }
        qApp -> processEvents();
    }
}

void KisPaintDeviceImpl::mirrorY()
{
    /* Read a line from bottom to top and and from top to bottom and write their values to each other */
    QRect r;
    if (hasSelection()) {
        r = selection() -> exactBounds();
    }
    else {
        r = exactBounds();
    }


    Q_INT32 y1, y2;
    for (y1 = r.top(), y2 = r.bottom(); y1 <= r.bottom(); ++y1, --y2) {
        KisHLineIteratorPixel itTop = createHLineIterator(r.x(), y1, r.width(), true);
        KisHLineIteratorPixel itBottom = createHLineIterator(r.x(), y2, r.width(), false);
        while (!itTop.isDone() && !itBottom.isDone()) {
            if (itBottom.isSelected()) {
                memcpy(itTop.rawData(), itBottom.oldRawData(), m_pixelSize);
            }
            ++itBottom;
            ++itTop;
        }
        qApp -> processEvents();
    }
}

bool KisPaintDeviceImpl::write(KoStore *store)
{
    bool retval = m_datamanager->write(store);
    emit ioProgress(100);

        return retval;
}

bool KisPaintDeviceImpl::read(KoStore *store)
{
    bool retval = m_datamanager->read(store);
    emit ioProgress(100);

        return retval;
}

void KisPaintDeviceImpl::convertTo(KisAbstractColorSpace * dstColorSpace, KisProfileSP dstProfile, Q_INT32 renderingIntent)
{
    if (profile() == 0) setProfile(m_owner -> profile());

    if ( (colorSpace() -> id() == dstColorSpace -> id())
         && profile()
         && dstProfile
         && (profile() -> profile() == dstProfile -> profile()) )
    {
        return;
    }

    KisPaintDeviceImpl dst(dstColorSpace, name());
    dst.setProfile(dstProfile);
    dst.setX(getX());
    dst.setY(getY());

    Q_INT32 x, y, w, h;
    extent(x, y, w, h);

    for (Q_INT32 row = y; row < y + h; ++row) {

#if 0

        KisHLineIterator srcIt = createHLineIterator( x, row, w, false );
        KisHLineIterator dstIt = dst.createHLineIterator( x, row, w, true );
        while ( !srcIt.isDone() ) {
            m_colorSpace->convertPixelsTo( srcIt.rawData(), m_profile, dstIt.rawData(), dstColorSpace, dstProfile, 1, renderingIntent );
            ++srcIt;
            ++dstIt;
        }

#else
        Q_INT32 column = x;
        Q_INT32 columnsRemaining = w;

        while (columnsRemaining > 0) {

            Q_INT32 numContiguousDstColumns = dst.numContiguousColumns(column, row, row);
            Q_INT32 numContiguousSrcColumns = numContiguousColumns(column, row, row);

            Q_INT32 columns = QMIN(numContiguousDstColumns, numContiguousSrcColumns);
            columns = QMIN(columns, columnsRemaining);

            const Q_UINT8 *srcData = pixel(column, row);
            Q_UINT8 *dstData = dst.writablePixel(column, row);

            m_colorSpace -> convertPixelsTo(srcData, m_profile, dstData, dstColorSpace, dstProfile, columns, renderingIntent);

            column += columns;
            columnsRemaining -= columns;
        }
#endif

    }

    if (undoAdapter() && undoAdapter() -> undo()) {
        undoAdapter() -> addCommand(new KisConvertLayerTypeCmd(undoAdapter(), this, m_datamanager, m_colorSpace, m_profile,
                                       dst.m_datamanager, dstColorSpace, dstProfile));
    }

    setData(dst.m_datamanager, dstColorSpace, dstProfile);

}

void KisPaintDeviceImpl::setData(KisDataManagerSP data, KisAbstractColorSpace * colorSpace, KisProfileSP profile)
{
    m_datamanager = data;
    m_colorSpace = colorSpace;
    m_pixelSize = m_colorSpace -> pixelSize();
    m_nChannels = m_colorSpace -> nChannels();
    setProfile(profile);
}

KisUndoAdapter *KisPaintDeviceImpl::undoAdapter() const
{
    if (m_owner) {
        return m_owner -> undoAdapter();
    }
    return 0;
}

void KisPaintDeviceImpl::convertFromImage(const QImage& img)
{
    // XXX: Apply import profile
    if (colorSpace() == KisColorSpaceRegistry::instance()->get("RGBA")) {
        writeBytes(img.bits(), 0, 0, img.width(), img.height());
    }
    else {
        Q_UINT8 * dstData = new Q_UINT8[img.width() * img.height() * pixelSize()];
        KisColorSpaceRegistry::instance()->get("RGBA")->convertPixelsTo(img.bits(), 0, dstData, colorSpace(), 0, img.width() * img.height());
        writeBytes(dstData, 0, 0, img.width(), img.height());
    }
}


void KisPaintDeviceImpl::setProfile(KisProfileSP profile)
{

    if (profile && profile -> colorSpaceSignature() == colorSpace() -> colorSpaceSignature() && profile -> valid()) {

        m_profile = profile;
    }
    else {
        m_profile = 0;
    }
    emit(profileChanged(m_profile));
}

QImage KisPaintDeviceImpl::convertToQImage(KisProfileSP dstProfile, float exposure)
{
    Q_INT32 x1;
    Q_INT32 y1;
    Q_INT32 w;
    Q_INT32 h;

    x1 = - getX();
    y1 = - getY();

    if (image()) {
        w = image()->width();
        h = image()->height();
    }
    else {
        extent(x1, y1, w, h);
    }

    return convertToQImage(dstProfile, x1, y1, w, h, exposure);
}

QImage KisPaintDeviceImpl::convertToQImage(KisProfileSP dstProfile, Q_INT32 x1, Q_INT32 y1, Q_INT32 w, Q_INT32 h, float exposure)
{
    if (w < 0)
        w = 0;

    if (h < 0)
        h = 0;

    Q_UINT8 * data = new Q_UINT8 [w * h * m_pixelSize];
    Q_CHECK_PTR(data);

    m_datamanager -> readBytes(data, x1, y1, w, h);
    QImage image = colorSpace() -> convertToQImage(data, w, h, m_profile, dstProfile, INTENT_PERCEPTUAL, exposure);
    delete[] data;

    return image;
}

KisRectIteratorPixel KisPaintDeviceImpl::createRectIterator(Q_INT32 left, Q_INT32 top, Q_INT32 w, Q_INT32 h, bool writable)
{
    if(hasSelection())
        return KisRectIteratorPixel(this, m_datamanager, m_selection->m_datamanager, left, top, w, h, m_x, m_y, writable);
    else
        return KisRectIteratorPixel(this, m_datamanager, NULL, left, top, w, h, m_x, m_y, writable);
}

KisHLineIteratorPixel  KisPaintDeviceImpl::createHLineIterator(Q_INT32 x, Q_INT32 y, Q_INT32 w, bool writable)
{
    if(hasSelection())
        return KisHLineIteratorPixel(this, m_datamanager, m_selection->m_datamanager, x, y, w, m_x, m_y, writable);
    else
        return KisHLineIteratorPixel(this, m_datamanager, NULL, x, y, w, m_x, m_y, writable);
}

KisVLineIteratorPixel  KisPaintDeviceImpl::createVLineIterator(Q_INT32 x, Q_INT32 y, Q_INT32 h, bool writable)
{
    if(hasSelection())
        return KisVLineIteratorPixel(this, m_datamanager, m_selection->m_datamanager, x, y, h, m_x, m_y, writable);
    else
        return KisVLineIteratorPixel(this, m_datamanager, NULL, x, y, h, m_x, m_y, writable);

}


void KisPaintDeviceImpl::emitSelectionChanged() {
    if(m_owner)
        m_owner -> slotSelectionChanged();
}

void KisPaintDeviceImpl::emitSelectionChanged(const QRect& r) {
    if(m_owner)
        m_owner -> slotSelectionChanged(r);
}


KisSelectionSP KisPaintDeviceImpl::selection(){
    if (!m_selection) {
        m_selection = new KisSelection(this, "layer selection for: " + name());
        Q_CHECK_PTR(m_selection);
        m_selection -> setVisible(true);
        m_selection -> setX(m_x);
        m_selection -> setY(m_y);
    }

    m_hasSelection = true;

    return m_selection;
}


bool KisPaintDeviceImpl::hasSelection()
{
    return m_hasSelection;
}


void KisPaintDeviceImpl::deselect()
{
    m_hasSelection = false;
}

void KisPaintDeviceImpl::addSelection(KisSelectionSP selection) {
    KisPainter painter(this -> selection().data());
    Q_INT32 x, y, w, h;
    selection -> extent(x, y, w, h);
    painter.bitBlt(x, y, COMPOSITE_OVER, selection.data(), x, y, w, h);
    painter.end();
}

void KisPaintDeviceImpl::subtractSelection(KisSelectionSP selection) {
    Q_INT32 x, y, w, h;
    KisPainter painter(this -> selection().data());
    selection -> invert();
    selection -> extent(x, y, w, h);
    painter.bitBlt(x, y, COMPOSITE_ERASE, selection.data(), x, y, w, h);
    selection -> invert();
    painter.end();
}

void KisPaintDeviceImpl::clearSelection()
{
    if (!hasSelection()) return;

    QRect r = m_selection -> selectedRect();
    r = r.normalize();

    for (Q_INT32 y = 0; y < r.height(); y++) {

        KisHLineIterator devIt = createHLineIterator(r.x(), r.y() + y, r.width(), true);
        KisHLineIterator selectionIt = m_selection -> createHLineIterator(r.x(), r.y() + y, r.width(), false);

        while (!devIt.isDone()) {
            // XXX: Optimize by using stretches

            m_colorSpace->applyInverseAlphaU8Mask( devIt.rawData(), selectionIt.rawData(), 1);

            ++devIt;
            ++selectionIt;
        }
    }
}

void KisPaintDeviceImpl::applySelectionMask(KisSelectionSP mask)
{
    QRect r = mask -> extent();
    crop(r);

    for (Q_INT32 y = r.top(); y <= r.bottom(); ++y) {

        KisHLineIterator pixelIt = createHLineIterator(r.x(), y, r.width(), true);
        KisHLineIterator maskIt = mask -> createHLineIterator(r.x(), y, r.width(), false);

        while (!pixelIt.isDone()) {
            // XXX: Optimize by using stretches

            m_colorSpace->applyAlphaU8Mask( pixelIt.rawData(), maskIt.rawData(), 1);

            ++pixelIt;
            ++maskIt;
        }
    }
}

bool KisPaintDeviceImpl::pixel(Q_INT32 x, Q_INT32 y, QColor *c, Q_UINT8 *opacity)
{
    KisHLineIteratorPixel iter = createHLineIterator(x, y, 1, false);

    Q_UINT8 *pix = iter.rawData();

    if (!pix) return false;

    colorSpace() -> toQColor(pix, c, opacity, m_profile);

    return true;
}


bool KisPaintDeviceImpl::pixel(Q_INT32 x, Q_INT32 y, KisColor * kc)
{
    KisHLineIteratorPixel iter = createHLineIterator(x, y, 1, false);

    Q_UINT8 *pix = iter.rawData();

    if (!pix) return false;

    kc->setColor(pix, m_colorSpace, m_profile);

    return true;
}

KisColor KisPaintDeviceImpl::colorAt(Q_INT32 x, Q_INT32 y)
{
    return KisColor(m_datamanager -> pixel(x - m_x, y - m_y), m_colorSpace, m_profile);
}

bool KisPaintDeviceImpl::setPixel(Q_INT32 x, Q_INT32 y, const QColor& c, Q_UINT8  opacity)
{
    KisHLineIteratorPixel iter = createHLineIterator(x, y, 1, true);

    colorSpace() -> fromQColor(c, opacity, iter.rawData(), m_profile);

    return true;
}

bool KisPaintDeviceImpl::setPixel(Q_INT32 x, Q_INT32 y, const KisColor& kc)
{
    Q_UINT8 * pix;
    if (kc.colorSpace() != m_colorSpace) {
        KisColor kc2 (kc, m_colorSpace, m_profile);
        pix = kc2.data();
    }
    else {
        pix = kc.data();
    }

    KisHLineIteratorPixel iter = createHLineIterator(x, y, 1, true);
    memcpy(iter.rawData(), pix, m_colorSpace->pixelSize());

    return true;
}


Q_INT32 KisPaintDeviceImpl::numContiguousColumns(Q_INT32 x, Q_INT32 minY, Q_INT32 maxY)
{
    return m_datamanager -> numContiguousColumns(x - m_x, minY - m_y, maxY - m_y);
}

Q_INT32 KisPaintDeviceImpl::numContiguousRows(Q_INT32 y, Q_INT32 minX, Q_INT32 maxX)
{
    return m_datamanager -> numContiguousRows(y - m_y, minX - m_x, maxX - m_x);
}

Q_INT32 KisPaintDeviceImpl::rowStride(Q_INT32 x, Q_INT32 y)
{
    return m_datamanager -> rowStride(x - m_x, y - m_y);
}

const Q_UINT8* KisPaintDeviceImpl::pixel(Q_INT32 x, Q_INT32 y)
{
    return m_datamanager -> pixel(x - m_x, y - m_y);
}

Q_UINT8* KisPaintDeviceImpl::writablePixel(Q_INT32 x, Q_INT32 y)
{
    return m_datamanager -> writablePixel(x - m_x, y - m_y);
}

void KisPaintDeviceImpl::setX(Q_INT32 x)
{
    m_x = x;
    if(m_selection)
        m_selection->setX(x);
}

void KisPaintDeviceImpl::setY(Q_INT32 y)
{
    m_y = y;
    if(m_selection)
        m_selection->setY(y);
}

KNamedCommand *KisPaintDeviceImpl::setVisibleCommand(bool newVisibility)
{
    return new KisPaintDeviceImplVisibilityCommand(this, visible(), newVisibility);
}

KNamedCommand *KisPaintDeviceImpl::setCompositeOpCommand(const KisCompositeOp& newCompositeOp)
{
    return new KisPaintDeviceImplCompositeOpCommand(this, compositeOp(), newCompositeOp);
}

#include "kis_paint_device_impl.moc"
