/* This file is part of the KDE project
 * Copyright (C) 2012 Arjen Hiemstra <ahiemstra@heimr.nl>
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
import org.krita.sketch 1.0
import "../components"

Panel {
    id: base;
    name: "Presets";
    panelColor: "#1e0058";

    /*actions: [
        Button {
            id: addButton;

            width: Constants.GridWidth / 2
            height: Constants.GridHeight;

            color: "transparent";
            image: "../images/svg/icon-add.svg"
            textColor: "white";
            shadow: false;
            highlight: false;
            onClicked: base.toggleEdit();

            states: State {
                name: "edit"
                when: base.state == "edit";

                PropertyChanges { target: addButton; text: "Cancel"; color: Constants.Theme.NegativeColor; width: Constants.GridWidth * 2; }
            }

            transitions: Transition {
                to: "edit";
                reversible: true;

                ParallelAnimation {
                    NumberAnimation { target: addButton; properties: "width"; duration: 250; }
                    ColorAnimation { target: addButton; properties: "color"; duration: 250; }
                }
            }
        },
        Button {
            id: editButton;

            width: Constants.GridWidth / 2
            height: Constants.GridHeight;

            text: ""
            image: "../images/svg/icon-edit.svg"
            color: "transparent";
            textColor: "white";
            shadow: false;
            highlight: false;
            onClicked: base.toggleEdit();

            states: State {
                name: "edit"
                when: base.state == "edit";

                PropertyChanges { target: editButton; text: "Save"; color: Constants.Theme.PositiveColor; width: Constants.GridWidth * 2; }
            }

            transitions: Transition {
                to: "edit";
                reversible: true;

                ParallelAnimation {
                    NumberAnimation { target: editButton; properties: "width"; duration: 250; }
                    ColorAnimation { target: editButton; properties: "color"; duration: 250; }
                }
            }
        }
    ]*/

    PresetModel {
        id: presetsModel;
        view: sketchView.view;
    }
    Connections {
        target: sketchView;
        onLoadingFinished: {
            var index = presetsModel.nameToIndex("Basic_ink_10");
            console.debug("New preset: " + index);
            presetsModel.activatePreset(index);
            peekViewGrid.currentIndex = index;
            fullViewGrid.currentIndex = index;
        }
    }

    peekContents: GridView {
        id: peekViewGrid;
        anchors.fill: parent;
        keyNavigationWraps: false

        model: presetsModel;
        delegate: Button {
            width: Constants.GridWidth;
            height: Constants.GridHeight;

            checked: GridView.isCurrentItem;

            color: (model.index === peekViewGrid.currentIndex) ? "#D2C8E5" : "transparent";
            shadow: false
            textSize: 10;
            image: model.image;

            highlightColor: Constants.Theme.HighlightColor;

            onClicked: {
                presetsModel.activatePreset(index);
                peekViewGrid.currentIndex = index;
                fullViewGrid.currentIndex = index;
            }
        }

        cellWidth: Constants.GridWidth;
        cellHeight: Constants.GridHeight;
    }

    fullContents: PageStack {
        id: contentArea;
        anchors.fill: parent;
        initialPage: GridView {
            id: fullViewGrid;
            anchors.fill: parent;
            model: presetsModel;
            delegate: Item {
                height: Constants.GridHeight;
                width: contentArea.width;
                Rectangle {
                    anchors.fill: parent;
                    color: (model.index === fullViewGrid.currentIndex) ? "#D2C8E5" : "transparent";
                }
                Rectangle {
                    id: presetThumbContainer;
                    anchors {
                        verticalCenter: parent.verticalCenter;
                        left: parent.left;
                    }
                    height: Constants.GridHeight;
                    width: height;
                    color: "transparent";
                    Image {
                        anchors.centerIn: parent;
                        cache: false;
                        source: model.image;
                        smooth: true;
                        width: parent.width * 0.8;
                        height: parent.height * 0.8;
                        fillMode: Image.PreserveAspectFit;
                    }
                }
                Label {
                    anchors {
                        verticalCenter: parent.verticalCenter;
                        left: presetThumbContainer.right;
                        right: parent.right;
                    }
                    text: model.text;
                }
                MouseArea {
                    anchors.fill: parent;
                    onClicked: {
                        presetsModel.activatePreset(index);
                        peekViewGrid.currentIndex = index;
                        fullViewGrid.currentIndex = index;
                    }
                }
            }

            cellWidth: contentArea.width;
            cellHeight: Constants.GridHeight;
        }
    }

    onStateChanged: if( state != "edit" && contentArea.depth > 1 ) {
        contentArea.pop();
    }

    function toggleEdit() {
        if( base.state == "edit" ) {
            base.state = "full";
            contentArea.pop();
        } else if( base.state == "full" ) {
            base.state = "edit";
            contentArea.push( editPresetPage );
        }
    }

    Component { id: editPresetPage; EditPresetPage { } }
}
