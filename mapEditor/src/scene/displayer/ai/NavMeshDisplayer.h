#ifndef URCHINENGINE_MAPEDITOR_NAVMESHDISPLAYER_H
#define URCHINENGINE_MAPEDITOR_NAVMESHDISPLAYER_H

#include "Urchin3dEngine.h"
#include "UrchinMapHandler.h"
#include "UrchinAIEngine.h"

namespace urchin
{

	class NavMeshDisplayer
	{
		public:
			explicit NavMeshDisplayer(SceneManager *);
			~NavMeshDisplayer();

			void displayNavMesh(const NavMesh &);

		private:
			void cleanCurrentDisplay();
			std::vector<Point3<float>> toDisplayPoints(const std::vector<Point3<float>> &, float) const;
			std::vector<IndexedTriangle3D<float>> toDisplayTriangles(const std::vector<std::shared_ptr<NavTriangle>> &) const;

			SceneManager *sceneManager;
			std::vector<GeometryModel *> navMeshModels;

			unsigned int loadedNavMeshId;
	};

}

#endif
