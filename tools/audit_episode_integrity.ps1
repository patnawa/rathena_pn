param(
	[switch]$StrictContent
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$failures = [Collections.Generic.List[string]]::new()
$warnings = [Collections.Generic.List[string]]::new()

function Fail([string]$Message) { $failures.Add($Message) }
function Warn([string]$Message) { $warnings.Add($Message) }
function RepoPath([string]$Path) { [IO.Path]::GetFullPath((Join-Path $repo $Path)) }

function Read-InstanceEntries([string[]]$Files) {
	$entries = [Collections.Generic.List[object]]::new()
	foreach ($relative in $Files) {
		$path = RepoPath $relative
		$current = $null
		foreach ($line in [IO.File]::ReadLines($path)) {
			if ($line -match '^\s*- Id:\s*(\d+)\s*$') {
				if ($null -ne $current) { $entries.Add($current) }
				$current = [pscustomobject]@{
					Id = [int]$Matches[1]
					Name = $null
					File = $relative
					Maps = [Collections.Generic.List[string]]::new()
				}
				continue
			}
			if ($null -eq $current) { continue }
			if ($line -match '^\s+Name:\s*(.+?)\s*$') { $current.Name = $Matches[1]; continue }
			if ($line -match '^\s+Map:\s*([A-Za-z0-9_@]+)\s*$') { $current.Maps.Add($Matches[1]); continue }
			if ($line -match '^\s+([A-Za-z0-9_@]+):\s*(?:true|false)\s*$') { $current.Maps.Add($Matches[1]) }
		}
		if ($null -ne $current) { $entries.Add($current) }
	}
	return $entries
}

function Read-MapCacheEntries([string[]]$Files, [Collections.Generic.HashSet[string]]$Wanted) {
	# Map-server resolves caches in this order: import, mode-specific, base.
	# Keep the first copy of a map so this audit examines the same geometry.
	$entries = @{}
	foreach ($relative in $Files) {
		$path = RepoPath $relative
		if (!(Test-Path -LiteralPath $path -PathType Leaf)) { continue }
		$stream = [IO.File]::OpenRead($path)
		$reader = [IO.BinaryReader]::new($stream)
		try {
			[void]$reader.ReadUInt32() # file size
			$mapCount = $reader.ReadUInt16()
			[void]$reader.ReadUInt16() # native-struct alignment padding
			for ($index = 0; $index -lt $mapCount; $index++) {
				$name = [Text.Encoding]::ASCII.GetString($reader.ReadBytes(12)).Trim([char]0)
				$width = $reader.ReadInt16()
				$height = $reader.ReadInt16()
				$length = $reader.ReadInt32()
				if ($length -lt 6 -or $reader.BaseStream.Position + $length -gt $reader.BaseStream.Length) {
					throw "Invalid compressed record for map '$name' in $relative"
				}
				if (!$Wanted.Contains($name) -or $entries.ContainsKey($name)) {
					[void]$reader.BaseStream.Seek($length, [IO.SeekOrigin]::Current)
					continue
				}

				$compressed = $reader.ReadBytes($length)
				# rAthena stores zlib streams. DeflateStream expects the raw DEFLATE
				# section, excluding the two-byte zlib header and four-byte checksum.
				$input = [IO.MemoryStream]::new($compressed, 2, $compressed.Length - 6, $false, $true)
				$inflate = [IO.Compression.DeflateStream]::new($input, [IO.Compression.CompressionMode]::Decompress)
				$output = [IO.MemoryStream]::new()
				try { $inflate.CopyTo($output) }
				finally { $inflate.Dispose(); $input.Dispose() }
				$cells = $output.ToArray()
				$output.Dispose()
				if ($cells.Length -ne $width * $height) {
					throw "Decoded cell count for '$name' is $($cells.Length), expected $($width * $height)"
				}
				$entries[$name] = [pscustomobject]@{
					Name = $name
					Width = $width
					Height = $height
					Cells = $cells
					File = $relative
				}
			}
		}
		finally { $reader.Dispose(); $stream.Dispose() }
	}
	return $entries
}

function Test-WalkableCell([object]$Map, [int]$X, [int]$Y) {
	if ($X -lt 0 -or $Y -lt 0 -or $X -ge $Map.Width -or $Y -ge $Map.Height) { return $false }
	$gatType = $Map.Cells[$X + $Y * $Map.Width]
	return $gatType -eq 0 -or $gatType -eq 3
}

function Test-ConnectedCells([object]$Map, [int]$FromX, [int]$FromY, [int]$ToX, [int]$ToY) {
	if (!(Test-WalkableCell $Map $FromX $FromY) -or !(Test-WalkableCell $Map $ToX $ToY)) { return $false }
	$from = $FromX + $FromY * $Map.Width
	$to = $ToX + $ToY * $Map.Width
	if ($from -eq $to) { return $true }
	$visited = [byte[]]::new($Map.Width * $Map.Height)
	$queue = [Collections.Generic.Queue[int]]::new()
	$visited[$from] = 1
	$queue.Enqueue($from)
	$offsets = @(
		@(-1,-1), @(0,-1), @(1,-1),
		@(-1, 0),           @(1, 0),
		@(-1, 1), @(0, 1),  @(1, 1)
	)
	while ($queue.Count -gt 0) {
		$current = $queue.Dequeue()
		$x = $current % $Map.Width
		$y = [Math]::Floor($current / $Map.Width)
		foreach ($offset in $offsets) {
			$nextX = $x + $offset[0]
			$nextY = $y + $offset[1]
			if (!(Test-WalkableCell $Map $nextX $nextY)) { continue }
			$next = $nextX + $nextY * $Map.Width
			if ($visited[$next]) { continue }
			if ($next -eq $to) { return $true }
			$visited[$next] = 1
			$queue.Enqueue($next)
		}
	}
	return $false
}

function Get-DatabaseIds([string[]]$Directories, [string]$Filter) {
	$ids = [Collections.Generic.HashSet[int]]::new()
	foreach ($directory in $Directories) {
		Get-ChildItem -LiteralPath (RepoPath $directory) -Filter $Filter -File | ForEach-Object {
			foreach ($line in [IO.File]::ReadLines($_.FullName)) {
				if ($line -match '^\s*- Id:\s*(\d+)\s*$') { [void]$ids.Add([int]$Matches[1]) }
			}
		}
	}
	return $ids
}

Write-Host 'Episode integrity audit'

# Every enabled NPC script must exist.
$enabledScripts = [Collections.Generic.List[string]]::new()
foreach ($config in @('npc/re/scripts_athena.conf', 'npc/scripts_custom.conf')) {
	foreach ($line in [IO.File]::ReadLines((RepoPath $config))) {
		if ($line -match '^\s*npc:\s*(\S.*?)\s*$') {
			$enabledScripts.Add($Matches[1])
			if (!(Test-Path -LiteralPath (RepoPath $Matches[1]) -PathType Leaf)) {
				Fail "Missing enabled NPC script: $($Matches[1]) ($config)"
			}
		}
	}
}
Write-Host "  enabled NPC scripts: $($enabledScripts.Count)"

# Every imported database file must exist.
$dbRoots = @(
	'db/item_db.yml', 'db/item_group_db.yml', 'db/mob_db.yml',
	'db/quest_db.yml', 'db/instance_db.yml', 'db/map_drops.yml',
	'db/item_randomopt_db.yml', 'db/item_randomopt_group.yml',
	'db/reputation.yml', 'db/reputation_group.yml', 'db/const.yml',
	'db/item_enchant.yml', 'db/item_reform.yml', 'db/item_combos.yml',
	'db/laphine_synthesis.yml', 'db/laphine_upgrade.yml',
	'db/skill_db.yml', 'db/status.yml', 'npc/custom/barters.yml'
)
$dbImports = [Collections.Generic.List[string]]::new()
$optionalImports = @(
	'db/import/item_db.yml', 'db/import/item_group_db.yml', 'db/import/mob_db.yml',
	'db/import/quest_db.yml', 'db/import/map_drops.yml',
	'db/import/item_randomopt_db.yml', 'db/import/item_randomopt_group.yml',
	'db/import/reputation.yml', 'db/import/reputation_group.yml', 'db/import/const.yml',
	'db/import/item_enchant.yml', 'db/import/item_reform.yml',
	'db/import/item_combos.yml', 'db/import/laphine_synthesis.yml',
	'db/import/laphine_upgrade.yml', 'db/import/skill_db.yml', 'db/import/status.yml'
)
foreach ($rootFile in $dbRoots) {
	foreach ($line in [IO.File]::ReadLines((RepoPath $rootFile))) {
		if ($line -match '^\s*- Path:\s*(\S.*?)\s*$') {
			$dbImports.Add($Matches[1])
			if (!(Test-Path -LiteralPath (RepoPath $Matches[1]) -PathType Leaf) -and $Matches[1] -notin $optionalImports) {
				Fail "Missing database import: $($Matches[1]) ($rootFile)"
			}
		}
	}
}
Write-Host "  database imports: $($dbImports.Count)"

# Duplicate top-level records inside one YAML file are ambiguous: the later
# entry silently replaces or merges with the first depending on database type.
# Imports may intentionally override a base record, so this check is scoped to
# duplicates within each individual file.
$recordFiles = @(Get-ChildItem -LiteralPath (RepoPath 'db/re'),(RepoPath 'db/import') -File |
	Where-Object Name -Match '(^|_)(mob|quest|instance)_db\.yml$|(^|_)item_db(_.+)?\.yml$')
$recordFileCount = 0
foreach ($file in $recordFiles) {
	$recordFileCount++
	$idLines = [Collections.Generic.List[object]]::new()
	$lineNumber = 0
	foreach ($line in [IO.File]::ReadLines($file.FullName)) {
		$lineNumber++
		if ($line -notmatch '^(\s*)- Id:\s*(\d+)\s*$') { continue }
		$idLines.Add([pscustomobject]@{
			Indent = $Matches[1].Length
			Id = [int]$Matches[2]
			Line = $lineNumber
		})
	}
	if ($idLines.Count -eq 0) { continue }
	$topLevelIndent = ($idLines | Measure-Object -Property Indent -Minimum).Minimum
	$seen = [Collections.Generic.HashSet[int]]::new()
	foreach ($record in $idLines) {
		if ($record.Indent -ne $topLevelIndent) { continue }
		if (!$seen.Add($record.Id)) {
			$relative = $file.FullName.Substring($repo.Length + 1)
			Fail "Duplicate top-level ID $($record.Id) in ${relative}:$($record.Line)"
		}
	}
}
Write-Host "  database record files checked: $recordFileCount"

# The map-server stores a monster display name in NAME_LENGTH (24 bytes),
# including the terminating NUL. Imported overlays must therefore stay at or
# below 23 UTF-8 bytes even though an older loader warning checks the wrong
# boundary.
$mobDisplayNames = 0
foreach ($file in Get-ChildItem -LiteralPath (RepoPath 'db/import') -Filter '*mob_db.yml' -File) {
	$lineNumber = 0
	foreach ($line in [IO.File]::ReadLines($file.FullName)) {
		$lineNumber++
		if ($line -notmatch '^\s+Name:\s*(.+?)\s*$') { continue }
		$name = $Matches[1].Trim('"', "'")
		$mobDisplayNames++
		$bytes = [Text.Encoding]::UTF8.GetByteCount($name)
		if ($bytes -gt 23) {
			$relative = $file.FullName.Substring($repo.Length + 1)
			Fail "Monster display name is $bytes bytes (maximum 23) at ${relative}:${lineNumber}: $name"
		}
	}
}
Write-Host "  imported monster display names: $mobDisplayNames"

# Build the authoritative map-name set.
$maps = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
foreach ($line in [IO.File]::ReadLines((RepoPath 'db/map_index.txt'))) {
	if ($line -match '^\s*([^/\s][^\s]*)') { [void]$maps.Add($Matches[1]) }
}
Write-Host "  map index entries: $($maps.Count)"

# Instance IDs/names must be unique and every source/additional map must exist.
$instanceFiles = @('db/re/instance_db.yml', 'db/import/instance_db.yml', 'db/import/chapter2_instance_db.yml')
$instances = @(Read-InstanceEntries $instanceFiles)
foreach ($group in ($instances | Group-Object Id | Where-Object Count -gt 1)) {
	Fail "Duplicate instance ID $($group.Name): $(($group.Group.Name) -join ', ')"
}
foreach ($group in ($instances | Group-Object Name | Where-Object Count -gt 1)) {
	Fail "Duplicate instance name $($group.Name)"
}
foreach ($instance in $instances) {
	if (!$instance.Name) { Fail "Instance ID $($instance.Id) has no name ($($instance.File))" }
	foreach ($map in $instance.Maps) {
		if (!$maps.Contains($map)) { Fail "Instance '$($instance.Name)' references missing map '$map'" }
	}
}
Write-Host "  instance definitions: $($instances.Count)"

# Literal custom-script map destinations and declarations must exist. Dynamic
# prefixes used by the stock MVP scripts are explicitly excluded.
$customFiles = @(Get-ChildItem -LiteralPath (RepoPath 'npc/custom') -Recurse -File -Include '*.txt','*.c')
$ignoredDynamicMaps = @('06guild_0', 'pvp_n_', 'SavePoint')
$literalMapRefs = 0
foreach ($file in $customFiles) {
	$lineNumber = 0
	foreach ($line in [IO.File]::ReadLines($file.FullName)) {
		$lineNumber++
		$candidates = [Collections.Generic.List[string]]::new()
		if ($line -match '^\s*([A-Za-z0-9_@]+),\d+,\d+,\d+\s+(?:script|warp|shop|duplicate)') {
			$candidates.Add($Matches[1])
		}
		foreach ($match in [regex]::Matches($line, '(?:warp|warpparty|areawarp|Go)\s*\(?\s*"([A-Za-z0-9_@]+)"')) {
			$candidates.Add($match.Groups[1].Value)
		}
		foreach ($map in $candidates) {
			$literalMapRefs++
			if (!$maps.Contains($map) -and $map -notin $ignoredDynamicMaps) {
				$relative = $file.FullName.Substring($repo.Length + 1)
				Fail "Missing custom-script map '$map' at ${relative}:$lineNumber"
			}
		}
	}
}
Write-Host "  literal custom map references: $literalMapRefs"

# High-risk episode arrival cells must be inside the map and walkable. These
# are player destinations, not decorative NPC centres (which may be blocked).
$requiredCells = @(
	@('bl_ice',36,84), @('bl_lava',163,17), @('bl_grass',157,19),
	@('bl_death',315,62), @('bl_soul',155,15), @('bl_venom',146,22),
	@('bl_temple',53,85), @('bl_depth1',251,107), @('bl_depth1',251,251),
	@('bl_depth1',283,180), @('bl_depth1',177,289), @('bl_depth1',107,253),
	@('bl_depth1',108,109), @('bl_depth1',73,178), @('bl_depth2',190,64),
	@('veledor',136,62), @('kindlbrk',115,215), @('mu_fild02',136,65),
	@('mu_fild03',136,65), @('mu_dun01',95,131), @('mu_dun02',95,131),
	@('deadroot',136,62), @('ragsruth',88,58), @('rgs_dun1',95,131),
	@('ch2safe4',86,142), @('uknw_ruin2',33,246),
	@('1@ch2a',86,146), @('1@ch2a',86,157), @('1@ch2a',86,136),
	@('1@ch2b',86,146), @('1@ch2b',86,157), @('1@ch2b',86,136),
	@('jor_crk',105,108), @('jor_crk_p',105,108), @('luna_sf1',258,151),
	@('jor_mbase',54,155), @('jor_mbase',313,106), @('mbase_in',289,124),
	@('jor_albe',192,209), @('luna_sf2',187,254)
)
$wantedCacheMaps = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
foreach ($cell in $requiredCells) { [void]$wantedCacheMaps.Add($cell[0]) }
$cacheEntries = Read-MapCacheEntries @('db/import/map_cache.dat', 'db/re/map_cache.dat', 'db/map_cache.dat') $wantedCacheMaps
foreach ($cell in $requiredCells) {
	$mapName = $cell[0]
	if (!$cacheEntries.ContainsKey($mapName)) {
		Fail "Required episode map '$mapName' is absent from all map caches"
		continue
	}
	if (!(Test-WalkableCell $cacheEntries[$mapName] $cell[1] $cell[2])) {
		Fail "Required episode arrival cell $mapName,$($cell[1]),$($cell[2]) is blocked or out of bounds"
	}
}
foreach ($mapName in @('1@ch2a','1@ch2b')) {
	if (!$cacheEntries.ContainsKey($mapName)) { continue }
	foreach ($destination in @(@(86,157), @(86,136))) {
		if (!(Test-ConnectedCells $cacheEntries[$mapName] 86 146 $destination[0] $destination[1])) {
			Fail "Chapter 2 instance route on '$mapName' is disconnected: 86,146 -> $($destination[0]),$($destination[1])"
		}
	}
}
Write-Host "  walkable episode arrival cells: $($requiredCells.Count)"

# All direct numeric quest calls in custom scripts must resolve. Arithmetic
# expressions (for example 9283 + round) are validated through their results,
# not mistaken for a direct ID.
$questIds = Get-DatabaseIds @('db/re', 'db/import') '*quest_db.yml'
$questRefs = 0
foreach ($file in $customFiles) {
	$lineNumber = 0
	foreach ($line in [IO.File]::ReadLines($file.FullName)) {
		$lineNumber++
		foreach ($match in [regex]::Matches($line, '(?:setquest|completequest|erasequest|checkquest|isbegin_quest)\s*\(?\s*(\d+)')) {
			$tail = $line.Substring($match.Index + $match.Length)
			if ($tail -match '^\s*[+-]') { continue }
			$questRefs++
			$id = [int]$match.Groups[1].Value
			if (!$questIds.Contains($id)) {
				$relative = $file.FullName.Substring($repo.Length + 1)
				Fail "Missing quest ID $id at ${relative}:$lineNumber"
			}
		}
		foreach ($match in [regex]::Matches($line, 'changequest\s+(\d+)\s*,\s*(\d+)')) {
			foreach ($groupIndex in 1,2) {
				$questRefs++
				$id = [int]$match.Groups[$groupIndex].Value
				if (!$questIds.Contains($id)) {
					$relative = $file.FullName.Substring($repo.Length + 1)
					Fail "Missing quest ID $id at ${relative}:$lineNumber"
				}
			}
		}
	}
}
Write-Host "  direct custom quest references: $questRefs"

# Biosphere's current-client and reserved custom quest IDs are contractual:
# missing even one produces a blank quest or a non-functional daily mission.
$requiredBiosphereQuests = @((16734..16771) + (16778..16801) + (17607..17615) + (900100..900107))
foreach ($id in $requiredBiosphereQuests) {
	if (!$questIds.Contains($id)) { Fail "Missing Biosphere quest ID $id" }
}
$reputationText = [IO.File]::ReadAllText((RepoPath 'db/import/reputation.yml'))
$reputationGroupText = [IO.File]::ReadAllText((RepoPath 'db/import/reputation_group.yml'))
$constantText = [IO.File]::ReadAllText((RepoPath 'db/import/const.yml'))
foreach ($entry in @(
	@('Id: 6','Depth 1 reputation ID'), @('Variable: RepPoints6','Depth 1 reputation variable'),
	@('Id: 9','Depth 2 reputation ID'), @('Variable: RepPoints9','Depth 2 reputation variable'),
	@('Minimum: -5000','client negative reputation bound'), @('Maximum: 5000','client positive reputation bound')
)) {
	if (!$reputationText.Contains($entry[0])) { Fail "Missing $($entry[1]) in db/import/reputation.yml" }
}
foreach ($literal in @('Id: 4','ScriptName: BioSphere','- 6','- 9')) {
	if (!$reputationGroupText.Contains($literal)) { Fail "Incomplete Biosphere reputation group: '$literal'" }
}
foreach ($literal in @('REPUTATION_BIOSPHERE_DEPTH1','REPUTATION_BIOSPHERE_DEPTH2')) {
	if (!$constantText.Contains($literal)) { Fail "Missing script constant $literal" }
}
Write-Host "  required Biosphere quests: $($requiredBiosphereQuests.Count)"

# The custom quest range must also be reproducible on the client. Stock quest
# records 16739-16770 are overridden because MuhRO separates samples and hunts.
$biosphereClientPatch = [IO.File]::ReadAllText((RepoPath 'client-patch/biosphere/OngoingQuests_Biosphere.lua'))
foreach ($id in 900100..900107) {
	if ($biosphereClientPatch -notmatch "(?<!\d)$id(?!\d)") { Fail "Biosphere client patch is missing custom quest ID $id" }
}
foreach ($literal in @('16739, 16740, 16741, 16742','ba_in01,252,353','BEGIN RATHENA_PN BIOSPHERE QUEST PATCH')) {
	if (!$biosphereClientPatch.Contains($literal)) { Fail "Incomplete Biosphere client quest patch: '$literal'" }
}
Write-Host '  Biosphere custom client quests: 8'

# Crown group 132 is disabled in the base database. The import provides the
# exact supported rolls and deterministic upgrades, including 2026 POW/CON,
# Fierce Attack, and Great Craftsman additions.
$biosphereEnchant = [IO.File]::ReadAllText((RepoPath 'db/import/item_enchant.yml'))
foreach ($literal in @(
	'BEGIN CODEX BIOSPHERE CROWN ENCHANT 132','- Id: 132','Time_DM_R_Crown_DK: true',
	'T_D_Jewel_POW_1','T_D_Jewel_CON_3','Fierce_A_Jewel_1','Great_C_Jewel_1',
	'Upgrade: Fierce_A_Jewel_5','Upgrade: Great_C_Jewel_5'
)) {
	if (!$biosphereEnchant.Contains($literal)) { Fail "Incomplete Biosphere crown enchant group: '$literal'" }
}
Write-Host '  Biosphere crown enchant group: 132'

# Pin the public kRO/Divine Pride drop tables used by all 26 Depth monsters.
$depthMobText = [IO.File]::ReadAllText((RepoPath 'db/import/biosphere_mob_db.yml'))
$depth1Families = @{
	22143=@('Bar_D_Fl_Energy','Bar_D_Fl_Crystal','Flame_Barmund_Rune','Flame_Barmund_Rune2','Barmund_Flame_Essence','Bar_D_Fl_Specimen')
	22147=@('Bar_D_Fl_Energy','Bar_D_Fl_Crystal','Flame_Barmund_Rune','Flame_Barmund_Rune2','Barmund_Flame_Essence','Bar_D_Fl_Specimen')
	22140=@('Bar_D_Ic_Energy','Bar_D_Ic_Crystal','Ice_Barmund_Rune','Ice_Barmund_Rune2','Barmund_Ice_Essence','Bar_D_Ic_Specimen')
	22150=@('Bar_D_Ic_Energy','Bar_D_Ic_Crystal','Ice_Barmund_Rune','Ice_Barmund_Rune2','Barmund_Ice_Essence','Bar_D_Ic_Specimen')
	22145=@('Bar_D_Ea_Energy','Bar_D_Ea_Crystal','Plain_Barmund_Rune','Plain_Barmund_Rune2','Barmund_Plain_Essence','Bar_D_Ea_Specimen')
	22155=@('Bar_D_Ea_Energy','Bar_D_Ea_Crystal','Plain_Barmund_Rune','Plain_Barmund_Rune2','Barmund_Plain_Essence','Bar_D_Ea_Specimen')
	22142=@('Bar_D_St_Energy','Bar_D_St_Crystal','Plain_Barmund_Rune','Plain_Barmund_Rune2','Barmund_Plain_Essence','Bar_D_St_Specimen')
	22144=@('Bar_D_St_Energy','Bar_D_St_Crystal','Plain_Barmund_Rune','Plain_Barmund_Rune2','Barmund_Plain_Essence','Bar_D_St_Specimen')
	22146=@('Bar_D_So_Energy','Bar_D_So_Crystal','Soul_Barmund_Rune','Soul_Barmund_Rune2','Barmund_Soul_Essence','Bar_D_So_Specimen')
	22154=@('Bar_D_So_Energy','Bar_D_So_Crystal','Soul_Barmund_Rune','Soul_Barmund_Rune2','Barmund_Soul_Essence','Bar_D_So_Specimen')
	22148=@('Bar_D_Pu_Energy','Bar_D_Pu_Crystal','Temple_Barmund_Rune','Temple_Barmund_Rune2','Barmund_Temple_Essence','Bar_D_Pu_Specimen')
	22149=@('Bar_D_Pu_Energy','Bar_D_Pu_Crystal','Temple_Barmund_Rune','Temple_Barmund_Rune2','Barmund_Temple_Essence','Bar_D_Pu_Specimen')
	22141=@('Bar_D_Co_Energy','Bar_D_Co_Crystal','Death_Barmund_Rune','Death_Barmund_Rune2','Barmund_Death_Essence','Bar_D_Co_Specimen')
	22151=@('Bar_D_Co_Energy','Bar_D_Co_Crystal','Death_Barmund_Rune','Death_Barmund_Rune2','Barmund_Death_Essence','Bar_D_Co_Specimen')
	22152=@('Bar_D_Po_Energy','Bar_D_Po_Crystal','Venom_Barmund_Rune','Venom_Barmund_Rune2','Barmund_Venom_Essence','Bar_D_Po_Specimen')
	22153=@('Bar_D_Po_Energy','Bar_D_Po_Crystal','Venom_Barmund_Rune','Venom_Barmund_Rune2','Barmund_Venom_Essence','Bar_D_Po_Specimen')
}
$depth1Rates = @(500,50,80,20,30,2500)
foreach ($id in $depth1Families.Keys) {
	$match = [regex]::Match($depthMobText, "(?ms)^\s*- Id: $id\s*`r?`n(?<body>.*?)(?=^\s*- Id:|\z)")
	if (!$match.Success) { Fail "Missing Depth 1 monster $id"; continue }
	$body = $match.Groups['body'].Value
	if ($body -notmatch '(?ms)Item:\s*Etel_Dust\s*\r?\n\s*Rate:\s*150\b') { Fail "Depth 1 monster $id is missing Etel Dust at rate 150" }
	for ($i = 0; $i -lt $depth1Families[$id].Count; ++$i) {
		$item = [regex]::Escape($depth1Families[$id][$i])
		$rate = $depth1Rates[$i]
		if ($body -notmatch "(?ms)Item:\s*$item\s*`r?`n\s*Rate:\s*$rate\b") {
			Fail "Depth 1 monster $id is missing $($depth1Families[$id][$i]) at rate $rate"
		}
	}
}
$depth2Common = @(
	@('Etel_Dust',200), @('Abyss_Jewel_Fragment',350), @('Time_Dim_J_Fragment',350),
	@('Abyss_Magic_Jewel',75), @('Time_Dimension_Jewel',75), @('Abyss_Rune_Ore',25)
)
foreach ($id in 22252..22261) {
	$match = [regex]::Match($depthMobText, "(?ms)^\s*- Id: $id\s*`r?`n(?<body>.*?)(?=^\s*- Id:|\z)")
	if (!$match.Success) { Fail "Missing Depth 2 monster $id"; continue }
	$body = $match.Groups['body'].Value
	foreach ($drop in $depth2Common) {
		$item = [regex]::Escape($drop[0]); $rate = $drop[1]
		if ($body -notmatch "(?ms)Item:\s*$item\s*`r?`n\s*Rate:\s*$rate\b") {
			Fail "Depth 2 monster $id is missing $($drop[0]) at rate $rate"
		}
	}
}
Write-Host '  canonical Biosphere Depth drop tables: 26'

# Custom server items must have a reproducible client metadata entry.
$clientPairs = @(
	@('db/import/zero_cell_item_db.yml', 'client-patch/zero_cell/SystemEN/itemInfo_ZeroCell.lua'),
	@('db/import/chapter2_item_db.yml', 'client-patch/chapter2/SystemEN/itemInfo_Chapter2.lua'),
	@('db/import/fashion_points_box_item_db.yml', 'client-patch/fashion_points/SystemEN/LuaFiles514/itemInfo_fashion_points.lua'),
	@('db/import/fashion_points_missing_item_db.yml', 'client-patch/fashion_points/SystemEN/LuaFiles514/itemInfo_fashion_points.lua')
)
$clientItemCount = 0
foreach ($pair in $clientPairs) {
	$serverIds = [Collections.Generic.HashSet[int]]::new()
	foreach ($line in [IO.File]::ReadLines((RepoPath $pair[0]))) {
		if ($line -match '^\s*- Id:\s*(\d+)\s*$') { [void]$serverIds.Add([int]$Matches[1]) }
	}
	$metadata = [IO.File]::ReadAllText((RepoPath $pair[1]))
	foreach ($id in $serverIds) {
		$clientItemCount++
		$metadataPattern = if ($pair[1] -like '*fashion_points*') { "(?<!\d)$id(?!\d)" } else { "\[$id\]\s*=" }
		if ($metadata -notmatch $metadataPattern) {
			Fail "Client metadata is missing item ID $id ($($pair[1]))"
		}
	}
}
Write-Host "  custom items with client metadata: $clientItemCount"

# Required destinations that motivated the episode fixes must remain present in
# the player warper. This checks literal destinations, not only menu captions.
$warper = [IO.File]::ReadAllText((RepoPath 'npc/custom/warper.txt'))
$requiredWarps = @(
	'bl_ice','bl_lava','bl_grass','bl_death','bl_soul','bl_venom','bl_temple',
	'bl_depth1','ba_chess','bl_depth2','ch1zero1','ch1zero2','ch1zero3','ch1zero4',
	'veledor','kindlbrk','mu_fild02','mu_fild03','mu_dun01','mu_dun02','deadroot','ragsruth','rgs_dun1'
)
foreach ($map in $requiredWarps) {
	if ($warper -notmatch ('"' + [regex]::Escape($map) + '"')) { Fail "Warper is missing required destination '$map'" }
}
Write-Host "  required episode warps: $($requiredWarps.Count)"

# Preserve the exact route fixes for previously blocked landing cells and the
# Chapter 2 story targets that actually contain the requested monsters.
$routeAssertions = @(
	@('npc/custom/varmundt_biosphere.txt', 'warp "bl_ice",36,84;', 'Biosphere Ice entrance'),
	@('npc/custom/varmundt_biosphere.txt', 'warp "bl_lava",163,17;', 'Biosphere Fire entrance'),
	@('npc/custom/varmundt_biosphere.txt', 'warp "bl_grass",157,19;', 'Biosphere Grass entrance'),
	@('npc/custom/varmundt_biosphere.txt', 'warp "bl_death",315,62;', 'Biosphere Death entrance'),
	@('npc/custom/warper.txt', '36,84,163,17,157,19,315,62,155,15,146,22,53,85', 'warper Biosphere destination order'),
	@('npc/custom/chapter2/Chapter2.txt', 'warp "uknw_ruin2",33,246;', 'Chapter 2 Shadow Jailer objective'),
	@('npc/custom/chapter2/Instances.txt', 'warp "ch2safe4",86,142;', 'Phantom Gate first-clear exit'),
	@('npc/custom/varmundt_biosphere.txt', 'ep17_2_main < 33', 'Episode 17.2 completion gate'),
	@('npc/custom/varmundt_biosphere.txt', 'get_reputation_points(REPUTATION_BIOSPHERE_DEPTH1) < 2000', 'Depth 2 reputation gate'),
	@('npc/custom/varmundt_biosphere.txt', 'callfunc "F_BiosphereDepth2Purge";', 'Depth 2 entry buff purge'),
	@('npc/custom/varmundt_biosphere.txt', 'Biosphere Relog Guard#depth', 'Depth relog escape guard'),
	@('npc/custom/warper.txt', 'callfunc("F_BiosphereDepth2Access")', 'warper Depth 2 access gate'),
	@('npc/custom/warper.txt', 'Go("alberta",214,74);', 'warper Sunken Tower entrance'),
	@('npc/custom/warper.txt', 'Go("dali02",79,60);', 'warper Geffen Night Arena entrance'),
	@('npc/custom/warper.txt', 'Go("t_garden",159,235);', 'warper Lake of Fire entrance'),
	@('npc/custom/warper.txt', 'Go("t_garden",172,235);', 'warper Hall of Life entrance'),
	@('npc/custom/warper.txt', 'Go("dali",124,88);', 'warper Tomb of Remorse entrance'),
	@('npc/custom/warper.txt', 'Go("e_tower",83,105);', 'warper Constellation Tower entrance'),
	@('npc/custom/warper.txt', 'Go("dali02",137,86);', 'warper Airship Crash entrance'),
	@('npc/custom/episode21/Progression.txt', 'isbegin_quest(18233) == 2', 'Episode 21 Episode 20 prerequisite'),
	@('npc/custom/episode21/Progression.txt', 'jor_tail,233,41,4', 'Episode 21 Shufapa entry'),
	@('npc/custom/warper.txt', 'callfunc("EP21_InstanceWarperAccess",21)', 'Episode 21 instance warper gate'),
	@('npc/custom/warper.txt', 'Go("luna_sf2",187,254);', 'Gimli guarded entrance'),
	@('db/import/quest_db.yml', 'regex:Location: jor_raise1\r?\n    MapName: Northern Raised Land', 'Episode 21 northern raised-land quest target'),
	@('db/import/quest_db.yml', 'regex:Location: jor_raise2\r?\n    MapName: Southern Raised Land', 'Episode 21 southern raised-land quest target'),
	@('db/import/mob_skill_db.txt', '22360,EP21_YORTUS_A@AL_TELEPORT', 'Episode 21 Jortus Bishop mob-skill owner'),
	@('npc/custom/fashion_points/FashionPoints.txt', "function`tscript`tFP_OpenBox", 'Fashion box opening function'),
	@('npc/custom/fashion_points/FashionEnchant.txt', 'Complete Fashion Enchanter#FP', 'complete Fashion enchant service'),
	@('npc/custom/fashion_points/FashionPoints.txt', 'Rental costumes cannot use recovery', 'Fashion recovery rental guard'),
	@('db/import/status.yml', 'Status: Mtp_W_Potion_100', 'Rgan transformation healing status'),
	@('db/skill_db.yml', 'db/import/garden_of_time_combat_skill_db.yml', 'Garden combat skill loader'),
	@('db/status.yml', 'db/import/garden_of_time_combat_status.yml', 'Garden combat status loader'),
	@('db/item_db.yml', 'db/import/garden_of_time_combat_item_db.yml', 'Garden counter-item loader'),
	@('db/mob_db.yml', 'db/import/garden_of_time_combat_mob_db.yml', 'Garden monster overlay loader'),
	@('npc/custom/varmundt_biosphere_quests.txt', 'ba_in01,252,353,4', 'regular Biosphere daily manager coordinate'),
	@('npc/custom/varmundt_biosphere_quests.txt', 'ba_in01,365,50,4', 'regular Biosphere material exchange coordinate'),
	@('npc/custom/varmundt_biosphere_quests.txt', 'ba_in01,359,53,4', 'regular Biosphere equipment exchange coordinate'),
	@('npc/custom/varmundt_biosphere_quests.txt', 'setarray .@group[1],16,17,18,19,57,58,59;', 'regular Biosphere native enchant groups'),
	@('npc/custom/varmundt_biosphere_quests.txt', 'setarray .@group[1],52,53,54,55,60,61,62;', 'reformed Biosphere native enchant groups'),
	@('npc/custom/varmundt_biosphere_quests.txt', '101769,101771,101772,101770,101933,101934,101935,101936,101937,101938', 'regular Biosphere reform interfaces'),
	@('npc/custom/varmundt_biosphere_depth.txt', 'item_enchant 98 + .@i;', 'Depth armor native enchant UI'),
	@('npc/custom/varmundt_biosphere_depth.txt', 'getitembound 102373,1,BOUND_ACCOUNT;', 'Depth armor engraving catalyst'),
	@('npc/custom/varmundt_biosphere_depth.txt', '16779,900106,16781', 'Depth 2 450/3000/species missions'),
	@('npc/custom/varmundt_biosphere_depth.txt', '.@reward = 102717;', 'Depth 2 3000-kill reward'),
	@('npc/custom/varmundt_biosphere_depth.txt', 'item_enchant 133;', 'Dimension weapon native enchant UI'),
	@('npc/custom/varmundt_biosphere_depth.txt', 'item_enchant 132;', 'Time Dimensions crown native enchant UI'),
	@('npc/custom/varmundt_biosphere_depth.txt', '.@result = 400529 + .@crown;', 'all legacy Time Dimensions crown recipes'),
	@('npc/custom/varmundt_biosphere_depth.txt', '80,65,50,35,25,20,10,7,5', 'crown random-upgrade success table'),
	@('npc/custom/varmundt_biosphere_depth.txt', '5,10,20,35,55,80,110,145,185', 'crown random-upgrade material table'),
	@('npc/custom/varmundt_biosphere_depth.txt', 'Rerolling only slot 4 costs 180 Abyss Magic Runes', 'crown stat-line reroll')
)
foreach ($assertion in $routeAssertions) {
	$content = [IO.File]::ReadAllText((RepoPath $assertion[0]))
	if ($assertion[1].StartsWith('regex:')) {
		if ($content -notmatch $assertion[1].Substring(6)) { Fail "Missing route regression guard: $($assertion[2]) ($($assertion[0]))" }
	} elseif (!$content.Contains($assertion[1])) {
		Fail "Missing route regression guard: $($assertion[2]) ($($assertion[0]))"
	}
}
Write-Host "  route regression assertions: $($routeAssertions.Count)"

# Legacy CSV mob skills cannot import a second TXT file. Ensure the maintained
# Garden source body is represented exactly once by the final canonical block.
$gardenSkillRows = @([IO.File]::ReadAllLines((RepoPath 'db/import/garden_of_time_combat_mob_skill_db.txt')) |
	Where-Object { $_ -and !$_.StartsWith('//') })
$gardenDuplicates = @($gardenSkillRows | Group-Object | Where-Object Count -gt 1)
foreach ($duplicate in $gardenDuplicates) { Fail "Duplicate Garden mob-skill row: $($duplicate.Name)" }
foreach ($row in $gardenSkillRows) {
	if (($row -split ',',-1).Count -ne 19) { Fail "Garden mob-skill row does not have 19 fields: $row" }
}
$canonicalMobSkills = [IO.File]::ReadAllText((RepoPath 'db/import/mob_skill_db.txt'))
$gardenBlock = [regex]::Match($canonicalMobSkills, '(?ms)^// BEGIN CODEX GARDEN OF TIME COMBAT OVERLAY\r?\n.*?^// END CODEX GARDEN OF TIME COMBAT OVERLAY\s*$')
if (!$gardenBlock.Success) {
	Fail 'Canonical mob-skill DB is missing the Garden clear/rebuild block'
} else {
	$canonicalGardenRows = @($gardenBlock.Value -split '\r?\n' | Where-Object { $_ -and !$_.StartsWith('//') })
	if (($canonicalGardenRows -join "`n") -cne ($gardenSkillRows -join "`n")) {
		Fail 'Canonical Garden mob-skill block differs from its maintained source file'
	}
}
Write-Host "  Garden combat mob-skill rows: $($gardenSkillRows.Count)"

# Instance-only maps must carry the normal anti-warp/branch and party-lock
# protections even while their full NPC implementations remain under audit.
$gardenMapflags = @(
	@('npc/re/mapflag/nobranch.txt', 'nobranch'),
	@('npc/re/mapflag/nomemo.txt', 'nomemo'),
	@('npc/re/mapflag/nowarpto.txt', 'nowarpto'),
	@('npc/re/mapflag/noteleport.txt', 'noteleport'),
	@('npc/re/mapflag/noteleport.txt', 'monster_noteleport'),
	@('npc/re/mapflag/partylock.txt', 'partylock'),
	@('npc/re/mapflag/restricted.txt', "restricted`t6")
)
foreach ($mapName in @('1@f_lake','1@ba_go')) {
	foreach ($flag in $gardenMapflags) {
		$content = [IO.File]::ReadAllText((RepoPath $flag[0]))
		if (!$content.Contains("$mapName`tmapflag`t$($flag[1])")) {
			Fail "Missing memorial-dungeon mapflag '$($flag[1])' for '$mapName' ($($flag[0]))"
		}
	}
}
Write-Host '  protected Garden instance maps: 2'

# A definition absent from all NPC scripts can be created, but players have no
# script path into it. Keep these visible until their upstream content exists.
$npcText = (@(Get-ChildItem -LiteralPath (RepoPath 'npc') -Recurse -File -Include '*.txt','*.c') |
	ForEach-Object { [IO.File]::ReadAllText($_.FullName) }) -join "`n"
$unreferenced = @($instances | Where-Object { $npcText.IndexOf('"' + $_.Name + '"', [StringComparison]::Ordinal) -lt 0 })
if ($unreferenced.Count) {
	$message = "$($unreferenced.Count) instance definitions have no literal NPC-script reference: " + (($unreferenced.Name | Sort-Object) -join ', ')
	if ($StrictContent) { Fail $message } else { Warn $message }
}

foreach ($warning in $warnings) { Write-Warning $warning }
if ($failures.Count) {
	foreach ($failure in $failures) { Write-Error $failure }
	Write-Host "FAILED: $($failures.Count) integrity error(s), $($warnings.Count) warning(s)."
	exit 1
}

Write-Host "PASS: no integrity errors; $($warnings.Count) content-completeness warning(s)."
