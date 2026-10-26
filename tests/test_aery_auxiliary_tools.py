"""Synthetic auxiliary-model guards; never uses private source/holdout charts."""
import copy
import hashlib
import json
import math
import os
from pathlib import Path
import random
import re
import shutil
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from build_aery_auxiliary import (BASE_SHA256, BASE_CANONICAL_LF_SHA256, FEATURES_SHA256, LEVELS, MAX_SHIFT, SPARSE_LEVELS, anchors_for,
                                  baseline, coverage, delta, error_metrics,
                                  manual_low_with_aux, read_base, render_header,
                                  ship_gate)


def row(number, level=8, x=None, holdout=False):
    # Different original groups remain different even at the same feature value.
    group = number * 5 + (0 if holdout else 1)
    return dict(md5=f'{number:032x}', group=f'{group:016x}', level=level,
                x=float(number if x is None else x), log_nps=2.0, base=level - 0.4,
                holdout=holdout, fold=(group // 5) % 4)


class AeryAuxiliaryTests(unittest.TestCase):
    def test_group_representative_quantiles_are_deterministic(self):
        rows = [row(i, x=i / 10) for i in range(1, 31)]
        variant = dict(rows[0], md5='f' * 32, x=0.15)
        rows.append(variant)
        expected = anchors_for(rows)
        shuffled = list(rows)
        random.Random(7).shuffle(shuffled)
        self.assertEqual(expected, anchors_for(shuffled))
        self.assertEqual(len(expected), 10)
        self.assertEqual(len({a['group'] for a in expected}), 10)
        self.assertEqual((expected[0]['x'], expected[-1]['x']), (0.1, 3.0))
        self.assertEqual(expected[0]['md5'], rows[0]['md5'])

    def test_holdout_never_becomes_an_anchor(self):
        with self.assertRaisesRegex(ValueError, 'Holdout'):
            anchors_for([row(1), row(2, holdout=True)])

    def test_all_sparse_groups_and_ten_per_other_level(self):
        rows = []
        ident = 1
        for level in LEVELS:
            for _ in range(SPARSE_LEVELS.get(level, 12)):
                rows.append(row(ident, level, x=level + ident / 10000))
                ident += 1
        anchors = anchors_for(rows)
        counts = coverage(rows, [], anchors)
        self.assertEqual(len(anchors), 230)
        self.assertEqual(len(counts), 26)
        self.assertEqual(sum(c['sparse_exception'] for c in counts), 6)
        with self.assertRaisesRegex(ValueError, 'Insufficient'):
            coverage(rows, [], anchors[:-1])
        leaked = copy.deepcopy(anchors)
        leaked[0]['group'] = 'ffffffffffffffff'
        with self.assertRaisesRegex(ValueError, 'membership'):
            coverage(rows, [], leaked)

    def test_baseline_interpolation_and_no_aux_outside_range(self):
        curve = [(1.0, 2.0), (3.0, 4.0)]
        self.assertEqual([baseline(curve, x) for x in (0, 1, 2, 3, 4)], [2, 2, 3, 4, 4])
        anchors = [dict(x=2.0, log_nps=2.0, residual=100.0)]
        params = dict(bandwidth_x=0.5, bandwidth_log_nps=0.5, blend=0.5)
        self.assertEqual(delta(anchors, 2, 2, params, (1, 3)), MAX_SHIFT)
        self.assertEqual(delta([dict(anchors[0], residual=-100)], 2, 2, params, (1, 3)), -MAX_SHIFT)
        for x, y in ((0.99, 2), (3.01, 2), (math.nan, 2), (math.inf, 2),
                     (2, math.nan), (2, math.inf), (2, -1)):
            self.assertEqual(delta(anchors, x, y, params, (1, 3)), 0)
        self.assertEqual(delta([], 2, 2, params, (1, 3)), 0)

    def test_manual_reduction_uses_unshifted_base(self):
        self.assertEqual(manual_low_with_aux(12, 0.5), 12)
        self.assertEqual(manual_low_with_aux(12, -0.5), 11)
        self.assertEqual(manual_low_with_aux(11, 0.5), 10.5)
        self.assertEqual(manual_low_with_aux(13, 0.5), 13.5)
        self.assertEqual(manual_low_with_aux(1, -0.5), 1)

    def test_header_requires_original_calibration_bytes(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / 'fake.h'
            path.write_text('not the original calibration', encoding='utf-8')
            with self.assertRaisesRegex(ValueError, 'changed'):
                read_base(path)

    def test_original_header_accepts_only_known_lf_and_crlf_forms(self):
        original = Path(__file__).resolve().parents[1] / 'src/calibration_tables.h'
        raw = original.read_bytes()
        canonical = raw.replace(b'\r\n', b'\n')
        self.assertEqual(hashlib.sha256(canonical).hexdigest(), BASE_CANONICAL_LF_SHA256)
        self.assertEqual(hashlib.sha256(canonical.replace(b'\n', b'\r\n')).hexdigest(), BASE_SHA256)
        expected = read_base(original)
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / 'calibration.h'
            for variant in (canonical, canonical.replace(b'\n', b'\r\n')):
                path.write_bytes(variant)
                self.assertEqual(read_base(path), expected)
                self.assertEqual(path.read_bytes(), variant)
            for variant in (canonical.replace(b'\n', b'\r\n', 1), canonical + b'\n',
                            canonical.replace(b'AeryCalibration', b'AeryCalibratioX', 1)):
                path.write_bytes(variant)
                with self.assertRaisesRegex(ValueError, 'changed'):
                    read_base(path)
        self.assertEqual(original.read_bytes(), raw)

    def test_metrics_and_gate_reject_regression(self):
        rows = [row(i, level=2) for i in range(1, 6)]
        metrics = error_metrics(rows, [2, 2, 2, 3, 4])
        self.assertEqual(metrics['mae'], 0.6)
        self.assertEqual(metrics['error80'], 1)
        with self.assertRaises(ValueError):
            error_metrics(rows, [2])
        cv = dict(baseline_mae=0.6, selected=dict(mae=0.5))
        result = dict(baseline=dict(mae=0.6, error80=1), auxiliary=dict(mae=0.59, error80=1),
                      manual_low_baseline=dict(mae=0.8, error80=1.1),
                      manual_low_auxiliary=dict(mae=0.79, error80=1.1), max_absolute_shift=0.5)
        self.assertTrue(all(ship_gate(cv, result).values()))
        result['auxiliary']['error80'] = 1.01
        self.assertFalse(ship_gate(cv, result)['holdout_error80_nonregressing'])
        result['auxiliary']['mae'] = 0.599
        self.assertFalse(ship_gate(cv, result)['holdout_mae_improves_one_percent'])

    def test_generated_helper_embeds_only_numeric_anchors(self):
        anchors = anchors_for([row(1, x=2)])
        header = render_header(anchors, dict(bandwidth_x=0.35, bandwidth_log_nps=0.8, blend=0.2), (1, 10))
        self.assertIn('AeryAuxiliaryDelta(double composite, double logNps)', header)
        self.assertIn('sumWeight + 1.0', header)
        self.assertNotIn(anchors[0]['md5'], header)
        self.assertNotIn(anchors[0]['group'], header)

    def test_published_reference_fixture_and_original_hashes(self):
        root = Path(__file__).resolve().parents[1]
        report = json.loads((root / 'docs/aery-auxiliary-reference.json').read_text(encoding='utf-8'))
        header = (root / 'src/aery_auxiliary_reference.h').read_text(encoding='utf-8')
        curve = read_base(root / 'src/calibration_tables.h')
        anchors = report['selected_anchors']
        self.assertEqual(report['status'], 'accepted')
        self.assertEqual(report['source_features_sha256'], FEATURES_SHA256)
        self.assertEqual(report['original_calibration_header_sha256'], BASE_SHA256)
        self.assertEqual(report['original_calibration_header_lf_sha256'], BASE_CANONICAL_LF_SHA256)
        self.assertEqual(report['original_curve_points'], 34)
        self.assertEqual((report['training_charts'], report['holdout_charts']), (1346, 328))
        self.assertEqual((report['training_groups'], report['holdout_groups']), (874, 199))
        self.assertEqual(report['overlapping_groups'], 0)
        self.assertEqual(len(anchors), 230)
        self.assertEqual(len({a['md5'] for a in anchors}), report['selected_charts'])
        self.assertEqual(len({a['group'] for a in anchors}), report['selected_groups'])
        self.assertEqual((report['selected_charts'], report['selected_groups']), (230, 200))
        self.assertEqual(hashlib.sha256(json.dumps(anchors, sort_keys=True, separators=(',', ':')).encode()).hexdigest(),
                         report['selected_anchors_sha256'])
        self.assertTrue(all(re.fullmatch('[0-9a-f]{32}', a['md5']) and
                            re.fullmatch('[0-9a-f]{16}', a['group']) for a in anchors))
        self.assertTrue(all(int(a['group'], 16) % 5 != 0 for a in anchors))
        self.assertEqual([record['level'] for record in report['coverage']], list(LEVELS))
        for level in LEVELS:
            selected = [a for a in anchors if a['level'] == level]
            self.assertEqual(len(selected), SPARSE_LEVELS.get(level, 10))
            self.assertEqual(len({a['group'] for a in selected}), len(selected))
            record = next(record for record in report['coverage'] if record['level'] == level)
            self.assertEqual(record['anchors'], len(selected))
            self.assertEqual(record['sparse_exception'], level in SPARSE_LEVELS)
            self.assertGreaterEqual(record['training_groups'], len(selected))
            if level in SPARSE_LEVELS:
                self.assertEqual(record['training_groups'], len(selected))
        for anchor in anchors:
            self.assertAlmostEqual(anchor['residual'], anchor['level'] - baseline(curve, anchor['x']), places=12)
        body = header.split('AeryAuxiliaryAnchors = {{', 1)[1].split('}};', 1)[0]
        triples = [tuple(map(float, group)) for group in re.findall(r'\{([^,]+), ([^,]+), ([^}]+)\}', body)]
        self.assertEqual(triples, [(a['x'], a['log_nps'], a['residual']) for a in anchors])
        self.assertEqual(header, render_header(anchors, report['parameters'], report['composite_range']))
        self.assertTrue(all(report['acceptance'].values()))

    @unittest.skipUnless(shutil.which('g++'), 'C++ parity check requires an installed g++')
    def test_cpp_matches_python_on_training_anchors_and_synthetic_queries(self):
        root = Path(__file__).resolve().parents[1]
        report = json.loads((root / 'docs/aery-auxiliary-reference.json').read_text(encoding='utf-8'))
        anchors, parameters, bounds = report['selected_anchors'], report['parameters'], report['composite_range']
        queries = [(a['x'], a['log_nps']) for a in anchors]
        queries += [(bounds[0] - 0.01 + i / 10, j / 2) for i in range(89) for j in range(11)]
        source = '''#include "aery_auxiliary_reference.h"
#include <iostream>
#include <iomanip>
#include <limits>
int main() {
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    if (pattern::AeryAuxiliaryDelta(nan, 2) != 0 || pattern::AeryAuxiliaryDelta(2, nan) != 0 ||
        pattern::AeryAuxiliaryDelta(inf, 2) != 0 || pattern::AeryAuxiliaryDelta(2, inf) != 0 ||
        pattern::AeryAuxiliaryDelta(2, -1) != 0) return 2;
    double x, y;
    while (std::cin >> x >> y) std::cout << std::setprecision(17) << pattern::AeryAuxiliaryDelta(x, y) << '\\n';
}
'''
        with tempfile.TemporaryDirectory() as temporary:
            folder = Path(temporary)
            cpp, executable = folder / 'parity.cpp', folder / ('parity.exe' if os.name == 'nt' else 'parity')
            cpp.write_text(source, encoding='utf-8')
            # MinGW's command-line path handling can lose non-ASCII workspace names.
            # Compile local basenames while preserving the published header bytes.
            shutil.copyfile(root / 'src/aery_auxiliary_reference.h', folder / 'aery_auxiliary_reference.h')
            command = [shutil.which('g++'), '-std=c++17', '-O2', '-save-temps=obj', cpp.name, '-o', executable.name]
            if os.name == 'nt':
                command += ['-static', '-static-libgcc', '-static-libstdc++']
            compiler_environment = dict(os.environ, TMP='.', TEMP='.', TMPDIR='.')
            compiled = subprocess.run(command, cwd=folder, env=compiler_environment, capture_output=True)
            self.assertEqual(compiled.returncode, 0, compiled.stderr.decode('utf-8', errors='replace'))
            result = subprocess.run([str(executable)], input=''.join(f'{x:.17g} {y:.17g}\n' for x, y in queries),
                                    text=True, check=True, capture_output=True)
            values = list(map(float, result.stdout.splitlines()))
            self.assertEqual(len(values), len(queries))
            for actual, (x, y) in zip(values, queries):
                self.assertAlmostEqual(actual, delta(anchors, x, y, parameters, bounds), places=12)
                self.assertLessEqual(abs(actual), MAX_SHIFT)


if __name__ == '__main__':
    unittest.main()
