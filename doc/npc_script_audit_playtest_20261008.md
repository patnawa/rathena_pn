# NPC script audit — client acceptance checklist (October 8, 2026)

Both audit releases are live (see the [audit record](npc_script_audit_20261008.md)).
Server-side behaviour was verified by the 105-check native release gate,
private whole-NPC startup and a production load that matches it. This checklist
covers what still needs a rendered client. Each line gives the place, the
action and the expected result. Tick each one off as you test it.

## Critical and high repairs

- [ ] **Ivan / Maristella (jor_mbase 233,277 / jalbe_in 68,46).** Finish the
  Cult Infiltration report chain once, then talk to Ivan again. Expected:
  - Ivan does not restart quest 17752.
  - Maristella pays 10 vouchers once only.
  - A character holding a stray 17756 has it cleared, gets no payment, and can
    continue to the 17769 report.
- [ ] **Old Glast Heim Challenge, Oscar's reward.** Use two characters on one
  account, each clearing its own instance, then alternate claims. Expected:
  each instance pays each account once.
- [ ] **Hall of Life gate (t_garden 172,235).** As a leader with a cart, open
  "Use one Hall Key", move the key to the cart, then confirm. Expected:
  "needs one Hall Key" and no reservation. A normal reservation consumes the
  key.
- [ ] **Fall of Glast Heim room Oscars.** Have two members open the same room
  Oscar's dialogue and both confirm. Expected: one population or boss only;
  the second member's dialogue closes without spawning.
- [ ] **Chapter 1 EG03 (ch1_vrgef1 138,146).** Step on the trigger with quest
  19239. Expected: you advance to 19240 straight away. If someone disconnects
  mid-cutscene, the cutscene is available again within about 90 seconds.
- [ ] **Vellgunde (icas_in 185,63), Ice Fire Mana Exchange.**
  - With a free bag slot: 35 Snow Flowers become one Ice Magic Stone.
  - With a full bag: refused before any flowers are taken.
- [ ] **Episode 21 field MVPs** (Icehorn, Yortus Arbiter, Yortus Bailiff,
  Jortus Judge, Jortus Executioner). Kill one. Expected: no respawn for about
  2 hours (± 10 minutes).
- [ ] **Episode 20 gates.**
  - The Diving Iwin's "Travel to Icy Zone" refuses characters below Episode 20
    step 7.
  - Return portals refuse destinations beyond your step (for example jor_root1
    to jor_maze below step 6), with the "Advance Episode 20" message.
- [ ] **Job Master (pn_office).** As a Druid or Karnos, talk to it. Expected:
  you are sent to the Druid Mentor in Prontera, and no Alitea option appears.
- [ ] **Vending during an NPC dialogue.** Open another player's vending list,
  then talk to any NPC and try to buy while its dialogue is open. Expected: the
  purchase is refused. Buying-store sales are refused the same way.
- [ ] **Kafra Special Reserve lottery (aldeba_in).** With a full bag, choose a
  lottery. Expected: "make room for every possible prize" before any roll, and
  no points spent.
- [ ] **Dylan (prt_pri00 61,136), badge reset.** With a full bag, choose reset.
  Expected: refused before the roll, and the badge is unchanged.

## Medium and low repairs that change what players see

- [ ] **Infinite Catalyst Box 3 (item 50150).** Use the box. Expected: its menu
  works and grants the chosen 30-day rental; the box remains.
- [ ] **Dialogue items** such as a teleport scroll. Use one, cancel or let it
  close, then talk to any NPC with a menu. Expected: the NPC's first menu is
  shown normally and isn't answered automatically.
- [ ] **Gold Point Manager dyes.** Expected:
  - The dialogue says the dyes are account-bound.
  - Received dyes can't be sold to an NPC or traded.
- [ ] **Warper.** Expected:
  - Guild Dungeons lists the four Gate Managers.
  - D8 "Tomb of the Fallen" goes to Ohno Tohiro in Lighthalzen.
  - A saved Last Warp into a guild dungeon or the Tomb is refused.
- [ ] **Missionary Rosetta (prontera 225,330).** Expected:
  - The second mission asks "Accept and travel / Not now".
  - Travel asks before writing memo slot 4.
- [ ] **Cardron (kin_in01 123,215).** Expected: refinement and reform name the
  selected equipped item and its refine level before charging.
- [ ] **Lake of Fire exit reward.** Expected: the full reward once per account
  per day, and the reduced reward on other characters the same day. Each
  account gets one extra full claim right after the release.
- [ ] **Sunken Tower.** A registered member who levels out of the rank band
  mid-run can re-enter the started run.
- [ ] **Airship Destruction rope.** A member who died or left can re-enter the
  party's live run. A character on cooldown can't regain the clear quest.
- [ ] **Late joiners.** A character who joins after the encounter started is
  refused the reward and can still leave. Check each of:
  - Final Battle: daily reward, crystals and story reward.
  - Sticky Sea: daily and story branches.
  - Ghost Ship: report.
- [ ] **Weekly Expedition.** Clicking the console during a wave shows a chat
  line, not a dialogue. Waves still advance when guardians die while you're in
  a dialogue.
- [ ] **Twilight Garden.** If the leader disconnects during the step that
  summons the Heart Hunters, they still appear about 6 seconds later.
- [ ] **Nyar (jor_mbase 310,110), storage.** Storages 1–3 open pages I–III, or
  show a "busy or unavailable" message.
- [ ] **Episode 21 doors 18539 and 18557.** Expected: you arrive outside the
  office or barracks and aren't bounced back indoors.
- [ ] **Chapter 1.**
  - The Kafra cart rental charges 700z.
  - Each investigator hides only for you, and stays hidden for 60 seconds even
    after a map change.
  - Verus believers require the Purification Amulet.

## If something fails

Note the character, map, coordinates and time. Each release keeps exact
backups and a guarded `release_deploy.py --rollback`:

- `/app/pn-npc-audit-20261008b` for the follow-up release;
- `/app/pn-npc-audit-20261008` for the first release.

Inspect the journals before rolling back.
