/* This file is part of the KDE project
 * Copyright (C) 2006 Thomas Zander <zander@kde.org>
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

#ifndef WORDS_H
#define WORDS_H

#define TextShape_SHAPEID "TextShapeID"

#include <KoCanvasResourceManager.h>

class KWFrameSet;
class KWTextFrameSet;

/// The words-global namespace for all Words related things.
namespace Words
{
    /// what should happen when the frame is full (too small for its contents)
    enum FrameBehavior {
        AutoExtendFrameBehavior,    ///< Make the frame bigger to fit the contents
        AutoCreateNewFrameBehavior, ///< Create a new frame on the next page
        IgnoreContentFrameBehavior  ///< Ignore the content and clip it
    };
    /// What happens on new page
    enum NewFrameBehavior {
        ReconnectNewFrame,  ///< On new page reconnect a copied from to the flow
        NoFollowupFrame,    ///< On new page don't create a new page
        CopyNewFrame        ///< On new page create a copy-frame and show that.
    };

    enum FrameSetType {
        BackgroundFrameSet,
        TextFrameSet,
        OtherFrameSet
    };

    /// Each text frame set can be catogorized in one of these items
    enum TextFrameSetType {
        OddPagesHeaderTextFrameSet,  ///< The frameSet that holds the headers for the odd pages
        EvenPagesHeaderTextFrameSet, ///< The frameSet that holds the headers for the even pages
        OddPagesFooterTextFrameSet,  ///< The frameSet that holds the footers for the odd pages
        EvenPagesFooterTextFrameSet, ///< The frameSet that holds the footers for the even pages
        MainTextFrameSet,   ///< The frameset that holds all the frames for the main text area
        OtherTextFrameSet   ///< Any other text frameset not managed by the auto-frame layout
    };

    /// used in KWPageSettings to determine if, and what kind of header/footer to use
    enum HeaderFooterType {
        HFTypeNone,       ///< Don't show the frames
        HFTypeEvenOdd,    ///< Show different content for even and odd pages
        HFTypeUniform    ///< Show the same content for each page
    };

    /// Specifies the horizontal position of the line that separates main text and foot note
    enum FootNoteSeparatorLinePos {
        FootNoteSeparatorLeft, ///< Left of page
        FootNoteSeparatorCenter, ///< Centered on page
        FootNoteSeparatorRight  ///< Right of page
    };

    /// Specifies the Words specific resources that you can put and get from the KoCanvasResourceManager
    enum WordsCanvasResources {
        // lets try to put Current in front of everything for ease of reading.
        CurrentPictureCount = KoCanvasResourceManager::WordsStart,
        CurrentTableCount,
        CurrentPageCount,
        FrameOutlineColor
    };

    /**
     * Returns true if the passed frameset is a known type that the frameLayout auto-creates.
     * @param fs the frameset to check. 0 is allowed.
     */
    bool isAutoGenerated(KWFrameSet *fs);
    /**
     * Returns true if the passed frameset is a header or a footer frameset.
     * @param fs the frameset to check. 0 is allowed.
     */
    bool isHeaderFooter(KWTextFrameSet *fs);
    /**
     * Returns a localized name for the TextFrameSetType.
     * @param type the frameset-type for which a localized name should be returned.
     */
    QString frameSetTypeName(TextFrameSetType type);
    QString frameSetTypeName(KWFrameSet* frameset);
}

#endif
