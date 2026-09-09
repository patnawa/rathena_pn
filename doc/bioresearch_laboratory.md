# Bioresearch Laboratory

This implementation adds the EDDA laboratory expedition on the installed native maps `1@gol1` and `1@gol2`. Sierra at Yuno (216,345) handles reservations and admission. The Warper sends eligible characters to (216,343), beside her.

The official foundation is documented in the [publisher update](https://renewal.playragnarok.com/news/updatedetail.aspx?id=356): level 170, Expedition and Battle modes, a shared daily entry, and a two-hour instance. Existing quest 16390 supplies the server's daily 04:00 reset. The local empty-reservation timeout is five minutes. Instance ID 80 uses both native maps; no compatibility terrain or executable change is required.

## Encounter and rewards

Both modes traverse seven areas. Battle finishes with the Unknown Swordsman; Expedition concludes after the exploration waves. Dialogue, wave populations, device timing and regroup behavior are original implementation choices. The public walkthrough establishes area order, sleep gas, blindness removal, four containment sections and the final boss, but does not provide a complete authoritative script or all timing values. This is a playable reconstruction, not a claim of an exact official encounter replica.

Expedition awards two Bio Experiment Fragments. Battle awards nine Bio Research Documents, fifteen Bio Experiment Fragments, one Unknown Antiquity and one random weapon from the existing 39-entry EDDA weapon group. All rewards are delivered together by Sierra in the final chamber. The selected weapon is retained across inventory-capacity retries. Quest 16400's client description matches these Battle rewards.

Admission and reward eligibility belong to the live instance and character roster. Failed admission must not consume the daily entry; a new reservation cannot inherit an old reservation's eligibility. After the run starts, disconnected members do not block regroup checkpoints; their exploration re-entry follows the current checkpoint. The final fight freezes only its online, present party members as eligible. A member absent at that lock cannot rejoin the fight or claim its completion reward. Rewards are checked for capacity before granting and can be claimed once per character in that run, including across the daily reset.

The original wave tuning uses 21 sections: one guard per Expedition section, or six actors per Battle section. Sleep gas pulses every ten seconds until its device is disabled for that section. The vision device removes blindness. Containment uses four native walkable squares, with temporary perimeter blocking and visible flame markers; clearing each section restores those cells. Re-entry during containment returns inside the active section.

## Validation boundary

Client GAT/GND/RSW resources and all referenced models/textures are present. Server and client walkability match. Automated tests and a production-binary startup validate script behavior and database loading; these do not replace a graphical party playthrough of pacing, combat balance or visual device placement.

Validated on 2026-09-09: 241 native encounter assertions and 401 Warper assertions passed with zero script errors or leaks; three native-map geometry tests passed; 11,393 client quest records remained valid. The production binary loaded the final database/scripts successfully in isolation. Deployment retained the server executables and passed service readiness and file-hash checks.
