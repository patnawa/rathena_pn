"""Exact reviewed service-region exceptions for independent regressions.

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
DOCUMENT_START = b'// Research documents become reputation at a fixed 2:3 ratio.\n'
DOCUMENT_OLD_SHA = '5be9add546e2f0033a612398e58dc512a5ad21b2b29d2485175ee7eb1be49810'
DOCUMENT_FIXED_SHA = 'a37f8a425350908ee1a294d7884571c3b6adc9610615fc194ec59c5b3416a720'
DOCUMENT_OLD_INPUT = b'\tinput .@amount,1,.@max;\n\tdelitem 1001289,.@amount * 2;\n'
DOCUMENT_FIXED_INPUT = b'''\tif (input(.@amount,1,.@max) != 0)
\t\tclose;
\tif (strcharinfo(3) != "ba_in01")
\t\tclose;
\tif (!callfunc("F_BioDepthQuestAccess"))
\t\tclose;
\t// Revalidate the selected order after the final dialogue suspension.
\t.@rep = get_reputation_points(REPUTATION_BIOSPHERE_DEPTH1);
\t.@limit = (5000 - .@rep + 2) / 3;
\tif (.@amount < 1 || .@amount > .@limit) {
\t\tmes "Your research reputation changed. Please choose a new amount.";
\t\tclose;
\t}
\tif (countitem(1001289) < .@amount * 2) {
\t\tmes "Your research documents changed. No documents were used.";
\t\tclose;
\t}
'''
DOCUMENT_MOVED_DEBIT = b'''\t// Keep the final partial-credit pair, but never consume redundant pairs.
\t// The reviewed ba_in01 callbacks do not mutate these payment resources.
\tdelitem 1001289,.@amount * 2;
'''

def normal(data):
    return data.replace(b'\r\n', b'\n').replace(b'\r', b'\n')

def checked_digest(data, pins):
    if hashlib.sha256(data).hexdigest() not in pins:
        raise ValueError('Unreviewed Biosphere service-region change')


def restore_reviewed_document(data):
    """Invert ONLY the independently pinned document repair in a test view."""
    data = normal(data)
    if data.count(DOCUMENT_START) != 1 or data.count(FUSION_START) != 1:
        raise ValueError('Ambiguous/missing exact document boundaries')
    start, end = data.index(DOCUMENT_START), data.index(FUSION_START)
    if end <= start:
        raise ValueError('Reordered document boundaries')
    region = data[start:end]
    checked_digest(region, {DOCUMENT_OLD_SHA, DOCUMENT_FIXED_SHA})
    if hashlib.sha256(region).hexdigest() == DOCUMENT_FIXED_SHA:
        if region.count(DOCUMENT_FIXED_INPUT) != 1 or region.count(DOCUMENT_MOVED_DEBIT) != 1:
            raise ValueError('Reviewed document inverse is not unique')
        region = region.replace(DOCUMENT_FIXED_INPUT, DOCUMENT_OLD_INPUT, 1)
        region = region.replace(DOCUMENT_MOVED_DEBIT, b'', 1)
        checked_digest(region, {DOCUMENT_OLD_SHA})
    return data[:start] + region + data[end:]

def without_reviewed_fusion(data):
    data = normal(data)
    if data.count(FUSION_START) != 1 or data.count(FUSION_END) != 1:
        raise ValueError('Ambiguous/missing exact fusion boundaries')
    start, end = data.index(FUSION_START), data.index(FUSION_END)
    if end <= start:
        raise ValueError('Reordered fusion boundaries')
    checked_digest(data[start:end], FUSION_PINS)
    return data[:start] + data[end:]


def reviewed_depth_baseline(data):
    """Protect every byte outside the exact reviewed fusion/document repairs."""
    return without_reviewed_fusion(restore_reviewed_document(data))

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
        (reviewed_depth_baseline, old_depth, new_depth, FUSION_START),
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


def document_selftest(old, new):
    expected = normal(old)
    for data in (normal(new), normal(new).replace(b'\n', b'\r\n')):
        assert restore_reviewed_document(data) == expected
    probes = [normal(new).replace(b'2:3 ratio.', b'2:4 ratio.', 1),
              normal(new).replace(b'.@amount > .@limit', b'.@amount >= .@limit', 1),
              normal(new) + DOCUMENT_START,
              normal(new).replace(DOCUMENT_START, b'', 1),
              normal(new).replace(FUSION_START, b'', 1),
              b'// unrelated mutation\n' + normal(new)]
    for data in probes:
        try:
            assert restore_reviewed_document(data) == expected
        except (ValueError, AssertionError):
            pass
        else:
            raise AssertionError('Document scope mutation was accepted')
    return {'positive_newline_forms': 2, 'mutations_rejected': len(probes)}
