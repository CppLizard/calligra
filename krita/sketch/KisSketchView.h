/* This file is part of the KDE project
 * Copyright (C) 2012 Boudewijn Rempt <boud@kogmbh.com>
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
#ifndef KRITA_SKETCH_VIEW_H
#define KRITA_SKETCH_VIEW_H

#include <QDeclarativeItem>

class KisSketchView : public QDeclarativeItem
{
    Q_OBJECT
    Q_PROPERTY(QObject* selectionManager READ selectionManager NOTIFY viewChanged)
    Q_PROPERTY(QObject* view READ view NOTIFY viewChanged)
    Q_PROPERTY(QString file READ file WRITE setFile NOTIFY fileChanged)
    Q_PROPERTY(bool modified READ isModified NOTIFY modifiedChanged)

    Q_PROPERTY(bool canUndo READ canUndo NOTIFY canUndoChanged);
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY canRedoChanged);

public:
    KisSketchView(QDeclarativeItem* parent = 0);
    virtual ~KisSketchView();

    QObject* selectionManager() const;
    QObject* doc() const;
    QObject* view() const;
    QString file() const;
    bool isModified() const;

    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0);
    virtual void componentComplete();
    virtual void geometryChanged(const QRectF& newGeometry, const QRectF& oldGeometry);

    void setFile(const QString &file);

    void showFloatingMessage(const QString message, const QIcon& icon);

    bool canUndo() const;
    bool canRedo() const;
public Q_SLOTS:
    void undo();
    void redo();

    void save();
    void saveAs(const QString& fileName, const QString& mimeType);

    void documentAboutToBeDeleted();
    void documentChanged();

Q_SIGNALS:
    void viewChanged();
    void fileChanged();
    void modifiedChanged();
    void floatingMessageRequested(QString message, QString iconName);
    void interactionStarted();
    void loadingFinished();
    void canUndoChanged();
    void canRedoChanged();

protected:
    virtual bool sceneEvent(QEvent* event);

private:
    class Private;
    Private * const d;

    Q_PRIVATE_SLOT(d, void update())
    Q_PRIVATE_SLOT(d, void resetDocumentPosition())
};

#endif // KRITA_SKETCH_CANVAS_H
