#include "CorePulseApp.h"
#include <iostream>

int main(int argc, char* argv[]) {
    std::cout << "CorePulse - Phase 1 Window Management Demo\n";
    
    try {
        CorePulse::CorePulseApp app;
        
        if (!app.initialize()) {
            std::cerr << "Failed to initialize application\n";
            return -1;
        }
        
        app.run();
        app.shutdown();
        
        std::cout << "Application completed successfully\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
        return -1;
    }
    
    return 0;
}