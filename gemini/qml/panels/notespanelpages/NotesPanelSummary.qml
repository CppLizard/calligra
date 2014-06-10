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
    Item {
        id: summaryView;
        anchors {
            top: parent.top;
            left: parent.left;
            right: parent.right;
            bottom: finalNote.top;
        }
        Rectangle {
            id: summaryTitleBar;
            anchors {
                top: parent.top;
                left: parent.left;
                right: parent.right;
            }
            height: Constants.GridHeight;
            color: "#e8e9ea";
            Button {
                anchors {
                    left: parent.left;
                    leftMargin: Constants.DefaultMargin;
                    verticalCenter: parent.verticalCenter;
                }
                height: parent.height - Constants.DefaultMargin * 2;
                width: height;
                image: Settings.theme.icon("SVG-Icon-MenuBack-1");
                onClicked: notesPageStack.pop();
            }
            Label {
                anchors {
                    left: parent.left;
                    right: parent.right;
                    verticalCenter: parent.verticalCenter;
                }
                text: "NOTE SUMMARY";
                color: "#5b6573";
                font.pixelSize: Constants.SmallFontSize
                font.bold: true;
                horizontalAlignment: Text.AlignHCenter;
            }
        }
        Item {
            id: notesSummary;
            anchors {
                top: summaryTitleBar.bottom;
                left: parent.left;
                right: parent.right;
                bottom: parent.bottom;
            }
            Rectangle {
                anchors.fill: parent;
                color: "white";
                border.color: "#e8e9ea";
                border.width: 1;
                opacity: 0.96;
            }
            ListView {
                id: notesSummaryList;
                anchors.fill: parent;
                model: (base.canvas !== null) ? base.canvas.notes : null;
                clip: true;
                delegate: Item {
                    height: {
                        if(model.expanded) {
                            return model.firstOfThisColor ? Constants.GridHeight * 2: Constants.GridHeight;
                        }
                        else {
                            return model.firstOfThisColor ? Constants.GridHeight : 0;
                        }
                    }
                    width: notesSummaryList.width;
                    Behavior on height { PropertyAnimation { duration: Constants.AnimationDuration; } }
                    Image {
                        id: colorImg;
                        anchors {
                            left: parent.left;
                            top: parent.top;
                            margins: Constants.DefaultMargin;
                        }
                        height: model.firstOfThisColor ? Constants.GridHeight - Constants.DefaultMargin : 0;
                        width: height;
                        opacity: height > 0 ? 1 : 0;
                        Behavior on opacity { PropertyAnimation { duration: Constants.AnimationDuration; } }
                        source: Settings.theme.icon("SVG-FillLabel-%1-1".arg(model.color));
                        sourceSize.width: width > height ? height : width;
                        sourceSize.height: width > height ? height : width;
                        Label {
                            anchors.centerIn: parent;
                            width: parent.width;
                            horizontalAlignment: Text.AlignHCenter;
                            text: model.colorCount;
                            color: "white";
                            font.bold: true;
                        }
                        Label {
                            anchors {
                                left: parent.right;
                                leftMargin: Constants.DefaultMargin;
                                verticalCenter: parent.verticalCenter;
                            }
                            color: "#00adf5";
                            opacity: 0.6;
                            text: model.categoryName;
                        }
                    }
                    MouseArea {
                        anchors {
                            top: parent.top;
                            left: parent.left;
                            right: parent.right;
                            verticalCenter: colorImg.verticalCenter;
                        }
                        height: colorImg.height;
                        onClicked: base.canvas.notes.toggleExpanded(index);
                    }
                    Label {
                        anchors {
                            left: parent.left;
                            right: parent.right;
                            bottom: parent.bottom;
                            margins: Constants.DefaultMargin;
                        }
                        height: font.pixelSize + Constants.DefaultMargin * 2;
                        text: model.text;
                        color: "#5b6573";
                        opacity: model.expanded ? 1 : 0;
                        Behavior on opacity { PropertyAnimation { duration: Constants.AnimationDuration; } }
                    }
                    Image {
                        anchors {
                            left: parent.left;
                            bottom: parent.bottom;
                            margins: Constants.DefaultMargin;
                        }
                        height: Constants.GridHeight - Constants.DefaultMargin;
                        width: height;
                        opacity: model.expanded ? 1 : 0;
                        Behavior on opacity { PropertyAnimation { duration: Constants.AnimationDuration; } }
                        source: model.image;
                        Label {
                            anchors {
                                left: parent.right;
                                margins: Constants.DefaultMargin;
                                verticalCenter: parent.verticalCenter;
                            }
                            visible: parent.source != "";
                            color: "#5b6573";
                            text: "Stamp note";
                        }
                    }
                }
            }
        }
    }
    Item {
        id: finalNote;
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
                    text: "Add Final Note";
                    opacity: 0.6;
                }
            }
        }
    }
}
