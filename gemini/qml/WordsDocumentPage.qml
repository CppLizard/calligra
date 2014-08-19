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
    signal canvasInteractionStarted();
    property alias document: wordsCanvas.document;
    property alias textEditor: wordsCanvas.textEditor;
    property QtObject canvas: wordsCanvas;
    property alias source: wordsCanvas.source;
    property alias navigateMode: controllerFlickable.enabled;
    onNavigateModeChanged: {
        if(navigateMode === true) {
            // This means we've just changed back from having edited stuff.
            // Consequently we want to deselect all selections. Tell the canvas about that.
            wordsCanvas.deselectEverything();
            toolManager.requestToolChange("PageToolFactory_ID");
        }
    }
    Connections {
        target: Constants;
        onGridSizeChanged: {
            var newState = (Constants.IsLandscape ? "" : "readermode");
            if(base.state !== newState) {
                base.state = newState;
            }
        }
    }
    onWidthChanged: {
        if(base.state === "readermode") {
            d.zoomToFit();
        }
    }
    onHeightChanged: {
        if(base.state === "readermode") {
            d.zoomToFit();
        }
    }
    function scrollToEnd() {
        controllerFlickable.contentY = controllerFlickable.contentHeight - controllerFlickable.height;
    }
    Rectangle {
        anchors.fill: parent;
        color: "#e8e9ea";
    }

    Flickable {
        id: bgScrollArea;
        anchors.fill: parent;
        contentHeight: controllerItem.documentSize.height;
        Item {
            width: parent.width;
            height: controllerItem.documentSize.height;
            MouseArea {
                anchors.fill: parent;
                property int oldX: 0
                property int oldY: 0
                property int swipeDistance: Settings.theme.adjustedPixel(10);
                onPressed: {
                    oldX = mouseX;
                    oldY = mouseY;
                }
                onReleased: {
                    base.canvasInteractionStarted();
                    controllerItem.pageChanging = true;
                    var xDiff = oldX - mouseX;
                    var yDiff = oldY - mouseY;
                    // Don't react if the swipe distance is too small
                    if(Math.abs(xDiff) < swipeDistance && Math.abs(yDiff) < swipeDistance) {
                        if(Math.abs(xDiff) > Settings.theme.adjustedPixel(2) || Math.abs(yDiff) > Settings.theme.adjustedPixel(2)) {
                            // If the swipe distance is sort of big (2 pixels on a 1080p screen)
                            // we can assume the finger has moved some distance and should be ignored
                            // as not-a-tap.
                            controllerItem.pageChanging = false;
                            return;
                        }
                        // This might be done in onClick, but that then eats the events, which is not useful
                        // for reader mode we'll accept clicks here, to simulate an e-reader style navigation mode
                        if(base.state === "readermode") {
                            // for reader mode we'll accept clicks here, to simulate an e-reader style navigation mode
                            if(mouse.x < width / 2) {
                                controllerFlickable.contentY = wordsCanvas.pagePosition(wordsCanvas.currentPageNumber - 1) + 1;
                            }
                            else if(mouse.x > width / 2) {
                                controllerFlickable.contentY = wordsCanvas.pagePosition(wordsCanvas.currentPageNumber + 1) + 1;
                            }
                        }
                        else {
                            if(mouse.x < width / 2) {
                                controllerFlickable.contentY = Math.max(0, controllerFlickable.contentY - controllerFlickable.height + (Constants.GridHeight * 1.5));
                            }
                            else if(mouse.x > width / 2) {
                                controllerFlickable.contentY = Math.min(controllerFlickable.contentHeight - controllerFlickable.height, controllerFlickable.contentY + controllerFlickable.height - (Constants.GridHeight * 1.5));
                            }
                        }
                    }
                    else if(base.state === "readermode") {
                        if ( Math.abs(xDiff) > Math.abs(yDiff) ) {
                            if( oldX > mouseX) {
                                // left
                                controllerFlickable.contentY = wordsCanvas.pagePosition(wordsCanvas.currentPageNumber + 1) + 1;
                            } else {
                                // right
                                controllerFlickable.contentY = wordsCanvas.pagePosition(wordsCanvas.currentPageNumber - 1) + 1;
                            }
                        } else {
                            if( oldY > mouseY) {/*up*/ }
                            else {/*down*/ }
                        }
                    }
                    controllerItem.pageChanging = false;
                }
            }
        }
    }
    Binding {
        target: controllerFlickable;
        property: "contentY";
        value: bgScrollArea.contentY;
        when: bgScrollArea.verticalVelocity !== 0;
    }
    Binding {
        target: bgScrollArea;
        property: "contentY";
        value: controllerFlickable.contentY;
        when: controllerFlickable.verticalVelocity !== 0;
    }
    Item {
        id: wordsContentItem;
        anchors {
            top: parent.top;
            bottom: parent.bottom;
            horizontalCenter: parent.horizontalCenter;
        }
        
        width: Math.min(controllerItem.documentSize.width, base.width);

        Calligra.TextDocumentCanvas {
            id: wordsCanvas;
            anchors.fill: parent;

            onLoadingBegun: baseLoadingDialog.visible = true;
            onLoadingFinished: {
                console.debug("doc and part: " + doc() + " " + part());
                mainWindow.setDocAndPart(doc(), part());
                baseLoadingDialog.hideMe();
                thumbnailSize = Qt.size(Settings.theme.adjustedPixel(280), Settings.theme.adjustedPixel(360));
            }
            onCurrentPageNumberChanged: navigatorListView.positionViewAtIndex(currentPageNumber - 1, ListView.Contain);
        }

        Flickable {
            id: controllerFlickable;
            anchors {
                top: parent.top;
                topMargin: Settings.theme.adjustedPixel(86);
                left: parent.left;
                right: parent.right;
                bottom: enabled ? parent.bottom : parent.top;
                bottomMargin: enabled ? 0 : -Settings.theme.adjustedPixel(86);
            }
            interactive: base.state !== "readermode";
            property int fastVelocity: Settings.theme.adjustedPixel(1000);
            onVerticalVelocityChanged: {
                if(Math.abs(verticalVelocity) > fastVelocity && !controllerItem.pageChanging) {
                    d.showThings();
                }
                else {
                    d.hideThings();
                }
            }

            boundsBehavior: controllerItem.documentSize.width < base.width ? Flickable.StopAtBounds : Flickable.DragAndOvershootBounds;

            Image {
                height: Settings.theme.adjustedPixel(40);
                width: Settings.theme.adjustedPixel(40);
                source: Settings.theme.icon("intel-Words-Handle-cursor");
                opacity: wordsCanvas.hasSelection ? 1 : 0;
                Behavior on opacity { PropertyAnimation { duration: Constants.AnimationDuration; } }
                x: wordsCanvas.selectionStartPos.x - width / 2;
                y: wordsCanvas.selectionStartPos.y - (height - 4);
                Rectangle {
                    anchors {
                        top: parent.bottom;
                        horizontalCenter: parent.horizontalCenter;
                    }
                    height: wordsCanvas.selectionStartPos.height - 4;
                    width: 4;
                    color: "#009bcd";
                }
            }
            Image {
                height: Settings.theme.adjustedPixel(40);
                width: Settings.theme.adjustedPixel(40);
                source: Settings.theme.icon("intel-Words-Handle-cursor");
                opacity: wordsCanvas.hasSelection ? 1 : 0;
                Behavior on opacity { PropertyAnimation { duration: Constants.AnimationDuration; } }
                x: wordsCanvas.selectionEndPos.x - width / 2;
                y: wordsCanvas.selectionEndPos.y + (wordsCanvas.selectionEndPos.height - 4);
                Rectangle {
                    anchors {
                        bottom: parent.top;
                        horizontalCenter: parent.horizontalCenter;
                    }
                    height: wordsCanvas.selectionEndPos.height - 4;
                    width: 4;
                    color: "#009bcd";
                }
            }

            PinchArea {
                x: controllerFlickable.contentX;
                y: controllerFlickable.contentY;
                height: controllerFlickable.height;
                width: controllerFlickable.width;

                onPinchStarted: {
                    controllerItem.beginZoomGesture();
                    base.canvasInteractionStarted();
                }
                onPinchUpdated: {
                    var newCenter = mapToItem( controllerFlickable, pinch.center.x, pinch.center.y );
                    controllerItem.zoomBy(pinch.scale - pinch.previousScale, Qt.point( newCenter.x, newCenter.y ) );
                }
                onPinchFinished: { controllerItem.endZoomGesture(); controllerFlickable.returnToBounds(); controllerItem.returnToBounds(); }

                MouseArea {
                    anchors.fill: parent;
                    onDoubleClicked: {
                        if(base.state === "readermode") {
                            // for reader mode, we don't accept editing input
                            base.canvasInteractionStarted();
                            return;
                        }
                        toolManager.requestToolChange("TextToolFactory_ID");
                        base.navigateMode = false;
                        base.canvasInteractionStarted();
                    }

                    property int oldX: 0
                    property int oldY: 0
                    property int swipeDistance: Settings.theme.adjustedPixel(10);
                    onPressed: {
                        oldX = mouseX;
                        oldY = mouseY;
                    }
                    onReleased: {
                        base.canvasInteractionStarted();
                        if(base.state !== "readermode") {
                            return;
                        }
                        var xDiff = oldX - mouseX;
                        var yDiff = oldY - mouseY;
                        controllerItem.pageChanging = true;
                        // Don't react if the swipe distance is too small
                        if(Math.abs(xDiff) < swipeDistance && Math.abs(yDiff) < swipeDistance) {
                            if(Math.abs(xDiff) > Settings.theme.adjustedPixel(2) || Math.abs(yDiff) > Settings.theme.adjustedPixel(2)) {
                                // If the swipe distance is sort of big (2 pixels on a 1080p screen)
                                // we can assume the finger has moved some distance and should be ignored
                                // as not-a-tap.
                                controllerItem.pageChanging = false;
                                return;
                            }
                            // This might be done in onClick, but that then eats the events, which is not useful
                            // for reader mode we'll accept clicks here, to simulate an e-reader style navigation mode
                            if(mouse.x < width / 4) {
                                controllerFlickable.contentY = wordsCanvas.pagePosition(wordsCanvas.currentPageNumber - 1) + 1;
                            }
                            else if(mouse.x > width * 3 / 4) {
                                controllerFlickable.contentY = wordsCanvas.pagePosition(wordsCanvas.currentPageNumber + 1) + 1;
                            }
                        }
                        else if( Math.abs(xDiff) > Math.abs(yDiff) ) {
                            if( oldX > mouseX) {
                                // left
                                controllerFlickable.contentY = wordsCanvas.pagePosition(wordsCanvas.currentPageNumber + 1) + 1;
                            } else {
                                // right
                                controllerFlickable.contentY = wordsCanvas.pagePosition(wordsCanvas.currentPageNumber - 1) + 1;
                            }
                        } else {
                            if( oldY > mouseY) {/*up*/ }
                            else {/*down*/ }
                        }
                        controllerItem.pageChanging = false;
                    }
                }
            }

            Calligra.CanvasControllerItem {
                id: controllerItem;
                canvas: wordsCanvas;
                flickable: controllerFlickable;
                property bool pageChanging: false;
                zoom: 1.0;
                minimumZoom: 0.5;
            }
        }
    }

    QtObject {
        id: d;
        property double previouszoom: 0;
        function restoreZoom() {
            controllerItem.zoom = previouszoom;
            previouszoom = 0;
        }
        function zoomToFit() {
            if(previouszoom === 0) {
                previouszoom = controllerItem.zoom;
            }
            controllerItem.zoomToPage();
        }
        function showThings() {
            if(base.state === "readermode") {
                return;
            }
            base.state = "sidebarShown";
            pageNumber.opacity = 1;
            hideTimer.stop();
            hidePageNumTimer.stop();
        }
        function hideThings() {
            if(navigatorSidebar.containsMouse) {
                return;
            }
            hideTimer.start();
            hidePageNumTimer.start();
        }
    }
    Timer {
        id: hidePageNumTimer;
        running: false;
        repeat: false;
        interval: 500;
        onTriggered: {
            pageNumber.opacity = 0;
        }
    }
    Timer {
        id: hideTimer;
        running: false;
        repeat: false;
        interval: 2000;
        onTriggered: {
            if(base.state === "readermode") {
                return;
            }
            base.state = "";
        }
    }
    states: [
        State {
            name: "sidebarShown"
            AnchorChanges { target: navigatorSidebar; anchors.left: parent.left; anchors.right: undefined; }
        },
        State {
            name: "readermode"
            PropertyChanges { target: navigatorSidebar; opacity: 0; }
        }
        ]
    transitions: [ Transition {
            AnchorAnimation { duration: Constants.AnimationDuration; }
        },
        Transition {
            to: "readermode";
            ScriptAction { script: d.zoomToFit(); }
        },
        Transition {
            from: "readermode";
            ScriptAction { script: d.restoreZoom(); }
        }
        ]
    Item {
        id: navigatorSidebar;
        property alias containsMouse: listViewMouseArea.containsMouse;
        anchors {
            top: parent.top;
            right: parent.left;
            bottom: parent.bottom;
            topMargin: Settings.theme.adjustedPixel(40) + Constants.ToolbarHeight;
            bottomMargin: Settings.theme.adjustedPixel(40);
        }
        width: Settings.theme.adjustedPixel(190);
        BorderImage {
            anchors {
                fill: parent;
                topMargin: -28;
                leftMargin: -36;
                rightMargin: -36;
                bottomMargin: -44;
            }
            border { left: 36; top: 28; right: 36; bottom: 44; }
            horizontalTileMode: BorderImage.Stretch;
            verticalTileMode: BorderImage.Stretch;
            source: Settings.theme.image("drop-shadows.png");
            opacity: (base.state === "sidebarShown") ? 1 : 0;
            Behavior on opacity { PropertyAnimation { duration: Constants.AnimationDuration; } }
            BorderImage {
                anchors {
                    fill: parent;
                    topMargin: 28;
                    leftMargin: 36;
                    rightMargin: 36;
                    bottomMargin: 44;
                }
                border { left: 8; top: 8; right: 8; bottom: 8; }
                horizontalTileMode: BorderImage.Stretch;
                verticalTileMode: BorderImage.Stretch;
                source: Settings.theme.image("drop-corners.png");
            }
        }
        Item {
            anchors {
                left: parent.right;
                verticalCenter: parent.verticalCenter;
            }
            height: Constants.GridHeight * 2;
            width: Constants.DefaultMargin * 3;
            clip: true;
            Rectangle {
                anchors {
                    fill: parent;
                    leftMargin: -(radius + 1);
                }
                radius: Constants.DefaultMargin;
                color: "#55595e";
                opacity: 0.5;
            }
            Rectangle {
                anchors {
                    top: parent.top;
                    bottom: parent.bottom;
                    margins: Constants.DefaultMargin;
                    horizontalCenter: parent.horizontalCenter;
                }
                width: 4;
                radius: 2;
                color: "white";
                opacity: 0.5;
            }
            MouseArea {
                anchors.fill: parent;
                onClicked: {
                    if(base.state === "sidebarShown" && base.state !== "readermode") {
                        base.state = "";
                        pageNumber.opacity = 0;
                    }
                    else {
                        d.showThings();
                    }
                    base.canvasInteractionStarted();
                }
            }
        }
        Rectangle {
            anchors {
                fill: parent;
                leftMargin: -Constants.DefaultMargin + 1;
            }
            radius: Constants.DefaultMargin;
            color: "#55595e";
            opacity: 0.5;
            Rectangle {
                anchors.fill: parent;
                radius: parent.radius;
                color: "transparent";
                border.width: 1;
                border.color: "black";
                opacity: 0.6;
            }
        }
        MouseArea {
            id: listViewMouseArea;
            anchors.fill: parent;
            hoverEnabled: true;
        }
        Button {
            anchors {
                top: parent.top;
                left: parent.left;
                right: parent.right;
            }
            height: Constants.GridHeight / 2;
            image: Settings.theme.icon("Arrow-ScrollUp-1");
            imageMargin: 2;
            Rectangle {
                anchors {
                    left: parent.left;
                    right: parent.right;
                    rightMargin: 1;
                    bottom: parent.bottom;
                }
                height: 1;
                color: "black";
                opacity: 0.3;
            }
        }
        Button {
            anchors {
                left: parent.left;
                right: parent.right;
                bottom: parent.bottom;
            }
            height: Constants.GridHeight / 2;
            image: Settings.theme.icon("Arrow-ScrollDown-1");
            imageMargin: 2;
            Rectangle {
                anchors {
                    top: parent.top;
                    left: parent.left;
                    right: parent.right;
                    rightMargin: 1;
                }
                height: 1;
                color: "black";
                opacity: 0.3;
            }
        }
        ListView {
            id: navigatorListView;
            anchors {
                fill: parent;
                topMargin: Constants.GridHeight / 2;
                bottomMargin: Constants.GridHeight / 2;
                rightMargin: 1;
            }
            clip: true;
            model: wordsCanvas.documentModel;
            delegate: Item {
                width: Settings.theme.adjustedPixel(190);
                height: Settings.theme.adjustedPixel(190);
                Image {
                    anchors {
                        top: parent.top;
                        right: parent.right;
                        bottom: parent.bottom;
                        margins: Settings.theme.adjustedPixel(5);
                    }
                    width: Settings.theme.adjustedPixel(140);
                    fillMode: Image.PreserveAspectFit;
                    source: model.decoration;
                    smooth: true;
                    Rectangle {
                        anchors.fill: parent;
                        color: "transparent";
                        border.width: 1;
                        border.color: "black";
                        opacity: 0.1;
                    }
                }
                Rectangle {
                    anchors.fill: parent;
                    color: "#00adf5"
                    opacity: (wordsCanvas.currentPageNumber === index + 1) ? 0.4 : 0;
                    Behavior on opacity { PropertyAnimation { duration: Constants.AnimationDuration; } }
                }
                Label {
                    anchors {
                        left: parent.left;
                        leftMargin: Constants.DefaultMargin;
                        verticalCenter: parent.verticalCenter;
                    }
                    text: index + 1;
                    font: Settings.theme.font("applicationSemi");
                    color: "#c1cdd1";
                }
                MouseArea {
                    anchors.fill: parent;
                    onClicked: {
                        controllerFlickable.contentY = wordsCanvas.pagePosition(index + 1) + 1;
                        base.canvasInteractionStarted();
                    }
                }
            }
        }
    }
    Item {
        id: pageNumber;
        anchors {
            right: parent.right;
            bottom: parent.bottom;
            margins: Constants.DefaultMargin;
        }
        opacity: 0;
        Behavior on opacity { PropertyAnimation { duration: Constants.AnimationDuration; } }
        height: Constants.GridHeight / 2;
        width: Constants.GridWidth;
        Rectangle {
            anchors.fill: parent;
            radius: Constants.DefaultMargin;
            color: Settings.theme.color("components/overlay/base");
            opacity: 0.7;
        }
        Label {
            anchors.centerIn: parent;
            color: Settings.theme.color("components/overlay/text");
            text: (wordsCanvas.documentModel === null) ? 0 : wordsCanvas.currentPageNumber + " of " + wordsCanvas.documentModel.rowCount();
        }
    }
    Item {
        id: zoomLevel;
        anchors {
            right: parent.right;
            bottom: (pageNumber.opacity > 0) ? pageNumber.top : parent.bottom;
            margins: Constants.DefaultMargin;
        }
        opacity: 0;
        Behavior on opacity { PropertyAnimation { duration: Constants.AnimationDuration; } }
        height: Constants.GridHeight / 2;
        width: Constants.GridWidth;
        Rectangle {
            anchors.fill: parent;
            radius: Constants.DefaultMargin;
            color: Settings.theme.color("components/overlay/base");
            opacity: 0.7;
        }
        Timer {
            id: hideZoomLevelTimer;
            repeat: false; running: false; interval: 1000;
            onTriggered: zoomLevel.opacity = 0;
        }
        Label {
            anchors.centerIn: parent;
            color: Settings.theme.color("components/overlay/text");
            text: wordsCanvas.zoomAction ? (wordsCanvas.zoomAction.effectiveZoom * 100).toFixed(2) + "%" : "";
            onTextChanged: {
                zoomLevel.opacity = 1;
                hideZoomLevelTimer.start();
            }
        }
    }
}
