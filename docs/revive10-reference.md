# 10K Revive 2026 기준 v2.12

10키 입력은 **10K Revive 2026 Skill Simulator**의 1~14단·개전(15)·오버조이(16)과 비교한다. v2.11의 Tryr BMS Dans 기준을 교체했다. 사용자 제공 코스 화면의 64개 곡명·채보명·개별 LV를 로컬 원본과 대조했고 모두 하나씩 일치했다. **개별 곡 LV는 코스의 단위 번호로 사용하지 않는다.** 원본 바이트의 MD5/SHA-256과 순서는 [코스 목록](revive10-courses.json)에 있다. 온라인 표가 연결되지 않아 최신 서버 데이터와의 동기화까지 확인한 것은 아니다.

실행 중에는 채보 폴더가 없어도 된다. 앱에 넣는 것은 계산된 기준점이며, 원본 BMS·오디오·이미지·개인 경로는 배포하지 않는다. 실시간 입력으로 곡명·게임 판정·클리어 여부를 알아내는 기능도 아니다.

## 계산과 기준점

v2.8의 10키 부담 계산을 그대로 사용한다. 새 16코스에 맞춰 계수를 다시 조정하지 않았다.

```text
부담 = strain + 1.6 × 조합 변화량 + 0.5 × 리듬 변화량
              + 0.4 × 같은 열 반복 부담 + 0.1 × 인접 묶음 반복 부담
```

각 코스의 네 채보를 순서대로 분석하되 사이에 10초의 분석용 공백을 둔다. 서로 다른 곡의 입력이 같은 8초 창에 섞이지 않게 하기 위한 것이며, 실제 코스의 휴식 시간이나 체력을 모사하지 않는다. 2초마다 평가한 8초 창 중 **부담이 큰 세 창의 평균**이 기준점이다. 따라서 네 곡 전체의 평균 난이도나 코스 합격 난이도가 아니다. CN도 물리적인 누름·홀드·뗌으로 변환하며 릴리즈 판정·게이지는 모사하지 않는다.

| 단위 | 패턴 부담 기준점 |
| --- | ---: |
| 1 | 3.37970 |
| 2 | 5.05468 |
| 3 | 8.36643 |
| 4 | 9.45195 |
| 5 | 11.94380 |
| 6 | 12.46370 |
| 7 | 12.65030 |
| 8 | 14.30990 |
| 9 | 16.06350 |
| 10 | 16.36590 |
| 11 | 16.96660 |
| 12 | 18.18570 |
| 13 | 18.70540 |
| 14 | 20.60020 |
| 개전 (15) | 22.79470 |
| 오버조이 (16) | 25.66800 |

가장 가까운 기준을 고르고 인접 기준의 중간값에서 단위를 바꾼다. low / mid low / 수준 / high와 범위 밖 화살표를 유지한다. 이번 측정은 16개 기준이 모두 증가해 각각 구분된다. 가장 가까운 6/7단 기준은 약 1.5% 차이여서 작은 입력 차이에도 등급이 바뀔 수 있다. 추후 역전되는 기준이 생기면 생성 도구는 인접 단계를 묶어 범위를 드러낸다.

기준 채보는 기존과 같이 LN 가중치 1, 실시간 입력은 사용자의 기존 설정 방향에 따라 LN 추가 부담 2/3을 적용한다. 설정·HUD·지난 결과·새 HTML 리포트가 같은 Revive 기준을 사용하며 저장된 과거 리포트는 바꾸지 않는다. 4K·7K·8K 기준과 5K 원본 보정표는 유지한다. 같은 버전의 5K 저레벨 조정은 [별도 변경](pattern-difficulty.md#5k-aery-저레벨-보정-v212)이다.

## 재현

원본 팩과 별도 설치한 Java 21·beatoraja JAR가 필요하다. 아래 자리표시는 실제 경로로 바꾼다. 새 출력 폴더를 사용한다. JAR는 배포하지 않는다.

```powershell
python -B tools/prepare_revive_reference.py prepare --courses docs/revive10-courses.json --root <chart-root> --output build-cpp-settings/revive-new
javac -encoding UTF-8 -classpath <beatoraja.jar> -d build-cpp-settings/revive-new tools/ExtractRevivePatterns.java
java -cp "build-cpp-settings/revive-new;<beatoraja.jar>" ExtractRevivePatterns build-cpp-settings/revive-new/stage-manifest.tsv build-cpp-settings/revive-new
python -B tools/prepare_revive_reference.py assemble --courses docs/revive10-courses.json --output build-cpp-settings/revive-new
cmake --build <build-dir> --target pattern_features
<build-dir>/pattern_features.exe build-cpp-settings/revive-new --windows
python -B tools/build_revive_reference.py build-cpp-settings/revive-new --courses docs/revive10-courses.json --header src/revive10_reference.h --report docs/revive10-reference-report.json --extractor <build-dir>/pattern_features.exe --decoder <beatoraja.jar>
$env:REVIVE_DECODER_JAR = "<beatoraja.jar>"
python -B -m unittest discover -s tests -p "test_revive*.py" -v
```

추출기는 해시 불일치·필수 채보 누락·스크래치·지뢰·분기형 채보·디코더 오류·지원하지 않는 LN을 거부한다. 64개 원본에서 102,300개 누름과 2,111개 홀드(CN 1,485개)를 추출했다. 기준 생성은 16개 코스의 키 수·등급·순서·해시·측정값이 모두 맞아야 한다. 집계값과 생성에 쓴 바이너리 해시는 [측정 보고서](revive10-reference-report.json)에 기록한다.

`src/dan_reference.h`와 `docs/dan-reference-report.json`의 과거 Tryr 10K 자료는 재현용으로 보존한다. 현재 10K 경로는 별도 `src/revive10_reference.h`를 참조하므로 과거 도구 재실행으로 Revive 기준이 덮이지 않는다. 원래 특징식의 설명과 과거 검사는 [v2.8 기록](10k-pattern-model.md)에 있다.

원본 64개 일치·디코딩·기준점 회귀는 확인했지만, 독립 채보·실제 연주·단위 합격 확률의 정확도 검증은 아니다. 특히 실시간 입력은 놓친 노트를 알 수 없고 코스의 체력·연속 플레이·게이지·배속 조건을 판단하지 않는다.
