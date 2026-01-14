#include "mve_physics.h"

namespace mve {
	PhysicsClass::PhysicsClass() {

	}
	PhysicsClass::~PhysicsClass() {
	}
	void PhysicsClass::step(float dt) {
		integrateForces(dt);

		broadPhase();
		
		detectCollisions();
		resolveCollisions();

		integrateVelocity(dt);

		//applySleep(dt); //sleep logic in integrate Velocity. 
	}

	void PhysicsClass::addSphereCollider(int objId, float radius){

		SphereCollider sCollider;
		sCollider.radius = 0.3f; //default radius

		int foundIndex = -1;
		for(int i = 0; i < rBodies.size(); i++) {
			if(rBodies[i].objId == objId) {
				rBodies[i].collidable = true;
				break;
			}
		}
		if (foundIndex < 0) {
			std::cout << "object ID: " << objId << " needs a rigidbody to add a collider.\n";
			return;
		}
		sCollider.bodyIndex = static_cast<uint32_t>(foundIndex);
		sphereColliders.push_back(sCollider);
	}

	void PhysicsClass::addBoxCollider(int objId, const glm::vec3& halfSize) {

		BoxCollider bCollider;
		bCollider.halfSize = halfSize; //default radius

		int foundIndex = -1;
		//std::cout << "rbodies size: " << rBodies.size() << "\n";
		for (int i = 0; i < rBodies.size(); i++) {
			if (rBodies[i].objId == objId) {
				foundIndex = i;
				rBodies[i].collidable = true;
				break;
			}
		}
		if (foundIndex < 0) {
			std::cout << "object ID: " << objId << " needs a rigidbody to add a collider.\n";
			return;
		}
		bCollider.bodyIndex = static_cast<uint32_t>(foundIndex);
		boxColliders.push_back(bCollider);
	}

	void PhysicsClass::setSpeed(int objId, const glm::vec3& speed) {

		//if rBodies is a vector:
		for (auto& body : rBodies) { //worst case: O(n), best case: O(1)
			if(body.objId == objId) {
				body.velocity = speed;
				return;
			}
		}

		//if rBodies is a map:
		//rBodies[objId].velocity = speed;
	}

	void PhysicsClass::applyForce(int objId, const glm::vec3& force) {
		for (auto& body : rBodies) { //worst case: O(n), best case: O(1)
			if (body.objId == objId) {
				body.force = force;
				return;
			}
		}
		//std::cout << "apply force on Object ID: " << objId << " failed. Object not found in physics system.\n";
	}

	void PhysicsClass::addRigidBody(MveGameObject& obj) {
		RigidBody body;
		body.objId = obj.getId();
		body.position = obj.transform.translation;
		rBodies.push_back(body);
	}

	Cell PhysicsClass::getCell(const glm::vec3& pos, float cellSize) {
		return Cell{
			static_cast<int>(std::floor(pos.x / cellSize)),
			static_cast<int>(std::floor(pos.y / cellSize)),
			static_cast<int>(std::floor(pos.z / cellSize))
		};
	}

	void PhysicsClass::buildGrid(float cellSize){
		grid.clear();
		
		for (uint32_t i = 0; i < sphereColliders.size(); i++) {
			Cell c = getCell(rBodies[sphereColliders[i].bodyIndex].position, cellSize);
			uint64_t key = hashCell(c); //simple hash function for Cell
			grid[key].push_back(i);
			//uint64_t key = mortonEncode(c);
			//grid[key].push_back(i);
		}
	}

	bool PhysicsClass::sphereSphere(const RigidBody& a, const RigidBody& b, float rA, float rB, Contact& out){
		glm::vec3 d = b.position - a.position;
		float distSq = glm::dot(d, d); //ax * bx + ay * by + az * bz
		float radiusSum = rA + rB;
		if (distSq <= radiusSum * radiusSum) {
			float dist = std::sqrt(distSq);
			out.penetration = radiusSum - dist;
			out.normal = (dist > 0) ? d / dist : glm::vec3(1, 0, 0); //if dist > 0, then d / dist, else set normal to arbitrary axis
			//out.point = a.position + out.normal * rA;
			return true;
		}
	}

	void PhysicsClass::broadPhase(){
		grid.clear();
		//debugPoints->clear();
		//building broadphase uniform grid
		for (uint32_t i = 0; i < boxColliders.size(); i++) {
			const RigidBody& body = rBodies[boxColliders[i].bodyIndex];
			AABB aabb = computeAABB(body, boxColliders[i]);

			insertCollider(i, aabb, 1.0f); //cell size of 1.0f
		}

		//generate pairs
		aabbPairs.clear();
		//Potential object collision pair generation using uniform grid: broadphase
		//worst case: O(grid size * cell.size^2)
		//Broadphase pair generation using uniform grid
		for (auto& [_, cell] : grid) { //cell is the vector in grid that contains collider indices
			for (size_t i = 0; i < cell.size(); i++) { 
				for(size_t j = i + 1; j < cell.size(); j++){
					// avoid duplicate pairs by ensuring j > i
					// emplace back constructs the pair into the vector directly
					aabbPairs.emplace_back( cell[i], cell[j] ); //cell[i] and cell[j] are collider indices
				}
			}
		}
		//std::cout << "in broadPhase, aabbPairs: " << aabbPairs.size() << " contacts: " << contacts.size() << "\n";
	}

	AABB PhysicsClass::computeAABB(const RigidBody& body, const BoxCollider& box) {
		return {
			body.position - box.halfSize,
			body.position + box.halfSize
		};
	}

	//
	void PhysicsClass::insertCollider(uint32_t colliderIndex, const AABB& aabb, float cellSize) {
		Cell min = getCell(aabb.min, cellSize);
		Cell max = getCell(aabb.max, cellSize);
		//std::cout << "max.x: " << max.x << " max.y: " << max.y << " max.z: " << max.z << "\n"; 
		for (int x = min.x; x <= max.x; x++)
			for (int y = min.y; y <= max.y; y++)
				for (int z = min.z; z <= max.z; z++)
				{
					grid[hashCell({ x, y, z })].push_back(colliderIndex);
					/*
					MveGameObject point = MveGameObject::makePointLight(0.1f);
					point.transform.translation = glm::vec3(
						(x + 0.5f) * cellSize,
						(y + 0.5f) * cellSize,
						(z + 0.5f) * cellSize
					);
					debugPoints->push_back(point);*/
				}
	}

	bool PhysicsClass::aabbIntersect(const AABB& a, const AABB& b) {
		return 
			(a.min.x <= b.max.x && a.max.x >= b.min.x) &&
			(a.min.y <= b.max.y && a.max.y >= b.min.y) &&
			(a.min.z <= b.max.z && a.max.z >= b.min.z);
	}

	//AABBAABB checks for collision between two AABBs and outputs contact info
	bool PhysicsClass::AABBAABB(const AABB& a, const AABB& b, Contact& out) {
		if (!aabbIntersect(a, b))return false;

		glm::vec3 overlap = {
			std::min(a.max.x, b.max.x) - std::max(a.min.x, b.min.x),
			std::min(a.max.y, b.max.y) - std::max(a.min.y, b.min.y),
			std::min(a.max.z, b.max.z) - std::max(a.min.z, b.min.z)
		};

		//find smallest penetration axis
		if (overlap.x < overlap.y && overlap.x < overlap.z) {

			out.normal = { (a.min.x < b.min.x) ? -1.f : 1.f, 0, 0 };
			out.penetration = overlap.x;
		}
		else if (overlap.y < overlap.x && overlap.y < overlap.z) {
			out.normal = { 0, (a.min.y < b.min.y) ? -1.f : 1.f, 0 };
			out.penetration = overlap.y;
		}
		else {
			out.normal = { 0, 0, (a.min.z < b.min.z) ? -1.f : 1.f };
			out.penetration = overlap.z;
		}
		return true;
	}


	OBB PhysicsClass::buildOBB(uint32_t colliderIndex){
		const BoxCollider& box = boxColliders[colliderIndex];
		const RigidBody& body = rBodies[box.bodyIndex];

		OBB obb;
		obb.center = body.position;
		obb.halfSize = box.halfSize;
		
		//mat3_cast converts quaternion to rotation matrix
		glm::mat3 rot = glm::mat3_cast(body.rotation);
		obb.axis[0] = rot[0]; //local x axis
		obb.axis[1] = rot[1]; //local y axis
		obb.axis[2] = rot[2]; //local z axis

		return obb;
	}

	SATResult PhysicsClass::testOBBvsOBB(const OBB& a, const OBB& b){
		float minOverlap = FLT_MAX;
		glm::vec3 bestAxis;

		//use 15 because there are 15 potential separating axes between two OBBs
		glm::vec3 axes[15]; 
		int axisCount = 0;

		//face normals
		for (int i = 0; i < 3; i++) axes[axisCount++] = a.axis[i]; 
		for (int i = 0; i < 3; i++) axes[axisCount++] = b.axis[i];

		//edge cross products
		//cross product finds a vector perpendicular to both input vectors
		for(int i = 0; i < 3; i++){
			for(int j = 0; j < 3; j++){
				glm::vec3 axis = glm::cross(a.axis[i], b.axis[j]);
				if(glm::length(axis * axis) > 1e-6f) {
					axes[axisCount++] = glm::normalize(axis);
				}
			}
		}

		glm::vec3 d = b.center - a.center;
		
		for (int i = 0; i < axisCount; i++) {
			glm::vec3 L = axes[i]; //L is the current axis being tested

			//projection radius of OBB A and B onto L

			float rA = 
				a.halfSize.x * std::abs(glm::dot(a.axis[0], L)) +
				a.halfSize.y * std::abs(glm::dot(a.axis[1], L)) +
				a.halfSize.z * std::abs(glm::dot(a.axis[2], L));

			float rB =
				b.halfSize.x * std::abs(glm::dot(b.axis[0], L)) +
				b.halfSize.y * std::abs(glm::dot(b.axis[1], L)) +
				b.halfSize.z * std::abs(glm::dot(b.axis[2], L));

			float dist = std::abs(glm::dot(d, L));
			float overlap = (rA + rB) - dist;

			if(overlap < 0){
				return { false }; //no collision
			}
			if(overlap < minOverlap){
				minOverlap = overlap;
				bestAxis = L;
			}
		}

		//ensure the normal points from A to B
		if (glm::dot(bestAxis, d) < 0){
			bestAxis = -bestAxis;
		}
		return { true, minOverlap, bestAxis };
			
	}

	void PhysicsClass::detectCollisions() {

		contacts.clear();
		/*
		//spphere collider. worst case: O(sphereColliders.size()^2)
		for (size_t i = 0; i < sphereColliders.size(); i++) {
			for (size_t j =  i + 1; j < sphereColliders.size(); j++) {
				Contact C;
				if(sphereSphere(
					rBodies[sphereColliders[i].bodyIndex], rBodies[sphereColliders[j].bodyIndex],
					sphereColliders[i].radius, sphereColliders[j].radius, C))
				{
					C.a = sphereColliders[i].bodyIndex;
					C.b = sphereColliders[j].bodyIndex;
					contacts.push_back(C);
				}

			}
		}*/

		for (auto& [iA, iB] : aabbPairs) { //if iA is aabPairs[n].first, iB is aabbPairs[n].second
			OBB A = buildOBB(iA);
			OBB B = buildOBB(iB);

			SATResult result = testOBBvsOBB(A, B);
			if (!result.hit)
				continue;
			else {
				//std::cout << "Collision detected between body " << boxColliders[iA].bodyIndex << " and body " << boxColliders[iB].bodyIndex << "\n";
			}

			Contact c;
			c.a = boxColliders[iA].bodyIndex;
			c.b = boxColliders[iB].bodyIndex;
			c.normal = result.normal;
			c.penetration = result.penetration;

			contacts.push_back(c);
		}
		//std::cout << "In detectCollision, aabbPairs: " << aabbPairs.size() << " contacts: " << contacts.size() << "\n";

	}


	void PhysicsClass::resolveCollisions() {
		const float restitution = 0.5f; //coefficient of restitution (bounciness)
		for (Contact& c : contacts) {
			RigidBody& A = rBodies[c.a];
			RigidBody& B = rBodies[c.b];

			glm::vec3 relativeVelocity = B.velocity - A.velocity;
			float velAlongNormal = glm::dot(relativeVelocity, c.normal);

			if (velAlongNormal > 0) continue; //objects are separating
			/*
			float massSum = A.mass + B.mass;
			if (massSum == 0.f) continue; //both objects are static*/

			float invMassA = (A.mass == 0.f) ? 0.f : 1.f / A.mass;
			float invMassB = (B.mass == 0.f) ? 0.f : 1.f / B.mass;
			float invMassSum = invMassA + invMassB;
			if (invMassSum == 0.f) continue; //both objects are static

			//IMPULSE application
			//Impulse is the change in momentum, and momentum is mass times velocity (p = m * v)
			float j = -(1 + restitution) * velAlongNormal;
			j /= invMassSum;
			glm::vec3 impulse = j * c.normal;
			A.velocity -= impulse * invMassA;
			B.velocity += impulse * invMassB;

			// positional correction (use invMass-based distribution)
			positionalCorrection(A, B, c);

			//this is FRICTION calculation
			glm::vec3 tangent = relativeVelocity - glm::dot(relativeVelocity, c.normal) * c.normal;
			//glm::length2 doesn't seem to be available in glm so do this instead for magnitude squared
			if (glm::dot(tangent, tangent) > 1e-8f) {
				tangent = glm::normalize(tangent);
				float jt = -glm::dot(relativeVelocity, tangent);
				jt /= invMassSum;

				float mu = 0.5f; //coefficient of friction
				//Coulomb's law of friction : frictional force between two surfaces is directly proportional to the normal force pressing them together Ff = mu * Fn
				//Ff: frictional force, mu: coefficient of friction, Fn: normal force
				jt = glm::clamp(jt, -j * mu, j * mu);
				glm::vec3 frictionImpulse = jt * tangent;
				A.velocity -= frictionImpulse * invMassA;
				B.velocity += frictionImpulse * invMassB;
			}
			
		}
	}

	void PhysicsClass::positionalCorrection(RigidBody& A, RigidBody& B, const Contact& c) {
		const float percent = 0.8f; //usually 20% to 80%
		const float slop = 0.01f; //usually 0.01 to 0.1

		float invMassA = (A.mass == 0.f) ? 0.f : 1.f / A.mass;
		float invMassB = (B.mass == 0.f) ? 0.f : 1.f / B.mass;
		float invMassSum = invMassA + invMassB;

		if (invMassSum == 0.f) return; //both objects are static

		float correctionMagnitude = percent * std::max(c.penetration - slop, 0.0f);
		glm::vec3 correction = (correctionMagnitude / invMassSum) * c.normal;

		A.position -= correction * invMassA;
		B.position += correction * invMassB;
	}

	uint64_t PhysicsClass::hashCell(const Cell& c) {
		//return c.x * 100 + c.y * 1000 + c.z * 10000;
		//using large prime numbers for hashing because they help distribute the hash values more uniformly
		return (uint64_t)(c.x * 73856093) ^ (uint64_t)(c.y * 19349663) ^ (uint64_t)(c.z * 83492791);
	}

	//Integration (semi-implicit Euler): https://math.libretexts.org/Bookshelves/Differential_Equations/Numerically_Solving_Ordinary_Differential_Equations_(Brorson)/01%3A_Chapters/1.07%3A_Symplectic_integrators
	void PhysicsClass::integrateForces(float dt) {
		for (RigidBody& b : rBodies) {
			if (b.mass == 0.0f) continue;
			glm::vec3 acceleration = b.force;
			b.velocity += acceleration * dt;
			b.force = {};
		}
	}

	void PhysicsClass::integrateVelocity(float dt) {
		const float sleepThreshold = 0.05f;
		const float sleepTime = 0.5f;

		for (RigidBody& b : rBodies) {
			if (b.mass == 0.0f) continue;
			b.position += b.velocity * dt;

			//sleep logic, I decided to implement it here for efficiency
			if (glm::length(b.velocity) < sleepThreshold) {
				b.sleepTimer += dt;
				if (b.sleepTimer > sleepTime)
					b.sleep = true;
			}
			else {
				b.sleepTimer = 0;
				b.sleep = false;
			}
		}
	}

}