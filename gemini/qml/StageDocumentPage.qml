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
import "components"
import org.calligra.CalligraComponents 0.1 as Calligra

Item {
    id: base;
    property alias document: stageCanvas.document;
    property alias textEditor: stageCanvas.textEditor;
    property QtObject canvas: stageCanvas;
    property alias source: stageCanvas.source;
    property alias navigateMode: controllerFlickable.enabled;
    onNavigateModeChanged: {
        if(navigateMode === true) {
            // This means we've just changed back from having edited stuff.
            // Consequently we want to deselect all selections. Tell the canvas about that.
            stageCanvas.deselectEverything();
            toolManager.requestToolChange("PageToolFactory_ID");
        }
    }
    Item {
        id: stageNavigator;
        anchors {
            top: parent.top;
            left: parent.left;
            bottom: parent.bottom;
        }
        width: Settings.theme.adjustedPixel(240);
        Rectangle {
            anchors.fill: parent;
            color: "#4e5359";
        }
        Rectangle {
            anchors {
                top: parent.top;
                right: parent.right;
                bottom: parent.bottom;
            }
            width: 1;
            color: "black";
            opacity: 0.1;
        }
        ListView {
            id: navigatorListView;
            anchors {
                fill: parent;
                topMargin: Settings.theme.adjustedPixel(86);
            }
            model: presentationModel;
            delegate: Item {
                height: Settings.theme.adjustedPixel(136);
                width: ListView.view.width;
                Rectangle {
                    anchors.fill: parent;
                    opacity: index === stageCanvas.currentSlide ? 0.6 : 0;
                    Behavior on opacity { PropertyAnimation { duration: Constants.AnimationDuration; } }
                    color: "#00adf5";
                }
                Label {
                    anchors {
                        top: parent.top;
                        left: parent.left;
                        bottom: parent.bottom;
                    }
                    width: Settings.theme.adjustedPixel(40);
                    color: index === stageCanvas.currentSlide ? "white" : "#c1cdd1";
                    text: index + 1;
                    horizontalAlignment: Text.AlignHCenter;
                    verticalAlignment: Text.AlignVCenter;
                }
                Calligra.Thumbnail {
                    anchors {
                        top: parent.top;
                        right: parent.right;
                        bottom: parent.bottom;
                        margins: Settings.theme.adjustedPixel(16);
                    }
                    width: parent.width - Settings.theme.adjustedPixel(56);
                    content: presentationModel.thumbnail(index);
                }
                MouseArea {
                    anchors.fill: parent;
                    onClicked: stageCanvas.currentSlide = index;
                }
            }
        }
    }
    Item {
        anchors {
            top: parent.top;
            left: stageNavigator.right;
            right: parent.right;
            bottom: parent.bottom;
            topMargin: Settings.theme.adjustedPixel(86);
        }
        clip: true;
        Rectangle {
            anchors.fill: parent;
            color: "#4e5359";
        }
        Calligra.PresentationCanvas {
            id: stageCanvas;
            anchors {
                fill: parent;
            }
            onLoadingBegun: baseLoadingDialog.visible = true;
            onLoadingFinished: {
                console.debug("doc and part: " + doc() + " " + part());
                mainWindow.setDocAndPart(doc(), part());
                baseLoadingDialog.hideMe();
            }
            onCurrentSlideChanged: navigatorListView.positionViewAtIndex(currentSlide, ListView.Contain);
        }
        Flickable {
            id: controllerFlickable;
            anchors {
                top: parent.top;
                left: parent.left;
                right: parent.right;
                bottom: enabled ? parent.bottom : parent.top;
            }
            Calligra.CanvasControllerItem {
                canvas: stageCanvas;
                flickable: controllerFlickable;
            }
            MouseArea {
                x: controllerFlickable.contentX;
                y: controllerFlickable.contentY;
                height: controllerFlickable.height;
                width: controllerFlickable.width;
                onClicked: {
                    if(mouse.x < width / 6) {
                        if(stageCanvas.currentSlide === 0) {
                            stageCanvas.currentSlide = stageCanvas.slideCount() - 1;
                        }
                        else {
                            stageCanvas.currentSlide = stageCanvas.currentSlide - 1;
                        }
                    }
                    else if(mouse.x > width * 5 / 6) {
                        var currentSlide = stageCanvas.currentSlide;
                        stageCanvas.currentSlide = stageCanvas.currentSlide + 1;
                        if(currentSlide === stageCanvas.currentSlide) {
                            stageCanvas.currentSlide = 0;
                        }
                    }
                }
                onDoubleClicked: {
                    if(mouse.x < width / 6 || mouse.x > width * 5 / 6) {
                        // don't accept double-clicks in the navigation zone
                        return;
                    }
                    toolManager.requestToolChange("TextToolFactory_ID");
                    base.navigateMode = false;
                }
            }
        }
    }
}
