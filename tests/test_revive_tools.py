"""Reference integrity checks with synthetic files; original packs are not used."""
import copy
import hashlib
import json
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from prepare_revive_reference import (SOURCE_NAME, assemble, course_id,
                                      join_stages, load_courses, read_notes,
                                      resolve_charts)


def stage(order, raw):
    return dict(order=order, sha256=hashlib.sha256(raw).hexdigest(),
                md5=hashlib.md5(raw).hexdigest())


def document():
    return dict(name=SOURCE_NAME, courses=[dict(rank=r, label=str(r), stages=[
        stage(i, f'{r},{i}'.encode()) for i in range(1, 5)]) for r in range(1, 17)])


class ReviveReferenceTests(unittest.TestCase):
    def test_complete_ordered_course_manifest_required(self):
        data = document()
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / 'courses.json'
            path.write_text(json.dumps(data), encoding='utf-8')
            self.assertEqual(len(load_courses(path)['courses']), 16)
            for mutate in (lambda d: d['courses'].pop(),
                           lambda d: d['courses'][0].update(rank=2),
                           lambda d: d['courses'][0]['stages'].reverse(),
                           lambda d: d['courses'][0]['stages'][0].update(sha256='../escape')):
                broken = copy.deepcopy(data)
                mutate(broken)
                path.write_text(json.dumps(broken), encoding='utf-8')
                with self.assertRaises(ValueError):
                    load_courses(path)

    def test_exact_bytes_required_and_identical_duplicates_allowed(self):
        raw = b'#TITLE source\n#BPM 120\n#00111:01\n'
        one = stage(1, raw)
        courses = [dict(stages=[one])]
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / 'first.bms').write_bytes(raw)
            (root / 'duplicate.bme').write_bytes(raw)
            (root / 'unrelated.bms').write_bytes(raw + b'changed')
            self.assertEqual(set(resolve_charts(root, courses)), {one['sha256']})
            wrong_md5 = copy.deepcopy(courses)
            wrong_md5[0]['stages'][0]['md5'] = '0' * 32
            with self.assertRaises(ValueError):
                resolve_charts(root, wrong_md5)
            (root / 'first.bms').write_bytes(raw + b'changed')
            (root / 'duplicate.bme').write_bytes(raw + b'changed')
            with self.assertRaisesRegex(ValueError, 'missing'):
                resolve_charts(root, courses)

    def test_timing_and_long_notes_are_preserved_with_isolated_stages(self):
        first = [(0, 100, 100), (9, 200, 900)]
        second = [(1, 300, 900), (2, 400, 400)]
        self.assertEqual(join_stages([first, second]),
                         [(0, 0, 0), (9, 100, 800), (1, 10800, 11400), (2, 10900, 10900)])
        with self.assertRaises(ValueError):
            join_stages([first, second], 7999)

    def test_decoded_event_validation(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / 'notes.txt'
            for raw in ('', '10 0 0\n', '0 -1 0\n', '0 20 19\n', '0 2\n'):
                path.write_text(raw, encoding='utf-8')
                with self.assertRaises(ValueError):
                    read_notes(path)
            path.write_text('9 200 300\n0 100 100\n', encoding='utf-8')
            self.assertEqual(read_notes(path), [(0, 100, 100), (9, 200, 300)])

    def test_identity_includes_order_and_course_rank(self):
        course = document()['courses'][0]
        swapped = copy.deepcopy(course)
        swapped['stages'].reverse()
        self.assertNotEqual(course_id(course), course_id(swapped))
        self.assertNotEqual(course_id(course), course_id(dict(course, rank=2)))

    def test_assembly_emits_sixteen_courses_and_sixty_four_stage_audits(self):
        data = document()
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / 'stages').mkdir()
            for course in data['courses']:
                for item in course['stages']:
                    (root / 'stages' / (item['sha256'] + '.txt')).write_text('0 0 100\n9 1000 1000\n')
            self.assertEqual(assemble(data, root), 16)
            self.assertEqual(len((root / 'decoded.tsv').read_text().splitlines()), 16)
            self.assertEqual(len((root / 'stage-analysis' / 'decoded.tsv').read_text().splitlines()), 64)
            metadata = json.loads((root / 'courses.json').read_text())
            self.assertEqual(metadata[0]['notes'], 8)
            self.assertEqual(metadata[0]['long_notes'], 4)
            first = read_notes(root / 'notes' / (metadata[0]['sha256'] + '.txt'))
            self.assertEqual(first[-1], (9, 34000, 34000))


@unittest.skipUnless(os.environ.get('REVIVE_DECODER_JAR') and shutil.which('java') and shutil.which('javac'),
                     'Set REVIVE_DECODER_JAR to an existing beatoraja decoder for Java integration checks')
class ReviveDecoderTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        cls.root = Path(cls.temp.name)
        cls.jar = str(Path(os.environ['REVIVE_DECODER_JAR']).resolve())
        source = Path(__file__).resolve().parents[1] / 'tools' / 'ExtractRevivePatterns.java'
        subprocess.run(['javac', '-encoding', 'UTF-8', '-classpath', cls.jar,
                        '-d', str(cls.root), str(source)], check=True, capture_output=True)

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def decode(self, raw, name):
        chart = self.root / (name + '.bms')
        chart.write_bytes(raw)
        ident = hashlib.sha256(raw).hexdigest()
        manifest = self.root / (name + '.tsv')
        manifest.write_text(f'{ident}\t{chart}\n', encoding='utf-8')
        output = self.root / name
        result = subprocess.run(['java', '-cp', os.pathsep.join((str(self.root), self.jar)),
                                 'ExtractRevivePatterns', str(manifest), str(output)], capture_output=True)
        return result, output, ident

    def test_decoder_preserves_ten_lanes_lnobj_ln_channels_bpm_and_stop(self):
        raw = (b'#PLAYER 3\n#TITLE synthetic\n#TOTAL 160\n#BPM 120\n#BPM02 240\n#STOP01 192\n'
               b'#LNTYPE 1\n#LNMODE 2\n#LNOBJ ZZ\n#WAV01 a.wav\n'
               b'#00011:01\n#00021:01\n#00025:0001\n'
               b'#00151:0101\n#00165:0101\n#00212:01ZZ\n'
               b'#00308:02\n#00313:0001\n#00409:01\n#00414:0001\n')
        result, output, ident = self.decode(raw, 'timing')
        self.assertEqual(result.returncode, 0, result.stderr.decode(errors='replace'))
        self.assertEqual(read_notes(output / 'stages' / (ident + '.txt')),
                         [(0, 0, 0), (5, 0, 0), (9, 1000, 1000),
                          (0, 2000, 3000), (9, 2000, 3000), (1, 4000, 5000),
                          (2, 6500, 6500), (3, 8500, 8500)])

    def test_decoder_rejects_scratch_hcn_and_conditional_sources(self):
        base = b'#PLAYER 3\n#TITLE synthetic\n#TOTAL 160\n#BPM 120\n#00011:01\n#00021:01\n'
        for suffix, addition, reason in (('scratch', b'#00016:01\n', 'Playable scratch'),
                                         ('hcn', b'#LNMODE 3\n#00151:0101\n', 'Unsupported or unpaired hold'),
                                         ('missing_hold_end', b'#00151:01\n', 'BMS decoding error'),
                                         ('random', b'#RANDOM 2\n#IF 1\n#00211:01\n#ENDIF\n', 'Conditional chart')):
            result, output, _ = self.decode(base + addition, suffix)
            self.assertNotEqual(result.returncode, 0, suffix)
            self.assertIn(reason, result.stderr.decode(errors='replace'))
            self.assertFalse((output / 'stages').exists())


if __name__ == '__main__':
    unittest.main()
