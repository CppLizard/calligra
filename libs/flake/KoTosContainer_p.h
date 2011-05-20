#ifndef KOTOSCONTAINER_P_H
#define KOTOSCONTAINER_P_H

#include "KoShapeContainer_p.h"

#include "KoTosContainer.h"

class KoTosContainerPrivate : public KoShapeContainerPrivate
{
public:

    KoTosContainerPrivate(KoShapeContainer *q);

    virtual ~KoTosContainerPrivate();

    KoTosContainer::ResizeBehavior resizeBehavior;
    QRectF preferredTextRect;
};

#endif // KOTOSCONTAINER_P_H
