#include "PathPortal.h"

namespace urchin
{

    PathPortal::PathPortal(const LineSegment3D<float> &portal, const std::shared_ptr<PathNode> &previousPathNode, const std::shared_ptr<PathNode> &nextPathNode) :
        portal(portal),
        previousPathNode(previousPathNode),
        nextPathNode(nextPathNode),
        bHasPivotPoint(false)
    {

    }

    void PathPortal::setPivotPoint(const Point3<float> &pivotPoint)
    {
        this->pivotPoint = pivotPoint;
        this->bHasPivotPoint = true;
    }

    bool PathPortal::hasPivotPoint() const
    {
        return bHasPivotPoint;
    }

    /**
     * @return pivot point. Pivot point represents a change of rotation in path or a change of polygon
     */
    const Point3<float> &PathPortal::getPivotPoint() const
    {
        return pivotPoint;
    }

    const LineSegment3D<float> &PathPortal::getPortal() const
    {
        return portal;
    }

    const std::shared_ptr<PathNode> &PathPortal::getPreviousPathNode() const
    {
        return previousPathNode;
    }

    const std::shared_ptr<PathNode> &PathPortal::getNextPathNode() const
    {
        return nextPathNode;
    }

}
