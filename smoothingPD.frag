#version 410 core

subroutine vec4 renderPassType();
subroutine uniform renderPassType renderPass;

// pass #1

subroutine(renderPassType) vec4 pass1() {}

// pass #2

uniform sampler2D positionTex;
uniform sampler2D tex;
uniform vec2 inverseSize;

in vec2 texCoords;

out vec4 fragColor;

// kernel size in pixels. recommandation is 5~10.
const int KERNEL_SIZE = 5;
const vec2 direction[4] = vec2[4](vec2(0, 1), vec2(1, 0), vec2(0, -1), vec2(-1, 0));

subroutine(renderPassType) vec4 pass2() {}

void main() {
    fragColor = renderPass();
    fragColor = renderPass();
}