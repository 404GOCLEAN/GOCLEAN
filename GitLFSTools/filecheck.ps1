$files = @(
  "Content/Assets/Prob/Item_Player/Mop/M_Mop.uasset",
  "Content/Hyeona/BP_GameSessionMode.uasset"
)
foreach ($file in $files) {
    if (Test-Path $file) {
        $stream = [System.IO.File]::OpenRead(
            (Resolve-Path $file)
        )
        try {
            $bytes = New-Object byte[] 100
            $count = $stream.Read($bytes, 0, 100)
            $header = [System.Text.Encoding]::ASCII.GetString(
                $bytes, 0, $count
            )
            if ($header.StartsWith(
                "version https://git-lfs.github.com/spec/v1"
            )) {
                Write-Host "LFS pointer was founded!: $file"
            } else {
                Write-Host "not pointer: $file"
            }
        }
        finally {
            $stream.Dispose()
        }
    } else {
        Write-Host "no file: $file"
    }
}
