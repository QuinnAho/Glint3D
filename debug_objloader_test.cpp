#include "engine/include/objloader.h"
#include <iostream>

int main() {
    ObjLoader loader;
    std::cout << "Loading assets/models/sphere.obj..." << std::endl;
    loader.load("assets/models/sphere.obj");
    
    std::cout << "Vertex count: " << loader.getVertCount() << std::endl;
    std::cout << "Index count: " << loader.getIndexCount() << std::endl;
    
    if (loader.getVertCount() > 0) {
        const float* positions = loader.getPositions();
        if (positions) {
            std::cout << "First vertex: (" << positions[0] << ", " << positions[1] << ", " << positions[2] << ")" << std::endl;
        }
    }
    
    return 0;
}