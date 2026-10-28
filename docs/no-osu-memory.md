# NoOsuMemory 추가 빌드 — 2026-10-27

`KeyNoteOverlayCpp-v2.14-NoOsuMemory-20261027-Windows-x64.zip`은 기존 v2.14 릴리즈에 별도로 제공하는 메모리 참조 제외 빌드다. 새 폴더에 풀어 `KeyNoteOverlayCpp.exe`를 실행한다. 기존 릴리즈 파일과 개인 설정을 덮어쓰지 않는다.

## 제외 범위

- CMake `KEYNOTE_OSU_MEMORY_LINK=OFF`로 `src/osu_link.cpp`와 연동용 채보 파서·파일 분석 소스를 오버레이 타깃에서 제외한다.
- osu! 선곡 연동 탭, 동의창, 폴더 선택, 시작/중지 명령, 연동 상태와 선택 곡 HUD를 전처리 단계에서 제외한다. 리더 객체나 작업 스레드를 만들지 않는다. 런타임 OFF 옵션이 아니며 설정 파일로 활성화할 수 없다.
- `ReadProcessMemory`, `VirtualQueryEx` 및 리더 심볼이 최종 EXE에 없는지 자동 검사한다. 기존 일반 빌드는 별도로 유지한다.

## 유지 범위

등록 키 입력, OBS 오버레이, 실시간 난이도와 NPS, 리포트, 트레이 및 설정을 유지한다. 이번 추가 패키지는 최근 작업한 자동 추정/수동 고정 분석 키 수와 사용 레인 접기도 포함한다. 등록한 4~10키 배치는 보존하며 기본값은 등록 키 수 유지다. 상세 사용법은 [analysis-key-mode.md](analysis-key-mode.md)에 있다.

게임용 표시/입력 제한 모드는 전경 프로세스를 `PROCESS_QUERY_LIMITED_INFORMATION`으로 열어 실행 파일 이름을 조회한다. `OpenProcess` 자체는 남지만 게임 메모리를 읽는 권한이나 API는 사용하지 않는다. 키보드 훅은 입력 오버레이를 위해 유지한다. 이 구분은 게임 운영사의 허용 또는 안티치트 안전성 보장을 뜻하지 않는다.

osu! 선곡 자동 조회와 그에 연결된 파일 난이도 표시는 없다. 파일을 직접 여는 별도 ChartDifficultyCalculator는 독립 앱이다. 선택 채보 UI는 제외하지만 공유 Revive CPP 계산 코드는 1~18K 실시간 입력 특징을 위해 포함한다. 6개 특징 모두 계산하며, 실시간 홀드는 170ms 이상 누른 입력을 LN으로 추정한다. 170ms 미만은 단타이며 홀드 입력이 없으면 0을 표시한다. 이 계산 자체는 게임 메모리를 조회하지 않는다. 4K LN v2 실시간 보조 비교도 포함한다.

## 빌드와 검증

```powershell
cmake -S . -B build-no-osu-memory -G Ninja -DCMAKE_BUILD_TYPE=Release -DKEYNOTE_OSU_MEMORY_LINK=OFF
cmake --build build-no-osu-memory --parallel 4
ctest --test-dir build-no-osu-memory --output-on-failure
```

C++17 Windows 컴파일러가 필요하다. 기본 CMake 옵션은 `ON`으로 기존 일반 빌드 동작을 유지한다. MinGW에서는 `no_osu_memory_binary`가 EXE의 imports와 심볼을 검사한다. UI 자체 검사는 연동 탭/컨트롤 제거, 제거된 명령의 무효화, 기존 실시간 HUD 배치를 확인한다. 일반 빌드도 기존 연동 경계 검사를 별도로 실행한다.

배포 패키지의 `VERIFICATION.md`와 `SHA256SUMS.txt`에 해당 바이너리의 검증 및 해시를 기록한다. 실제 게임·OBS 송출·다른 PC/DPI·장시간 사용은 별도 확인이 필요하다.
