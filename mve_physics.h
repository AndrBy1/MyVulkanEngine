//thanks to ChatGPT
//AABB (Axis-Aligned Bounding Box) collisions

#pragma once

#include "mve_game_object.h"

namespace mve{
	struct RigidBody {
		glm::vec3 position{ 0.f };
		glm::vec3 velocity{ 0.f }; //
		glm::vec3 angularVelocity{ 0.f };
		glm::vec3 force{ 0.f };
		int objId; //might not needed bc the rigid body is stored in a vector where the index is the objId
		float mass{ 1.f };
		bool isStatic{ false };
		bool collidable = true;

	};

	struct Contact {
		glm::vec3 point; //contact point
		glm::vec3 normal;
		float penetration;
	};

	struct SphereCollider {
		float radius;
		uint32_t bodyIndex; //index of the rigid body in the physics system 
	};

	struct CellKey { //for a uniform grid
		int x, y, z;
	};

	class PhysicsClass {
	public:
		PhysicsClass();
		~PhysicsClass();

		void step(float dt); //step is a single update per frame

		void setSpeed(int objId, const glm::vec3& speed);
		void applyForce(int objId, const glm::vec3& force);

		void addRigidBody(MveGameObject& obj);

		CellKey getCellKey(const glm::vec3& pos, float cellSize);

		void buildGrid(float cellSize);



		std::vector<RigidBody> rBodies; //probably better to keep this as a vector for cache efficiency
		std::vector<Contact> contacts;
		std::vector<SphereCollider> sphereColliders;



	private:
		//Integration (semi-implicit Euler): https://math.libretexts.org/Bookshelves/Differential_Equations/Numerically_Solving_Ordinary_Differential_Equations_(Brorson)/01%3A_Chapters/1.07%3A_Symplectic_integrators
		void integrateForces(float dt);
		void integrateVelocity(float dt);

		// https://research.ncl.ac.uk/game/mastersdegree/gametechnologies/physicstutorials/6accelerationstructures/Physics%20-%20Spatial%20Acceleration%20Structures.pdf
		// Narrow phase collision detection: iterates over the list of potential collision pairs, and determines whether they really are colliding, 
		// and if so, resolves the collision
		bool sphereSphere(const RigidBody& a, const RigidBody& b, float rA, float rB, Contact& out);

		//broad phase collision detection using uniform grid
		void detectCollisions();

		//collision response: Impulse solver
		void resolveCollisions();

		//Penetration correction. To prevent sinking due to numerical errors
		void positionalCorrection(RigidBody& a, RigidBody& b, const Contact& c);

		//build grid
		std::unordered_map<uint64_t, std::vector<uint32_t>> grid; //maps cell keys to rigid body indices
	};
}