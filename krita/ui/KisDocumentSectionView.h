/*
  Copyright (c) 2006 Gábor Lehel <illissius@gmail.com>

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

#ifndef KIS_DOCUMENT_SECTION_VIEW_H
#define KIS_DOCUMENT_SECTION_VIEW_H

#include <QTreeView>
#include "kritaui_export.h"

class QStyleOptionViewItem;
class KisDocumentSectionModel;

/**
 * A View widget on document sections (for example, layers, pages,
 * sheets...). The widget can show the document sections as big
 * thumbnails, in a listview with two rows of informative text and
 * icons per document section or as single rows of text and property
 * icons.
 *
 * The KisDocumentSectionView is designed as a Qt4 model-view widget.
 * See the relevant (extensive) Qt documentation about the design
 * basis for this widget.
 *
 * Usage: simply use this widget in your designer .ui file. Krita's
 * and karbon's layerboxes are KisDocumentSectionView based.
 */
class KRITAUI_EXPORT KisDocumentSectionView: public QTreeView
{
    Q_OBJECT
Q_SIGNALS:
    /**
     * Emitted whenever the user clicks with the secondary mouse
     * button on an item. It is up to the application to design the
     * contents of the context menu and show it.
     */
    void contextMenuRequested(const QPoint &globalPos, const QModelIndex &index);
    void selectionChanged(const QModelIndexList &);
public:

    /**
     * Create a new KisDocumentSectionView.
     */
    explicit KisDocumentSectionView(QWidget *parent = 0);
    virtual ~KisDocumentSectionView();

    /// how items should be displayed
    enum DisplayMode {
        /// large fit-to-width thumbnails, with only titles or page numbers
        ThumbnailMode,

        /// smaller thumbnails, with titles and property icons in two rows
        DetailedMode,

        /// no thumbnails, with titles and property icons in a single row
        MinimalMode
    };

    virtual void paintEvent (QPaintEvent *event);

    virtual void dropEvent(QDropEvent *ev);

    virtual void dragEnterEvent(QDragEnterEvent *e);

    virtual void dragMoveEvent(QDragMoveEvent *ev);

    virtual void dragLeaveEvent(QDragLeaveEvent *e);

    /**
     * Set the display mode of the view to one of the options.
     *
     * @param mode The KisDocumentSectionView::DisplayMode mode
     */
    void setDisplayMode(DisplayMode mode);

    /**
     * @return the currently active display mode
     */
    DisplayMode displayMode() const;

    /**
     * Add toggle actions for all the properties associated with the
     * current document section associated with the model index to the
     * specified menu.
     *
     * For instance, if a document section can be locked and visible,
     * the menu will be expanded with locked and visilbe toggle
     * actions.
     *
     * For instance
     @code
     KisDocumentSectionView * sectionView;
     QModelIndex index = getCurrentDocumentSection();
     QMenu menu;
     if (index.isValid()) {
         sectionView->addPropertyActions(&menu, index);
     } else {
         menu.addAction(...); // Something to create a new document section, for example.
     }

     @endcode
     *
     * @param menu A pointer to the menu that will be expanded with
     * the toglge actions
     * @param index The model index associated with the document
     * section that may or may not provide a number of toggle actions.
     */
    void addPropertyActions(QMenu *menu, const QModelIndex &index);

    void updateNode(const QModelIndex &index);

protected:
    virtual bool viewportEvent(QEvent *event);
    virtual void contextMenuEvent(QContextMenuEvent *event);
    virtual void showContextMenu(const QPoint &globalPos, const QModelIndex &index);
    virtual void startDrag (Qt::DropActions supportedActions);
    QPixmap createDragPixmap() const;

    /**
     * Calculates the index of the nearest item to the cursor position
     */
    int cursorPageIndex() const;

protected Q_SLOTS:
    virtual void currentChanged(const QModelIndex &current, const QModelIndex &previous);
    virtual void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    virtual void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

private Q_SLOTS:
    void slotActionToggled(bool on, const QPersistentModelIndex &index, int property);

private:

    /**
     * Permit to know if a slide is dragging
     *
     * @return boolean
     */
    bool isDragging() const;

    /**
     * Setter for the dragging flag
     *
     * @param flag boolean
     */
    void setDraggingFlag(bool flag = true);

    bool m_draggingFlag;

    QStyleOptionViewItem optionForIndex(const QModelIndex &index) const;
    typedef KisDocumentSectionModel Model;
    class PropertyAction;
    class Private;
    Private* const d;
};

#endif
