# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  extract_grf_v3.ps1
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/extract_grf_v3.ps1
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

param(
    [Parameter(Mandatory = $true)]
    [string]$Grf,

    [string[]]$Search = @(),

    [string]$OutputDirectory
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Read-Exact {
    param([System.IO.Stream]$Stream, [int]$Count)
    $buffer = [byte[]]::new($Count)
    $read = 0
    while ($read -lt $Count) {
        $current = $Stream.Read($buffer, $read, $Count - $read)
        if ($current -eq 0) { throw "Unexpected end of stream." }
        $read += $current
    }
    return $buffer
}

function Read-UInt32 {
    param([System.IO.Stream]$Stream)
    return [BitConverter]::ToUInt32((Read-Exact $Stream 4), 0)
}

function Read-UInt64 {
    param([System.IO.Stream]$Stream)
    return [BitConverter]::ToUInt64((Read-Exact $Stream 8), 0)
}

function Expand-Zlib {
    param([byte[]]$Bytes)
    if ($Bytes.Length -lt 6) { throw "Invalid zlib stream." }
    $input = [System.IO.MemoryStream]::new($Bytes, 2, $Bytes.Length - 6, $false)
    $zlib = [System.IO.Compression.DeflateStream]::new(
        $input,
        [System.IO.Compression.CompressionMode]::Decompress
    )
    $output = [System.IO.MemoryStream]::new()
    try {
        $zlib.CopyTo($output)
        return $output.ToArray()
    }
    finally {
        $output.Dispose()
        $zlib.Dispose()
        $input.Dispose()
    }
}

$grfPath = (Resolve-Path -LiteralPath $Grf).Path
$stream = [System.IO.File]::OpenRead($grfPath)
try {
    $magicBytes = Read-Exact $stream 16
    $magic = [Text.Encoding]::ASCII.GetString($magicBytes)
    [void](Read-Exact $stream 14)

    if ($magic.StartsWith('Master of Magic')) {
        $tableOffset = [uint64](Read-UInt32 $stream)
        $seed = Read-UInt32 $stream
        $fileCount = (Read-UInt32 $stream) - $seed - 7
        $version = Read-UInt32 $stream
    }
    elseif ($magic.StartsWith('Event Horizon')) {
        $tableOffset = (Read-UInt64 $stream) + 4
        $fileCount = Read-UInt32 $stream
        $version = Read-UInt32 $stream
    }
    else {
        throw "Unsupported GRF signature."
    }

    if ($version -ne 0x200 -and $version -ne 0x300) {
        throw ('Unsupported GRF version: 0x{0:X}' -f $version)
    }

    [void]$stream.Seek([int64]$tableOffset, [System.IO.SeekOrigin]::Current)
    $compressedTableSize = Read-UInt32 $stream
    $tableSize = Read-UInt32 $stream
    $table = Expand-Zlib (Read-Exact $stream ([int]$compressedTableSize))
    if ($table.Length -ne $tableSize) {
        throw "GRF table size mismatch: expected $tableSize, got $($table.Length)."
    }

    $encoding = [Text.Encoding]::GetEncoding(1252)
    $position = 0
    $matches = [Collections.Generic.List[object]]::new()
    for ($i = 0; $i -lt $fileCount; $i++) {
        $nameEnd = [Array]::IndexOf($table, [byte]0, $position)
        if ($nameEnd -lt 0) { throw "Invalid GRF table at entry $i." }
        $name = $encoding.GetString($table, $position, $nameEnd - $position)
        $position = $nameEnd + 1

        $compressedSize = [BitConverter]::ToUInt32($table, $position); $position += 4
        $alignedSize = [BitConverter]::ToUInt32($table, $position); $position += 4
        $size = [BitConverter]::ToUInt32($table, $position); $position += 4
        $type = $table[$position]; $position++
        if ($version -eq 0x300) {
            $offset = [BitConverter]::ToUInt64($table, $position); $position += 8
        }
        else {
            $offset = [uint64][BitConverter]::ToUInt32($table, $position); $position += 4
        }

        if (($type -band 1) -eq 0) { continue }
        $wanted = $Search.Count -eq 0
        foreach ($pattern in $Search) {
            if ($name -match $pattern) { $wanted = $true; break }
        }
        if (-not $wanted) { continue }

        $entry = [pscustomobject]@{
            Name = $name
            Size = $size
            CompressedSize = $compressedSize
            AlignedSize = $alignedSize
            Type = $type
            Offset = $offset
        }
        $matches.Add($entry)

        if ($OutputDirectory) {
            if (($type -band 6) -ne 0) {
                Write-Warning "Skipping encrypted entry: $name (type $type)"
                continue
            }
            [void]$stream.Seek(46 + [int64]$offset, [System.IO.SeekOrigin]::Begin)
            $packed = Read-Exact $stream ([int]$compressedSize)
            $contents = if ($compressedSize -eq $size) { $packed } else { Expand-Zlib $packed }
            $relative = $name -replace '\\', [IO.Path]::DirectorySeparatorChar
            $target = Join-Path $OutputDirectory $relative
            $parent = Split-Path -Parent $target
            [void][IO.Directory]::CreateDirectory($parent)
            [IO.File]::WriteAllBytes($target, $contents)
        }
    }

    Write-Output ('Signature={0}; Version=0x{1:X}; Files={2}; Matches={3}' -f $magic.TrimEnd([char]0), $version, $fileCount, $matches.Count)
    $matches | Sort-Object Name | Format-Table Name, Size, CompressedSize, Type, Offset -AutoSize
}
finally {
    $stream.Dispose()
}
