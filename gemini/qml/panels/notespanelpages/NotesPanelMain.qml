/* This file is part of the KDE project
 * Copyright (C) 2014 Dan Leinir Turthra Jensen <admin@leinir.dk>
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

import QtQuick 1.1
import "../../components"
import org.calligra 1.0

Item {
    id: base;
    property QtObject canvas: null;
    Rectangle {
        id: titleBar;
        anchors {
            top: parent.top;
            left: parent.left;
            right: parent.right;
        }
        height: Constants.GridHeight;
        color: "#e8e9ea";
        Label {
            anchors {
                left: parent.left;
                leftMargin: Constants.DefaultMargin;
                verticalCenter: parent.verticalCenter;
            }
            text: "ADD NOTES";
            color: "#5b6573";
            font.pixelSize: Constants.SmallFontSize
            font.bold: true;
        }
        Row {
            anchors {
                right: parent.right;
                rightMargin: Constants.DefaultMargin;
                verticalCenter: parent.verticalCenter;
            }
            height: parent.height - Constants.DefaultMargin * 2;
            Button {
                height: parent.height;
                width: height;
                image: Settings.theme.icon("SVG-Icon-AddVoiceComment-1");
            }
            Button {
                height: parent.height;
                width: height;
                image: Settings.theme.icon("SVG-Icon-AddPhotoComment-1");
            }
            Button {
                height: parent.height;
                width: height;
                image: Settings.theme.icon("SVG-Icon-NotesSummary-1");
                onClicked: notesPageStack.push(summaryView);
                Image {
                    anchors.fill: parent;
                    source: Settings.theme.icon("SVG-NotesSummary-Counter-1");
                    sourceSize.width: width > height ? height : width;
                    sourceSize.height: width > height ? height : width;
                    Text {
                        anchors {
                            right: parent.right;
                            top: parent.top;
                        }
                        height: parent.height * 0.57291667;
                        width: parent.width * 0.45833333;
                        color: "white";
                        horizontalAlignment: Text.AlignHCenter;
                        verticalAlignment: Text.AlignVCenter;
                        font: Settings.theme.font("small");
                        text: base.canvas.notes.count;
                    }
                }
            }
        }
    }
    Item {
        id: stickerTemplates;
        anchors {
            top: titleBar.bottom;
            left: parent.left;
            right: parent.right;
        }
        height: Constants.GridHeight * 2;
        Flow {
            anchors.fill: parent;
            spacing: Constants.DefaultMargin;
            Repeater {
                model: ListModel {
                    ListElement { image: "Sticker-ThumbsUp.svg"; }
                    ListElement { image: "Sticker-Feather.svg"; }
                    ListElement { image: "Sticker-Apple.svg"; }
                    ListElement { image: "Sticker-ArrowTarget.svg"; }
                    ListElement { image: "Sticker-Lightbulb.svg"; }
                    ListElement { image: "Sticker-Ribbon.svg"; }
                    ListElement { image: "Sticker-OK.svg"; }
                    ListElement { image: "Sticker-A.svg"; }
                    ListElement { image: "Sticker-B.svg"; }
                    ListElement { image: "Sticker-C.svg"; }
                    ListElement { image: "Sticker-D.svg"; }
                    ListElement { image: "Sticker-F.svg"; }
                }
                Image {
                    width: Constants.GridHeight - Constants.DefaultMargin;
                    height: width;
                    source: Settings.theme.image(model.image);
                    sourceSize.width: width > height ? height : width;
                    sourceSize.height: width > height ? height : width;
                    MouseArea {
                        anchors.fill: parent;
                        onClicked: {
                            base.canvas.addSticker(Settings.theme.image(model.image));
                            toolManager.requestToolChange("InteractionTool");
                            viewLoader.item.navigateMode = false;
                        }
                    }
                }
            }
        }
    }
    Flickable {
        id: noteTemplatesFlickable;
        anchors {
            top: stickerTemplates.bottom;
            left: parent.left;
            right: parent.right;
            bottom: customNote.top;
        }
        clip: true;
        contentHeight: noteTemplatesColumn.height;
        Column {
            id: noteTemplatesColumn;
            width: base.width;
            height: childrenRect.height;
            Repeater {
                model: ListModel {
                    ListElement { text: "Check Spelling"; color: "Red"; }
                    ListElement { text: "Needs more support"; color: "Red"; }
                    ListElement { text: "Go deeper, perhaps"; color: "Yellow"; }
                    ListElement { text: "Great point!"; color: "Green"; }
                    ListElement { text: "Good use of vocabulary!"; color: "Green"; }
                    ListElement { text: "Nice!"; color: "Green"; }
                    ListElement { text: "Well done!"; color: "Green"; }
                    ListElement { text: "Splendid!"; color: "Green"; }
                    ListElement { text: "Smashing!"; color: "Green";  }
                }
                Item {
                    height: Constants.GridHeight;
                    width: base.width;
                    MouseArea {
                        anchors.fill: parent;
                        onClicked: {
                            base.canvas.addNote(model.text, model.color);
                            toolManager.requestToolChange("InteractionTool");
                            viewLoader.item.navigateMode = false;
                        }
                    }
                    Rectangle {
                        width: base.width;
                        height: 1;
                        color: "#e8e9ea";
                        opacity: 0.7;
                    }
                    Row {
                        anchors.verticalCenter: parent.verticalCenter;
                        height: Constants.GridHeight / 2;
                        width: base.width;
                        Item { height: parent.height; width: Constants.DefaultMargin; }
                        Image {
                            height: parent.height;
                            width: height * 0.26470588;
                            source: Settings.theme.icon("SVG-Comment-DragHandle-1");
                            sourceSize.width: width > height ? height : width;
                            sourceSize.height: width > height ? height : width;
                        }
                        Item { height: parent.height; width: Constants.DefaultMargin * 2; }
                        Label {
                            anchors.verticalCenter: parent.verticalCenter;
                            width: parent.width - parent.height * 1.3 - Constants.DefaultMargin * 3;
                            text: model.text;
                            color: "#5b6573";
                        }
                        Image {
                            height: parent.height;
                            width: height;
                            source: Settings.theme.icon("SVG-Label-%1-1".arg(model.color));
                            sourceSize.width: width > height ? height : width;
                            sourceSize.height: width > height ? height : width;
                        }
                    }
                }
            }
        }
    }
    ScrollDecorator { flickableItem: noteTemplatesFlickable; anchors.fill: noteTemplatesFlickable; }
    Item {
        id: customNote;
        anchors {
            left: parent.left;
            right: parent.right;
            bottom: parent.bottom;
        }
        height: Constants.GridHeight;
        Rectangle {
            anchors.fill: parent;
            color: "#e8e9ea";
            opacity: 0.7;
        }
        Rectangle {
            anchors {
                fill: parent;
                margins: Constants.DefaultMargin;
            }
            color: "white";
            MouseArea {
                anchors.fill: parent;
                onClicked: notesPageStack.push(customNoteView);
            }
            Row {
                anchors.centerIn: parent;
                height: parent.height / 3;
                spacing: Constants.DefaultMargin;
                Image {
                    height: parent.height;
                    width: height;
                    source: Settings.theme.icon("SVG-AddComment-Inline-1");
                    sourceSize.width: width > height ? height : width;
                    sourceSize.height: width > height ? height : width;
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter;
                    width: paintedWidth;
                    font.pixelSize: parent.height;
                    color: "#00adf5";
                    text: "Add Custom Note";
                    opacity: 0.6;
                }
            }
        }
    }
}
