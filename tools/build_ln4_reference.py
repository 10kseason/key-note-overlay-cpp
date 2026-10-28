"""Prepare/measure only original 4K LN v2 courses; never modify the song library."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import re
from prepare_dan_reference import parse_chart
from build_dan_reference import bands

PACKS = {'Level 1': range(1, 5), 'Level 2': range(5, 8), 'Level 3': range(8, 11),
         'Extra Level': range(11, 14), 'FINAL': range(14, 16)}
NAMES = {11: 'Yoake', 12: 'Yuugure', 13: 'Yoru', 14: 'Yami', 15: 'Yume'}


def select(info, keys):
    if keys != 4 or info.get('Creator') != '_underjoy':
        return None
    title = re.fullmatch(r'4K LN Dan Courses v2 - (Level [123]|Extra Level|FINAL) -', info.get('Title', ''))
    version = re.fullmatch(r'(\d+)(?:st|nd|rd|th) Dan(?: - (\w+))? \(Marathon\)', info.get('Version', ''))
    if not title or not version:
        return None
    rank = int(version[1])
    return rank if rank in PACKS[title[1]] and version[2] == NAMES.get(rank) else None


def prepare(songs, output):
    rows = {}
    for folder in sorted(songs.glob('*4K LN Dan Courses v2*')):
        for path in sorted(folder.glob('*.osu')):
            raw = path.read_bytes()
            try:
                info, keys, notes = parse_chart(raw)
            except ValueError:
                continue  # Unsupported converted key counts are not references.
            rank = select(info, keys)
            if rank is None:
                continue
            events = ''.join(f'{lane} {start} {end}\n' for lane, start, end in notes)
            row = dict(rank=rank, keys=4, title=info['Title'], version=info['Version'], creator=info['Creator'],
                       beatmap_id=int(info['BeatmapID']), beatmapset_id=int(info['BeatmapSetID']),
                       sha256=hashlib.sha256(raw).hexdigest(), events_sha256=hashlib.sha256(events.encode()).hexdigest(),
                       notes=len(notes), holds=sum(end > start for _, start, end in notes), events=events)
            if rank in rows and rows[rank]['events_sha256'] != row['events_sha256']:
                raise ValueError(f'Conflicting original LN rank {rank}')
            rows[rank] = row
    if set(rows) != set(range(1, 16)):
        raise ValueError(f'Expected all 15 original courses, got {sorted(rows)}')
    (output / 'notes').mkdir(parents=True, exist_ok=True)
    for row in rows.values():
        (output / 'notes' / (row['sha256'] + '.txt')).write_text(row.pop('events'), encoding='ascii')
    ordered = [rows[rank] for rank in sorted(rows)]
    (output / 'courses.json').write_text(json.dumps(ordered, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    (output / 'decoded.tsv').write_text(''.join(f"{r['sha256']}\t{r['rank']}\n" for r in ordered), encoding='ascii')
    print('Prepared 15 original 4K LN v2 courses; converted charts excluded.')


def build(root, header, report):
    rows = {r['sha256']: r for r in json.loads((root / 'courses.json').read_text(encoding='utf-8'))}
    measured = []
    for line in (root / 'measurements.tsv').read_text().splitlines():
        ident, rank, strain, nps = line.split('\t')
        row = dict(rows[ident], strain=float(strain), nps=float(nps))
        if row['rank'] != int(rank) or not all(math.isfinite(row[k]) and row[k] > 0 for k in ('strain', 'nps')):
            raise ValueError('Invalid measurement')
        measured.append(row)
    if len(measured) != 15 or {r['rank'] for r in measured} != set(range(1, 16)):
        raise ValueError('Expected exactly one measurement per rank')
    curve = bands(measured)
    lines = ['#pragma once', '#include "dan_reference.h"',
             '// Generated from original 4K LN v2; see docs/ln4-reference-report.json.', 'namespace pattern {',
             f'inline constexpr std::array<DanBand, {len(curve)}> Dan4LnReference = {{{{']
    lines += [f"    {{{b['strain']:.17g}, {b['low']}, {b['high']}}}," for b in curve]
    lines += ['}};', '}']
    header.write_text('\n'.join(lines) + '\n', encoding='utf-8')
    data = dict(name='4K LN Dan Courses v2', creator='_underjoy', courses=15,
                statistic='highest valid live-replay window; 100ms refresh plus pre-keydown and EOF; hold weight 2/3',
                assignment='nearest reference band; reversed adjacent ranks pooled and shown as a range',
                evaluation='reference construction only; no held-out or clear-prediction validation',
                sources=[f'https://osu.ppy.sh/beatmapsets/{i}' for i in sorted({r['beatmapset_id'] for r in measured})],
                measurements_sha256=hashlib.sha256((root / 'measurements.tsv').read_bytes()).hexdigest(),
                bands=curve, references=sorted(measured, key=lambda r: r['rank']))
    report.write_text(json.dumps(data, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    pooled = [(b['low'], b['high']) for b in curve if b['low'] != b['high']]
    print(f'Generated {len(curve)} bands; pooled ranks: {pooled}')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('mode', choices=['prepare', 'build'])
    parser.add_argument('--root', type=Path, required=True)
    parser.add_argument('--songs', type=Path)
    parser.add_argument('--header', type=Path)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    if args.mode == 'prepare':
        prepare(args.songs, args.root)
    else:
        build(args.root, args.header, args.report)
