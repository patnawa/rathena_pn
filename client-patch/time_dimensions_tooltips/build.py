"""Build and verify a separate Time Dimensions tooltip candidate; never install it."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess

PACKAGE = Path(__file__).resolve().parent
MODULE = 'SystemEN/itemInfo_TimeDimensions.lua'
HOOK = 'dofile("SystemEN/itemInfo_TimeDimensions.lua")'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def lua_string(value):
    return json.dumps(value, ensure_ascii=True)


def module_source(repairs):
    lines = [
        '-- Reviewed Time Dimensions autocast descriptions, 2026-09-22.',
        '-- Facts and Gravity source links: corrections.json and',
        '-- doc/dimension_other_jobs_audit_20260922.md.',
        '-- Loaded after existing item merges; resources and other fields stay intact.',
        'local repairs = {',
    ]
    for item in repairs:
        lines.append('  [%d] = {' % item['id'])
        for edit in item['edits']:
            lines.append('    {%s, %s},' % tuple(lua_string(edit[key]) for key in ('before', 'after')))
        lines.append('  },')
    lines.extend([
        '}',
        'for id, edits in pairs(repairs) do',
        '  local item = tbl[id]',
        '  if item and item.identifiedDescriptionName then',
        '    for index, line in ipairs(item.identifiedDescriptionName) do',
        '      for _, edit in ipairs(edits) do',
        '        if line == edit[1] then',
        '          item.identifiedDescriptionName[index] = edit[2]',
        '          break',
        '        end',
        '      end',
        '    end',
        '  end',
        'end',
        '',
    ])
    return '\n'.join(lines)


def verify(client, candidate, lua, repairs, proof_dir=None):
    expected = {str(item['id']): item['edits'] for item in repairs}
    code = [
        'dofile(%s)' % lua_string((client / 'SystemEN/itemInfo.lua').as_posix()),
        'local expected = {}',
    ]
    for item_id, edits in expected.items():
        code.append('expected[%s] = {}' % item_id)
        for edit in edits:
            code.append('expected[%s][%s] = %s' % (item_id, lua_string(edit['before']), lua_string(edit['after'])))
    code.extend([
        'local function clone(value)',
        '  if type(value) ~= "table" then return value end',
        '  local copy = {}; for k, v in pairs(value) do copy[k] = clone(v) end; return copy',
        'end',
        'local function equal(a, b)',
        '  if type(a) ~= type(b) then return false end',
        '  if type(a) ~= "table" then return a == b end',
        '  for k, v in pairs(a) do if not equal(v, b[k]) then return false end end',
        '  for k in pairs(b) do if a[k] == nil then return false end end',
        '  return true',
        'end',
        'local before = clone(tbl)',
        'local changes = 0; local count = 0',
        'for id, edits in pairs(expected) do',
        '  assert(before[id], "Missing base item " .. id)',
        '  local found = {}',
        '  for i, line in ipairs(before[id].identifiedDescriptionName) do',
        '    if edits[line] then',
        '      before[id].identifiedDescriptionName[i] = edits[line]; found[line] = true; changes = changes + 1',
        '    end',
        '  end',
        '  for line in pairs(edits) do assert(found[line], "Tooltip source drift for " .. id .. ": " .. line) end',
        '  count = count + 1',
        'end',
        'dofile("SystemEN/itemInfo.lua")',
        'assert(equal(before, tbl), "Candidate differs beyond reviewed tooltip lines")',
        'dofile("SystemEN/itemInfo_TimeDimensions.lua")',
        'assert(equal(before, tbl), "Tooltip override is not idempotent")',
        'print("PASS: " .. count .. " weapon tooltips, " .. changes .. " corrected lines; full loader and unchanged item metadata verified")',
    ])
    proof = (proof_dir or candidate) / 'verify_tooltips.lua'
    proof.write_text('\n'.join(code) + '\n', encoding='ascii')
    result = subprocess.run([str(lua), str(proof)], cwd=candidate, capture_output=True, text=True, check=True)
    print(result.stdout.strip())
    return result.stdout.strip()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--client', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--lua', type=Path, required=True)
    args = parser.parse_args()
    client, output, lua = (path.resolve() for path in (args.client, args.output, args.lua))
    if output.exists() or client == output or client in output.parents:
        raise SystemExit('Use a new, separate candidate directory; no files changed.')
    repairs = json.loads((PACKAGE / 'corrections.json').read_text(encoding='utf-8'))
    source = module_source(repairs)
    if (PACKAGE / MODULE).read_text(encoding='ascii') != source:
        raise SystemExit('Generated module differs from corrections.json; no files changed.')
    original = client / 'SystemEN/itemInfo.lua'
    loader = original.read_bytes()
    if HOOK.encode() in loader:
        raise SystemExit('Baseline already contains the Time Dimensions hook; no files changed.')
    before_hash = digest(original)
    shutil.copytree(client / 'SystemEN', output / 'SystemEN')
    target = output / 'SystemEN/itemInfo.lua'
    newline = b'\r\n' if b'\r\n' in loader else b'\n'
    target.write_bytes(loader.rstrip(b'\r\n') + newline + newline + HOOK.encode() + newline)
    shutil.copyfile(PACKAGE / MODULE, output / MODULE)
    proof = verify(client, output, lua, repairs)
    assert digest(original) == before_hash, 'Source loader unexpectedly changed'
    receipt = {
        'verified': True, 'installed': False, 'published': False,
        'source_client': str(client), 'candidate': str(output), 'verification': proof,
        'source_loader_sha256': before_hash,
        'release_payload': {name: digest(output / name) for name in ('SystemEN/itemInfo.lua', MODULE)},
    }
    (output / 'tooltip-candidate.json').write_text(json.dumps(receipt, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(receipt, indent=2))


if __name__ == '__main__':
    main()
