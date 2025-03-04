
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

#include <iostream>

#include <myprogram.hpp>
#include <objreader.hpp>
#include <intensity_map.hpp>

Program intensityProgram;
ObjData object;

// For MVP Uniforms
GLuint modelMatLoc;
GLuint viewMatLoc;
GLuint projMatLoc;
glm::vec3 eye(0, 0, 5);
const float PI = 3.14159265358979323846f;
float cameraTheta, cameraPhi;

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

// Phong Shading
GLuint lightPositionLoc;
GLuint eyePositionLoc;
GLuint lightColorLoc;
GLuint diffuseColorLoc;
GLuint specularColorLoc;
GLuint shininessLoc;

glm::vec3 lightPosition(10, 10, 5);
glm::vec3 lightColor(120);
float shininess = 12;

// Maybe for intensity?
GLuint vertexPositionBufferObject;
GLuint vertexNormalBufferObject;
GLuint normalVAO;
GLuint elementBufferObject;

// For estimating principal direction
Program normalPositionProgram;

GLuint normalTexture;
GLuint positionTexture;
GLuint geoPositionFrameBufferObject;
GLuint geoNormalFrameBufferObject;

Program positionProgram;

void init(GLFWwindow *window)
{
    intensityProgram.loadShader("phongShader.vert", "phongShader.frag");
    glUseProgram(intensityProgram.programID);

    object.loadObject("", "teapot.obj");

    glGenVertexArrays(1, &normalVAO);
    glBindVertexArray(normalVAO);

    glGenBuffers(1, &vertexPositionBufferObject);
    glBindBuffer(GL_ARRAY_BUFFER, vertexPositionBufferObject);
    glBufferData(GL_ARRAY_BUFFER, object.nVertices * sizeof(glm::vec3), object.vertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);

    glGenBuffers(1, &vertexNormalBufferObject);
    glBindBuffer(GL_ARRAY_BUFFER, vertexNormalBufferObject);
    glBufferData(GL_ARRAY_BUFFER, object.nSyncedNormals * sizeof(glm::vec3), object.syncedNormals.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);

    glGenBuffers(1, &elementBufferObject);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBufferObject);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, object.nElements3 * sizeof(glm::u16vec3), object.elements3.data(), GL_STATIC_DRAW);

    // intensities //

    lightPositionLoc = glGetUniformLocation(intensityProgram.programID, "lightPosition");
    eyePositionLoc = glGetUniformLocation(intensityProgram.programID, "eyePosition");
    lightColorLoc = glGetUniformLocation(intensityProgram.programID, "lightColor");
    diffuseColorLoc = glGetUniformLocation(intensityProgram.programID, "diffuseColor");
    specularColorLoc = glGetUniformLocation(intensityProgram.programID, "specularColor");
    shininessLoc = glGetUniformLocation(intensityProgram.programID, "shininess");

    // normals //

    normalPositionProgram.loadShader("normalShader.vert", "normalShader.frag");
    glGenTextures(1, &normalTexture);
    glBindTexture(GL_TEXTURE_2D, normalTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, 640, 480, 0, GL_RGB, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // 프레임 버퍼 생성
    glGenFramebuffers(1, &geoNormalFrameBufferObject);
    glBindFramebuffer(GL_FRAMEBUFFER, geoNormalFrameBufferObject);
    // 프레임 버퍼를 텍스쳐에 저장?
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, normalTexture, 0);
    // Specifies a list of color buffers to be drawn into
    GLenum normalDrawBuffer[] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, normalDrawBuffer);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "FBO Error\n";
    glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);

    // positions //

    glGenTextures(1, &positionTexture);
    glBindTexture(GL_TEXTURE_2D, positionTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, 1024, 1024, 0, GL_RGB, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenFramebuffers(1, &geoPositionFrameBufferObject);
    glBindFramebuffer(GL_FRAMEBUFFER, geoPositionFrameBufferObject);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, positionTexture, 0);
    GLenum posDrawBuffer[] = {GL_COLOR_ATTACHMENT1};
    glDrawBuffers(1, posDrawBuffer);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "FBO Error\n";
    glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
}

void render(GLFWwindow *window)
{
    int w, h;
    glfwGetFramebufferSize(window, &w, &h);

    glBindVertexArray(normalVAO);
    glBindBuffer(GL_ARRAY_BUFFER, elementBufferObject);

    glm::mat4 modelMat = glm::mat4({{1, 0, 0, 0},
                                    {0, 1, 0, 0},
                                    {0, 0, 1, 0},
                                    {0, -1.5, 0, 1}});

    glm::mat4 rotateX = glm::rotate(cameraPhi, glm::vec3(1, 0, 0));
    glm::mat4 rotateY = glm::rotate(cameraTheta, glm::vec3(0, 1, 0));
    glm::vec3 eyePosition = rotateX * rotateY * glm::vec4(eye, 1);
    glm::mat4 viewMat = glm::lookAt(eyePosition, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

    glm::mat4 projMat = glm::perspective(45 / (float)180 * 3.141592f, w / (float)h, 0.01f, 1000.0f);

    // normal

    glBindFramebuffer(GL_FRAMEBUFFER, geoNormalFrameBufferObject);

    glUseProgram(normalPositionProgram.programID);
    glBindVertexArray(normalVAO);
    glBindBuffer(GL_ARRAY_BUFFER, elementBufferObject);

    modelMatLoc = glGetUniformLocation(normalPositionProgram.programID, "modelMat");
    viewMatLoc = glGetUniformLocation(normalPositionProgram.programID, "viewMat");
    projMatLoc = glGetUniformLocation(normalPositionProgram.programID, "projMat");

    glUniformMatrix4fv(modelMatLoc, 1, GL_FALSE, glm::value_ptr(modelMat));
    glUniformMatrix4fv(viewMatLoc, 1, GL_FALSE, glm::value_ptr(viewMat));
    glUniformMatrix4fv(projMatLoc, 1, GL_FALSE, glm::value_ptr(projMat));

    glViewport(0, 0, w, h);

    glClearColor(0, 0.3, 0, 0);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDrawElements(GL_TRIANGLES, object.nElements3 * 3, GL_UNSIGNED_SHORT, 0);

    // intensities

    glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);

    glUseProgram(intensityProgram.programID);
    glBindVertexArray(normalVAO);
    glBindBuffer(GL_ARRAY_BUFFER, elementBufferObject);

    modelMatLoc = glGetUniformLocation(intensityProgram.programID, "modelMat");
    viewMatLoc = glGetUniformLocation(intensityProgram.programID, "viewMat");
    projMatLoc = glGetUniformLocation(intensityProgram.programID, "projMat");

    glViewport(0, 0, w, h);

    glClearColor(0, 0, 0, 0);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUniformMatrix4fv(modelMatLoc, 1, GL_FALSE, glm::value_ptr(modelMat));
    glUniformMatrix4fv(viewMatLoc, 1, GL_FALSE, glm::value_ptr(viewMat));
    glUniformMatrix4fv(projMatLoc, 1, GL_FALSE, glm::value_ptr(projMat));

    glUniform3fv(lightPositionLoc, 1, glm::value_ptr(lightPosition));
    glUniform3fv(eyePositionLoc, 1, glm::value_ptr(eyePosition));
    glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
    glUniform3fv(diffuseColorLoc, 1, glm::value_ptr(object.materialData[0].diffuseColor));
    glUniform3fv(specularColorLoc, 1, glm::value_ptr(object.materialData[0].specularColor));
    glUniform1f(shininessLoc, shininess);

    glDrawElements(GL_TRIANGLES, object.nElements3 * 3, GL_UNSIGNED_SHORT, 0);

    // normals
    glBindFramebuffer(GL_FRAMEBUFFER, geoNormalFrameBufferObject);

    glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE); // return to default

    glfwSwapBuffers(window);
}
