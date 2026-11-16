# Shadow — Point Light Shadow Mapping

C++20과 OpenGL 4.1을 사용하는 점광원 그림자 맵 모듈입니다.

Shadow 모듈은 점광원 위치에서 장면의 깊이를 큐브맵(6면)에 렌더링하고, 메인 셰이더가 `samplerCubeShadow`로 그림자 여부를 판정할 수 있도록 텍스처와 Uniform을 제공합니다.

**호출자는 FBO, 큐브맵 텍스처, 광원 View/Projection 행렬을 직접 만들 필요가 없습니다.**

## 1. 프로젝트 구성

```text
include/
└── renderer/
    └── ShadowMap.h

src/
└── renderer/
    └── ShadowMap.cpp

shaders/
├── shadow_depth.vert   // depth 패스 Vertex Shader
├── shadow_depth.frag   // depth 패스 Fragment Shader
└── fshader.glsl        // 메인 셰이더 (그림자 판정 포함)
```

### 의존성

- C++20
- OpenGL 4.1 / GLAD
- GLM — 행렬 및 벡터 자료형
- Renderer 모듈 — `Renderer::drawDepth()`, `Shader`
- Resource 모듈 — `Mesh`

`ShadowMap.cpp`는 `CMakeLists.txt`의 `add_executable` 대상에 포함되어 있어야 합니다.

GPU 리소스를 생성하므로 **OpenGL Context가 초기화된 이후** 생성해야 합니다.

## 2. 기본 사용법

```cpp
#include "renderer/ShadowMap.h"

glEnable(GL_DEPTH_TEST);
glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);   // 큐브맵 면 경계 이음새 방지

Renderer renderer;
Mesh gothic(getPath("data/gothic.obj"));
Shader shader(getPath("shaders/vshader.glsl"), getPath("shaders/fshader.glsl"));
Shader depthShader(getPath("shaders/shadow_depth.vert"),
                   getPath("shaders/shadow_depth.frag"));

const glm::mat4 model(1.0f);
const glm::vec3 lightPos(5.0f);

PointShadowMap shadowMap;
shadowMap.render(renderer, gothic, model, lightPos, depthShader); // 1. depth 패스
shadowMap.bind();                                                 // 2. 큐브맵 바인딩
shadowMap.applyUniforms(shader, lightPos);                        // 3. 메인 셰이더 Uniform

while (!glfwWindowShouldClose(window)) {
    glViewport(0, 0, width, height);  // render() 이후 viewport를 반드시 복구
    // ... 일반 렌더링 (renderer.draw(gothic, materials, shader))
}
```

현재 광원과 그림자를 드리우는 메시가 정적이므로 depth 패스는 **루프 진입 전 1회**만 실행합니다.

## 3. 공개 API

```cpp
class PointShadowMap {
public:
    PointShadowMap();
    ~PointShadowMap();

    void render(Renderer& renderer, const Mesh& mesh, const glm::mat4& model,
                const glm::vec3& lightPos, const Shader& depthShader) const;
    void bind() const;
    void applyUniforms(const Shader& shader, const glm::vec3& lightPos) const;
};
```

| 함수 | 설명 |
|---|---|
| `PointShadowMap()` | 1024×1024 depth 큐브맵과 FBO 생성 |
| `~PointShadowMap()` | 큐브맵 텍스처와 FBO 해제 |
| `render()` | 광원 위치에서 `mesh`의 깊이를 큐브맵 6면에 렌더링 |
| `bind()` | 큐브맵을 텍스처 유닛 1에 바인딩한 뒤 활성 유닛을 0으로 되돌림 |
| `applyUniforms()` | `shader.use()` 후 `shadowMap`, `shadowFarPlane`, `lightPos` 설정 |

복사는 금지되어 있습니다. (GPU 리소스 이중 해제 방지)

### render()

| 인자 | 설명 |
|---|---|
| `renderer` | `drawDepth()`로 메시를 그릴 Renderer |
| `mesh` | 그림자를 드리우는 메시 |
| `model` | 메시의 Model Matrix (메인 패스와 같은 값이어야 함) |
| `lightPos` | 월드 좌표 광원 위치 |
| `depthShader` | `shadow_depth.vert/.frag`로 만든 Shader |

처리 순서:

1. 90° FOV, 종횡비 1, near/far = 0.1/30 의 Projection 생성
2. `depthShader`에 `model`, `lightPos`, `shadowFarPlane` 설정
3. FBO 바인딩, 컬러 버퍼 비활성화(`GL_NONE`), viewport를 1024×1024로 변경
4. 6면마다 큐브맵 면을 depth attachment로 연결 → `lookAt`으로 View 계산 → `lightViewProjection` 설정 → depth clear → `renderer.drawDepth(mesh)`
5. 기본 프레임버퍼(0)로 복귀

FBO가 불완전하면 `std::runtime_error("Shadow map framebuffer is incomplete")`를 던집니다.

**호출 후 viewport는 1024×1024로 남아 있으므로 호출자가 다시 설정해야 합니다.**

## 4. Shader 인터페이스 계약

Uniform 이름은 `ShadowMap.cpp`에 고정되어 있습니다. 셰이더를 새로 만들거나 수정할 때 아래 이름을 그대로 써야 합니다.

### depth 패스 (`shadow_depth.vert` / `shadow_depth.frag`)

| Uniform | 타입 | 설정 주체 |
|---|---|---|
| `model` | `mat4` | `render()` |
| `lightViewProjection` | `mat4` | `render()` (면마다) |
| `lightPos` | `vec3` | `render()` |
| `shadowFarPlane` | `float` | `render()` |

Fragment Shader는 하드웨어 depth 대신 **광원까지의 선형 거리**를 저장합니다.

```glsl
gl_FragDepth = length(worldPosition - lightPos) / shadowFarPlane;
```

Vertex 입력은 `layout(location = 0) in vec3 aPos;` 하나만 사용합니다.

### 메인 패스 (`fshader.glsl`)

| Uniform | 타입 | 값 | 설정 주체 |
|---|---|---|---|
| `shadowMap` | `samplerCubeShadow` | 텍스처 유닛 1 | `applyUniforms()` |
| `shadowFarPlane` | `float` | 30.0 | `applyUniforms()` |
| `lightPos` | `vec3` | 호출자가 전달 | `applyUniforms()` |

그림자 판정 방식:

```glsl
vec3 fromLight = FragPos - lightPos;              // FragPos는 월드 좌표
float referenceDepth = (distanceToLight - bias) / shadowFarPlane;
float visibility = texture(shadowMap, vec4(fromLight, referenceDepth)); // 0 = 그림자, 1 = 밝음
```

- 큐브맵은 `GL_COMPARE_REF_TO_TEXTURE` + `GL_LEQUAL` + `GL_LINEAR`로 설정되어 있어, `texture()`가 비교 결과(하드웨어 2×2 PCF)를 0~1로 돌려줍니다.
- bias는 `max(0.045 * (1.0 - diffuseFactor), 0.012)`로 표면 기울기에 따라 달라집니다.
- far plane 밖이거나 광원을 등진 면은 `visibility = 1.0`으로 처리합니다.
- 메인 Vertex Shader가 출력하는 `FragPos`는 **월드 좌표**여야 합니다.

## 5. 내부 상수

`ShadowMap.cpp`의 익명 namespace에 정의되어 있으며 외부에 공개되지 않습니다.

| 상수 | 값 | 의미 |
|---|---|---|
| `kShadowMapSize` | 1024 | 큐브맵 한 면의 해상도 |
| `kShadowTextureUnit` | 1 | 그림자 큐브맵 텍스처 유닛 |
| `kShadowNearPlane` | 0.1 | 광원 Projection near |
| `kShadowFarPlane` | 30.0 | 광원 Projection far, 거리 정규화 기준 |
| `kCubeFaces` | 6개 | 면별 시선 방향과 up 벡터 |

`kCubeFaces`의 순서와 up 벡터는 `GL_TEXTURE_CUBE_MAP_POSITIVE_X` ~ `NEGATIVE_Z` 규칙에 맞춘 값이므로 **변경하면 안 됩니다.**

| 면 | direction | up |
|---|---|---|
| +X | ( 1, 0, 0) | (0, -1, 0) |
| -X | (-1, 0, 0) | (0, -1, 0) |
| +Y | ( 0, 1, 0) | (0, 0, 1) |
| -Y | ( 0,-1, 0) | (0, 0, -1) |
| +Z | ( 0, 0, 1) | (0, -1, 0) |
| -Z | ( 0, 0,-1) | (0, -1, 0) |

## 6. 텍스처 유닛 사용 현황

| 유닛 | 사용처 |
|---|---|
| 0 | Renderer — `texture_diffuse` |
| 1 | Shadow — `shadowMap` (큐브맵) |

새 텍스처를 추가할 때는 유닛 0, 1을 피해야 합니다.

## 7. 리소스 수명 관리

| 클래스 | 소유 리소스 |
|---|---|
| PointShadowMap | depth 큐브맵 텍스처 1개, FBO 1개 |

**PointShadowMap은 OpenGL Context가 파괴되기 전에 소멸해야 합니다.** 현재는 `runScene()` 지역 변수이므로 `glfwDestroyWindow()` 전에 자동으로 소멸합니다.

## 8. 제약 및 주의사항

- **점광원 1개만 지원합니다.** 광원마다 `PointShadowMap` 객체가 필요하고, 셰이더 Uniform도 배열로 확장해야 합니다.
- **`render()` 한 번에 메시 1개만 그립니다.** 각 면마다 depth를 clear하므로 `render()`를 여러 번 호출하면 마지막 메시만 남습니다. 여러 메시를 그림자에 넣으려면 면 루프 안에서 여러 번 `drawDepth()`를 호출하도록 확장해야 합니다.
- **정적 그림자입니다.** 광원이나 메시가 움직이면 매 프레임 `render()`를 다시 호출하고, 이후 viewport를 복구해야 합니다.
- `drawDepth()`는 SubMesh/Material을 무시하고 전체 인덱스를 한 번에 그립니다. 투명도와 색은 반영되지 않습니다.
- 스테인드글라스(`rose_window.obj`)는 현재 depth 패스에 포함되지 않습니다.
- far plane(30)보다 먼 표면은 항상 밝게 처리됩니다. 장면이 커지면 `kShadowFarPlane`을 늘려야 합니다.

## 9. 확장 방향

다음 목표인 **Coloured Shadow Mapping**은 이 모듈을 기반으로 확장합니다.

- 스테인드글라스 메시를 별도 패스로 렌더링해 색과 투과율을 담은 컬러 큐브맵을 추가
- 메인 셰이더에서 `visibility`와 투과 색을 곱해 색 그림자 계산
- 새 Uniform과 텍스처 유닛은 6절 표에 추가

이 기능들은 현재 공개 API에 포함되어 있지 않습니다.

## 10. AI / 협업 작업 지침

이 문서를 프롬프트로 사용할 때 지켜야 할 규칙입니다.

- 그림자 관련 코드는 `ShadowMap.h/.cpp`에만 작성하고 `main.cpp`에 FBO나 큐브맵 코드를 다시 넣지 않습니다.
- main에서는 `render()` → `bind()` → `applyUniforms()` 순서로 호출만 합니다.
- Uniform 이름(`shadowMap`, `shadowFarPlane`, `lightPos`, `model`, `lightViewProjection`)을 바꿀 때는 `ShadowMap.cpp`와 GLSL 파일을 함께 수정합니다.
- `render()`의 `model`은 메인 패스에서 같은 메시에 쓰는 Model Matrix와 같아야 합니다. 다르면 그림자가 어긋납니다.
- 텍스처 유닛 0, 1은 예약되어 있습니다.
- `kCubeFaces`의 순서와 up 벡터는 수정하지 않습니다.

---

**모듈 책임 요약**

- **Shadow:** 점광원 depth 큐브맵 생성, depth 패스 실행, 그림자 Uniform 설정
- **Renderer:** depth 패스에서 실제 Draw Call 수행 (`drawDepth()`)
