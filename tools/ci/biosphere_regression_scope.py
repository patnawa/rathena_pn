"""Exact reviewed material-only exceptions for older independent regressions.

Never ignore an arbitrary NPC region: excluded bytes must match one of the
manually reviewed old/current SHA256 values. Callers still require the broad
callback gate and compare every remaining tested/protected byte.
"""
import hashlib

FUSION_START = b'// Eight-element material fusion.\n'
FUSION_END = b'// Armor purchase, native enchant UI, and native reform catalysts.\n'
FUSION_PINS = {
    '56fce7b5409d93c52cb73a5455b2d353eb6fb5a48e395cc1b0f9d09af20af3ec',
    '2e857818e6aae27b41bbba68d33f06a4430671a28c262e0f5bf8557307fd242b',
}
EQUIPMENT = b'ba_in01,359,53,4\tscript\tEllie#biosphere_equipment\t4_EP17_ELYUMINA,{\n'
QUESTS_PREFIX_PINS = {
    'ef38aa2c2ad1ba0d23657f5dbc6650bb387e659d7dec83969577a065a19b7d5f',
    'd5c4f210208940d44253d39945ace7d01c98a301afc170b59956de60cc59ac03',
}

def normal(data):
    return data.replace(b'\r\n', b'\n').replace(b'\r', b'\n')

def checked_digest(data, pins):
    if hashlib.sha256(data).hexdigest() not in pins:
        raise ValueError('Unreviewed Biosphere material-only scope change')

def without_reviewed_fusion(data):
    data = normal(data)
    if data.count(FUSION_START) != 1 or data.count(FUSION_END) != 1:
        raise ValueError('Ambiguous/missing exact fusion boundaries')
    start, end = data.index(FUSION_START), data.index(FUSION_END)
    if end <= start:
        raise ValueError('Reordered fusion boundaries')
    checked_digest(data[start:end], FUSION_PINS)
    return data[:start] + data[end:]

def equipment_tail(data):
    data = normal(data)
    if data.count(EQUIPMENT) != 1:
        raise ValueError('Ambiguous/missing exact equipment Ellie declaration')
    prefix, tail = data.split(EQUIPMENT)
    checked_digest(prefix, QUESTS_PREFIX_PINS)
    return EQUIPMENT + tail

def selftest(old_depth, new_depth, old_quests, new_quests):
    positives = negatives = 0
    for fn, old, new, marker in (
        (without_reviewed_fusion, old_depth, new_depth, FUSION_START),
        (equipment_tail, old_quests, new_quests, EQUIPMENT),
    ):
        expected = fn(old)
        for data in (normal(new), normal(new).replace(b'\n', b'\r\n')):
            assert fn(data) == expected, 'Protected source changed'
            positives += 1
        probes = [normal(new).replace(marker, marker + b'// changed\n', 1),
                  normal(new) + marker,
                  b'// unexpected source change\n' + normal(new)]
        for probe in probes:
            try:
                assert fn(probe) == expected
            except (ValueError, AssertionError):
                negatives += 1
            else:
                raise AssertionError('Scope mutation control was accepted')
    return {'positive_newline_forms': positives, 'mutations_rejected': negatives}
