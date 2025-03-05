#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>

// #include <intensity_map.hpp>
// #include <tam_generator.hpp>

#include <myprogram.hpp>
#include <objreader.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

//
// init
//
Program normalPositionProgram, pdProgram;
ObjData obj;

GLuint normalVAO;
GLuint positionVBO, normalVBO;
GLuint vertexElement;

GLuint normalPositionFB;
GLuint normalTexture, positionTexture;
GLenum norposTexDrawBuffers[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
GLuint renderBufferobject;

// pd1: umbilic unsolved, pd2: umbilic solved.
GLuint pd1FB;
GLuint pd1Texture, pd2Texture;
GLuint pdVAO;
GLuint pdQuadVBO;
GLenum pdTexDrawBuffers[1] = {GL_COLOR_ATTACHMENT2};

// test

Program testProgram;

GLuint testVAO;
GLuint testQuadVBO;

float quadVertices[] = {
    // positions   // texCoords
    -1.0f, 1.0f, 0.0f, 1.0f,
    -1.0f, -1.0f, 0.0f, 0.0f,
    1.0f, -1.0f, 1.0f, 0.0f,

    -1.0f, 1.0f, 0.0f, 1.0f,
    1.0f, -1.0f, 1.0f, 0.0f,
    1.0f, 1.0f, 1.0f, 1.0f};

// Run Gen-Bind-Data Buffer with GL_STATIC_DRAW
// `glGenBuffers(1, buffer);`
// `glBindBuffer(target, *buffer);`
// `glBufferData(target, memSize, data, GL_STATIC_DRAW);`
void glGBDArrayBuffer(GLenum target, GLuint *buffer, GLsizeiptr memSize, const void *data)
{
    glGenBuffers(1, buffer);
    glBindBuffer(target, *buffer);
    glBufferData(target, memSize, data, GL_STATIC_DRAW);
}

// Run Gen-Bind Texture(s) and TexImage2D-TexParameteri with some default settings
// `glGenTextures(1, texture);`
// `glBindTexture(GL_TEXTURE_2D, *texture);`
// `glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);`
// `glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);`
// `glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);`
void glGBIPTexture2D(GLuint *texture, int w, int h)
{
    glGenTextures(1, texture);
    glBindTexture(GL_TEXTURE_2D, *texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

// movement

const float PI = 3.14159265358979323846f;
float cameraTheta(0), cameraPhi(0);

namespace comparator
{
    float min(const float &a, const float &b)
    {
        return a > b ? b : a;
    }
    float max(const float &a, const float &b)
    {
        return a > b ? a : b;
    }
}

void cursorPosCallback(GLFWwindow *window, double xpos, double ypos)
{
    static double lastX = 0;
    static double lastY = 0;
    // when left mouse button clicked
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1))
    {
        double dx = xpos - lastX;
        double dy = ypos - lastY;
        int w, h;
        glfwGetWindowSize(window, &w, &h);
        // rotate 180 degree per each width/height dragging
        cameraTheta -= dx / w * PI; // related with y-axis rotation
        cameraPhi -= dy / h * PI;   // related with x-axis rotation
        cameraPhi = comparator::max(-PI / 2 + 0.01f, comparator::min(cameraPhi, PI / 2 - 0.01f));
        // printf("%.3f %.3f\n", cameraTheta, cameraPhi);
    }
    // whenever, save current cursor position as previous one
    lastX = xpos;
    lastY = ypos;
}

float testOffset = 0.00001;
float testOffsetDelta = 0.00001;
bool enableCaseTest = false;
float testCloseToZero = 0.0005;
float testCloseToZeroDelta = 0.0001;

void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_RIGHT && action > GLFW_RELEASE)
    {
        testOffset = comparator::min(0.1, comparator::max(0, testOffset + testOffsetDelta));
        std::cout << "Offset: " << testOffset << std::endl;
    }
    else if (key == GLFW_KEY_LEFT && action > GLFW_RELEASE)
    {
        testOffset = comparator::min(0.1, comparator::max(0, testOffset - testOffsetDelta));
        std::cout << "Offset: " << testOffset << std::endl;
    }
    else if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    {
        enableCaseTest = !enableCaseTest;
    }
    else if (key == GLFW_KEY_UP && action > GLFW_RELEASE)
    {
        testCloseToZero = comparator::min(0.1, comparator::max(0, testCloseToZero + testCloseToZeroDelta));
        std::cout << "CloseToZero: " << testCloseToZero << std::endl;
    }
    else if (key == GLFW_KEY_DOWN && action > GLFW_RELEASE)
    {
        testCloseToZero = comparator::min(0.1, comparator::max(0, testCloseToZero - testCloseToZeroDelta));
        std::cout << "CloseToZero: " << testCloseToZero << std::endl;
    }
}

void pdInit(GLFWwindow *window)
{
    obj.loadObject("obj/", "teapot.obj");

    //
    // Normal & Position program
    //

    normalPositionProgram.loadShader("normalPosition.vert", "normalPosition.frag");
    glUseProgram(normalPositionProgram.programID);

    glGenVertexArrays(1, &normalVAO);
    glBindVertexArray(normalVAO);

    glGBDArrayBuffer(GL_ARRAY_BUFFER, &positionVBO, sizeof(glm::vec3) * obj.nVertices, obj.vertices.data());

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);

    glGBDArrayBuffer(GL_ARRAY_BUFFER, &normalVBO, sizeof(glm::vec3) * obj.nSyncedNormals, obj.syncedNormals.data());

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);

    glGBDArrayBuffer(GL_ELEMENT_ARRAY_BUFFER, &vertexElement, sizeof(glm::u16vec3) * obj.nElements3, obj.elements3.data());

    // framebuffer

    int w, h;
    glfwGetFramebufferSize(window, &w, &h);

    glGenFramebuffers(1, &normalPositionFB);
    glBindFramebuffer(GL_FRAMEBUFFER, normalPositionFB);

    glGBIPTexture2D(&normalTexture, w, h);
    glGBIPTexture2D(&positionTexture, w, h);

    // each attachment are mapped into `layout(location = ?) out ~~`
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, normalTexture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, positionTexture, 0);
    // `glDrawBuffers` set the buffer list to be drawn
    glDrawBuffers(2, norposTexDrawBuffers);

    glGenRenderbuffers(1, &renderBufferobject);
    glBindRenderbuffer(GL_RENDERBUFFER, renderBufferobject);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, renderBufferobject);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;

    //
    // umbilic unsolved principal direction
    //

    pdProgram.loadShader("pd.vert", "pd.frag");
    glUseProgram(pdProgram.programID);

    glGenFramebuffers(1, &pd1FB);
    glBindFramebuffer(GL_FRAMEBUFFER, pd1FB);

    glGBIPTexture2D(&pd1Texture, w, h);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, pd1Texture, 0);
    glDrawBuffers(1, pdTexDrawBuffers);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout
            << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;

    glGenVertexArrays(1, &pdVAO);
    glBindVertexArray(pdVAO);

    glGBDArrayBuffer(GL_ARRAY_BUFFER, &pdQuadVBO, 24 * sizeof(float), quadVertices);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));

    //
    // try to solve umbolic points - uncompleted
    //

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    testProgram.loadShader("test.vert", "test.frag");
    glUseProgram(testProgram.programID);

    glGenVertexArrays(1, &testVAO);
    glBindVertexArray(testVAO);

    glGBDArrayBuffer(GL_ARRAY_BUFFER, &testQuadVBO, 24 * sizeof(float), quadVertices);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
}

//
// render
//

void pdRender(GLFWwindow *window)
{

    glm::mat4 modelMat = glm::mat4({{1, 0, 0, 0},
                                    {0, 1, 0, 0},
                                    {0, 0, 1, 0},
                                    {0, -1.5, 0, 1}});
    // modelMat = glm::scale(glm::vec3(0.01)) * modelMat;
    glm::mat4 rotateX = glm::rotate(cameraPhi, glm::vec3(1, 0, 0));
    glm::mat4 rotateY = glm::rotate(cameraTheta, glm::vec3(0, 1, 0));
    glm::vec3 eye(0, 0, 5);
    glm::vec3 eyePosition = rotateY * rotateX * glm::vec4(eye, 1);
    glm::mat4 viewMat = glm::lookAt(eyePosition, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    glm::mat4 projMat = glm::perspective(45 / (float)180 * PI, w / (float)h, 0.01f, 1000.0f);

    //
    // Normal & Position texture render
    //

    glUseProgram(normalPositionProgram.programID);

    glBindFramebuffer(GL_FRAMEBUFFER, normalPositionFB);
    glBindVertexArray(normalVAO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertexElement);

    glViewport(0, 0, w, h);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    GLuint modelMatLoc = glGetUniformLocation(normalPositionProgram.programID, "modelMat");
    GLuint viewMatLoc = glGetUniformLocation(normalPositionProgram.programID, "viewMat");
    GLuint projMatLoc = glGetUniformLocation(normalPositionProgram.programID, "projMat");
    glUniformMatrix4fv(modelMatLoc, 1, GL_FALSE, glm::value_ptr(modelMat));
    glUniformMatrix4fv(viewMatLoc, 1, GL_FALSE, glm::value_ptr(viewMat));
    glUniformMatrix4fv(projMatLoc, 1, GL_FALSE, glm::value_ptr(projMat));

    glDrawElements(GL_TRIANGLES, obj.nElements3 * 3, GL_UNSIGNED_SHORT, 0);

    //
    // Principal Direction 1 render
    //

    glUseProgram(pdProgram.programID);

    glBindFramebuffer(GL_FRAMEBUFFER, pd1FB);
    glBindVertexArray(pdVAO);
    glBindBuffer(GL_ARRAY_BUFFER, pdQuadVBO);

    glViewport(0, 0, w, h);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    // value below (0 and 1) is same with GL_TEXTURE{value}
    GLuint normTexLoc = glGetUniformLocation(pdProgram.programID, "normalTexture");
    glUniform1i(normTexLoc, 0);
    GLuint posTexLoc = glGetUniformLocation(pdProgram.programID, "positionTexture");
    glUniform1i(posTexLoc, 1);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, normalTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, positionTexture);

    // test values
    GLuint OFFSETLoc = glGetUniformLocation(pdProgram.programID, "OFFSET");
    glUniform1f(OFFSETLoc, testOffset);
    GLuint enableCaseTestLoc = glGetUniformLocation(pdProgram.programID, "enableCaseTest");
    glUniform1i(enableCaseTestLoc, enableCaseTest);
    GLuint closeToZeroLoc = glGetUniformLocation(pdProgram.programID, "CLOSETOZERO");
    glUniform1f(closeToZeroLoc, testCloseToZero);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    //
    // test render
    //

    glUseProgram(testProgram.programID);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glBindVertexArray(testVAO);

    glViewport(0, 0, w, h);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    GLuint pdTextureLoc = glGetUniformLocation(testProgram.programID, "pdTexture");
    glUniform1i(pdTextureLoc, 2);
    OFFSETLoc = glGetUniformLocation(testProgram.programID, "OFFSET");
    glUniform1f(OFFSETLoc, testOffset);
    enableCaseTestLoc = glGetUniformLocation(testProgram.programID, "enableCaseTest");
    glUniform1i(enableCaseTestLoc, enableCaseTest);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, pd1Texture);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glfwSwapBuffers(window);
}

//
// main
//

int main()
{

    // init

    if (!glfwInit())
    {
        std::cout << "GLFW Init Failed" << std::endl;
        return -1;
    }

// If MacOS
#ifdef __APPLE__
    // Get OpenGL Version 410
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    // forward compatibility: necessary for OS X
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    // set as core profile
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif

    GLFWwindow *window = glfwCreateWindow(640, 480, "Line Art Illustration", 0, 0);
    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK)
    {
        std::cout << "GLEW Init Failed" << std::endl;
        return -1;
    }

    // init(window);
    pdInit(window);

    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetKeyCallback(window, keyCallback);

    // render

    while (!glfwWindowShouldClose(window))
    {
        // render(window);
        pdRender(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
