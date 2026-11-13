//
// Created by Sohal03 on 26. 11. 5..
//
#include "renderer/Shader.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

using namespace glm;
using namespace std;

Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath) {
    const string vertexProgram = readShaderFile(vertexPath);
    const string fragmentProgram = readShaderFile(fragmentPath);

    const GLuint vertex = compileShader(GL_VERTEX_SHADER, vertexProgram);
    const GLuint fragment = compileShader(GL_FRAGMENT_SHADER, fragmentProgram);

    if (vertex == 0 || fragment == 0) {
        if (vertex != 0) glDeleteShader(vertex);
        if (fragment != 0) glDeleteShader(fragment);
        throw runtime_error("Shader compilation failed");
    }

    programID = glCreateProgram();
    glAttachShader(programID, vertex);
    glAttachShader(programID, fragment);
    glLinkProgram(programID);

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    GLint success = GL_FALSE;
    glGetProgramiv(programID, GL_LINK_STATUS, &success);

    if (success != GL_TRUE) {
        GLint logLength = 0;
        glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &logLength);

        vector<GLchar> log(logLength > 0 ? logLength : 1);
        glGetProgramInfoLog(programID, static_cast<GLsizei>(log.size()), nullptr, log.data());

        glDeleteProgram(programID);
        programID = 0;

        throw runtime_error("Shader program linking failed:\n" + string(log.data()));
    }
}

Shader::~Shader() {
    if (programID != 0)
        glDeleteProgram(programID);
}

void Shader::use() const {
    glUseProgram(programID);
}

void Shader::setBool(const std::string& name, const bool value) const {
    glUniform1i(glGetUniformLocation(programID, name.c_str()), value);
}

void Shader::setInt(const string& name, const int value) const {
    glUniform1i(glGetUniformLocation(programID, name.c_str()), value);
}

void Shader::setFloat(const string& name, const float value) const {
    glUniform1f(glGetUniformLocation(programID, name.c_str()), value);
}

void Shader::setVec3(const string& name, const vec3& value) const {
    glUniform3fv(glGetUniformLocation(programID, name.c_str()), 1, value_ptr(value));
}

void Shader::setMat4(const string& name, const mat4& value) const {
    glUniformMatrix4fv(glGetUniformLocation(programID, name.c_str()), 1,GL_FALSE, value_ptr(value));
}

string Shader::readShaderFile(const string& path) {
    ifstream file(path);
    if (!file.is_open()) throw runtime_error("Failed to open shader file: " + path);

    stringstream buffer;
    buffer << file.rdbuf();

    if (file.bad()) throw runtime_error("Failed to read shader file: " + path);
    return buffer.str();
}

GLuint Shader::compileShader(const GLenum type, const string& source) {
    const GLuint shader = glCreateShader(type);

    const char* code = source.c_str();

    glShaderSource(shader, 1, &code, nullptr);
    glCompileShader(shader);

    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (success != GL_TRUE) {
        GLint logLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

        vector<GLchar> log(logLength > 0 ? logLength : 1);
        glGetShaderInfoLog(shader, static_cast<GLsizei>(log.size()), nullptr, log.data());

        const string message(log.data());

        cerr << "Shader compilation failed:\n" << message << endl;
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}
