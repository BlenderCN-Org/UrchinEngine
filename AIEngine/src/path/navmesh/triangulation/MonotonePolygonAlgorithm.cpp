#include <utility>
#include <algorithm>
#include <sstream>

#include "MonotonePolygonAlgorithm.h"

namespace urchin
{

	TypedPoint::TypedPoint(unsigned int pointIndex, PointType type) :
			pointIndex(pointIndex), type(type)
	{

	}

	Edge::Edge(unsigned int startIndex, unsigned int endIndex) :
			startIndex(startIndex), endIndex(endIndex), isProcessed(false)
	{

	}

	EdgeHelper:: EdgeHelper(Edge edge, unsigned int helperPointIndex, PointType helperPointType) :
			edge(edge), helperPointIndex(helperPointIndex), helperPointType(helperPointType)
	{

	}

	MonotonePolygonError::MonotonePolygonError(const std::string &message) :
			std::runtime_error(message)
	{

	}

	/**
	 * @param polygonPoints Polygon points are in CCW order and holes in CW order.
	 * @param endContourIndices Delimiter between polygon points and holes points.
	 */
	MonotonePolygonAlgorithm::MonotonePolygonAlgorithm(const std::vector<Point2<float>> &polygonPoints, const std::vector<unsigned int> &endContourIndices,
                                                       const std::vector<std::string> &contourNames) :
			polygonPoints(polygonPoints),
			endContourIndices(endContourIndices),
            contourNames(contourNames)
	{
		edgeHelpers.reserve(5);

		#ifdef _DEBUG
//			logInputData("Debug monotone polygon.", Logger::INFO);
		#endif
	}

	/**
	 * Create Y-monotone polygons.
	 * Y-monotone polygon: any lines on X-axis should intersect the polygon once (point/line) or not at all.
	 */
	std::vector<MonotonePolygon> MonotonePolygonAlgorithm::createYMonotonePolygons()
	{
		yMonotonePolygons.clear();

		try
		{
			createYMonotonePolygonsDiagonals();
		}catch(MonotonePolygonError &error)
		{
			logInputData(std::string(error.what()), Logger::ERROR);
			return yMonotonePolygons;
		}

		if(diagonals.empty())
		{
			std::vector<unsigned int> monotonePointsIndices;
			monotonePointsIndices.reserve(polygonPoints.size());
			for(unsigned int i=0; i<polygonPoints.size(); ++i)
			{
				monotonePointsIndices.push_back(i);
			}
			MonotonePolygon monotonePolygon;
			monotonePolygon.setCcwPoints(monotonePointsIndices);
			yMonotonePolygons.emplace_back(monotonePolygon);
		}else
		{
			yMonotonePolygons.reserve(diagonals.size());
			for(auto itDiagonal = diagonals.begin(); itDiagonal!=diagonals.end(); ++itDiagonal)
			{
				Edge &startDiagonal = itDiagonal->second;
				if(!startDiagonal.isProcessed)
				{
					unsigned int yMonotonePolygonsIndex = yMonotonePolygons.size();
					MonotonePolygon monotonePolygon;
					yMonotonePolygons.emplace_back(monotonePolygon);

					std::vector<unsigned int> monotonePointsIndices;
					monotonePointsIndices.reserve(polygonPoints.size()/2 + 1); //estimated memory size
					monotonePointsIndices.push_back(startDiagonal.startIndex);
					monotonePointsIndices.push_back(startDiagonal.endIndex);

					unsigned int previousPointIndex = startDiagonal.startIndex;
					unsigned int currentPointIndex = startDiagonal.endIndex;

					while(true)
					{
						unsigned int nextPointIndex = retrieveNextPointIndex(previousPointIndex, currentPointIndex, yMonotonePolygonsIndex);
						if(nextPointIndex==startDiagonal.startIndex)
						{
							break;
						}

						#ifdef _DEBUG
                            //check no duplicate points inserted in monotone
							for(unsigned int monotonePointsIndex: monotonePointsIndices)
							{
								if(nextPointIndex==monotonePointsIndex)
								{
									std::stringstream logStream;
                                    logStream.precision(std::numeric_limits<float>::max_digits10);
									logStream<<"Duplicate point ("<<polygonPoints[nextPointIndex]<<") inserted in monotone polygon: ";

									logInputData(logStream.str(), Logger::ERROR);
								}
							}
						#endif
						monotonePointsIndices.push_back(nextPointIndex);

						previousPointIndex = currentPointIndex;
						currentPointIndex = nextPointIndex;

						if(monotonePointsIndices.size() > polygonPoints.size())
						{
							logInputData("Impossible to close monotone polygon.", Logger::ERROR);
							yMonotonePolygons.clear();
							return yMonotonePolygons;
						}

						markDiagonalProcessed(itDiagonal, yMonotonePolygonsIndex);
					}

					yMonotonePolygons[yMonotonePolygonsIndex].setCcwPoints(monotonePointsIndices);
				}
			}
		}

		#ifdef _DEBUG
//			logOutputData("Debug monotone polygon.", yMonotonePolygons, Logger::INFO);
		#endif

		return yMonotonePolygons;
	}

	void MonotonePolygonAlgorithm::createYMonotonePolygonsDiagonals()
	{
		edgeHelpers.clear();
		diagonals.clear();

		bool isMonotonePolygon;
		std::vector<TypedPoint> sortedTypedPoints = buildSortedTypedPoints(isMonotonePolygon);
		if(isMonotonePolygon)
		{ //polygon is already monotone: no diagonal to create
			return;
		}

		for(const auto &typedPoint : sortedTypedPoints)
		{
			switch(typedPoint.type)
			{
				case PointType::START_VERTEX:
					handleStartVertex(typedPoint.pointIndex);
					break;
				case PointType::SPLIT_VERTEX:
					handleSplitVertex(typedPoint.pointIndex);
					break;
				case PointType::END_VERTEX:
					handleEndVertex(typedPoint.pointIndex);
					break;
				case PointType::MERGE_VERTEX:
					handleMergeVertex(typedPoint.pointIndex);
					break;
				case PointType::REGULAR_DOWN_VERTEX:
					handleRegularDownVertex(typedPoint.pointIndex);
					break;
				case PointType::REGULAR_UP_VERTEX:
					handleRegularUpVertex(typedPoint.pointIndex);
					break;
				default:
					throw std::runtime_error("Unknown type of the point: " + std::to_string(typedPoint.type));
			}
		}
	}

	/**
	 * @param isMonotonePolygon [out] Returns true if polygon is already monotone
	 */
	std::vector<TypedPoint> MonotonePolygonAlgorithm::buildSortedTypedPoints(bool &isMonotonePolygon) const
	{
		std::vector<TypedPoint> sortedTypedPoints;
		sortedTypedPoints.reserve(polygonPoints.size());

		isMonotonePolygon = true;
		for(unsigned int i=0; i<polygonPoints.size(); ++i)
		{
			PointType pointType;
			unsigned int previousIndex = previousPointIndex(i);
			unsigned int nextIndex = nextPointIndex(i);

			bool currentAbovePrevious = isFirstPointAboveSecond(i, previousIndex);
			bool currentAboveNext = isFirstPointAboveSecond(i, nextIndex);

			if(currentAbovePrevious && currentAboveNext)
			{
				Vector2<float> previousToOrigin = polygonPoints[previousIndex].vector(polygonPoints[i]);
				Vector2<float> originToNext = polygonPoints[i].vector(polygonPoints[nextIndex]);
				float orientationResult = previousToOrigin.crossProduct(originToNext);

				if(orientationResult>=0.0)
				{
					pointType = PointType::START_VERTEX;
				}else
				{
					pointType = PointType::SPLIT_VERTEX;
					isMonotonePolygon = false;
				}
			}else if(!currentAbovePrevious && !currentAboveNext)
			{
				Vector2<float> previousToOrigin = polygonPoints[previousIndex].vector(polygonPoints[i]);
				Vector2<float> originToNext = polygonPoints[i].vector(polygonPoints[nextIndex]);
				float orientationResult = previousToOrigin.crossProduct(originToNext);

				if(orientationResult>=0.0)
				{
					pointType = PointType::END_VERTEX;
				}else
				{
					pointType = PointType::MERGE_VERTEX;
					isMonotonePolygon = false;
				}
			}else if(!currentAbovePrevious)
			{
				pointType = PointType::REGULAR_DOWN_VERTEX;
			}else
			{
				pointType = PointType::REGULAR_UP_VERTEX;
			}

			sortedTypedPoints.emplace_back(TypedPoint(i, pointType));
		}

		std::sort(sortedTypedPoints.begin(), sortedTypedPoints.end(), [&](const TypedPoint &left, const TypedPoint &right)
						{return isFirstPointAboveSecond(left.pointIndex, right.pointIndex);});

		if(!sortedTypedPoints.empty() && sortedTypedPoints[0].type!=PointType::START_VERTEX)
		{
			throw MonotonePolygonError("First point in the vector should be a start vertex. Point type: " + std::to_string(sortedTypedPoints[0].type));
		}

		return sortedTypedPoints;
	}

	bool MonotonePolygonAlgorithm::isFirstPointAboveSecond(unsigned int firstIndex, unsigned int secondIndex) const
	{
		if(polygonPoints[firstIndex].Y == polygonPoints[secondIndex].Y)
		{
			return polygonPoints[firstIndex].X < polygonPoints[secondIndex].X;
		}
		return polygonPoints[firstIndex].Y > polygonPoints[secondIndex].Y;
	}

	void MonotonePolygonAlgorithm::handleStartVertex(unsigned int i)
	{
		createEdgeHelper(i, i, PointType::START_VERTEX);
	}

	void MonotonePolygonAlgorithm::handleSplitVertex(unsigned int i)
	{
		auto edgeHelperIt = findNearestLeftEdgeHelper(i);

		createDiagonals(i, edgeHelperIt->helperPointIndex);

		edgeHelperIt->helperPointIndex = i;
		edgeHelperIt->helperPointType = PointType::SPLIT_VERTEX;

		createEdgeHelper(i, i, PointType::SPLIT_VERTEX);
	}

	void MonotonePolygonAlgorithm::handleEndVertex(unsigned int i)
	{
		unsigned int previousEdgeIndex = previousPointIndex(i);
		auto edgeHelperIt = findEdgeHelper(previousEdgeIndex);
		if(edgeHelperIt->helperPointType==PointType::MERGE_VERTEX)
		{
			createDiagonals(i, edgeHelperIt->helperPointIndex);
		}
		VectorEraser::erase(edgeHelpers, edgeHelperIt);
	}

	void MonotonePolygonAlgorithm::handleMergeVertex(unsigned int i)
	{
		unsigned int previousEdgeIndex = previousPointIndex(i);
		auto edgeHelperIt = findEdgeHelper(previousEdgeIndex);
		if(edgeHelperIt->helperPointType==PointType::MERGE_VERTEX)
		{
			createDiagonals(i, edgeHelperIt->helperPointIndex);
		}
		VectorEraser::erase(edgeHelpers, edgeHelperIt);

		auto nearestLeftEdgeHelperIt = findNearestLeftEdgeHelper(i);
		if(nearestLeftEdgeHelperIt->helperPointType==PointType::MERGE_VERTEX)
		{
			createDiagonals(i, nearestLeftEdgeHelperIt->helperPointIndex);
		}
		nearestLeftEdgeHelperIt->helperPointIndex = i;
		nearestLeftEdgeHelperIt->helperPointType = PointType::MERGE_VERTEX;
	}

	void MonotonePolygonAlgorithm::handleRegularDownVertex(unsigned int i)
	{
		unsigned int previousEdgeIndex = previousPointIndex(i);
		auto edgeHelperIt = findEdgeHelper(previousEdgeIndex);
		if(edgeHelperIt->helperPointType==PointType::MERGE_VERTEX)
		{
			createDiagonals(i, edgeHelperIt->helperPointIndex);
		}
		VectorEraser::erase(edgeHelpers, edgeHelperIt);

		createEdgeHelper(i, i, PointType::REGULAR_DOWN_VERTEX);
	}

	void MonotonePolygonAlgorithm::handleRegularUpVertex(unsigned int i)
	{
		auto edgeHelperIt = findNearestLeftEdgeHelper(i);
		if(edgeHelperIt->helperPointType==PointType::MERGE_VERTEX)
		{
			createDiagonals(i, edgeHelperIt->helperPointIndex);
		}
		edgeHelperIt->helperPointIndex = i;
		edgeHelperIt->helperPointType = PointType::REGULAR_UP_VERTEX;
	}

	unsigned int MonotonePolygonAlgorithm::nextPointIndex(unsigned int pointIndex) const
	{
		unsigned int nextPointIndex = pointIndex + 1;

		auto it = std::find(endContourIndices.begin(), endContourIndices.end(), nextPointIndex);
		if(it==endContourIndices.begin())
		{
			nextPointIndex = 0;
		}else if(it!=endContourIndices.end())
		{
			nextPointIndex = *(--it);
		}

		return nextPointIndex;
	}

	unsigned int MonotonePolygonAlgorithm::previousPointIndex(unsigned int pointIndex) const
	{
		unsigned int previousPointIndex = pointIndex - 1;
		if(pointIndex==0)
		{
			previousPointIndex = endContourIndices[0] - 1;
		}else
		{
			auto it = std::find(endContourIndices.begin(), endContourIndices.end(), pointIndex);
			if(it!=endContourIndices.end())
			{
				previousPointIndex = (*(++it)) - 1;
			}
		}

		return previousPointIndex;
	}

	void MonotonePolygonAlgorithm::createEdgeHelper(unsigned int edgeStartIndex, unsigned int helperPointIndex, PointType pointType)
	{
		Edge edge(edgeStartIndex, nextPointIndex(edgeStartIndex));
		edgeHelpers.emplace_back(EdgeHelper(edge, helperPointIndex, pointType));
	}

	std::vector<EdgeHelper>::iterator MonotonePolygonAlgorithm::findEdgeHelper(unsigned int edgeIndex)
	{
		for(auto it=edgeHelpers.begin(); it!=edgeHelpers.end(); ++it)
		{
			if(it->edge.startIndex==edgeIndex)
			{
				return it;
			}
		}

		throw MonotonePolygonError("Impossible to find edge and his helper for edge index: " + std::to_string(edgeIndex));
	}

	std::vector<EdgeHelper>::iterator MonotonePolygonAlgorithm::findNearestLeftEdgeHelper(unsigned int pointIndex)
	{
		Point2<float> point = polygonPoints[pointIndex];

		float nearestDistance = -std::numeric_limits<float>::max();
		auto nearestLeftEdgeHelperIt = edgeHelpers.end();

		for(auto it=edgeHelpers.begin(); it!=edgeHelpers.end(); ++it)
		{
			Line2D<double> edge(polygonPoints[it->edge.startIndex].template cast<double>(), polygonPoints[it->edge.endIndex].template cast<double>());

			double edgeHorizontalDistanceToPoint = edge.horizontalDistance(point.template cast<double>());
			bool isEdgeOnLeftOfPoint = edgeHorizontalDistanceToPoint < 0.0;
			if(isEdgeOnLeftOfPoint && edgeHorizontalDistanceToPoint > nearestDistance)
			{
				nearestDistance = edgeHorizontalDistanceToPoint;
				nearestLeftEdgeHelperIt = it;
			}
		}

		if(nearestLeftEdgeHelperIt==edgeHelpers.end())
		{
			throw MonotonePolygonError("Impossible to find edge on left for point index: " + std::to_string(pointIndex));
		}

		return nearestLeftEdgeHelperIt;
	}

	void MonotonePolygonAlgorithm::createDiagonals(unsigned int index1, unsigned int index2)
	{
		diagonals.insert(std::make_pair(index1, Edge(index1, index2)));
		diagonals.insert(std::make_pair(index2, Edge(index2, index1)));
	}

	/**
	 * Returns the next point after edge [edgeStartIndex, edgeEndIndex] in order to form a monotone polygon.
	 * If edge [edgeEndIndex, nextPointIndex] is a diagonal: mark the diagonal as processed.
	 */
	unsigned int MonotonePolygonAlgorithm::retrieveNextPointIndex(unsigned int edgeStartIndex, unsigned int edgeEndIndex, unsigned int yMonotonePolygonsIndex)
	{
		std::vector<std::pair<unsigned int, it_diagonals>> possibleNextPoints = retrievePossibleNextPoints(edgeEndIndex);
		if(possibleNextPoints.size()==1)
		{ //only one possible edge
			markDiagonalProcessed(possibleNextPoints[0].second, yMonotonePolygonsIndex);
			return possibleNextPoints[0].first;
		}

		int bestCCWPointIndex = -1;
		int bestCWPointIndex = -1;
		double minAngleCCW = std::numeric_limits<double>::max();
		double maxAngleCW = -std::numeric_limits<double>::max();

		Vector2<double> edgeVector = polygonPoints[edgeStartIndex].template cast<double>().vector(polygonPoints[edgeEndIndex].template cast<double>());
		for(unsigned int i=0; i<possibleNextPoints.size(); ++i)
		{
			unsigned int testPointIndex = possibleNextPoints[i].first;
			Vector2<double> nextEdgeVector = polygonPoints[edgeEndIndex].template cast<double>().vector(polygonPoints[testPointIndex].template cast<double>());
			double orientationResult = edgeVector.crossProduct(nextEdgeVector);
			double angle = edgeVector.normalize().dotProduct(nextEdgeVector.normalize());

			if(orientationResult > 0.0)
			{ //counter clockwise
				if(angle < minAngleCCW)
				{
					minAngleCCW = angle;
					bestCCWPointIndex = i;
				}
			}else
			{ //clockwise
				if(angle > maxAngleCW)
				{
					maxAngleCW = angle;
					bestCWPointIndex = i;
				}
			}
		}

		auto nextPoint = possibleNextPoints[bestCCWPointIndex!=-1 ? bestCCWPointIndex : bestCWPointIndex];
		markDiagonalProcessed(nextPoint.second, yMonotonePolygonsIndex);

		return nextPoint.first;
	}

	std::vector<std::pair<unsigned int, MonotonePolygonAlgorithm::it_diagonals>> MonotonePolygonAlgorithm::retrievePossibleNextPoints(unsigned int edgeEndIndex)
	{
		std::vector<std::pair<unsigned int, it_diagonals>> possibleNextPoints;
		possibleNextPoints.reserve(diagonals.size());

		unsigned int nextPolygonPointIndex = nextPointIndex(edgeEndIndex);
		possibleNextPoints.emplace_back(std::make_pair(nextPolygonPointIndex, diagonals.end()));

		auto range = diagonals.equal_range(edgeEndIndex);
		for(auto it = range.first; it != range.second; ++it)
		{
			if(!it->second.isProcessed)
			{
				possibleNextPoints.emplace_back(std::make_pair(it->second.endIndex, it));
			}
		}

		return possibleNextPoints;
	}

	void MonotonePolygonAlgorithm::markDiagonalProcessed(MonotonePolygonAlgorithm::it_diagonals itDiagonal, unsigned int yMonotonePolygonsIndex)
	{
		if(itDiagonal!=diagonals.end())
		{
			itDiagonal->second.isProcessed = true;
			yMonotonePolygons[yMonotonePolygonsIndex].addSharedEdge(itDiagonal->second.startIndex, itDiagonal->second.endIndex);
		}
	}

	void MonotonePolygonAlgorithm::logInputData(const std::string &message, Logger::CriticalityLevel logLevel) const
	{
		std::stringstream logStream;
		logStream.precision(std::numeric_limits<float>::max_digits10);

        unsigned int contourIndex = 0;
		logStream<<message<<std::endl;
		logStream<<"Monotone polygon input data:"<<std::endl;
        logStream<<"\tPoints ("<<contourNames[contourIndex++]<<"):"<<std::endl;
		for(unsigned int i=0; i<polygonPoints.size(); ++i)
		{
			logStream<<"\t\t"<<polygonPoints[i]<<std::endl;

			if((i+1)!=polygonPoints.size() && std::find(endContourIndices.begin(), endContourIndices.end(), i+1) != endContourIndices.end())
			{
				logStream<<"\tHole ("<<contourNames[contourIndex++]<<"):"<<std::endl;
			}
		}
		Logger::logger().log(logLevel, logStream.str());

        #ifdef _DEBUG
//            exportSVG(std::string(std::getenv("HOME")) + "/monotonePoints.html");
        #endif
	}

    void MonotonePolygonAlgorithm::exportSVG(const std::string &filename) const
    {
        SVGExporter svgExporter(filename);

        std::vector<Point2<float>> svgPolygonPoints;
        std::copy(polygonPoints.begin(), polygonPoints.begin() + endContourIndices[0], std::back_inserter(svgPolygonPoints));
        svgExporter.addShape(new SVGPolygon(svgPolygonPoints, SVGPolygon::LIME));

        for(unsigned int i=0; i<endContourIndices.size()-1; ++i)
        {
            std::vector<Point2<float>> svgHolePoints;
            std::copy(polygonPoints.begin()+endContourIndices[i], polygonPoints.begin() + endContourIndices[i + 1], std::back_inserter(svgHolePoints));
            svgExporter.addShape(new SVGPolygon(svgHolePoints, SVGPolygon::RED, 0.5));
        }

        svgExporter.generateSVG(250);
    }

	void MonotonePolygonAlgorithm::logOutputData(const std::string &message, const std::vector<std::vector<unsigned int>> &yMonotonePolygons, Logger::CriticalityLevel logLevel) const
	{
		std::stringstream logStream;
		logStream.precision(std::numeric_limits<float>::max_digits10);

		logStream<<message<<std::endl;
		logStream<<"Monotone polygon output data:"<<std::endl;
		for(unsigned int i=0; i<yMonotonePolygons.size(); ++i)
		{
			logStream<<" - Monotone polygon "<<i<<":"<<std::endl;
			for(unsigned int pointIndex : yMonotonePolygons[i])
			{
				logStream<<" - "<<polygonPoints[pointIndex]<<std::endl;
			}
		}
		Logger::logger().log(logLevel, logStream.str());
	}

}
