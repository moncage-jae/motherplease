# RawAssets

엔진에 임포트하기 전 단계의 **원본 소스 파일**을 보관합니다.

## 하위 폴더

| 폴더 | 내용 | 담당 |
|------|------|------|
| `3D/` | .blend, .max, .fbx 원본 모델링 파일 | 3D모델러 |
| `2D/` | .psd, .ai, 컨셉아트, 텍스처 원본 | 기획, 2D아트 |
| `VFX/` | VFX 소스 파일, 시퀀스, 파티클 원본 | VFX |
| `Audio/` | 오디오 원본, DAW 프로젝트 파일 | 사운드 |

## 규칙

- UE5 Content 폴더에 들어가는 최종 에셋이 아닌, **작업 원본**만 이곳에 보관합니다.
- 파일명 예시: `CH_Patient_v02.blend`, `TX_WallBrick_Diffuse.psd`
- 3D 폴더 내부는 `Characters/`, `Environments/`, `Props/`로 구분합니다.
- **Git LFS 대상 폴더입니다.** .blend, .psd, .fbx 등은 자동으로 LFS 처리됩니다.
