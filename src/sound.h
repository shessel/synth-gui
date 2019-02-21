#pragma once

struct ADSR
{
    float a;
    float d;
    float s;
    float r;
};

void sound_init();
void sound_deinit();
void sound_generate(ADSR adsr);
void sound_play();