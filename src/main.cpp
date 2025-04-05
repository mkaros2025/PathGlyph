#include <glad/glad.h>

#include <chrono>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include "core/Application.h"

using namespace PathGlyph;

int main(int argc, char* argv[]) {
    try {        
        PathGlyph::Application app(1000, 600, "PathGlyph");
        app.run();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}