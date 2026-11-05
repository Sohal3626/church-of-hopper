# Camera — Camera Movement & Input Control

C++20, GLM 및 GLFW를 사용하는 카메라 제어 모듈입니다.

Camera 모듈은 3D 공간에서 카메라의 위치와 방향을 관리하고, 키보드 및 마우스 입력을 카메라 이동과 회전에 반영합니다.

**Camera는 View Matrix를 계산하며, 입력 이벤트 연결은 별도의 CameraControl 모듈에서 담당합니다.**

## 1. 프로젝트 구성

```text
include/
└── camera/
    ├── Camera.h
    └── CameraControl.h

src/
└── camera/
    ├── Camera.cpp
    └── CameraControl.cpp
```

구현 파일 이름은 실제 프로젝트의 CMake 설정과 일치해야 합니다.

### 의존성

- C++20
- GLM — 벡터 및 행렬 연산
- GLFW — 키보드 및 마우스 입력

Camera 자체의 위치 및 방향 계산에는 OpenGL 버퍼 생성이 필요하지 않습니다.

## 2. 기본 사용법

### Camera 생성

```cpp
#include "camera/Camera.h"

Camera camera;
```

현재 Camera는 기본 생성자를 사용하며, 초기 설정은 클래스 내부에 정의되어 있습니다.

| 속성                | 초기값                |
| ----------------- | ------------------ |
| Position          | `(0.0, 1.5, -0.5)` |
| Front             | `(0.0, 0.0, -1.0)` |
| Up                | `(0.0, 1.0, 0.0)`  |
| Yaw               | `-90°`             |
| Pitch             | `0°`               |
| FOV               | `45°`              |
| Mouse Sensitivity | `0.1`              |
| Movement Speed    | `5.0`              |

현재 헤더에는 초기값을 외부에서 변경하는 생성자나 Setter가 제공되지 않습니다.

## 3. Camera API

### 공개 함수

| 함수                                      | 설명                     |
| --------------------------------------- | ---------------------- |
| `processKeyboard(direction, deltaTime)` | 카메라 이동 입력 처리           |
| `processMouse(xoffset, yoffset)`        | 마우스 이동에 따른 카메라 방향 갱신   |
| `getViewMatrix()`                       | 현재 카메라의 View Matrix 반환 |
| `getPosition()`                         | 카메라의 현재 위치 반환          |
| `getFov()`                              | 카메라의 FOV 반환            |

### 키보드 이동

```cpp
camera.processKeyboard(CameraMovement::Forward, deltaTime);
```

지원하는 이동 방향은 다음과 같습니다.

```cpp
enum class CameraMovement {
    Forward,
    Backward,
    Left,
    Right
};
```

`deltaTime`은 프레임 사이의 경과 시간으로, 프레임 속도에 따른 이동량 차이를 줄이는 데 사용합니다.

### 마우스 회전

```cpp
camera.processMouse(xoffset, yoffset);
```

마우스의 상대 이동량을 전달하여 카메라 방향을 갱신합니다.

`xoffset`과 `yoffset`은 화면상의 절대 좌표가 아니라 이전 입력 위치와의 차이입니다.

## 4. View Matrix

현재 Camera는 다음 방식으로 View Matrix를 계산합니다.

```cpp
glm::mat4 view = camera.getViewMatrix();
```

내부 계산은 `lookAt()`을 사용합니다.

```cpp
glm::lookAt(position, position + front, up);
```

이 행렬은 Shader의 View Uniform에 전달할 수 있습니다.

```cpp
shader.use();
shader.setMat4("view", camera.getViewMatrix());
```

View Matrix는 카메라의 위치 및 방향이 변하면 다시 계산해야 합니다.

## 5. GLFW 입력 연결

`CameraControl.h`는 GLFW 입력 처리를 위한 다음 함수를 선언합니다.

```cpp
void processInput(GLFWwindow* window, Camera& camera, float delta);
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
```

### 카메라 객체 연결

GLFW 콜백은 `GLFWwindow*`를 인자로 받습니다.

현재 프로젝트에서는 Window User Pointer를 사용하여 카메라 객체에 접근할 수 있습니다.

```cpp
Camera camera;

glfwSetWindowUserPointer(window, &camera);
```

콜백 등록:

```cpp
glfwSetCursorPosCallback(window, mouse_callback);
glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
```

카메라 객체는 해당 콜백이 사용하는 동안 유효해야 합니다.

### 키보드 입력 처리

렌더링 루프에서 호출합니다.

```cpp
processInput(window, camera, deltaTime);
```

현재 프로젝트에서 사용하는 키보드 매핑은 다음과 같습니다.

| 입력  | 동작       |
| --- | -------- |
| W   | Forward  |
| S   | Backward |
| A   | Left     |
| D   | Right    |
| ESC | 창 종료 요청  |

키보드 매핑을 변경하려면 Camera 자체보다는 입력 처리 구현을 수정합니다.

### 마우스 입력

마우스 콜백은 GLFW에서 입력된 커서 이동량을 카메라에 전달합니다.

마우스 커서를 화면 중앙에 고정하지 않고 게임 내 조작에 사용하려면 다음 설정을 사용할 수 있습니다.

```cpp
glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
```

## 6. Renderer 및 Shader 연동

Camera는 GPU 버퍼와 Shader Program을 직접 관리하지 않습니다.

Renderer에서 사용할 행렬과 카메라 위치를 제공합니다.

```cpp
shader.use();

shader.setMat4("view", camera.getViewMatrix());
shader.setVec3("viewPos", camera.getPosition());

renderer.draw(mesh, materials, shader);
```

### Projection Matrix

현재 Camera는 FOV 값을 제공하지만 Projection Matrix를 직접 생성하는 공개 함수는 없습니다.

따라서 호출자가 GLM을 사용하여 계산합니다.

```cpp
const glm::mat4 projection = glm::perspective(
    glm::radians(camera.getFov()),
    static_cast<float>(width) / height,
    0.1f,
    100.0f
);

shader.setMat4("projection", projection);
```

`width`와 `height`는 렌더링 영역의 크기이며, `height`는 0이 아니어야 합니다.

## 7. 프레임 시간 처리

프레임마다 경과 시간을 계산하여 Camera 입력 함수에 전달합니다.

```cpp
float lastFrame = 0.0f;

while (!glfwWindowShouldClose(window)) {
    const float currentFrame = static_cast<float>(glfwGetTime());
    const float deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    processInput(window, camera, deltaTime);

    // 렌더링 처리
}
```

`deltaTime`을 이용하면 렌더링 프레임 속도가 달라도 이동 속도를 시간 기준으로 계산할 수 있습니다.

## 8. 제약 및 주의사항

- 현재 Camera에는 위치, 방향, FOV를 직접 설정하는 공개 Setter가 없습니다.
- GLFW 입력 콜백은 등록한 Window와 유효한 Camera 객체가 필요합니다.
- Camera는 Projection Matrix를 직접 생성하지 않습니다.
- 현재 공개 API에는 카메라 이동 속도와 마우스 감도를 런타임에 변경하는 함수가 없습니다.
- 여러 Window에서 독립적인 마우스 입력 상태를 관리하려면 입력 상태 저장 방식을 별도로 고려해야 합니다.

## 9. 협업 시 사용 지침

일반적인 카메라 사용에는 다음 인터페이스만 필요합니다.

```cpp
Camera camera;

glfwSetWindowUserPointer(window, &camera);
glfwSetCursorPosCallback(window, mouse_callback);
glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

// 렌더링 루프
processInput(window, camera, deltaTime);

shader.use();
shader.setMat4("view", camera.getViewMatrix());
shader.setVec3("viewPos", camera.getPosition());
```

카메라의 방향 계산이나 GLFW 콜백 내부 구현을 직접 작성할 필요는 없습니다.

---

**모듈별 책임 요약**

- **Camera:** 카메라 위치, 방향, FOV 및 View Matrix
- **CameraControl:** GLFW 입력 처리 및 콜백
- **Shader:** View/Projection Uniform 전달
- **Renderer:** Mesh 및 Material 렌더링
