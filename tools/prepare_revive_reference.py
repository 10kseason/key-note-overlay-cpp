"""Offline, hash-pinned Revive 2026 reference preparation. Never opens audio.

prepare writes a private path manifest for ExtractRevivePatterns.java. assemble
joins its decoded stages for the existing pattern_features executable. Outputs
belong in an ignored build directory; only the curated metadata is distributable.
"""
import argparse
import hashlib
import json
import re
from pathlib import Path


SOURCE_NAME = '10K Revive 2026 Skill Simulator'
CHART_SUFFIXES = {'.bms', '.bme', '.bml'}
GAP_MS = 10000


def load_courses(path):
    document = json.loads(Path(path).read_text(encoding='utf-8'))
    courses = document['courses']
    if document.get('name') != SOURCE_NAME or [r['rank'] for r in courses] != list(range(1, 17)):
        raise ValueError('Expected the ordered 16 Revive 2026 courses')
    for course in courses:
        if [s['order'] for s in course['stages']] != [1, 2, 3, 4]:
            raise ValueError('Each course requires exactly four ordered stages')
        for stage in course['stages']:
            for key, length in (('sha256', 64), ('md5', 32)):
                if not re.fullmatch('[0-9a-f]{' + str(length) + '}', stage[key]):
                    raise ValueError('Invalid chart hash')
    return document


def resolve_charts(root, courses):
    """Require byte identity; filenames and titles never substitute for hashes."""
    expected = {s['sha256']: s['md5'] for c in courses for s in c['stages']}
    found = {}
    for path in sorted(Path(root).rglob('*')):
        if not path.is_file() or path.suffix.lower() not in CHART_SUFFIXES:
            continue
        raw = path.read_bytes()
        sha = hashlib.sha256(raw).hexdigest()
        if sha not in expected:
            continue
        if hashlib.md5(raw).hexdigest() != expected[sha]:
            raise ValueError('Chart MD5/SHA256 identity disagreement')
        # Identical copies are harmless; choosing a stable path is reproducible.
        found.setdefault(sha, path.resolve())
    missing = sorted(set(expected) - set(found))
    if missing:
        raise ValueError(f'{len(missing)} required chart(s) missing: ' + ', '.join(missing))
    return found


def read_notes(path):
    notes = []
    for line in Path(path).read_text(encoding='utf-8').splitlines():
        fields = line.split()
        if len(fields) != 3:
            raise ValueError('Invalid decoded event')
        lane, start, end = map(int, fields)
        if not 0 <= lane < 10 or start < 0 or end < start:
            raise ValueError('Invalid lane or hold duration')
        notes.append((lane, start, end))
    if not notes:
        raise ValueError('Decoded chart is empty')
    return sorted(notes, key=lambda n: (n[1], n[0], n[2]))


def join_stages(stages, gap_ms=GAP_MS):
    if gap_ms < 8000:
        raise ValueError('Stage gap must isolate the 8-second estimator windows')
    joined, cursor = [], 0
    for notes in stages:
        origin = min(n[1] for n in notes)
        end = max(n[2] for n in notes)
        joined.extend((lane, start - origin + cursor, stop - origin + cursor)
                      for lane, start, stop in notes)
        cursor += end - origin + gap_ms
    return joined


def course_id(course):
    # Grade and ordering are part of identity even if two courses reuse charts.
    identity = [course['rank'], [s['sha256'] for s in course['stages']], GAP_MS]
    return hashlib.sha256(json.dumps(identity, separators=(',', ':')).encode()).hexdigest()


def prepare(document, root, output):
    found = resolve_charts(root, document['courses'])
    rows = []
    for sha, path in sorted(found.items()):
        if any(char in str(path) for char in '\t\r\n'):
            raise ValueError('Chart path contains manifest delimiters')
        rows.append(f'{sha}\t{path}')
    output.mkdir(parents=True, exist_ok=True)
    (output / 'stage-manifest.tsv').write_text('\n'.join(rows) + '\n', encoding='utf-8')
    return len(rows)


def assemble(document, output):
    courses = []
    all_notes = {}
    stage_notes = {}
    stage_manifest = []
    # Validate everything before replacing any derived course output.
    for course in document['courses']:
        stages = [read_notes(output / 'stages' / (s['sha256'] + '.txt')) for s in course['stages']]
        for stage, notes in zip(course['stages'], stages):
            if stage['sha256'] not in stage_notes:
                stage_notes[stage['sha256']] = notes
                stage_manifest.append(f"{stage['sha256']}\t10\t{course['rank']}\tstage{stage['order']}\n")
        ident = course_id(course)
        notes = join_stages(stages)
        all_notes[ident] = notes
        courses.append(dict(keys=10, rank=course['rank'], label=course['label'],
                            sha256=ident, source=SOURCE_NAME,
                            identity='SHA256 of grade, ordered chart hashes and analysis gap; not a chart-file hash',
                            stage_gap_ms=GAP_MS, stages=course['stages'],
                            notes=len(notes), long_notes=sum(b > a for _, a, b in notes)))
    (output / 'notes').mkdir(parents=True, exist_ok=True)
    for ident, notes in all_notes.items():
        (output / 'notes' / (ident + '.txt')).write_text(
            ''.join(f'{lane} {start} {end}\n' for lane, start, end in notes), encoding='utf-8')
    (output / 'decoded.tsv').write_text(''.join(
        f"{c['sha256']}\t10\t{c['rank']}\tRevive2026\n" for c in courses), encoding='utf-8')
    (output / 'courses.json').write_text(json.dumps(courses, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    # Optional per-stage pattern_features output helps audit a course dominated
    # by one song; it never changes the strongest-three course statistic.
    audit_root = output / 'stage-analysis'
    (audit_root / 'notes').mkdir(parents=True, exist_ok=True)
    for ident, notes in stage_notes.items():
        (audit_root / 'notes' / (ident + '.txt')).write_text(
            ''.join(f'{lane} {start} {end}\n' for lane, start, end in notes), encoding='utf-8')
    (audit_root / 'decoded.tsv').write_text(''.join(stage_manifest), encoding='utf-8')
    return len(courses)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action', choices=('prepare', 'assemble'))
    parser.add_argument('--courses', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--root', type=Path)
    args = parser.parse_args()
    document = load_courses(args.courses)
    if args.action == 'prepare':
        if args.root is None:
            parser.error('prepare requires --root')
        print('matched_charts=' + str(prepare(document, args.root, args.output)))
    else:
        print('assembled_courses=' + str(assemble(document, args.output)))


if __name__ == '__main__':
    main()
