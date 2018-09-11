#include "HeightfieldPointHelper.h"

#include "math/algorithm/MathAlgorithm.h"
#include "math/geometry/2d/object/LineSegment2D.h"

namespace urchin
{

    template<class T> HeightfieldPointHelper<T>::HeightfieldPointHelper(const std::vector<Point3<T>> &heightfieldPoints, unsigned int heightfieldXSize) :
            heightfieldPoints(heightfieldPoints),
            heightfieldXSize(heightfieldXSize)
    {
        heightfieldZSize = heightfieldPoints.size() / heightfieldXSize;

        xInterval = heightfieldPoints[1].X - heightfieldPoints[0].X;
        zInterval = heightfieldPoints[heightfieldXSize].Z - heightfieldPoints[0].Z;
    }

    /**
     * @return Nearest heightfield point (3D) of the provided coordinate point (2D)
     */
    template<class T> Point3<T> HeightfieldPointHelper<T>::findPointAt(const Point2<T> &point) const
    {
        Vector2<T> farLeftToPoint = Point2<T>(heightfieldPoints[0].X, heightfieldPoints[0].Z).vector(point);
        int xIndex = MathAlgorithm::clamp(static_cast<int>(std::round(farLeftToPoint.X / xInterval)), 0, static_cast<int>(heightfieldXSize - 1));
        int zIndex = MathAlgorithm::clamp(static_cast<int>(std::round(farLeftToPoint.Y / zInterval)), 0, static_cast<int>(heightfieldZSize - 1));

        return heightfieldPoints[xIndex + zIndex * heightfieldXSize];
    }

    /**
     * @return Heightfield height of the provided coordinate point (2D)
     */
    template<class T> T HeightfieldPointHelper<T>::findHeightAt(const Point2<T> &point) const
    {
        Vector2<T> farLeftToPoint = Point2<T>(heightfieldPoints[0].X, heightfieldPoints[0].Z).vector(point);
        int xIndex = MathAlgorithm::clamp(static_cast<int>(std::round(farLeftToPoint.X / xInterval)), 0, static_cast<int>(heightfieldXSize - 1));
        int zIndex = MathAlgorithm::clamp(static_cast<int>(std::round(farLeftToPoint.Y / zInterval)), 0, static_cast<int>(heightfieldZSize - 1));

        int nearestPointIndex = xIndex + zIndex * heightfieldXSize;
        Point3<T> nearestPoint = heightfieldPoints[nearestPointIndex];

        int leftRightPointXIndex = (nearestPoint.X > point.X) ? std::max(xIndex - 1, 0) : std::min(xIndex + 1, static_cast<int>(heightfieldXSize - 1));
        int leftRightPointIndex = leftRightPointXIndex + zIndex * heightfieldXSize;
        Point3<T> leftRightPoint = heightfieldPoints[leftRightPointIndex];

        int farNearPointZIndex = (nearestPoint.Z > point.Y) ? std::max(zIndex - 1, 0) : std::min(zIndex + 1, static_cast<int>(heightfieldZSize - 1));
        int farNearPointIndex = xIndex + farNearPointZIndex * heightfieldXSize;
        Point3<T> farNearPoint = heightfieldPoints[farNearPointIndex];

        if(nearestPointIndex==leftRightPointIndex || nearestPointIndex==farNearPointIndex || leftRightPointIndex==farNearPointIndex)
        { //coordinates are outside heightfield or on edge: return approximate point
            return nearestPoint.Y;
        }

        const Point3<T> coordinatePoint3D(point.X, 0.0, point.Y);
        Vector3<T> planeNormal = leftRightPoint.vector(farNearPoint).crossProduct(leftRightPoint.vector(nearestPoint)).normalize();
        return planeNormal.dotProduct(coordinatePoint3D.vector(nearestPoint)) / planeNormal.Y;
    }

    /**
     * @return Path points which follow the topography of the terrain between start and end point
     */
    template<class T> std::vector<Point3<T>> HeightfieldPointHelper<T>::followTopography(const Point3<T> &startPoint, const Point3<T> &endPoint) const
    {
        LineSegment2D<T> pathLine(Point2<T>(startPoint.X, startPoint.Y), Point2<T>(endPoint.X, endPoint.Z));

        std::vector<Point3<T>> pathPoints; //TODO estimate reserve() memory size
        pathPoints.push_back(startPoint);

        Vector2<T> farLeftToStartPoint = Point2<T>(heightfieldPoints[0].X, heightfieldPoints[0].Z).vector(Point2<T>(startPoint.X, startPoint.Z));
        Vector2<T> farLeftToEndPoint = Point2<T>(heightfieldPoints[0].X, heightfieldPoints[0].Z).vector(Point2<T>(endPoint.X, endPoint.Z));

        //X lines collision
        int xStartIndex = MathAlgorithm::clamp(static_cast<int>(std::floor(farLeftToStartPoint.X / xInterval)), 0, static_cast<int>(heightfieldXSize - 1));
        int xEndIndex = MathAlgorithm::clamp(static_cast<int>(std::ceil(farLeftToEndPoint.X / xInterval)), 0, static_cast<int>(heightfieldXSize - 1));
        int zLastIndex = (heightfieldZSize - 1) * heightfieldXSize;
        for(int xCoord = xStartIndex; xCoord < xEndIndex; ++xCoord)
        {
            Point3<T> firstLinePoint = heightfieldPoints[xCoord];
            Point3<T> endLinePoint = heightfieldPoints[xCoord + zLastIndex];
            LineSegment2D<T> xLine(Point2<T>(firstLinePoint.X, firstLinePoint.Z), Point2<T>(endLinePoint.X, endLinePoint.Z));

            bool hasIntersection;
            Point2<T> intersectionPoint = xLine.intersectPoint(pathLine, hasIntersection);
            if(hasIntersection)
            {
                T intersectionHeight = findHeightAt(intersectionPoint);
                pathPoints.push_back(Point3<T>(intersectionPoint.X, intersectionHeight, intersectionPoint.Y));
            }
        }

        //Z lines collision
        int zStartIndex = MathAlgorithm::clamp(static_cast<int>(std::floor(farLeftToStartPoint.Y / zInterval)), 0, static_cast<int>(heightfieldZSize - 1));
        int zEndIndex = MathAlgorithm::clamp(static_cast<int>(std::ceil(farLeftToEndPoint.Y / zInterval)), 0, static_cast<int>(heightfieldZSize - 1));
        int xLastIndex = heightfieldXSize - 1;
        for(int zCoord = zStartIndex; zCoord < zEndIndex; ++zCoord)
        {
            Point3<T> firstLinePoint = heightfieldPoints[zCoord * heightfieldXSize];
            Point3<T> endLinePoint = heightfieldPoints[xLastIndex + zCoord * heightfieldXSize];
            LineSegment2D<T> zLine(Point2<T>(firstLinePoint.X, firstLinePoint.Z), Point2<T>(endLinePoint.X, endLinePoint.Z));

            bool hasIntersection; //TODO avoid code duplication
            Point2<T> intersectionPoint = zLine.intersectPoint(pathLine, hasIntersection);
            if(hasIntersection)
            {
                T intersectionHeight = findHeightAt(intersectionPoint);
      //TODO          pathPoints.push_back(Point3<T>(intersectionPoint.X, intersectionHeight, intersectionPoint.Y));
            }
        }

        //TODO sort points...

        //Oblique lines collision
        //No implemented

        pathPoints.push_back(endPoint);
        return pathPoints;
    }

    //explicit template
    template class HeightfieldPointHelper<float>;

    template class HeightfieldPointHelper<double>;
}