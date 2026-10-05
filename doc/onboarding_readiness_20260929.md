# Onboarding and instance readiness pilot

`npc/custom/main_office/readiness.txt` adds exact objectives and navigation for ten
initial states: below level 200, invitation not accepted, and active quests 18368,
18369, 18370, 18371, 24069, 24070, 24071 and 24072. Coordinates and transitions
come from `npc/custom/chapter1/CH1.c`. Later Chapter 1 states retain quest-window
guidance; the pilot does not claim coverage of the entire chapter.

The Instance Desk offers Crossroads of Tangled Mana, Nyrholt, Phantom of Nyrholt,
Ghost Palace and Charleston in Distress. It shows the actual missing prerequisite,
cooldown, reservation or readiness, an entrance destination and material tags.
Expired stock-instance cooldowns explicitly require the existing entrance reset
conversation. Merely viewing the desk never clears them.

`PN_InstanceMissing` is shared by the desk and the actual admission sites. Chapter
2 checks it before reservation and again immediately before entry. Ghost Palace
and Charleston check it immediately before `instance_enter`, after their existing
dialogs. Core admission still performs the final reservation/member/map check.
Chapter 2 original-roster membership, nonleader reentry and conflicting party
reservations remain enforced. No progression, reward or currency is changed by
the read-only helper.

`tools/ci/onboarding_readiness_test.py` links the production native VM and executes
the actual functions. It uses explicit transport, registry and party-lookup
boundaries, real quest-state/timer builtins, real leader and instance lookup, and
real inventory metadata. Cases cover all ten navigation states, five ready/party/
leader/reservation sets, level boundaries, active/completed/expired timers, Apples,
Phantom cooldown, matching reservations and original-roster checks. It also checks
that live admission sites still call the shared predicate. Run against a freshly
built candidate; old map objects are incompatible with the changed session layout.

The final musl candidate's 71-check release gate passed this native fixture under
source identity `bf65a030e5370f8a57808de02b5af9bdf3d74858c152920c6f268a5a2d629c01`.
The earlier old-object fixture result is superseded by that freshly built run.
Rendered client verification remains separate. No retention or time-to-objective
improvement is claimed without a measured player trial.
