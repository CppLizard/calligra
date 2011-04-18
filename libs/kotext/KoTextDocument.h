/* This file is part of the KDE project
 * Copyright (C) 2008 Girish Ramakrishnan <girish@forwardbias.in>
 * Copyright (C) 2009 Thomas Zander <zander@kde.org>
 * Copyright (C) 2008 Pierre Stirnweiss <pierre.stirnweiss_koffice@gadz.org>
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

#ifndef KOTEXTDOCUMENT_H
#define KOTEXTDOCUMENT_H

#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QUrl>

#include "KoList.h"

#include "KoOdfNotesConfiguration.h"

class KoStyleManager;
class KoInlineTextObjectManager;
class KUndoStack;
class KoTextEditor;
class KoOdfLineNumberingConfiguration;
class KoChangeTracker;

/**
 * KoTextDocument provides an easy mechanism to set and access the
 * editing members of a QTextDocument. The meta data are stored as resources
 * in the QTextDocument using QTextDocument::addResource() and fetched
 * using QTextDocument::resource().
 *
 */
class KOTEXT_EXPORT KoTextDocument
{
public:
    /// Constructor
    KoTextDocument(QTextDocument *document);
    /// Constructor
    KoTextDocument(const QTextDocument *document);

    /// Destructor
    ~KoTextDocument();

    /// Returns the document that was passed in the constructor
    QTextDocument *document() const;

    ///Returns the text editor for that document
    KoTextEditor *textEditor();

    ///Sets the text editor for the document
    void setTextEditor(KoTextEditor *textEditor);

    /// Sets the style manager that defines the named styles in the document
    void setStyleManager(KoStyleManager *styleManager);

    /// Returns the style manager
    KoStyleManager *styleManager() const;

    /// Sets the change tracker of the document
    void setChangeTracker(KoChangeTracker *changeTracker);

    ///Returns the change tracker of the document
    KoChangeTracker *changeTracker() const;

    /// set the notes configuration of the document
    void setNotesConfiguration(KoOdfNotesConfiguration *notesConfiguration);

    /// @return the notes configuration
    KoOdfNotesConfiguration *notesConfiguration(KoOdfNotesConfiguration::NoteClass noteClass) const;

    /// set the notes configuration of the document
    void setLineNumberingConfiguration(KoOdfLineNumberingConfiguration *lineNumberingConfiguration);

    /// @return the notes configuration
    KoOdfLineNumberingConfiguration *lineNumberingConfiguration() const;

    ///Sets the global undo stack
    void setUndoStack(KUndoStack *undoStack);

    ///Returns the global undo stack
    KUndoStack *undoStack() const;

    ///Sets the global heading list
    void setHeadingList(KoList *list);

    ///Returns the global heading list
    KoList *headingList() const;

    /// Sets the lists of the document
    void setLists(const QList<KoList *> &lists);

    /// Returns the lists in the document
    QList<KoList *> lists() const;

    /// Adds a list to the document
    void addList(KoList *list);

    /// Removes a list from the document
    void removeList(KoList *list);

    /// Returns the KoList that holds \a block; 0 if block is not part of any list
    KoList *list(const QTextBlock &block) const;

    /// Returns the KoList that holds \a list
    KoList *list(QTextList *textList) const;

    /// Return the KoList that holds \a listId
    KoList *list(KoListStyle::ListIdType listId) const;

    /// Return the selections used during painting.
    QVector<QAbstractTextDocumentLayout::Selection> selections() const;

    /**
     * Set the selections to use for painting.
     *
     * The selections are used to apply temporary styling to
     * parts of a document.
     *
     * \param selections The new selections to use.
     */
    void setSelections(const QVector<QAbstractTextDocumentLayout::Selection> &selections);

    /// Returns the KoInlineTextObjectManager
    KoInlineTextObjectManager *inlineTextObjectManager() const;

    /// Set the KoInlineTextObjectManager
    void setInlineTextObjectManager(KoInlineTextObjectManager *manager);

    /**
     * Enum to describe the text document's automatic resizing behaviour. This is evaluated only
     * for the textshape.
     */
    enum ResizeMethod {
        /// Resize the shape to fit the content. This makes sure that the text shape takes op
        /// only as much space as absolutely necessary to fit the entire text into its boundaries.
        AutoResize,
        /// Specifies whether or not to automatically increase the width of the drawing object
        /// if text is added to fit the entire width of the text into its boundaries.
        /// Compared to AutoResize above this only applied to the width whereas the height is
        /// not resized. Also this only grows but does not shrink again if text is removed again.
        AutoGrowWidth,
        /// Specifies whether or not to automatically increase the height of the drawing object
        /// if text is added to fit the entire height of the text into its boundaries.
        AutoGrowHeight,
        /// This combines the AutoGrowWidth and AutoGrowHeight and automatically increase width
        /// and height to fit the entire text into its boundaries.
        AutoGrowWidthAndHeight,
        /// Shrink the content displayed within the shape to match into the shape's boundaries. This
        /// will scale the content down as needed to display the whole document.
        ShrinkToFitResize,
        /// Deactivates auto-resizing. This is the default resizing method.
        NoResize
    };

    /**
     * Specifies how the document should be resized upon a change in the document.
     *
     * If auto-resizing is turned on, text will not be wrapped unless enforced by e.g. a newline.
     *
     * By default, NoResize is set.
     */
    void setResizeMethod(ResizeMethod method);

    /**
     * Returns the auto-resizing mode. By default, this is NoResize.
     *
     * @see setResizeMethod
     */
    ResizeMethod resizeMethod() const;

    /**
     * Specifies if tabs are relative to paragraph indent.
     *
     * By default it's false.
     */
    void setRelativeTabs(bool relative);

    /**
     * Returns if tabs are placed relative to paragraph indent.
     *
     * By default, this is false.
     *
     * @see setRelativeTabs
     */
    bool relativeTabs() const;

    /**
     * Clears the text in the document. Unlike QTextDocument::clear(), this
     * function does not clear the resources of the QTextDocument.
     */
    void clearText();

    /// Enum (type) used to add resources using QTextDocument::addResource()
    enum ResourceType {
        StyleManager = QTextDocument::UserResource,
        Lists,
        InlineTextManager,
        ChangeTrackerResource,
        UndoStack,
        TextEditor,
        FootNotesConfiguration,
        EndNotesConfiguration,
        LineNumberingConfiguration,
        RelativeTabs,
        HeadingList,
        Selections
    };

    static const QUrl StyleManagerURL;
    static const QUrl ListsURL;
    static const QUrl InlineObjectTextManagerURL;
    static const QUrl ChangeTrackerURL;
    static const QUrl UndoStackURL;
    static const QUrl TextEditorURL;
    static const QUrl FootNotesConfigurationURL;
    static const QUrl EndNotesConfigurationURL;
    static const QUrl LineNumberingConfigurationURL;
    static const QUrl RelativeTabsURL;
    static const QUrl HeadingListURL;
    static const QUrl SelectionsURL;

private:
    QTextDocument *m_document;
};

#endif // KOTEXTDOCUMENT_H
