# To do
- Broad phase
	- (2) **OPTIMIZATION**: Use a memory pool when create new OverlappingPair
	- (2) **OPTIMIZATION**: Implement double AABBox trees: static and dynamic (see Bullet)
	- (2) **OPTIMIZATION**: Rebalance AABBox tree for better performance
- Narrow phase
	- (2) **NEW FEATURE**: Support joints between shapes
	- (3) **OPTIMIZATION**: GJK, don't test voronoi region opposite to last point added (2D: A, B, AB | 3D: ABC)
	- (3) **OPTIMIZATION**: EPA, add new point on convex hull to improve thanks to adjacent edges: find more faster triangles visible by new point (see Reactphysics3d code)
	- (3) **OPTIMIZATION**: Use std::set#merge from C++17 in RayTestResult::addResults and NarrowPhaseManager::handleContinuousCollision
- Character
	- (2) **NEW FEATURE**: Handle stair for character controller
	- (3) **NEW FEATURE**: Apply impulse on objects hit by character

# Known bugs
- Constraint solver
	- (2) A restitution of 1.0 introduce new force in the system. Example: a superball bounces more and more higher at each collision
- Island
	- (2) A body balancing from one side to the other side (ex: cone on his base) could sleep when velocity reach zero

# Notes
- 
