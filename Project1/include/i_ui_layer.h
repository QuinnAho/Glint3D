#pragma once
class Application;

struct IUILayer {
    virtual ~IUILayer() = default;
    virtual void init(Application&) {}
    virtual void beginFrame() {}
    virtual void draw(Application&) {}
    virtual void endFrame() {}
    virtual void shutdown() {}
};
