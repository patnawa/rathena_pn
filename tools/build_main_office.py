#!/usr/bin/env python3
"""Build PN office aliases from the owner's extracted, unmodified client maps."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zlib

MAPS = {'pn_office': 'iz_ac01', 'pn_train': 'guild_vs1', 'pn_style': 'iz_ac02'}

def cache_records(path):
    data = Path(path).read_bytes()
    count = struct.unpack_from('<H', data, 4)[0]
    offset, records = 8, {}
    for _ in range(count):
        name, w, h, size = struct.unpack_from('<12shhi', data, offset)
        end = offset + 20 + size
        assert end <= len(data) and w > 0 and h > 0
        raw = data[offset:end]
        assert len(zlib.decompress(raw[20:])) == w * h
        records[name.split(b'\0')[0].decode()] = raw
        offset = end
    assert offset == len(data)
    return records

def write_cache(path, records):
    body = b''.join(records.values())
    Path(path).write_bytes(struct.pack('<IHH', 8 + len(body), len(records), 0) + body)

def make_grf(entries):
    body, table = bytearray(), bytearray()
    for name, raw in sorted(entries.items()):
        assert name.startswith('data\\pn_') and '..' not in name
        packed = zlib.compress(raw, 9)
        table += name.encode('ascii') + b'\0'
        table += struct.pack('<IIIBI', len(packed), len(packed), len(raw), 1, len(body))
        body += packed
    packed = zlib.compress(table, 9)
    return (b'Master of Magic\0' + bytes(14) + struct.pack('<IIII', len(body), 0, len(entries)+7, 0x200)
            + body + struct.pack('<II', len(packed), len(table)) + packed)

def build(root, assets, output):
    output.mkdir(parents=True, exist_ok=False)
    effective = {}
    for rel in ['db/import/map_cache.dat', 'db/re/map_cache.dat', 'db/map_cache.dat']:
        if (root/rel).exists():
            for name, raw in cache_records(root/rel).items():
                effective.setdefault(name, raw)
    aliases, entries, manifest = {}, {}, {'maps': {}, 'files': {}}
    for alias, source in MAPS.items():
        record = effective[source]
        w, h = struct.unpack_from('<hh', record, 12)
        cells = zlib.decompress(record[20:])
        gat = (assets/(source+'.gat')).read_bytes()
        assert gat[:6] == b'GRAT\x01\x02'
        assert struct.unpack_from('<II', gat, 6) == (w, h)
        client_cells = bytes(struct.unpack_from('<I', gat, 14+20*i+16)[0] for i in range(w*h))
        assert all((a in (0,3)) == (b in (0,3)) for a,b in zip(cells,client_cells)), source+' walkability drift'
        aliases[alias] = alias.encode().ljust(12,b'\0')+record[12:]
        for ext in ['gat','gnd','rsw']:
            raw = (assets/(source+'.'+ext)).read_bytes()
            original_hash = hashlib.sha256(raw).hexdigest()
            if ext == 'rsw':
                assert raw[:6] == b'GRSW\x02\x01', 'Only reviewed RSW 2.1 is supported'
                data=bytearray(raw)
                for offset, suffix in [(46,'gnd'),(86,'gat')]:
                    assert raw[offset:offset+40].split(b'\0')[0] == (source+'.'+suffix).encode()
                    data[offset:offset+40]=(alias+'.'+suffix).encode().ljust(40,b'\0')
                raw=bytes(data)
            name='data\\'+alias+'.'+ext
            entries[name]=raw
            manifest['files'][name]={'source_sha256': original_hash, 'sha256':hashlib.sha256(raw).hexdigest()}
        manifest['maps'][alias]={'source':source,'width':w,'height':h}
    write_cache(output/'office-map-cache.dat',aliases)
    grf=make_grf(entries)
    (output/'pn_office.grf').write_bytes(grf)
    manifest['grf_sha256']=hashlib.sha256(grf).hexdigest()
    (output/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print(json.dumps(manifest['maps']))

def merge(cache, aliases):
    records=cache_records(cache)
    for name, raw in cache_records(aliases).items():
        assert name in MAPS, 'Unexpected alias'
        assert name not in records or records[name] == raw, 'Existing alias differs: '+name
        records[name]=raw
    write_cache(cache,records)

if __name__ == '__main__':
    p=argparse.ArgumentParser(description=__doc__)
    sub=p.add_subparsers(dest='mode',required=True)
    b=sub.add_parser('build'); b.add_argument('root',type=Path);b.add_argument('assets',type=Path);b.add_argument('output',type=Path)
    m=sub.add_parser('merge-cache');m.add_argument('cache',type=Path);m.add_argument('aliases',type=Path)
    a=p.parse_args()
    if a.mode=='build':build(a.root,a.assets,a.output)
    else:merge(a.cache,a.aliases)
