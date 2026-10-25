"""Extract only selected Dan course notes into a private, ignored build folder.

No audio, player scores, replays or runtime configuration are read or modified.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re


EXTRAS_4 = {'ALPHA': 11, 'BETA': 12, 'GAMMA': 13, 'DELTA': 14, 'EPSILON': 15, 'ZETA': 16}
EXTRAS_7 = {'Gamma': 11, 'Azimuth': 12, 'Zenith': 13, 'Stellium': 14}
EXTRAS_8 = {'Flare': 11, 'Aqua': 12, 'Life': 13, 'Oblatus': 14, 'Demise': 15}
REQUIRED_RANKS = {4: range(-2, 17), 7: range(15), 8: range(16), 10: range(1, 13)}


def parse_chart(raw):
    text = raw.decode('utf-8-sig')
    if '[HitObjects]' not in text:
        raise ValueError('Missing hit objects')
    header, body = text.split('[HitObjects]', 1)
    info = dict(re.findall(r'^([A-Za-z]+)\s*:\s*(.*?)\s*$', header, re.M))
    if info.get('Mode') != '3':
        raise ValueError('Not native mania')
    keys = int(float(info['CircleSize']))
    if keys not in REQUIRED_RANKS or float(info['CircleSize']) != keys:
        raise ValueError('Unexpected key mode')
    notes = []
    for line in body.splitlines():
        if not line.strip() or line.lstrip().startswith('//'):
            continue
        fields = line.split(',')
        x, start, flags = int(fields[0]), int(fields[2]), int(fields[3])
        if flags & 128:
            end = int(fields[5].split(':')[0])
        elif flags & 1:
            end = start
        else:
            raise ValueError('Unsupported hit object')
        if not 0 <= x <= 512 or start < 0 or end < start:
            raise ValueError('Invalid note')
        notes.append((min(keys - 1, x * keys // 512), start, end))
    notes.sort(key=lambda n: (n[1], n[0], n[2]))
    if not notes:
        raise ValueError('Empty course')
    return info, keys, notes


def course_rank(info, keys):
    title, creator, version = (info.get(k, '') for k in ('Title', 'Creator', 'Version'))
    if keys == 4 and creator == 'DDMythical':
        # Keep one coherent v1 ladder. Pre-v1, v2, Sample, skill/practice packs
        # and republished edits have different labels/scales and are excluded.
        if title not in ('Dan ~ REFORM ~ 1st Pack', 'Dan ~ REFORM ~ 2nd Pack', 'Dan ~ REFORM ~ 3rd Pack'):
            return None
        match = re.fullmatch(r'~ (INTRO-(1st|2nd|3rd)|([1-9][0-9]*)(st|nd|rd|th)|EXTRA-([A-Z]+)) ~ \(Marathon\)', version)
        if not match:
            return None
        rank = int(match[2][0]) - 3 if match[2] else int(match[3]) if match[3] else EXTRAS_4.get(match[5])
        if rank is None:
            return None
        if '1st Pack' in title and -2 <= rank <= 5 or '2nd Pack' in title and 6 <= rank <= 15 or '3rd Pack' in title and rank == 16:
            return rank
    if keys == 7 and creator == 'Jinjin':
        if title not in ('Regular Dan Phase I', 'Regular Dan Phase II', 'Regular Dan Phase III', 'Dan Phase IV'):
            return None
        if title == 'Dan Phase IV':
            return 14 if version == 'Stellium Dan (Regular)' else None
        match = re.fullmatch(r'(\d+)(st|nd|rd|th) Dan(?: \(Marathon\))?', version)
        if match and 0 <= int(match[1]) <= 10:
            return int(match[1])
        match = re.fullmatch(r'(Gamma|Azimuth|Zenith) Dan', version)
        if match:
            return EXTRAS_7[match[1]]
    if keys == 8 and creator == 'HeavenUsurper':
        packs = {'Easy': range(4), 'Medium': range(4, 8), 'Hard': range(8, 11),
                 'Insane': range(11, 14), 'Extreme': range(14, 16)}
        match = re.fullmatch(r'osu!mania 8k Regular Dan Course v1 \((Easy|Medium|Hard|Insane|Extreme) Pack\)', title)
        version_match = re.fullmatch(r'Regular-(\d+(?:st|nd|rd|th)|Flare|Aqua|Life|Oblatus|Demise)', version)
        if match and version_match:
            label = version_match[1]
            rank = int(re.match(r'\d+', label)[0]) if label[0].isdigit() else EXTRAS_8[label]
            if rank in packs[match[1]]:
                return rank
    if keys == 10 and creator == 'Tryr':
        phases = {'10K BMS Dans Phase I': range(1, 5), '10K BMS Dans Phase II': range(5, 9),
                  '10K BMS Dans Phase III': range(9, 13)}
        match = re.fullmatch(r'Dan ([1-9]|10)', version)
        # 11/12 are internal ordering indices, not numeric course names.
        rank = int(match[1]) if match else {'Kaiden': 11, 'Catastrophe': 12}.get(version)
        if rank in phases.get(title, ()):
            return rank
    return None


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--root', action='append', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--keys', type=int, nargs='+', choices=sorted(REQUIRED_RANKS), default=sorted(REQUIRED_RANKS))
    args = parser.parse_args()
    courses = {}
    for root in args.root:
        for path in sorted(root.rglob('*.osu')):
            # Limit reads in large song libraries to plausible course filenames.
            if 'KeyWeaver' in path.name or not re.search(r'REFORM|Regular Dan Phase|Dan Phase IV|8k Regular Dan Course v1|10K BMS Dans|^[0-3](st|nd|rd|th)\.osu$', path.name):
                continue
            raw = path.read_bytes()
            try:
                info, keys, notes = parse_chart(raw)
            except (ValueError, KeyError, IndexError, UnicodeError):
                continue
            rank = course_rank(info, keys)
            if rank is None or keys not in args.keys:
                continue
            events = ''.join(f'{lane} {start} {end}\n' for lane, start, end in notes)
            event_hash = hashlib.sha256(events.encode()).hexdigest()
            key = (keys, rank)
            if key in courses:
                if courses[key]['events_sha256'] != event_hash:
                    raise ValueError(f'Conflicting course revisions for {keys}K rank {rank}; select one explicit source')
                continue
            courses[key] = dict(keys=keys, rank=rank, sha256=hashlib.sha256(raw).hexdigest(), events_sha256=event_hash,
                                creator=info['Creator'], title=info['Title'], version=info['Version'],
                                notes=len(notes), events=events)
    for keys in args.keys:
        ranks = REQUIRED_RANKS[keys]
        missing = [r for r in ranks if (keys,r) not in courses]
        if missing:
            raise ValueError(f'Missing required {keys}K reference ranks: {missing}')
    (args.output / 'notes').mkdir(parents=True, exist_ok=True)
    manifest, metadata = [], []
    for key in sorted(courses):
        row = courses[key]
        ident = row['sha256']
        (args.output / 'notes' / f'{ident}.txt').write_text(row.pop('events'), encoding='ascii')
        manifest.append(f"{ident}\t{row['keys']}\t{row['rank']}\t{ident[:16]}\n")
        metadata.append(row)
    (args.output / 'decoded.tsv').write_text(''.join(manifest), encoding='ascii')
    (args.output / 'courses.json').write_text(json.dumps(metadata, indent=2, ensure_ascii=False), encoding='utf-8')
    print(json.dumps({'courses':len(courses),'by_keys':{str(k):sum(key[0]==k for key in courses) for k in args.keys}}))


if __name__ == '__main__':
    main()
