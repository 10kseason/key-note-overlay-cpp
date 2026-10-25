"""Offline reference ingestion checks; uses synthetic metadata, never game files."""
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from prepare_dan_reference import parse_chart, course_rank
from build_dan_reference import bands


class DanReferenceTests(unittest.TestCase):
    def test_taps_holds_and_last_lane(self):
        raw = b'osu file format v14\nMode:3\nCircleSize:4\n[HitObjects]\n512,192,200,128,0,600:0:0:0:0:\n0,192,100,1,0,0:0:0:0:'
        _, keys, notes = parse_chart(raw)
        self.assertEqual(keys, 4)
        self.assertEqual(notes, [(0, 100, 100), (3, 200, 600)])
        for invalid in (raw.replace(b'Mode:3', b'Mode:0'), raw.replace(b'CircleSize:4', b'CircleSize:4.5'),
                        raw.replace(b'600:', b'199:'), raw.replace(b'512,', b'513,')):
            with self.assertRaises(ValueError):
                parse_chart(invalid)

    def test_tryr_10k_bms_phase_boundaries(self):
        for phase, versions in [('I', [('Dan 1',1), ('Dan 4',4)]), ('II', [('Dan 5',5), ('Dan 8',8)]),
                                ('III', [('Dan 9',9), ('Dan 10',10), ('Kaiden',11), ('Catastrophe',12)])]:
            for version, rank in versions:
                info = dict(Creator='Tryr', Title=f'10K BMS Dans Phase {phase}', Version=version)
                self.assertEqual(course_rank(info, 10), rank)
                self.assertIsNone(course_rank(info, 7))
        info = dict(Creator='Tryr', Title='10K BMS Dans Phase III', Version='Dan 9')
        for change in (dict(Title='10KCT'), dict(Title='10K Dans'), dict(Version='Dan 0'), dict(Version='Dan ξ'),
                       dict(Version='Dan 11'), dict(Version='Dan 9 [1.1x]'), dict(Creator='KeyWeaver'),
                       dict(Title='10K BMS Dans Phase I'), dict(Title='10K BMS Dans Phase II')):
            self.assertIsNone(course_rank(dict(info, **change), 10))
        self.assertIsNone(course_rank(dict(Creator='semyon422', Title='10K Dans', Version='Dan ξ'), 10))

    def test_reform_v1_pack_boundaries(self):
        info = dict(Creator='DDMythical', Title='Dan ~ REFORM ~ 1st Pack', Version='~ INTRO-1st ~ (Marathon)')
        self.assertEqual(course_rank(info, 4), -2)
        self.assertEqual(course_rank(dict(info, Version='~ 5th ~ (Marathon)'), 4), 5)
        self.assertIsNone(course_rank(dict(info, Version='~ 6th ~ (Marathon)'), 4))
        self.assertEqual(course_rank(dict(info, Title='Dan ~ REFORM ~ 2nd Pack', Version='~ 6th ~ (Marathon)'), 4), 6)
        for title in ('Dan ~ REFORM ~ Sample', 'Dan ~ REFORM ~ Pre-v1', 'Dan ~ REFORM ~ 1st Pack v2'):
            self.assertIsNone(course_rank(dict(info, Title=title), 4))

    def test_jinjin_regular_not_ln(self):
        info = dict(Creator='Jinjin', Title='Dan Phase IV', Version='Stellium Dan (Regular)')
        self.assertEqual(course_rank(info, 7), 14)
        self.assertIsNone(course_rank(dict(info, Version='Stellium Dan (LN)'), 7))
        self.assertEqual(course_rank(dict(info, Title='Regular Dan Phase I', Version='0th Dan (Marathon)'), 7), 0)

    def test_pooling_preserves_rank_ambiguity(self):
        rows = [dict(rank=i, strain=s) for i, s in enumerate([1, 5, 4, 2, 6])]
        self.assertEqual(bands(rows), [dict(strain=1,low=0,high=0), dict(strain=11/3,low=1,high=3), dict(strain=6,low=4,high=4)])
        self.assertEqual(bands([dict(rank=0,strain=2), dict(rank=1,strain=2)]), [dict(strain=2,low=0,high=1)])

    def test_8k_regular_v1_packs_and_named_ranks(self):
        for pack, labels in [('Easy', [('0th',0), ('3rd',3)]), ('Medium', [('4th',4), ('7th',7)]),
                             ('Hard', [('8th',8), ('10th',10)]), ('Insane', [('Flare',11), ('Aqua',12), ('Life',13)]),
                             ('Extreme', [('Oblatus',14), ('Demise',15)])]:
            for label, rank in labels:
                info = dict(Creator='HeavenUsurper', Title=f'osu!mania 8k Regular Dan Course v1 ({pack} Pack)', Version=f'Regular-{label}')
                self.assertEqual(course_rank(info, 8), rank)
                self.assertIsNone(course_rank(info, 7))
                self.assertIsNone(course_rank(dict(info, Creator='KeyWeaver'), 8))
                self.assertIsNone(course_rank(dict(info, Version=f'Regular-{label} [1.1x]'), 8))
                self.assertIsNone(course_rank(dict(info, Title=info['Title'].replace('Regular', 'LN')), 8))
        self.assertIsNone(course_rank(dict(Creator='HeavenUsurper', Title='osu!mania 8k Regular Dan Course v1 (Easy Pack)', Version='Regular-10th'), 8))
        _, keys, notes = parse_chart(b'Mode:3\nCircleSize:8\n[HitObjects]\n512,192,100,1,0,0:0:0:0:')
        self.assertEqual((keys, notes), (8, [(7, 100, 100)]))


if __name__ == '__main__':
    unittest.main()
