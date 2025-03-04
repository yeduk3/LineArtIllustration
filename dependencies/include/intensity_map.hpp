
#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>

void init(GLFWwindow *window);
void render(GLFWwindow *window);
void cursorPosCallback(GLFWwindow *window, double xpos, double ypos);
