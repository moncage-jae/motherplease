# Project/

UE5 프로젝트 본체 폴더입니다.

## 구조

| 폴더 | 내용 |
|------|------|
| `Config/` | 프로젝트 설정 파일 (.ini) |
| `Content/` | 엔진에 임포트된 에셋 (.uasset, .umap) |
| `Source/` | C++ 소스 코드 (.cpp, .h) |

## 규칙

- `Content/` 안에는 **임포트 완료된 에셋만** 넣습니다
- 작업 중인 원본 파일은 `RawAssets/` 또는 `_WIP/` 에 보관
- `.uasset`, `.umap` 파일은 Git LFS로 자동 관리됩니다
- `Source/` 는 일반 Git으로 관리됩니다 (LFS 제외)
