param(
    [Parameter(Mandatory = $true)][string]$Cache,
    [Parameter(Mandatory = $true)][string[]]$Alias
)

$path = (Resolve-Path -LiteralPath $Cache).Path
$data = [IO.File]::ReadAllBytes($path)
$count = [BitConverter]::ToUInt16($data, 4)
$offset = 8
$records = @{}

for ($i = 0; $i -lt $count; $i++) {
    $start = $offset
    $name = [Text.Encoding]::ASCII.GetString($data, $offset, 12).Trim([char]0)
    $length = [BitConverter]::ToInt32($data, $offset + 16)
    $records[$name] = [byte[]]$data[$start..($start + 19 + $length)]
    $offset += 20 + $length
}

if (($Alias.Count % 2) -ne 0) {
    throw 'Alias must contain source/target pairs.'
}

$newRecords = [Collections.Generic.List[byte[]]]::new()
for ($i = 0; $i -lt $Alias.Count; $i += 2) {
    $source = $Alias[$i]
    $target = $Alias[$i + 1]
    if (!$records.ContainsKey($source)) { throw "Missing source map $source" }
    if ($target.Length -gt 11) { throw "Target map name $target exceeds 11 characters" }
    if ($records.ContainsKey($target)) { continue }
    $record = [byte[]]$records[$source].Clone()
    [Array]::Clear($record, 0, 12)
    [Text.Encoding]::ASCII.GetBytes($target).CopyTo($record, 0)
    $newRecords.Add($record)
    $records[$target] = $record
}

if ($newRecords.Count -eq 0) { exit 0 }

$stream = [IO.MemoryStream]::new()
$stream.Write($data, 0, $data.Length)
foreach ($record in $newRecords) { $stream.Write($record, 0, $record.Length) }
$result = $stream.ToArray()
[BitConverter]::GetBytes([uint16]($count + $newRecords.Count)).CopyTo($result, 4)
[BitConverter]::GetBytes([uint16]0).CopyTo($result, 6)
[IO.File]::WriteAllBytes($path, $result)
Write-Host "Added $($newRecords.Count) aliases to $path"
