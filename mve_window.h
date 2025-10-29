//MveWindow class handles the window creation and management using GLFW library

#pragma once 
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
namespace mve {
    class MveWindow {

    public:
        MveWindow(int w, int h, const char* name);
        ~MveWindow();

        MveWindow(const MveWindow&) = delete; // Disable copy constructor
        MveWindow& operator=(const MveWindow&) = delete;

        bool shouldClose() const { return glfwWindowShouldClose(window); }

        //mvextent2D is a struct that holds width and height as uint32_t
        VkExtent2D getExtent() { return { static_cast<uint32_t>(width), static_cast<uint32_t>(height) }; }
        bool wasWindowResized() { return framebufferResized; }
        void resetWindowResizedFlag() { framebufferResized = false; }
        GLFWwindow* getGLFWwindow() const { return window; }

        void createWindowSurface(VkInstance instance, VkSurfaceKHR* surface);
    private:
        static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
        void initWindow();

        int width;
        int height;
        bool framebufferResized = false;

        const char* windowName;
        GLFWwindow* window;

    };
}