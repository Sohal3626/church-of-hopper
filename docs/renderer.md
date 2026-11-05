# Renderer — OpenGL Rendering & Shader Management

C++20과 OpenGL 4.1을 사용하는 메시 렌더링 모듈입니다.

Renderer 모듈은 Resource 모듈에서 로드한 Mesh와 Material을 사용하여 GPU 리소스를 생성하고 화면에 렌더링하는 역할을 담당합니다.

**호출자는 VAO, VBO, EBO를 직접 생성하거나 OpenGL Draw Call을 작성할 필요가 없습니다.**

## 1. 프로젝트 구성

```text
include/
└── renderer/
    ├── Renderer.h
    └── Shader.h

src/
└── renderer/
    ├── Renderer.cpp
    └── Shader.cpp
```

### 의존성

- C++20
- OpenGL 4.1
- GLAD — OpenGL 함수 로딩
- GLM — 행렬 및 벡터 자료형
- Resource 모듈 — Mesh, Material, Texture

`Renderer.cpp`와 `Shader.cpp`는 프로젝트 빌드 대상에 포함되어야 합니다.

GPU 리소스를 생성하거나 사용하는 작업에는 유효한 OpenGL Context가 필요합니다.

## 2. 기본 사용법

### 객체 생성

```cpp
#include "renderer/Renderer.h"
#include "renderer/Shader.h"
#include "resource/Mesh.h"
#include "resource/Material.h"

Mesh gothic("data/gothic.obj");
Material materials("data/gothic.mtl");

Shader shader(vertexShaderPath, fragmentShaderPath);
Renderer renderer;
```

Shader와 Renderer는 OpenGL Context가 초기화된 이후 생성하고 사용해야 합니다.

### 메시 렌더링

```cpp
renderer.draw(gothic, materials, shader);
```

`draw()`는 필요한 GPU 버퍼와 텍스처를 내부에서 준비합니다.

Mesh나 Material을 GPU에 미리 업로드하기 위한 별도의 호출은 필요하지 않습니다.

## 3. Renderer

Renderer 클래스는 Mesh별 GPU 버퍼를 생성하고 관리하며 실제 Draw Call을 수행합니다.

### 공개 API

```cpp
Renderer();
~Renderer();

void draw(const Mesh& mesh, Material& materials, const Shader& shader);
```

| 함수 | 설명 |
|---|---|
| `Renderer()` | Renderer 객체 생성 |
| `~Renderer()` | 관리 중인 GPU 메시 버퍼 해제 |
| `draw()` | Mesh와 Material을 지정한 Shader로 렌더링 |

Renderer는 복사와 이동을 허용하지 않습니다.

### draw()

```cpp
void draw(const Mesh& mesh, Material& materials, const Shader& shader);
```

| 인자 | 설명 |
|---|---|
| `mesh` | 렌더링할 Mesh |
| `materials` | Mesh에 적용할 Material |
| `shader` | 렌더링에 사용할 Shader 프로그램 |

`materials`는 GPU 텍스처 업로드가 필요하므로 변경 가능한 참조로 전달합니다.

### 내부 처리 순서

`draw()`는 다음 작업을 수행합니다.

1. Mesh의 GPU 업로드 여부 확인
2. 업로드되지 않았다면 VAO/VBO/EBO 생성
3. Material이 관리하는 Texture 업로드
4. Shader 프로그램 활성화
5. VAO 바인딩
6. SubMesh별 Material 조회
7. Texture 및 Uniform 설정
8. `glDrawElements()` 호출
9. VAO 바인딩 해제

Texture 업로드에 실패하면 `std::runtime_error`를 발생시킵니다.

## 4. Mesh GPU 버퍼 관리

Renderer는 Mesh 하나당 다음 OpenGL 객체를 관리합니다.

| 객체 | 역할 |
|---|---|
| VAO | 정점 속성 설정 저장 |
| VBO | 정점 데이터 저장 |
| EBO | 인덱스 데이터 저장 |

내부 자료구조:

```cpp
struct MeshBuffer {
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;
};

std::unordered_map<const Mesh*, MeshBuffer> meshBuffers;
```

Mesh 객체의 주소를 키로 사용하여 해당 Mesh의 GPU 버퍼를 관리합니다.

### 자동 업로드

```cpp
renderer.draw(mesh, materials, shader);
```

최초 호출 시 Renderer는 `upload()`를 통해 GPU 버퍼를 생성합니다.

동일한 Mesh를 다시 그릴 때는 이미 생성된 GPU 버퍼를 재사용합니다.

```cpp
void upload(const Mesh& mesh);
```

`upload()`는 Renderer의 private 함수이므로 외부에서 직접 호출할 수 없습니다.

### 정점 속성

현재 Renderer는 다음 속성을 사용합니다.

| Location | Vertex 멤버 | 자료형 |
|---|---|---|
| 0 | `position` | `vec4` |
| 1 | `normal` | `vec4` |
| 2 | `texCoord` | `vec2` |

각 속성의 메모리 오프셋은 `offsetof()`를 사용하여 계산합니다.

**Vertex Shader의 입력 Location 및 자료형과 일치해야 합니다.**

## 5. SubMesh 렌더링

하나의 Mesh는 여러 SubMesh를 포함할 수 있습니다.

Renderer는 각 SubMesh를 순회하면서 재질과 텍스처를 적용합니다.

```cpp
for (const auto& [indexStart, indexCount, mtlName] : mesh.subMeshes) {
    const material& mat = materials.get(mtlName);

    // 재질 및 텍스처 적용
    // Draw Call
}
```

### 재질 조회

각 SubMesh의 `mtlName`을 사용하여 Material에서 해당 재질을 조회합니다.

```cpp
const material& mat = materials.get(mtlName);
```

재질이 존재하지 않으면 `Material::get()`이 제공하는 기본 재질을 사용합니다.

### Texture 바인딩

현재 구현에서는 확산 반사 텍스처를 텍스처 유닛 0에 바인딩합니다.

```cpp
if (mat.texture) {
    mat.texture->bind(0);
    shader.setBool("useTexture", true);
} else {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    shader.setBool("useTexture", false);
}
```

### Draw Call

SubMesh의 인덱스 범위를 사용하여 `glDrawElements()`를 호출합니다.

```cpp
const auto offset = static_cast<uintptr_t>(indexStart) * sizeof(uint32_t);

glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indexCount),
               GL_UNSIGNED_INT, reinterpret_cast<const void*>(offset));
```

`indexStart`는 인덱스 번호이므로 EBO의 바이트 오프셋으로 변환해야 합니다.

## 6. Shader

Shader 클래스는 Vertex Shader 및 Fragment Shader 파일을 읽어 OpenGL Shader Program을 생성하고 관리합니다.

### Shader 생성

```cpp
Shader shader("shaders/vshader.glsl", "shaders/fshader.glsl");
```

생성자:

```cpp
Shader(const std::string& vertexPath, const std::string& fragmentPath);
```

Shader 파일 경로는 현재 작업 디렉터리를 기준으로 해석됩니다.

프로젝트의 `getPath()`를 사용하면 프로젝트 소스 디렉터리를 기준으로 경로를 구성할 수 있습니다.

### Shader 활성화

```cpp
shader.use();
```

`use()`는 해당 Shader Program을 현재 렌더링에 사용할 프로그램으로 활성화합니다.

### Uniform 설정

Shader 클래스는 다음 Uniform 설정 함수를 제공합니다.

| 함수 | 값의 자료형 |
|---|---|
| `setBool(name, value)` | `bool` |
| `setInt(name, value)` | `int` |
| `setFloat(name, value)` | `float` |
| `setVec3(name, value)` | `glm::vec3` |
| `setMat4(name, value)` | `glm::mat4` |

사용 예시:

```cpp
shader.use();

shader.setMat4("projection", projection);
shader.setMat4("view", view);
shader.setMat4("model", model);

shader.setVec3("lightPos", glm::vec3(5.0f));
shader.setVec3("viewPos", camera.getPosition());
```

Uniform 이름은 GLSL 파일에 선언된 이름과 일치해야 합니다.

Uniform을 설정할 때는 해당 Shader Program이 활성화된 상태여야 합니다.

## 7. Renderer에서 사용하는 Uniform

현재 Renderer 구현은 다음 Uniform을 설정합니다.

| Uniform | 설명 |
|---|---|
| `texture_diffuse` | 확산 반사 텍스처의 샘플러 유닛 (0) |
| `useTexture` | 텍스처 사용 여부 |
| `material.diffuse` | 재질의 확산 반사 색상 |

해당 Uniform은 GLSL에서 실제로 선언하고 사용해야 합니다.

예시 Fragment Shader 인터페이스:

```glsl
#version 410 core

struct Material {
    vec3 diffuse;
};

uniform Material material;
uniform sampler2D texture_diffuse;
uniform bool useTexture;
```

위 코드는 Uniform 선언 예시이며 완전한 Fragment Shader는 아닙니다.

### 호출자가 설정할 Uniform

현재 Renderer는 다음 값을 자동으로 계산하지 않습니다.

- Model Matrix
- View Matrix
- Projection Matrix
- 카메라 위치
- 조명 위치

따라서 렌더링 전에 호출자가 필요한 Uniform을 설정해야 합니다.

## 8. 기본 렌더링 루프

```cpp
// OpenGL 및 GLAD 초기화 완료

{
    Camera camera;
    Renderer renderer;

    Mesh gothic(getPath("data/gothic.obj"));
    Material materials(getPath("data/gothic.mtl"));

    Shader shader(vertexShaderPath, fragmentShaderPath);
    glm::mat4 model(1.0f);

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const glm::mat4 projection = glm::perspective(
            glm::radians(camera.getFov()),
            static_cast<float>(SCR_WIDTH) / SCR_HEIGHT,
            0.1f, 100.0f
        );

        const glm::mat4 view = camera.getViewMatrix();

        shader.use();

        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
        shader.setMat4("model", model);

        shader.setVec3("lightPos", glm::vec3(5.0f));
        shader.setVec3("viewPos", camera.getPosition());

        renderer.draw(gothic, materials, shader);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

// GPU 리소스 해제 후 OpenGL 종료
glfwDestroyWindow(window);
glfwTerminate();
```

이 예시는 기본 렌더링 흐름을 설명하며 GLFW 초기화, 콜백 등록, 입력 처리 및 `glEnable(GL_DEPTH_TEST)` 등의 설정은 생략했습니다.

## 9. 리소스 수명 관리

Renderer와 Shader는 OpenGL GPU 리소스를 소유합니다.

| 클래스 | 소유 리소스 |
|---|---|
| Renderer | VAO, VBO, EBO |
| Shader | Shader Program |
| Texture | OpenGL Texture 및 CPU 이미지 데이터 |

각 객체의 소멸자가 GPU 리소스를 해제합니다.

**GPU 리소스를 소유하는 객체는 OpenGL Context가 파괴되기 전에 소멸해야 합니다.**

```cpp
{
    Renderer renderer;
    Shader shader(vertexPath, fragmentPath);
    Material materials(mtlPath);

    // 렌더링
}

// GPU 리소스 소멸 완료
glfwDestroyWindow(window);
glfwTerminate();
```

Renderer는 복사 및 이동이 금지되어 있으므로 하나의 Renderer가 자신이 생성한 GPU 버퍼를 소유합니다.

## 10. 제약 및 주의사항

### Mesh 캐시

현재 Renderer는 Mesh 객체의 주소를 캐시 키로 사용합니다.

따라서 다음 사항에 주의해야 합니다.

- GPU에 업로드한 Mesh를 이동하거나 파괴하면 캐시 키가 유효하지 않을 수 있습니다.
- Mesh의 정점 또는 인덱스 데이터를 변경해도 GPU 버퍼는 자동으로 갱신되지 않습니다.
- 개별 Mesh의 GPU 버퍼를 명시적으로 제거하는 공개 API는 없습니다.
- Renderer는 GPU 버퍼가 필요할 때 생성하지만, 현재 구조에서는 소멸 시 전체 버퍼를 해제합니다.

### SubMesh

현재 구현은 `mesh.subMeshes`를 순회하며 그리기 때문에, 목록이 비어 있으면 Draw Call을 수행하지 않습니다.

### Material

현재 Renderer는 SubMesh의 `mtlName`을 기준으로 재질을 선택합니다.

재질 이름이 일치하지 않으면 기본 재질이 반환됩니다.

`Material::uploadTextures()`는 렌더링 중 호출되며, 현재 Texture 구현은 최초 GPU 업로드 후 재전송을 생략합니다.

### Shader

- Shader 파일은 실행 시 접근 가능한 경로에 있어야 합니다.
- GLSL의 Uniform 이름은 C++ 코드와 일치해야 합니다.
- Shader 프로그램은 유효한 OpenGL Context 안에서 생성 및 사용해야 합니다.

### OpenGL 상태

Renderer는 렌더링에 필요한 일부 OpenGL 상태를 변경합니다.

현재 구현이 모든 OpenGL 상태를 호출 전 상태로 복원하는 것은 아닙니다.

다른 렌더링 코드와 함께 사용할 경우 텍스처 유닛 및 Shader 프로그램 등의 상태를 고려해야 합니다.

## 11. 향후 확장

프로젝트 요구사항에 따라 다음 기능을 추가할 수 있습니다.

- Scene 관리 인터페이스
- 모델별 Transform 관리
- Runtime Material Switching
- Colored Shadow Mapping
- Volumetric Ray Marching
- GPU 버퍼 업데이트 및 개별 해제
- 다중 렌더링 패스 지원

이 기능들은 현재 공개 API에 포함되어 있지 않습니다.

## 12. 협업 시 사용 지침

기본적인 메시 렌더링에 필요한 호출은 다음과 같습니다.

```cpp
Mesh mesh(objPath);
Material materials(mtlPath);
Shader shader(vertexPath, fragmentPath);

Renderer renderer;

renderer.draw(mesh, materials, shader);
```

GPU 버퍼 생성, SubMesh 렌더링 및 GPU 리소스 해제를 직접 구현할 필요는 없습니다.

모델을 추가하거나 렌더링할 때 Renderer의 내부 구현을 수정하지 말고 공개 API를 사용해 주세요.

---

**모듈별 책임 요약**

- **Resource:** OBJ/MTL/Texture 로딩 및 Texture GPU 관리
- **Renderer:** Mesh GPU 버퍼 관리 및 Draw Call
- **Shader:** GLSL 프로그램 생성 및 Uniform 설정
- **Camera:** 카메라 위치, 방향 및 View Matrix 계산
- **CamInput:** GLFW 입력을 Camera에 전달
