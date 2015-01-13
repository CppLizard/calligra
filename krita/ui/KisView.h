/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
   Copyright (C) 2007 Thomas Zander <zander@kde.org>
   Copyright (C) 2010 Benjamin Port <port.benjamin@gmail.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
#ifndef __koView_h__
#define __koView_h__

#include <QWidget>

#include <KoColorSpace.h>
#include <KoColorProfile.h>

#include <kis_types.h>
#include "krita_export.h"

class KisPart;
class KisDocument;
class KisMainWindow;
class KisPrintJob;
class KisCanvasController;
class KisZoomManager;
class KisCanvas2;
class KisViewManager;
class KisDocument;
class KisCanvasResourceProvider;
class KisCoordinatesConverter;

class KoZoomController;
class KoZoomManager;
class KoZoomController;
struct KoPageLayout;

// KDE classes
class KStatusBar;
class KAction;
class KActionCollection;

// Qt classes
class QToolBar;
class QDragEnterEvent;
class QDropEvent;
class QPrintDialog;
class QCloseEvent;


/**
 * This class is used to display a @ref KisDocument.
 *
 * Multiple views can be attached to one document at a time.
 */
class KRITAUI_EXPORT KisView : public QWidget
{
    Q_OBJECT

public:
    /**
     * Creates a new view for the document.
     */
    KisView(KisPart *part, KisDocument *document, KActionCollection *actionCollection, QWidget *parent = 0);
virtual ~KisView();

    KAction *undoAction() const;
    KAction *redoAction() const;

    // Temporary while teasing apart view and mainwindow
    void setViewManager(KisViewManager *view);
    KisViewManager *viewManager() const;

public:

    /**
     *  Retrieves the document object of this view.
     */
    KisDocument *document() const;

    /**
     * Reset the view to show the given document.
     */
    virtual void setDocument(KisDocument *document);

    /**
     * Tells this view that its document has got deleted (called internally)
     */
    void setDocumentDeleted();

    /**
     * In order to print the document represented by this view a new print job should
     * be constructed that is capable of doing the printing.
     * The default implementation returns 0, which silently cancels printing.
     */
    virtual KisPrintJob * createPrintJob();

    /**
     * In order to export the document represented by this view a new print job should
     * be constructed that is capable of doing the printing.
     * The default implementation call createPrintJob.
     */
    virtual KisPrintJob * createPdfPrintJob();

    /**
     * @return the page layout to be used for printing.
     * Default is the documents layout.
     * Reimplement if your application needs to use a different layout.
     */
    virtual KoPageLayout pageLayout() const;

    /**
     * Create a QPrintDialog based on the @p printJob
     */
    virtual QPrintDialog *createPrintDialog(KisPrintJob *printJob, QWidget *parent);

    /**
     * @return the KisMainWindow in which this view is currently.
     */
    KisMainWindow * mainWindow() const;

    /**
     * @return the statusbar of the KisMainWindow in which this view is currently.
     */
    KStatusBar * statusBar() const;

    /**
     * This adds a widget to the statusbar for this view.
     * If you use this method instead of using statusBar() directly,
     * KisView will take care of removing the items when the view GUI is deactivated
     * and readding them when it is reactivated.
     * The parameters are the same as QStatusBar::addWidget().
     *
     * Note that you can't use KStatusBar methods (inserting text items by id).
     * But you can create a KStatusBarLabel with a dummy id instead, and use
     * it directly, to get the same look and feel.
     */
    void addStatusBarItem(QWidget * widget, int stretch = 0, bool permanent = false);

    /**
     * Remove a widget from the statusbar for this view.
     */
    void removeStatusBarItem(QWidget * widget);

    /**
     * Return the zoomController for this view.
     */
    virtual KoZoomController *zoomController() const;

    /// create a list of actions that when activated will change the unit on the document.
    QList<QAction*> createChangeUnitActions(bool addPixelUnit = false);

public:

    /**
     * The zoommanager handles everything action-related to zooming
     */
    KisZoomManager *zoomManager() const;

    /**
     * The CanvasController decorates the canvas with scrollbars
     * and knows where to start painting on the canvas widget, i.e.,
     * the document offset.
     */
    KisCanvasController *canvasController() const;
    KisCanvasResourceProvider *resourceProvider() const;

    /**
     * @return the canvas object
     */
    KisCanvas2 *canvasBase() const;

    /// @return the image this view is displaying
    KisImageWSP image() const;


    KisCoordinatesConverter *viewConverter() const;

    void resetImageSizeAndScroll(bool changeCentering,
                                 const QPointF oldImageStillPoint = QPointF(),
                                 const QPointF newImageStillPoint = QPointF());

    void setCurrentNode(KisNodeSP node);
    KisNodeSP currentNode() const;
    KisLayerSP currentLayer() const;
    KisMaskSP currentMask() const;

    /// Convenience method to get at the active selection (the
    /// selection of the current layer, or, if that does not exist,
    /// the global selection.
    KisSelectionSP selection();

public slots:

    /**
     * Display a message in the status bar (calls QStatusBar::message())
     * @todo rename to something more generic
     */
    void slotActionStatusText(const QString &text);

    /**
     * End of the message in the status bar (calls QStatusBar::clear())
     * @todo rename to something more generic
     */
    void slotClearStatusText();


signals:
    // From KisImage
    void sigSizeChanged(const QPointF &oldStillPoint, const QPointF &newStillPoint);
    void sigProfileChanged(const KoColorProfile *  profile);
    void sigColorSpaceChanged(const KoColorSpace*  cs);

protected:

    // QWidget overrides
    virtual void dragEnterEvent(QDragEnterEvent * event);
    virtual void dropEvent(QDropEvent * event);
    virtual bool event( QEvent* event );
    virtual void closeEvent(QCloseEvent *event);

    /**
     * Generate a name for this view.
     */
    QString newObjectName();

public slots:
    void slotLoadingFinished();
    void slotSavingFinished();
    void slotImageResolutionChanged();
    void slotImageSizeChanged(const QPointF &oldStillPoint, const QPointF &newStillPoint);

private:

    bool queryClose();

    class Private;
    Private * const d;
};

#endif
