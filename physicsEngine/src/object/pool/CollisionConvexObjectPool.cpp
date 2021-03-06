#include "CollisionConvexObjectPool.h"
#include "object/CollisionConvexObject3D.h"
#include "object/CollisionBoxObject.h"
#include "object/CollisionCapsuleObject.h"
#include "object/CollisionConeObject.h"
#include "object/CollisionConvexHullObject.h"
#include "object/CollisionCylinderObject.h"
#include "object/CollisionCylinderObject.h"
#include "object/CollisionTriangleObject.h"

namespace urchin
{

    CollisionConvexObjectPool::CollisionConvexObjectPool()
    {
        unsigned int maxElementSize = maxObjectSize({
            sizeof(CollisionBoxObject),
            sizeof(CollisionCapsuleObject),
            sizeof(CollisionConeObject),
            sizeof(CollisionConvexHullObject),
            sizeof(CollisionCylinderObject),
            sizeof(CollisionCylinderObject),
            sizeof(CollisionTriangleObject)
        });
        unsigned int objectsPoolSize = ConfigService::instance()->getUnsignedIntValue("collisionObject.poolSize");

        //pool is synchronized because elements are created in narrow phase (= synchronized phase called by different threads) and deleted by different threads outside the narrow phase
        objectsPool = new SyncFixedSizePool<CollisionConvexObject3D>("collisionConvexObjectsPool", maxElementSize, objectsPoolSize);
    }

    CollisionConvexObjectPool::~CollisionConvexObjectPool()
    {
        delete objectsPool;
    }

    SyncFixedSizePool<CollisionConvexObject3D> *CollisionConvexObjectPool::getObjectsPool()
    {
        return objectsPool;
    }

    unsigned int CollisionConvexObjectPool::maxObjectSize(std::vector<unsigned int> objectsSize)
    {
        unsigned int result = 0;
        for(unsigned int objectSize : objectsSize)
        {
            result = std::max(result, objectSize);
        }
        return result;
    }

}
