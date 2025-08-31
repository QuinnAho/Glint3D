#include "application.h"
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

int main()
{
    auto* app = new Application();
    if (!app->init("OBJ Viewer", 800, 600))
        return -1;

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg([](void* p){ static_cast<Application*>(p)->frame(); }, app, 0, true);
#else
    app->run();
    delete app;
#endif
    return 0;
}
