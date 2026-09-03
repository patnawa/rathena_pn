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
	'db/item_randomopt_db.yml', 'db/item_randomopt_group.yml'
)
$dbImports = [Collections.Generic.List[string]]::new()
$optionalImports = @(
	'db/import/item_db.yml', 'db/import/item_group_db.yml', 'db/import/mob_db.yml',
	'db/import/quest_db.yml', 'db/import/map_drops.yml',
	'db/import/item_randomopt_db.yml', 'db/import/item_randomopt_group.yml'
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

# Custom server items must have a reproducible client metadata entry.
$clientPairs = @(
	@('db/import/zero_cell_item_db.yml', 'client-patch/zero_cell/SystemEN/itemInfo_ZeroCell.lua'),
	@('db/import/chapter2_item_db.yml', 'client-patch/chapter2/SystemEN/itemInfo_Chapter2.lua')
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
		if ($metadata -notmatch "\[$id\]\s*=") {
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
