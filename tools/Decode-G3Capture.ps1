param(
    [Parameter(Mandatory = $true)]
    [string]$Path
)

$rows = foreach ($line in Get-Content -LiteralPath $Path) {
    if ($line -notmatch '^(?<ms>\d+),RX,(?<check>OK|BAD),(?<len>\d+),(?<hex>[0-9A-Fa-f ]+)$') {
        continue
    }
    $bytes = @($Matches.hex -split ' ' | ForEach-Object { [Convert]::ToByte($_, 16) })
    if ($bytes.Count -lt 9) { continue }
    [pscustomobject]@{
        TimeMs   = [int64]$Matches.ms
        Checksum = $Matches.check
        Length   = [int]$Matches.len
        Source   = ('0x{0:X2}' -f $bytes[3])
        Target   = ('0x{0:X2}' -f $bytes[4])
        Command  = ('0x{0:X2}' -f $bytes[5])
        Data     = (($bytes[6..($bytes.Count - 3)] | ForEach-Object { '{0:X2}' -f $_ }) -join ' ')
    }
}

$rows | Group-Object Source, Target, Command, Length | Sort-Object Count -Descending |
    ForEach-Object {
        $sample = $_.Group[0]
        [pscustomobject]@{
            Count    = $_.Count
            Source   = $sample.Source
            Target   = $sample.Target
            Command  = $sample.Command
            Length   = $sample.Length
            Sample   = $sample.Data
        }
    } | Format-Table -AutoSize

