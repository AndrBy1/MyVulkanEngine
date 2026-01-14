//thanks to ChatGPT

#pragma once

#include "mve_game_object.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#define GLM_ENABLE_EXPERIMENTAL //need for gtx
#include <glm/gtx/quaternion.hpp>

#include <iostream>

namespace mve{
	struct RigidBody {
		glm::vec3 position{ 0.f };
		glm::vec3 velocity{ 0.f }; //
		glm::vec3 angularVelocity{ 0.f };
		//quat is quaternion representation for rotation
		glm::quat rotation; 
		glm::vec3 force{ 0.f };
		int objId; //might not needed bc the rigid body is stored in a vector where the index is the objId
		float mass{ 1.f };
		float sleepTimer = 0.f;
		bool sleep{ false };
		bool collidable = false;

	};

	struct Contact {
		uint32_t a;
		uint32_t b;
		glm::vec3 normal;
		float penetration; //depth of penetration
	};


	//If you only generate one contact point per collision, you might miss important details about how the objects interact, especially in complex collisions.
	//By having multiple contact points, you can capture a more accurate representation of the collision, leading to better simulation of forces, friction, and overall behavior of the objects involved.
	struct ContactManifold {
		uint32_t A, B;
		glm::vec3 normal;
		std::vector<Contact> points;
	};

	struct OBB { //OBB (Oriented Bounding Box)
		glm::vec3 center;
		glm::vec3 axis[3]; //local x, y, z axes
		glm::vec3 halfSize; // half-widths along each axis
	};

	//Separating Axis Theorem: https://www.geometrictools.com/Documentation/MethodOfSeparatingAxes.pdf
	struct SATResult {
		bool hit;
		float penetration;
		glm::vec3 normal;
	};

	//AABB (Axis-Aligned Bounding Box) collisions
	struct AABB {
		glm::vec3 min;
		glm::vec3 max;
	};

	struct BoxCollider {
		//half extents is half the size of the box in each dimension
		glm::vec3 halfSize;
		uint32_t bodyIndex;
	};

	struct SphereCollider {
		float radius;
		uint32_t bodyIndex; //index of the rigid body in the physics system 
	};

	struct Cell { //for a uniform grid
		int x, y, z;
	};

	class PhysicsClass {
	public:
		PhysicsClass();
		~PhysicsClass();

		void step(float dt); //step is a single update per frame

		void addSphereCollider(int objId, float radius);
		void addBoxCollider(int objId, const glm::vec3& halfSize);

		void setSpeed(int objId, const glm::vec3& speed);
		void applyForce(int objId, const glm::vec3& force);

		void addRigidBody(MveGameObject& obj);


		Cell getCell(const glm::vec3& pos, float cellSize);

		std::vector<RigidBody> rBodies; //probably better to keep this as a vector for cache efficiency


	private:
		void buildGrid(float cellSize);

		
		bool sphereSphere(const RigidBody& a, const RigidBody& b, float rA, float rB, Contact& out);


		// broad phase collision detection using uniform grid
		void detectCollisions();

		//collision response: Impulse solver
		void resolveCollisions();

		//Penetration correction. To prevent sinking due to numerical errors
		void positionalCorrection(RigidBody& a, RigidBody& b, const Contact& c);

		// https://research.ncl.ac.uk/game/mastersdegree/gametechnologies/physicstutorials/6accelerationstructures/Physics%20-%20Spatial%20Acceleration%20Structures.pdf
		// broad phase: determine which objects can possibly collide against each other, and then store the potential object collision in a list called collision pair
		// Narrow phase collision detection: iterates over the list of potential collision pairs, and determines whether they really are colliding, 
		// and if so, resolves the collision
		void broadPhase();

		void insertCollider(uint32_t colliderIndex, const AABB& aabb, float cellSize);
		AABB computeAABB(const RigidBody& body, const BoxCollider& box);
		//AABB vs AABB collision and outputs contact info
		bool AABBAABB(const AABB& a, const AABB& b, Contact& out);
		bool aabbIntersect(const AABB& a, const AABB& b);

		OBB buildOBB(uint32_t colliderIndex);
		SATResult testOBBvsOBB(const OBB& a, const OBB& b);

		void project(const std::vector<glm::vec3>& vertices, const glm::vec3& axis, float& min, float& max);

		uint64_t hashCell(const Cell& c);

		//Integration (semi-implicit Euler): https://math.libretexts.org/Bookshelves/Differential_Equations/Numerically_Solving_Ordinary_Differential_Equations_(Brorson)/01%3A_Chapters/1.07%3A_Symplectic_integrators
		void integrateForces(float dt);
		void integrateVelocity(float dt);
		void applySleep(float dt);

		std::vector<SphereCollider> sphereColliders;
		std::vector<BoxCollider> boxColliders;
		std::vector<Contact> contacts;
		std::vector<MveGameObject>* debugPoints;
		//build grid
		std::unordered_map<uint64_t, std::vector<uint32_t>> grid; //maps cell keys to rigid body indices
		std::vector<std::pair<uint32_t, uint32_t>> aabbPairs; //maps rigid body indices to their AABBs potential collision pairs
	};
}
/*
aabbPairs:
	1: (0,1)
	2: (0,2)
	3: (1,2)
	4: (2,3)
	5: (3,4)
	6: (4,5)


grid:
	CellKey1: [0, 1]
	CellKey2: [0, 2]
	CellKey3: [1, 2]
	CellKey4: [2, 3]
	CellKey5: [3, 4]
	CellKey6: [4, 5]
*/