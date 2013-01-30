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
#include "TextPlugin.h"
#include "TextToolFactory.h"
#include "ReferencesToolFactory.h"
#include "ReviewToolFactory.h"
#ifdef CREATE_TEXTDOCUMENT_INSPECTOR
#include "TextDocumentInspectionDockerFactory.h"
#endif
#include "TextShapeFactory.h"

#include <KoShapeRegistry.h>
#include <KoDockRegistry.h>
#include <KoToolRegistry.h>

#include <kpluginfactory.h>

K_PLUGIN_FACTORY(TextPluginFactory, registerPlugin<TextPlugin>();)
K_EXPORT_PLUGIN(TextPluginFactory("TextShape"))

TextPlugin::TextPlugin(QObject * parent, const QVariantList &)
        : QObject(parent)
{
    KoToolRegistry::instance()->add(new TextToolFactory());
    KoToolRegistry::instance()->add(new ReviewToolFactory());
    KoToolRegistry::instance()->add(new ReferencesToolFactory());
#ifdef CREATE_TEXTDOCUMENT_INSPECTOR
    KoDockRegistry::instance()->add(new TextDocumentInspectionDockerFactory());
#endif
    KoShapeRegistry::instance()->add(new TextShapeFactory());
}

#include <TextPlugin.moc>
