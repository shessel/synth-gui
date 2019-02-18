#include <SFML/Window.hpp>
#include <SFML/OpenGL.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <imgui.h>
#include <imgui-SFML.h>

#include <iostream>

int main()
{
    // create the window
    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.stencilBits = 8;
    settings.antialiasingLevel = 4;
    settings.majorVersion = 4;
    settings.minorVersion = 3;
    // settings.attributeFlags = settings.Core;
    // imgui-sfml backend doesn't support modern GL backend, but disabling core allows old stuff
#ifdef _DEBUG
    settings.attributeFlags |= settings.Debug;
#endif


    sf::RenderWindow window(sf::VideoMode(800, 600), "OpenGL", sf::Style::Default, settings);
    window.setVerticalSyncEnabled(true);
    window.setFramerateLimit(60);

    // activate the window
    window.setActive(true);
    ImGui::SFML::Init(window);

    // load resources, initialize the OpenGL states, ...

    // run the main loop
    bool running = true;
    sf::Vector3f clear_color;

    while (running)
    {
        // handle events
        sf::Event event;
        sf::Clock deltaClock;
        while (window.pollEvent(event))
        {
            ImGui::SFML::ProcessEvent(event);
            if (event.type == sf::Event::Closed)
            {
                // end the program
                running = false;
            }
            else if (event.type == sf::Event::Resized)
            {
                // adjust the viewport when the window is resized
                glViewport(0, 0, event.size.width, event.size.height);
            }
        }
       
        ImGui::SFML::Update(window, deltaClock.restart());
       
        {
            ImGui::Begin("Hello, world!");
            if (ImGui::Button("Look at this pretty button"))
            {
                std::cout << "button clicked" << std::endl;
            }
            ImGui::End();
        }
        {
            static float f = 0.0f;
            static int counter = 0;
            bool show_demo_window;
            bool show_another_window;


            ImGuiIO& io = ImGui::GetIO();
            io.MouseClickedPos;

            ImGui::Begin("Hello again, world!");                          // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &show_another_window);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

            if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            const float values[] = { 0.2f, 0.0f, 0.4f, 0.5f };
            ImVec2 mousePosRel;
            bool mouseDown;
            ImGui::PlotLines("Lines", values, 4, 0, "yadayada", 0.0f, 1.0f, ImVec2(100.0f, 100.0f), 4, &mousePosRel, &mouseDown);
            if (mouseDown)
            {
                std::cout << mousePosRel.x << " " << mousePosRel.y << std::endl;
            }
            ImGui::PlotHistogram("Hist", values, 4, 0, "yadayada", 0.0f, 1.0f, ImVec2(100.0f, 100.0f));

            //ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();
        }

        glClearColor(clear_color.x, clear_color.y, clear_color.z, 1.0f);
        // clear the buffers
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //window.draw(shape);
        ImGui::SFML::Render(window);

        // end the current frame (internally swaps the front and back buffers)
        window.display();
    }

    // release resources...

    return 0;
}