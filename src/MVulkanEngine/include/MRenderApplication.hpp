#ifndef MRENDERAPPLICATION_HPP
#define MRENDERAPPLICATION_HPP

#include <cstdint>

class MRenderApplication {
public:
    void Init();
    void Run();

    virtual void SetUp();// = 0;
    //virtual void UpdatePerFrame(uint32_t imageInde) = 0;
    virtual void ComputeAndDraw(uint32_t imageInde);// = 0;

    virtual void RecreateSwapchainAndRenderPasses();// = 0;
    virtual void CreateRenderPass();// = 0;
    virtual void PreComputes();// = 0;
    virtual void Clean();

    inline void SetCameraMoved(bool state) { m_cameraMoved = state; }
    inline bool GetCameraMoved() { return m_cameraMoved; }

    //typedef void (MRenderApplication::* SetCameraMove)(bool state);

protected:
    uint32_t    m_currentFrame = 0;
    bool        m_cameraMoved = false;

    uint16_t WIDTH = 1280;
    uint16_t HEIGHT = 800;

    void init();
    void renderLoop();

    void drawFrame();
};

#endif