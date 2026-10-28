"""Generate synthetic 4K parity fixtures using the user's original calculator.

Only the output directory is written. Runtime/native tests need no Python/YAML.
"""
import argparse
import contextlib
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import random
import sys
import yaml

sys.dont_write_bytecode = True
AXES = ('STREAM', 'SPEED', 'JACK', 'CHORD', 'TECH', 'HOLD')


def cases():
    def note(t, lane, kind='note'):
        return dict(time=t, column=lane + 1, type=kind)
    data = {}
    for nps in (5, 10, 20):
        data[f'uniform_{nps}'] = [note(i / nps, i % 4) for i in range(8 * nps)]
    for nps in (10, 20):
        data[f'jack_{nps}'] = [note(i / nps, 1) for i in range(8 * nps)]
    for width in (2, 4):
        data[f'chord_{width}'] = [note(i * .2, (i + lane) % 4) for i in range(40) for lane in range(width)]
    data['spider_150ms'] = [note(i * .15, i * 3 % 4) for i in range(53)]
    data['mixed_rhythm'] = [note(i * .1 + (0 if i % 4 < 2 else .025), i * 3 % 4) for i in range(80)]
    data['ln_overlap'] = [note(i * .5, lane, 'ln_start') for i in range(16) for lane in (0, 2)]
    data['ln_overlap'] += [note(i * .5 + .375, lane, 'ln_end') for i in range(16) for lane in (0, 2)]
    data['ln_overlap'] += [note(i / 10, 1 + (i % 2) * 2) for i in range(80)]
    data['ln_staggered'] = [note(i * .65 + lane * .02, lane, 'ln_start') for i in range(12) for lane in range(4)]
    data['ln_staggered'] += [note(i * .65 + lane * .02 + .32 + (lane % 3) * .04, lane, 'ln_end') for i in range(12) for lane in range(4)]
    data['ln_occupancy'] = [note(0, lane, 'ln_start') for lane in range(4)] + [note(8, lane, 'ln_end') for lane in range(4)]
    for seed in range(3):
        rng = random.Random(1400 + seed)
        notes = []
        for lane in range(4):
            t = rng.uniform(.01, .15)
            while t < 7.5:
                if rng.random() < .35:
                    end = min(7.9, t + rng.uniform(.1, .55))
                    notes += [note(t, lane, 'ln_start'), note(end, lane, 'ln_end')]
                    t = end + rng.uniform(.015, .12)
                else:
                    notes.append(note(t, lane)); t += rng.uniform(.04, .24)
        data[f'random_mixed_{seed}'] = notes
    data['boundary_timing'] = [note(i * .125, i % 3) for i in range(64)]
    data['single'] = [note(.2, 0)]
    data['empty'] = []
    return data


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    protected = [args.source / 'new_calc.py', args.source / 'config.yaml']
    hashes = {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in protected}
    config = yaml.safe_load(protected[1].read_text(encoding='utf-8'))
    spec = importlib.util.spec_from_file_location('original_revive4', protected[0])
    original = importlib.util.module_from_spec(spec); spec.loader.exec_module(original)
    args.output.mkdir(parents=True, exist_ok=False)
    kind = {'note': 0, 'ln_start': 1, 'ln_end': 2}
    for name, notes in cases().items():
        with contextlib.redirect_stdout(io.StringIO()):
            result = original.calculate_total_difficulty(notes, 8, key_mode=4,
                preset_name='qwilight_bms_ez', config=config)
        values = [8, len(notes), result['revive_difficulty'], result['revive_lv'], result['circus_rating']]
        values += [result['skill_radar'][axis] for axis in AXES]
        text = ' '.join(format(v, '.17g') for v in values) + '\n'
        text += ''.join(f"{n['time']:.17g} {n['column']-1} {kind[n['type']]}\n" for n in notes)
        (args.output / (name + '.tsv')).write_text(text, encoding='ascii')
        print(f'{name}: {len(notes)} events', flush=True)
    assert hashes == {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in protected}
    (args.output / 'provenance.json').write_text(json.dumps(dict(
        source_hashes=hashes, cases=len(cases()), keys=4, preset='qwilight_bms_ez',
        weights=config['weighted_nps']['weights_4k'],
        visual=config['distance_difficulty']['visual_distance_matrices']['weights_4k'],
        scope='synthetic parity; fixed placement; auto target; no scratch; no player skill validation'),
        indent=2, ensure_ascii=False) + '\n', encoding='utf-8')


if __name__ == '__main__':
    main()
