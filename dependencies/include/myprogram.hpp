// 아주대 신현준 교수님 toys.h 참고

// 중복 include 방지.
#pragma once

// 프로그램 선언을 위한 OpenGL 인클루드
#include <GL/glew.h>

#include <fstream>
#include <iostream>

struct Program
{
    GLuint programID = 0;
    GLuint vertexShaderID = 0;
    GLuint fragShaderID = 0;

    std::string loadText(const char *filename)
    {
        std::fstream file(filename);
        if (!file.is_open())
        {
            std::cerr << filename << " File Not Found" << std::endl;
            return "";
        }

        std::istreambuf_iterator<char> begin(file), end;
        return std::string(begin, end);
    }

    void loadShader(const char *vShaderFile, const char *fShaderFile)
    {
        cleanUp();

        // Read Shader File
        // c_str()은 const char * 값을 반환.
        // Text로 받지 않으면 dangling pointer 발생.
        std::string vShaderText = loadText(vShaderFile);
        // vertex shader는 있어야 함.
        if (vShaderText.length() < 1)
        {
            std::cerr << "Vertex shader has no code" << std::endl;
            return;
        }
        std::string fShaderText = loadText(fShaderFile);

        // Create Program
        programID = glCreateProgram();
        // Create Shader by its type
        vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
        fragShaderID = glCreateShader(GL_FRAGMENT_SHADER);

        // 쉐이더 아이디에 코드 등록.
        // 두번쨰 인자를 통해 코드가 여러 개 들어올 수 있다. 따라서 코드의 포인터를 넣는 것.
        // 소스 등록 -> 컴파일 -> 붙이기
        const GLchar *vShaderCode = vShaderText.c_str();
        glShaderSource(vertexShaderID, 1, &vShaderCode, 0);
        glCompileShader(vertexShaderID);
        glAttachShader(programID, vertexShaderID);

        const GLchar *fShaderCode = fShaderText.c_str();
        glShaderSource(fragShaderID, 1, &fShaderCode, 0);
        glCompileShader(fragShaderID);
        glAttachShader(programID, fragShaderID);

        // 다 붙이면 링크 후 사용 등록
        glLinkProgram(programID);
        glUseProgram(programID);

        std::cout << "Program " << programID << " created" << std::endl;
    }
    void cleanUp()
    {
        // Delete all programs
        if (programID)
            glDeleteProgram(programID);
        if (vertexShaderID)
            glDeleteShader(vertexShaderID);
        if (fragShaderID)
            glDeleteShader(fragShaderID);

        // value reset
        programID = vertexShaderID = fragShaderID = 0;
    }
    ~Program()
    {
        cleanUp();
    }
};
