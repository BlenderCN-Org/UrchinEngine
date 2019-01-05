#include <limits>
#include <cmath>
#include <cassert>
#include <utility>

#include "CollisionHeightfieldShape.h"

namespace urchin
{

    CollisionHeightfieldShape::CollisionHeightfieldShape(std::vector<Point3<float>> vertices, unsigned int xLength, unsigned int zLength) :
            CollisionShape3D(),
            vertices(std::move(vertices)),
            xLength(xLength),
            zLength(zLength)
    {
        assert(this->vertices.size()==xLength*zLength);
        localAABBox = buildLocalAABBox();

        unsigned int trianglesPoolSize = ConfigService::instance()->getUnsignedIntValue("collisionShape.heightfieldTrianglesPoolSize");
        triangleShapesPool = new FixedSizePool<TriangleShape3D<float>>("triangleShapesPool", sizeof(TriangleShape3D<float>), trianglesPoolSize);
        collisionTriangleObjectsPool = new FixedSizePool<CollisionTriangleObject>("collisionTriangleObjectsPool", sizeof(CollisionTriangleObject), trianglesPoolSize);
    }

    CollisionHeightfieldShape::~CollisionHeightfieldShape()
    {
        trianglesInAABBox.clear();
        delete triangleShapesPool;
        delete collisionTriangleObjectsPool;
    }

    std::unique_ptr<BoxShape<float>> CollisionHeightfieldShape::buildLocalAABBox() const
    {
        Point3<float> min(vertices[0].X, std::numeric_limits<float>::max(), vertices[0].Z);
        Point3<float> max(vertices[xLength-1].X, -std::numeric_limits<float>::max(), vertices[vertices.size()-1].Z);

        for (const auto &vertex : vertices)
        {
            if(min.Y > vertex.Y)
            {
                min.Y = vertex.Y;
            }
            if(max.Y < vertex.Y)
            {
                max.Y = vertex.Y;
            }
        }

        //center on Y axis
        float maxAbsoluteYValue = std::max(std::abs(min.Y), std::abs(max.Y));
        min.Y = -maxAbsoluteYValue;
        max.Y = maxAbsoluteYValue;

        #ifdef _DEBUG //check BoxShape is centered
            assert(std::abs(std::abs(min.X)-max.X) < 0.01);
            assert(std::abs(std::abs(min.Y)-max.Y) < 0.01);
            assert(std::abs(std::abs(min.Z)-max.Z) < 0.01);
        #endif

        Vector3<float> halfSizes((max.X-min.X) / 2.0, (max.Y-min.Y) / 2.0, (max.Z-min.Z) / 2.0);
        return std::make_unique<BoxShape<float>>(halfSizes);
    }

    CollisionShape3D::ShapeType CollisionHeightfieldShape::getShapeType() const
    {
        return CollisionHeightfieldShape::HEIGHTFIELD_SHAPE;
    }

    const ConvexShape3D<float> *CollisionHeightfieldShape::getSingleShape() const
    {
        throw std::runtime_error("Impossible to retrieve single convex shape for heightfield shape");
    }

    std::shared_ptr<CollisionShape3D> CollisionHeightfieldShape::scale(float scale) const
    {
        if((scale - std::numeric_limits<float>::min()) > 1.0 || (scale + std::numeric_limits<float>::min()) < 0.0)
        {
            throw std::runtime_error("Scaling a heightfield shape is currently not supported");
        }

        return std::make_shared<CollisionHeightfieldShape>(vertices, xLength, zLength);
    }

    const std::vector<Point3<float>> &CollisionHeightfieldShape::getVertices() const
    {
        return vertices;
    }

    unsigned int CollisionHeightfieldShape::getXLength() const
    {
        return xLength;
    }

    unsigned int CollisionHeightfieldShape::getZLength() const
    {
        return zLength;
    }

    AABBox<float> CollisionHeightfieldShape::toAABBox(const PhysicsTransform &physicsTransform) const
    {
        if(!lastTransform.equals(physicsTransform))
        {
            const Matrix3<float> &orientation = physicsTransform.retrieveOrientationMatrix();
            Point3<float> extend(
                    localAABBox->getHalfSize(0) * std::abs(orientation(0)) + localAABBox->getHalfSize(1) * std::abs(orientation(3)) + localAABBox->getHalfSize(2) * std::abs(orientation(6)),
                    localAABBox->getHalfSize(0) * std::abs(orientation(1)) + localAABBox->getHalfSize(1) * std::abs(orientation(4)) + localAABBox->getHalfSize(2) * std::abs(orientation(7)),
                    localAABBox->getHalfSize(0) * std::abs(orientation(2)) + localAABBox->getHalfSize(1) * std::abs(orientation(5)) + localAABBox->getHalfSize(2) * std::abs(orientation(8))
            );

            const Point3<float> &position = physicsTransform.getPosition();

            lastAABBox = AABBox<float>(position - extend, position + extend);
            lastTransform = physicsTransform;
        }

        return lastAABBox;
    }

    CollisionConvexObject3D *CollisionHeightfieldShape::toConvexObject(const PhysicsTransform &physicsTransform) const
    {
        throw std::runtime_error("Impossible to transform heightfield shape to convex object");
    }

    Vector3<float> CollisionHeightfieldShape::computeLocalInertia(float mass) const
    {
        AABBox<float> aabbox = toAABBox(PhysicsTransform());
        float width = 2.0f * aabbox.getHalfSize(0);
        float height = 2.0f * aabbox.getHalfSize(1);
        float depth = 2.0f * aabbox.getHalfSize(2);

        float localInertia1 = (1.0f/12.0f) * mass * (height*height + depth*depth);
        float localInertia2 = (1.0f/12.0f) * mass * (width*width + depth*depth);
        float localInertia3 = (1.0f/12.0f) * mass * (width*width + height*height);
        return Vector3<float>(localInertia1, localInertia2, localInertia3);
    }

    float CollisionHeightfieldShape::getMaxDistanceToCenter() const
    {
        throw std::runtime_error("Impossible to get max distance to center for heightfield shape. A heightfield body must be static.");
    }

    float CollisionHeightfieldShape::getMinDistanceToCenter() const
    {
        return 0.0f;
    }

    CollisionShape3D *CollisionHeightfieldShape::clone() const
    {
        return new CollisionHeightfieldShape(vertices, xLength, zLength);
    }

    const std::vector<CollisionTriangleShape> &CollisionHeightfieldShape::findTrianglesInAABBox(const AABBox<float> &checkAABBox) const
    {
        trianglesInAABBox.clear();

        auto vertexXRange = computeStartEndIndices(checkAABBox.getMin().X, checkAABBox.getMax().X, Axis::X);
        auto vertexZRange = computeStartEndIndices(checkAABBox.getMin().Z, checkAABBox.getMax().Z, Axis::Z);

        for(unsigned int z = vertexZRange.first; z < vertexZRange.second; ++z)
        {
            for (unsigned int x = vertexXRange.first; x < vertexXRange.second; ++x)
            {
                createTrianglesMatchHeight(x, z, checkAABBox.getMin().Y, checkAABBox.getMax().Y);
            }
        }

        return trianglesInAABBox;
    }

    const std::vector<CollisionTriangleShape> &CollisionHeightfieldShape::findTrianglesInLineSegment(const LineSegment3D<float> &ray) const
    {
        trianglesInAABBox.clear();

        bool raySameXValues = ray.getA().X == ray.getB().X;
        bool raySameZValues = ray.getA().Z == ray.getB().Z;

        //2d line equation variables
        float slopeLineZX = (ray.getB().X - ray.getA().X) / (ray.getB().Z - ray.getA().Z);
        float slopeLineXY = (ray.getB().Y - ray.getA().Y) / (ray.getB().X - ray.getA().X);
        float interceptLineZX = ray.getA().X - (slopeLineZX * ray.getA().Z);
        float interceptLineXY = ray.getA().Y - (slopeLineXY * ray.getA().X);

        float rayMinZ = std::min(ray.getA().Z, ray.getB().Z);
        float rayMaxZ = std::max(ray.getA().Z, ray.getB().Z);
        auto vertexZRange = computeStartEndIndices(rayMinZ, rayMaxZ, Axis::Z);

        for(unsigned int z = vertexZRange.first; z < vertexZRange.second; ++z)
        {
            float zStartValue = vertices[xLength * z].Z;
            float zEndValue = vertices[xLength * (z + 1)].Z;
            float xStartValue = raySameZValues ? ray.getA().X : slopeLineZX * zStartValue + interceptLineZX;
            float xEndValue = raySameZValues ? ray.getB().X : slopeLineZX * zEndValue + interceptLineZX;
            if(xStartValue > xEndValue)
            {
                std::swap(xStartValue, xEndValue);
            }

            auto vertexXRange = computeStartEndIndices(xStartValue, xEndValue, Axis::X);

            for (unsigned int x = vertexXRange.first; x < vertexXRange.second; ++x)
            {
                float xCurrentValue = vertices[x].X;
                float xNextValue = vertices[x + 1].X;
                float rayMinY = raySameXValues ? ray.getA().Y : slopeLineXY * xCurrentValue + interceptLineXY;
                float rayMaxY = raySameXValues ? ray.getB().Y : slopeLineXY * xNextValue + interceptLineXY;
                if(rayMinY > rayMaxY)
                {
                    std::swap(rayMinY, rayMaxY);
                }

                createTrianglesMatchHeight(x, z, rayMinY, rayMaxY);
            }
        }

        return trianglesInAABBox;
    }

    /**
     * @param minValue Lower bound value on X (or Z) axis
     * @param maxValue Upper bound value on X (or Z) axis
     */
    std::pair<unsigned int, unsigned int> CollisionHeightfieldShape::computeStartEndIndices(float minValue, float maxValue, Axis axis) const
    {
        float halfSize = axis==Axis::X ? localAABBox->getHalfSizes().X : localAABBox->getHalfSizes().Z;
        float verticesDistance = axis==Axis::X ? vertices[1].X - vertices[0].X : vertices[xLength].Z - vertices[0].Z;
        int maxLength = axis==Axis::X ? static_cast<int>(xLength-1) : static_cast<int>(zLength-1);

        auto rawStartVertex = static_cast<int>((minValue + halfSize) / verticesDistance);
        auto startVertex = static_cast<unsigned int>(MathAlgorithm::clamp(rawStartVertex, 0, maxLength));

        auto rawEndVertex = static_cast<int>((maxValue + halfSize) / verticesDistance) + 1;
        auto endVertex = static_cast<unsigned int>(MathAlgorithm::clamp(rawEndVertex, 0, maxLength));

        return std::make_pair(startVertex, endVertex);
    }

    void CollisionHeightfieldShape::createTrianglesMatchHeight(unsigned int x, unsigned int z, float minY, float maxY) const
    {
        Point3<float> point1 = vertices[x + xLength * z]; //far-left
        Point3<float> point2 = vertices[x + 1 + xLength * z]; //far-right
        Point3<float> point3 = vertices[x + xLength * (z + 1)]; //near-left
        Point3<float> point4 = vertices[x + 1 + xLength * (z + 1)]; //near-right

        bool hasDiagonalPointAbove = point2.Y > minY || point3.Y > minY;
        bool hasDiagonalPointBelow = point2.Y < maxY || point3.Y < maxY;

        if( (point1.Y > minY || hasDiagonalPointAbove) && (point1.Y < maxY || hasDiagonalPointBelow) )
        {
            createCollisionTriangleShape(point1, point3, point2);
        }

        if( (point4.Y > minY || hasDiagonalPointAbove) && (point4.Y < maxY || hasDiagonalPointBelow) )
        {
            createCollisionTriangleShape(point2, point3, point4);
        }
    }

    void CollisionHeightfieldShape::createCollisionTriangleShape(const Point3<float> &p1, const Point3<float> &p2, const Point3<float> &p3) const
    {
        void *shapeMemPtr = triangleShapesPool->allocate();
        trianglesInAABBox.emplace_back(CollisionTriangleShape(new (shapeMemPtr) TriangleShape3D<float>(p1, p2, p3), triangleShapesPool));
        trianglesInAABBox.back().setupConvexObjectPool(collisionTriangleObjectsPool);
    }

}
