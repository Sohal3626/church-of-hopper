# Church-of-Hopper
이 프로젝트는 한양대학교ERICA 컴퓨터학부의 2026학년도 2학기 [CSE4020]컴퓨터그래픽스   
과제 목적으로 제작되었습니다. 

---

## 주요 구현목표

### Coloured Shadow Mapping

스테인드글라스의 색상 및 투과율을 반영하여
표면에 색 그림자를 생성합니다.

### Volumetric Ray Marching

화면의 각 픽셀에서 View Ray를 생성하고,
Ray Marching을 통해 대기 중 산란광을 계산합니다.

### Runtime Material Switching

동일한 Geometry에 서로 다른 Material을 적용하여
상호작용에 따른 외형 변화를 구현합니다.

### Visibility Detection

View Frustum 또는 Occlusion Query를 이용하여
메뚜기 Mesh의 가시성을 판정합니다. 

## 추가 구현목표

### 기타 시각효과
메뚜기가 깨어났을 때 여러 시각효과를 추가하여 공포 분위기를 조성합니다.

### 움직임 구현
mesh의 일부 점을 움직여 메뚜기 모델이 꿈틀거리는것 처럼 만듭니다.

--- 
## 개발 환경
- Language: C++ 20
- Graphics API: OpenGL 4.1 Core
- Window Management: GLFW
- OpenGL Loader: GLAD
- Mathematics: GLM
- Build System: CMake
- Package Manager: vcpkg

---

## 조작 방법

| Key   | Action |
|-------|--------|
| W     | Move Forward |
| S     | Move Backward |
| A     | Move Left |
| D     | Move Right |
| f     | Interact |
| ESC   | Exit |

---

## 개발 진행상황

- [x] FPS Camera
- [x] Model Loading
- [x] Basic Lighting
- [ ] Shadow Mapping
- [ ] Coloured Shadow Mapping
- [ ] Volumetric Lighting
- [ ] Runtime Material Switching
- [ ] Camera Frustum Detection
- [ ] Grasshopper Movement
- [ ] Grasshopper Animation
- [ ] Horror Effects

## Third-Party Assets

### 3D Models

#### Long-winged Rice Grasshopper (Oxya japonica)

- **Author:** ffish.asia / floraZia.com
- **Source:** https://sketchfab.com/3d-models/cc0-long-winged-rice-grasshopper-c719fe8ae71747f9ae63a0b8b18216fa
- **License:** CC0 1.0 Universal

#### Gothic building with stained glass window
- **Author:** Riruru
- **Source:** https://sketchfab.com/3d-models/gothic-building-with-stained-glass-window-c09a7445958149d4a9cd8666011f145f
- **License:** CC Attribution-NonCommercial

---
## 모듈 및 API 문서

프로젝트는 기능별 모듈로 구성되어 있으며, 각 모듈의 사용 방법은 별도 문서에서 확인할 수 있습니다.

### Resource

OBJ 메시, MTL 재질 및 이미지 텍스처의 로딩과 관리를 담당합니다. 텍스처의 GPU 업로드 및 바인딩 기능도 제공합니다.

[Resource API 문서](docs/resource.md)

### Renderer

OpenGL GPU 버퍼 관리, Mesh 렌더링, Shader 프로그램 및 Uniform 설정을 담당합니다.

[Renderer API 문서](docs/renderer.md)

### Camera

카메라 이동 및 회전, View Matrix 계산, GLFW 키보드·마우스 입력 처리를 담당합니다.

[Camera API 문서](docs/camera.md)