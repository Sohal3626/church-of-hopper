# dataLoader — OBJ / MTL / Texture Loader

C++로 작성된 OBJ 메시, MTL 재질 및 이미지 텍스처 로더입니다.

이 라이브러리는 모델 데이터를 **CPU 메모리로 불러오는 역할**을 담당합니다. GPU 버퍼 생성, 텍스처 업로드, OpenGL 렌더링은 담당하지 않습니다.

## 1. 프로젝트 구성

모든 로더 관련 파일은 `dataLoader/` 디렉터리에 위치합니다.

```text
dataLoader/
├── Mesh.h
├── Mesh.cpp
├── Material.h
├── Material.cpp
├── Texture.h
├── stb_image.h
├── stb_image.cpp
└── README.md
```

### 의존성

- C++20
- GLM — 벡터 및 수학 자료형
- stb_image — 이미지 파일 로딩

`Mesh.cpp`, `Material.cpp`, `stb_image.cpp`는 프로젝트 빌드 대상에 포함해야 합니다.

`stb_image.cpp`에는 다음 구현 매크로를 한 번만 정의합니다.

```cpp
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
```

## 2. 기본 사용법

### OBJ 및 MTL 불러오기

```cpp
#include "dataLoader/Mesh.h"
#include "dataLoader/Material.h"

Mesh mesh;
Material materials;

if (!mesh.loadObj("assets/scene.obj")) {
    // OBJ 로딩 실패 처리
}

if (!materials.loadMtl("assets/scene.mtl")) {
    // MTL 로딩 실패 처리
}
```

OBJ와 MTL 파일은 별도로 로딩합니다. 현재 로더는 OBJ 내부의 `mtllib` 선언을 자동으로 처리하지 않습니다.

상대 파일 경로는 프로그램의 현재 작업 디렉터리를 기준으로 해석되므로 실행 설정을 확인해야 합니다.

## 3. Mesh 데이터 구조

`Mesh`는 다음 데이터를 제공합니다.

| 필드          | 설명                  |
| ----------- | ------------------- |
| `vertices`  | 정점 데이터 배열           |
| `indices`   | 삼각형을 구성하는 정점 인덱스 배열 |
| `subMeshes` | 재질별 인덱스 범위          |

### Vertex

각 Vertex에는 다음 정보가 포함됩니다.

- `position`: 정점 위치. 동차 좌표의 `w`는 1
- `normal`: 정점 법선. 방향 벡터의 `w`는 0
- `texCoord`: 텍스처 UV 좌표
- `padding`: 메모리 정렬을 위한 공간

### Index Buffer

`indices` 배열에는 실제 정점 데이터가 아니라 `vertices` 배열의 인덱스가 저장됩니다.

```cpp
indices = {0, 1, 2, 2, 3, 0};
```

위 배열은 삼각형 두 개를 표현합니다.

각 삼각형은 연속된 인덱스 세 개로 구성됩니다.

## 4. SubMesh

하나의 OBJ 모델에 여러 재질이 적용될 수 있으므로, 로더는 재질별로 인덱스 범위를 구분합니다.

```cpp
struct MeshGroup {
    uint32_t indexStart;
    uint32_t indexCount;
    std::string mtlName;
};
```

| 필드           | 설명                            |
| ------------ | ----------------------------- |
| `indexStart` | `indices` 배열에서 해당 그룹이 시작하는 위치 |
| `indexCount` | 그룹에 포함된 인덱스 개수                |
| `mtlName`    | 해당 그룹의 재질 이름                  |

예시:

| 그룹 | 시작 인덱스 | 인덱스 개수 | 재질    |
| -- | -----: | -----: | ----- |
| 0  |      0 |    300 | Glass |
| 1  |    300 |    120 | Frame |
| 2  |    420 |    600 | Wall  |

**모든 SubMesh는 동일한 Vertex Buffer와 Index Buffer를 공유합니다.**

각 SubMesh는 인덱스 배열에서 사용하는 범위만 다릅니다.

`indexCount`는 삼각형 개수가 아닌 인덱스 개수입니다.

## 5. Material

`Material` 클래스는 MTL 파일을 읽고 재질 정보를 저장합니다.

### 재질 조회

```cpp
const material& mat = materials.get("Glass");
```

`get()`은 해당 이름의 재질이 없으면 기본 재질을 반환합니다.

존재 여부를 확인하려면 `find()`를 사용합니다.

```cpp
if (const material* mat = materials.find("Glass")) {
    // 재질이 존재하는 경우
}
```

### 지원하는 MTL 속성

| MTL      | C++ 멤버           | 설명        |
| -------- | ---------------- | --------- |
| `Ka`     | `ambient`        | 주변광 반사 색상 |
| `Kd`     | `diffuse`        | 확산 반사 색상  |
| `Ks`     | `specular`       | 정반사 색상    |
| `Ke`     | `emission`       | 발광 색상     |
| `Ns`     | `shininess`      | 광택 지수     |
| `Ni`     | `opticalDensity` | 굴절률       |
| `d`      | `dissolve`       | 불투명도      |
| `illum`  | `illumination`   | 조명 모델     |
| `map_Kd` | `texture`        | 확산 반사 텍스처 |

현재 로더는 `map_Ke`를 파싱하지 않습니다.

### 재질과 SubMesh 연결

SubMesh에 저장된 `mtlName`을 사용합니다.

```cpp
for (const auto& group : mesh.subMeshes) {
    const material& mat = materials.get(group.mtlName);

    // mat.diffuse
    // mat.specular
    // mat.texture
}
```

별도의 Material ID 연결 작업은 필요하지 않습니다.

## 6. Texture

`Texture`는 stb_image를 이용하여 이미지 파일을 CPU 메모리에 로딩합니다.

GPU 텍스처 생성과 GLSL 샘플링은 수행하지 않습니다.

GPU 업로드를 위해 다음 Getter를 제공하는 구성을 사용합니다.

```cpp
const stbi_uc* getData() const;
int getWidth() const;
int getHeight() const;
int getChannels() const;
```

*위 Getter는 최신 `Texture.h`에 실제로 추가되어 있는지 확인해야 합니다.*

### 텍스처 접근

```cpp
const material& mat = materials.get("Glass");

if (mat.texture != nullptr) {
    const Texture* tex = mat.texture;

    // 이미지 데이터 접근
}
```

`material::texture`는 `Material` 내부 저장소의 Texture를 참조하는 비소유 포인터입니다. 소유자인 `Material` 객체가 파괴된 이후에는 사용할 수 없습니다.

### MTL 텍스처 경로

```mtl
newmtl Glass
Kd 0.8 0.8 0.8
map_Kd glass.png
```

텍스처 파일의 상대 경로를 MTL 파일 위치 기준으로 해석하도록 구현했는지 확인해야 합니다.

## 7. OpenGL 렌더링에 연결하기

로더의 데이터를 GPU로 업로드할 때 다음과 같이 사용합니다.

| 로더 데이터             | OpenGL 용도          |
| ------------------ | ------------------ |
| `mesh.vertices`    | VBO                |
| `mesh.indices`     | EBO                |
| `Vertex::position` | Position Attribute |
| `Vertex::normal`   | Normal Attribute   |
| `Vertex::texCoord` | UV Attribute       |
| `mat.texture`      | GPU 텍스처 업로드        |
| `mesh.subMeshes`   | 재질별 Draw Call      |

### SubMesh 렌더링 예시

VAO, VBO, EBO, Shader 설정이 완료되어 있다고 가정합니다.

```cpp
for (const auto& group : mesh.subMeshes) {
    const material& mat = materials.get(group.mtlName);

    // Shader Uniform 설정
    // Texture 바인딩

    glDrawElements(
        GL_TRIANGLES,
        static_cast<GLsizei>(group.indexCount),
        GL_UNSIGNED_INT,
        reinterpret_cast<const void*>(
            static_cast<uintptr_t>(group.indexStart)
            * sizeof(uint32_t)
        )
    );
}
```

`glDrawElements()`의 마지막 인자는 EBO의 바이트 오프셋입니다. 따라서 `indexStart`에 인덱스 자료형의 크기를 곱해야 합니다.

정점 속성의 오프셋은 `offsetof(Vertex, position)` 등으로 구하는 것을 권장합니다.

### 스테인드글라스 렌더링

MTL의 `map_Kd`는 기본적으로 표면의 확산 반사 텍스처입니다.

프로젝트에서 이를 색유리의 투과율로 활용할 수 있지만, 실제 투과광 계산은 GLSL에서 별도로 구현해야 합니다.

Shadow Map과 Colored Transmission Map은 이 로더가 반환하는 텍스처가 아니라 GPU 렌더링 과정에서 생성되는 데이터입니다.

## 8. 알려진 제약 및 주의사항

현재까지 제공된 코드 기준으로 다음 사항에 유의해야 합니다.

- OBJ의 `v//vn` 형식 처리에 문제가 있을 수 있습니다.
- 음수 상대 인덱스를 사용하는 OBJ는 정상적으로 처리되지 않을 수 있습니다.
- UV나 법선이 누락된 정점은 안전하게 초기화해야 합니다.
- Material 객체를 복사하거나 이동하면 내부 Texture 포인터의 유효성을 확인해야 합니다.
- 동일 로더 객체로 파일을 여러 번 읽을 때 기존 데이터가 어떻게 처리되는지 확인해야 합니다.

## 9. 협업 시 권장사항

로더를 사용하는 팀원은 내부 파싱 코드를 수정할 필요 없이 다음 인터페이스를 사용하면 됩니다.

```cpp
mesh.loadObj(filename);
materials.loadMtl(filename);

mesh.vertices;
mesh.indices;
mesh.subMeshes;

materials.get(name);
materials.find(name);
```

GPU 리소스 생성 및 해제는 렌더링 담당 코드에서 처리합니다.

새로운 OBJ/MTL 파일이 정상적으로 로딩되지 않는 경우, 해당 파일과 오류 메시지를 함께 공유해 주세요.
