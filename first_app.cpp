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
        globalPool = MveDescriptorPool::Builder(MveDevice)
            .setMaxSets(MveSwapChain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MveSwapChain::MAX_FRAMES_IN_FLIGHT)
            //.addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MveSwapChain::MAX_FRAMES_IN_FLIGHT)
			.build();
        loadGameObjects(); // Load the model data before creating the pipeline
    }

    FirstApp::~FirstApp() {}

    void FirstApp::run() {
        //noncoherentAtomsize is the smallest memory range the device allows when syncing between host and device memory
        std::vector<std::unique_ptr<MveBuffer>> uboBuffers(MveSwapChain::MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < uboBuffers.size(); i++) {
            uboBuffers[i] = std::make_unique<MveBuffer>(
                MveDevice,
                sizeof(GlobalUbo),
                1,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
            );
            uboBuffers[i]->map();
        }

        auto globalSetLayout = MveDescriptorSetLayout::Builder(MveDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
            //.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
            .build();

        std::vector<VkDescriptorSet> globalDescriptorSets(MveSwapChain::MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < globalDescriptorSets.size(); i++) {
            auto bufferInfo = uboBuffers[i]->descriptorInfo();
            mveDescriptorWriter(*globalSetLayout, *globalPool)
                .writeBuffer(0, &bufferInfo)
                //.writeBuffer(1, &bufferInfo)
                .build(globalDescriptorSets[i]);
        }

        SimpleRenderSystem simpleRenderSystem{
            MveDevice,
            MveRenderer.getSwapChainRenderPass(),
            globalSetLayout->getDescriptorSetLayout()
		};

        PointLightSystem pointLightSystem{
            MveDevice,
            MveRenderer.getSwapChainRenderPass(),
            globalSetLayout->getDescriptorSetLayout() 
        };

		MveCamera camera{};

        auto viewerObject = MveGameObject::createGameObject();
		viewerObject.transform.translation.z = -2.5f;
		KeyboardMovementController cameraController{};

        //returns a high precision value representing current time
		auto currentTime = std::chrono::high_resolution_clock::now();

		std::cout << "sizeof(GlobalUbo): " << sizeof(GlobalUbo) << "\n";

        while (!MveWindow.shouldClose()) {
            //checks and processes window level events such as keyboard and mouse input
            glfwPollEvents();

			auto newTime = std::chrono::high_resolution_clock::now();//get the current time
            //calculate the time difference between the current and previous frame in seconds
			float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count(); 
            currentTime = newTime;

			frameTime = std::min(frameTime, 0.1f); //add max allowable frame time to avoid large jumps

			cameraController.moveInPlaneXZ(MveWindow.getGLFWwindow(), frameTime, viewerObject);
            camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

			float aspect = MveRenderer.getAspectRatio();
			//camera.setOrthographicProjection(-aspect, aspect, -1, 1, -1, 1);
			camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 100.f);

            if (auto commandBuffer = MveRenderer.beginFrame()) {
				int frameIndex = MveRenderer.getFrameIndex();
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

				pointLightSystem.update(frameInfo, ubo);
                //std::cout << "buffer data: " << typeid(&ubo).name() << std::endl;
				uboBuffers[frameIndex]->writeToBuffer(&ubo); //don't need offset or size because we are using the entire buffer which is set in the constructor which is found at the top of this function
				uboBuffers[frameIndex]->flush();

                //render
				MveRenderer.beginSwapChainRenderPass(commandBuffer);

                //order here matters
				simpleRenderSystem.renderGameObjects(frameInfo);
                pointLightSystem.render(frameInfo);

				MveRenderer.endSwapChainRenderPass(commandBuffer);
				MveRenderer.endFrame();
            }
        }
        //waits for the device to finish all operations before destroying resources
		vkDeviceWaitIdle(MveDevice.device());
    }

    void FirstApp::loadGameObjects() {
        //smooth uses smooth shading for vertex normals where the normal was calculated as if there was a smooth surface
        //flat uses flat shading where the normal is the same for the entire face 
		//WE DON"T NEED ../ BEFORE THE PATH BECAUSE THE WORKING DIRECTORY IS SET TO THE PROJECT FOLDER
        std::shared_ptr<MveModel> MveModel = MveModel::createModelFromFile(MveDevice, "models/flat_vase.obj");

        auto flatVase = MveGameObject::createGameObject();
        flatVase.model = MveModel;
        flatVase.transform.translation = { -.5f, .5f, 0.f };
        flatVase.transform.scale = { 3.f, 1.5f, 3.f };
        gameObjects.emplace(flatVase.getId(), std::move(flatVase));

        MveModel = MveModel::createModelFromFile(MveDevice, "models/smooth_vase.obj");

        auto smoothVase = MveGameObject::createGameObject();
        smoothVase.model = MveModel;
        smoothVase.transform.translation = { .5f, .5f, 0.f };
        smoothVase.transform.scale = { 3.f, 1.5f, 3.f };
        gameObjects.emplace(smoothVase.getId(), std::move(smoothVase));

        MveModel = MveModel::createModelFromFile(MveDevice, "models/quad.obj");
        auto tile = MveGameObject::createGameObject();
        tile.model = MveModel;
        tile.transform.translation = { 0.f, .5f, 0.f };
        tile.transform.scale = { 1.f, 1.f, 1.f };
        gameObjects.emplace(tile.getId(), std::move(tile));

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
            auto rotateLight = glm::rotate(glm::mat4(1.f), (i * glm::two_pi<float>()) / lightColors.size(), { 0.f, -1.f, 0.f });
            pointLight.transform.translation = glm::vec3(rotateLight * glm::vec4(-1.f, -1.f, -1.f, 0.f));
            //pointLight.transform.translation.y = -2.f;
            //std::cout << "Light: " << i << "is at position: " << pointLight.transform.translation.y << "\n";
            gameObjects.emplace(pointLight.getId(), std::move(pointLight));
        }
    }
}