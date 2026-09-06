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
function Read-ScriptCode([string]$Path) {
	$content = [IO.File]::ReadAllText($Path)
	# Preserve newlines so diagnostics still report the source line number.
	$content = [regex]::Replace($content, '(?ms)/\*.*?\*/', {
		param($match)
		return [regex]::Replace($match.Value, '[^\r\n]', ' ')
	})
	return [regex]::Replace($content, '(?m)//.*$', '')
}

function Read-InstanceEntries([string[]]$Files) {
	$entries = [Collections.Generic.List[object]]::new()
	foreach ($relative in $Files) {
		$path = RepoPath $relative
		$current = $null
		foreach ($line in [IO.File]::ReadLines($path)) {
			if ($line -match '^\s*- Id:\s*(\d+)\s*(?:#.*)?$') {
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

function Read-MapCacheNames([string[]]$Files) {
	$names = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
	foreach ($relative in $Files) {
		$path = RepoPath $relative
		if (!(Test-Path -LiteralPath $path -PathType Leaf)) { continue }
		$stream = [IO.File]::OpenRead($path)
		$reader = [IO.BinaryReader]::new($stream)
		try {
			[void]$reader.ReadUInt32()
			$mapCount = $reader.ReadUInt16()
			[void]$reader.ReadUInt16()
			for ($index = 0; $index -lt $mapCount; ++$index) {
				$name = [Text.Encoding]::ASCII.GetString($reader.ReadBytes(12)).Trim([char]0)
				[void]$names.Add($name)
				[void]$reader.ReadInt16()
				[void]$reader.ReadInt16()
				$length = $reader.ReadInt32()
				if ($length -lt 6 -or $reader.BaseStream.Position + $length -gt $reader.BaseStream.Length) {
					throw "Invalid compressed record for map '$name' in $relative"
				}
				[void]$reader.BaseStream.Seek($length, [IO.SeekOrigin]::Current)
			}
		}
		finally { $reader.Dispose(); $stream.Dispose() }
	}
	return $names
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
				if ($line -match '^\s*- Id:\s*(\d+)\s*(?:#.*)?$') { [void]$ids.Add([int]$Matches[1]) }
			}
		}
	}
	return $ids
}

Write-Host 'Episode integrity audit'

# Resolve the same recursive NPC configuration tree used by a Renewal map
# server, then require every enabled script to exist.
$enabledScripts = [Collections.Generic.List[string]]::new()
$enabledScriptEntries = [Collections.Generic.List[object]]::new()
$visitedNpcConfigs = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
function Read-NpcConfig([string]$Config) {
	if (!$visitedNpcConfigs.Add($Config)) { return }
	$configPath = RepoPath $Config
	if (!(Test-Path -LiteralPath $configPath -PathType Leaf)) {
		Fail "Missing NPC configuration: $Config"
		return
	}
	$lineNumber = 0
	foreach ($line in [IO.File]::ReadLines($configPath)) {
		$lineNumber++
		if ($line -match '^\s*import:\s*(\S.*?)\s*$') {
			Read-NpcConfig $Matches[1]
			continue
		}
		if ($line -match '^\s*npc:\s*(\S.*?)\s*$') {
			$scriptPath = [IO.Path]::GetFullPath((RepoPath $Matches[1]))
			if (!$scriptPath.StartsWith($repo + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
				Fail "Enabled NPC script escapes the repository: $($Matches[1]) (${Config}:$lineNumber)"
				continue
			}
			$script = $scriptPath.Substring($repo.Length + 1).Replace('\','/')
			$enabledScripts.Add($script)
			$enabledScriptEntries.Add([pscustomobject]@{ Path = $script; Config = $Config; Line = $lineNumber })
			if (!(Test-Path -LiteralPath $scriptPath -PathType Leaf)) {
				Fail "Missing enabled NPC script: $script ($Config)"
			}
		}
	}
}
Read-NpcConfig 'npc/re/scripts_main.conf'
foreach ($group in $enabledScriptEntries | Group-Object Path | Where-Object Count -gt 1) {
	$origins = @($group.Group | ForEach-Object { "$($_.Config):$($_.Line)" }) -join ', '
	Fail "NPC script is enabled more than once: $($group.Name) ($origins)"
}
$enabledScripts = @($enabledScriptEntries | Group-Object Path | ForEach-Object { $_.Group[0].Path })
Write-Host "  unique enabled NPC scripts: $($enabledScripts.Count)"

# These persistent town services must not disappear during episode deployments.
foreach ($service in @('npc/custom/healer.txt', 'npc/custom/grademk_services.txt', 'npc/custom/episode_skip_tina.txt')) {
	if ($enabledScripts -notcontains $service) { Fail "Required custom service is disabled: $service" }
}

# Regression guards supplement, but do not replace, the compiled behavioral
# tests in tools/ci/inventory_enchant_test.cpp or an in-client service test.
$workshopService = [IO.File]::ReadAllText((RepoPath 'npc/custom/grademk_services.txt'))
foreach ($literal in @('modifyinventoryenchant(', '@inventorylist_uniqueid$', '.@unique_id$[.@pick]')) {
	if (!$workshopService.Contains($literal)) { Fail "Missing guarded workshop upgrade: '$literal'" }
}
if ($workshopService -match '\b(?:delitemidx|getitembound4)\b') {
	Fail 'Grade Workshop must not delete/recreate an existing enchanted item'
}
$ticketService = [IO.File]::ReadAllText((RepoPath 'npc/custom/episode_skip_tina.txt'))
if ($ticketService -notmatch 'checkweight\(\.@ticket,1\)[\s\S]*?set Zeny, Zeny - \.@price') {
	Fail 'Episode ticket purchase must check inventory capacity before charging'
}
if ($ticketService -notmatch 'case 7:\s+ep19_main = 100;\s+callfunc "F_CompleteEpisodeGate",17649;') {
	Fail 'Episode 19 ticket must synchronize both story completion representations'
}
foreach ($relative in @('npc/custom/episode21/GimliInfiltration.txt', 'npc/custom/episode21/MysteriousGhostShip.txt')) {
	$service = [IO.File]::ReadAllText((RepoPath $relative))
	if ($service -notmatch 'return EP21_(?:Gimli_Complete|GhostShip_Access)[^;]+callfunc\("EP21_MainComplete"\);') {
		Fail "Episode 21 story-clear compatibility is missing: $relative"
	}
}

# Every imported database file must exist.
$dbRoots = @(
	'db/item_db.yml', 'db/item_group_db.yml', 'db/mob_db.yml',
	'db/quest_db.yml', 'db/instance_db.yml', 'db/map_drops.yml',
	'db/item_randomopt_db.yml', 'db/item_randomopt_group.yml',
	'db/reputation.yml', 'db/reputation_group.yml', 'db/const.yml',
	'db/item_enchant.yml', 'db/item_reform.yml', 'db/item_combos.yml',
	'db/laphine_synthesis.yml', 'db/laphine_upgrade.yml',
	'db/skill_db.yml', 'db/status.yml', 'db/mercenary_db.yml',
	'db/pet_db.yml',
	'npc/custom/barters.yml'
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

# rAthena ignores db/import by default because `make import` generates empty
# templates there.  Custom episode fragments are different: a fresh clone or
# deployment archive must contain them.  Catch the exact packaging failure
# that otherwise appears only as missing DBs and unknown mobs at map startup.
$requiredPackagedImports = @(
	'db/import/episode20_instance_db.yml',
	'db/import/episode20_mob_db.yml',
	'db/import/episode20_mob_skill_db.txt',
	'db/import/episode20_quest_db.yml',
	'db/import/legacy_compat_quest_db.yml',
	'db/import/thanatos_mob_db.yml',
	'db/import/thanatos_mob_skill_db.txt',
	'db/import/thanatos_quest_db.yml'
)
$gitIgnoreLines = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
foreach ($line in [IO.File]::ReadLines((RepoPath '.gitignore'))) { [void]$gitIgnoreLines.Add($line.Trim()) }
foreach ($relative in $requiredPackagedImports) {
	if (!(Test-Path -LiteralPath (RepoPath $relative) -PathType Leaf)) {
		Fail "Missing packaged episode database: $relative"
		continue
	}
	if (!$gitIgnoreLines.Contains("!/$relative")) {
		Fail "Required episode database is not exempted from .gitignore: $relative"
	}
}
Write-Host "  packaged episode database fragments: $($requiredPackagedImports.Count)"

# An import fragment can be perfectly valid YAML yet remain completely inert
# when its root database never references it. Require every runtime database
# fragment under db/import to be wired. The Geffen YAML file is explicitly a
# provenance manifest for legacy TXT mob skills and is not a loadable DB type.
$normalizedDbImports = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
foreach ($import in $dbImports) { [void]$normalizedDbImports.Add($import.Replace('\','/')) }
$runtimeImportFragments = 0
foreach ($file in Get-ChildItem -LiteralPath (RepoPath 'db/import') -Filter '*.yml' -File) {
	$type = $null
	$bodyLine = -1
	$lines = [IO.File]::ReadAllLines($file.FullName)
	for ($lineIndex = 0; $lineIndex -lt $lines.Length; $lineIndex++) {
		$line = $lines[$lineIndex]
		if ($line -match '^\s*Type:\s*(\S+)\s*$') { $type = $Matches[1]; break }
	}
	if (!$type -or $type -eq 'MOB_SKILL_DB_MANIFEST') { continue }
	for ($lineIndex = 0; $lineIndex -lt $lines.Length; $lineIndex++) {
		if ($lines[$lineIndex] -match '^\s*Body:\s*$') { $bodyLine = $lineIndex; break }
	}
	# `make import` materializes header-only templates for every database type.
	# They are deliberately ignored and are not runtime fragments.  Count a
	# file only when Body has an actual non-comment record so a local build
	# cannot turn a clean integrity audit into dozens of false positives.
	if ($bodyLine -lt 0) { continue }
	$hasBodyRecord = $false
	for ($lineIndex = $bodyLine + 1; $lineIndex -lt $lines.Length; $lineIndex++) {
		$trimmed = $lines[$lineIndex].Trim()
		if ($trimmed -and !$trimmed.StartsWith('#')) { $hasBodyRecord = $true; break }
	}
	if (!$hasBodyRecord) { continue }
	++$runtimeImportFragments
	$relative = 'db/import/' + $file.Name
	if (!$normalizedDbImports.Contains($relative)) {
		Fail "Runtime database fragment is not imported: $relative ($type)"
	}
}
Write-Host "  wired runtime import fragments: $runtimeImportFragments"

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
		if ($line -notmatch '^(\s*)- Id:\s*(\d+)\s*(?:#.*)?$') { continue }
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
$cacheFiles = @('db/import/map_cache.dat', 'db/re/map_cache.dat', 'db/map_cache.dat')
$cacheMapNames = Read-MapCacheNames $cacheFiles
Write-Host "  unique cached maps: $($cacheMapNames.Count)"

# Instance IDs/names must be unique and every source/additional map must exist.
$instanceFiles = [Collections.Generic.List[string]]::new()
$instanceFiles.Add('db/re/instance_db.yml')
Get-ChildItem -LiteralPath (RepoPath 'db/import') -Filter '*instance_db.yml' -File | ForEach-Object {
	$instanceFiles.Add($_.FullName.Substring($repo.Length + 1).Replace('\','/'))
}
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

# Literal map destinations and declarations in every enabled runtime script
# must exist. Scanning only npc/custom lets broken official/episode scripts go
# unnoticed and lets disabled scratch files influence the result.
$enabledFiles = @($enabledScripts | ForEach-Object {
	$path = RepoPath $_
	if (Test-Path -LiteralPath $path -PathType Leaf) { Get-Item -LiteralPath $path }
})
$ignoredDynamicMaps = @('06guild_0', 'pvp_n_', 'SavePoint', 'Random', 'RandomAll')
$literalMapRefs = 0
$literalEnabledMaps = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
$literalDestinations = [Collections.Generic.List[object]]::new()
$literalDestinationKeys = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
$enabledScriptSet = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
foreach ($script in $enabledScripts) { [void]$enabledScriptSet.Add($script.Replace('\','/')) }
foreach ($file in $enabledFiles) {
	$relativeFile = $file.FullName.Substring($repo.Length + 1).Replace('\','/')
	$isEnabledFile = $enabledScriptSet.Contains($relativeFile)
	$lineNumber = 0
	foreach ($line in ((Read-ScriptCode $file.FullName) -split '\r?\n')) {
		$lineNumber++
		$candidates = [Collections.Generic.List[string]]::new()
		if ($line -match '^\s*([A-Za-z0-9_@]+),\d+,\d+,\d+\s+(?:script|warp|shop|duplicate)') {
			$candidates.Add($Matches[1])
		}
		foreach ($match in [regex]::Matches($line, '(?:warp|warpparty|areawarp|Go)\s*\(?\s*"([A-Za-z0-9_@]+)"')) {
			$tail = $line.Substring($match.Index + $match.Length)
			if ($tail -match '^\s*\+') { continue }
			$candidates.Add($match.Groups[1].Value)
		}
		foreach ($match in [regex]::Matches($line, '(?:warp|warpparty|Go)\s*\(?\s*"(?<map>[A-Za-z0-9_@]+)"\s*,\s*(?<x>-?\d+)\s*,\s*(?<y>-?\d+)')) {
			if (!$isEnabledFile -or !$relativeFile.StartsWith('npc/custom/', [StringComparison]::OrdinalIgnoreCase)) { continue }
			$map = $match.Groups['map'].Value
			$x = [int]$match.Groups['x'].Value
			$y = [int]$match.Groups['y'].Value
			$key = "$map,$x,$y"
			if ($literalDestinationKeys.Add($key)) {
				$literalDestinations.Add([pscustomobject]@{
					Map = $map; X = $x; Y = $y
					File = $relativeFile
					Line = $lineNumber
				})
			}
		}
		foreach ($map in $candidates) {
			$literalMapRefs++
			[void]$literalEnabledMaps.Add($map)
			if (!$maps.Contains($map) -and $map -notin $ignoredDynamicMaps) {
				$relative = $file.FullName.Substring($repo.Length + 1)
				Fail "Missing enabled-script map '$map' at ${relative}:$lineNumber"
			}
			if (!$cacheMapNames.Contains($map) -and $map -notin $ignoredDynamicMaps) {
				$relative = $file.FullName.Substring($repo.Length + 1)
				Fail "Enabled-script map '$map' has no map-cache entry at ${relative}:$lineNumber"
			}
		}
	}
}
Write-Host "  literal enabled map references: $literalMapRefs ($($literalEnabledMaps.Count) maps)"

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
# Standing cells in front of the restored workshop counter, Tina, and healer.
# The workshop counter blocks portions of row 183; row 181 is the approach aisle.
# These also guard against accidentally deploying an incompatible map cache.
$serviceCells = @(
	@('grademk',30,181), @('grademk',32,181), @('grademk',36,181),
	@('grademk',38,181), @('grademk',42,181), @('grademk',44,181),
	@('grademk',46,181), @('grademk',48,181), @('grademk',50,181),
	@('prontera',162,192), @('malangdo',132,113)
)
$requiredCells += $serviceCells
$wantedCacheMaps = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
foreach ($cell in $requiredCells) { [void]$wantedCacheMaps.Add($cell[0]) }
foreach ($destination in $literalDestinations) {
	if ($destination.Map -notin $ignoredDynamicMaps) { [void]$wantedCacheMaps.Add($destination.Map) }
}
$cacheEntries = Read-MapCacheEntries $cacheFiles $wantedCacheMaps
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
foreach ($destination in $literalDestinations) {
	if ($destination.Map -in $ignoredDynamicMaps -or $destination.X -le 0 -or $destination.Y -le 0) { continue }
	if (!$cacheEntries.ContainsKey($destination.Map)) {
		Fail "Literal warp destination map '$($destination.Map)' has no decodable cache entry ($($destination.File):$($destination.Line))"
		continue
	}
	if (!(Test-WalkableCell $cacheEntries[$destination.Map] $destination.X $destination.Y)) {
		Fail "Literal warp destination $($destination.Map),$($destination.X),$($destination.Y) is blocked or out of bounds ($($destination.File):$($destination.Line))"
	}
}
foreach ($cell in $serviceCells) {
	if ($cell[0] -ne 'grademk' -or !$cacheEntries.ContainsKey('grademk')) { continue }
	if (!(Test-ConnectedCells $cacheEntries['grademk'] 38 177 $cell[1] $cell[2])) {
		Fail "Restored workshop approach is disconnected: grademk,38,177 -> $($cell[1]),$($cell[2])"
	}
}
foreach ($mapName in @('1@ch2a','1@ch2b')) {
	if (!$cacheEntries.ContainsKey($mapName)) { continue }
	foreach ($destination in @(@(86,157), @(86,136), @(78,141), @(94,141), @(78,151), @(94,151))) {
		if (!(Test-ConnectedCells $cacheEntries[$mapName] 86 146 $destination[0] $destination[1])) {
			Fail "Chapter 2 instance route on '$mapName' is disconnected: 86,146 -> $($destination[0]),$($destination[1])"
		}
	}
}
Write-Host "  walkable episode arrival cells: $($requiredCells.Count)"
Write-Host "  unique literal warp destinations: $($literalDestinations.Count)"

# All direct numeric quest calls in enabled scripts must resolve. Arithmetic
# expressions (for example 9283 + round) are validated through their results,
# not mistaken for a direct ID.
$questIds = Get-DatabaseIds @('db/re', 'db/import') '*quest_db.yml'
$questRefs = 0
foreach ($file in $enabledFiles) {
	$lineNumber = 0
	foreach ($line in ((Read-ScriptCode $file.FullName) -split '\r?\n')) {
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
Write-Host "  direct enabled quest references: $questRefs"

# Some enabled scripts traverse quest IDs arithmetically, so a literal-only
# scan cannot prove those ranges exist. The Morroc reward migration walks every
# ID from 5251 through 5275, and Lasagna uses three omitted repeat-state IDs.
$requiredLegacyQuests = @((5251..5275) + 14564,14577,14578)
foreach ($id in $requiredLegacyQuests) {
	if (!$questIds.Contains($id)) { Fail "Missing required legacy compatibility quest ID $id" }
}
$legacyQuestText = [IO.File]::ReadAllText((RepoPath 'db/import/legacy_compat_quest_db.yml'))
$legacyMoroccTargets = @{
	5265 = 'MOROCC_1'
	5266 = 'MOROCC_2'
	5267 = 'MOROCC_3'
	5268 = 'MOROCC_4'
}
foreach ($entry in $legacyMoroccTargets.GetEnumerator()) {
	$record = [regex]::Match($legacyQuestText, "(?ms)^\s*- Id:\s*$($entry.Key)\s*`r?`n(?<body>.*?)(?=^\s*- Id:|\z)")
	if (!$record.Success -or $record.Groups['body'].Value -notmatch "(?m)^\s+- Mob:\s+$([regex]::Escape($entry.Value))\s*`r?$" -or $record.Groups['body'].Value -notmatch '(?m)^\s+Count:\s+10\s*\r?$') {
		Fail "Legacy quest $($entry.Key) does not hunt exactly 10 $($entry.Value)"
	}
}
Write-Host "  required legacy compatibility quests: $($requiredLegacyQuests.Count)"

# Literal item references in enabled scripts must resolve to an active renewal
# item record. Quoted decimal IDs are always a scripting mistake: rAthena
# interprets those as Aegis names, not numeric IDs.
$itemIds = [Collections.Generic.HashSet[int]]::new()
$itemNames = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
$itemNameById = [Collections.Generic.Dictionary[int,string]]::new()
$itemIdByName = [Collections.Generic.Dictionary[string,int]]::new([StringComparer]::OrdinalIgnoreCase)
foreach ($file in Get-ChildItem -LiteralPath (RepoPath 'db/re'),(RepoPath 'db/import') -File -Filter '*item_db*.yml') {
	$currentItemId = $null
	foreach ($line in [IO.File]::ReadLines($file.FullName)) {
		if ($line -match '^\s*- Id:\s*(\d+)\s*(?:#.*)?$') {
			$currentItemId = [int]$Matches[1]
			[void]$itemIds.Add($currentItemId)
			continue
		}
		if ($null -ne $currentItemId -and $line -match '^\s+AegisName:\s*([^\s#]+)') {
			$aegis = $Matches[1].Trim('"', "'")
			[void]$itemNames.Add($aegis)
			if ($itemNameById.ContainsKey($currentItemId) -and !$itemNameById[$currentItemId].Equals($aegis, [StringComparison]::OrdinalIgnoreCase)) {
				Fail "Item ID $currentItemId maps to conflicting Aegis names '$($itemNameById[$currentItemId])' and '$aegis'"
			} else {
				$itemNameById[$currentItemId] = $aegis
			}
			if ($itemIdByName.ContainsKey($aegis) -and $itemIdByName[$aegis] -ne $currentItemId) {
				Fail "Item AegisName '$aegis' maps to conflicting IDs $($itemIdByName[$aegis]) and $currentItemId"
			} else {
				$itemIdByName[$aegis] = $currentItemId
			}
		}
	}
}
Write-Host "  consistent item identities: $($itemNameById.Count) unique records"
$itemCommandPattern = '(?<![A-Za-z0-9_])(?:getitembound4|getitembound|getitem2|getitem|delitem2|delitem|countitem|checkweight)\b\s*\(?\s*(?<token>"[^"]+"|''[^'']+''|\d+)'
$itemRefs = 0
foreach ($file in $enabledFiles) {
	$relative = $file.FullName.Substring($repo.Length + 1).Replace('\','/')
	$lineNumber = 0
	foreach ($line in ((Read-ScriptCode $file.FullName) -split '\r?\n')) {
		$lineNumber++
		foreach ($match in [regex]::Matches($line, $itemCommandPattern)) {
			$itemRefs++
			$raw = $match.Groups['token'].Value
			$token = $raw.Trim('"', "'")
			if ($token -match '^\d+$') {
				if ($raw.StartsWith('"') -or $raw.StartsWith("'")) {
					Fail "Quoted numeric item ID $token at ${relative}:$lineNumber"
					continue
				}
				$id = [int]$token
				# Muh Coin is an optional server-specific item. The Fashion NPC
				# probes it with getiteminfo and disables that payment path if absent.
				if ($id -eq 50000 -and $relative -eq 'npc/custom/fashion_points/FashionPoints.txt') { continue }
				if (!$itemIds.Contains($id)) { Fail "Missing item ID $id at ${relative}:$lineNumber" }
				continue
			}
			if ($token -match '^[A-Za-z][A-Za-z0-9_]*$' -and !$itemNames.Contains($token)) {
				Fail "Missing item AegisName '$token' at ${relative}:$lineNumber"
			}
		}
	}
}
Write-Host "  literal enabled item references: $itemRefs"

# Validate literal monster identifiers used by runtime spawn commands. These
# failures otherwise appear only when a player reaches the affected encounter.
$mobIds = [Collections.Generic.HashSet[int]]::new()
$mobNames = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
$mobIdentities = [Collections.Generic.List[object]]::new()
foreach ($file in Get-ChildItem -LiteralPath (RepoPath 'db/re'),(RepoPath 'db/import') -File -Filter '*mob_db.yml') {
	$currentMobId = $null
	foreach ($line in [IO.File]::ReadLines($file.FullName)) {
		if ($line -match '^\s*- Id:\s*(\d+)\s*(?:#.*)?$') {
			$currentMobId = [int]$Matches[1]
			[void]$mobIds.Add($currentMobId)
			continue
		}
		if ($null -ne $currentMobId -and $line -match '^\s+AegisName:\s*([^\s#]+)') {
			$aegis = $Matches[1].Trim('"', "'")
			[void]$mobNames.Add($aegis)
			$mobIdentities.Add([pscustomobject]@{ Id = $currentMobId; Aegis = $aegis })
		}
	}
}
foreach ($group in $mobIdentities | Group-Object Id) {
	$names = @($group.Group.Aegis | Sort-Object -Unique)
	if ($names.Count -gt 1) { Fail "Monster ID $($group.Name) maps to conflicting Aegis names: $($names -join ', ')" }
}
foreach ($group in $mobIdentities | Group-Object Aegis) {
	$ids = @($group.Group.Id | Sort-Object -Unique)
	if ($ids.Count -gt 1) { Fail "Monster AegisName '$($group.Name)' maps to conflicting IDs: $($ids -join ', ')" }
}
Write-Host "  consistent monster identities: $($mobIdentities.Count) records"
$spawnPatterns = @(
	'^\s*(?:monster)\b\s*\(?\s*[^,;\r\n]+,\s*[^,]+,\s*[^,]+,\s*"[^"]*"\s*,\s*(?<mob>"[^"]+"|\d+)',
	'^\s*(?:areamonster)\b\s*\(?\s*[^,;\r\n]+,\s*[^,]+,\s*[^,]+,\s*[^,]+,\s*[^,]+,\s*"[^"]*"\s*,\s*(?<mob>"[^"]+"|\d+)',
	'^\s*(?:bg_monster)\b\s*\(?\s*[^,;\r\n]+,\s*[^,]+,\s*[^,]+,\s*[^,]+,\s*"[^"]*"\s*,\s*(?<mob>"[^"]+"|\d+)'
)
$spawnRefs = 0
foreach ($file in $enabledFiles) {
	$relative = $file.FullName.Substring($repo.Length + 1).Replace('\','/')
	$lineNumber = 0
	foreach ($line in ((Read-ScriptCode $file.FullName) -split '\r?\n')) {
		$lineNumber++
		foreach ($pattern in $spawnPatterns) {
			foreach ($match in [regex]::Matches($line, $pattern)) {
				$spawnRefs++
				$token = $match.Groups['mob'].Value.Trim('"')
				if ($token -match '^\d+$') {
					if (!$mobIds.Contains([int]$token)) { Fail "Missing monster ID $token at ${relative}:$lineNumber" }
				} elseif (!$mobNames.Contains($token)) {
					Fail "Missing monster AegisName '$token' at ${relative}:$lineNumber"
				}
			}
		}
	}
}
Write-Host "  literal enabled monster spawns: $spawnRefs"

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
# supported rolls, including 2026 POW/CON, Fierce Attack, and Great Craftsman
# additions. The eight client jewel families use weighted upgrades through Lv10;
# the two extra custom families retain their existing deterministic recipes.
$biosphereEnchant = [IO.File]::ReadAllText((RepoPath 'db/import/item_enchant.yml'))
foreach ($literal in @(
	'BEGIN CODEX BIOSPHERE CROWN ENCHANT 132','- Id: 132','Time_DM_R_Crown_DK: true',
	'T_D_Jewel_POW_1','T_D_Jewel_CON_3','Fierce_A_Jewel_1','Great_C_Jewel_1',
	'Upgrade: Fierce_A_Jewel_5','Upgrade: Great_C_Jewel_5'
)) {
	if (!$biosphereEnchant.Contains($literal)) { Fail "Incomplete Biosphere crown enchant group: '$literal'" }
}
Write-Host '  Biosphere crown enchant group: 132'
$biosphereCrown = [regex]::Match($biosphereEnchant, '(?s)# BEGIN CODEX BIOSPHERE CROWN ENCHANT 132.*?# END CODEX BIOSPHERE CROWN ENCHANT 132').Value
$biosphereRolls = [regex]::Matches($biosphereCrown, '(?m)^            RandomUpgrades:\r?\n(?:              - Upgrade: [^\r\n]+\r?\n                Chance: \d+\r?\n)+')
if ($biosphereRolls.Count -ne 72) { Fail "Biosphere weighted upgrade count is $($biosphereRolls.Count), expected 72" }
foreach ($roll in $biosphereRolls) {
	$total = 0
	foreach ($chance in [regex]::Matches($roll.Value, 'Chance: (\d+)')) { $total += [int]$chance.Groups[1].Value }
	if ($total -ne 100000) { Fail "Biosphere crown upgrade probabilities total $total, expected 100000" }
}

# The Grade Workshop service must retain its native crown table and weighted
# upgrade recipes; a loaded NPC alone does not prove its enchant UI can open.
$workshopEnchantPath = 'db/import/grademk_item_enchant.yml'
if ($dbImports -notcontains $workshopEnchantPath) {
	Fail "Grade Workshop enchant database is not imported: $workshopEnchantPath"
}
if (Test-Path -LiteralPath (RepoPath $workshopEnchantPath)) {
	$workshopEnchant = [IO.File]::ReadAllText((RepoPath $workshopEnchantPath))
	$targets = [regex]::Matches($workshopEnchant, '(?m)^      (?:Time_DM|Frontier)_R_Crown_\w+: true\s*$')
	if ($targets.Count -ne 36) { Fail "Grade Workshop crown target count is $($targets.Count), expected 36" }
	$rolls = [regex]::Matches($workshopEnchant, '(?m)^            RandomUpgrades:\r?\n(?:              - Upgrade: [^\r\n]+\r?\n                Chance: \d+\r?\n)+')
	if ($rolls.Count -ne 90) { Fail "Grade Workshop weighted upgrade count is $($rolls.Count), expected 90" }
	foreach ($roll in $rolls) {
		$total = 0
		foreach ($chance in [regex]::Matches($roll.Value, 'Chance: (\d+)')) { $total += [int]$chance.Groups[1].Value }
		if ($total -ne 100000) { Fail "Grade Workshop upgrade probabilities total $total, expected 100000" }
	}
} else {
	Fail "Missing Grade Workshop enchant database: $workshopEnchantPath"
}
Write-Host '  Grade Workshop crown enchants: 36 targets, 90 weighted upgrades'

$workshopGroupIds = [Collections.Generic.HashSet[int]]::new()
foreach ($relative in @('db/re/item_enchant.yml', 'db/import/item_enchant.yml', 'db/import/grademk_item_enchant.yml', 'db/import/grademk_service_enchants.yml')) {
	if (!(Test-Path -LiteralPath (RepoPath $relative))) { Fail "Missing workshop enchant dependency: $relative"; continue }
	if ($dbImports -notcontains $relative) { Fail "Workshop enchant dependency is not imported: $relative" }
	foreach ($match in [regex]::Matches([IO.File]::ReadAllText((RepoPath $relative)), '(?m)^\s*- Id:\s*(\d+)\s*(?:#.*)?$')) {
		[void]$workshopGroupIds.Add([int]$match.Groups[1].Value)
	}
}
foreach ($id in @(7,8,9,10,11,12,13,15,16,17,18,19,52,53,54,55,117,118,119,120,121,122,123,124,142,163,164)) {
	if (!$workshopGroupIds.Contains($id)) { Fail "Grade Workshop NPC references missing enchant group $id" }
}
Write-Host '  Grade Workshop native enchant dependencies: 27 groups'

# Chapter 1 footwear group 163 must reproduce the official upgrade behavior:
# ranks 1-4 advance one rank at 90% or skip two ranks at 10%, while rank 5
# advances deterministically to rank 6. Native weighted-upgrade support must
# remain wired through the database parser and request handler.
$chapter1EnchantMatch = [regex]::Match($biosphereEnchant, '(?ms)^\s*- Id: 163\s*$.*\z')
$perfectUpgradePath = 'db/import/perfect_item_enchant.yml'
if ($dbImports -notcontains $perfectUpgradePath) { Fail "Perfect upgrade database is not imported: $perfectUpgradePath" }
$perfectUpgradeData = [IO.File]::ReadAllText((RepoPath $perfectUpgradePath))
$chapter1Perfect = [regex]::Match($perfectUpgradeData, '(?ms)^  - Id: 163\s*$.*?(?=^  - Id:|\z)').Value
if ([regex]::Matches($chapter1Perfect, '(?m)^          - Enchant:').Count -ne 16) {
	Fail 'Chapter 1 needs 16 separately selectable guaranteed final upgrades'
}
$crownPerfect = [regex]::Match($perfectUpgradeData, '(?ms)^  - Id: 164\s*$.*?(?=^  - Id:|\z)').Value
if ([regex]::Matches($crownPerfect, '(?m)^          - Enchant:').Count -ne 20) {
	Fail 'Frontier crown needs 20 separately selectable guaranteed upgrades'
}
if (!$chapter1EnchantMatch.Success) {
	Fail 'Missing Chapter 1 footwear enchant group 163'
} else {
	$chapter1Enchant = $chapter1EnchantMatch.Value
	$perfectCount = [regex]::Matches($chapter1Enchant, '(?m)^\s+- Item:\s+Ch01_S_E_\w+01\s*$').Count
	if ($perfectCount -ne 16) { Fail "Chapter 1 enchant group 163 has $perfectCount perfect entry enchants, expected 16" }

	$families = @('Life','Regiment','GAE','Immo','Vain','Flame','Frozen','Earth','Death','Poison','Fighting','Prudence','Pride','Sageness','Volition','Will')
	$weightedCount = [regex]::Matches($chapter1Enchant, '(?m)^\s+RandomUpgrades:\s*$').Count
	if ($weightedCount -ne 64) { Fail "Chapter 1 enchant group 163 has $weightedCount weighted upgrade steps, expected 64" }

	foreach ($family in $families) {
		foreach ($level in 1..4) {
			$from = '{0:D2}' -f $level
			$next = '{0:D2}' -f ($level + 1)
			$skip = '{0:D2}' -f ($level + 2)
			$weightedPattern = "(?ms)^\s+- Enchant:\s+Ch01_S_E_$family$from\s*`r?`n\s+RandomUpgrades:\s*`r?`n\s+- Upgrade:\s+Ch01_S_E_$family$next\s*`r?`n\s+Chance:\s+90000\s*`r?`n\s+- Upgrade:\s+Ch01_S_E_$family$skip\s*`r?`n\s+Chance:\s+10000\s*$"
			if ($chapter1Enchant -notmatch $weightedPattern) {
				Fail "Chapter 1 enchant group 163 has an invalid weighted transition for Ch01_S_E_$family$from"
			}
		}

		$finalPattern = "(?m)^\s+- Enchant:\s+Ch01_S_E_$($family)05\s*`r?`n\s+Upgrade:\s+Ch01_S_E_$($family)06\s*$"
		if ($chapter1Perfect -notmatch $finalPattern) {
			Fail "Chapter 1 enchant group 163 is missing deterministic transition Ch01_S_E_$($family)05 -> Ch01_S_E_$($family)06"
		}
	}
}
$itemdbHeader = [IO.File]::ReadAllText((RepoPath 'src/map/itemdb.hpp'))
$upgradeHeader = [IO.File]::ReadAllText((RepoPath 'src/map/enchant_upgrade.hpp'))
$itemdbSource = [IO.File]::ReadAllText((RepoPath 'src/map/itemdb.cpp'))
$clifSource = [IO.File]::ReadAllText((RepoPath 'src/map/clif.cpp'))
foreach ($check in @(
	@($upgradeHeader, 'std::vector<s_item_enchant_random_upgrade> random_upgrades;', 'weighted upgrade data structure'),
	@($itemdbSource, 'Random upgrade chances must total 100000', 'weighted upgrade parser validation'),
	@($itemdbSource, 'slotNode["PerfectUpgrades"]', 'separate guaranteed upgrade parser'),
	@($clifSource, 'select_enchant_upgrade_result( *upgrade,', 'weighted upgrade runtime selection'),
	@($clifSource, 'enchant_slot->upgrade.enchants, enchant_slot->perfect_upgrades,', 'separate request-mode recipe selection'),
	@($clifSource, 'p->slot, true, p->ITID', 'guaranteed request target validation')
)) {
	if (!$check[0].Contains($check[1])) { Fail "Missing $($check[2])" }
}
$upgradePacketDb = [IO.File]::ReadAllText((RepoPath 'src/map/clif_packetdb.hpp'))
foreach ($literal in @(
	'0x0bf0, sizeof( struct PACKET_CZ_REQUEST_UPGRADE_ENCHANT ), clif_parse_enchantwindow_upgrade',
	'PACKET_CZ_REQUEST_PERFECT_UPGRADE_ENCHANT ), clif_parse_enchantwindow_perfect_upgrade',
	'0x0bf2, 13, clif_parse_enchantwindow_reset'
)) {
	if (!$upgradePacketDb.Contains($literal)) { Fail "Missing modern enchant packet route: $literal" }
}
Write-Host '  Chapter 1 footwear enchant upgrades: 64 weighted + 16 final'

# Fashion enchant status IDs 37/38 require matching C++ enum/export records
# and database status definitions; a script-only name compiles but cannot run.
$statusHeader = [IO.File]::ReadAllText((RepoPath 'src/map/status.hpp'))
$scriptConstants = [IO.File]::ReadAllText((RepoPath 'src/map/script_constants.hpp'))
$statusImport = [IO.File]::ReadAllText((RepoPath 'db/import/status.yml'))
foreach ($status in @('SC_CONTENTS_37','SC_CONTENTS_38')) {
	if (!$statusHeader.Contains($status)) { Fail "Missing source status enum $status" }
	if (!$scriptConstants.Contains("export_constant($status)")) { Fail "Missing script export for $status" }
}
foreach ($status in @('Contents_37','Contents_38')) {
	if ($statusImport -notmatch "(?m)^\s+- Status:\s+$status\s*$") { Fail "Missing status database record $status" }
}
Write-Host '  Fashion enchant source statuses: 2'

# Every equipment-enchant/reform service must mutate the original inventory
# record. Recreating non-stackable equipment changes its generated unique ID,
# while `delequip` destroys it before a script can safely rebuild the record.
$scriptSource = [IO.File]::ReadAllText((RepoPath 'src/map/script.cpp'))
foreach ($literal in @(
	'BUILDIN_FUNC(modifyequipitem)',
	'BUILDIN_DEF(modifyequipitem,"iiiiiii?")',
	'memcmp(&current_item, &expected_item, sizeof(struct item)) == 0',
	'replacement_data->equip != old_data->equip',
	'script_equipment_slot_transition_mask(0, 1) == 0x1U',
	'(selected_item.card[slot] != 0 || (card_slot == slot && new_card_id != 0))',
	'new_card->subtype != CARD_ENCHANT'
)) {
	if (!$scriptSource.Contains($literal)) { Fail "Incomplete in-place equipment mutation API: '$literal'" }
}
if ($scriptSource.Contains('replacement_data->slots != old_data->slots')) {
	Fail 'In-place item-ID conversion still rejects every intrinsic slot-count transition'
}
$equipmentMutationConsumers = @(
	@('npc/custom/fashion_points/FashionEnchant.txt', 'modifyequipitem(.@part,.@equip_id,0,-1,.@slot,0,.@enchant)', 'Fashion enchant application'),
	@('npc/custom/fashion_points/FashionPoints.txt', 'modifyequipitem(.@part,.@equip_id,0,-1,.@slot,.@enchant,0)', 'Fashion stone recovery'),
	@('npc/custom/chapter2/Chapter2.txt', 'modifyequipitem(.@part,.@oldid,.@newid,.@newref,-1,0,0)', 'Chapter 2 refine/reform'),
	@('npc/custom/instances/HallOfLife.txt', 'modifyequipitem(EQI_HEAD_LOW,420231,0,-1,3,.@current_card,.@target_card)', 'Hall of Life barrier'),
	@('npc/custom/varmundt_biosphere_depth.txt', 'modifyequipitem(EQI_HEAD_TOP,.@id,0,-1,.@slot', 'Biosphere crown reroll')
)
foreach ($consumer in $equipmentMutationConsumers) {
	$content = [IO.File]::ReadAllText((RepoPath $consumer[0]))
	if (!$content.Contains($consumer[1])) { Fail "Missing in-place equipment mutation: $($consumer[2]) ($($consumer[0]))" }
	if ($content -match '(?m)\bdelequip\s') { Fail "Destructive delequip remains in equipment mutation consumer: $($consumer[0])" }
	if ($content -match '\bgetequipuniqueid\s*\(') { Fail "Unique-ID rejection remains in equipment mutation consumer: $($consumer[0])" }
}
Write-Host "  in-place equipment mutation consumers: $($equipmentMutationConsumers.Count)"

$guardedOfficialEquipmentConsumers = @(
	@('npc/events/RWC_2012.txt', 'modifyequipitem(.@part,.@equip_id,.@slotted,-1,-1,0,0)', 'RWC slot conversion'),
	@('npc/re/instances/FridayDungeon.txt', 'modifyequipenchantstate(.@slot,.@equip_id,.@equip_refine,', 'Friday enchant/reset'),
	@('npc/re/instances/PoringVillage.txt', 'deleteequipitem(EQI_HEAD_LOW,.@equip_id,.@equip_refine', 'Poring Village break'),
	@('npc/re/instances/SarahAndFenrir.txt', 'deleteequipitem(.@part,.@equip_id,.@equip_refine', 'Sarah earring break'),
	@('npc/re/merchants/bio4_reward.txt', 'deleteequipitem(.@part,.@equip_item,.@refine_count', 'Bio4 equipment break')
)
foreach ($consumer in $guardedOfficialEquipmentConsumers) {
	$content = [IO.File]::ReadAllText((RepoPath $consumer[0]))
	if (!$content.Contains($consumer[1])) { Fail "Missing guarded equipment operation: $($consumer[2]) ($($consumer[0]))" }
	if ($content -match '(?m)\b(?:delequip|delitemidx)\b') { Fail "Unsafe equipment deletion remains: $($consumer[0])" }
	if ($content -match '(?m)\bgetitem2?\s+\.@(?:equip|sarah)') { Fail "Equipment reconstruction remains: $($consumer[0])" }
}
Write-Host "  guarded official equipment consumers: $($guardedOfficialEquipmentConsumers.Count)"

# Episode 19 reused stock Moscovia quest IDs 18119-18121. Keep the remap and
# login migration paired so neither quest line can overwrite the other.
$moscovia = [IO.File]::ReadAllText((RepoPath 'npc/quests/quests_moscovia.txt'))
$moscoviaCompat = [IO.File]::ReadAllText((RepoPath 'npc/custom/moscovia_quest_compat.txt'))
foreach ($id in 900200..900202) {
	if (!$questIds.Contains($id)) { Fail "Missing remapped Moscovia quest ID $id" }
	if ($moscovia -notmatch "(?<!\d)$id(?!\d)") { Fail "Moscovia script does not use remapped quest ID $id" }
	if ($moscoviaCompat -notmatch "(?<!\d)$id(?!\d)") { Fail "Moscovia migration does not cover quest ID $id" }
}
foreach ($id in 18119..18121) {
	if ($moscovia -match "(?<!\d)$id(?!\d)") { Fail "Moscovia script still collides on Episode 19 quest ID $id" }
}
Write-Host '  Moscovia/Episode 19 quest collision remaps: 3'

# Geffen Magic Tournament is advertised by the warper, so its script must be
# enabled and must retain the repaired reservation, portal, and reward guards.
$customLoader = [IO.File]::ReadAllText((RepoPath 'npc/scripts_custom.conf'))
$gmt = [IO.File]::ReadAllText((RepoPath 'npc/custom/official/GeffenMagicTournament.txt'))
foreach ($literal in @(
	'npc: npc/custom/official/GeffenMagicTournament.txt',
	'.@instance_id = instance_create(.@md_name$);',
	'GMTRewardClaimed = 1;',
	'if (!checkweight(6671,.@reward_amount))'
)) {
	$source = if ($literal.StartsWith('npc:')) { $customLoader } else { $gmt }
	if (!$source.Contains($literal)) { Fail "Incomplete Geffen Magic Tournament repair: '$literal'" }
}
if ($gmt.Contains('InventoryCheck')) { Fail 'Geffen Magic Tournament still calls undefined InventoryCheck' }
Write-Host '  Geffen Magic Tournament activation guards: present'

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
		if ($line -match '^\s*- Id:\s*(\d+)\s*(?:#.*)?$') { [void]$serverIds.Add([int]$Matches[1]) }
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
	@('src/map/atcommand.cpp', 'ACMD_FUNC(bs)', 'live battle-status command implementation'),
	@('src/map/atcommand.cpp', 'ACMD_DEF(bs)', 'live battle-status command registration'),
	@('src/map/atcommand.cpp', '{ "icecastle",    185, 212 }', 'Episode 19 @go destination'),
	@('src/map/atcommand.cpp', '{ "bl_depth1",       0,   0 }', 'Biosphere Depth @go destination'),
	@('src/map/atcommand.cpp', '{ "itemmall",       22,  43 }', 'service-mall @go destination'),
	@('client-patch/chapter2/install_client_patch.ps1', '& $questInstaller -DataRoot $clientRoot', 'Chapter 2 quest installer client-root handoff'),
	@('db/import/quest_db.yml', 'regex:Location: jor_raise1\r?\n    MapName: Northern Raised Land', 'Episode 21 northern raised-land quest target'),
	@('db/import/quest_db.yml', 'regex:Location: jor_raise2\r?\n    MapName: Southern Raised Land', 'Episode 21 southern raised-land quest target'),
	@('db/import/mob_skill_db.txt', '22360,EP21_YORTUS_A@AL_TELEPORT', 'Episode 21 Jortus Bishop mob-skill owner'),
	@('npc/custom/fashion_points/FashionPoints.txt', "function`tscript`tFP_OpenBox", 'Fashion box opening function'),
	@('npc/custom/fashion_points/FashionEnchant.txt', 'Complete Fashion Enchanter#FP', 'complete Fashion enchant service'),
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

# Chapter 2's fallback maps must retain the published party lifecycle and the
# compatibility adaptations that prevent the old story/daily reward exploits.
$chapter2Main = [IO.File]::ReadAllText((RepoPath 'npc/custom/chapter2/Chapter2.txt'))
$chapter2Instances = [IO.File]::ReadAllText((RepoPath 'npc/custom/chapter2/Instances.txt'))
$chapter2Flags = [IO.File]::ReadAllText((RepoPath 'npc/custom/chapter2/Mapflags.txt'))
$chapter2Mobs = [IO.File]::ReadAllText((RepoPath 'db/import/chapter2_mob_db.yml'))
$chapter2Items = [IO.File]::ReadAllText((RepoPath 'db/import/chapter2_item_db.yml'))
foreach ($literal in @(
	'instance_create(.@name$,IM_PARTY,.@party_id)',
	'is_party_leader()',
	"setinstancevar 'ch2_roster$",
	"function`tscript`tCH2_OpenPhantomCrystal",
	'if (CH2_Step < 3)',
	'if (!callfunc("CH2_Complete")) { mes "Complete Chapter 2 before buying Kindle Hals."'
)) {
	if (!$chapter2Main.Contains($literal)) { Fail "Missing Chapter 2 lifecycle/access guard: '$literal'" }
}
if ($chapter2Main.Contains('instance_id(IM_CHAR)') -or $chapter2Main.Contains('instance_create(.@name$,IM_CHAR')) {
	Fail 'Chapter 2 still creates character-owned instances instead of party instances'
}
foreach ($literal in @(
	'"Snapdragon Phantom",22711,1',
	'UMOB_DAMAGETAKEN,1',
	'UMOB_DAMAGETAKEN,100',
	'CH2_ClaimInstanceClear',
	'rand(100) < 10',
	'callfunc "CH2_GiveInstanceItem",1002700,4',
	'instance_destroy .@iid'
)) {
	if (!$chapter2Instances.Contains($literal)) { Fail "Missing Chapter 2 encounter/reward guard: '$literal'" }
}
if (([regex]::Matches($chapter2Instances, '(?<!\d)106441(?!\d)')).Count -ne 1) {
	Fail 'Chapter 2 Phantom Crystal must have exactly one scripted acquisition path'
}
$storyCredit = [regex]::Match($chapter2Instances, '(?ms)if \(!callfunc\("CH2_Complete"\).*?return 1;').Value
if (!$storyCredit -or $storyCredit -match 'CH2_DailyPhantom\s*=\s*1') {
	Fail 'Chapter 2 story Phantom clear can pre-arm the repeatable daily'
}
foreach ($mapName in @('1@ch2a','1@ch2b')) {
	if (!$chapter2Flags.Contains("$mapName`tmapflag`tpartylock")) { Fail "Chapter 2 instance map '$mapName' lacks a local partylock" }
}
$phantomMob = [regex]::Match($chapter2Mobs, '(?ms)^\s*- Id: 22711\s.*?(?=^\s*- Id:|\z)').Value
if (!$phantomMob -or $phantomMob -notmatch 'AegisName:\s*CH2_ANTIRRHINUM2' -or
	$phantomMob -notmatch 'Hp:\s*50000000' -or $phantomMob -notmatch 'Defense:\s*749' -or
	$phantomMob -notmatch 'MagicDefense:\s*556' -or $phantomMob -notmatch 'aegis_300783') {
	Fail 'Chapter 2 monster 22711 no longer matches the published Phantom identity/core stats/card'
}
if ($chapter2Mobs -match 'Ch2_Phantom_Crystal') { Fail 'Phantom Crystal remains in a mob drop table and would duplicate the scripted 10% roll' }
$crystalItem = [regex]::Match($chapter2Items, '(?ms)^\s*- Id: 106441\s.*?(?=^\s*- Id:|\z)').Value
if (!$crystalItem -or $crystalItem -notmatch 'Type:\s*Usable' -or $crystalItem -notmatch 'CH2_OpenPhantomCrystal') {
	Fail 'Chapter 2 Phantom Crystal is not a usable transparent material exchange'
}
Write-Host '  Chapter 2 party/encounter/reward guards: present'

# The active server revision uses the legacy 19-column mob-skill database.
# Validate the entire custom overlay, not only rows exercised by a smoke test:
# a syntactically valid row can still be silently rejected when either its mob
# or skill ID is absent. The special `clear` rows intentionally have no skill.
$skillIds = Get-DatabaseIds @('db/re', 'db/import') '*skill_db.yml'
$customMobSkillRows = 0
$mobSkillLine = 0
foreach ($row in [IO.File]::ReadLines((RepoPath 'db/import/mob_skill_db.txt'))) {
	++$mobSkillLine
	if (!$row -or $row.StartsWith('//')) { continue }
	++$customMobSkillRows
	# Regex.Split preserves trailing empty CSV fields consistently on both
	# Windows PowerShell 5.1 and PowerShell 7; `-split ',', -1` has different
	# negative-limit semantics between those runtimes.
	$fields = [regex]::Split($row, ',')
	if ($fields.Count -ne 19) {
		Fail "Custom mob-skill row has $($fields.Count) fields instead of 19 at db/import/mob_skill_db.txt:$mobSkillLine"
		continue
	}
	$parsedMob = 0
	if (![int]::TryParse($fields[0], [ref]$parsedMob) -or ($parsedMob -ge 0 -and !$mobIds.Contains($parsedMob))) {
		Fail "Custom mob-skill row references missing mob '$($fields[0])' at db/import/mob_skill_db.txt:$mobSkillLine"
	}
	if ($fields[1] -eq 'clear') { continue }
	$parsedSkill = 0
	if (![int]::TryParse($fields[3], [ref]$parsedSkill) -or !$skillIds.Contains($parsedSkill)) {
		Fail "Custom mob-skill row references missing skill '$($fields[3])' at db/import/mob_skill_db.txt:$mobSkillLine"
	}
}
Write-Host "  validated custom mob-skill rows: $customMobSkillRows"

# Legacy CSV mob skills cannot import a second TXT file. Ensure each maintained
# staging body is represented exactly once by a named canonical block.
$canonicalMobSkills = [IO.File]::ReadAllText((RepoPath 'db/import/mob_skill_db.txt'))
$canonicalMobSkillLines = @($canonicalMobSkills -split '\r?\n')
function Test-StagedMobSkillBlock([string]$sourcePath, [string]$marker, [string]$label, [bool]$requireGlobalUnique = $true) {
	$sourceRows = @([IO.File]::ReadAllLines((RepoPath $sourcePath)) |
		Where-Object { $_ -and !$_.StartsWith('//') })
	foreach ($duplicate in @($sourceRows | Group-Object | Where-Object Count -gt 1)) {
		Fail "Duplicate $label mob-skill row: $($duplicate.Name)"
	}
	foreach ($row in $sourceRows) {
		if (([regex]::Split($row, ',')).Count -ne 19) { Fail "$label mob-skill row does not have 19 fields: $row" }
		if ($requireGlobalUnique) {
			$canonicalCopies = @($canonicalMobSkillLines | Where-Object { $_ -ceq $row }).Count
			if ($canonicalCopies -ne 1) { Fail "$label mob-skill row occurs $canonicalCopies times in the canonical database: $row" }
		}
	}
	$escapedMarker = [regex]::Escape($marker)
	$block = [regex]::Match($canonicalMobSkills, "(?ms)^// BEGIN CODEX $escapedMarker\r?\n(?<body>.*?)^// END CODEX $escapedMarker\s*$")
	if (!$block.Success) {
		Fail "Canonical mob-skill DB is missing the $label block"
	} else {
		$canonicalRows = @($block.Groups['body'].Value -split '\r?\n' | Where-Object { $_ -and !$_.StartsWith('//') })
		if (($canonicalRows -join "`n") -cne ($sourceRows -join "`n")) {
			Fail "Canonical $label mob-skill block differs from its maintained source file"
		}
	}
	Write-Host "  $label mob-skill rows: $($sourceRows.Count)"
}
Test-StagedMobSkillBlock 'db/import/garden_of_time_combat_mob_skill_db.txt' 'GARDEN OF TIME COMBAT OVERLAY' 'Garden combat' $false
Test-StagedMobSkillBlock 'db/import/episode20_mob_skill_db.txt' 'EPISODE 20 COMPLETE MOB SKILLS' 'Episode 20'
Test-StagedMobSkillBlock 'db/import/thanatos_mob_skill_db.txt' 'THANATOS TOWER MEMORY MOB SKILLS' 'Thanatos Tower'

# Every source map copied by an enabled instance must carry the standard
# instance containment set. Derive this across all instance records and all
# enabled mapflag scripts so new episodes cannot evade the check.
$mapFlagsByMap = @{}
foreach ($file in $enabledFiles) {
	foreach ($line in ((Read-ScriptCode $file.FullName) -split '\r?\n')) {
		if ($line -notmatch '(?i)^\s*(?<map>[A-Za-z0-9_@]+)\s+mapflag\s+(?<flag>[A-Za-z0-9_]+)(?:\s+(?<argument>[^\r\n]+))?\s*$') { continue }
		$mapName = $Matches['map']
		$flagName = $Matches['flag'].ToLowerInvariant()
		$signature = $flagName
		if (!$mapFlagsByMap.ContainsKey($mapName)) {
			$mapFlagsByMap[$mapName] = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
		}
		[void]$mapFlagsByMap[$mapName].Add($signature)
	}
}
$requiredInstanceMapFlags = @('nobranch','nomemo','noteleport','monster_noteleport','nowarpto','partylock','restricted')
$instanceSourceMaps = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
foreach ($instance in $instances) {
	foreach ($mapName in $instance.Maps) { [void]$instanceSourceMaps.Add($mapName) }
}
foreach ($mapName in $instanceSourceMaps) {
	foreach ($flag in $requiredInstanceMapFlags) {
		if (!$mapFlagsByMap.ContainsKey($mapName) -or !$mapFlagsByMap[$mapName].Contains($flag)) {
			Fail "Instance source map '$mapName' is missing containment mapflag '$flag'"
		}
	}
}
Write-Host "  protected instance source maps: $($instanceSourceMaps.Count)"

# A definition absent from an enabled script that can actually create that
# exact instance has no player path into it. Associate instance_create's
# argument with literals assigned to that argument, and follow only helper
# functions that pass their first argument into instance_create. This prevents
# an unrelated quoted name elsewhere in a creator file from masking a missing
# entrance (the old classic-Thanatos false positive).
$enabledNpcSources = [Collections.Generic.List[object]]::new()
foreach ($script in $enabledScripts) {
	$path = RepoPath $script
	if (!(Test-Path -LiteralPath $path -PathType Leaf)) { continue }
	$content = Read-ScriptCode $path
	$enabledNpcSources.Add([pscustomobject]@{ Path = $script; Content = $content })
}

$reachableInstanceNames = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
$functionBodies = [Collections.Generic.List[object]]::new()
foreach ($source in $enabledNpcSources) {
	foreach ($match in [regex]::Matches($source.Content, '(?ms)^function\s+script\s+(?<name>[A-Za-z0-9_]+)\s*\{(?<body>.*?)^\}')) {
		$functionBodies.Add([pscustomobject]@{
			Name = $match.Groups['name'].Value
			Body = $match.Groups['body'].Value
			Path = $source.Path
		})
	}
}

# Direct literal arguments are unambiguous. For variable arguments, collect
# only values assigned to that same scalar/array in the source containing the
# call; do not accept arbitrary quoted captions from the file.
foreach ($source in $enabledNpcSources) {
	foreach ($match in [regex]::Matches($source.Content, '(?i)(?<![A-Za-z0-9_])instance_create\s*\(\s*(?:"(?<dq>[^"]+)"|''(?<sq>[^'']+)'')')) {
		$name = if ($match.Groups['dq'].Success) { $match.Groups['dq'].Value } else { $match.Groups['sq'].Value }
		[void]$reachableInstanceNames.Add($name)
	}
	foreach ($match in [regex]::Matches($source.Content, '(?i)(?<![A-Za-z0-9_])instance_create\s*\(\s*(?<var>[.$#''@A-Za-z_][.$#''@A-Za-z0-9_]*\$?)(?:\s*\[[^\]\r\n]+\])?')) {
		$variable = $match.Groups['var'].Value
		$escaped = [regex]::Escape($variable)
		$assignmentPattern = '(?im)^\s*(?:set\s+)?' + $escaped + '(?:\s*\[[^\]\r\n]+\])?\s*(?:=|,)\s*(?<values>[^;\r\n]+)'
		foreach ($assignment in [regex]::Matches($source.Content, $assignmentPattern)) {
			foreach ($literal in [regex]::Matches($assignment.Groups['values'].Value, '(?:"(?<dq>[^"]+)"|''(?<sq>[^'']+)'')')) {
				$name = if ($literal.Groups['dq'].Success) { $literal.Groups['dq'].Value } else { $literal.Groups['sq'].Value }
				[void]$reachableInstanceNames.Add($name)
			}
		}
		$setArrayPattern = '(?ims)^\s*setarray\s+' + $escaped + '(?:\s*\[[^\]\r\n]+\])?\s*,(?<values>[^;]+);'
		foreach ($setArray in [regex]::Matches($source.Content, $setArrayPattern)) {
			foreach ($literal in [regex]::Matches($setArray.Groups['values'].Value, '(?:"(?<dq>[^"]+)"|''(?<sq>[^'']+)'')')) {
				$name = if ($literal.Groups['dq'].Success) { $literal.Groups['dq'].Value } else { $literal.Groups['sq'].Value }
				[void]$reachableInstanceNames.Add($name)
			}
		}
	}
}

# Episode helpers accept an instance name as getarg(0). Track that data flow,
# including thin wrappers which pass getarg(0) into another such helper.
$passthroughCreators = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
foreach ($function in $functionBodies) {
	if ($function.Body -match '(?i)instance_create\s*\(\s*getarg\s*\(\s*0\s*\)') {
		[void]$passthroughCreators.Add($function.Name)
		continue
	}
	foreach ($create in [regex]::Matches($function.Body, '(?i)instance_create\s*\(\s*(?<var>[.$#''@A-Za-z_][.$#''@A-Za-z0-9_]*\$?)')) {
		$escaped = [regex]::Escape($create.Groups['var'].Value)
		if ($function.Body -match ('(?im)^\s*(?:set\s+)?' + $escaped + '\s*(?:=|,)\s*getarg\s*\(\s*0\s*\)')) {
			[void]$passthroughCreators.Add($function.Name)
			break
		}
	}
}
do {
	$addedCreator = $false
	foreach ($function in $functionBodies) {
		if ($passthroughCreators.Contains($function.Name)) { continue }
		foreach ($creator in $passthroughCreators) {
			$creatorPattern = [regex]::Escape($creator)
			if ($function.Body -match ('(?i)\bcallfunc\s*\(?\s*["'']' + $creatorPattern + '["'']\s*,\s*getarg\s*\(\s*0\s*\)')) {
				[void]$passthroughCreators.Add($function.Name); $addedCreator = $true; break
			}
			foreach ($argument in [regex]::Matches($function.Body, '(?im)^\s*(?:set\s+)?(?<var>[.$#''@A-Za-z_][.$#''@A-Za-z0-9_]*\$?)\s*(?:=|,)\s*getarg\s*\(\s*0\s*\)')) {
				$argumentPattern = [regex]::Escape($argument.Groups['var'].Value)
				if ($function.Body -match ('(?i)\bcallfunc\s*\(?\s*["'']' + $creatorPattern + '["'']\s*,\s*' + $argumentPattern + '\b')) {
					[void]$passthroughCreators.Add($function.Name); $addedCreator = $true; break
				}
			}
			if ($addedCreator) { break }
		}
	}
} while ($addedCreator)


foreach ($creator in $passthroughCreators) {
	$pattern = '(?i)\bcallfunc\s*\(?\s*["'']' + [regex]::Escape($creator) + '["'']\s*,\s*(?:"(?<dq>[^"]+)"|''(?<sq>[^'']+)'')'
	foreach ($source in $enabledNpcSources) {
		foreach ($call in [regex]::Matches($source.Content, $pattern)) {
			$name = if ($call.Groups['dq'].Success) { $call.Groups['dq'].Value } else { $call.Groups['sq'].Value }
			[void]$reachableInstanceNames.Add($name)
		}
		$variablePattern = '(?i)\bcallfunc\s*\(?\s*["'']' + [regex]::Escape($creator) + '["'']\s*,\s*(?<var>[.$#''@A-Za-z_][.$#''@A-Za-z0-9_]*\$?)(?:\s*\[[^\]\r\n]+\])?'
		foreach ($call in [regex]::Matches($source.Content, $variablePattern)) {
			$escaped = [regex]::Escape($call.Groups['var'].Value)
			$assignmentPattern = '(?im)^\s*(?:set\s+)?' + $escaped + '(?:\s*\[[^\]\r\n]+\])?\s*(?:=|,)\s*(?<values>[^;\r\n]+)'
			foreach ($assignment in [regex]::Matches($source.Content, $assignmentPattern)) {
				foreach ($literal in [regex]::Matches($assignment.Groups['values'].Value, '(?:"(?<dq>[^"]+)"|''(?<sq>[^'']+)'')')) {
					$name = if ($literal.Groups['dq'].Success) { $literal.Groups['dq'].Value } else { $literal.Groups['sq'].Value }
					[void]$reachableInstanceNames.Add($name)
				}
			}
			$setArrayPattern = '(?ims)^\s*setarray\s+' + $escaped + '(?:\s*\[[^\]\r\n]+\])?\s*,(?<values>[^;]+);'
			foreach ($setArray in [regex]::Matches($source.Content, $setArrayPattern)) {
				foreach ($literal in [regex]::Matches($setArray.Groups['values'].Value, '(?:"(?<dq>[^"]+)"|''(?<sq>[^'']+)'')')) {
					$name = if ($literal.Groups['dq'].Success) { $literal.Groups['dq'].Value } else { $literal.Groups['sq'].Value }
					[void]$reachableInstanceNames.Add($name)
				}
			}
		}
	}
}

$unreferenced = @($instances | Where-Object { !$reachableInstanceNames.Contains($_.Name) })
if ($unreferenced.Count) {
	$message = "$($unreferenced.Count) instance definitions have no enabled creator for their exact name: " + (($unreferenced.Name | Sort-Object) -join ', ')
	if ($StrictContent) { Fail $message } else { Warn $message }
}
Write-Host "  exact reachable instance names: $($reachableInstanceNames.Count)"

foreach ($warning in $warnings) { Write-Warning $warning }
if ($failures.Count) {
	foreach ($failure in $failures) { Write-Error $failure -ErrorAction Continue }
	Write-Host "FAILED: $($failures.Count) integrity error(s), $($warnings.Count) warning(s)."
	exit 1
}

Write-Host "PASS: no integrity errors; $($warnings.Count) content-completeness warning(s)."
