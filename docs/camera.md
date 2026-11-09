
# Resource — Mesh / Material / Texture Management

C++20을 기반으로 구현된 3D 리소스 로딩 및 관리 모듈입니다.

Resource 모듈은 OBJ 메시, MTL 재질 및 이미지 텍스처를 로드하고 관리합니다.

**Mesh의 GPU 버퍼(VAO/VBO/EBO) 생성 및 Draw Call은 Renderer 모듈에서 담당합니다.**

## 1. 프로젝트 구성

헤더와 구현 파일은 각각 `include/resource/`, `src/resource/`에 위치합니다.

```text
include/
└── resource/
    ├── Mesh.h
    ├── Material.h
    ├── Texture.h
    └── stb_image.h

src/
└── resource/
    ├── Mesh.cpp
    ├── Material.cpp
    ├── texture.cpp
    └── stb_image.cpp
```

### 의존성

- C++20
- GLM — 벡터 및 수학 자료형
- stb_image — 이미지 로딩
- OpenGL / GLAD — GPU 텍스처 관리

`stb_image.cpp`에서 `STB_IMAGE_IMPLEMENTATION`을 정의합니다.

```cpp
#define STB_IMAGE_IMPLEMENTATION
#include "resource/stb_image.h"
```

해당 매크로는 프로젝트 전체에서 중복 정의하면 안 됩니다.

## 2. 기본 사용법

### OBJ 및 MTL 로딩

파일 경로를 생성자에 전달하여 리소스를 로드할 수 있습니다.

```cpp
#include "resource/Mesh.h"
#include "resource/Material.h"

Mesh mesh("data/model.obj");
Material materials("data/model.mtl");
```

기본 생성자와 명시적 로딩 함수를 사용할 수도 있습니다.

```cpp
Mesh mesh;
Material materials;

if (!mesh.loadObj("data/model.obj")) {
    // OBJ 로딩 실패 처리
}

if (!materials.loadMtl("data/model.mtl")) {
    // MTL 로딩 실패 처리
}
```

현재 OBJ와 MTL은 별도로 로딩해야 합니다.

OBJ의 `mtllib` 선언을 이용한 MTL 자동 로딩은 지원하지 않습니다.

### 로딩 실패 처리

- `Mesh(filename)`은 `loadObj()`를 호출하지만 반환값을 확인하지 않습니다.
- `Material(filename)`은 `loadMtl()` 실패 시 `std::runtime_error`를 발생시킵니다.
- `loadObj()`와 `loadMtl()`은 성공 여부를 `bool`로 반환합니다.

상대 경로는 프로세스의 현재 작업 디렉터리를 기준으로 해석됩니다.

## 3. Mesh

`Mesh` 클래스는 OBJ 파일을 파싱하여 정점, 인덱스, SubMesh 및 Bounding Box를 관리합니다.

### 주요 데이터

| 필드 | 설명 |
|---|---|
| `vertices` | 정점 데이터 |
| `indices` | 정점 인덱스 |
| `subMeshes` | 재질별 인덱스 범위 |
| `boundingBox` | 로컬 좌표계 AABB |

`vertices`, `indices`, `subMeshes`는 공개 멤버입니다.

`boundingBox`는 private 멤버로 관리됩니다.

### Vertex

각 정점은 다음 구조체로 표현됩니다.

```cpp
struct alignas(16) Vertex {
    glm::vec4 position;
    glm::vec4 normal;
    glm::vec2 texCoord;
    float padding[2];

    bool operator==(const Vertex& v) const {
        return position == v.position
            && texCoord == v.texCoord
            && normal == v.normal;
    }
};
```

| 멤버 | 자료형 | 설명 |
|---|---|---|
| `position` | `glm::vec4` | 정점 위치 |
| `normal` | `glm::vec4` | 정점 법선 |
| `texCoord` | `glm::vec2` | 텍스처 좌표 |
| `padding` | `float[2]` | 메모리 정렬용 공간 |

정점 데이터의 GPU 업로드와 Vertex Attribute 설정은 Renderer에서 수행합니다.

### Index Buffer

`indices`는 정점 배열의 인덱스를 저장합니다.

```cpp
indices = {0, 1, 2, 2, 3, 0};
```

위 예시는 두 개의 삼각형을 나타냅니다.

동일한 정점을 여러 삼각형에서 참조할 수 있으므로 정점 데이터의 중복을 줄일 수 있습니다.

### OBJ 파싱

현재 OBJ 로더는 다음 항목을 처리합니다.

| OBJ 명령 | 설명 |
|---|---|
| `v` | 정점 위치 |
| `vt` | 텍스처 좌표 |
| `vn` | 정점 법선 |
| `f` | 면 데이터 |
| `usemtl` | 재질 변경 및 SubMesh 구분 |

다각형 면은 삼각형으로 분할합니다.

동일한 위치·UV·법선 인덱스 조합을 갖는 정점은 중복 생성을 방지합니다.

`usemtl` 선언 이전에 면이 등장하면 `default`라는 이름의 SubMesh를 생성합니다.

## 4. AABB — Axis-Aligned Bounding Box

Mesh는 정점 좌표를 이용하여 축 정렬 경계 상자(AABB)를 계산하고 보관합니다.

AABB는 각 좌표축의 최솟값과 최댓값으로 정의되는 직육면체입니다.

### 자료형

`Mesh.h`에는 다음 구조체가 정의되어 있습니다.

```cpp
struct AABB {
    glm::vec3 min;
    glm::vec3 max;
};
```

AABB는 별도 파일로 분리하지 않고 Mesh와 동일한 헤더에 정의합니다.

### Mesh API

```cpp
[[nodiscard]] const AABB& getAABB() const;
```

Mesh는 다음 멤버를 사용하여 Bounding Box를 관리합니다.

```cpp
private:
    AABB boundingBox{};

    void computeBB();
```

| 멤버 | 역할 |
|---|---|
| `boundingBox` | 계산된 AABB 보관 |
| `computeBB()` | 정점 데이터를 이용해 AABB 계산 |
| `getAABB()` | AABB를 읽기 전용 참조로 반환 |

### Bounding Box 계산

`computeBB()`는 모든 정점을 순회하여 좌표별 최솟값과 최댓값을 계산합니다.

```cpp
void Mesh::computeBB() {
    if (vertices.empty()) {
        boundingBox = {};
        return;
    }

    boundingBox.min = glm::vec3(vertices[0].position);
    boundingBox.max = glm::vec3(vertices[0].position);

    for (const auto& vertex : vertices) {
        const glm::vec3 pos(vertex.position);

        boundingBox.min = glm::min(boundingBox.min, pos);
        boundingBox.max = glm::max(boundingBox.max, pos);
    }
}
```

정점 수가 `n`일 때 계산 시간 복잡도는 O(n)입니다.

계산된 AABB를 저장하므로 `getAABB()`의 시간 복잡도는 O(1)입니다.

### OBJ 로딩과 연결

Bounding Box는 정점 로딩이 완료된 후 계산해야 합니다.

`Mesh::loadObj()`가 성공적으로 파싱을 마친 뒤 다음 함수를 호출합니다.

```cpp
computeBB();

return true;
```

**구현 확인 사항:** 2026-11-09에 제공된 `Mesh.cpp`에는 `computeBB()`가 정의되어 있지만, `loadObj()`에서 해당 함수를 호출하는 코드가 누락되어 있습니다.

정확한 AABB를 사용하려면 이 호출을 추가해야 합니다.

### 좌표계

Mesh가 보관하는 AABB는 원본 정점 데이터의 로컬 좌표계를 기준으로 합니다.

렌더링 과정에서 Model Matrix를 적용하더라도 저장된 AABB가 자동으로 변경되지는 않습니다.

월드 좌표계에서 AABB 판정이 필요하다면 Model Matrix를 별도로 반영해야 합니다.

예를 들어 Frustum 모듈과 연동할 수 있습니다.

```cpp
bool visible = frustum.intersectsAABB(
    mesh.getAABB(),
    model
);
```

여기서 `model`은 해당 Mesh를 렌더링할 때 사용하는 Model Matrix입니다.

### 제약사항

- 현재는 Mesh 전체에 대해 AABB 하나만 관리합니다.
- SubMesh별 AABB는 제공하지 않습니다.
- 빈 Mesh의 AABB는 유효한 경계 상자로 취급하면 안 됩니다.
- `vertices`를 변경하면 기존 AABB가 실제 정점 데이터와 일치하지 않을 수 있습니다.
- `computeBB()`는 private 함수이므로 외부에서 직접 호출할 수 없습니다.
- Vertex Shader에서 발생하는 추가적인 정점 변형은 자동으로 반영되지 않습니다.

## 5. SubMesh

하나의 Mesh에는 여러 재질이 적용될 수 있습니다.

Mesh는 재질별 인덱스 범위를 `MeshGroup` 구조체로 관리합니다.

```cpp
struct MeshGroup {
    uint32_t indexStart;
    uint32_t indexCount;
    std::string mtlName;
};
```

### 멤버

| 필드 | 설명 |
|---|---|
| `indexStart` | 인덱스 배열의 시작 위치 |
| `indexCount` | 해당 SubMesh의 인덱스 개수 |
| `mtlName` | 적용할 재질 이름 |

예시:

| 그룹 | 시작 인덱스 | 인덱스 개수 | 재질 |
|---|---:|---:|---|
| 0 | 0 | 300 | MaterialA |
| 1 | 300 | 120 | MaterialB |
| 2 | 420 | 600 | MaterialC |

모든 SubMesh는 동일한 Mesh의 정점 및 인덱스 배열을 공유합니다.

`indexCount`는 삼각형 개수가 아니라 인덱스 개수입니다.

### SubMesh 순회

```cpp
for (const auto& [indexStart, indexCount, mtlName] : mesh.subMeshes) {
    // SubMesh 처리
}
```

구조적 바인딩은 `MeshGroup` 멤버의 선언 순서를 따릅니다.

## 6. Material

`Material` 클래스는 MTL 파일을 로드하고 재질 정보를 관리합니다.

여러 재질을 이름으로 구분하며, 각 재질에 연결된 Texture 객체를 관리합니다.

### 재질 조회

```cpp
const material& mat = materials.get("MaterialA");
```

`get()`은 지정한 재질이 존재하지 않을 경우 기본 재질을 반환합니다.

재질 존재 여부를 확인하려면 `find()`를 사용할 수 있습니다.

```cpp
if (const material* mat = materials.find("MaterialA")) {
    // 재질 존재
}
```

### 지원하는 MTL 속성

| MTL | C++ 멤버 | 설명 |
|---|---|---|
| `Ka` | `ambient` | 주변광 반사 색상 |
| `Kd` | `diffuse` | 확산 반사 색상 |
| `Ks` | `specular` | 정반사 색상 |
| `Ke` | `emission` | 발광 색상 |
| `Ns` | `shininess` | 광택 |
| `Ni` | `opticalDensity` | 광학 밀도 / 굴절률 |
| `d` | `dissolve` | 불투명도 |
| `illum` | `illumination` | 조명 모델 |
| `map_Kd` | `texture` | 확산 반사 텍스처 |

현재 `map_Ke` 등 추가 텍스처 맵은 파싱하지 않습니다.

### Material과 SubMesh 연결

SubMesh의 `mtlName`을 이용하여 해당 재질을 조회할 수 있습니다.

```cpp
for (const auto& group : mesh.subMeshes) {
    const material& mat = materials.get(group.mtlName);

    // 재질 데이터 사용
}
```

별도의 Material ID 연결 과정은 필요하지 않습니다.

### 텍스처 경로 처리

현재 `map_Kd` 파서는 경로 구분자를 `/`로 정규화한 후 파일명만 추출합니다.

예시:

```mtl
newmtl MaterialA
Kd 0.8 0.8 0.8
map_Kd C:\Assets\Textures\diffuse.png
```

이 경우 `diffuse.png`를 추출하고 MTL 파일과 동일한 디렉터리에서 텍스처를 찾습니다.

따라서 현재 구현에서는 **텍스처가 MTL 파일과 동일한 디렉터리에 있어야 합니다.**

서로 다른 재질에서 동일한 텍스처 경로를 참조하면 Texture 객체를 공유합니다.

## 7. Texture

`Texture` 클래스는 이미지 데이터를 CPU 메모리에 로드하고 OpenGL 텍스처 객체를 관리합니다.

### 이미지 로딩

```cpp
Texture texture("data/texture.png");
```

현재 구현은 stb_image를 사용합니다.

이미지는 RGB 3채널로 변환하여 저장합니다.

원본 이미지에 Alpha 채널이 포함되어 있어도 현재 구현에서는 보존되지 않습니다.

이미지 로딩 과정에서 세로 방향 뒤집기가 활성화됩니다.

### GPU 업로드

```cpp
texture.upload();
```

`upload()`는 다음 과정을 수행합니다.

1. CPU 이미지 데이터 유효성 검사
2. OpenGL 텍스처 객체 생성
3. `GL_RGB8` 형식으로 이미지 업로드
4. Mipmap 생성
5. Wrap 및 Filter 설정

이미 GPU 업로드가 완료된 경우 추가 전송 없이 성공을 반환합니다.

GPU 업로드 시에는 유효한 OpenGL Context가 필요합니다.

### Texture 바인딩

```cpp
texture.bind(0);
```

지정한 텍스처 유닛을 활성화하고 해당 텍스처를 바인딩합니다.

### 리소스 해제

Texture 소멸자는 관리 중인 리소스를 해제합니다.

- GPU 텍스처 객체: `glDeleteTextures()`
- CPU 이미지 데이터: `stbi_image_free()`

GPU 리소스는 OpenGL Context가 파괴되기 전에 해제해야 합니다.

### Material에서 Texture 접근

```cpp
const material& mat = materials.get("MaterialA");

if (mat.texture != nullptr) {
    mat.texture->bind(0);
}
```

`material::texture`는 Material이 소유한 Texture 객체를 참조하는 비소유 포인터입니다.

Material 객체가 파괴된 이후에는 해당 포인터를 사용하면 안 됩니다.

## 8. Material::uploadTextures()

Material은 관리하는 모든 Texture를 GPU에 업로드할 수 있습니다.

```cpp
bool success = materials.uploadTextures();
```

내부적으로 Texture 저장소를 순회하며 `upload()`를 호출합니다.

```cpp
for (auto& [name, texture] : textures) {
    if (!texture.upload()) {
        // 업로드 실패 처리
    }
}
```

하나 이상의 텍스처 업로드가 실패하면 `false`를 반환합니다.

이미 업로드된 텍스처는 중복 전송하지 않습니다.

일반적인 렌더링 과정에서는 Renderer가 텍스처 업로드를 처리하므로 외부에서 명시적으로 호출할 필요가 없습니다.

## 9. Renderer 연동

Resource 모듈은 Renderer가 사용하는 데이터를 제공합니다.

| Resource 데이터 | Renderer 용도 |
|---|---|
| `mesh.vertices` | VBO 생성 |
| `mesh.indices` | EBO 생성 |
| `mesh.subMeshes` | 재질별 Draw Call |
| `material.diffuse` | Shader Uniform |
| `material.texture` | GPU 텍스처 바인딩 |

### 기본 렌더링 예제

```cpp
#include "resource/Mesh.h"
#include "resource/Material.h"
#include "renderer/Renderer.h"
#include "renderer/Shader.h"

// OpenGL 및 GLAD 초기화 이후

Mesh mesh("data/model.obj");
Material materials("data/model.mtl");

Shader shader(vertexShaderPath, fragmentShaderPath);
Renderer renderer;

// 렌더링 루프 내부
renderer.draw(mesh, materials, shader);
```

Renderer는 내부적으로 다음 작업을 수행합니다.

- 최초 사용 시 VAO/VBO/EBO 생성
- Mesh별 GPU 버퍼 캐싱
- Material 텍스처 업로드
- SubMesh별 Texture 바인딩
- Shader Uniform 설정
- `glDrawElements()` 호출

호출자는 VAO/VBO/EBO 생성과 Draw Call을 직접 작성할 필요가 없습니다.

카메라 및 변환 행렬 등의 Uniform은 호출 측에서 설정합니다.

Shader에서 사용하는 Uniform 이름은 Renderer 구현과 일치해야 합니다.

## 10. 제약 및 주의사항

### OBJ 파싱

현재 구현에는 다음 제한이 있습니다.

- `mtllib` 자동 로딩을 지원하지 않습니다.
- `v//vn`처럼 UV 인덱스가 생략된 형식은 잘못 해석될 수 있습니다.
- 음수 상대 인덱스를 올바르게 변환하지 않습니다.
- 잘못된 정점 인덱스에 대한 범위 검사가 충분하지 않습니다.
- 같은 Mesh 객체에서 `loadObj()`를 반복 호출할 때 기존 데이터가 자동 초기화되지 않습니다.

### AABB

- 현재 구현은 Mesh 전체의 AABB만 관리합니다.
- AABB는 로컬 좌표계 기준입니다.
- Model Matrix 변환은 AABB 데이터에 자동 반영되지 않습니다.
- 정점 데이터를 변경하면 AABB를 다시 계산해야 합니다.
- 빈 Mesh의 AABB는 유효하지 않습니다.

### Material

- 동일한 Material 객체에서 `loadMtl()`을 반복 호출할 때 기존 데이터가 자동 초기화되지 않습니다.
- Texture는 Material 내부에서 소유합니다.
- Material은 복사 및 이동을 허용하지 않습니다.
- 지원하지 않는 MTL 속성은 파싱되지 않습니다.

### Texture

- 이미지는 RGB 3채널로 로딩됩니다.
- Alpha 채널은 보존되지 않습니다.
- 최초 GPU 업로드 이후 CPU 이미지 변경사항은 자동 반영되지 않습니다.
- 이미지 로딩 실패 시 기본 텍스처를 생성하지 않습니다.
- GPU 텍스처 해제에는 유효한 OpenGL Context가 필요합니다.

### Renderer 연동

현재 Renderer는 Mesh 객체의 주소를 이용해 GPU 버퍼를 캐싱합니다.

따라서 Mesh가 파괴되거나 변경되었을 때 캐시를 갱신하는 별도의 기능은 제공되지 않습니다.

## 11. 협업 시 사용 지침

일반적인 모델 로딩과 렌더링에는 다음 인터페이스를 사용합니다.

```cpp
Mesh mesh(objPath);
Material materials(mtlPath);

renderer.draw(mesh, materials, shader);
```

리소스 사용자는 다음 작업을 직접 구현할 필요가 없습니다.

- OBJ/MTL 파싱
- 정점 중복 제거
- VAO/VBO/EBO 생성
- 텍스처 GPU 업로드
- SubMesh별 Draw Call
- GPU 버퍼 해제

리소스 파일을 추가할 때는 OBJ, MTL, 이미지 파일의 경로를 확인해야 합니다.

로딩에 실패한 경우 대상 파일과 오류 로그를 함께 확인하세요.

---

**모듈별 책임 요약**

- **Resource:** Mesh, Material, Texture 로딩 및 관리
- **Mesh:** 정점·인덱스·SubMesh·AABB 관리
- **Renderer:** GPU 메시 버퍼 및 Draw Call 관리
- **Shader:** GLSL 프로그램 및 Uniform 관리
- **Camera:** 카메라의 위치·방향 및 View/Projection Matrix 제공
- **Frustum:** 점 및 AABB의 카메라 시야 범위 판정
