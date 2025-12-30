#include "mve_physics.h"

namespace mve {
	PhysicsClass::PhysicsClass() {
	}
	PhysicsClass::~PhysicsClass() {
	}
	void PhysicsClass::step(float dt) {
		integrateForces(dt);
		detectCollisions();
		resolveCollisions();
		integrateVelocity(dt);
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
	}

	void PhysicsClass::addRigidBody(MveGameObject& obj) {
		RigidBody body;
		body.objId = obj.getId();
		body.position = obj.transform.translation;
		rBodies.push_back(body);
		SphereCollider collider;
		collider.radius = 0.3f; //default radius
		collider.bodyIndex = static_cast<uint32_t>(rBodies.size() - 1);
		sphereColliders.push_back(collider);
	}

	CellKey PhysicsClass::getCellKey(const glm::vec3& pos, float cellSize) {
		return CellKey{
			static_cast<int>(std::floor(pos.x / cellSize)),
			static_cast<int>(std::floor(pos.y / cellSize)),
			static_cast<int>(std::floor(pos.z / cellSize))
		};
	}

	void PhysicsClass::buildGrid(float cellSize){
		grid.clear();
		for (uint32_t i = 0; i < sphereColliders.size(); i++) {
			CellKey c = getCellKey(rBodies[sphereColliders[i].bodyIndex].position, cellSize);
			uint64_t key = c.x * 100 + c.y * 10 + c.z; //simple hash function for CellKey
			grid.emplace(key, i);
			//uint64_t key = mortonEncode(c);
			//grid[key].push_back(i);
		}
	}

	//Integration (semi-implicit Euler): https://math.libretexts.org/Bookshelves/Differential_Equations/Numerically_Solving_Ordinary_Differential_Equations_(Brorson)/01%3A_Chapters/1.07%3A_Symplectic_integrators
	void PhysicsClass::integrateForces(float dt){
		for (RigidBody& b : rBodies) {
			if (b.mass == 0.0f) continue;
			glm::vec3 acceleration = b.force * b.mass;
			b.velocity += acceleration * dt;
			b.force = {};
		}
	}
	void PhysicsClass::integrateVelocity(float dt){
		for (RigidBody& b : rBodies) {
			if (b.mass == 0.0f) continue;
			b.position += b.velocity * dt;
		}
	}

	bool PhysicsClass::sphereSphere(const RigidBody& a, const RigidBody& b, float rA, float rB, Contact& out){
		glm::vec3 d = b.position - a.position;
		float distSq = glm::dot(d, d); //ax * bx + ay * by + az * bz
		float radiusSum = rA + rB;
		if (distSq <= radiusSum * radiusSum) {
			float dist = std::sqrt(distSq);
			out.penetration = radiusSum - dist;
			out.normal = (dist > 0) ? d / dist : glm::vec3(1, 0, 0); //if dist > 0, d / dist, else set normal to arbitrary axis
			//out.point = a.position + out.normal * rA;
			return true;
		}
	}

	void PhysicsClass::detectCollisions(){
		contacts.clear();
		//worst case: O(sphereColliders.size()^2)
		for (size_t i = 0; i < sphereColliders.size(); i++) {
			for (size_t j =  i + 1; j < sphereColliders.size(); j++) {
				Contact C;
				if(sphereSphere(
					rBodies[sphereColliders[i].bodyIndex], rBodies[sphereColliders[j].bodyIndex],
					sphereColliders[i].radius, sphereColliders[j].radius, C)) 
				{
					C.point[0] = sphereColliders[i].bodyIndex;
					C.point[1] = sphereColliders[j].bodyIndex;
					C.point[2] = 0.f; //unused
					contacts.push_back(C);
				}

			}
		}
	}

	void PhysicsClass::resolveCollisions(){
		const float restitution = 0.5f; //coefficient of restitution (bounciness)
		for (Contact& c : contacts) {
			RigidBody& A = rBodies[c.point[0]];
			RigidBody& B = rBodies[c.point[1]];

			glm::vec3 relativeVelocity = B.velocity - A.velocity;
			float velAlongNormal = glm::dot(relativeVelocity, c.normal);

			if (velAlongNormal > 0) continue; //objects are separating

			float massSum = A.mass + B.mass;
			if (massSum == 0.f) continue; //both objects are static

			float j = -(1 + restitution) * velAlongNormal;
			j /= massSum;

			glm::vec3 impulse = j * c.normal;
			A.velocity -= impulse * (1.f / A.mass);
			B.velocity += impulse * (1.f / B.mass);

			positionalCorrection(A, B, c);
		}
	}

	void PhysicsClass::positionalCorrection(RigidBody& A, RigidBody& B, const Contact& c){
		const float percent = 0.8f; //usually 20% to 80%
		const float slop = 0.01f; //usually 0.01 to 0.1

		float massSum = A.mass + B.mass;
		if (massSum == 0.f) return; //both objects are static

		glm::vec3 correction = (std::max(c.penetration - slop, 0.0f) / massSum) * percent * c.normal;

		A.position -= correction * (1.f / A.mass);
		B.position += correction * (1.f / B.mass);
	}
}