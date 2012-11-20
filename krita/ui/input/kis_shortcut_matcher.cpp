/*
 *  Copyright (c) 2012 Dmitry Kazakov <dimula73@gmail.com>
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

#include "kis_shortcut_matcher.h"

#include <QMouseEvent>

#include "kis_abstract_input_action.h"
#include "kis_stroke_shortcut.h"


struct KisShortcutMatcher::Private
{
    Private() : suppressAllActions(false) {}

    QList<KisSingleActionShortcut*> keyShortcuts;
    QList<KisStrokeShortcut*> strokeShortcuts;
    QList<KisAbstractInputAction*> actions;

    QList<Qt::Key> keys;
    QList<Qt::MouseButton> buttons;

    KisStrokeShortcut *runningShortcut;
    KisStrokeShortcut *readyShortcut;
    QList<KisStrokeShortcut*> readyShortcuts;

    bool suppressAllActions;
};

KisShortcutMatcher::KisShortcutMatcher()
    : m_d(new Private)
{
    m_d->runningShortcut = 0;
    m_d->readyShortcut = 0;
}

KisShortcutMatcher::~KisShortcutMatcher()
{
    qDeleteAll(m_d->keyShortcuts);
    qDeleteAll(m_d->strokeShortcuts);
    qDeleteAll(m_d->actions);
    delete m_d;
}

void KisShortcutMatcher::addShortcut(KisSingleActionShortcut *shortcut)
{
    m_d->keyShortcuts.append(shortcut);
}

void KisShortcutMatcher::addShortcut(KisStrokeShortcut *shortcut)
{
    m_d->strokeShortcuts.append(shortcut);
}

void KisShortcutMatcher::addAction(KisAbstractInputAction *action)
{
    m_d->actions.append(action);
}

bool KisShortcutMatcher::keyPressed(Qt::Key key)
{
    bool retval = false;

    if (m_d->keys.contains(key)) reset();

    if (!m_d->runningShortcut) {
        retval = tryRunSingleActionShortcut(key, 0);
    }

    m_d->keys.append(key);

    if (!m_d->runningShortcut) {
        prepareReadyShortcuts();
        tryActivateReadyShortcut();
    }

    return retval;
}

bool KisShortcutMatcher::keyReleased(Qt::Key key)
{
    if (!m_d->keys.contains(key)) reset();
    else m_d->keys.removeOne(key);

    if (!m_d->runningShortcut) {
        prepareReadyShortcuts();
        tryActivateReadyShortcut();
    }

    return false;
}

bool KisShortcutMatcher::buttonPressed(Qt::MouseButton button, QMouseEvent *event)
{
    bool retval = false;

    if (m_d->buttons.contains(button)) reset();

    if (!m_d->runningShortcut) {
        prepareReadyShortcuts();
        retval = tryRunReadyShortcut(button, event);
    }

    m_d->buttons.append(button);

    if (!m_d->runningShortcut) {
        prepareReadyShortcuts();
        tryActivateReadyShortcut();
    }

    return retval;
}

bool KisShortcutMatcher::buttonReleased(Qt::MouseButton button, QMouseEvent *event)
{
    bool retval = false;

    if (m_d->runningShortcut) {
        retval = tryEndRunningShortcut(button, event);
    }

    if (!m_d->buttons.contains(button)) reset();
    else m_d->buttons.removeOne(button);

    if (!m_d->runningShortcut) {
        prepareReadyShortcuts();
        tryActivateReadyShortcut();
    }

    return retval;
}

bool KisShortcutMatcher::wheelEvent(KisSingleActionShortcut::WheelAction wheelAction, QWheelEvent *event)
{
    if (m_d->runningShortcut) return false;

    return tryRunWheelShortcut(wheelAction, event);
}

bool KisShortcutMatcher::mouseMoved(QMouseEvent *event)
{
    if (!m_d->runningShortcut) return false;

    m_d->runningShortcut->action()->inputEvent(event);
    return true;
}

void KisShortcutMatcher::reset()
{
    m_d->keys.clear();
    m_d->buttons.clear();
}

void KisShortcutMatcher::suppressAllActions(bool value)
{
    m_d->suppressAllActions = value;
}

bool KisShortcutMatcher::tryRunWheelShortcut(KisSingleActionShortcut::WheelAction wheelAction, QWheelEvent *event)
{
    return tryRunSingleActionShortcutImpl(wheelAction, event);
}

bool KisShortcutMatcher::tryRunSingleActionShortcut(Qt::Key key, QKeyEvent *event)
{
    return tryRunSingleActionShortcutImpl(key, event);
}

template<typename T, typename U>
bool KisShortcutMatcher::tryRunSingleActionShortcutImpl(T param, U *event)
{
    if (m_d->suppressAllActions) return false;

    KisSingleActionShortcut *goodCandidate = 0;

    foreach(KisSingleActionShortcut *s, m_d->keyShortcuts) {
        if(s->match(m_d->keys, param) &&
           (!goodCandidate || s->priority() > goodCandidate->priority())) {

            goodCandidate = s;
        }
    }

    if (goodCandidate) {
        goodCandidate->action()->begin(goodCandidate->shortcutIndex(), event);
        goodCandidate->action()->end(0);
    }

    return goodCandidate;
}

void KisShortcutMatcher::prepareReadyShortcuts()
{
    m_d->readyShortcuts.clear();
    if (m_d->suppressAllActions) return;

    foreach(KisStrokeShortcut *s, m_d->strokeShortcuts) {
        if (s->matchReady(m_d->keys, m_d->buttons)) {
            m_d->readyShortcuts.append(s);
        }
    }
}

bool KisShortcutMatcher::tryRunReadyShortcut(Qt::MouseButton button, QMouseEvent *event)
{
    KisStrokeShortcut *goodCandidate = 0;

    foreach(KisStrokeShortcut *s, m_d->readyShortcuts) {
        if (s->matchBegin(button) &&
            (!goodCandidate || s->priority() > goodCandidate->priority())) {

            goodCandidate = s;
        }
    }

    if (goodCandidate) {
        if (m_d->readyShortcut) {
            if (m_d->readyShortcut != goodCandidate) {
                m_d->readyShortcut->action()->deactivate();
                goodCandidate->action()->activate();
            }
            m_d->readyShortcut = 0;
        } else {
            goodCandidate->action()->activate();
        }

        goodCandidate->action()->begin(goodCandidate->shortcutIndex(), event);
        m_d->runningShortcut = goodCandidate;
    }

    return goodCandidate;
}

void KisShortcutMatcher::tryActivateReadyShortcut()
{
    KisStrokeShortcut *goodCandidate = 0;

    foreach(KisStrokeShortcut *s, m_d->readyShortcuts) {
        if (!goodCandidate || s->priority() > goodCandidate->priority()) {
            goodCandidate = s;
        }
    }

    if (goodCandidate) {
        if (m_d->readyShortcut && m_d->readyShortcut != goodCandidate) {
            m_d->readyShortcut->action()->deactivate();
            m_d->readyShortcut = 0;
        }

        if (!m_d->readyShortcut) {
            goodCandidate->action()->activate();
            m_d->readyShortcut = goodCandidate;
        }
    } else if (m_d->readyShortcut) {
        m_d->readyShortcut->action()->deactivate();
        m_d->readyShortcut = 0;
    }
}

bool KisShortcutMatcher::tryEndRunningShortcut(Qt::MouseButton button, QMouseEvent *event)
{
    Q_ASSERT(m_d->runningShortcut);
    Q_ASSERT(!m_d->readyShortcut);

    if (m_d->runningShortcut->matchBegin(button)) {
        m_d->runningShortcut->action()->end(event);
        m_d->runningShortcut->action()->deactivate();
        m_d->runningShortcut = 0;
    }

    return !m_d->runningShortcut;
}
