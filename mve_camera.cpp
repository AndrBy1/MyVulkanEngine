#include "mve_camera.h"

#include <cassert> //cassert is used for the assert function which helps in debugging by checking conditions at runtime
#include <limits>	//limits is used to define properties of fundamental types, such as the maximum value for float

namespace mve {
	//this function sets up an orthographic projection matrix for the camera 
	//which is used in 3D graphics to represent a view where objects are the same size regardless of their distance from the camera.
	//the far is used to determine how far away objects can be before they are no longer rendered. the near is used to determine how close objects can be before they are no longer rendered.
	//top is the upper bound of the view volume, bottom is the lower bound, left is the left bound, and right is the right bound.
	void MveCamera::setOrthographicProjection(float left, float right, float top, float bottom, float near, float far) {
		projectionMatrix = glm::mat4{ 1.0f }; 
		projectionMatrix[0][0] = 2.f / (right - left); //this scales the x-coordinates to fit within the left and right bounds. for example, if left is -1 and right is 1, this value would be 1, meaning that x-coordinates will be scaled to fit within the range of -1 to 1.
		projectionMatrix[1][1] = 2.f / (bottom - top);
		projectionMatrix[2][2] = 1.f / (far - near);
		projectionMatrix[3][0] = -(right + left) / (right - left);
		projectionMatrix[3][1] = -(bottom + top) / (bottom - top);
		projectionMatrix[3][2] = -near / (far - near);
	}

	//this function sets up a perspective projection matrix for the camera, which simulates the way the human eye sees the world.
	//ASPECT is the aspect ratio of the viewport, which is the ratio of its width to its height.
	//FOVY is the vertical field of view angle in radians, which determines how wide the camera's view is.
	void MveCamera::setPerspectiveProjection(float fovy, float aspect, float near, float far) {
		assert(glm::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f); // Ensure aspect ratio is not too close to zero
		//tanHalfFovy is the tangent of half the vertical field of view angle (fovy). 
		//it is used to calculate the scaling factors for the x and y coordinates in the projection matrix.
		const float tanHalfFovy = tan(fovy / 2.f); 
		// Initialize the projection matrix to a zero matrix
		projectionMatrix = glm::mat4{ 0.f };
		// Set the diagonal elements of the projection matrix based on the aspect ratio and field of view
		//ex: if aspect is 16/9 and fovy is 90 degrees (pi/2 radians), then tanHalfFovy would be 1, and projectionMatrix[0][0] would be 9/16, scaling the x-coordinates accordingly.
		projectionMatrix[0][0] = 1.f / (aspect * tanHalfFovy);
		projectionMatrix[1][1] = 1.f / (tanHalfFovy);
		// Set the elements responsible for depth mapping and perspective division
		//ex: if near is 0.1 and far is 100, then projectionMatrix[2][2] would be approximately 1.001001, which helps map depth values correctly between the near and far planes.
		projectionMatrix[2][2] = far / (far - near);
		projectionMatrix[2][3] = 1.f;
		projectionMatrix[3][2] = -(far * near) / (far - near);
	}

	//this function sets the camera's view matrix based on a given position, direction, and up vector.
	void MveCamera::setViewDirection(glm::vec3 position, glm::vec3 direction, glm::vec3 up) {
		//w is the normalized direction vector, representing the forward direction of the camera.
		const glm::vec3 w{ glm::normalize(direction) };
		//u is the right vector, calculated as the cross product of w and the up vector, and then normalized. it represents the right direction of the camera.
		const glm::vec3 u{ glm::normalize(glm::cross(w, up)) };
		//v is the true up vector, calculated as the cross product of w and u. it represents the upward direction of the camera.
		const glm::vec3 v{ glm::cross(w, u) };

		viewMatrix = glm::mat4{ 1.f };
		viewMatrix[0][0] = u.x;
		viewMatrix[1][0] = u.y;
		viewMatrix[2][0] = u.z;
		viewMatrix[0][1] = v.x;
		viewMatrix[1][1] = v.y;
		viewMatrix[2][1] = v.z;
		viewMatrix[0][2] = w.x;
		viewMatrix[1][2] = w.y;
		viewMatrix[2][2] = w.z;
		viewMatrix[3][0] = -glm::dot(u, position);
		viewMatrix[3][1] = -glm::dot(v, position);
		viewMatrix[3][2] = -glm::dot(w, position);

		inverseViewMatrix = glm::mat4{ 1.f };
		inverseViewMatrix[0][0] = u.x;
		inverseViewMatrix[0][1] = u.y;
		inverseViewMatrix[0][2] = u.z;
		inverseViewMatrix[1][0] = v.x;
		inverseViewMatrix[1][1] = v.y;
		inverseViewMatrix[1][2] = v.z;
		inverseViewMatrix[2][0] = w.x;
		inverseViewMatrix[2][1] = w.y;
		inverseViewMatrix[2][2] = w.z;
		inverseViewMatrix[3][0] = position.x;
		inverseViewMatrix[3][1] = position.y;
		inverseViewMatrix[3][2] = position.z;

	}

	//this function sets the camera's view matrix based on a target point, direction, and up vector.
	void MveCamera::setViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up) {
		setViewDirection(position, target - position, up);
	}

	//this function sets the camera's view matrix based on a given position and rotation in YXZ order (yaw, pitch, roll).
	void MveCamera::setViewYXZ(glm::vec3 position, glm::vec3 rotation) {
		// Calculate the cosine and sine of the rotation angles for each axis so that we can construct the rotation matrix.
		const float c3 = glm::cos(rotation.z);
		const float s3 = glm::sin(rotation.z);
		const float c2 = glm::cos(rotation.x);
		const float s2 = glm::sin(rotation.x);
		const float c1 = glm::cos(rotation.y);
		const float s1 = glm::sin(rotation.y);
		//here u, v and w are being assigned the right, up, and forward vectors of the camera based on the rotation angles.
		const glm::vec3 u{ (c1 * c3 + s1 * s2 * s3), (c2 * s3), (c1 * s2 * s3 - c3 * s1) };
		const glm::vec3 v{ (c3 * s1 * s2 - c1 * s3), (c2 * c3), (c1 * c3 * s2 + s1 * s3) };
		const glm::vec3 w{ (c2 * s1), (-s2), (c1 * c2) };
		viewMatrix = glm::mat4{ 1.f };
		viewMatrix[0][0] = u.x;
		viewMatrix[1][0] = u.y;
		viewMatrix[2][0] = u.z;
		viewMatrix[0][1] = v.x;
		viewMatrix[1][1] = v.y;
		viewMatrix[2][1] = v.z;
		viewMatrix[0][2] = w.x;
		viewMatrix[1][2] = w.y;
		viewMatrix[2][2] = w.z;
		//assigning the translation components of the view matrix to position the camera correctly in the 3D world.
		viewMatrix[3][0] = -glm::dot(u, position);
		viewMatrix[3][1] = -glm::dot(v, position);
		viewMatrix[3][2] = -glm::dot(w, position);

		inverseViewMatrix = glm::mat4{ 1.f };
		inverseViewMatrix[0][0] = u.x;
		inverseViewMatrix[0][1] = u.y;
		inverseViewMatrix[0][2] = u.z;
		inverseViewMatrix[1][0] = v.x;
		inverseViewMatrix[1][1] = v.y;
		inverseViewMatrix[1][2] = v.z;
		inverseViewMatrix[2][0] = w.x;
		inverseViewMatrix[2][1] = w.y;
		inverseViewMatrix[2][2] = w.z;
		inverseViewMatrix[3][0] = position.x;
		inverseViewMatrix[3][1] = position.y;
		inverseViewMatrix[3][2] = position.z;
	}
}
