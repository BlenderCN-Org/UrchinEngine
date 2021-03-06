#ifndef URCHINENGINE_AITERRAIN_H
#define URCHINENGINE_AITERRAIN_H

#include <vector>
#include "UrchinCommon.h"

#include "input/AIEntity.h"

namespace urchin
{

    class AITerrain : public AIEntity
    {
        public:
            AITerrain(std::string, const Transform<float> &, bool, const std::vector<Point3<float>> &, unsigned int, unsigned int);

            AIEntity::AIEntityType getType() const override;

            const std::vector<Point3<float>> &getVertices() const;
            unsigned int getXLength() const;
            unsigned int getZLength() const;

        private:
            std::vector<Point3<float>> vertices;
            unsigned int xLength;
            unsigned int zLength;
    };

}

#endif
