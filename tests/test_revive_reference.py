"""Reject incomplete/mislabeled calibration inputs before generating a reference."""
import copy
import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from build_revive_reference import measured_courses
from prepare_revive_reference import course_id, load_courses


class ReviveMeasurementTests(unittest.TestCase):
    def test_missing_duplicate_invalid_and_misidentified_features(self):
        doc = load_courses(Path(__file__).resolve().parents[1] / 'docs/revive10-courses.json')
        metadata = [dict(c, keys=10, sha256=course_id(c)) for c in doc['courses']]
        lines = [f"{course_id(c)}\t10\t{c['rank']}\tRevive2026\t{c['rank']+2}\t10\t5" for c in doc['courses']]
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / 'courses.json').write_text(json.dumps(metadata), encoding='utf-8')
            path = root / 'features.tsv'
            path.write_text('\n'.join(lines), encoding='utf-8')
            self.assertEqual([r['rank'] for r in measured_courses(root, doc)], list(range(1, 17)))
            for broken in (lines[:-1], lines + [lines[0]],
                           [lines[0].replace('\t10\t1\t', '\t10\t2\t')] + lines[1:],
                           [lines[0].replace('\t3\t10\t5', '\tnan\t10\t5')] + lines[1:],
                           [lines[0].replace('\t3\t10\t5', '\t-1\t10\t5')] + lines[1:]):
                path.write_text('\n'.join(broken), encoding='utf-8')
                with self.assertRaises(ValueError):
                    measured_courses(root, doc)
            path.write_text('\n'.join(lines), encoding='utf-8')
            wrong = copy.deepcopy(metadata)
            wrong[0]['stages'].reverse()
            (root / 'courses.json').write_text(json.dumps(wrong), encoding='utf-8')
            with self.assertRaisesRegex(ValueError, 'stage identity'):
                measured_courses(root, doc)


if __name__ == '__main__':
    unittest.main()
