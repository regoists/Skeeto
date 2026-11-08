# Static x86 PE reader. No LoadLibrary, Add-Type or DLL execution.
function Get-SktPeImage([string]$Path) {
    $item=Get-Item -LiteralPath $Path
    if ($item.Length -lt 256 -or $item.Length -gt 64MB) { throw (Get-SktText "PE 文件尺寸无效：$Path" "Invalid PE file size: $Path") }
    [byte[]]$data=[IO.File]::ReadAllBytes($Path)
    function Read-U16([int64]$Offset) {
        if ($Offset -lt 0 -or $Offset -gt $data.Length-2) { throw (Get-SktText 'PE WORD 越界。' 'PE WORD is out of bounds.') }
        return [BitConverter]::ToUInt16($data,[int]$Offset)
    }
    function Read-U32([int64]$Offset) {
        if ($Offset -lt 0 -or $Offset -gt $data.Length-4) { throw (Get-SktText 'PE DWORD 越界。' 'PE DWORD is out of bounds.') }
        return [BitConverter]::ToUInt32($data,[int]$Offset)
    }
    if ((Read-U16 0) -ne 0x5a4d) { throw (Get-SktText '文件不是 MZ。' 'The file has no MZ signature.') }
    [int64]$nt=Read-U32 60
    if ($nt -lt 64 -or (Read-U32 $nt) -ne 0x4550 -or (Read-U16 ($nt+4)) -ne 0x14c -or
        ((Read-U16 ($nt+22)) -band 0x2000) -eq 0 -or (Read-U16 ($nt+24)) -ne 0x10b) { throw (Get-SktText '文件不是有效 x86 PE DLL。' 'The file is not a valid x86 PE DLL.') }
    $sections=Read-U16 ($nt+6)
    $optionalSize=Read-U16 ($nt+20)
    if ($sections -lt 1 -or $sections -gt 96 -or $optionalSize -lt 104 -or (Read-U32 ($nt+116)) -lt 1) { throw (Get-SktText 'PE 节/可选头无效。' 'Invalid PE section or optional header.') }
    $table=$nt+24+$optionalSize
    if ($table+40*$sections -gt $data.Length) { throw (Get-SktText 'PE 节表越界。' 'PE section table is out of bounds.') }
    function Resolve-Rva([uint32]$Rva,[uint32]$Count=1) {
        for ($i=0; $i -lt $sections; ++$i) {
            $section=$table+40*$i
            [uint64]$start=Read-U32 ($section+12)
            [uint64]$size=Read-U32 ($section+16)
            if ([uint64]$Rva -ge $start -and [uint64]$Rva+$Count -le $start+$size) {
                [uint64]$raw=Read-U32 ($section+20)
                [uint64]$offset=$raw+$Rva-$start
                if ($offset+$Count -gt $data.Length) { throw (Get-SktText 'PE RVA 越界。' 'PE RVA is out of bounds.') }
                return [int]$offset
            }
        }
        throw (Get-SktText 'PE RVA 无对应文件数据。' 'PE RVA has no matching file data.')
    }
    $export=Resolve-Rva (Read-U32 ($nt+120)) 40
    $base=Read-U32 ($export+16)
    $functions=Read-U32 ($export+20)
    $namesCount=Read-U32 ($export+24)
    if ($functions -gt 4096 -or $namesCount -gt $functions -or $functions -lt 1) { throw (Get-SktText 'PE 导出数量无效。' 'Invalid PE export count.') }
    $addresses=Resolve-Rva (Read-U32 ($export+28)) ($functions*4)
    $names=@{}
    if ($namesCount) {
        $namesTable=Resolve-Rva (Read-U32 ($export+32)) ($namesCount*4)
        $ordinals=Resolve-Rva (Read-U32 ($export+36)) ($namesCount*2)
        for ($i=0; $i -lt $namesCount; ++$i) {
            $ordinal=Read-U16 ($ordinals+2*$i)
            if ($ordinal -ge $functions) { throw (Get-SktText 'PE 名称序号无效。' 'Invalid PE name ordinal.') }
            $offset=Resolve-Rva (Read-U32 ($namesTable+4*$i))
            $length=0
            while ($offset+$length -lt $data.Length -and $length -lt 256 -and $data[$offset+$length]) { ++$length }
            if ($length -eq 256 -or $offset+$length -ge $data.Length) { throw (Get-SktText 'PE 导出名未终止。' 'Unterminated PE export name.') }
            $name=[Text.Encoding]::ASCII.GetString($data,$offset,$length)
            if (-not $names.ContainsKey([int]$ordinal)) { $names[[int]$ordinal]=@() }
            $names[[int]$ordinal]+=$name
        }
    }
    for ($i=0; $i -lt $functions; ++$i) {
        if ((Read-U32 ($addresses+4*$i)) -eq 0) { continue }
        if ($names.ContainsKey($i)) {
            foreach ($name in $names[$i]) { [pscustomobject]@{ordinal=([uint64]$base+$i); name=$name} }
        } else { [pscustomobject]@{ordinal=([uint64]$base+$i); name=''} }
    }
}
function Get-SktPeExports([string]$Path) {
    Get-SktPeImage $Path | ForEach-Object { if ($_.name) { $_.name } }
}
function Get-SktPeCatalog([string]$Path) {
    Get-SktPeImage $Path | Sort-Object ordinal,name | ForEach-Object { [string]$_.ordinal + ':' + $_.name }
}
function Assert-SktWininetExports($Proxy, $System) {
    # Named thunks resolve by name, not the build machine's ordinal. An older
    # system may legitimately lack newer exports; every API it does provide
    # must be covered. Ordinal-only exports retain their exact identity.
    $available=[Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
    foreach ($entry in $Proxy) {
        $key=if ($entry.name) { 'name:' + $entry.name } else { 'ordinal:' + $entry.ordinal }
        $available.Add($key) | Out-Null
    }
    if ($available.Count -lt 100) { throw (Get-SktText 'WinINet 代理导出数量无效。' 'Invalid WinINet proxy export count.') }
    foreach ($entry in $System) {
        $key=if ($entry.name) { 'name:' + $entry.name } else { 'ordinal:' + $entry.ordinal }
        if (-not $available.Contains($key)) {
            throw (Get-SktText "代理未覆盖当前 Windows 的 WinINet 导出：$key" "The proxy does not cover this Windows WinINet export: $key")
        }
    }
}
function Test-SktNativeNecola([string]$Path) {
    $names=@(Get-SktPeExports $Path)
    return '?necola_export@@YAXXZ' -cin $names -and 'SkeetoBootstrapQuery' -cnotin $names -and 'necola_export' -cnotin $names
}
