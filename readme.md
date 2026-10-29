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
메뚜기가 께어났을 때의 공포 연출 추가로 역겨움을 더합니다.

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

- [ ] FPS Camera
- [ ] Model Loading
- [ ] Basic Lighting
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
- **License:** [CC0 1.0 Universal](https://creativecommons.org/publicdomain/zero/1.0/)