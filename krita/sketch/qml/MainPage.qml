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
import "components"
import "panels"

Page {
    property string pageName: "MainPage";
    SketchView {
        id: sketchView;
        width: parent.width;
        height: parent.height;

        file: parent.initialFile;

        onInteractionStarted: { panelBar.collapse(); Krita.VirtualKeyboardController.requestHideKeyboard(); }
        onLoadingFinished: {
            loadingDialog.hide("Done!");
            savingDialog.hide("Done!");
        }
        onSavingFinished: {
            loadingDialog.hide("Done!");
            savingDialog.hide("Done!");

            if (d.saveRequested) {
                d.loadNewFile();
            }

            if (d.closeRequested) {
                d.closeWindow();
            }
        }
        onProgress: {
            if (value === -1 || value === 100) {
                loadingDialog.hide("Done!");
                savingDialog.hide("Done!");
            }
            loadingDialog.progress = value;
            savingDialog.progress = value;
        }
        onViewChanged: if (window.sketchKisView !== undefined) { window.sketchKisView = view; }
    }
    Connections {
        target: window;
        onSwitchedToSketch: sketchView.activate();
    }


    ToolManager {
        id: toolManager;
        view: sketchView.view;
    }

    LayerModel {
        id: layerModel;
        // Notice - the model needs to know about the engine before the view, hence it is set first
        // This could be rectified, but for now know that the order here is important.
        engine: QMLEngine;
        view: sketchView.view;
    }

    PanelBar {
        id: panelBar;
        panelHeight: parent.height;
        width: parent.width;
    }

    NewImagePanel {
        id: newPanel;
        anchors.left: parent.left;
        width: Constants.GridWidth * 4;
        height: parent.height;

        onClicked: d.beginCreateNewFile(options);
    }

    OpenImagePanel {
        id: openPanel;
        anchors.left: parent.left;
        width: Constants.GridWidth * 4;
        height: parent.height;

        onClicked: d.beginOpenFile(file);
    }

    MenuPanel {
        id: menuPanel;

        anchors.bottom: parent.bottom;

        width: parent.width;
        z: 10;

        newButtonChecked: !newPanel.collapsed;
        openButtonChecked: !openPanel.collapsed;

        onCollapsedChanged: if ( collapsed ) {
            newPanel.collapsed = true;
            openPanel.collapsed = true;
        }

        onButtonClicked: {
            switch( button ) {
                case "new": {
                    d.previousFile = Settings.currentFile;
                    newPanel.collapsed = !newPanel.collapsed;
                    openPanel.collapsed = true;
                }
                case "open": {
                    d.previousFile = Settings.currentFile;
                    openPanel.collapsed = !openPanel.collapsed;
                    newPanel.collapsed = true;
                }
                case "save":
                    if (!Settings.temporaryFile) {
                        savingDialog.show("Saving file...");
                        sketchView.save();
                    } else {
                        pageStack.push( saveAsPage, { view: sketchView } );
                    }
                case "saveAs":
                    pageStack.push( saveAsPage, { view: sketchView } );
                case "settings":
                    pageStack.push( settingsPage );
                case "help":
                    pageStack.push( helpPage );
                case "undo":
                    sketchView.undo();
                case "redo":
                    sketchView.redo();
                case "minimize":
                    Krita.Window.minimize();
                case "close":
                    Krita.Window.close();
                case "zoomIn":
                    sketchView.zoomIn();
                case "zoomOut":
                    sketchView.zoomOut();
                case "switchToDesktop":
                    switchToDesktopAction.trigger();
            }
        }
    }

    MessageStack {
        id: messageStack;
        anchors {
            horizontalCenter: parent.horizontalCenter;
            bottom: menuPanel.top;
            bottomMargin: Constants.GridHeight;
        }
        Connections {
            target: sketchView;
            onFloatingMessageRequested: messageStack.showMessage(message, iconName);
        }
    }

    ToolOverlayPanel {
        id: toolOverlay;
        anchors {
            left: menuPanel.left;
            leftMargin: (Constants.IsLandscape ? Constants.GridWidth * 4: Constants.GridWidth * 2) + Constants.DefaultMargin;
            right: menuPanel.right;
            rightMargin: (Constants.IsLandscape ? 0 : Constants.GridWidth * 2) + Constants.DefaultMargin;
            bottom: menuPanel.top;
            bottomMargin: Constants.DefaultMargin;
        }
    }

    Dialog {
        id: loadingDialog;
        title: "Loading";
        message: "Please wait...";
        textAlign: Text.AlignHCenter;
        progress: 0;
        modalBackgroundColor: "#ffffff";
    }

    Dialog {
        id: savingDialog;
        title: "Saving";
        message: "Please wait...";
        textAlign: Text.AlignHCenter;

        modalBackgroundColor: "#ffffff";
    }

    Dialog {
        id: progressDialog;

        title: Krita.ProgressProxy.taskName != "" ? Krita.ProgressProxy.taskName : "Applying...";
    }

    Dialog {
        id: modifiedDialog;
        title: "Image was modified";
        message: "The image was modified. Do you want to save your changes?";

        buttons: [ "Save", "Discard", "Cancel" ];

        onButtonClicked: {
            switch(button) {
                case 0: {
                    if (Settings.temporaryFile) {
                        d.saveRequested = true;
                        pageStack.push( saveAsPage, { view: sketchView, updateCurrentFile: false } );
                    } else {
                        savingDialog.show("Please wait...");
                        sketchView.save();

                        if (d.closeRequested) {
                            d.closeWindow();
                        } else {
                            d.loadNewFile();
                        }
                    }
                }
                case 1: {
                    if (d.closeRequested) {
                        d.closeWindow();
                    } else {
                        d.loadNewFile();
                    }
                }
                default: {
                    Settings.currentFile = d.previousFile;
                    d.saveRequested = false;
                    d.closeRequested = false;
                }
            }
        }

        onCanceled: {
            Settings.currentFile = d.previousFile;
            d.saveRequested = false;
            d.closeRequested = false;
        }
    }

    Connections {
        target: Krita.ProgressProxy;

        onTaskStarted: progressDialog.show();
        onTaskEnded: progressDialog.hide();

        onValueChanged: progressDialog.progress = value;
    }

    Connections {
        target: Settings;
        onTemporaryFileChanged: if (window.temporaryFile !== undefined) window.temporaryFile = Settings.temporaryFile;
    }

    Connections {
        target: Krita.Window;

        onCloseRequested: {
            if (sketchView.modified) {
                d.closeRequested = true;
                modifiedDialog.show();
            } else {
                d.closeWindow();
            }
        }
    }

    Component.onCompleted: {
        Krita.Window.allowClose = false;
        loadingDialog.show("Please wait...");

        if(Settings.currentFile.indexOf("temp") == -1) {
            sketchView.file = Settings.currentFile;
        }
    }

    Component { id: openImagePage; OpenImagePage { onFinished: { pageStack.pop(); d.beginOpenFile(file); } } }
    Component { id: settingsPage; SettingsPage { } }
    Component { id: helpPage; HelpPage { } }
    Component { id: saveAsPage; SaveImagePage { onFinished: { pageStack.pop(); d.saveFileAs(file, type); } } }
    Component { id: customImagePage; CustomImagePage { onFinished: { pageStack.pop(); d.beginCreateNewFile(options); } } }

    QtObject {
        id: d;

        property string previousFile;
        property bool closeRequested;
        property bool saveRequested;

        property variant newFileOptions;
        property string fileToOpen;

        function beginCreateNewFile(options) {
            if(options !== undefined) {
                newFileOptions = options;
                if (sketchView.modified) {
                    modifiedDialog.show();
                } else {
                    d.loadNewFile();
                }
            } else {
                pageStack.push(customImagePage);
            }
        }

        function beginOpenFile(file) {
            if(file === sketchView.file)
                return;

            if(file !== "") {
                fileToOpen = file;
                if(sketchView.modified) {
                    modifiedDialog.show();
                } else {
                    d.loadNewFile();
                }
            } else {
                pageStack.push(openImagePage);
            }
        }

        function loadNewFile() {
            saveRequested = false;
            loadingDialog.progress = 0;

            if(newFileOptions !== undefined) {
                loadingDialog.show("Creating new image...");
                if(newFileOptions.source === undefined) {
                    Settings.currentFile = Krita.ImageBuilder.createBlankImage(newFileOptions);
                    Settings.temporaryFile = true;
                } else if(newFileOptions.source == "clipboard") {
                    Settings.currentFile = Krita.ImageBuilder.createImageFromClipboard();
                    Settings.temporaryFile = true;
                }
            } else {
                loadingDialog.show("Loading " + fileToOpen);
                Settings.currentFile = fileToOpen;
                sketchView.file = Settings.currentFile;
            }
            menuPanel.collapsed = true;
            fileToOpen = "";
            newFileOptions = null;
        }

        function saveFileAs(file, type) {
            savingDialog.show("Saving image to " + file);

            sketchView.saveAs( file, type );

            Settings.temporaryFile = false;
        }

        function closeWindow() {

            Krita.Window.allowClose = true;
            Krita.Window.closeWindow();
        }
    }
}
