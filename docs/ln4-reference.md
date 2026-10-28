# 4K LN Dan Courses v2 보조 기준 — 2026-10-28

`_underjoy`의 **원본 4K LN v2 1~15단 Marathon**을 추가했다. Level 1/2/3, Extra Level, FINAL의 5팩이며 5K·8K 변환본, KeyWeaver 편집본과 다른 제작자 버전은 제외했다. 메타데이터·노트 수·원본 및 이벤트 SHA-256은 [구성 보고서](ln4-reference-report.json)에 있다. 원본 채보·음악·osu! 설정은 수정하지 않는다.

## 표시와 해석

- 일반 **4K Reform v1** 값은 유지하고 별도 `4K LN v2 · LN 8단 수준` 줄을 표시한다. 실시간 입력, 완료 결과의 넓은 HUD, osu! 원본 4K 선택 채보, 연주 HTML 리포트, 파일 분석기의 기본 가중치 모드에 적용한다.
- 수동 4K 또는 자동으로 4K가 확정된 분석도 같은 LN 기준을 쓴다. 키 수 추정 중/충돌/분석 부족/한도 초과 때는 값을 보류한다. 5K 등 다른 분석 기준에는 적용하지 않는다.
- LN 입력량에 따라 주 단위를 자동 교체하거나 두 단위를 합산하지 않는다. 같은 관측 부담을 일반 코스와 LN 코스에 각각 비교한 값이다. 단위 합격, 정확도, 지구력이나 개인별 LN 실력을 측정하는 값은 아니다.
- 11~15는 Yoake·Yuugure·Yoru·Yami·Yume 코스의 원래 숫자 단위를 사용한다. Reform의 α~ζ 명칭을 재사용하지 않는다.

## 계산 방식

기존 실시간 계산기를 파일 전체에 재생한다. 최근 최대 8초 창, 0.1초 갱신·각 누름 직전·파일 끝 평가, LN 부담 **2/3**, 최고 유효 구간이라는 현재 앱과 같은 조건이다. 기존 Reform 기준표와 주 난이도 수식은 바꾸지 않았다. 이 새 표를 기준 채보 측정용 가중치 1 모드에는 적용하지 않는다.

인접 단위 순서가 측정값에서 뒤집히면 평균으로 묶어 범위를 드러낸다. 원본 3단은 약 3.08598, 4단은 약 2.88550으로 나와 **3~4단** 하나의 기준 구간으로 묶였다. 총 15코스·14구간이다. 다른 구간은 원래 숫자 순서를 유지한다. 경계는 이웃 기준값의 중간점이고, 기존 low / mid low / 수준 / high 표시를 공유한다.

기준 구성 자료를 다시 비교한 결과이며 별도의 미사용 채보나 플레이어 클리어 데이터로 검증한 정확도는 아니다. 기본 4K 계산기에서 LN 순서 역전이 발생했다는 한계도 유지한다.

## 재현

```powershell
python tools/build_ln4_reference.py prepare --songs <Songs> --root <private-output>
cmake --build <build-dir> --target ln4_reference
# <private-output>를 작업 디렉터리로 사용하여 실행
<build-dir>/ln4_reference.exe .
python tools/build_ln4_reference.py build --root <private-output> --header src/ln4_reference.h --report docs/ln4-reference-report.json
```

출력 폴더에는 필요한 노트 텍스트와 해시만 만들며 오디오는 읽거나 복사하지 않는다. 공개용 보고서에는 원본의 절대 경로를 넣지 않는다. 정확히 1~15단이 모두 있어야 생성하며 같은 단위의 원본 내용이 충돌하면 중단한다. 단위 기준점·역전 범위·범위 밖 표시·다른 키 모드 차단, 실시간 수동 4K 경로와 HUD 렌더링, 파일 리포트의 가중치 구분을 자동 검사한다.
