/*
 *  Copyright (c) 2013 Sven Langkamp <sven.langkamp@gmail.com>
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


#ifndef KIS_ACTION_MANAGER_H
#define KIS_ACTION_MANAGER_H

#include <kritaui_export.h>

#include <QPointer>

#include "KisView.h"

#include "kstandardaction.h"

class KisViewManager;
class KisAction;
class KisOperationUIFactory;
class KisOperation;
class KisOperationConfiguration;

/**
 * @brief The KisActionManager class keeps track of (nearly) all actions in krita, and enables
 * and disables the action depending on the state of the application.
 */
class KRITAUI_EXPORT KisActionManager
{

public:
    KisActionManager(KisViewManager* viewManager);
    virtual ~KisActionManager();

    void setView(QPointer<KisView> imageView);
    
    void addAction(const QString& name, KisAction* action);
    void takeAction(KisAction* action);

    KisAction *actionByName(const QString &name) const;

    void registerOperationUIFactory(KisOperationUIFactory* factory);

    void registerOperation(KisOperation* operation);

    void runOperation(const QString &id);

    void runOperationFromConfiguration(KisOperationConfiguration* config);
    
    void updateGUI();

    /// Create a KisAction based on a KStandardAction. The KStandardAction is deleted.
    KisAction *createStandardAction(KStandardAction::StandardAction,
                                    const QObject *receiver, const char *member);

private:
    void dumpActionFlags();

    class Private;
    Private* const d;
};

#endif // KIS_ACTION_MANAGER_H
