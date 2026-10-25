# Key Note Overlay C++

Windows rhythm-game key overlay for OBS, osu!, and Qwilight. It captures configured keyboard lanes with a low-level Win32 keyboard hook and renders notes in a straight lane layout. Unregistered keys are ignored before rendering.

Download the portable Windows x64 ZIP from [Releases](https://github.com/10kseason/key-note-overlay-cpp/releases). Extract it to a writable folder and run `KeyNoteOverlayCpp.exe`. Keep the included license notices with the executable. Project code is [MIT licensed](LICENSE); see [privacy and third-party notices](docs/privacy-and-licenses.md).

## Features

- Low-latency native C++/Win32 implementation
- Opt-in osu!/Qwilight mode: remain visible and on top when switching to Discord/OBS, pass mouse clicks through, and ignore input from other apps
- Initial settings placement avoids the saved overlay and stays within the monitor work area when space permits
- Korean settings UI with keyboard, position/OBS, appearance/motion, difficulty, and key color tabs
- Large live difficulty, completed-pattern result, and current NPS cards in settings and an optional OBS HUD
- Opt-in local session reports: NPS/difficulty graphs, lane timing timeline, peak-window summary, and automatic HTML saving after ten seconds of inactivity
- Dan course comparisons for 4K Reform v1, 7K Jinjin Regular, 8K Regular v1, and 10K Revive 2026 (1–14, Kaiden, OVERJOY), with low / mid low / 수준 / high bands
- osu!mania strain details, locally calibrated AERY 5K estimates
- AERY 5K low-level feedback adjustment: up to one level lower, tapering back to the original scale by estimated level 13
- One-second NPS, eight-second average, burst density, chords, repeated lanes, and hold interference
- 4K, 5K, 6K, 7K, 8K, 9K, and 10K presets, plus custom layouts
- Press-to-record key layouts, replace one key, reorder lanes, and remove lanes
- Strict duplicate/unknown key and numeric validation before applying a draft
- Separate edited/applied/saved states, restore saved settings, and save-on-close prompt
- Verified temporary-file saves preserve the previous settings when replacement fails
- Drag the overlay with temporary move mode, automatically save its position, and restore it on startup
- Center the overlay on the settings window's monitor
- Long-note rendering while a key is held
- Long notes are split visually into head, low-alpha body, and tail cap
- Box or circle note heads
- Classic, Neon, and Minimal visual presets
- Transparent lane background, thin note outline, and key press glow/pulse
- Top or bottom compact key labels
- Adjustable whole-overlay opacity
- Per-key color picker/HEX and 0–100% opacity, saved by physical key across presets
- Chord alignment: keydowns within 15 ms render on the first keydown row
- Adjustable overlay position, size, note travel, and release fade
- Always-on-top and click-through overlay options
- Separate capture-safe overlay/settings windows for OBS
- Tray icon while running: click to open settings, right-click for settings or exit
- One exit path closes the overlay, settings window, keyboard hook, and tray icon together

## Build

Requirements:

- Windows
- CMake 3.20+
- Ninja
- MinGW g++ at `C:/msys64/mingw64/bin/g++.exe`, or edit `build_cpp_overlay.bat` for your compiler path

Build:

```bat
build_cpp_overlay.bat
```

Run:

```bat
run_cpp_overlay.bat
```

The executable is produced at:

```text
build-cpp-settings\key_note_overlay_cpp.exe
```

The previous `build-cpp-ninja` executable and existing `dist` archive are left intact.
Use `run_cpp_overlay.bat` to start the new build from the repository directory and find your existing local config.

## Usage

설정은 다섯 탭으로 나뉩니다.

- **키 배치**: 4K~10K 프리셋, 순서대로 키 등록, 선택 키 변경, 순서 이동, 삭제, 쉼표로 직접 입력.
- **위치 · OBS**: osu!·Qwilight 게임용 모드, 드래그 이동·위치 자동 저장, 좌표와 크기, 현재 모니터 중앙 배치, 마우스 클릭 통과, 항상 위, 위치 확인용 배경.
- **모양 · 움직임**: Classic/Neon/Minimal, 노트 크기, 레인 간격, 불투명도, 롱노트 진하기, 이동 거리와 사라지는 시간, 모양 옵션.
- **난이도**: 최근 입력의 추정 점수와 세부 지표, 분석 켜기/끄기, OBS 표시, 분석 초기화.
- **키 색상**: 키별 색상 선택·HEX 입력, 불투명도 0~100%, 미리보기, 선택 키 기본값.

### 키 배치 편집

1. 프리셋을 고르거나 **순서대로 등록**을 누릅니다.
2. 원하는 키를 왼쪽부터 하나씩 누른 뒤 **등록 완료**를 누릅니다. 첫 키부터 새 배치를 만듭니다.
3. 한 키만 바꾸려면 목록에서 선택하고 **선택 키 변경**을 누른 뒤 새 키를 누릅니다.
4. **← 앞으로 / 뒤로 →**로 순서를 옮기거나 **삭제**합니다. 키는 최소 1개가 필요합니다.
5. **적용 + 테스트**로 확인하고 **적용 + 저장**으로 보관합니다.

4K~10K 프리셋은 **키 수별로 수정한 배치**를 기억합니다. 예를 들어 4K를 `A,S,K,L`로 바꾼 뒤 7K로 이동했다가 4K를 고르면 수정한 키가 다시 나옵니다. 선택만으로 실행 중 배치는 바뀌지 않으며, **적용 + 저장**을 누르면 편집한 여러 프리셋이 JSON·INI에 함께 저장되어 재실행 후에도 유지됩니다. **저장 상태로 되돌리기**는 프리셋별 변경도 되돌립니다. 기존 설정 파일에 현재 배치만 있으면 그 배치를 해당 키 수의 프리셋으로 이어받습니다.

목록의 위→아래가 오버레이의 왼쪽→오른쪽입니다. 등록 중 **Esc**를 누르거나 다른 창으로 이동하면 등록 전 배치로 돌아옵니다. 길게 누른 키의 자동 반복은 무시합니다. 등록은 설정 창에 포커스가 있을 때만 동작하며 다른 프로그램의 입력을 차단하지 않습니다.

직접 입력도 계속 지원합니다. 예: `D,F,SPACE,J,K`. 특수 문자는 `SEMICOLON`, `COMMA`, `LBRACKET` 같은 이름을 사용할 수 있습니다. 숫자패드는 `NUM0`~`NUM9`, `NUMADD`, `NUMSUBTRACT`, `NUMMULTIPLY`, `NUMDIVIDE`, `NUMDECIMAL`로 구분합니다. 숫자패드에서 숫자를 등록할 때는 Num Lock을 켜세요. 좌우 수식 키는 `LSHIFT/RSHIFT`, `LCTRL/RCTRL`, `LALT/RALT`입니다. Esc 자체를 레인으로 쓰려면 직접 `ESC`를 입력하세요.

잘못된 키, 중복 키, 빈 항목, 범위를 벗어난 숫자는 오류를 표시합니다. 오류가 있으면 실행 중인 배치는 바뀌지 않습니다.

### 키별 색상과 투명도

**키 색상** 탭에서 키를 선택하고 **색상 선택…** 또는 `#RRGGBB` 입력으로 색을 정합니다. **불투명도 0~100%**는 노트 본체·롱노트·테두리·글자·잔상 전체에 적용됩니다. 0%면 숨기고 100%면 진하게 보이며 입력과 NPS 계산은 그대로입니다. 체크무늬 미리보기로 편집한 값을 확인한 뒤 **적용 + 테스트 → 적용 + 저장**하세요.

설정은 D·F·SPACE 같은 실제 키에 붙으므로 순서·프리셋·테마를 바꿔도 유지됩니다. HEX 빈칸은 기존 팔레트를 사용하고 **선택 키 기본값**은 그 키의 색과 불투명도만 복원합니다. 전체 불투명도와 키별 불투명도는 곱해집니다. 이전 설정 파일도 그대로 불러옵니다. [저장 형식과 검증](docs/key-colors.md)

### 적용과 저장

- **적용**: 현재 편집 내용을 실행 중인 오버레이에 반영합니다.
- **적용 + 저장**: 적용한 뒤 JSON과 호환용 INI를 저장합니다.
- **적용 + 테스트**: 적용하고 전체 레인에 테스트 노트를 표시합니다.
- **저장 상태로 되돌리기**: 편집 내용과 실행 중인 오버레이를 마지막 저장 상태로 되돌립니다. 아직 저장하지 않았다면 시작할 때 불러온 상태입니다.

저장은 같은 폴더의 임시 파일에 기록하고 디스크 반영·내용 재확인을 마친 뒤 기존 파일을 교체합니다. 파일 잠금이나 권한 문제로 교체하지 못하면 이전 저장 파일과 미저장 표시를 유지합니다. 시작할 때 우선 사용하는 JSON이 저장됐다면 호환 INI의 갱신 실패는 별도 안내하며, 다음 실행에는 JSON 설정을 불러옵니다. 비어 있거나 중간에 잘린 JSON은 INI로 복원합니다. 저장 완료 문구에서 실제 저장 위치를 확인할 수 있습니다.

포터블 설정은 해당 실행 파일 폴더에 보관됩니다. 새 배포 폴더에는 기본 설정이 들어 있으므로 기존 배치를 옮기려면 사용하던 `key_note_cpp_config.json`과 `key_note_cpp_config.ini`를 새 폴더에 복사하세요. 개발용 실행 스크립트는 기존 작업 폴더 설정을 찾는 동작을 유지합니다.

프리셋과 테마 선택도 적용 전까지는 편집 내용에만 반영됩니다. 테마를 바꿔도 다른 탭에서 편집한 키·위치·크기는 유지됩니다. 제목의 `*`와 상태 문구로 미적용 또는 미저장 변경을 확인할 수 있습니다.

설정 창이나 오버레이를 닫거나, 트레이 메뉴의 **종료**, 기존 단축키 `Ctrl+Shift+F12`를 사용하면 앱 전체를 종료합니다. 변경 내용이 남아 있으면 저장/저장하지 않고 종료/취소를 선택합니다. 취소하거나 저장에 실패하면 창과 트레이가 유지됩니다. 종료가 확정되면 설정 창·오버레이·트레이 아이콘과 키보드 훅을 함께 정리합니다. 키 등록 중에는 Esc로 등록을 취소한 뒤 종료하세요.

### 오버레이 위치 저장

**위치 · OBS → 마우스로 위치 이동** 또는 트레이의 이동 메뉴를 켜고 오버레이를 잡아 끌면 놓은 좌표를 자동 저장합니다. 끝나면 이동 모드를 꺼서 원래 클릭 통과·배경으로 돌아갑니다. X/Y를 입력하고 **위치 저장**을 눌러도 됩니다. 다음 실행에 마지막 저장 위치로 복원하며, 다른 미저장 설정은 건드리지 않습니다. 화면 밖이면 **이 모니터 중앙으로 → 위치 저장**을 사용하세요. [동작과 검증](docs/overlay-position.md)

### 트레이

실행 중 Windows 알림 영역에 **Key Note Overlay** 아이콘을 표시합니다. Windows 설정에 따라 숨겨진 아이콘 메뉴 안에 있을 수 있습니다. 아이콘을 클릭하면 설정 창을 열거나 최소화 상태에서 복원하고, 우클릭하면 **설정 열기 / 오버레이 마우스로 이동 / 종료** 메뉴가 나옵니다. 닫기 버튼은 앱 전체 종료이며, 설정 창을 최소화해도 오버레이와 트레이는 계속 동작합니다.

Explorer가 다시 시작되면 트레이 아이콘을 재등록합니다. 트레이 등록에 실패하면 실행 중에만 5초 간격으로 재시도합니다. 작업 관리자 강제 종료나 충돌에서는 앱이 정리 코드를 실행할 수 없으므로 Windows가 트레이를 새로 고칠 때까지 잔상이 남을 수 있습니다. [종료 처리와 검증](docs/tray-lifecycle.md)에 구현 범위와 검증 방법을 정리했습니다.

### osu! · Qwilight 게임 위에 표시 (v2.12)

**위치 · OBS → 게임용 모드 (osu! · Qwilight 입력 / 항상 표시) → 적용 + 저장**을 선택합니다. **마우스로 위치 이동**으로 배치한 뒤 이동 모드를 끝내고 게임으로 돌아가세요. 설정 창은 최소화해도 됩니다.

클릭 통과·항상 위·포커스 비활성 창으로 표시하며, Discord/OBS로 전환해도 표시를 유지합니다. 게임 외 앱의 타건은 새로 집계하지 않습니다. 설정의 난이도 탭에서 하는 직접 연습은 계속 지원합니다. 게임용 모드에서는 위치 확인용 배경을 숨깁니다. 기본값은 꺼짐입니다. 게임 창만 공유할 때 별도 오버레이를 자동 합성하는 기능은 아니며, 게임과 오버레이만 함께 공유하려면 OBS 합성이 필요합니다.

osu!는 창 모드 또는 테두리 없는 전체 화면을 사용하세요. Qwilight는 창 모드와 테두리 없는 전체 화면을 대상으로 합니다. 독점 전체 화면 위에 주입하는 기능은 없습니다. 구현 근거, 인식 범위와 확인 방법은 [게임 오버레이 설명](docs/game-overlay.md)에 있습니다. 실제 게임 플레이·OBS 캡처 검증은 별도로 필요합니다.

처음 실행할 때 설정 창은 저장된 오버레이 주변의 빈 공간에 배치합니다. 해당 모니터에 공간이 없으면 다른 모니터를 확인합니다. 두 창이 들어갈 공간 자체가 없는 화면에서는 설정 창을 화면 안에 두고 겹치는 면적을 최소화합니다. 저장된 오버레이 좌표는 바꾸지 않습니다.

### 실시간 패턴 난이도

v2.12는 **에리팩 실제 ⑤1~10에서 약 +1로 표시된다**는 피드백을 반영했습니다. 기존 추정값 ⑤2~11은 한 단계 내리고, ⑤11~13 사이에서는 감소량을 점차 줄이며 ⑤13 이상은 유지합니다. 최저값은 ⑤1입니다. 예: ⑤6→⑤5, ⑤11→⑤10, ⑤12→⑤11.5. 설정·HUD·지난 결과·새 리포트에 함께 적용되며 기존 HTML은 보존합니다. 원본 보정표·strain·NPS·다른 키 모드는 유지하고, 실제 플레이 정확도는 아직 재검증하지 않았습니다. [식과 검증 범위](docs/pattern-difficulty.md#5k-aery-저레벨-보정-v212)

게임을 플레이하거나 **난이도** 탭을 열고 등록된 키를 치면 **추정 난이도와 현재 NPS**가 큰 숫자로 나란히 갱신됩니다. 4K는 **Reform v1**, 7K는 **Jinjin Regular**, 10K는 **10K Revive 2026 Skill Simulator의 1~14단·개전(15)·오버조이(16)** 기준으로 표시합니다. 8K는 **HeavenUsurper Regular v1**의 0~10단·Flare(11)·Aqua(12)·Life(13)·Oblatus(14)·Demise(15) 기준으로 표시합니다. 5K는 AERY ⑤를 유지합니다. 다른 키 수에는 기존 0~100 입력 지수를 씁니다.

v2.12는 코스 화면과 원본 해시를 대조한 **64개 채보로 Revive 16코스**의 기준점을 계산했습니다. strain에 키 조합 변화·리듬 간격 변화·반복 부담을 더하는 기존 특징식은 유지했습니다. 현재·완료 결과·새 리포트에 같은 기준을 사용하며, 실행할 때 채보 폴더는 필요하지 않습니다. 16단계가 각각 구분되지만 실제 연주 정확도나 단위 합격을 검증한 결과는 아닙니다. [기준표·계산·한계](docs/revive10-reference.md)

단위의 높낮이는 **7단 low → 7단 mid low → 7단 수준 → 7단 high**처럼 표시합니다. 현재 분석과 지난 결과에 같은 표현을 쓰며, 비교 기준과 원래 strain ★ 값은 세부 지표에 남깁니다. 입력 부담이 비슷해 분리되지 않는 인접 단계는 `8~9단`처럼 범위로 표시합니다. 기준 최저 미만/최고 초과에는 `↓/↑`를 붙이고 높낮이를 세분하지 않습니다.

큰 NPS는 **최근 1초**, 난이도는 **최근 8초** 기준입니다. 8초 평균 NPS, 2초 순간 밀도, 동시치기, 같은 키 연타, 홀드 중 추가 입력도 설정 창에서 볼 수 있습니다.

v2.5에서는 **롱노트로 추가되는 홀드 간섭·릴리즈 부담을 기존의 2/3**로 낮췄습니다. 단타와 기본 밀도는 유지하며, 최종 단위 숫자 전체를 2/3로 곱하지는 않습니다. 기준 코스의 가중치는 1, 실시간 입력은 2/3이며 Revive에서도 유지합니다. 화면의 `기존 보정 오차`는 LN·저레벨 조정 전 채보 검증값이며, 새 가중치의 실타건 정확도를 뜻하지 않습니다.

- 처음에는 입력을 모으는 동안 **분석 중**으로 표시합니다. 단발 입력이나 한 번의 동시치기만으로 등급을 매기지 않습니다.
- 모든 키를 뗀 뒤 마지막 유효한 누름/뗌부터 **10초간 입력이 없을 때** 측정을 끝냅니다. 짧게 쉬거나 다시 입력을 모으는 동안에는 **마지막 추정값 유지**를 표시하고 같은 세션으로 이어갑니다. NPS와 입력 통계는 실제 최근 입력대로 갱신됩니다.
- 측정이 끝나면 분석과 NPS 사이에 **아까 쳤던 패턴은: 7단 mid low 난이도입니다.**처럼 결과를 표시합니다. 해당 입력 세션의 유효한 최고 8초 구간 추정치를 남기며, 대기하거나 다음 패턴을 치는 중에도 다음 결과가 확정될 때까지 유지합니다. 표본 부족·버퍼 초과 세션은 이전 결과를 덮지 않습니다.
- **분석 초기화**, 키 배치 변경, 분석 켜기/끄기, 키 등록은 화면의 지난 결과도 지웁니다. 결과는 설정 파일에는 기록하지 않습니다. 리포트 자동 저장을 켰다면 진행 중 기록을 별도 HTML로 마무리합니다.
- **OBS 오버레이에도 표시**를 끄고 적용하면 설정 창에서만 볼 수 있습니다. **입력 난이도 분석 사용**을 끄고 적용하면 집계도 멈춥니다.
- **분석 초기화**로 새 구간을 시작할 수 있습니다. 키 배치를 바꾸거나 키 등록을 시작할 때도 초기화합니다.
- 다른 설정 탭에서 값을 입력하는 동작, 키 배치 등록, 테스트 노트는 집계하지 않습니다. 난이도 탭에서는 등록된 키가 버튼을 누르는 대신 패턴 입력으로 처리됩니다.
- HUD는 청록색 실시간 난이도, 연두색 지난 결과, 노란색 NPS 순서입니다. 폭 600px 미만이면 같은 순서로 세로 배치합니다. HUD 공간만큼 노트 이동 영역이 줄어들고, HUD를 끄면 기존 이동 영역을 사용합니다.

osu!mania strain 규칙과 단위 코스 66개의 강한 구간 비교, 로컬 BMS 채보 보정을 적용한 **구간 근사값**입니다. 단위 클리어 판정이나 전체 곡의 공식 별점, 플레이어 실력 등급은 아니며 놓친 노트, 판정, 손 배치와 개인 숙련도는 알 수 없습니다. 단위 비교에는 별도 보류 표본으로 측정한 정확도가 없습니다. BMS의 참고 오차는 오프라인 채보 검증 결과이며 실제 타건 정확도를 보장하지 않습니다. 기본 상태에서는 입력 내역을 파일로 저장하지 않습니다. 리포트를 켜도 외부로 전송하지 않습니다. 계산 기준, 보정 검증 수치와 한계는 [난이도 추정 설명](docs/pattern-difficulty.md)을 참고하세요.

### 연주 그래프와 자동 리포트 (v2.10)

**난이도 → 연주 리포트 자동 저장 → 적용 + 저장**을 누릅니다. 기본값은 꺼짐이며, 켠 뒤부터 등록된 레인의 누름·뗌 시각과 분석값을 기록합니다. 모든 키를 뗀 상태에서 마지막 입력 후 10초가 지나면 설정 파일 옆 `reports` 폴더에 HTML 한 개를 저장합니다. **리포트 폴더 열기**로 찾아 브라우저에서 열 수 있습니다.

리포트에는 시간별 NPS·추정 난이도, 가장 어려웠던 관측 구간, 레인별 타건·홀드 타임라인, 연주 길이·총 누름·최대 NPS가 들어 있습니다. 실제 키 이름·입력 문자열·게임 창 이름은 저장하지 않으며, 인터넷 연결 없이 열 수 있습니다. OBS 모드의 등록 키는 게임 밖에서도 수집될 수 있으므로 기록이 필요할 때만 켜세요. 게임용 모드는 osu!·Qwilight 및 설정의 난이도 연습으로 입력을 제한합니다. 공유할 HTML에는 로컬 시각과 레인별 타이밍이 포함됩니다.

설정 변경·분석 초기화·정상 종료 때에는 진행 중 기록을 구분해서 저장합니다. 저장 실패는 화면에 표시하고 **저장 재시도**로 복구하며, 미저장 기록이 4개가 되면 새 수집을 일시 중지합니다. 강제 종료·충돌로 아직 저장되지 않은 기록은 복구되지 않습니다. [저장 범위·한도·검증](docs/session-reports.md)

## Verification

```bat
ctest --test-dir build-cpp-settings --output-on-failure
```

The eight tests cover analytical strain reference cases, session completion/retention, input sequences and key profiles, JSON/INI round trips, hidden native settings controls, application/tray lifecycle, local HTML reports, native game-process recognition, and initial window placement. They include real locked/read-only file failures, save retry, truncated JSON fallback, current NPS boundaries, minimum HUD text width, draft isolation, validation, recording/cancellation, reordering, restoring, tab visibility, monitor centering, display/toggles, and preview exclusion. Report checks cover default-off behavior, delayed timers, duplicate prevention, bounded buffers, HTML escaping, Unicode paths, failed writes, retry, opt-out, and saving partial sessions on normal exit. The first report's output is deliberately blocked in a separate workflow; unexpected self-test exceptions must return failure with a diagnostic instead of aborting. See [UI self-test diagnosis](docs/ui-selftest-recovery.md). Off-screen HUD bitmaps and test files stay under the build directory; these tests do not load personal settings, alter the desktop tray, capture the desktop, install a keyboard hook, or inject keyboard input. Game-recognition fixtures are hidden test processes, not actual games. An optional `--tray-smoke-test` briefly adds a real tray icon and checks its removal; see the lifecycle document.

Physical keyboard behavior, visual appearance at different Windows display scales, and real OBS/game capture still require interactive verification. Passing these tests does not establish in-game input latency or capture compatibility.

## OBS Capture

Use `Window Capture` and select:

```text
Key Note Overlay C++ Overlay
```

Do not capture `Key Note Overlay C++ Settings`; that is only the configuration GUI.

## Config

The local runtime config is:

```text
key_note_cpp_config.json
```

That file is ignored by Git because it can contain personal key layout and capture settings. Use `key_note_cpp_config.example.json` as a template. The app still writes `key_note_cpp_config.ini` for backward compatibility.

Optional JSON `input.preset_4k` through `input.preset_10k` strings store each key count's comma-separated custom layout. INI uses `[input_presets]` with `4K` through `10K` keys. Empty or missing entries use the built-in layout; invalid keys, duplicates, or mismatched key counts are ignored. Saves only need to write custom overrides. Existing `input.lanes` remains the active layout, so older configurations still load.

Example:

```json
{
  "overlay": {
    "x": 80,
    "y": 40,
    "width": 900,
    "height": 220,
    "duration_ms": 850,
    "travel_px": 180,
    "note_width": 58,
    "note_height": 34,
    "visual_preset": "Classic",
    "opacity_percent": 100,
    "lane_spacing_percent": 100,
    "long_note_alpha": 92,
    "debug_background": true,
    "click_through": true,
    "always_on_top": true,
    "circle_notes": false,
    "lane_background": true,
    "rounded_boxes": true,
    "thin_outline": true,
    "key_text_bottom": true,
    "difficulty_enabled": true,
    "difficulty_hud": true,
    "report_auto_save": false,
    "game_overlay": false
  },
  "input": {
    "mode": "8K",
    "lanes": ["A", "S", "D", "F", "J", "K", "L", "SEMICOLON"]
  }
}
```

Special key names are normalized when saving. For example, `;` becomes `SEMICOLON` and `[` becomes `LBRACKET`.
