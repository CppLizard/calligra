/*
 *  Copyright (c) 2011 Dmitry Kazakov <dimula73@gmail.com>
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

#include "kis_strokes_queue.h"

#include <QQueue>
#include <QMutex>
#include <QMutexLocker>
#include "kis_stroke.h"
#include "kis_updater_context.h"

struct KisStrokesQueue::Private {
    Private()
        : openedStrokesCounter(0),
          needsExclusiveAccess(false),
          wrapAroundModeSupported(false),
          lodNNeedsSynchronization(true),
          desiredLevelOfDetail(0) {}

    QQueue<KisStrokeSP> strokesQueue;
    int openedStrokesCounter;
    bool needsExclusiveAccess;
    bool wrapAroundModeSupported;
    bool lodNNeedsSynchronization;
    int desiredLevelOfDetail;
    QMutex mutex;
    KisStrokeStrategyFactory lod0ToNStrokeStrategyFactory;

    void startLod0ToNStroke(int levelOfDetail);
};


KisStrokesQueue::KisStrokesQueue()
  : m_d(new Private)
{
}

KisStrokesQueue::~KisStrokesQueue()
{
    foreach(KisStrokeSP stroke, m_d->strokesQueue) {
        stroke->cancelStroke();
    }

    delete m_d;
}

void KisStrokesQueue::Private::startLod0ToNStroke(int levelOfDetail)
{
    // precondition: lock held!
    if (!this->lod0ToNStrokeStrategyFactory) return;

    KisStrokeStrategy *lod0ToN = this->lod0ToNStrokeStrategyFactory();
    KisStrokeSP sync(new KisStroke(lod0ToN, levelOfDetail));
    lod0ToN->setCancelStrokeId(sync);
    this->strokesQueue.enqueue(sync);
    sync->endStroke();
    this->lodNNeedsSynchronization = false;
}

KisStrokeId KisStrokesQueue::startStroke(KisStrokeStrategy *strokeStrategy)
{
    QMutexLocker locker(&m_d->mutex);

    KisStrokeStrategy* lodBuddyStrategy =
        m_d->desiredLevelOfDetail ?
        strokeStrategy->createLodClone(m_d->desiredLevelOfDetail) : 0;

    KisStrokeSP stroke(new KisStroke(strokeStrategy));

    if (lodBuddyStrategy) {
        if (m_d->lodNNeedsSynchronization) {
            m_d->startLod0ToNStroke(m_d->desiredLevelOfDetail);
        }

        KisStrokeSP buddy(new KisStroke(lodBuddyStrategy, m_d->desiredLevelOfDetail));
        lodBuddyStrategy->setCancelStrokeId(buddy);
        stroke->setLodBuddy(buddy);
        m_d->strokesQueue.enqueue(buddy);
    }

    KisStrokeId id(stroke);
    strokeStrategy->setCancelStrokeId(id);
    m_d->strokesQueue.enqueue(stroke);
    m_d->openedStrokesCounter++;

    if (stroke->type() == KisStroke::LEGACY) {
        m_d->lodNNeedsSynchronization = false;
    }

    return id;
}

void KisStrokesQueue::addJob(KisStrokeId id, KisStrokeJobData *data)
{
    QMutexLocker locker(&m_d->mutex);

    KisStrokeSP stroke = id.toStrongRef();
    Q_ASSERT(stroke);
    stroke->addJob(data);

    KisStrokeSP buddy = stroke->lodBuddy();
    if (buddy) {
        KisStrokeJobData *clonedData =
            data->createLodClone(buddy->worksOnLevelOfDetail());
        KIS_ASSERT_RECOVER_RETURN(clonedData);

        buddy->addJob(clonedData);
    }
}

void KisStrokesQueue::endStroke(KisStrokeId id)
{
    QMutexLocker locker(&m_d->mutex);

    KisStrokeSP stroke = id.toStrongRef();
    Q_ASSERT(stroke);
    stroke->endStroke();
    m_d->openedStrokesCounter--;

    KisStrokeSP buddy = stroke->lodBuddy();
    if (buddy) {
        buddy->endStroke();
    }
}

bool KisStrokesQueue::cancelStroke(KisStrokeId id)
{
    QMutexLocker locker(&m_d->mutex);

    KisStrokeSP stroke = id.toStrongRef();
    if(stroke) {
        stroke->cancelStroke();
        m_d->openedStrokesCounter--;

        KisStrokeSP buddy = stroke->lodBuddy();
        if (buddy) {
            buddy->cancelStroke();
        }
    }
    return stroke;
}

bool KisStrokesQueue::tryCancelCurrentStrokeAsync()
{
    bool anythingCanceled = false;

    QMutexLocker locker(&m_d->mutex);

    if (!m_d->strokesQueue.isEmpty()) {
        KisStrokeSP currentStroke = m_d->strokesQueue.head();

        /**
         * We cancel only ended strokes. This is done to avoid
         * handling dangling pointers problem (KisStrokeId). The owner
         * of a stroke will cancel the stroke itself if needed.
         */
        if (currentStroke->isEnded()) {
            currentStroke->cancelStroke();
            anythingCanceled = true;

            KisStrokeSP buddy = currentStroke->lodBuddy();
            if (buddy) {
                buddy->endStroke();
            }
        }
    }

    /**
     * NOTE: We do not touch the openedStrokesCounter here since
     *       we work with closed id's only here
     */

    return anythingCanceled;
}

void KisStrokesQueue::processQueue(KisUpdaterContext &updaterContext,
                                   bool externalJobsPending)
{
    updaterContext.lock();
    m_d->mutex.lock();

    while(updaterContext.hasSpareThread() &&
          processOneJob(updaterContext,
                        externalJobsPending));

    m_d->mutex.unlock();
    updaterContext.unlock();
}

bool KisStrokesQueue::needsExclusiveAccess() const
{
    return m_d->needsExclusiveAccess;
}

bool KisStrokesQueue::wrapAroundModeSupported() const
{
    return m_d->wrapAroundModeSupported;
}

bool KisStrokesQueue::isEmpty() const
{
    QMutexLocker locker(&m_d->mutex);
    return m_d->strokesQueue.isEmpty();
}

qint32 KisStrokesQueue::sizeMetric() const
{
    QMutexLocker locker(&m_d->mutex);
    if(m_d->strokesQueue.isEmpty()) return 0;

    // just a rough approximation
    return m_d->strokesQueue.head()->numJobs() * m_d->strokesQueue.size();
}

void KisStrokesQueue::setDesiredLevelOfDetail(int lod)
{
    bool needsSync = m_d->desiredLevelOfDetail != lod;
    m_d->desiredLevelOfDetail = lod;

    if (needsSync) {
        m_d->lodNNeedsSynchronization = true;
        m_d->startLod0ToNStroke(m_d->desiredLevelOfDetail);
    }
}

void KisStrokesQueue::setLod0ToNStrokeStrategyFactory(const KisStrokeStrategyFactory &factory)
{
    m_d->lod0ToNStrokeStrategyFactory = factory;
}

KUndo2MagicString KisStrokesQueue::currentStrokeName() const
{
    QMutexLocker locker(&m_d->mutex);
    if(m_d->strokesQueue.isEmpty()) return KUndo2MagicString();

    return m_d->strokesQueue.head()->name();
}

bool KisStrokesQueue::hasOpenedStrokes() const
{
    QMutexLocker locker(&m_d->mutex);
    return m_d->openedStrokesCounter;
}

bool KisStrokesQueue::processOneJob(KisUpdaterContext &updaterContext,
                                    bool externalJobsPending)
{
    if(m_d->strokesQueue.isEmpty()) return false;
    bool result = false;

    qint32 numMergeJobs;
    qint32 numStrokeJobs;
    updaterContext.getJobsSnapshot(numMergeJobs, numStrokeJobs);

    int levelOfDetail = updaterContext.currentLevelOfDetail();

    if(checkStrokeState(numStrokeJobs, levelOfDetail) &&
       checkExclusiveProperty(numMergeJobs, numStrokeJobs) &&
       checkSequentialProperty(numMergeJobs, numStrokeJobs) &&
       checkBarrierProperty(numMergeJobs, numStrokeJobs,
                            externalJobsPending)) {

        KisStrokeSP stroke = m_d->strokesQueue.head();
        updaterContext.addStrokeJob(stroke->popOneJob());
        result = true;
    }

    return result;
}

bool KisStrokesQueue::checkStrokeState(bool hasStrokeJobsRunning,
                                       int runningLevelOfDetail)
{
    KisStrokeSP stroke = m_d->strokesQueue.head();
    bool result = false;

    /**
     * We cannot start/continue a stroke if its LOD differs from
     * the one that is running on CPU
     */
    bool hasLodCompatibility = checkLevelOfDetailProperty(runningLevelOfDetail);
    bool hasJobs = stroke->hasJobs();

    /**
     * The stroke may be cancelled very fast. In this case it will
     * end up in the state:
     *
     * !stroke->isInitialized() && stroke->isEnded() && !stroke->hasJobs()
     *
     * This means that !isInitialised() doesn't imply there are any
     * jobs present.
     */
    if(!stroke->isInitialized() && hasJobs && hasLodCompatibility) {
        m_d->needsExclusiveAccess = stroke->isExclusive();
        m_d->wrapAroundModeSupported = stroke->supportsWrapAroundMode();

        result = true;
    }
    else if(hasJobs && hasLodCompatibility) {
        result = true;
    }
    else if(stroke->isEnded() && !hasJobs && !hasStrokeJobsRunning) {
        m_d->strokesQueue.dequeue(); // deleted by shared pointer
        m_d->needsExclusiveAccess = false;
        m_d->wrapAroundModeSupported = false;

        if(!m_d->strokesQueue.isEmpty()) {
            result = checkStrokeState(false, runningLevelOfDetail);
        }
    }

    return result;
}

bool KisStrokesQueue::checkExclusiveProperty(qint32 numMergeJobs,
                                             qint32 numStrokeJobs)
{
    if(!m_d->strokesQueue.head()->isExclusive()) return true;
    Q_UNUSED(numMergeJobs);
    Q_UNUSED(numStrokeJobs);
    Q_ASSERT(!(numMergeJobs && numStrokeJobs));
    return numMergeJobs == 0;
}

bool KisStrokesQueue::checkSequentialProperty(qint32 numMergeJobs,
                                              qint32 numStrokeJobs)
{
    Q_UNUSED(numMergeJobs);

    KisStrokeSP stroke = m_d->strokesQueue.head();
    if(!stroke->prevJobSequential() && !stroke->nextJobSequential()) return true;

    Q_ASSERT(!stroke->prevJobSequential() || numStrokeJobs <= 1);
    return numStrokeJobs == 0;
}

bool KisStrokesQueue::checkBarrierProperty(qint32 numMergeJobs,
                                           qint32 numStrokeJobs,
                                           bool externalJobsPending)
{
    KisStrokeSP stroke = m_d->strokesQueue.head();
    if(!stroke->nextJobBarrier()) return true;

    return !numMergeJobs && !numStrokeJobs && !externalJobsPending;
}

bool KisStrokesQueue::checkLevelOfDetailProperty(int runningLevelOfDetail)
{
    KisStrokeSP stroke = m_d->strokesQueue.head();

    return runningLevelOfDetail < 0 ||
        stroke->worksOnLevelOfDetail() == runningLevelOfDetail;
}
