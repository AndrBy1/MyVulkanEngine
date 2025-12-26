#pragma once

#include "mve_game_object.h"

namespace mve{
	struct RigidBody {
		glm::vec3 velocity{ 0.f }; //
		glm::vec3 angularVelocity{ 0.f };
		int objId; //might not needed bc the rigid body is stored in a vector where the index is the objId
		float mass{ 1.f };
		bool isStatic{ false };
	};

	class PhysicsClass {
	public:
		PhysicsClass();
		~PhysicsClass();

		void step(float dt); //step is a single update per frame

		void setSpeed(int objId, const glm::vec3& speed);
		void applyForce(int objId, const glm::vec3& force);

		void addRigidBody(int objId);

		std::vector<RigidBody> rBodies; //probably better to keep this as a vector for cache efficiency

	private:

	};
}