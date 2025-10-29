#include "mve_window.h"

#include <stdexcept>
namespace mve {
    MveWindow::MveWindow(int w, int h, const char* name) : width(w), height(h), windowName(name) {
        initWindow();
    }

    MveWindow::~MveWindow() {
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void MveWindow::initWindow() {
        glfwInit();
        //glfwWindowHint sets window properties before creation, GLFW_CLIENT_API specifies which client API to create context for, here we specify no context. GLFW_RESIZABLE allows window to be resizable
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        window = glfwCreateWindow(width, height, windowName, nullptr, nullptr); //last two args are monitor and share. 
        //glfwSetWindowUserPointer pairs glfw window object with arbituary pointer value
        glfwSetWindowUserPointer(window, this);
        //the glfw library allows to register callback function that when the window is resized, the function will be called with the arguments being the window pointer and new width and height
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    }

    void MveWindow::createWindowSurface(VkInstance instance, VkSurfaceKHR* surface) {
        if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
    }

    void MveWindow::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        //reinterpret_cast allows convertion of one pointer type to another. Risky to use. 
        auto mveWindow = reinterpret_cast<MveWindow*>(glfwGetWindowUserPointer(window));
        mveWindow->framebufferResized = true;
        mveWindow->width = width;
        mveWindow->height = height;
    }
}
