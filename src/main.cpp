#include <SFML/Window.hpp>
#include <SFML/OpenGL.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui-SFML.h>

#include <iostream>

#include "synth.h"

void imgui_sound_desc()
{
    static sound_desc desc = {
        0,              // base_sound_id
        1.0f,           // amplitude
        440.0f,         // frequency
        0,              // frequency_modifier_id
        0,              // amplitude_modifier_id
        { 0.0f, 1.0f }, // frequency_modifier_params
        { 0.0f, 1.0f }, // amplitude_modifier_params
    };

    bool sound_desc_changed = false;
    ImGui::Text("Wave Form");   
    ImGui::SameLine();
    sound_desc_changed |= ImGui::InputScalar("##base_sound_id", ImGuiDataType_U32, &desc.base_sound_id);

    ImGui::Text("Frequency");
    ImGui::SameLine();
    sound_desc_changed |= ImGui::InputFloat("##frequency", &desc.frequency);
    int frequency_modifier_id = desc.frequency_modifier_id;
    sound_desc_changed |= ImGui::InputInt("##frequency_modifier_id", &frequency_modifier_id);
    desc.frequency_modifier_id = static_cast<uint8_t>(frequency_modifier_id);
    sound_desc_changed |= ImGui::SliderFloat("##frequency_modifier_params.begin", &desc.frequency_modifier_params.begin, 0.0f, 1.0f);
    sound_desc_changed |= ImGui::SliderFloat("##frequency_modifier_params.end", &desc.frequency_modifier_params.end, 0.0f, 1.0f);

    ImGui::Text("Amplitude");
    ImGui::SameLine();
    sound_desc_changed |= ImGui::InputFloat("##amplitude", &desc.amplitude);
    int amplitude_modifier_id = desc.amplitude_modifier_id;
    sound_desc_changed |= ImGui::InputInt("##amplitude_modifier_id", &amplitude_modifier_id);
    desc.amplitude_modifier_id = static_cast<uint8_t>(amplitude_modifier_id);
    sound_desc_changed |= ImGui::SliderFloat("##amplitude_modifier_params.begin", &desc.amplitude_modifier_params.begin, 0.0f, 1.0f);
    sound_desc_changed |= ImGui::SliderFloat("##amplitude_modifier_params.end", &desc.amplitude_modifier_params.end, 0.0f, 1.0f);
    
    if (ImGui::Button("Play Sound"))
    {
        int16_t* buffer = synth_generate_sound(&desc, 1);
        synth_queue_sound(buffer);
        Sleep(1000);
        delete[] buffer;
    }
}

int main()
{
    synth_init();
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

    unsigned int windowWidth = 800;
    unsigned int windowHeight = 600;
    sf::RenderWindow window(sf::VideoMode(windowWidth, windowHeight), "OpenGL", sf::Style::Default, settings);
    window.setVerticalSyncEnabled(true);
    window.setFramerateLimit(60);

    // activate the window
    window.setActive(true);
    ImGui::SFML::Init(window);

    // load resources, initialize the OpenGL states, ...

    // run the main loop
    bool running = true;
    sf::Vector3f clear_color;
    ADSR adsr = { .001f, 0.001f, 1.0f, 0.95f };
    synth_generate(adsr);

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
                windowWidth = event.size.width;
                windowHeight = event.size.height;
                glViewport(0, 0, event.size.width, event.size.height);
            }
        }
       
        ImGui::SFML::Update(window, deltaClock.restart());

        ImGui::SetNextWindowSize(ImVec2(static_cast<float>(windowWidth), static_cast<float>(windowHeight)));
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        {
            ImGuiWindowFlags flags = 0;
            flags |= ImGuiWindowFlags_NoDecoration;
            flags |= ImGuiWindowFlags_NoMove;
            flags |= ImGuiWindowFlags_NoScrollWithMouse;
            flags |= ImGuiWindowFlags_NoBackground;
            ImGui::Begin("Main", nullptr, flags);

            if (ImGui::Button("Play"))
            {
                synth_play();
            }
            {
                //ImGui::BeginChild("stuff", ImVec2(200.0f, 0.0f), true, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysUseWindowPadding);
                //
                //ImGui::Text("Envelope");                
                //
                //bool adsrChanged = false;
                //adsrChanged |= ImGui::VSliderFloat("##attack", ImVec2(20.0f, 100.0f), &adsr.a, 0.0f, 1.0f);
                //ImGui::SameLine();
                //adsrChanged |= ImGui::VSliderFloat("##decay", ImVec2(20.0f, 100.0f), &adsr.d, 0.0f, 1.0f);
                //ImGui::SameLine();
                //adsrChanged |= ImGui::VSliderFloat("##sustain", ImVec2(20.0f, 100.0f), &adsr.s, 0.0f, 1.0f);
                //ImGui::SameLine();
                //adsrChanged |= ImGui::VSliderFloat("##release", ImVec2(20.0f, 100.0f), &adsr.r, 0.0f, 1.0f);
                //
                //if (adsrChanged)
                //{
                //    synth_generate(adsr);
                //}
                
                //ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

                //const float values[] = { 0.2f, 0.0f, 0.4f, 0.5f };
                //ImVec2 mousePosRel;
                //bool mouseDown;
                //ImGui::PlotLines("Lines", values, 4, 0, "yadayada", 0.0f, 1.0f, ImVec2(0.0f, 100.0f), 4, &mousePosRel, &mouseDown);
                //if (mouseDown)
                //{
                //    std::cout << mousePosRel.x << " " << mousePosRel.y << std::endl;
                //}
                //ImGui::PlotHistogram("Hist", values, 4, 0, "yadayada", 0.0f, 1.0f, ImVec2(0.0f, 100.0f));

                //ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
                //ImGui::EndChild();
            }
            {
                ImGui::BeginChild("sound generator", ImVec2(800.0f, 800.0f), true, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysUseWindowPadding);
                imgui_sound_desc();
                ImGui::EndChild();
            }

            ImGui::End();
        }

        glClearColor(clear_color.x, clear_color.y, clear_color.z, 1.0f);
        // clear the buffers
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //ImGui::ShowDemoWindow();
        //window.draw(shape);
        ImGui::SFML::Render(window);

        // end the current frame (internally swaps the front and back buffers)
        window.display();
    }

    // release resources...
    synth_deinit();

    return 0;
}