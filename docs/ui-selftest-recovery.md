# UI 자체 검사 비정상 종료 조사와 수정 v2.11

## 확인한 증거

2026-10-26 v2.10 배포 EXE의 `--ui-self-test`가 한 번 종료 코드 3으로 끝났다. 당시 로그는 버퍼링되어 마지막 실행 위치를 복원할 수 없었다. Windows 오류 보고에는 APPCRASH와 예외 코드 `0x40000015`가 남아 있었지만 정확한 호출 스택은 확보하지 못했다. 현재 v2.10 바이너리를 별도 작업 폴더에서 30회 실행했을 때 자연 재현되지 않았다.

이전 UI 검사에는 첫 리포트 저장의 성공 여부와 관계없이 `std::filesystem::directory_iterator(reports)`를 호출하는 경로가 있었다. 저장 폴더를 만들지 못하면 `filesystem_error`가 발생하고, 자체 검사 진입점 밖으로 빠져나가 `terminate`/abort로 이어질 수 있다. 파일로 출력 경로를 막은 최소 C++ 재현에서 처리되지 않은 `filesystem_error`와 종료 코드 3을 확인했다.

**동일한 비정상 종료를 일으키는 코드 경로는 확인했다. 최초 일회성 사건도 이 경로였는지, 그때 첫 저장이 왜 실패했는지는 확정하지 않았다.** 접근할 수 없는 오류 보고를 우회하거나 다른 앱·게임을 종료하지 않았다.

## 변경 이유와 동작

- 폴더 순회의 생성뿐 아니라 증가 단계도 `error_code`로 검사한다. 생성자만 바꾼 range-for는 순회 중 오류에서 여전히 예외를 던질 수 있다.
- 테스트용 폴더 생성·존재 확인·정리도 오류를 검사한다. 이미 있는 폴더는 재사용하지 않고 새로 만든 직접 하위 폴더만 경계를 확인한 뒤 정리한다.
- 테스트 로그는 각 검사 직후 flush한다. 자체 검사 진입점은 예외를 잡아 `key_note_cpp_selftest_exception.log`와 종료 코드 2를 남긴다. 정상 통과는 0, 검사 실패는 1이다. 일반 앱 실행을 광범위한 예외 무시로 감싸지 않는다.
- 별도 `--ui-self-test --report-output-blocked` 경로는 첫 리포트 출력 폴더를 파일로 막는다. 실제 저장 실패 → 기록 보존 → 폴더 검사 오류 반환 → 장애 파일 제거 → 재시도 성공을 검증한다. 실패를 단순히 건너뛰어 통과로 표시하지 않는다.

## 재실행

```powershell
cmake -S . -B build-cpp-settings/game-overlay-release -G Ninja -DCMAKE_CXX_COMPILER=C:/msys64/mingw64/bin/g++.exe -DCMAKE_BUILD_TYPE=Release
cmake --build build-cpp-settings/game-overlay-release --parallel 4
ctest --test-dir build-cpp-settings/game-overlay-release --output-on-failure
```

CTest가 각 검사에 서로 다른 작업 폴더를 배정한다. 사용자 설정·리포트 폴더에서 자체 검사를 실행하지 않는다. `overlay_selftest_exception_boundary`는 고의로 예외를 발생시키므로 종료 코드 2가 기대 결과다. CTest에는 예상 실패로 등록되며, 패키지 검증에서는 코드 2와 진단 내용을 별도로 확인한다.

이 수정은 확인한 테스트의 예외 탈출 경로를 막는다. 모든 네이티브 충돌 원인을 차단하거나 실제 게임 사용 안정성을 증명하는 것은 아니다. 재발 시 즉시 flush된 마지막 검사와 예외 로그로 다음 원인을 좁힐 수 있다.
