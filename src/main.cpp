#include <SFML/Window.hpp>
#include <SFML/OpenGL.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui-SFML.h>

#include <vector>

#include "synth.h"

void imgui_sound_desc()
{
    static bool loop_playing = false;
    static std::vector<sound_desc> descs;

    if (ImGui::Button("Add Desc"))
    {
        if (descs.empty())
        {
            descs.emplace_back(sound_desc{
                0,              // base_sound_id
                1.0f,           // amplitude
                440.0f,         // frequency
                0.0f,           // amplitude_min
                80.0f,          // frequency_min
                0,              // frequency_modifier_id
                0,              // amplitude_modifier_id
                { 0.0f, 1.0f }, // frequency_modifier_params
                { 0.0f, 1.0f }, // amplitude_modifier_params
                });
        }
        else
        {
            descs.emplace_back(descs.back());
        }
    }

    bool any_sound_desc_changed = false;
    for (size_t i = 0; i < descs.size(); ++i)
    {
        auto& desc = descs[i];
        ImGui::PushID(static_cast<int>(i));
        bool sound_desc_changed = false;
        ImGui::Text("Wave Form");
        ImGui::SameLine();
        sound_desc_changed |= ImGui::InputScalar("##base_sound_id", ImGuiDataType_U32, &desc.base_sound_id);

        ImGui::Text("Frequency");
        ImGui::SameLine();
        sound_desc_changed |= ImGui::InputFloat("##frequency", &desc.frequency);
        ImGui::Text("Frequency Min");
        ImGui::SameLine();
        sound_desc_changed |= ImGui::InputFloat("##frequency_min", &desc.frequency_min);
        int frequency_modifier_id = desc.frequency_modifier_id;
        sound_desc_changed |= ImGui::InputInt("##frequency_modifier_id", &frequency_modifier_id);
        desc.frequency_modifier_id = static_cast<uint8_t>(frequency_modifier_id);
        sound_desc_changed |= ImGui::SliderFloat("##frequency_modifier_params.begin", &desc.frequency_modifier_params.begin, 0.0f, 1.0f);
        sound_desc_changed |= ImGui::SliderFloat("##frequency_modifier_params.end", &desc.frequency_modifier_params.end, 0.0f, 1.0f);

        ImGui::Text("Amplitude");
        ImGui::SameLine();
        sound_desc_changed |= ImGui::SliderFloat("##amplitude", &desc.amplitude, 0.0f, 1.0f);
        ImGui::Text("Amplitude Min");
        ImGui::SameLine();
        sound_desc_changed |= ImGui::SliderFloat("##amplitude_min", &desc.amplitude_min, 0.0f, 1.0f);
        int amplitude_modifier_id = desc.amplitude_modifier_id;
        sound_desc_changed |= ImGui::InputInt("##amplitude_modifier_id", &amplitude_modifier_id);
        desc.amplitude_modifier_id = static_cast<uint8_t>(amplitude_modifier_id);
        sound_desc_changed |= ImGui::SliderFloat("##amplitude_modifier_params.begin", &desc.amplitude_modifier_params.begin, 0.0f, 1.0f);
        sound_desc_changed |= ImGui::SliderFloat("##amplitude_modifier_params.end", &desc.amplitude_modifier_params.end, 0.0f, 1.0f);

        any_sound_desc_changed |= sound_desc_changed;
        ImGui::PopID();
    }
    
    if (ImGui::Button("Play Sound"))
    {
        int16_t* buffer = synth_generate_sound(descs.data(), static_cast<uint8_t>(descs.size()));
        synth_queue_sound(buffer);
        Sleep(1000);
        delete[] buffer;
    }

    ImGui::SameLine();
    static int16_t* buffer = nullptr;
    if (loop_playing)
    {
        if (ImGui::Button("Stop") && loop_playing)
        {
            synth_end_current_loop();
            if (buffer)
            {
                delete[] buffer;
                buffer = nullptr;
            }
            loop_playing = false;
        }
        else if (any_sound_desc_changed)
        {
            synth_update_generated_sound(buffer, descs.data(), static_cast<uint8_t>(descs.size()));
        }
    }
    else if (ImGui::Button("Play Sound Continuously"))
    {
        if (buffer)
        {
            synth_end_current_loop();
            delete[] buffer;
            buffer = nullptr;
        }
        buffer = synth_generate_sound(descs.data(), static_cast<uint8_t>(descs.size()));
        synth_queue_sound(buffer, static_cast<uint32_t>(-1));
        loop_playing = true;
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
    // imgui-sfml backend doesn't support modern GL backend, but disabling core allows old stuff
    // settings.attributeFlags = settings.Core;
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