/*
 * This file is part of the KDE project
 *
 * Copyright (C) 2013 Shantanu Tushar <shantanu@kde.org>
 * Copyright (C) 2013 Sujith Haridasan <sujith.h@gmail.com>
 * Copyright (C) 2013 Arjen Hiemstra <ahiemstra@heimr.nl>
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
 *
 */

#ifndef CQTEXTDOCUMENTCANVAS_H
#define CQTEXTDOCUMENTCANVAS_H

#include "CQCanvasBase.h"

class KWDocument;
class CQTextDocumentModel;
class KoFindText;
class KoDocument;
class KoZoomController;
class KoCanvasController;
class KoCanvasBase;
class KoPart;
class KUrl;
class KoFindMatch;

class CQTextDocumentCanvas : public CQCanvasBase
{
    Q_OBJECT
    Q_PROPERTY(QString searchTerm READ searchTerm WRITE setSearchTerm NOTIFY searchTermChanged)
    Q_PROPERTY(QObject* documentModel READ documentModel NOTIFY documentModelChanged)
    Q_PROPERTY(QSize documentSize READ documentSize NOTIFY documentSizeChanged)
    Q_PROPERTY(int currentPageNumber READ currentPageNumber WRITE setCurrentPageNumber NOTIFY currentPageNumberChanged)
    Q_PROPERTY(QObjectList linkTargets READ linkTargets NOTIFY linkTargetsChanged)

public:
    CQTextDocumentCanvas(QDeclarativeItem* parent = 0);
    ~CQTextDocumentCanvas();

    int currentPageNumber() const;
    void setCurrentPageNumber(const int &currentPageNumber);
    int cameraY() const;
    void setCameraY (int cameraY);

    QString searchTerm() const;
    void setSearchTerm(const QString &term);

    QObject *documentModel() const;
    QSize documentSize() const;

    virtual void render(QPainter* painter, const QRectF& target);

    KWDocument* document() const;
    Q_INVOKABLE QObject* doc() const;
    Q_INVOKABLE QObject* part() const;

    QObjectList linkTargets() const;

    Q_INVOKABLE qreal pagePosition( int page );

signals:
    void searchTermChanged();
    void documentModelChanged();
    void documentSizeChanged();
    void currentPageNumberChanged();
    void cameraYChanged();
    void linkTargetsChanged();

protected:
    virtual bool event( QEvent* event );
    virtual void geometryChanged(const QRectF& newGeometry, const QRectF& oldGeometry);
    virtual void openFile(const QString& uri);

private slots:
    void findNoMatchFound();
    void findMatchFound(const KoFindMatch& match);
    void updateCanvas();
    void findPrevious();
    void findNext();
    void updateDocumentSize(const QSize &size);

private:
    void createAndSetCanvasControllerOn(KoCanvasBase *canvas);
    void createAndSetZoomController(KoCanvasBase *canvas);
    void updateZoomControllerAccordingToDocument(const KoDocument *document);
    void alignTopWith(int y);
    void gotoPage(int pageNumber, KoDocument *document);

    class Private;
    Private * const d;
};
#endif // CQTEXTDOCUMENTCANVAS_H
