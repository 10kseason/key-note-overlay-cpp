"""Build monotonic, explicitly ambiguous Dan reference bands; no fitted SR ratios."""
import argparse
import hashlib
import json
import math
from pathlib import Path
from prepare_dan_reference import REQUIRED_RANKS


def bands(rows):
    blocks = []
    for row in sorted(rows, key=lambda r: r['rank']):
        blocks.append(dict(total=row['strain'], count=1, low=row['rank'], high=row['rank']))
        # Adjacent reference courses can reverse under an 8-second strain proxy.
        # Pool those ranks and expose the ambiguity instead of inventing an order.
        while len(blocks) > 1 and blocks[-2]['total']/blocks[-2]['count'] >= blocks[-1]['total']/blocks[-1]['count']:
            b, a = blocks.pop(), blocks.pop()
            blocks.append(dict(total=a['total']+b['total'], count=a['count']+b['count'], low=a['low'], high=b['high']))
    return [dict(strain=b['total']/b['count'], low=b['low'], high=b['high']) for b in blocks]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('root', type=Path)
    parser.add_argument('--header', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    metadata = {r['sha256']:r for r in json.loads((args.root/'courses.json').read_text(encoding='utf-8'))}
    rows = []
    seen = set()
    for line in (args.root/'features.tsv').read_text().splitlines():
        ident, keys, rank, group, measurement, baseline, nps = line.split('\t')
        if ident in seen:
            raise ValueError('Duplicate course feature row')
        seen.add(ident)
        row = dict(metadata[ident], strain=float(measurement), nps=float(nps))
        if row['keys'] != int(keys) or row['rank'] != int(rank):
            raise ValueError('Feature metadata mismatch')
        if not all(math.isfinite(row[k]) and row[k] > 0 for k in ('strain', 'nps')):
            raise ValueError('Invalid course measurement')
        rows.append(row)
    expected = {(k,r) for k, ranks in REQUIRED_RANKS.items() for r in ranks}
    if len(rows) != len(expected) or {(r['keys'],r['rank']) for r in rows} != expected:
        raise ValueError(f'All {len(expected)} selected courses must have usable windows')
    header = ['#pragma once','#include <array>',
              '// Generated from selected course references; see docs/dan-reference-report.json.',
              'namespace pattern {','struct DanBand { double strain; int low; int high; };']
    report = dict(version=4, statistic='mean of strongest three overlapping 8-second comparison-load windows per course; 2-second step; 4/7/8K use strain, 10K uses pattern load',
                  assignment='nearest reference band, midpoint boundaries; descending adjacent references pooled as a rank range',
                  height='low / mid low / level / high; half-band normalized below and above anchor; anchor = level; no height outside reference bounds',
                  evaluation='reference construction only; no held-out accuracy or player-clear prediction claimed',
                  features_sha256=hashlib.sha256((args.root/'features.tsv').read_bytes()).hexdigest(), profiles={})
    sources = {4:('Reform v1','https://www.danreform.com/'),
               7:('Jinjin Regular','https://osu.ppy.sh/community/forums/topics/449279'),
               8:('HeavenUsurper 8K Regular v1','https://osu.ppy.sh/beatmapsets/2336219'),
               10:('Tryr 10K BMS Dans Phase I-III','https://osu.ppy.sh/beatmapsets/1093043')}
    for keys in REQUIRED_RANKS:
        selected = [r for r in rows if r['keys']==keys]
        curve = bands(selected)
        if keys == 10 and len(curve) != len(selected):
            raise ValueError('10K pattern-load references no longer separate every course; review the measurement before release')
        header.append(f'inline constexpr std::array<DanBand, {len(curve)}> Dan{keys}Reference = {{{{')
        header += [f"    {{{b['strain']:.9f}, {b['low']}, {b['high']}}}," for b in curve]
        header.append('}};')
        name, source = sources[keys]
        report['profiles'][str(keys)] = dict(name=name, source=source, courses=len(selected), bands=curve, references=selected)
        if keys == 8:
            report['profiles']['8']['extra_labels'] = {'11':'Flare', '12':'Aqua', '13':'Life', '14':'Oblatus', '15':'Demise'}
            report['profiles']['8']['pack_sources'] = [f'https://osu.ppy.sh/beatmapsets/{ident}' for ident in
                                                     (2336219, 2338305, 2593505, 2595237, 2596836)]
        if keys == 10:
            report['profiles']['10']['metric'] = 'stars + 1.6*pattern_variety + 0.5*rhythm_complexity + 0.4*column_repeat_load + 0.1*adjacent_repeat_load'
            report['profiles']['10']['evaluation'] = 'weights calibrated on these 12 courses; individual reference separation is not held-out accuracy or clear prediction'
            report['profiles']['10']['extra_labels'] = {'11': 'Kaiden', '12': 'Catastrophe'}
            report['profiles']['10']['rank_note'] = '11/12 are ordering indices for named courses, not numeric Dan labels'
            report['profiles']['10']['pack_sources'] = [f'https://osu.ppy.sh/beatmapsets/{ident}' for ident in
                                                      (1093043, 1093064, 1093066)]
        print(f'{keys}K: courses={len(selected)}, bands={len(curve)}, pooled='+str([(b['low'],b['high']) for b in curve if b['low']!=b['high']]))
    header.append('}')
    args.header.write_text('\n'.join(header)+'\n', encoding='utf-8')
    args.report.write_text(json.dumps(report,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')


if __name__ == '__main__':
    main()
