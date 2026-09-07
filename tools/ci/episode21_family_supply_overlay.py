#!/usr/bin/env python3
"""Exact, self-contained Family transaction overlay for QuestInfo audits.

The Episode 20/21 QuestInfo migration owns only registration relocation. This
module owns the independently reviewed Family transaction anchors and exact
inverse. It accepts only the complete QI-only file or complete final overlay,
so the QuestInfo audit can canonicalize an installed overlay without treating
transaction changes as part of its own migration.
"""
from __future__ import annotations

import hashlib


NPC = 'npc/custom/episode21/FamilyReputation.txt'
QI_ONLY_SHA = 'c72019f51033d96054c277d952d993a45a68062da9f7b4ab99ee76edde2a9605'
OVERLAY_SHA = '3e6217fa35f57cc1969810938b523d2970b4982e6ce0c383aefea26a2d266aeb'

FRESH_KEY_OLD = '''\t\tif (countitem(.@item[.@i]) < 10) {
'''
FRESH_KEY_NEW = '''\t\t// The delivery menu can remain open across the 04:00 reset.
\t\t// Re-sample once after its final yield and use that key for this commit.
\t\t.@key = callfunc("EP21_DailyKey");
\t\tif (EP21_Supply_Daily == .@key) {
\t\t\tmes "Today's procurement contract is already complete. New contracts are posted at 04:00 server time.";
\t\t\tclose;
\t\t}
\t\tif (countitem(.@item[.@i]) < 10) {
'''

MARKER_OLD = '''\t\tEP21_Supply_Daily = .@key;
\t\tEP21_SupplyFamily = 0;
\t\tmes "Contract complete: ^3366FF10 Wigner vouchers^000000 and ^33AA33100 " + .@family$[.@i] + " reputation^000000.";
'''
MARKER_NEW = '''\t\tEP21_Supply_Daily = .@key;
\t\tEP21_SupplyFamily = 0;
\t\t// completequest refreshes before these two character markers change.
\t\tquestinfo_refresh();
\t\tmes "Contract complete: ^3366FF10 Wigner vouchers^000000 and ^33AA33100 " + .@family$[.@i] + " reputation^000000.";
'''

ACCEPT_OLD = '''\tEP21_SupplyFamily = .@choice;
\tsetquest .@quest[.@choice];
\tmes "Contract registered. Return with ten units of item ^3366FF" + .@item[.@choice] + "^000000.";
'''
ACCEPT_NEW = '''\t// A completed row remains in the native quest log and must be recycled.
\t// Preserve an already-active selected row as a recovery path.
\tif (isbegin_quest(.@quest[.@choice]) == 2)
\t\terasequest .@quest[.@choice];
\tif (!isbegin_quest(.@quest[.@choice]))
\t\tsetquest .@quest[.@choice];
\tEP21_SupplyFamily = .@choice;
\tmes "Contract registered. Return with ten units of item ^3366FF" + .@item[.@choice] + "^000000.";
'''

CAPACITY_OLD = '''\t\tif (!checkweight(1001618,10)) {
\t\t\tmes "Make room for ten Wigner Merchant Guild Vouchers first.";
\t\t\tclose;
\t\t}
'''
CAPACITY_NEW = '''\t\tif (!callsub(L_EP21VoucherCapacity)) {
\t\t\tmes "Make room for ten Wigner Merchant Guild Vouchers first.";
\t\t\tclose;
\t\t}
'''

HELPER_OLD = '''\tend;

OnInit:
'''
HELPER_NEW = '''\tend;

// Predict the exact plain getitem below. Native pc_checkadditem only checks an
// item's ID; pc_additem instead uses the first matching bound/expiry/UID/cards
// row, or the first empty allowed slot. Keep the conservative pre-debit policy.
L_EP21VoucherCapacity:
\tif (getiteminfo(1001618,ITEMINFO_WEIGHT) != 0 || Weight > MaxWeight)
\t\treturn false;
\t.@slots = getinventoryslots();
\tif (.@slots < 1 || .@slots > MAX_INVENTORY)
\t\treturn false;
\tgetinventorylist;
\t.@match = -1;
\tfor (.@i = 0; .@i < @inventorylist_count; ++.@i) {
\t\tif (@inventorylist_idx[.@i] < .@slots)
\t\t\t++.@occupied;
\t\tif (.@match >= 0 || @inventorylist_id[.@i] != 1001618)
\t\t\tcontinue;
\t\tif (@inventorylist_bound[.@i] != 0 || @inventorylist_expire[.@i] != 0 || @inventorylist_uniqueid$[.@i] != "0" || @inventorylist_card1[.@i] != 0 || @inventorylist_card2[.@i] != 0 || @inventorylist_card3[.@i] != 0 || @inventorylist_card4[.@i] != 0)
\t\t\tcontinue;
\t\t.@match = .@i;
\t}
\tif (.@match >= 0)
\t\treturn @inventorylist_idx[.@match] < .@slots && @inventorylist_amount[.@match] <= 29990;
\treturn .@occupied < .@slots;

OnInit:
'''

EDITS = (
    (FRESH_KEY_OLD, FRESH_KEY_NEW),
    (MARKER_OLD, MARKER_NEW),
    (ACCEPT_OLD, ACCEPT_NEW),
    (CAPACITY_OLD, CAPACITY_NEW),
    (HELPER_OLD, HELPER_NEW),
)


def _sha(data):
    return hashlib.sha256(data).hexdigest()


def _normalize(data):
    result = data.replace(b'\r\n', b'\n')
    if b'\r' in result:
        raise AssertionError('Only LF/CRLF family overlay forms are supported')
    return result


def _rewrite(text, edits):
    result = text
    for old, new in edits:
        if result.count(old) != 1:
            raise AssertionError('Ambiguous or missing Family overlay anchor')
        result = result.replace(old, new, 1)
    return result


def apply_to_qi_only(data):
    """Return the exact LF transaction overlay for the exact QI-only file."""
    active = _normalize(data)
    if _sha(active) != QI_ONLY_SHA:
        raise AssertionError('Family overlay input is not the exact QI-only source')
    candidate = _rewrite(active.decode('utf-8'), EDITS).encode()
    if _sha(candidate) != OVERLAY_SHA:
        raise AssertionError('Family overlay anchors did not reproduce reviewed bytes')
    return candidate


def remove_from_overlay(data):
    """Return exact LF QI-only bytes for the exact reviewed overlay."""
    active = _normalize(data)
    if _sha(active) != OVERLAY_SHA:
        raise AssertionError('Family source is not the exact reviewed overlay')
    canonical = _rewrite(active.decode('utf-8'), tuple(
        (new, old) for old, new in reversed(EDITS))).encode()
    if _sha(canonical) != QI_ONLY_SHA:
        raise AssertionError('Family overlay inverse did not reconstruct QI-only bytes')
    return canonical


def canonicalize_for_questinfo(path, data):
    """Return ``(QI-only LF bytes, overlay phase)`` for the Family path."""
    if path != NPC:
        raise AssertionError('Family overlay adapter received another source')
    active = _normalize(data)
    digest = _sha(active)
    if digest == QI_ONLY_SHA:
        return active, 'qi-only'
    if digest == OVERLAY_SHA:
        return remove_from_overlay(active), 'family-transaction'
    raise AssertionError('Unknown FamilyReputation overlay bytes')
