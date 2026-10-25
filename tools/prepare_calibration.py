"""Match local chart bytes to cached BMS tables without modifying source data.

Writes a private build-time manifest; never ship it or the extracted chart events.
"""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import re


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--tables', type=Path, required=True)
    ap.add_argument('--root', type=Path, action='append', required=True)
    ap.add_argument('--output', type=Path, required=True)
    args = ap.parse_args()
    labels = {}
    for table in args.tables.glob('*.bmt'):
        data = json.loads(gzip.decompress(table.read_bytes()))
        name = data.get('name')
        if name not in ('5KEYS AERY', '発狂BMS難度推定表 NORMAL'):
            continue
        for folder in data['folder']:
            label = folder['name']
            if name == '5KEYS AERY':
                match = re.fullmatch(r'⑤LEVEL (\d+)(\+?)', label)
                if not match or int(match[1]) > 20:
                    continue
                level, mode = int(match[1]) + (0.5 if match[2] else 0), 5
            else:
                match = re.fullmatch(r'N★(\d+\.\d+)\.\.\.(\d+\.\d+)', label)
                if not match:
                    continue  # Do not invent targets for open-ended buckets.
                level, mode = (float(match[1]) + float(match[2])) / 2, 8
            for song in folder.get('songs', []):
                if song.get('md5'):
                    labels[song['md5']] = (mode, level)
    matches, seen, scanned, skipped_random = [], set(), 0, 0
    for root in args.root:
        for path in root.rglob('*'):
            if path.suffix.lower() not in ('.bms', '.bme', '.bml') or not path.is_file():
                continue
            raw = path.read_bytes()
            scanned += 1
            md5 = hashlib.md5(raw).hexdigest()
            if md5 in seen or md5 not in labels:
                continue
            seen.add(md5)
            if re.search(rb'^\s*#(?:RANDOM|SETRANDOM)\b', raw, re.I | re.M):
                skipped_random += 1
                continue
            mode, level = labels[md5]
            # Variants in the same song directory stay in the same validation split.
            group = hashlib.sha256(path.parent.name.casefold().encode()).hexdigest()[:16]
            matches.append((md5, mode, level, group, str(path.resolve())))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(''.join('\t'.join(map(str, row))+'\n' for row in sorted(matches)), encoding='utf-8')
    print(json.dumps({'scanned': scanned, 'matched': len(matches), 'random_skipped': skipped_random,
                      'profiles': {str(k): sum(r[1] == k for r in matches) for k in (5, 8)}}))


if __name__ == '__main__':
    main()
