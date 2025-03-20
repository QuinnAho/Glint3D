#include "application.h"

int main()
{
    Application app;
    // Initialize with title and size
    if (!app.init("OBJ Viewer", 800, 600))
    {
        return -1;
    }

    // Run the main loop
    app.run();

    return 0;
}
