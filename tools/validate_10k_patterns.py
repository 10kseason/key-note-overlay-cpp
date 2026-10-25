"""Reference-fit and deterministic perturbation diagnostics, not held-out accuracy.

Reads the ignored 12-course note extraction and runs the native feature extractor.
Only the aggregate report is suitable for distribution; generated note copies are not.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import subprocess


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('root', type=Path)
    parser.add_argument('--extractor', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    courses = json.loads((args.root / 'courses.json').read_text(encoding='utf-8'))
    if len(courses) != 12 or {r['rank'] for r in courses} != set(range(1, 13)) or any(r['keys'] != 10 for r in courses):
        raise ValueError('Exactly the 12 selected 10K BMS courses are required')
    anchors = {}
    for line in (args.root / 'features.tsv').read_text().splitlines():
        ident, keys, rank, group, load, baseline, nps = line.split('\t')
        anchors[int(rank)] = float(load)
    ordered = [anchors[i] for i in range(1, 13)]
    if not all(b > a for a, b in zip(ordered, ordered[1:])):
        raise ValueError('Reference courses are not individually separable')
    def grade(load):
        return 1 + sum(load >= (a+b)/2 for a, b in zip(ordered, ordered[1:]))

    args.output.mkdir(parents=True, exist_ok=False)
    (args.output / 'notes').mkdir()
    manifest, metadata = [], {}
    for course in courses:
        original = [tuple(map(int, line.split())) for line in
                    (args.root / 'notes' / f"{course['sha256']}.txt").read_text().splitlines()]
        for variant in ('jitter_3ms_seed1', 'jitter_3ms_seed2', 'rate_0.98', 'rate_1.02', 'mirror'):
            rng = random.Random(course['rank'] * 100 + (2 if variant.endswith('seed2') else 1))
            rate = float(variant[5:]) if variant.startswith('rate_') else 1.0
            changed = []
            for lane, start, end in original:
                jitter = rng.randint(-3, 3) if variant.startswith('jitter_') else 0
                changed.append((9-lane if variant == 'mirror' else lane,
                                round(start/rate)+jitter, round(end/rate)+jitter))
            changed.sort(key=lambda n: (n[1], n[0], n[2]))
            data = ''.join(f'{lane} {start} {end}\n' for lane, start, end in changed)
            ident = hashlib.sha256((variant + data).encode()).hexdigest()
            (args.output / 'notes' / f'{ident}.txt').write_text(data, encoding='ascii')
            manifest.append(f"{ident}\t10\t{course['rank']}\t{ident[:16]}\n")
            metadata[ident] = dict(rank=course['rank'], variant=variant)
    (args.output / 'decoded.tsv').write_text(''.join(manifest), encoding='ascii')
    # ASCII '.' avoids narrow-argv encoding ambiguity under Windows in Unicode workspaces.
    subprocess.run([str(args.extractor.resolve()), '.'], cwd=args.output, check=True)
    results = []
    for line in (args.output / 'features.tsv').read_text().splitlines():
        ident, keys, rank, group, load, baseline, nps = line.split('\t')
        row = dict(metadata[ident], load=float(load))
        row['estimated_rank'] = grade(row['load'])
        row['rank_delta'] = row['estimated_rank'] - row['rank']
        row['relative_load_change'] = row['load'] / anchors[row['rank']] - 1
        results.append(row)
    if len(results) != 60:
        raise ValueError('A perturbed course did not produce usable windows')
    report = dict(version=1, method='native 8-second comparison metric; mean of strongest three windows at 2-second steps; reference hold weight 1',
                  limitation='12 calibration courses reused; perturbations are synthetic, not independent charts or player accuracy',
                  formula='stars + 1.6*variety + 0.5*rhythm + 0.4*column_repeats + 0.1*adjacent_repeats',
                  reference_individual_bands=12,
                  minimum_relative_anchor_gap=min(b/a-1 for a,b in zip(ordered,ordered[1:])),
                  anchors=[dict(rank=i, load=anchors[i]) for i in range(1,13)], perturbations={})
    for variant in sorted({r['variant'] for r in results}):
        selected = [r for r in results if r['variant'] == variant]
        report['perturbations'][variant] = dict(courses=len(selected), exact=sum(r['rank_delta']==0 for r in selected),
            max_absolute_rank_delta=max(abs(r['rank_delta']) for r in selected),
            max_relative_load_change=max(abs(r['relative_load_change']) for r in selected), results=selected)
    args.report.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({k:{field:value for field,value in v.items() if field!='results'}
                      for k,v in report['perturbations'].items()}))


if __name__ == '__main__':
    main()
