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
import org.calligra 1.0
import "git" as Git
import "../../components"

Page {
    id: base;
    property string pageName: "accountsPageGit";
    property QtObject accountDetails: null;
    Git.RepositoryContent {
        anchors.fill: parent;
        localrepo: (accountDetails !== null) ? accountDetails.readProperty("localrepo") : "";
        privateKeyFile: (accountDetails !== null) ? accountDetails.readProperty("privateKeyFile") : "";
        needsPrivateKeyPassphrase: (accountDetails !== null) ? accountDetails.readProperty("needsPrivateKeyPassphrase") : false;
        publicKeyFile: (accountDetails !== null) ? accountDetails.readProperty("publicKeyFile") : "";
        userForRemote: (accountDetails !== null) ? accountDetails.readProperty("userForRemote") : "";
    }
}
