# Resource — Mesh / Material / Texture Management

C++20으로 작성된 OBJ 메시, MTL 재질 및 이미지 텍스처 리소스 관리 모듈입니다.

Resource 모듈은 모델 및 재질 파일을 CPU 메모리로 불러오고, 텍스처를 GPU에 업로드하고 바인딩하는 기능을 제공합니다.

**Mesh의 GPU 버퍼 관리(VAO/VBO/EBO)와 실제 Draw Call은 Renderer 모듈에서 담당합니다.**

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
- OpenGL / GLAD — 텍스처 GPU 업로드 및 바인딩

`stb_image.cpp`에서 `STB_IMAGE_IMPLEMENTATION`을 정의하며, 프로젝트 전체에서 중복 정의해서는 안 됩니다.

```cpp
#define STB_IMAGE_IMPLEMENTATION
#include "resource/stb_image.h"
```

## 2. 기본 사용법

### OBJ 및 MTL 불러오기

파일 경로를 생성자에 직접 전달할 수 있습니다.

```cpp
#include "resource/Mesh.h"
#include "resource/Material.h"

Mesh gothic("data/gothic.obj");
Material materials("data/gothic.mtl");
```

기존의 명시적 로딩 함수도 사용할 수 있습니다.

```cpp
Mesh mesh;
Material materials;

if (!mesh.loadObj("data/gothic.obj")) {
    // OBJ 로딩 실패 처리
}

if (!materials.loadMtl("data/gothic.mtl")) {
    // MTL 로딩 실패 처리
}
```

**현재 OBJ와 MTL은 별도로 로딩해야 합니다.** OBJ 파일의 `mtllib` 선언을 이용한 자동 MTL 로딩은 구현되어 있지 않습니다.

### 로딩 실패 처리

두 생성자의 실패 처리는 서로 다릅니다.

- `Mesh(filename)`은 `loadObj()`를 호출하며, 반환값을 확인하지 않습니다.
- `Material(filename)`은 `loadMtl()` 실패 시 `std::runtime_error`를 발생시킵니다.
- `loadObj()`와 `loadMtl()`은 로딩 성공 여부를 `bool`로 반환합니다.

상대 경로는 프로세스의 현재 작업 디렉터리를 기준으로 해석됩니다.

프로젝트에서 제공하는 `getPath()`를 사용하면 프로젝트 소스 디렉터리를 기준으로 경로를 구성할 수 있습니다.

## 3. Mesh

`Mesh` 클래스는 OBJ 파일을 파싱하여 정점, 인덱스 및 SubMesh 정보를 저장합니다.

| 필드 | 설명 |
|---|---|
| `vertices` | 정점 데이터 |
| `indices` | 정점 인덱스 |
| `subMeshes` | 재질별 인덱스 범위 |

### Vertex

각 `Vertex`에는 다음 정보가 저장됩니다.

| 멤버 | 자료형 | 설명 |
|---|---|---|
| `position` | `glm::vec4` | 정점 위치. `w = 1` |
| `normal` | `glm::vec4` | 정점 법선. `w = 0` |
| `texCoord` | `glm::vec2` | 텍스처 UV 좌표 |
| `padding` | 헤더 정의 참조 | 메모리 정렬용 공간 |

실제 GPU 정점 속성 설정은 Renderer에서 처리합니다.

### Index Buffer

`indices`는 `vertices` 배열의 인덱스를 저장합니다.

```cpp
indices = {0, 1, 2, 2, 3, 0};
```

이 예시는 삼각형 두 개를 표현합니다. 각 삼각형은 세 개의 인덱스로 구성됩니다.

### OBJ 로딩 방식

현재 로더는 다음 항목을 처리합니다.

- `v` — 정점 위치
- `vt` — UV 좌표
- `vn` — 법선
- `f` — 면 정보
- `usemtl` — 재질별 SubMesh 구분

다각형 면은 삼각형으로 분할되며, 동일한 `FaceVertex` 조합은 중복 정점 생성을 피하도록 처리합니다.

`usemtl`이 나오기 전에 면이 등장하면 이름이 `default`인 SubMesh를 생성합니다.

## 4. SubMesh

하나의 OBJ 모델에는 여러 재질이 적용될 수 있습니다.

이를 위해 `Mesh`는 재질 적용 범위별로 `MeshGroup`을 저장합니다.

```cpp
struct MeshGroup {
    uint32_t indexStart;
    uint32_t indexCount;
    std::string mtlName;
};
```

| 필드 | 설명 |
|---|---|
| `indexStart` | `indices` 배열에서 시작하는 인덱스 위치 |
| `indexCount` | 해당 그룹에 포함된 인덱스 개수 |
| `mtlName` | `usemtl`에서 지정한 재질 이름 |

예시:

| 그룹 | 시작 인덱스 | 인덱스 개수 | 재질 |
|---|---:|---:|---|
| 0 | 0 | 300 | Glass |
| 1 | 300 | 120 | Frame |
| 2 | 420 | 600 | Wall |

모든 SubMesh는 동일한 Mesh의 정점 및 인덱스 배열을 공유합니다.

**`indexCount`는 삼각형 개수가 아니라 인덱스 개수입니다.**

### SubMesh 순회

C++17 구조적 바인딩을 사용할 수 있습니다.

```cpp
for (const auto& [indexStart, indexCount, mtlName] : mesh.subMeshes) {
    // 각 SubMesh 정보 사용
}
```

구조적 바인딩은 구조체 멤버의 선언 순서를 기준으로 동작합니다.

## 5. Material

`Material` 클래스는 MTL 파일을 읽어 재질 정보와 Texture 객체를 관리합니다.

### 재질 조회

```cpp
const material& mat = materials.get("Glass");
```

`get()`은 해당 재질이 존재하지 않으면 기본 재질을 반환합니다.

존재 여부를 직접 확인하려면 `find()`를 사용합니다.

```cpp
if (const material* mat = materials.find("Glass")) {
    // 재질이 존재함
}
```

### 지원하는 MTL 속성

| MTL | C++ 멤버 | 설명 |
|---|---|---|
| `Ka` | `ambient` | 주변광 반사 색상 |
| `Kd` | `diffuse` | 확산 반사 색상 |
| `Ks` | `specular` | 정반사 색상 |
| `Ke` | `emission` | 발광 색상 |
| `Ns` | `shininess` | 광택 지수 |
| `Ni` | `opticalDensity` | 광학 밀도 / 굴절률 |
| `d` | `dissolve` | 불투명도 |
| `illum` | `illumination` | 조명 모델 번호 |
| `map_Kd` | `texture` | 확산 반사 텍스처 |

현재 `map_Ke` 등 추가 텍스처 맵은 처리하지 않습니다.

### Material과 SubMesh 연결

SubMesh의 `mtlName`을 재질 조회에 사용합니다.

```cpp
for (const auto& group : mesh.subMeshes) {
    const material& mat = materials.get(group.mtlName);

    // mat.diffuse
    // mat.specular
    // mat.texture
}
```

별도의 Material ID 연결 작업은 필요하지 않습니다.

### 텍스처 경로 처리

현재 `map_Kd` 파서는 경로 구분자를 `/`로 정규화한 다음, 파일명만 추출합니다.

```mtl
newmtl Glass
Kd 0.8 0.8 0.8
map_Kd C:\OldProject\Textures\glass.png
```

이 경우 파일명 `glass.png`를 추출하고, 현재 MTL 파일과 동일한 디렉터리에서 텍스처를 찾습니다.

즉, **현재 구현에서는 텍스처가 MTL 파일과 같은 디렉터리에 있어야 합니다.**

서로 다른 재질이 같은 파일 경로를 참조하면 하나의 Texture 객체를 공유합니다.

## 6. Texture

`Texture` 클래스는 이미지 데이터를 CPU 메모리에 로드하고 OpenGL 텍스처 객체를 관리합니다.

### 이미지 로딩

```cpp
Texture texture("data/glass.png");
```

현재 구현은 stb_image를 사용하며, 이미지를 RGB 3채널로 변환해서 저장합니다.

따라서 원본 이미지에 Alpha 채널이 있어도 현재 구현에서는 보존되지 않습니다.

이미지 로딩 시 세로 방향 뒤집기를 활성화합니다.

### GPU 업로드

```cpp
texture.upload();
```

`upload()`는 다음 작업을 수행합니다.

1. CPU 이미지 데이터 유효성 검사
2. GPU 텍스처 생성
3. `GL_RGB8` 형식으로 이미지 업로드
4. Mipmap 생성
5. Wrap 및 Filter 설정

이미 GPU 텍스처가 생성되어 있다면 추가 전송 없이 성공을 반환합니다.

**GPU 업로드를 실행하려면 유효한 OpenGL Context가 필요합니다.**

### Texture 바인딩

```cpp
texture.bind(0);
```

지정한 텍스처 유닛을 활성화하고 해당 OpenGL 텍스처를 바인딩합니다.

### 리소스 해제

Texture 소멸자는 다음 리소스를 해제합니다.

- GPU 텍스처 객체: `glDeleteTextures()`
- CPU 이미지 데이터: `stbi_image_free()`

GPU 리소스는 OpenGL Context가 파괴되기 전에 해제해야 합니다.

### Material에서 Texture 접근

```cpp
const material& mat = materials.get("Glass");

if (mat.texture != nullptr)
    mat.texture->bind(0);
```

`material::texture`는 Material이 소유하는 Texture 객체를 가리키는 비소유 포인터입니다.

따라서 Material 객체가 파괴된 뒤에는 해당 포인터를 사용하면 안 됩니다.

## 7. Material::uploadTextures()

Material은 자신이 관리하는 텍스처들을 일괄 업로드할 수 있습니다.

```cpp
bool success = materials.uploadTextures();
```

내부적으로 Texture 저장소를 순회하여 각각의 `upload()`를 호출합니다.

```cpp
for (auto& [name, texture] : textures) {
    if (!texture.upload()) {
        // 업로드 실패 처리
    }
}
```

하나 이상의 텍스처 업로드가 실패하면 `false`를 반환합니다.

이미 업로드한 Texture는 실제 이미지 데이터를 다시 전송하지 않습니다.

일반적인 렌더링 사용 과정에서는 Renderer가 이 작업을 호출하므로, 외부에서 별도로 실행할 필요가 없습니다.

## 8. Renderer와 연결하기

Resource 모듈은 Renderer에서 사용하는 데이터를 제공합니다.

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

Mesh gothic("data/gothic.obj");
Material materials("data/gothic.mtl");

Shader shader(vertexShaderPath, fragmentShaderPath);
Renderer renderer;

// 렌더링 루프 내부
renderer.draw(gothic, materials, shader);
```

Renderer는 내부적으로 다음 작업을 수행합니다.

- 최초 사용 시 Mesh의 VAO/VBO/EBO 생성
- Mesh별 GPU 버퍼 캐싱
- Material 텍스처 업로드
- SubMesh별 Texture 바인딩
- Shader Uniform 설정
- `glDrawElements()` 호출

**호출자는 직접 VAO/VBO/EBO를 생성하거나 Draw Call을 작성할 필요가 없습니다.**

카메라 및 변환 행렬 등의 Shader Uniform은 현재 호출 측에서 설정합니다.

Shader에서 사용하는 Uniform 이름은 Renderer의 구현과 일치해야 합니다.

## 9. 스테인드글라스 렌더링

MTL의 `map_Kd`는 기본적으로 확산 반사 텍스처입니다.

프로젝트에서는 이를 스테인드글라스의 색상 정보로 활용할 수 있지만, 실제 빛의 투과 및 그림자 색상 계산은 별도의 렌더링 기법과 GLSL 코드가 필요합니다.

현재 Resource 모듈은 다음 기능을 직접 구현하지 않습니다.

- Colored Shadow Mapping
- Volumetric Ray Marching
- Colored Transmission Map 생성
- Runtime Material Switching의 재질 교체 정책

이러한 기능은 Renderer 및 이후 상위 시스템에서 처리할 예정입니다.

## 10. 제약 및 주의사항

### OBJ 파싱

현재 구현에는 다음 제한이 있습니다.

- `mtllib` 자동 로딩을 지원하지 않습니다.
- `v//vn`처럼 UV 인덱스가 생략된 형식은 현재 파싱 방식에서 잘못 해석될 수 있습니다.
- 음수 상대 인덱스를 올바르게 변환하지 않습니다.
- 잘못된 정점 인덱스에 대한 범위 검사가 충분하지 않습니다.
- 같은 Mesh 객체에서 `loadObj()`를 반복 호출할 때 기존 데이터가 자동 초기화되지 않습니다.

### Material

- 같은 Material 객체에서 `loadMtl()`을 반복 호출할 때 기존 데이터가 자동 초기화되지 않습니다.
- Texture는 Material 내부에서 소유합니다.
- 현재 Material은 복사 및 이동을 허용하지 않습니다.
- 지원하지 않는 MTL 속성은 파싱되지 않습니다.

### Texture

- 이미지는 RGB 3채널로 로딩됩니다.
- Alpha 채널은 보존하지 않습니다.
- 현재 `upload()`는 최초 업로드 후 데이터 변경을 GPU에 자동 반영하지 않습니다.
- 이미지 로딩 실패 시 기본 텍스처를 생성하지 않습니다.
- GPU 텍스처 해제에는 유효한 OpenGL Context가 필요합니다.

### Renderer 연동

현재 Renderer는 Mesh 객체의 주소를 이용해 GPU 버퍼를 캐싱합니다.

따라서 Mesh가 파괴되거나 변경되었을 때 캐시를 갱신하는 별도 기능은 아직 제공되지 않습니다.

## 11. 협업 시 사용 지침

일반적인 모델 로딩과 렌더링에서는 다음 인터페이스를 사용하면 됩니다.

```cpp
Mesh mesh(objPath);
Material materials(mtlPath);

renderer.draw(mesh, materials, shader);
```

다음 작업을 직접 구현할 필요는 없습니다.

- OBJ/MTL 파싱
- 정점 중복 제거
- VAO/VBO/EBO 생성
- 텍스처 GPU 업로드
- SubMesh별 Draw Call
- GPU 버퍼 해제

리소스 파일을 추가할 때는 OBJ, MTL, 이미지 파일의 실제 경로를 확인해 주세요.

새로운 파일이 정상적으로 로딩되지 않는 경우 해당 파일과 오류 로그를 함께 공유해 주세요.

---

**모듈별 책임 요약**

- **Resource:** Mesh, Material, Texture 로딩 및 관리
- **Renderer:** GPU 메시 버퍼와 Draw Call 관리
- **Shader:** GLSL 프로그램 생성 및 Uniform 설정
- **Camera:** 카메라 위치와 방향 및 투영 관련 상태
