# Gamma Settings (Geode Mod)

화면 감마 / 밝기 / 대비를 설정 화면에서 조절하는 모드입니다.

## 빌드
1. Geode SDK + Geode CLI 설치, `GEODE_SDK` 환경변수 설정
2. 이 폴더에서:
   ```
   geode build
   ```
3. 생성된 `.geode` 파일이 `geode/mods` 폴더에 설치됩니다.

## 설정
- 활성화 / 감마 / 밝기 / 대비 (게임 내 Geode > 모드 > Gamma Settings > 설정)

`mod.json`의 `id`와 `developer`는 본인 것으로 바꿔주세요.

## GitHub Actions로 빌드 (로컬 설치 없이)
1. 이 폴더를 GitHub 저장소로 push
2. 저장소의 Actions 탭 → Build Mod 실행 완료 대기
3. Artifacts의 `gamma-settings-geode`에서 `.geode` 파일 다운로드
4. `Geometry Dash/geode/mods` 폴더에 넣기
