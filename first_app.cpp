#include "first_app.h"

#include "keyboard_movement_controller.h"

#include "mve_camera.h"
#include "simple_render_system.h"
#include "point_light_system.h"
#include "mve_buffer.h"

#include <stdexcept>

//libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <array> //for std::array
#include <chrono> //for time related functions
#include <cassert> 
#include <typeinfo>
#include <iostream> 

namespace mve {

    FirstApp::FirstApp() {
        globalPool = MveDescriptorPool::Builder(mveDevice)
            .setMaxSets(MveSwapChain::MAX_FRAMES_IN_FLIGHT + 3)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MveSwapChain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3) //for texture images
			.build();
        loadGameObjects(); // Load the model data before creating the pipeline
    }

    FirstApp::~FirstApp() {}

    void FirstApp::run() {
        //noncoherentAtomsize is the smallest memory range the device allows when syncing between host and device memory
        std::vector<std::unique_ptr<MveBuffer>> uboBuffers(MveSwapChain::MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < uboBuffers.size(); i++) {
            uboBuffers[i] = std::make_unique<MveBuffer>(
                mveDevice,
                sizeof(GlobalUbo),
                1,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
            );
            uboBuffers[i]->map();
        }

        auto globalSetLayout = MveDescriptorSetLayout::Builder(mveDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
            .build();
        setLayouts.push_back(globalSetLayout->getDescriptorSetLayout());

        auto textureSetLayout = MveDescriptorSetLayout::Builder(mveDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) //Error occur bc of this
            .build();
        setLayouts.push_back(textureSetLayout->getDescriptorSetLayout());

        std::vector<VkDescriptorSet> globalDescriptorSets(MveSwapChain::MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < globalDescriptorSets.size(); i++) {
            auto bufferInfo = uboBuffers[i]->descriptorInfo();
            mveDescriptorWriter(*globalSetLayout, *globalPool)
                .writeBuffer(0, &bufferInfo)
                //.writeBuffer(1, &bufferInfo)
                .build(globalDescriptorSets[i]);
        }

        /**/
        std::vector<VkDescriptorSet> textureDescriptorSets(imageInfos.size());
        for (int i = 0; i < imageInfos.size(); i++) {
            //VkDescriptorImageInfo roomImageInfo = models[i].getTextureImage().descriptorInfo(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            //std::cout << "ImageInfo sampler: " << roomImageInfo.sampler << ", imageView: " << roomImageInfo.imageView << ", layout: " << roomImageInfo.imageLayout << std::endl;
            mveDescriptorWriter(*textureSetLayout, *globalPool)
                .writeImage(0, &imageInfos[i])
                .build(textureDescriptorSets[i]);
        }
        
        int ind = 1;
        for (auto& kv : gameObjects) {
			std::cout << " obj id: " << kv.first << "\n";
            auto& obj = kv.second;
            if (obj.model == nullptr) continue;
            //TODO: make this moore efficient
            //if (obj.model->getTextureImage() == nullptr) continue;
            if (obj.textureImage == nullptr) { 
				//use fallback texture
                //obj.model->attachTextureFromFile("textures/white.png");
				obj.setTextureDescriptor(textureDescriptorSets[0]);
                continue; 
            }
			std::cout << "index: " << ind << "\n";
			kv.second.setTextureDescriptor(textureDescriptorSets[ind]);
            //obj.model->setTextureDescriptor(textureDescriptorSets[ind]);
            ind++;
        }

        SimpleRenderSystem simpleRenderSystem{
            mveDevice, mveRenderer.getSwapChainRenderPass(), setLayouts
		};

        PointLightSystem pointLightSystem{
            mveDevice, mveRenderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout() 
        };


		MveCamera camera{};

        auto viewerObject = MveGameObject::createGameObject();
		viewerObject.transform.translation.z = -2.5f;
		KeyboardMovementController cameraController{};

        //returns a high precision value representing current time
		auto currentTime = std::chrono::high_resolution_clock::now();

		PhysicsClass physics;

        physics.addRigidBody(gameObjects.at(0));
        physics.addRigidBody(gameObjects.at(1));
		physics.addBoxCollider(0, {0.3f, 0.3f, 0.3f});
		physics.addBoxCollider(1, { 0.3f, 0.3f, 0.3f });
		physics.applyForce(1, { -100.f, 35.f, 70.f });

        while (!mveWindow.shouldClose()) {
            //checks and processes window level events such as keyboard and mouse input
            glfwPollEvents();

			auto newTime = std::chrono::high_resolution_clock::now();//get the current time
            //calculate the time difference between the current and previous frame in seconds
			float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count(); 
            currentTime = newTime;

			frameTime = std::min(frameTime, 0.1f); //add max allowable frame time to avoid large jumps

			cameraController.moveInPlaneXZ(mveWindow.getGLFWwindow(), frameTime, viewerObject);
            camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

			float aspect = mveRenderer.getAspectRatio();
			//camera.setOrthographicProjection(-aspect, aspect, -1, 1, -1, 1);
			camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 100.f);

            if (auto commandBuffer = mveRenderer.beginFrame()) {
				int frameIndex = mveRenderer.getFrameIndex();
                FrameInfo frameInfo{
                    frameIndex,
                    frameTime,
                    commandBuffer,
                    camera,
                    globalDescriptorSets[frameIndex],
                    gameObjects };

                //update
                GlobalUbo ubo{};
                ubo.projection = camera.getProjection();
                ubo.view = camera.getView();
                ubo.inverseView = camera.getInverseView();
                physics.step(frameTime);
                
				pointLightSystem.update(frameInfo, ubo);
                //std::cout << "buffer data: " << typeid(&ubo).name() << std::endl;
				uboBuffers[frameIndex]->writeToBuffer(&ubo); //don't need offset or size because we are using the entire buffer which is set in the constructor which is found at the top of this function
				uboBuffers[frameIndex]->flush();

                physics.step(frameTime);

				//update game object positions from physics simulation
                for(auto& body: physics.rBodies){
                    //std::cout << "Object ID: " << body.objId << " is at position: " << gameObjects.at(body.objId).transform.translation.z << "\n";
					if (body.sleep) continue;
                    gameObjects.at(body.objId).transform.translation = body.position;
                    //gameObjects.at(body.objId).transform.rotation = body.rotation;
				}

                //render
				mveRenderer.beginSwapChainRenderPass(commandBuffer);

                //order here matters
				simpleRenderSystem.renderGameObjects(frameInfo);
                pointLightSystem.render(frameInfo);

				mveRenderer.endSwapChainRenderPass(commandBuffer);
				mveRenderer.endFrame();
            }
        }
        //waits for the device to finish all operations before destroying resources
		vkDeviceWaitIdle(mveDevice.device());
    }

    void FirstApp::loadGameObjects() {
        //smooth uses smooth shading for vertex normals where the normal was calculated as if there was a smooth surface
        //flat uses flat shading where the normal is the same for the entire face 
		//WE DON"T NEED ../ BEFORE THE PATH BECAUSE THE WORKING DIRECTORY IS SET TO THE PROJECT FOLDER
        
        //fallback image needs to be created before any image since imageInfos relies on it to be in [0]
        fallbackImage.createTextureImage("textures/white.png"); 
        imageInfos.push_back(fallbackImage.descriptorInfo(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));

        makeModelObj("models/flat_vase.obj", { -.2f, .5f, 0.f }, { 3.f, 1.5f, 3.f });
        makeModelObj("models/smooth_vase.obj", { .8f, .2f, -0.5f }, { 3.f, 1.5f, 3.f });
        makeModelObj("models/viking_room.obj", { 0.f, .3f, 1.f }, { 1.f, 1.f, 1.f }, { glm::radians(90.f), glm::radians(90.f), 0.f }, "textures/viking_room.png");
        makeModelObj("models/quad.obj", { 0.f, .5f, 0.f }, { 1.f, 1.f, 1.f });

        std::vector<glm::vec3> lightColors{
            {1.f, .1f, .1f},
            {.1f, .1f, 1.f},
            {.1f, 1.f, .1f},
            {1.f, 1.f, .1f},
            {.1f, 1.f, 1.f},
            {1.f, 1.f, 1.f}  //
        };

        for (int i = 0; i < lightColors.size(); i++) {
            auto pointLight = MveGameObject::makePointLight(0.2f);
            pointLight.color = lightColors[i];
            
			//rotate function creates a rotation matrix given an angle and an axis of rotation
			//param1: m is the matrix to be rotated. param2: angle in radians. param3: axis of rotation
            auto rotateLight = glm::rotate(glm::mat4(1.f), (i * glm::two_pi<float>()) / lightColors.size(), { 0.f, -1.f, 0.f });
            pointLight.transform.translation = glm::vec3(rotateLight * glm::vec4(-1.f, -1.f, -1.f, 0.f));
            //pointLight.transform.translation.y = -2.f;
            //std::cout << "Light: " << i << "is at position: " << pointLight.transform.translation.y << "\n";
            gameObjects.emplace(pointLight.getId(), std::move(pointLight));
        }
    }

    void FirstApp::makeModelObj(std::string modelPath, glm::vec3 position, glm::vec3 scale, glm::vec3 rotation, std::string texturePath) {
        std::shared_ptr<MveModel> MveModel = MveModel::createModelFromFile(mveDevice, modelPath);
        auto obj = MveGameObject::createGameObject();
        obj.model = MveModel;
        obj.transform.translation = position;
        obj.transform.scale = scale;
        obj.transform.rotation = rotation;
		if (texturePath != ""){
            imageInfos.push_back(obj.attachTextureFromFile(texturePath));
        }
        //When the object is getting emplaced, IT GETS DESTROYED AFTER THE FUNCTION ENDS BC IT IS A LOCAL VARIABLE
        gameObjects.emplace(obj.getId(), std::move(obj));
	}
}