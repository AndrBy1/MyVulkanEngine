#include "first_app.h"

// std
#include <cstdlib>
#include <iostream>
#include <stdexcept>

int main() {
    mve::FirstApp app{};

    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

//Validation layers are optional components that hook into Vulkan function calls to apply additional operations
//swap chain is a series of fram buffers used to display images to window
//graphics pipeline outputs to a target frame buffer
//frame buffer can have different attachments like color buffer and depth buffer
//there are generally multiple buffers, some (sometimes 1 or 2) operating in the back and one in front on display. When front is done, it gets swap with a back buffer. 
//v-sync is the moment at which the display draws the new images, tied to refresh rate
//cant execute commands directly through function calls, first record them to command buffer, then submit buffer to device qeue to be executed
//Command buffers are objects used to record commands which can be subsequently submitted to a device queue for execution
//frame buffers are used to store the results of rendering operations, such as color and depth information, which can then be presented to the display
//stride is the number of bytes between each row
//interpolation is process of estimating unknown value that falls between known values
//Barycentric coordinates allow representation of a point in a triangle as a weighted average of the triangles vertices P = aV1 + bV2 + cV3
//push constants are a way to push constants onto a shader via command buffer
//homogeneous matrix is a matrix that contains extra component (w) for transformations, rotations and scale. 
//w = 1 point represents same 3D position, w = 0 represents firection vector. 
//
//Ray tracing is image-order rendering
//orthographic projection is a way of projection 3D obj into 2D screen without perspecitve distortion
//frustom is a shape that captures viewers line of sight as if they're looking through rectangular window
//perspective matrix that transforms frustum and obj it contains into orthographic view volume would provide perspective to obj within this space. This creates perspective
//z fighting or stitch or depth fighting: when 2 surfaces are very close blend wierdly
//    l, r = left, right clipping planes. b, t = bottom, top clipping planes. n, f = near, far clipping planes
//    [2n/(r-l), 0, -(r+l)/(r-l), 0]            [2/(r-l), 0, 0, -(r+l)/(r-l)]            [n, 0, 0, 0]  
//    [0, 2n/(b-t), -(b+t)/(b-t), 0]     =      [0, 2/(b-t), 0, -(b+t)/(b-t)]     *      [0, n, 0, 0]  
//    [ 0,  0,  f/(f-n),  -fn/(f-n)]            [0, 0, 1/(f-n), -n/(f-n)]                [0, 0, (f+n), -fn]
//    [ 0,  0,  1,  0]                           [ 0,  0,  0,  1]                         [0, 0, 1, 0]  
//an index buffer is an array of numbers, each number corresponds to a vertex in vertex buffer. This is the order of vertices to be rendered.
//uniform buffers provide arbitrary read-only buffers to shaders, similar to push constants but can be much larger and more flexible 
// however can be slower, require additional setup and require binding before draw call is made which can have some overhead 
//overhead is the extra processing time required to manage and maintain the uniform buffer, including memory allocation, data transfer, and synchronization
//Descriptors point to a resource like a buffer or image and describe how that resource can be accessed by shaders
//descriptors need to be grouped into sets to bind 
//DescriptorSetLayout is a template for creating descriptor sets, defining the types and number of resources that can be bound to the pipeline. need to be provided at pipeline creation
//Bind descriptor sets before draw call
//descriptor sets can only be created using a descriptor pool
//when using descriptor sets:
//bind pipeline, bind global descriptor set at set 0
//for each material in Materials bind material descriptor set at set 1
//for each obj that uses material bind obj descriptor set at set 2, bind object model, draw obj