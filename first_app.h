//render manages swap chain, command buffers and draw frame
//simple render system sets up pipeline layout, simple push constants struct and render game objects

#pragma once //pragma once prevents multiple inclusions of the same header file similar to #ifndef guards
#include "mve_window.h"
#include "mve_device.h"
#include "mve_game_object.h"
#include "mve_renderer.h"
#include "mve_descriptors.h"

#include <memory>
#include <vector>

namespace mve {
    class FirstApp {
    public:
        static constexpr int WIDTH = 800;
        static constexpr int HEIGHT = 600;

        FirstApp();
        ~FirstApp();

        //becuase this is managing Vulkan objects for pipeline layout and command buffers, we should delete copy constructors 
        FirstApp(const FirstApp&) = delete; // Disable copy constructor
        FirstApp& operator=(const FirstApp&) = delete;

        void run();

    private:
        void loadGameObjects();

        std::vector<MveModel::Vertex> generateTriangles(int num);
        //order here matters
        MveWindow mveWindow{ WIDTH, HEIGHT, "HELLO VULKAN!" };
        MveDevice mveDevice{ mveWindow };
		MveRenderer mveRenderer{ mveWindow, mveDevice };
		MveImage fallbackImage{ mveDevice }; //when creating images they need to be set here so they don't get destroyed too early
		//order of declaration matters, need to be destroyed in reverse order of creation
        std::unique_ptr<MveDescriptorPool> globalPool{};
        std::vector<VkDescriptorImageInfo> imageInfos;
        std::vector<VkDescriptorSetLayout> setLayouts;

        MveGameObject::Map gameObjects;
        MveGameObject::id_t roomId = static_cast<MveGameObject::id_t>(-1);
    };
}