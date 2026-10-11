param(
    [switch]$RepairHead
)

$ErrorActionPreference = "Stop"

$root = (Get-Location).Path
$contentPath = Join-Path $root "Content"

if (!(Test-Path -LiteralPath (Join-Path $root ".git"))) {
    throw "Run this script from the Git repository root."
}

if (!(Test-Path -LiteralPath $contentPath)) {
    throw "Content directory not found."
}

$lfsObjects = (& git rev-parse --git-path lfs/objects).Trim()

if (!$lfsObjects) {
    throw "Could not resolve the Git LFS object directory."
}

if (![System.IO.Path]::IsPathRooted($lfsObjects)) {
    $lfsObjects = Join-Path $root $lfsObjects
}

$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$backupRoot = Join-Path $env:TEMP "GOCLEAN_LFS_Backup_$timestamp"
$reportPath = Join-Path $env:TEMP "GOCLEAN_LFS_Report_$timestamp.csv"

$assetExtensions = @(
    ".uasset", ".umap", ".uexp", ".ubulk",
    ".uptnl", ".ushaderbytecode"
)

$results = [System.Collections.Generic.List[object]]::new()
$files = @(Get-ChildItem -LiteralPath $contentPath -File -Recurse -Force)

Write-Host "Scan root: $contentPath"
Write-Host "LFS objects: $lfsObjects"

if ($RepairHead) {
    Write-Host "Mode: REPAIR" -ForegroundColor Yellow
}
else {
    Write-Host "Mode: SCAN ONLY" -ForegroundColor Cyan
}

$index = 0

foreach ($file in $files) {
    $index++

    if ($index % 500 -eq 0) {
        Write-Progress `
            -Activity "Scanning Content files" `
            -Status "$index / $($files.Count)" `
            -PercentComplete (
                100 * $index / [Math]::Max(1, $files.Count)
            )
    }

    $isAsset = $assetExtensions -contains $file.Extension.ToLowerInvariant()

    $stream = [System.IO.File]::OpenRead($file.FullName)

    try {
        $buffer = New-Object byte[] 4096
        $read = $stream.Read($buffer, 0, $buffer.Length)
        $prefix = [System.Text.Encoding]::ASCII.GetString(
            $buffer, 0, $read
        )
    }
    finally {
        $stream.Dispose()
    }

    $isLfsPointer = $prefix.StartsWith(
        "version https://git-lfs.github.com/spec/v1"
    )

    $hasConflict = $isAsset -and (
        $prefix -match '(?m)^(<<<<<<<|=======|>>>>>>>)'
    )

    if (!$isLfsPointer -and !$hasConflict) {
        continue
    }

    $status = ""
    $detail = ""
    $oid = ""
    $expectedSize = -1L

    if ($file.Length -gt 1MB) {
        $status = "MANUAL_CHECK"
        $detail = "Suspicious file is larger than 1 MB."
    }
    else {
        $body = [System.IO.File]::ReadAllText($file.FullName)

        if ($hasConflict) {
            $headMatch = [regex]::Match(
                $body,
                '(?s)<<<<<<<[^\r\n]*\r?\n(.*?)======='
            )

            if ($headMatch.Success) {
                $headText = $headMatch.Groups[1].Value

                $oidMatch = [regex]::Match(
                    $headText,
                    '(?m)^oid sha256:([0-9a-fA-F]{64})\s*$'
                )

                $sizeMatch = [regex]::Match(
                    $headText,
                    '(?m)^size (\d+)\s*$'
                )

                if ($oidMatch.Success -and $sizeMatch.Success) {
                    $oid = $oidMatch.Groups[1].Value.ToLowerInvariant()
                    $expectedSize = [long]$sizeMatch.Groups[1].Value
                }
                else {
                    $status = "MANUAL_CHECK"
                    $detail = "Could not parse the HEAD candidate."
                }
            }
            else {
                $status = "MANUAL_CHECK"
                $detail = "Could not parse conflict markers."
            }
        }
        elseif ($isLfsPointer) {
            $oidMatch = [regex]::Match(
                $body,
                '(?m)^oid sha256:([0-9a-fA-F]{64})\s*$'
            )

            $sizeMatch = [regex]::Match(
                $body,
                '(?m)^size (\d+)\s*$'
            )

            if ($oidMatch.Success -and $sizeMatch.Success) {
                $oid = $oidMatch.Groups[1].Value.ToLowerInvariant()
                $expectedSize = [long]$sizeMatch.Groups[1].Value
            }
            else {
                $status = "MANUAL_CHECK"
                $detail = "Could not parse the LFS pointer."
            }
        }
    }

    if ($status -eq "" -and $oid) {
        $objectPath = Join-Path $lfsObjects (
            "$($oid.Substring(0,2))/$($oid.Substring(2,2))/$oid"
        )

        if (!(Test-Path -LiteralPath $objectPath -PathType Leaf)) {
            $status = "OBJECT_MISSING"
            $detail = "LFS object is missing locally."
        }
        else {
            $objectFile = Get-Item -LiteralPath $objectPath

            $actualHash = (
                Get-FileHash -LiteralPath $objectPath -Algorithm SHA256
            ).Hash.ToLowerInvariant()

            if ($actualHash -ne $oid) {
                $status = "HASH_MISMATCH"
                $detail = "LFS object SHA-256 mismatch."
            }
            elseif ($objectFile.Length -ne $expectedSize) {
                $status = "SIZE_MISMATCH"
                $detail = "LFS object size mismatch."
            }
            else {
                $objectStream = [System.IO.File]::OpenRead($objectPath)

                try {
                    $objectBuffer = New-Object byte[] 4096
                    $objectRead = $objectStream.Read(
                        $objectBuffer, 0, $objectBuffer.Length
                    )

                    $objectPrefix = [System.Text.Encoding]::ASCII.GetString(
                        $objectBuffer, 0, $objectRead
                    )
                }
                finally {
                    $objectStream.Dispose()
                }

                $objectIsPointer = $objectPrefix.StartsWith(
                    "version https://git-lfs.github.com/spec/v1"
                )

                $objectHasConflict = $objectPrefix -match (
                    '(?m)^(<<<<<<<|=======|>>>>>>>)'
                )

                if ($objectIsPointer -or $objectHasConflict) {
                    $status = "OBJECT_IS_TEXT"
                    $detail = "LFS object contains pointer or conflict text."
                }
                elseif ($RepairHead) {
                    $relativePath = $file.FullName.Substring(
                        $contentPath.Length
                    )

                    while (
                        $relativePath.StartsWith("\") -or
                        $relativePath.StartsWith("/")
                    ) {
                        $relativePath = $relativePath.Substring(1)
                    }

                    $backupPath = Join-Path $backupRoot $relativePath
                    $backupDirectory = Split-Path -Parent $backupPath

                    New-Item -ItemType Directory `
                        -Path $backupDirectory -Force | Out-Null

                    Copy-Item -LiteralPath $file.FullName `
                        -Destination $backupPath -Force

                    Copy-Item -LiteralPath $objectPath `
                        -Destination $file.FullName -Force

                    $status = if ($hasConflict) {
                        "RESTORED_HEAD"
                    }
                    else {
                        "RESTORED_OBJECT"
                    }

                    $detail = "Restored from verified LFS object."
                }
                else {
                    $status = if ($hasConflict) {
                        "READY_HEAD"
                    }
                    else {
                        "READY_OBJECT"
                    }

                    $detail = "Object hash and size verified."
                }
            }
        }
    }

    $results.Add([PSCustomObject]@{
        File         = $file.FullName
        Size         = $file.Length
        HasLfs       = $isLfsPointer
        HasConflict  = $hasConflict
        CandidateOID = $oid
        Status       = $status
        Detail       = $detail
    })

    Write-Host "[$status] $($file.FullName)"
}

Write-Progress -Activity "Scanning Content files" -Completed

$results | Export-Csv -LiteralPath $reportPath `
    -NoTypeInformation -Encoding UTF8

Write-Host ""
Write-Host "========== SUMMARY =========="
Write-Host "Files scanned: $($files.Count)"
Write-Host "Findings: $($results.Count)"
Write-Host "Report: $reportPath"

$results |
    Group-Object Status |
    Sort-Object Name |
    ForEach-Object {
        Write-Host ("{0}: {1}" -f $_.Name, $_.Count)
    }

if ($RepairHead) {
    Write-Host "Backup directory: $backupRoot"
}
else {
    Write-Host "Scan only. No files were modified."
}