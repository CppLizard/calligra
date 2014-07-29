/*
 *  Copyright (c) 2014 Sven Langkamp <sven.langkamp@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */


#include "kis_tooltip_manager.h"

#include <QFile>
#include <QAction>
#include <QInputDialog>
#include <QDomDocument>
#include <QFile>
#include <kactioncollection.h>
#include <kdebug.h>
#include <kmenubar.h>

#include <KoMainWindow.h>
#include "kis_view2.h"

KisTooltipManager::KisTooltipManager(KisView2* view) : QObject(view), m_view(view), m_recording(false)
{
    m_view->mainWindow()->menuBar()->installEventFilter(this);
}

KisTooltipManager::~KisTooltipManager()
{
    if (m_recording) {
        QFile f("tooltips.txt");
        f.open(QFile::WriteOnly);

        QDomDocument doc;
        QDomElement root;

        root = doc.createElement("tooltips");
        doc.appendChild(root);


        QMapIterator<QString, QString> it(m_tooltipMap);
        while (it.hasNext()) {
            it.next();
            QDomElement tooltip = doc.createElement("tooltip");
            tooltip.setAttribute("action", it.key());
            tooltip.appendChild(doc.createTextNode(it.value()));
            root.appendChild(tooltip);
        }
        QTextStream stream(&f);
        stream << doc.toString();

        f.close();
    }
}

void KisTooltipManager::record()
{
    m_recording = true;
    QList<QAction*> actions =  m_view->actionCollection()->actions();
    foreach(KXMLGUIClient* client, m_view->childClients() ) {
        actions.append(client->actionCollection()->actions());
    }

    foreach(QAction* action, actions) {
        action->disconnect();
        connect(action, SIGNAL(triggered()), this, SLOT(captureToolip()));
    }
}

void KisTooltipManager::captureToolip()
{
    QString id = sender()->objectName();

    QString oldTooltip;
    if (m_tooltipMap.contains(id)) {
        oldTooltip = m_tooltipMap[id];
    }

    bool ok;
    QString tooltip = QInputDialog::getText(m_view, "Add Tooltip",
                                            "New Tooltip:", QLineEdit::Normal,
                                            oldTooltip, &ok);
    if (ok && !tooltip.isEmpty()) {
        dynamic_cast<QAction*>(sender())->setToolTip(tooltip);
        m_tooltipMap[id] = tooltip;
    }
}


#include "kis_tooltip_manager.moc"
