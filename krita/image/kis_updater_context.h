/*
 *  Copyright (c) 2010 Dmitry Kazakov <dimula73@gmail.com>
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

#ifndef __KIS_UPDATER_CONTEXT_H
#define __KIS_UPDATER_CONTEXT_H

#include <QMutex>
#include <QRunnable>
#include <QThreadPool>

#include "kis_base_rects_walker.h"
#include "kis_async_merger.h"

class KisUpdateJobItem :  public QObject, public QRunnable
{
    Q_OBJECT

public:
    KisUpdateJobItem() {
        setAutoDelete(false);
    }

    void run() {
//        qDebug() << "Executing job" << m_walker->changeRect() << "on thread" << QThread::currentThreadId();
        m_merger.startMerge(*m_walker);

        QRect changeRect = m_walker->changeRect();
        emit sigContinueUpdate(changeRect);
        setDone();

        emit sigDoSomeUsefulWork();
        emit sigJobFinished();
    }

    inline void setWalker(KisBaseRectsWalkerSP walker) {
        m_accessRect = walker->accessRect();
        m_changeRect = walker->changeRect();

        m_walker = walker;
    }

    inline void setDone() {
        m_walker = 0;
    }

    inline bool isRunning() const {
        return m_walker;
    }

    inline const QRect& accessRect() const {
        return m_accessRect;
    }

    inline const QRect& changeRect() const {
        return m_changeRect;
    }

signals:
    void sigContinueUpdate(const QRect& rc);
    void sigDoSomeUsefulWork();
    void sigJobFinished();

private:
    /**
     * Open walker for the testing suite.
     * Please, do not use it in production code.
     */
    friend class KisSimpleUpdateQueueTest;
    inline KisBaseRectsWalkerSP walker() const {
        return m_walker;
    }

private:
    KisBaseRectsWalkerSP m_walker;
    KisAsyncMerger m_merger;

    /**
     * These rects cache actual values from the walker
     * to iliminate concurrent access to a walker structure
     */
    QRect m_accessRect;
    QRect m_changeRect;
};


class KRITAIMAGE_EXPORT KisUpdaterContext : public QObject
{
    Q_OBJECT

public:
    KisUpdaterContext(qint32 threadCount = -1);
    virtual ~KisUpdaterContext();


    /**
     * Check whether there is a spare thread for running
     * one more job
     */
    bool hasSpareThread();

    /**
     * Checks whether the walker intersects with any
     * of currently executing walkers. If it does,
     * it is not allowed to go in. It should be called
     * with the lock held.
     *
     * \see lock()
     */
    bool isJobAllowed(KisBaseRectsWalkerSP walker);

    /**
     * Registers the job and starts executing it.
     * The caller must ensure that the context is locked
     * with lock(), job is allowed with isWalkerAllowed() and
     * there is a spare thread for running it with hasSpareThread()
     *
     * \see lock()
     * \see isWalkerAllowed()
     * \see hasSpareThread()
     */
    virtual void addJob(KisBaseRectsWalkerSP walker);

    /**
     * Block execution of the caller until all the jobs are finished
     */
    void waitForDone();

    /**
     * Locks the context to guarantee an exclusive access
     * to the context
     */
    void lock();

    /**
     * Unlocks the context
     *
     * \see lock()
     */
    void unlock();

signals:
    void sigContinueUpdate(const QRect& rc);
    void sigDoSomeUsefulWork();
    void sigSpareThreadAppeared();

protected slots:
    void slotJobFinished();

protected:
    static bool walkerIntersectsJob(KisBaseRectsWalkerSP walker,
                                    const KisUpdateJobItem* job);
    qint32 findSpareThread();

protected:
    QMutex m_lock;
    QVector<KisUpdateJobItem*> m_jobs;
    QThreadPool m_threadPool;
};

class KRITAIMAGE_EXPORT KisTestableUpdaterContext : public KisUpdaterContext
{
public:
    /**
     * Creates an explicit number of threads
     */
    KisTestableUpdaterContext(qint32 threadCount);

    /**
     * The only difference - it doesn't start execution
     * of the job
     */
    void addJob(KisBaseRectsWalkerSP walker);

    const QVector<KisUpdateJobItem*> getJobs();
    void clear();
};


#endif /* __KIS_UPDATER_CONTEXT_H */

