# 트레이와 앱 종료 (2026-10-25)

실행 중 트레이에 `Key Note Overlay` 아이콘을 둔다. 클릭·키보드 선택은 설정 창을 열거나 복원하고, 우클릭 메뉴에는 **설정 열기 / 오버레이 마우스로 이동 / 종료**가 있다. Windows의 알림 영역 설정에 따라 숨겨진 아이콘 메뉴에 들어갈 수 있다.

이전에는 오버레이 창이 기본 `WM_CLOSE` 처리로 파괴되어도 설정 창과 메시지 루프가 남았다. 이제 오버레이 닫기, 설정 닫기, 트레이 종료, 기존 `Ctrl+Shift+F12`가 설정 저장 확인을 공유한다. 취소나 저장 실패는 종료를 중단한다. 종료 확정 시 하나의 `ShutdownApplication` 경로에서 트레이 제거, 키보드 훅 해제, 두 창 파괴, 메시지 루프 종료를 순서대로 처리한다. 재진입해도 중복 제거하지 않는다. 시작 중 창 생성 실패와 메시지 루프 종료에서도 같은 정리 함수를 호출한다.

트레이는 설정 창을 콜백 소유자로 등록하고, 그 창을 파괴하기 전에 `NIM_DELETE`를 호출한다. [Microsoft Shell_NotifyIcon 문서](https://learn.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shell_notifyiconw)에 따라 아이콘을 추가할 때마다 알림 버전을 설정한다. 최신 버전 설정에 실패하면 이전 콜백 형식으로 처리한다. Explorer의 `TaskbarCreated` 메시지에는 재등록하며, 등록 실패는 앱이 실행되는 동안에만 5초 간격으로 재시도한다.

닫기는 전체 종료다. 최소화는 실행을 유지하며 트레이에서 복원할 수 있다. 자동 시작, 서비스, 별도 감시 프로세스, 새 단축키나 설정 형식은 추가하지 않았다. 강제 프로세스 종료·충돌에서는 삭제 코드가 실행되지 않으므로 Windows가 트레이를 갱신하기 전까지 잔상이 남을 수 있다. 이를 막기 위한 상주 감시 프로그램은 사용하지 않는다.

## 검증

```powershell
cmake --build build-cpp-settings
ctest --test-dir build-cpp-settings --output-on-failure
```

`overlay_tray_lifecycle`은 숨겨진 실제 창 두 개와 임시 설정을 사용한다. 트레이 API와 종료 확인 대화상자만 테스트 대역으로 바꿔 등록 중복, 새/이전 콜백, Explorer 재시작 메시지, 등록 실패 후 재시도, 저장 취소/실패/성공, 두 창에서의 종료, 창 파괴, 세션 종료를 확인한다. 실제 Explorer를 다시 시작하지 않는다. 키보드 훅·입력 주입·데스크톱 캡처는 사용하지 않는다.

실제 트레이를 확인하려면 별도 테스트 작업 폴더에서 실행한다. 이 모드는 아이콘을 잠깐 추가하고 Windows에 존재를 조회한 뒤, 오버레이 닫기로 앱을 종료하고 아이콘이 제거됐는지 다시 조회한다. 화면에 창을 띄우거나 키보드 훅을 설치하지 않는다.

```powershell
# <isolated-test-directory>는 존재하는 별도 테스트 폴더다.
Start-Process -FilePath <absolute-exe-path> -ArgumentList '--tray-smoke-test' -WorkingDirectory <isolated-test-directory> -WindowStyle Hidden -Wait
```

2026-10-25 Release 빌드에서 CTest 4종 및 실제 트레이 검사 5개가 통과했다. 첫 실제 트레이 검사는 격리 환경의 등록 실패로 통과하지 못했으며, 데스크톱 접근을 허용한 동일 EXE의 검사에서 등록·존재 조회·두 창 종료·아이콘 제거를 확인했다. 트레이 메뉴의 실제 마우스 조작, Explorer 프로세스의 실제 재시작, 강제 종료의 잔상 시간, 다른 Windows 배율은 별도 실사용 확인이 필요하다.
