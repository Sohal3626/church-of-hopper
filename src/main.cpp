#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

// 경로 맞춰주는 매크로
#ifndef PROJECT_SOURCE_DIR
#define PROJECT_SOURCE_DIR "."
#endif

inline std::string getPath(const std::string& relativePath) {
    return std::string(PROJECT_SOURCE_DIR) + "/" + relativePath;
}

#include "dataLoader/stb_image.h"

// 창 크기
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// 카메라 초기값 (높이 1.5)
glm::vec3 cameraPos = glm::vec3(0.0f, 1.5f, -0.5f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

bool firstMouse = true;
float yaw = -90.0f;
float pitch = 0.0f;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
float fov = 45.0f;

// 프레임 시간
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// 콜백 함수들
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
unsigned int loadTexture(const char* path);
unsigned int createShaderProgram(const char* vertexPath, const char* fragmentPath);

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

// obj 파일 파싱 (다각형 삼각분할 포함)
bool loadSimpleOBJ(const std::string& path, std::vector<Vertex>& out_vertices) {
    std::vector<glm::vec3> temp_positions;
    std::vector<glm::vec2> temp_uvs;
    std::vector<glm::vec3> temp_normals;

    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[오류] OBJ 못열음: " << path << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string prefix;
        ss >> prefix;

        if (prefix == "v") {
            glm::vec3 pos;
            ss >> pos.x >> pos.y >> pos.z;
            temp_positions.push_back(pos);
        }
        else if (prefix == "vt") {
            glm::vec2 uv;
            ss >> uv.x >> uv.y;
            temp_uvs.push_back(uv);
        }
        else if (prefix == "vn") {
            glm::vec3 norm;
            ss >> norm.x >> norm.y >> norm.z;
            temp_normals.push_back(norm);
        }
        else if (prefix == "f") {
            std::vector<Vertex> faceVertices;
            std::string vStr;

            while (ss >> vStr) {
                unsigned int pIdx = 0, uIdx = 0, nIdx = 0;
                std::stringstream vSS(vStr);
                std::string token;

                std::vector<int> indices;
                while (std::getline(vSS, token, '/')) {
                    if (token.empty()) indices.push_back(0);
                    else indices.push_back(std::stoi(token));
                }

                if (indices.size() > 0) pIdx = indices[0];
                if (indices.size() > 1) uIdx = indices[1];
                if (indices.size() > 2) nIdx = indices[2];

                Vertex v{};
                if (pIdx > 0 && pIdx <= temp_positions.size()) v.Position = temp_positions[pIdx - 1];
                if (uIdx > 0 && uIdx <= temp_uvs.size())       v.TexCoords = temp_uvs[uIdx - 1];
                if (nIdx > 0 && nIdx <= temp_normals.size())   v.Normal = temp_normals[nIdx - 1];

                faceVertices.push_back(v);
            }

            // 사각형 이상이면 삼각형으로 쪼개기
            if (faceVertices.size() >= 3) {
                for (size_t i = 1; i < faceVertices.size() - 1; ++i) {
                    out_vertices.push_back(faceVertices[0]);
                    out_vertices.push_back(faceVertices[i]);
                    out_vertices.push_back(faceVertices[i + 1]);
                }
            }
        }
    }
    return true;
}

int main() {
    // glfw 초기화
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Church of Hopper", NULL, NULL);
    if (window == NULL) {
        std::cout << "창 생성 실패" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // 마우스 고정
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad 초기화
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "GLAD 초기화 실패" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    // 셰이더 불러오기
    std::string vsPath = getPath("src/dataLoader/vshader.glsl");
    std::string fsPath = getPath("src/dataLoader/fshader.glsl");

    unsigned int shaderProgram = createShaderProgram(vsPath.c_str(), fsPath.c_str());
    if (shaderProgram == 0) {
        std::cerr << "[오류] 셰이더 생성 실패" << std::endl;
        return -1;
    }

    // 모델이랑 텍스처 로드
    std::vector<Vertex> cathedralVertices;
    if (!loadSimpleOBJ(getPath("data/gothic.obj"), cathedralVertices)) {
        std::cerr << "[경고] obj 로딩 실패" << std::endl;
    }

    unsigned int texture = loadTexture(getPath("data/gothic.png").c_str());

    // 버퍼 바인딩
    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, cathedralVertices.size() * sizeof(Vertex), cathedralVertices.data(), GL_STATIC_DRAW);

    // 정점 속성 세팅
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

    glBindVertexArray(0);

    glUseProgram(shaderProgram);
    glUniform1i(glGetUniformLocation(shaderProgram, "texture_diffuse"), 0);

    // 루프 시작
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        // 행렬 계산
        glm::mat4 projection = glm::perspective(glm::radians(fov), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 model = glm::mat4(1.0f);

        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

        // 광원 & 시선 위치 넘기기
        glUniform3f(glGetUniformLocation(shaderProgram, "lightPos"), 5.0f, 5.0f, 5.0f);
        glUniform3f(glGetUniformLocation(shaderProgram, "viewPos"), cameraPos.x, cameraPos.y, cameraPos.z);

        // 그리기
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(cathedralVertices.size()));
        glBindVertexArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 정리
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}

// 셰이더 파일 파일 로드 및 컴파일
unsigned int createShaderProgram(const char* vertexPath, const char* fragmentPath) {
    std::string vertexCode;
    std::string fragmentCode;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;

    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try {
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);
        std::stringstream vShaderStream, fShaderStream;

        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();

        vShaderFile.close();
        fShaderFile.close();

        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();
    }
    catch (std::ifstream::failure& e) {
        std::cerr << "[오류] 셰이더 읽기 실패\n"
            << " - VS: " << vertexPath << "\n"
            << " - FS: " << fragmentPath << std::endl;
        return 0;
    }

    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    int success;
    char infoLog[512];

    // 버텍스 셰이더
    unsigned int vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex, 512, NULL, infoLog);
        std::cerr << "VS 컴파일 에러:\n" << infoLog << std::endl;
        return 0;
    }

    // 프래그먼트 셰이더
    unsigned int fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        std::cerr << "FS 컴파일 에러:\n" << infoLog << std::endl;
        return 0;
    }

    // 셰이더 프로그램 링크
    unsigned int program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "프로그램 링크 에러:\n" << infoLog << std::endl;
        return 0;
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    return program;
}

// WASD 키보드 이동 (Y축 높이 1.5 고정)
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float cameraSpeed = static_cast<float>(5.0 * deltaTime);

    // 수평 이동용 벡터
    glm::vec3 frontXZ = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z));

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * frontXZ;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * frontXZ;

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(frontXZ, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(frontXZ, cameraUp)) * cameraSpeed;

    // 높이 고정
    cameraPos.y = 1.5f;
}

// 마우스 회전
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)  pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

// 창 크기 조절
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// 이미지 텍스처 로더
unsigned int loadTexture(const char* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format = GL_RGB;
        if (nrComponents == 1)      format = GL_RED;
        else if (nrComponents == 3) format = GL_RGB;
        else if (nrComponents == 4) format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else {
        std::cout << "[경고] 텍스처 로딩 실패: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}