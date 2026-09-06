# Abyss Researcher conversion audit

Read-only audit, 2026-09-06. Scope: `L_Convert` in
`npc/custom/varmundt_biosphere_depth.txt`, currently lines 712–768. No NPC,
recipe, engine, or test source was changed. The normalized-LF label-through-EOF
snapshot is SHA-256
`ad460e707f4872b554be9637218d6b987513b4c7c95f5e3b4490194a63bf5e9d`.

## Confirmed finding: post-input checks are not transaction-wide

The maximum quantity is calculated from current materials and Zeny **before**
`input`. After that real script suspension, the code only calls `checkweight`,
then deletes each input in order, subtracts Zeny, and grants the output. There is
no current-resource preflight covering the entire batch before the first deletion.

This can lose materials, not merely display a stale maximum:

- **All five recipes:** start with sufficient resources, reach the quantity
  prompt, reduce current Zeny below the selected batch cost, then submit the
  quantity. All materials are deleted first. `Zeny -= total` then fails and ends
  the script, without granting output; the reduced current Zeny is unchanged.
- **Recipes 3–5:** reduce a later material below the selected requirement while
  input waits. Earlier `delitem` calls succeed; the later one aborts the script.
  Earlier materials are not restored, and no Zeny charge or output occurs.
- Missing the **first/only** material does not partially delete that same item:
  native `delitem` preflights one item type before deleting it. It still aborts
  with a native error rather than a clean transaction-level refusal.

Exact engine evidence: `script.cpp::buildin_input` sets RERUNLINE and resumes on
the received value; `buildin_delitem_search` performs count-only then delete passes
for **one call**, not the whole recipe; `buildin_delitem` sets END and closes UI
on shortage. `pc.cpp::pc_setparam(SP_ZENY)` rejects negative values without changing
Zeny, and `script.cpp::set_reg_num` sets END on that failure. The script does not
call `pc_payzeny`; that function also rejects insufficient funds but does not make
preceding item deletions transactional. **Do not characterize this stale-Zeny
path as free output: the following `getitem` does not execute.**

This is source-proven behavior for state changed across a suspension. No live
player mutation, reproduction against a server, or claim that ordinary trade is
available during NPC input is made. An isolated VM regression can change the
attached fixture's state exactly at the input boundary.

## All five existing recipes

Amounts below are per one output and preserve the current script exactly.
Every listed ID resolves in effective Renewal imports as Etc, Weight 10, with no
item-specific Stack override.

| Recipe | Inputs, in deletion order | Zeny | Output |
| --- | --- | ---: | --- |
| 1 | 1001550 ×10 | 10,000 | 1001552 Abyss_Magic_Jewel |
| 2 | 1001551 ×10 | 10,000 | 1001553 Time_Dimension_Jewel |
| 3 | 1001552 ×10; 1001553 ×10 | 20,000 | 1001554 Abyss_Rune_Ore |
| 4 | 1001554 ×5; 6607 ×5 | 30,000 | 1001555 Abyss_Rune |
| 5 | 1001555 ×5; 6608 ×5; 6755 ×5; 25866 ×3 | 50,000 | 1001556 Time_D_Ma_Rune |

Other material identities: 1001550 Abyss_Jewel_Fragment, 1001551
Time_Dim_J_Fragment, 6607 Temporal_Crystal, 6608 Coagulated_Spell, 6755
Polluted_Spell, and 25866 Spell_Of_Time. This audit verifies current project data,
not an external claim about official acquisition or economy.

## Quantity and capacity findings

**Ignored input status.** `input .@amount,1,.@max` clamps a supplied value into
that range and returns -1/0/1 for below/in/above range. Its return is discarded.
A submitted 0 or negative value therefore becomes a purchase of 1; a value above
the quoted maximum becomes a purchase of that maximum. The packet handler simply
stores the supplied numeric value. Whether a particular native-client Escape
button submits 0 was not established and must not be assumed. Checking the input
return status avoids unintended clamped transactions.

**No signed-32-bit multiplication overflow is established.** With MAX_ZENY =
INT_MAX, the Zeny-only maximum quantities are 214748, 214748, 107374, 71582, and
42949. Their total prices stay at or below INT_MAX; material multipliers are at
most 10. Ordinary single stacks also limit batches to 3000 or 6000.

**Narrower native item-amount fields are a conditional risk.** `delitem` assigns
the requested quantity to `item.amount` (int16), while `checkweight` and `getitem`
use uint16. `countitem` aggregates non-rental stacks by ID and does not distinguish
binding/cards/UID; `pc_additem` can retain separate stacks when those attributes
differ. If aggregate input exceeds a signed-16-bit request, a batch of 3277 with
a ×10 input requests 32770, which narrows to -32766 on this build and is skipped
by `delitem`. The ×5 boundary is 6554 outputs. Larger uint16 output narrowing is
also possible from the uncapped theoretical maxima. This arithmetic is established;
normal player acquisition/carrying capacity sufficient for such inventories was
not established. It warrants a defensive batch cap, not a claim of a demonstrated
live exploit.

The post-input `checkweight` is current, so ordinary weight/slot deterioration
is rejected before deletion. It is conservative: it does not credit input weight
or slots that deletion would release. Also, `pc_checkadditem` matches existing
stacks by ID, whereas `pc_additem` additionally matches binding/cards/UID; unusual
existing output-stack variants can make that generic preflight imperfect. That
shared-engine capacity issue is separate from this NPC's missing resource check.

## Minimal proposed patch, for assignment/review

Keep all five recipes and their order unchanged. Immediately after a successful
quantity input, and before any deletion:

```text
if (input(.@amount,1,.@max) != 0)
    close;
.@total = .@cost * .@amount;
if (Zeny < .@total) {
    mes "Your materials or Zeny changed. Please try again.";
    close;
}
for (.@i = 0; .@i < .@matcount; ++.@i) {
    if (countitem(.@mat[.@i]) < .@need[.@i] * .@amount) {
        mes "Your materials or Zeny changed. Please try again.";
        close;
    }
}
// Existing current checkweight, then existing deletions/payment/output.
```

There must be **no further suspension** between that preflight and mutation.
Optionally call the already-present pure `S_Access` with threshold 0 after input
to revalidate the original level/story/Depth-1 access policy; do not introduce
a new Depth-2 reputation threshold.

For a safe batch-width guard, cap the displayed/input maximum so every per-material
deletion is at most 30000, the native MAX_AMOUNT, and the output is at most 30000.
That yields at most 3000 for recipes 1–3 and 6000 for recipes 4–5, without changing
per-unit economy. MAX_AMOUNT is **not exported as a script constant** in this
snapshot: do not insert an undefined symbol. A documented local bound can be
derived from the verified engine limit. Treat this as a separately explicit part
of the patch review, not an invented official batch limit.

## Focused regression proposal and project precedent

Use the actual extracted `L_Convert` path and real input/delitem/Zeny assignment
semantics, with explicit native inventory/payment boundaries and sanitizer checks:

- Every recipe: quantities 1 and the allowed maximum consume exact inputs/Zeny
  and create the exact output; quantity 0, negative, and above maximum create none.
- Every recipe: lower Zeny by one below the chosen cost while input waits; expect
  no mutation at all and no native error after the fix.
- Every recipe: shortage of each material in turn, especially the last of the
  four inputs in recipe 5; preserve every other material, Zeny, and output count.
- Changed capacity, exact capacity, replenished resources, and input timeout;
  no partial transaction. Conditional width fixtures straddle 32767 material
  units and 65535 output units and prove the chosen cap prevents native narrowing.
- If access revalidation is included, change each original eligibility predicate
  while input waits; reject without payment or changed recipe policy.

The existing Rare Poison Herb Collector in `npc/re/merchants/3rd_trader.txt`
(around lines 84–106) checks current `countitem` and Zeny after input and before
deleting/payment. The Craft Book Merchant in `npc/re/merchants/alchemist.txt`
(around lines 83–115) validates quantity and then current Zeny/capacity after its
last confirmation. `doc/script_commands.txt` explicitly documents input's clamp
and status return. The nearby Omega/Ellie conversion patterns inspected have the
same pre-input-only resource pattern and are **not** a safe precedent; they were
not expanded into this task or edited.

Evidence level: direct source trace and effective-data/arithmetic inspection,
not a newly executed native VM regression. Engine snapshots: script.cpp
`6e01f947d419ae89527dc40ad37d0f184af1af37f86742a6ad315eefcd47fb8d`, pc.cpp
`d02a7ac0a42c3ab768075580dba3b01a8941c1cac4e0997895fa24ba262fda8d`.
