#include "Shader.h"
#include <glad/glad.h>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

Shader::Shader(const char* vertexPath, const char* fragmentPath) : ID(0) {
    std::string vertexCode;
    std::string fragmentCode;

    auto readShaderFile = [](const char* path) {
        std::ifstream file(path);
        if (!file) {
            throw std::runtime_error("Failed to open shader file: " + std::string(path));
        }

        std::ostringstream contents;
        contents << file.rdbuf();
        if (file.bad()) {
            throw std::runtime_error("Failed to read shader file: " + std::string(path));
        }
        return contents.str();
    };

    vertexCode = readShaderFile(vertexPath);
    fragmentCode = readShaderFile(fragmentPath);

    unsigned int vertex = 0;
    unsigned int fragment = 0;

    try {
        vertex = glCreateShader(GL_VERTEX_SHADER);
        const char* vertexSource = vertexCode.c_str();
        glShaderSource(vertex, 1, &vertexSource, nullptr);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");

        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        const char* fragmentSource = fragmentCode.c_str();
        glShaderSource(fragment, 1, &fragmentSource, nullptr);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");

        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");

        glDeleteShader(vertex);
        glDeleteShader(fragment);
    } catch (...) {
        if (vertex != 0) {
            glDeleteShader(vertex);
        }
        if (fragment != 0) {
            glDeleteShader(fragment);
        }
        if (ID != 0) {
            glDeleteProgram(ID);
            ID = 0;
        }
        throw;
    }
}

Shader::~Shader() {
    if (ID != 0) {
        glDeleteProgram(ID);
    }
}

void Shader::use() {
    glUseProgram(ID);
}

void Shader::setMat4(const std::string& name, const glm::mat4& mat) const {
    glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}

void Shader::setInt(const std::string& name, int value) const {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::checkCompileErrors(unsigned int shader, std::string type) {
    int success = 0;
    char infoLog[1024] = {};

    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
            throw std::runtime_error(
                "Shader compilation failed (" + type + "):\n" + std::string(infoLog));
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
            throw std::runtime_error(
                "Shader program linking failed:\n" + std::string(infoLog));
        }
    }
}
