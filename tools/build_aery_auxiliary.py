"""Build a bounded AERY residual helper without changing the original curve.

Model selection uses the original four training group folds. The untouched
holdout is measured only after parameters are fixed, and never selects them.
Only aggregate evidence and hashed chart/group membership are distributable.
"""
import argparse
import hashlib
import itertools
import json
import math
from pathlib import Path
import re
import statistics

BASE_SHA256 = '68f840148ca158b5798010a3956ff14cc526493477beed21f2ef5c7f48a204a1'
BASE_CANONICAL_LF_SHA256 = '1b11861b677623cabcc324d7a642f3f6020fcd8f8259be9edc39722c72f3aa2b'
FEATURES_SHA256 = 'b213ecbefc5ca3e7270db2c5edafa0e9999b6b53dcb71e6ca697e49a6ae3c52b'
LEVELS = tuple(range(1, 16)) + (15.5, 16, 16.5, 17, 17.5, 18, 18.5, 19, 19.5, 20, 20.5)
SPARSE_LEVELS = {1: 2, 2: 3, 3: 6, 4: 7, 5: 4, 7: 8}
MAX_SHIFT = 0.5
BANDWIDTH_X = (0.35, 0.6, 1.0)
BANDWIDTH_LOG_NPS = (0.2, 0.4, 0.8)
BLENDS = (0.1, 0.2, 0.3, 0.4, 0.5)


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def read_base(path):
    raw = Path(path).read_bytes()
    canonical = raw.replace(b'\r\n', b'\n')
    # Git checkouts may use LF or CRLF. Accept those exact original forms,
    # including no content changes or mixed line endings, without rewriting either.
    if (hashlib.sha256(canonical).hexdigest() != BASE_CANONICAL_LF_SHA256 or
            raw not in (canonical, canonical.replace(b'\n', b'\r\n'))):
        raise ValueError('The original calibration header changed')
    text = canonical.decode('utf-8')
    body = re.search(r'AeryCalibration = \{\{(.*?)\}\};', text, re.S)
    if body is None:
        raise ValueError('Missing original AERY calibration')
    curve = [tuple(map(float, pair)) for pair in re.findall(r'\{([\d.]+),\s*([\d.]+)\}', body[1])]
    if len(curve) != 34 or any(a[0] >= b[0] for a, b in zip(curve, curve[1:])):
        raise ValueError('Expected the original 34 ordered calibration points')
    return curve


def baseline(curve, x):
    if x <= curve[0][0]:
        return curve[0][1]
    for a, b in zip(curve, curve[1:]):
        if x <= b[0]:
            return a[1] + (b[1] - a[1]) * (x - a[0]) / (b[0] - a[0])
    return curve[-1][1]


def manual_low(value):
    reduction = 1.0 if value <= 11 else max(0.0, (13.0 - value) / 2.0)
    return max(1.0, value - reduction)


def manual_low_with_aux(base, auxiliary):
    # User feedback remains a fixed adjustment of the original base estimate;
    # an auxiliary shift must not change the amount of manual correction.
    reduction = min(1.0, max(0.0, (13.0 - base) / 2.0))
    return max(1.0, base + auxiliary - reduction)


def read_rows(path, curve):
    if sha256(path) != FEATURES_SHA256:
        raise ValueError('Features do not match the original published calibration')
    rows, seen = [], set()
    for line in Path(path).read_text(encoding='utf-8').splitlines():
        ident, keys, label, group, stars, _, nps = line.split('\t')
        if keys != '5':
            continue
        if ident in seen or not re.fullmatch('[0-9a-f]{32}', ident) or not re.fullmatch('[0-9a-f]{16}', group):
            raise ValueError('Invalid or duplicate chart/group identity')
        seen.add(ident)
        label, stars, nps = float(label), float(stars), float(nps)
        if label not in LEVELS or not all(math.isfinite(v) and v > 0 for v in (label, stars, nps)):
            raise ValueError('Invalid source measurement')
        y = math.log1p(nps)
        x = stars + y
        rows.append(dict(md5=ident, group=group, level=label, x=x, log_nps=y,
                         base=baseline(curve, x), holdout=int(group, 16) % 5 == 0,
                         fold=(int(group, 16) // 5) % 4))
    train = [r for r in rows if not r['holdout']]
    holdout = [r for r in rows if r['holdout']]
    if (len(train), len(holdout), len({r['group'] for r in train}), len({r['group'] for r in holdout})) != (1346, 328, 874, 199):
        raise ValueError('Original training/holdout membership counts changed')
    if {r['group'] for r in train} & {r['group'] for r in holdout}:
        raise ValueError('Training/holdout group leakage')
    return train, holdout


def anchors_for(rows, count=10):
    """One chart per (directory group, level), spread over composite quantiles.

    Original membership is frozen: copied/renamed chart variants never create
    additional groups. Their median-feature chart is the representative.
    """
    groups = {}
    for row in rows:
        if row['holdout']:
            raise ValueError('Holdout rows cannot become anchors')
        groups.setdefault((row['level'], row['group']), []).append(row)
    by_level = {}
    for (level, _), members in groups.items():
        ordered = sorted(members, key=lambda r: (r['x'], r['log_nps'], r['md5']))
        by_level.setdefault(level, []).append(ordered[(len(ordered) - 1) // 2])
    anchors = []
    for level in sorted(by_level):
        ordered = sorted(by_level[level], key=lambda r: (r['x'], r['log_nps'], r['group'], r['md5']))
        total = min(count, len(ordered))
        indices = [0] if total == 1 else [(i * (len(ordered) - 1) + (total - 1) // 2) // (total - 1) for i in range(total)]
        for i in indices:
            row = ordered[i]
            anchors.append(dict(md5=row['md5'], group=row['group'], level=level,
                                x=row['x'], log_nps=row['log_nps'], residual=row['level'] - row['base']))
    return anchors


def raw_delta(anchors, x, log_nps, bandwidth_x, bandwidth_y, bounds):
    if not math.isfinite(x) or not math.isfinite(log_nps) or log_nps < 0 or not bounds[0] <= x <= bounds[1]:
        return 0.0
    total, weight = 0.0, 0.0
    for anchor in anchors:
        dx = (x - anchor['x']) / bandwidth_x
        dy = (log_nps - anchor['log_nps']) / bandwidth_y
        w = math.exp(-0.5 * (dx * dx + dy * dy))
        total += w * anchor['residual']
        weight += w
    # A pseudo-observation with residual zero shrinks isolated matches.
    return total / (weight + 1.0)


def delta(anchors, x, log_nps, parameters, bounds):
    raw = raw_delta(anchors, x, log_nps, parameters['bandwidth_x'], parameters['bandwidth_log_nps'], bounds)
    return max(-MAX_SHIFT, min(MAX_SHIFT, parameters['blend'] * raw))


def error_metrics(rows, predictions):
    errors = sorted(abs(p - r['level']) for r, p in zip(rows, predictions))
    if len(errors) != len(rows) or not errors:
        raise ValueError('Incomplete evaluation')
    return dict(charts=len(errors), mae=statistics.mean(errors),
                error80=errors[math.ceil(len(errors) * 0.8) - 1])


def select_parameters(train, bounds):
    folds = []
    for fold in range(4):
        training = [r for r in train if r['fold'] != fold]
        validation = [r for r in train if r['fold'] == fold]
        if not training or not validation:
            raise ValueError('Four nonempty training group folds are required')
        anchors = anchors_for(training)
        if {a['group'] for a in anchors} & {r['group'] for r in validation}:
            raise ValueError('CV anchor/group leakage')
        folds.append((validation, anchors))
    baseline_mae = statistics.mean(error_metrics(rows, [r['base'] for r in rows])['mae'] for rows, _ in folds)
    candidates = []
    for bx, by in itertools.product(BANDWIDTH_X, BANDWIDTH_LOG_NPS):
        offsets = [[raw_delta(anchors, r['x'], r['log_nps'], bx, by, bounds) for r in rows] for rows, anchors in folds]
        for blend in BLENDS:
            fold_metrics = []
            for (rows, _), raw in zip(folds, offsets):
                predictions = [r['base'] + max(-MAX_SHIFT, min(MAX_SHIFT, blend * d)) for r, d in zip(rows, raw)]
                fold_metrics.append(error_metrics(rows, predictions))
            candidates.append(dict(bandwidth_x=bx, bandwidth_log_nps=by, blend=blend,
                                   mae=statistics.mean(m['mae'] for m in fold_metrics), folds=fold_metrics))
    selected = min(candidates, key=lambda c: (c['mae'], c['blend'], c['bandwidth_x'], c['bandwidth_log_nps']))
    return dict(baseline_mae=baseline_mae, selected=selected, candidates=candidates,
                candidate_count=len(candidates),
                baseline_caveat='The frozen original curve was fitted on all original training groups. CV measures only the incremental auxiliary helper, not a complete refit.')


def coverage(train, holdout, anchors):
    records = []
    for level in LEVELS:
        training = {r['group'] for r in train if r['level'] == level}
        testing = {r['group'] for r in holdout if r['level'] == level}
        selected = [a for a in anchors if a['level'] == level]
        required = SPARSE_LEVELS.get(level, 10)
        if len(selected) != required or len({a['group'] for a in selected}) != required:
            raise ValueError(f'Insufficient or duplicated anchors for level {level}')
        if level in SPARSE_LEVELS and len(training) != required:
            raise ValueError('Sparse-level original membership changed')
        if any(a['group'] not in training or a['group'] in testing for a in selected):
            raise ValueError('Anchor membership is not training-only')
        records.append(dict(level=level, training_groups=len(training), holdout_groups=len(testing),
                            anchors=len(selected), sparse_exception=level in SPARSE_LEVELS))
    return records


def evaluate_holdout(holdout, anchors, parameters, bounds):
    shifts = [delta(anchors, r['x'], r['log_nps'], parameters, bounds) for r in holdout]
    base = [r['base'] for r in holdout]
    aux = [r['base'] + d for r, d in zip(holdout, shifts)]
    return dict(baseline=error_metrics(holdout, base), auxiliary=error_metrics(holdout, aux),
                manual_low_baseline=error_metrics(holdout, [manual_low(p) for p in base]),
                manual_low_auxiliary=error_metrics(holdout, [manual_low_with_aux(p, d) for p, d in zip(base, shifts)]),
                max_absolute_shift=max(map(abs, shifts)))


def ship_gate(cv, result):
    # Fixed before looking at the holdout. Failed candidates are never retuned
    # using those outcomes, and no live header is emitted after a failed gate.
    tolerance = 1e-12
    return dict(cv_improves=cv['selected']['mae'] < cv['baseline_mae'],
                holdout_mae_improves_one_percent=result['auxiliary']['mae'] <= result['baseline']['mae'] * 0.99,
                holdout_error80_nonregressing=result['auxiliary']['error80'] <= result['baseline']['error80'] + tolerance,
                manual_mae_nonregressing=result['manual_low_auxiliary']['mae'] <= result['manual_low_baseline']['mae'] + tolerance,
                manual_error80_nonregressing=result['manual_low_auxiliary']['error80'] <= result['manual_low_baseline']['error80'] + tolerance,
                maximum_shift_bounded=result['max_absolute_shift'] <= MAX_SHIFT)


def render_header(anchors, parameters, bounds):
    lines = ['#pragma once', '#include <algorithm>', '#include <array>', '#include <cmath>',
             '// Generated by tools/build_aery_auxiliary.py; original calibration_tables.h is unchanged.',
             '// Training-group residual anchors; no chart events or file paths are embedded.',
             'namespace pattern {',
             'struct AeryAuxiliaryAnchor { double composite; double logNps; double residual; };',
             f'inline constexpr std::array<AeryAuxiliaryAnchor, {len(anchors)}> AeryAuxiliaryAnchors = {{{{']
    lines += [f"    {{{a['x']:.17g}, {a['log_nps']:.17g}, {a['residual']:.17g}}}," for a in anchors]
    lines += ['}};',
              f"inline constexpr double AeryAuxiliaryBandwidthX = {parameters['bandwidth_x']:.17g};",
              f"inline constexpr double AeryAuxiliaryBandwidthLogNps = {parameters['bandwidth_log_nps']:.17g};",
              f"inline constexpr double AeryAuxiliaryBlend = {parameters['blend']:.17g};",
              f'inline constexpr double AeryAuxiliaryMaxShift = {MAX_SHIFT:.17g};',
              'inline double AeryAuxiliaryDelta(double composite, double logNps) {',
              '    if (!std::isfinite(composite) || !std::isfinite(logNps) || logNps < 0.0 ||',
              f'        composite < {bounds[0]:.17g} || composite > {bounds[1]:.17g}) return 0.0;',
              '    double weightedResidual = 0.0, sumWeight = 0.0;',
              '    for (const auto& anchor : AeryAuxiliaryAnchors) {',
              '        const double dx = (composite - anchor.composite) / AeryAuxiliaryBandwidthX;',
              '        const double dy = (logNps - anchor.logNps) / AeryAuxiliaryBandwidthLogNps;',
              '        const double weight = std::exp(-0.5 * (dx * dx + dy * dy));',
              '        weightedResidual += weight * anchor.residual;',
              '        sumWeight += weight;',
              '    }',
              '    // Shrink low-support queries toward the unchanged original curve.',
              '    return std::clamp(AeryAuxiliaryBlend * weightedResidual / (sumWeight + 1.0),',
              '                      -AeryAuxiliaryMaxShift, AeryAuxiliaryMaxShift);',
              '}', '}']
    return '\n'.join(lines) + '\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--features', type=Path, required=True)
    parser.add_argument('--base', type=Path, required=True)
    parser.add_argument('--header', type=Path)
    parser.add_argument('--report', type=Path)
    parser.add_argument('--select-only', action='store_true')
    args = parser.parse_args()
    curve = read_base(args.base)
    train, holdout = read_rows(args.features, curve)
    bounds = (curve[0][0], curve[-1][0])
    anchors = anchors_for(train)
    counts = coverage(train, holdout, anchors)
    cv = select_parameters(train, bounds)
    parameters = {key: cv['selected'][key] for key in ('bandwidth_x', 'bandwidth_log_nps', 'blend')}
    if args.select_only:
        print(json.dumps(dict(anchors=len(anchors), parameters=parameters, cv_baseline_mae=cv['baseline_mae'], cv_selected_mae=cv['selected']['mae'])))
        return
    if args.header is None or args.report is None:
        parser.error('--header and --report are required for the final evaluation')
    result = evaluate_holdout(holdout, anchors, parameters, bounds)
    gates = ship_gate(cv, result)
    accepted = all(gates.values())
    report = dict(version=1, status='accepted' if accepted else 'rejected',
                  source_features_sha256=FEATURES_SHA256, original_calibration_header_sha256=BASE_SHA256,
                  original_calibration_header_lf_sha256=BASE_CANONICAL_LF_SHA256,
                  original_curve_points=34, anchors=len(anchors),
                  selected_charts=len({a['md5'] for a in anchors}), selected_groups=len({a['group'] for a in anchors}),
                  selected_anchors_sha256=hashlib.sha256(json.dumps(anchors, sort_keys=True, separators=(',', ':')).encode()).hexdigest(),
                  training_charts=len(train),
                  training_groups=len({r['group'] for r in train}), holdout_charts=len(holdout),
                  holdout_groups=len({r['group'] for r in holdout}), overlapping_groups=0,
                  split='Original directory-name group hash; group % 5 == 0 remains holdout; (group // 5) % 4 selects training CV fold',
                  selection='One median-composite chart per (level, original directory group), then 10 evenly spaced composite quantiles; use every available training group at levels 1-5 and 7',
                  features='x = original strain + log1p(peak-window NPS); y = log1p(peak-window NPS); strongest-three 8-second windows; original LN weight 1',
                  correction='clamp(blend * sum(weight * (label - frozen_C34(x))) / (sum(weight) + 1), -0.5, 0.5); Gaussian kernel; zero outside original composite range',
                  parameters=parameters, composite_range=list(bounds), max_shift=MAX_SHIFT,
                  manual_low_order='max(1, base + auxiliary - clamp((13 - base) / 2, 0, 1)); the manual reduction is determined from the original base, not the shifted estimate',
                  model_selection=cv, holdout=result, acceptance=gates, coverage=counts, selected_anchors=anchors,
                  limitations=['Reuses existing training data; it is not new independent evaluation data.',
                               'Directory groups preserve the original split but do not certify semantic song identity.',
                               'Sparse levels use fewer than 10 original training groups by explicit exception.',
                               'The holdout is an offline chart-label proxy with LN weight 1, not live input accuracy; live LN weight remains 2/3.',
                               'The manual low-level feedback adjustment is retained, not refitted to table labels.',
                               'Parameters were frozen by training-only CV before one holdout evaluation; failed gates do not trigger holdout-guided retuning.'])
    args.report.write_text(json.dumps(report, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')
    print(json.dumps(dict(status=report['status'], anchors=len(anchors), parameters=parameters, holdout=result, acceptance=gates)))
    if not accepted:
        raise SystemExit(2)
    args.header.write_text(render_header(anchors, parameters, bounds), encoding='utf-8')


if __name__ == '__main__':
    main()
