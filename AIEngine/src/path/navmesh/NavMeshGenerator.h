#ifndef URCHINENGINE_NAVMESHGENERATOR_H
#define URCHINENGINE_NAVMESHGENERATOR_H

#include <memory>
#include <vector>
#include <mutex>
#include <map>
#include "UrchinCommon.h"

#include "input/AIWorld.h"
#include "input/AIObject.h"
#include "path/navmesh/model/NavMeshConfig.h"
#include "path/navmesh/model/NavMesh.h"
#include "path/navmesh/model/NavPolygon.h"
#include "path/navmesh/polyhedron/Polyhedron.h"
#include "path/navmesh/polyhedron/PolyhedronFace.h"
#include "path/navmesh/csg/CSGPolygon.h"
#include "path/navmesh/triangulation/TriangulationAlgorithm.h"

namespace urchin
{

	typedef std::map<std::shared_ptr<AIObject>, std::unique_ptr<Polyhedron>>::const_iterator it_polyhedron;

	struct PolyhedronFaceIndex
	{
		PolyhedronFaceIndex(it_polyhedron, unsigned int);

		it_polyhedron polyhedronRef;
		unsigned int faceIndex;
	};

	class NavMeshGenerator
	{
		public:
            NavMeshGenerator();

			void setNavMeshConfig(std::shared_ptr<NavMeshConfig>);

			std::shared_ptr<NavMesh> generate(AIWorld &);
			NavMesh retrieveLastGeneratedNavMesh() const;

		private:
			void updateExpandedPolyhedrons(AIWorld &);

			std::vector<PolyhedronFaceIndex> findWalkableFaces() const;

			std::vector<std::shared_ptr<NavPolygon>> createNavigationPolygonFor(const PolyhedronFaceIndex &) const;
			std::vector<Point2<float>> reversePoints(const std::vector<Point2<float>> &) const;
			std::vector<Point2<float>> reverseAndFlatPointsOnYAxis(const std::vector<Point3<float>> &) const;
			std::vector<CSGPolygon<float>> computeObstacles(const PolyhedronFaceIndex &) const;
			CSGPolygon<float> computePolyhedronFootprint(const std::unique_ptr<Polyhedron> &, const PolyhedronFace &) const;
			std::vector<Point3<float>> elevateTriangulatedPoints(const TriangulationAlgorithm &, const PolyhedronFace &) const;
			Point3<float> elevatePoints(const Point2<float> &, float, const PolyhedronFace &) const;

			const float polygonMinDotProductThreshold;
			const float polygonMergePointsDistanceThreshold;

            mutable std::mutex navMeshMutex;
			std::shared_ptr<NavMeshConfig> navMeshConfig;
			std::multimap<std::shared_ptr<AIObject>, std::unique_ptr<Polyhedron>> expandedPolyhedrons;

			std::shared_ptr<NavMesh> navMesh;
	};

}

#endif
