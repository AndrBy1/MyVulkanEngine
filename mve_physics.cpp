#include "mve_physics.h"

namespace mve {
	PhysicsClass::PhysicsClass() {
	}
	PhysicsClass::~PhysicsClass() {
	}
	void PhysicsClass::step(float dt) {
		for (auto& body : rBodies) {
			if (body.isStatic) continue;

			
		}
	}
	void PhysicsClass::setSpeed(int objId, const glm::vec3& speed) {

		//if rBodies is a vector:
		for (auto& body : rBodies) {
			if(body.objId == objId) {
				body.velocity = speed;
				return;
			}
		}

		//if rBodies is a map:
		//rBodies[objId].velocity = speed;
	}
	void PhysicsClass::applyForce(int objId, const glm::vec3& force) {
		
	}
	void PhysicsClass::addRigidBody(int objId) {
		RigidBody body;
		body.objId = objId;
		rBodies.push_back(body);
		//rBodies[objId] = body;
	}
}