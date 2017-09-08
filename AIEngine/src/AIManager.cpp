#include <utility>
#include "UrchinCommon.h"

#include "AIManager.h"
#include "path/pathfinding/AStar.h"

namespace urchin
{

    AIManager::AIManager() :
        navMeshGenerator(new NavMeshGenerator())
    {
        NumericalCheck::instance()->perform();
    }

    AIManager::~AIManager()
    {
        delete navMeshGenerator;
    }

    void AIManager::updateAI(std::shared_ptr<AIWorld> aiWorld)
    {
        navMeshGenerator->generate(std::move(aiWorld));
    }

    NavMeshGenerator *AIManager::getNavMeshGenerator() const
    {
        return navMeshGenerator;
    }

    std::vector<Point3<float>> AIManager::computePath(const Point3<float> &startPoint, const Point3<float> &endPoint) const
    {
        AStar aStar(navMeshGenerator->getNavMesh());
        return aStar.findPath(Point3<float>(-55.0, 0.0, 0.0), Point3<float>(55.0, 0.0, 0.0));
    }
}
