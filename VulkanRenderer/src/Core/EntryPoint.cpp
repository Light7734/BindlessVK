#include "Core/Base.h"
#include "Core/Timer.h"
#include "Core/Window.h"
#include "Graphics/Renderer.h"

#include <glfw/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/matrix.hpp>

int main()
{
    // init logger
    Logger::Init();

    // create window & pipeline
    Window   window   = Window(800, 600);
    Renderer pipeline = Renderer(&window, 3);

    // fps calculator
    Timer    timer;
    uint32_t frames = 0u;

    /**  main loop  */
    while (!window.IsClosed())
    {
        // handle events
        glfwPollEvents();

        // render frame
        pipeline.BeginScene();
        pipeline.DrawQuad(glm::mat4(1.0f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
        pipeline.EndScene();

        // calculate fps
        frames++;
        if (timer.ElapsedTime() >= 1.0f)
        {
            LOG(trace, "FPS: {}", frames);

            timer.Reset();
            frames = 0u;
        }
    }

    return 0;
}