# Extended memo quest client metadata

Release the four files under `SystemEN` and `System` together. Each small existing
quest loader still loads the canonical `SystemEN/OngoingQuests.lub` and then
adds `SystemEN/ExtendedMemoQuests.lua`. No canonical quest records are removed.
If another release changed a loader, append only the new dofile line.

The native Warp Portal destination list uses the client's existing variable-length
0x0abe packet; this fragment supplies quest descriptions only. Server code/data
are required for six-slot storage and selection. All inter-server binaries must
be rebuilt and restarted together because MAX_MEMOPOINTS changes shared structures.
