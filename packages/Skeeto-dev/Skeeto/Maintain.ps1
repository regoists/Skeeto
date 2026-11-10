# PowerShell 5.1. One-shot maintenance only; never loaded in the game process.
[CmdletBinding()]
param(
    [ValidateSet('Migrate','Apply','Uninstall','Validate','Prepare','Clean')][string]$Mode = 'Migrate',
    [string]$GameRoot,
    [string]$PayloadRoot,
    [int]$UpdateProtocol = 0,
    [int]$WaitPid = 0,
    [long]$WaitStartTime = 0,
    [string]$StartedEvent,
    [switch]$AdoptExistingProxy,
    [switch]$InteractiveMenu,
    [ValidateSet("auto","zh","en")][string]$Language = "auto",
    [switch]$NonInteractive
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
# Match the released installer's Chinese culture detection once, outside
# all game threads. Other locales use
# English; -Language zh/en allows a deliberate override and focused checks.
$script:SktLanguage = $Language
if ($Language -eq 'auto') {
    $script:SktLanguage = if ([Globalization.CultureInfo]::CurrentCulture.TwoLetterISOLanguageName -eq 'zh' -or [Globalization.CultureInfo]::InstalledUICulture.TwoLetterISOLanguageName -eq 'zh') { 'zh' } else { 'en' }
}
# Also keep native .NET diagnostic details in the selected UI language.
[Threading.Thread]::CurrentThread.CurrentUICulture = [Globalization.CultureInfo]::GetCultureInfo($(if ($script:SktLanguage -eq 'zh') { 'zh-CN' } else { 'en-US' }))
function Get-SktText([string]$Zh,[string]$En) {
    if ($script:SktLanguage -eq 'zh') { return $Zh }
    return $En
}
if ([string]::IsNullOrWhiteSpace($PayloadRoot)) { $PayloadRoot = Split-Path -Parent $PSScriptRoot }
$script:PackageLayout = 2
$script:Allowed = @('necola.dll','Skeeto\Manifest.json',
    'left4dead2_skeeto.exe','start_skeeto.bat','start_skeeto_with_update.bat','start_necola_with_update.bat','skeeto.ini',
    'skeeto_ws_show_changelog.txt','skeeto_ws_changelog_shown.txt','skeeto_update\READY')
$script:PayloadFiles = @(
    'skeeto.dll',
    'bin\wininet.dll',
    'VERSION.txt',
    'SKEETO_WORKSHOP.txt',
    'skeeto_apply_update.bat',
    'Setup_安装 维护 卸载.bat',
    'Skeeto\Maintain.ps1',
    'Skeeto\README.txt',
    'Skeeto\Commands.txt',
    'Skeeto_更新日志.txt',
    'Skeeto\DIY\Background_主菜单背景.txt',
    'Skeeto\DIY\Menu_主菜单UI.txt',
    'Skeeto\DIY\ESCMenu_ESC菜单UI.txt',
    'Skeeto\DIY\Plugins_插件平台.txt',
    'Skeeto\DIY\Crosshair_贴图准星.txt',
    'Skeeto\DIY\HitFeedback_命中反馈.txt',
    'Skeeto\DIY\Agent Skills\README.txt',
    'Skeeto\DIY\Agent Skills\skeeto-hit-feedback\SKILL.md',
    'Skeeto\DIY\Agent Skills\skeeto-hit-feedback\scripts\validate_feedback.py',
    'Skeeto\DIY\Examples 命中反馈json示例\skeeto_ci_my_random_sfx.json',
    'Skeeto\DIY\Examples 命中反馈json示例\skeeto_ci_my_sequence.json',
    'Skeeto\DIY\Examples 命中反馈json示例\skeeto_ci_my_style.json',
    'Skeeto\DIY\Examples 命中反馈json示例\skeeto_ff_my_style.json',
    'Skeeto\DIY\Examples 命中反馈json示例\skeeto_fx_my_style.json',
    'Skeeto\DIY\Examples 命中反馈json示例\skeeto_si_my_simple.json',
    'Skeeto\DIY\Examples 命中反馈json示例\skeeto_si_my_streak.json',
    'Skeeto\DIY\Examples 命中反馈json示例\skeeto_si_my_world.json',
    'Skeeto\PluginSDK\README.md',
    'Skeeto\PluginSDK\include\skeeto_plugin_api.h',
    'Skeeto\PluginSDK\examples\HelloMenu\plugin.cpp',
    'Skeeto\PluginSDK\examples\HelloMenu\build.bat')
$script:Allowed += $script:PayloadFiles
$script:Retired = @('left4dead2_skeeto.exe','start_skeeto.bat','start_skeeto_with_update.bat','start_necola_with_update.bat','skeeto.ini')
$script:MaintenanceMutex = $null
$script:MaintenanceLocked = $false

function Write-SktMessage([string]$Message,[switch]$Failure,[ConsoleColor]$Color='Gray') {
    Write-Host $Message -ForegroundColor $(if ($Failure) { 'Red' } else { $Color })
}
function Stop-SktConflict([string]$Message) {
    # Existing user edits/foreign targets need action, not an endless retry loop.
    if ($Mode -in @('Prepare','Apply')) { Write-SktMessage $Message -Failure; exit 2 }
    throw $Message
}
# One writer (including its wait for exit) per installation. No game thread
# waits on this lock; duplicate helpers exit and the DLL monitors the owner.
function Enter-SktMaintenance([string]$Root) {
    $sha=[Security.Cryptography.SHA256]::Create()
    try { $key=[BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::Unicode.GetBytes($Root.ToUpperInvariant()))).Replace('-','') }
    finally { $sha.Dispose() }
    $script:MaintenanceMutex=[Threading.Mutex]::new($false,('Local\SkeetoMaintenance.v1.'+$key))
    try { $script:MaintenanceLocked=$script:MaintenanceMutex.WaitOne(0) }
    catch [Threading.AbandonedMutexException] { $script:MaintenanceLocked=$true }
    if (-not $script:MaintenanceLocked) { throw (Get-SktText '另一维护助手正在工作，将稍后重试。' 'Another maintenance helper is active; retry later.') }
}
# Flush the recovery control record before touching any target. Fixed temporary
# name is owned by the exclusive maintenance writer; interrupted writes are safe.
function Write-SktControl([string]$Path,[string]$Text) {
    $temporary=$Path+'.tmp'
    Get-SktPath ([IO.Path]::GetDirectoryName($Path)) ([IO.Path]::GetFileName($temporary)) -Internal | Out-Null
    $bytes=[Text.Encoding]::UTF8.GetBytes($Text)
    $stream=[IO.File]::Open($temporary,[IO.FileMode]::Create,[IO.FileAccess]::Write,[IO.FileShare]::None)
    try { $stream.Write($bytes,0,$bytes.Length); $stream.Flush($true) } finally { $stream.Dispose() }
    if ([IO.File]::Exists($Path)) { [IO.File]::Replace($temporary,$Path,[NullString]::Value) }
    else { [IO.File]::Move($temporary,$Path) }
}
function Get-SktInterrupted([string]$Root) {
    $active=Get-SktPath $Root 'skeeto_migrate_backup\active.json' -Internal
    if (-not [IO.File]::Exists($active)) { return $null }
    $pointer=Read-SktJson $active
    if ($pointer.id -notmatch '^(current|(update-)?\d{8}T\d{6}-[a-f0-9]{32})$') { throw 'Invalid active transaction ID.' }
    $directory=Get-SktPath $Root ('skeeto_migrate_backup\'+$pointer.id) -Internal
    $path=Get-SktPath $directory 'transaction.json' -Internal
    if ((Get-SktHash $path) -cne $pointer.journalHash) { throw 'Active transaction journal hash mismatch.' }
    $journal=Read-SktJson $path
    if ($journal.schema -ne 1 -or $journal.gameRoot -ine $Root -or @($journal.entries).Count -gt 256) { throw 'Invalid active transaction journal.' }
    $seen=@{}
    foreach ($entry in $journal.entries) {
        if ($seen.ContainsKey($entry.relative) -or $entry.relative -in @('skeeto_setting.ini','current.hud','left4dead2.exe') -or
            ($entry.before -and $entry.before -notmatch '^[a-f0-9]{64}$') -or ($entry.after -and $entry.after -notmatch '^[a-f0-9]{64}$')) { throw 'Invalid active transaction entry.' }
        $seen[$entry.relative]=$true
        Get-SktPath $Root $entry.relative -Internal | Out-Null
    }
    $script:Allowed+=@($journal.entries | ForEach-Object relative)
    return [pscustomobject]@{active=$active;directory=$directory;journal=$journal}
}
function Repair-SktInterrupted($Pending) {
    if (-not $Pending) { return }
    Assert-SktStopped
    $complete=$true
    foreach ($entry in $Pending.journal.entries) {
        if ($entry.relative -eq 'skeeto_update\READY') { continue }
        if ((Get-SktHash (Get-SktPath $Pending.journal.gameRoot $entry.relative)) -ne $entry.after) { $complete=$false; break }
    }
    if ($complete) {
        $outcome='committed'
        $ready=Get-SktPath $Pending.journal.gameRoot 'skeeto_update\READY'
        if ([IO.File]::Exists($ready)) {
            $receipt=Read-SktJson $ready
            $manifest=@($Pending.journal.entries | Where-Object relative -eq 'Skeeto\Manifest.json')
            if ($manifest.Count -eq 1 -and $receipt.manifestHash -eq $manifest[0].after) { [IO.File]::Delete($ready) }
        }
    }
    else { Restore-SktTransaction $Pending.journal $Pending.directory; $outcome='rolled-back' }
    Write-SktControl (Join-Path $Pending.directory 'result.txt') $outcome
    [IO.File]::Delete($Pending.active)
    Write-SktMessage (Get-SktText "已恢复未完成事务：$outcome" "Recovered interrupted transaction: $outcome")
}
# Recovery data exists only until the operation commits or is restored.
function Remove-SktOwnedTransactionFiles([string]$Directory,[array]$Names) {
    foreach ($child in [IO.Directory]::EnumerateDirectories($Directory)) { return }
    $paths=@()
    foreach ($file in [IO.Directory]::EnumerateFiles($Directory)) {
        if ($paths.Count -ge 260 -or [IO.Path]::GetFileName($file) -notin $Names) { return }
        $paths+=Get-SktPath $Directory ([IO.Path]::GetFileName($file)) -Internal
    }
    # Keep ownership until every snapshot has been removed. A transient lock
    # must not leave orphan snapshots that the next retry cannot recognize.
    foreach ($path in $paths) {
        if ([IO.Path]::GetFileName($path) -ne 'transaction.json') { [IO.File]::Delete($path) }
    }
    foreach ($path in $paths) {
        if ([IO.Path]::GetFileName($path) -eq 'transaction.json') { [IO.File]::Delete($path) }
    }
    [IO.Directory]::Delete($Directory)
}
function Remove-SktOldUpdates([string]$Root,[string]$Keep,[string]$OnlyId='') {
    $base=Get-SktPath $Root 'skeeto_migrate_backup' -Internal
    if (-not [IO.Directory]::Exists($base) -or [IO.File]::Exists((Join-Path $base 'active.json'))) { return }
    $count=0
    # Filter by operation namespace before applying the budget. Arbitrarily
    # many manual migration folders must never starve automatic cleanup.
    if (-not $OnlyId) {
        foreach ($scratchDir in [IO.Directory]::EnumerateDirectories($base,'scratch-*')) {
            if (++$count -gt 128) { break }
            $name=[IO.Path]::GetFileName($scratchDir)
            if ($name -notmatch '^scratch-[a-f0-9]{32}$') { continue }
            try { Remove-SktOwnedTransactionFiles (Get-SktPath $base $name -Internal) @('bridge.json','pending.txt') }
            catch { Write-Warning 'Interrupted scratch cleanup deferred.' }
        }
    }
    $count=0
    $directories=if($OnlyId){@(Join-Path $base $OnlyId)}else{@([IO.Directory]::EnumerateDirectories($base))}
    foreach ($directory in $directories) {
        if (-not [IO.Directory]::Exists($directory)) { continue }
        $id=[IO.Path]::GetFileName($directory)
        if ($OnlyId -and $id -ne $OnlyId) { continue }
        if ($id -eq $Keep -or $id -notmatch '^(current|(update-)?\d{8}T\d{6}-[a-f0-9]{32})$') { continue }
        try {
            $directory=Get-SktPath $base $id -Internal
            $journalPath=Get-SktPath $directory 'transaction.json' -Internal
            if (-not [IO.File]::Exists($journalPath)) {
                # Journal publication precedes every backup copy. A killed
                # writer here can leave only its journal temp (or an empty dir).
                Remove-SktOwnedTransactionFiles $directory @('transaction.json.tmp')
                continue
            }
            if (++$count -gt 128) { Write-Warning 'Recovery cleanup budget reached.'; break }
            $journal=Read-SktJson $journalPath
            $result=Get-SktPath $directory 'result.txt' -Internal
            if ($journal.schema -ne 1 -or $journal.gameRoot -ine $Root -or $journal.operation -notin @('Apply','Migrate','Uninstall')) { continue }
            if ([IO.File]::Exists($result) -and [IO.File]::ReadAllText($result).Trim() -notin @('committed','rolled-back')) { continue }
            $names=@('transaction.json','result.txt','transaction.json.tmp','result.txt.tmp')
            foreach ($entry in $journal.entries) {
                if ($entry.backup) {
                    if ($entry.backup -notmatch '^original-[0-9]+\.bin$') { throw 'Invalid old backup name.' }
                    $names+=$entry.backup
                }
            }
            Remove-SktOwnedTransactionFiles $directory $names
        } catch { Write-Warning ("Old automatic backup cleanup deferred: "+$_.Exception.Message) }
    }
    if ([IO.Directory]::Exists($base) -and -not @([IO.Directory]::EnumerateFileSystemEntries($base)).Count) {
        try { [IO.Directory]::Delete($base) } catch { Write-Warning 'Empty recovery directory cleanup deferred.' }
    }
}
function Get-SktHash([string]$Path) {
    if (-not [IO.File]::Exists($Path)) { return $null }
    $algorithm = [Security.Cryptography.SHA256]::Create()
    $stream = [IO.File]::OpenRead($Path)
    try { return [BitConverter]::ToString($algorithm.ComputeHash($stream)).Replace('-','').ToLowerInvariant() }
    finally { $stream.Dispose(); $algorithm.Dispose() }
}
function Get-SktPath([string]$Root, [string]$Relative, [switch]$Internal, [switch]$LexicalOnly) {
    if (-not $Internal -and $Relative -cnotin $script:Allowed) { throw (Get-SktText "非维护文件：$Relative" "Not a managed file: $Relative") }
    $base = [IO.Path]::GetFullPath($Root).TrimEnd('\')
    $path = [IO.Path]::GetFullPath((Join-Path $base $Relative))
    if (-not $path.StartsWith($base + '\', [StringComparison]::OrdinalIgnoreCase)) { throw (Get-SktText '文件越过目标目录。' 'File path escapes the target directory.') }
    if ($LexicalOnly) { return $path } # Metadata only; never use this result for IO.
    $check = $path
    while ($check.Length -ge $base.Length) {
        if (Test-Path -LiteralPath $check) {
            if ((Get-Item -LiteralPath $check -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) { throw (Get-SktText "不操作链接/重解析点：$check" "Refusing a link or reparse point: $check") }
        }
        if ($check -eq $base) { break }
        $check = [IO.Path]::GetDirectoryName($check)
    }
    return $path
}
function Test-SktGameRoot([string]$Path) {
    return $Path -and [IO.File]::Exists((Join-Path $Path 'left4dead2.exe')) -and
        [IO.File]::Exists((Join-Path $Path 'bin\engine.dll')) -and [IO.Directory]::Exists((Join-Path $Path 'left4dead2'))
}
function Find-SktGameRoot {
    if ($GameRoot) {
        $root = (Resolve-Path -LiteralPath $GameRoot).ProviderPath.TrimEnd('\')
        if (-not (Test-SktGameRoot $root)) { throw (Get-SktText '目标目录缺少 left4dead2.exe、bin\engine.dll 或 left4dead2 文件夹。' 'The target needs left4dead2.exe, bin\engine.dll and the left4dead2 folder.') }
        return $root
    }
    $candidate = $PayloadRoot
    for ($i=0; $candidate -and $i -lt 6; ++$i) {
        if (Test-SktGameRoot $candidate) { return [IO.Path]::GetFullPath($candidate).TrimEnd('\') }
        $candidate = [IO.Path]::GetDirectoryName($candidate.TrimEnd('\'))
    }
    $libraries = @()
    foreach ($key in @('HKCU:\Software\Valve\Steam','HKLM:\SOFTWARE\WOW6432Node\Valve\Steam')) {
        $reg = Get-ItemProperty -LiteralPath $key -ErrorAction SilentlyContinue
        if ($reg) {
            foreach ($field in @('SteamPath','InstallPath')) {
                $property = $reg.PSObject.Properties[$field]
                if ($property -and $property.Value) { $libraries += [string]$property.Value }
            }
        }
    }
    foreach ($library in @($libraries)) {
        $vdf = Join-Path $library 'steamapps\libraryfolders.vdf'
        if ([IO.File]::Exists($vdf) -and (Get-Item -LiteralPath $vdf).Length -lt 1MB) {
            foreach ($match in [regex]::Matches([IO.File]::ReadAllText($vdf), '"path"\s+"([^"]+)"')) {
                $libraries += $match.Groups[1].Value.Replace('\\','\')
            }
        }
    }
    # Registry and libraryfolders.vdf can spell the same Windows path differently.
    $seenRoots = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    $roots = @(foreach ($library in $libraries) {
        $root = [IO.Path]::GetFullPath((Join-Path $library 'steamapps\common\Left 4 Dead 2')).TrimEnd('\')
        if ($seenRoots.Add($root) -and (Test-SktGameRoot $root)) { $root }
    })
    if ($roots.Count -eq 1) { return [IO.Path]::GetFullPath($roots[0]).TrimEnd('\') }
    if ($NonInteractive) { throw (Get-SktText '无法唯一定位游戏目录，请传入 -GameRoot。' 'Could not select a unique game directory. Specify -GameRoot.') }
    if ($roots.Count -gt 1) { Write-SktMessage ((Get-SktText "发现多个游戏库，请选择目标：`r`n" "Multiple game libraries found. Select the target:`r`n") + ($roots -join "`r`n")) }
    $chosen = (Read-Host (Get-SktText '请输入或拖入游戏根目录（含 left4dead2.exe），然后回车' 'Type or drag in the game root containing left4dead2.exe, then press Enter')).Trim('"',' ')
    if (-not (Test-SktGameRoot $chosen)) { throw (Get-SktText '未找到有效游戏目录。' 'No valid game directory was found.') }
    return (Resolve-Path -LiteralPath $chosen).ProviderPath.TrimEnd('\')
}
function Test-SktGamesRunning {
    return @(Get-Process -Name left4dead2,left4dead2_skeeto,left4dead2_necola -ErrorAction SilentlyContinue).Count -gt 0
}
function Assert-SktStopped {
    if (Test-SktGamesRunning) {
        throw (Get-SktText '请完全退出游戏及旧启动器后重试；脚本不会结束进程。' 'Exit the game and legacy launchers before retrying. This tool will not terminate them.')
    }
}
function New-SktDeferException([string]$Code,[string]$Zh,[string]$En) {
    return [InvalidOperationException]::new(('DEFER:' + $Code + '|' + (Get-SktText $Zh $En)))
}
function New-SktOccupancyException([string]$Relative) {
    return [IO.IOException]::new(('OCCUPANCY:' + $Relative + '|' + (Get-SktText ("无法独占打开关键文件，整笔更新中止（未改已装文件，READY 保留）：$Relative") ("Cannot exclusively open critical file; whole update aborted (installed files unchanged, READY kept): $Relative"))))
}
function Write-SktApplyOutcome([string]$Root,[string]$Code,[string]$Detail='') {
    if (-not $Root) { return }
    $line = if ($Detail) { "$Code|$Detail" } else { $Code }
    try { Write-SktControl (Get-SktPath $Root 'skeeto_update\apply_result.txt' -Internal) $line }
    catch { Write-Warning 'apply_result write deferred.' }
}
function Wait-SktExclusiveOpen([string]$Path,[string]$Relative,[int]$TimeoutSeconds = 90) {
    $deadline = [DateTime]::UtcNow.AddSeconds([Math]::Max(1, $TimeoutSeconds))
    $delay = 400
    while ($true) {
        try {
            $stream = [IO.File]::Open($Path,[IO.FileMode]::Open,[IO.FileAccess]::ReadWrite,[IO.FileShare]::None)
            $stream.Dispose()
            return
        } catch {
            if (Test-SktGamesRunning) { throw (New-SktDeferException 'GAME_RUNNING' '请完全退出游戏及旧启动器后重试；脚本不会结束进程。' 'Exit the game and legacy launchers before retrying. This tool will not terminate them.') }
            if ([DateTime]::UtcNow -ge $deadline) { throw (New-SktOccupancyException $Relative) }
            Start-Sleep -Milliseconds $delay
            $delay = [Math]::Min(8000, [int]($delay * 1.5))
        }
    }
}
function Test-SktCriticalRelative([string]$Relative) {
    return $Relative -eq 'skeeto.dll' -or $Relative -eq 'bin\wininet.dll'
}
function Set-SktFile([string]$Source,[string]$Target,$Before,[string]$After) {
    if ((Get-SktHash $Target) -ne $Before) { throw (Get-SktText "文件在操作前已改变：$Target" "File changed before the operation: $Target") }
    [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($Target)) | Out-Null
    # One reserved temporary per managed target, not one orphan per crash.
    $temp = $Target + '.skt-update.tmp'
    Get-SktPath ([IO.Path]::GetDirectoryName($temp)) ([IO.Path]::GetFileName($temp)) -Internal | Out-Null
    try {
        [IO.File]::Copy($Source,$temp,$true)
        if ((Get-SktHash $temp) -ne $After -or (Get-SktHash $Target) -ne $Before) { throw (Get-SktText "复制校验失败或目标已改变：$Target" "Copy verification failed or the target changed: $Target") }
        $flush=[IO.File]::Open($temp,[IO.FileMode]::Open,[IO.FileAccess]::ReadWrite,[IO.FileShare]::None)
        try { $flush.Flush($true) } finally { $flush.Dispose() }
        if ($null -eq $Before) { [IO.File]::Move($temp,$Target) }
        else { [IO.File]::Replace($temp,$Target,[NullString]::Value) }
    } finally { if ([IO.File]::Exists($temp)) { [IO.File]::Delete($temp) } }
}
function Read-SktJson([string]$Path) {
    if (-not [IO.File]::Exists($Path) -or (Get-Item -LiteralPath $Path).Length -gt 2MB) { throw (Get-SktText "缺少或过大的维护清单：$Path" "Missing or oversized maintenance manifest: $Path") }
    return ([IO.File]::ReadAllText($Path) | ConvertFrom-Json)
}
function Get-SktRuntimeFiles([string]$Root,$State=$null) {
    foreach ($name in @('skeeto.dll','bin\wininet.dll')) {
        $path=Get-SktPath $Root $name
        if (-not [IO.File]::Exists($path)) { continue }
        $hash=Get-SktHash $path
        $known=$State -and @($State.files | Where-Object { $_.relative -ceq $name -and $_.hash -ceq $hash }).Count -eq 1
        $modern=$false
        if (-not $known) {
            try { $exports=@(Get-SktPeExports $path) }
            catch [IO.IOException] { throw }
            catch [UnauthorizedAccessException] { throw }
            catch { Write-SktMessage (Get-SktText "保留无法识别的文件：$name" "Keeping an unrecognized file: $name") -Color Yellow; continue }
            if ($name -eq 'bin\wininet.dll') {
                $known='SkeetoWininetProxyIdentity' -cin $exports
                if (-not $known -and 'InternetOpenA' -cin $exports -and 'InternetCloseHandle' -cin $exports) {
                    # Existing copies already have these three Skeeto-specific
                    # bootstrap strings. This identifies the user's actual
                    # pre-marker proxy, not any DLL merely named wininet.dll.
                    $text=[Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($path))
                    $known=$text.Contains("SkeetoBootstrapQuery`0") -and $text.Contains("reuse-core-owner`0") -and $text.Contains("core-load-failed`0")
                }
            } else {
                $known='SkeetoBootstrapQuery' -cin $exports -or 'necola_export' -cin $exports
                $modern='SkeetoBootstrapQuery' -cin $exports
            }
        }
        if ($known) {
            $role=if($name -eq 'skeeto.dll'){'core'}else{'proxy'}
            [pscustomobject]@{relative=$name;hash=$hash;role=$role;modern=$modern}
        } else { Write-SktMessage (Get-SktText "保留无法识别为 Skeeto 的文件：$name" "Keeping a file not identified as Skeeto: $name") -Color Yellow }
    }
}
function Test-SktKnownInstall([string]$Root,[string]$Relative,[string]$Role) {
    # A leftover ownership manifest is no longer maintained. It must not block
    # replacement of a file that still identifies as Skeeto.
    if ($Role -in @('marker','plugin','tool','document')) { return $true }
    $path=Get-SktPath $Root $Relative -Internal
    if (-not [IO.File]::Exists($path)) { return $true }
    try { $exports=@(Get-SktPeExports $path) }
    catch [IO.IOException] { throw }
    catch [UnauthorizedAccessException] { throw }
    catch { return $false }
    if ($Relative -eq 'bin\wininet.dll' -or $Role -eq 'proxy') {
        if ('SkeetoWininetProxyIdentity' -cin $exports) { return $true }
        if ('InternetOpenA' -cin $exports -and 'InternetCloseHandle' -cin $exports) {
            $text=[Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($path))
            return $text.Contains("SkeetoBootstrapQuery`0") -and $text.Contains("reuse-core-owner`0") -and $text.Contains("core-load-failed`0")
        }
        return $false
    }
    return ('SkeetoBootstrapQuery' -cin $exports -or 'necola_export' -cin $exports)
}
function Get-SktState([string]$Root) {
    $manifest=Get-SktPath $Root 'Skeeto\Manifest.json'
    if (-not [IO.File]::Exists($manifest)) {
        if ($Mode -in @('Prepare','Apply') -and $UpdateProtocol -eq 1) {
            # No ownership file is installed. A missing Steam proxy is still the
            # legacy bridge: do not treat it as a migrated install.
            if (-not [IO.File]::Exists((Get-SktPath $Root 'bin\wininet.dll' -Internal))) { return $null }
            $files=@(foreach ($name in @('skeeto.dll','bin\wininet.dll')) {
                [pscustomobject]@{relative=$name;hash=$null;role=$(if($name -eq 'skeeto.dll'){'core'}else{'proxy'})}
            })
            return [pscustomobject]@{schema=1;install_layout=2;gameRoot=$Root;semver=$null;files=$files}
        }
        $runtime=@(Get-SktRuntimeFiles $Root)
        $core=@($runtime | Where-Object role -eq 'core');$proxy=@($runtime | Where-Object role -eq 'proxy')
        if ($core.Count -ne 1 -or -not $core[0].modern -or $proxy.Count -ne 1) { return $null }
        # Root core plus proxy is the current runtime. No installed manifest
        # is required to prepare the next update. The DLL compares its
        # compiled version before calling this engine.
        return [pscustomobject]@{schema=1;install_layout=2;gameRoot=$Root;semver=$null;files=$runtime}
    }
    $payload=Read-SktJson $manifest
    if ($payload.schema -ne 1 -or $payload.product -cne 'Skeeto' -or $payload.semver -notmatch '^\d+\.\d+\.\d+$') {
        throw (Get-SktText '安装归属记录无效。' 'Invalid installation ownership record.')
    }
    $seen=@{}
    if (@($payload.files).Count -gt 128) { throw (Get-SktText '安装归属文件超过 128 个。' 'Installation ownership exceeds 128 files.') }
    foreach ($entry in $payload.files) {
        if ($seen.ContainsKey($entry.relative) -or $entry.role -cnotin @('core','plugin','proxy','marker','tool','document') -or
            $entry.relative -in @('necola.dll','necola_orig.dll','Skeeto\Manifest.json','skeeto_update\READY','skeeto_setting.ini','current.hud','left4dead2.exe') + $script:Retired -or
            ($entry.role -ne 'document' -and $entry.hash -notmatch '^[a-f0-9]{64}$')) { throw (Get-SktText '安装归属记录包含无效文件。' 'Invalid owned file entry.') }
        $seen[$entry.relative]=$true
        Get-SktPath $Root $entry.relative -Internal -LexicalOnly:($entry.role -eq 'document') | Out-Null
        # The installed ownership file is no longer updated. A stale hash must
        # not block repair of a file that still identifies as Skeeto. A file
        # that does not identify as Skeeto still stops the operation.
        if ($Mode -notin @('Prepare','Apply') -and $entry.role -in @('core','plugin','proxy','marker')) {
            $current=Get-SktHash (Get-SktPath $Root $entry.relative -Internal)
            if ($current -and $current -ne $entry.hash -and -not (Test-SktKnownInstall $Root $entry.relative $entry.role)) {
                Stop-SktConflict (Get-SktText "安装文件与归属清单不符，已停止：$($entry.relative)" "Installed file does not match ownership; stopped: $($entry.relative)")
            }
        }
    }
    if (@($payload.files | Where-Object role -eq 'core').Count -ne 1) { throw (Get-SktText '安装归属记录必须包含一个核心。' 'Ownership requires exactly one core.') }
    # The old installation's table is independent of the new package's layout.
    $script:Allowed += @($payload.files | ForEach-Object { $_.relative })
    $files=@($payload.files) + @([pscustomobject]@{relative='Skeeto\Manifest.json'; hash=(Get-SktHash $manifest); role='tool'})
    return [pscustomobject]@{schema=1; install_layout=$payload.install_layout; gameRoot=$Root; semver=$payload.semver; files=$files}
}
function Get-SktPayloadMetadata([string]$Root) {
    $manifestPath=Get-SktPath $Root 'Skeeto\Manifest.json'
    $manifestHash=Get-SktHash $manifestPath
    $metadata = Read-SktJson $manifestPath
    if ($metadata.schema -ne 1 -or $metadata.product -cne 'Skeeto' -or $metadata.update_protocol -ne 1 -or $metadata.install_layout -ne $script:PackageLayout -or $metadata.semver -notmatch '^\d+\.\d+\.\d+$') { throw (Get-SktText '此维护器只支持 layout=2 的有效包。' 'This tool requires a valid layout=2 package.') }
    if ($metadata.requires_manual_migrate -isnot [bool]) { throw (Get-SktText '手动迁移声明格式无效。' 'Invalid manual migration declaration.') }
    $seen = @{}
    $payloadAllowed = $script:PayloadFiles
    foreach ($entry in $metadata.files) {
        if ($seen.ContainsKey($entry.relative)) { throw (Get-SktText '包清单包含重复文件。' 'The package manifest contains duplicate files.') }
        $seen[$entry.relative] = $true
        if ($entry.relative -cnotin $payloadAllowed) { throw (Get-SktText '包内包含不属于此布局的文件。' 'The package includes a file outside this layout.') }
        Get-SktPath $Root $entry.relative -LexicalOnly | Out-Null
        if ($entry.role -cnotin @('core','plugin','proxy','marker','tool','document') -or ($entry.role -ne 'document' -and $entry.hash -notmatch '^[a-f0-9]{64}$')) { throw (Get-SktText '包文件哈希无效。' 'Invalid package file hash.') }
    }
    foreach ($required in $payloadAllowed) {
        if (-not $seen.ContainsKey($required)) { throw (Get-SktText "缺少必需维护文件：$required" "Missing required maintenance file: $required") }
    }
    $metadata | Add-Member -NotePropertyName manifestHash -NotePropertyValue $manifestHash
    return $metadata
}
function Get-SktPayload([string]$Root) {
    $metadata=Get-SktPayloadMetadata $Root
    foreach ($entry in $metadata.files) {
        if ($entry.role -eq 'document') { continue } # Optional content cannot gate core updates.
        $actual=Get-SktHash (Get-SktPath $Root $entry.relative)
        if ($actual -eq $entry.hash) { continue }
        # Apply only installs core/proxy into the game root. Stage-only tools
        # (Setup bat, etc.) must not fail a validated envelope after Prepare.
        if ($Mode -eq 'Apply' -and $entry.role -eq 'tool' -and $entry.relative -notin @('Skeeto\Maintain.ps1','Skeeto\Manifest.json')) {
            Write-SktMessage (Get-SktText "暂存工具文件校验跳过（不影响核心安装）：$($entry.relative)" "Stage tool hash skipped for Apply (core install unaffected): $($entry.relative)") -Color Yellow
            continue
        }
        throw (Get-SktText "包文件缺失或哈希不符：$($entry.relative)" "Package file missing or hash mismatch: $($entry.relative)")
    }
    $version = [IO.File]::ReadAllText((Get-SktPath $Root 'VERSION.txt'))
    $marker = [IO.File]::ReadAllText((Get-SktPath $Root 'SKEETO_WORKSHOP.txt'))
    if ($version -notmatch ('(?m)^semver=' + [regex]::Escape($metadata.semver) + '\r?$') -or
        $marker -notmatch '(?m)^SkeetoWorkshopPackage=1\r?$' -or $marker -notmatch ('(?m)^install_layout=' + [regex]::Escape([string]$metadata.install_layout) + '\r?$')) { throw (Get-SktText '版本和布局标记与清单不一致。' 'Version or layout markers do not match the manifest.') }
    $coreEntry=@($metadata.files | Where-Object role -eq 'core')
    if ($coreEntry.Count -ne 1) { throw (Get-SktText '包清单必须包含一个核心。' 'Package requires exactly one core.') }
    $proxyEntry=@($metadata.files | Where-Object role -eq 'proxy')
    if ($proxyEntry.Count -ne 1) { throw (Get-SktText '当前布局必须包含一个代理。' 'This layout requires exactly one proxy.') }
    if ($Mode -notin @('Prepare','Apply')) {
        # Release validation and explicit installation check the PE/export
        # catalog. Automatic updates verify the released bytes above.
        $core = @(Get-SktPeExports (Get-SktPath $Root $coreEntry[0].relative))
        if ('SkeetoBootstrapQuery' -cnotin $core -or 'necola_export' -cnotin $core) { throw (Get-SktText '核心不支持此次架构。' 'The core does not support this loading architecture.') }
        $proxy = @(Get-SktPeImage (Get-SktPath $Root $proxyEntry[0].relative))
        $system = Join-Path $env:WINDIR 'SysWOW64\wininet.dll'
        if (-not [IO.File]::Exists($system)) { $system = Join-Path $env:WINDIR 'System32\wininet.dll' }
        $real = @(Get-SktPeImage $system)
        Assert-SktWininetExports $proxy $real
    }
    $engine=Get-SktHash (Get-SktPath $Root 'Skeeto\Maintain.ps1')
    if ($marker -notmatch '(?m)^update_protocol=1\r?$' -or
        $marker -notmatch ('(?m)^engine_sha256=' + $engine + '\r?$')) { throw (Get-SktText '更新信封与包内更新器不一致。' 'Update envelope does not match its engine.') }
    if ((Get-SktHash (Get-SktPath $Root 'Skeeto\Manifest.json')) -ne $metadata.manifestHash) { throw (Get-SktText '包清单在校验期间改变。' 'The package manifest changed during validation.') }
    return $metadata
}
function New-SktChange([string]$Root,[string]$Relative,$Source) {
    $target = Get-SktPath $Root $Relative
    if ([IO.Directory]::Exists($target)) { throw (Get-SktText "预期文件却发现目录：$target" "Expected a file but found a directory: $target") }
    return [pscustomobject]@{relative=$Relative; before=(Get-SktHash $target); after=$(if($Source){Get-SktHash $Source}else{$null}); source=$Source; backup=$null}
}
function Restore-SktTransaction($Journal,[string]$Directory) {
    $work = @()
    foreach ($entry in $Journal.entries) {
        $target = Get-SktPath $Journal.gameRoot $entry.relative
        $current = Get-SktHash $target
        if ($current -eq $entry.before) { continue }
        # READY belongs to staging, not user data. A newer downloaded engine
        # may already have re-prepared it; keep that valid newer receipt.
        if ($entry.relative -eq 'skeeto_update\READY' -and $current -and $current -ne $entry.after) { continue }
        if ($current -ne $entry.after) { throw (Get-SktText "后续修改阻止回退：$target" "A later change blocks rollback: $target") }
        $backup = $null
        if ($entry.before) {
            if ($entry.backup -notmatch '^original-[0-9]+\.bin$') { throw (Get-SktText '无效备份文件名。' 'Invalid backup file name.') }
            $backup = Get-SktPath $Directory $entry.backup -Internal
            if ((Get-SktHash $backup) -ne $entry.before) { throw (Get-SktText "备份损坏：$backup" "Damaged backup: $backup") }
        }
        $work += [pscustomobject]@{entry=$entry; target=$target; backup=$backup}
    }
    [array]::Reverse($work)
    foreach ($item in $work) {
        if ($item.backup) { Set-SktFile $item.backup $item.target $item.entry.after $item.entry.before }
        else {
            if ((Get-SktHash $item.target) -ne $item.entry.after) { throw (Get-SktText '回退期间文件已改变。' 'A file changed during rollback.') }
            [IO.File]::Delete($item.target)
        }
    }
}
function Invoke-SktTransaction([string]$Root,[array]$Changes,[string]$Operation) {
    # Process table is Soft for Apply: defer instead of writing a failure receipt.
    # Mid-write Assert below stays Hard so a Steam relaunch can roll back.
    if (Test-SktGamesRunning) {
        if ($Operation -eq 'Apply') {
            throw (New-SktDeferException 'GAME_RUNNING' '请完全退出游戏及旧启动器后重试；脚本不会结束进程。' 'Exit the game and legacy launchers before retrying. This tool will not terminate them.')
        }
        Assert-SktStopped
    }
    $changesToMake = @($Changes | Where-Object { $_.before -ne $_.after })
    if (-not $changesToMake.Count) { Write-SktMessage (Get-SktText '文件已是目标状态，无需重复修改。' 'Files already match the target; no changes needed.'); return $null }
    # Honesty red line: locked skeeto.dll / bin\wininet.dll aborts the whole
    # transaction. Poll exclusive open on those two; other before-targets keep
    # a single exclusive probe so non-critical locks do not burn a long wait.
    foreach ($entry in $changesToMake) {
        $target = Get-SktPath $Root $entry.relative
        if (-not $entry.before) { continue }
        if (Test-SktCriticalRelative $entry.relative) {
            Wait-SktExclusiveOpen $target $entry.relative
        } else {
            try {
                $stream = [IO.File]::Open($target,[IO.FileMode]::Open,[IO.FileAccess]::ReadWrite,[IO.FileShare]::None)
                $stream.Dispose()
            } catch {
                throw (New-SktOccupancyException $entry.relative)
            }
        }
    }
    # The maintenance mutex allows one transaction. Use one fixed slot, so
    # an inaccessible leftover cannot accumulate a new folder on every retry.
    $id = 'current'
    $directory = Get-SktPath $Root ('skeeto_migrate_backup\' + $id) -Internal
    Remove-SktOldUpdates $Root '' $id
    if ([IO.Directory]::Exists($directory)) {
        if ($Operation -eq 'Apply') {
            $orphan = $directory + '.orphan-' + [DateTime]::UtcNow.ToString('yyyyMMddHHmmss')
            try { [IO.Directory]::Move($directory, $orphan) }
            catch {
                throw (New-SktDeferException 'BACKUP_SLOT' '上次恢复数据尚未清理，未创建额外备份。' 'Previous recovery data could not be cleared; no additional backup was created.')
            }
        } else {
            throw [IO.IOException]::new((Get-SktText '上次恢复数据尚未清理，未创建额外备份。' 'Previous recovery data could not be cleared; no additional backup was created.'))
        }
    }
    [IO.Directory]::CreateDirectory($directory) | Out-Null
    $journal = [pscustomobject]@{schema=1; gameRoot=$Root; operation=$Operation; status='prepared'; entries=$changesToMake}
    $journalPath = Join-Path $directory 'transaction.json'
    $index=0
    $originals=@{}
    foreach ($entry in $changesToMake) {
        if ($entry.before) {
            if (-not $originals.ContainsKey($entry.before)) { $originals[$entry.before]='original-' + $index + '.bin' }
            $entry.backup=$originals[$entry.before]
        }
        ++$index
    }
    # Journal first, active pointer last: an interruption while copying backups
    # cannot claim a recoverable transaction or change installed files.
    $active=Get-SktPath $Root 'skeeto_migrate_backup\active.json' -Internal
    try {
    Write-SktControl $journalPath ($journal | ConvertTo-Json -Depth 7)
    foreach ($entry in $changesToMake) {
        if ($entry.before) {
            $backup = Join-Path $directory $entry.backup
            if ([IO.File]::Exists($backup)) { continue } # Same old bytes already preserved once.
            [IO.File]::Copy((Get-SktPath $Root $entry.relative),$backup,$false)
            if ((Get-SktHash $backup) -ne $entry.before) { throw (Get-SktText '备份校验失败，未执行安装。' 'Backup verification failed. Installation was not performed.') }
            $flush=[IO.File]::Open($backup,[IO.FileMode]::Open,[IO.FileAccess]::ReadWrite,[IO.FileShare]::None)
            try { $flush.Flush($true) } finally { $flush.Dispose() }
        }
    }
    Write-SktControl $active ([pscustomobject]@{id=$id;journalHash=(Get-SktHash $journalPath)} | ConvertTo-Json -Compress)
    } catch {
        $preparationFailure=$_.Exception.Message
        # No target was changed. Reclaim this invocation's known files rather
        # than letting every disk/write retry consume another backup directory.
        try {
            if (-not [IO.File]::Exists($active)) {
                $names=@('transaction.json','transaction.json.tmp') + @($changesToMake | Where-Object backup | ForEach-Object backup)
                Remove-SktOwnedTransactionFiles $directory $names
            }
        } catch { Write-Warning 'Failed preparation cleanup deferred.' }
        throw $preparationFailure
    }
    try {
        foreach ($entry in $changesToMake) {
            Assert-SktStopped
            $target = Get-SktPath $Root $entry.relative
            if ($entry.source) { Set-SktFile $entry.source $target $entry.before $entry.after }
            else {
                if ((Get-SktHash $target) -ne $entry.before) { throw (Get-SktText "文件在删除前已改变：$target" "File changed before removal: $target") }
                [IO.File]::Delete($target)
            }
            if ((Get-SktHash $target) -ne $entry.after) { throw (Get-SktText "提交后校验失败：$target" "Verification failed after replacement: $target") }
        }
        # The prepared journal is immutable once the first target is changed.
        # A crash/disk failure writing a cosmetic outcome cannot destroy recovery.
        try { Write-SktControl (Join-Path $directory 'result.txt') 'committed'; [IO.File]::Delete($active) }
        catch { Write-SktMessage (Get-SktText '文件已提交；结果备注写入失败，原始恢复清单仍完整。' 'Files were applied. The result note could not be written; the original recovery journal is intact.') }
        Remove-SktOldUpdates $Root ''
        if ([IO.Directory]::Exists($directory)) {
            Write-SktMessage (Get-SktText '文件操作完成，临时恢复数据暂未清理；下次操作不会另建备份。' 'Files applied; recovery cleanup deferred. The next operation will not create another backup.') -Color Yellow
        } else { Write-SktMessage (Get-SktText '文件操作完成，临时恢复数据已清理。' 'File operation completed; temporary recovery data cleared.') -Color Green }
        return $id
    } catch {
        $failure=$_.Exception.Message
        try { Assert-SktStopped; Restore-SktTransaction $journal $directory; $journal.status='rolled-back' }
        catch { $journal.status='recovery-required'; $failure += (Get-SktText "；恢复未完成：$($_.Exception.Message)。请保留 $directory，退出游戏后重新运行安装或更新。" "; recovery incomplete: $($_.Exception.Message). Keep $directory, exit the game and rerun installation or update.") }
        try {
            Write-SktControl (Join-Path $directory 'result.txt') $journal.status
            if ($journal.status -eq 'rolled-back') {
                [IO.File]::Delete($active)
                Remove-SktOldUpdates $Root '' $id
            }
        } catch { Write-Warning 'Recovery outcome could not be saved; active transaction retained.' }
        throw $failure
    }
}
function New-SktTextSource([string]$Directory,[string]$Name,[string]$Text) {
    $path = Join-Path $Directory $Name
    [IO.File]::WriteAllText($path,$Text,(New-Object Text.UTF8Encoding($false)))
    return $path
}
function Show-SktLaunchOptions([string]$Root) {
    foreach ($name in @('skeeto.ini','kpatch.ini')) {
        $path = Join-Path $Root $name
        if ([IO.File]::Exists($path) -and (Get-Item -LiteralPath $path).Length -lt 1MB) {
            foreach ($line in [IO.File]::ReadAllLines($path,[Text.Encoding]::Default)) {
                if ($line -match '^\s*cmdline\s*=\s*(.*)$' -and $Matches[1].Trim()) {
                    Write-SktMessage ((Get-SktText "$name 的旧启动项（仅供核对，请自行粘贴到 Steam 启动选项）：" "Legacy launch options in $name (review and copy to Steam launch options yourself): ") + $Matches[1])
                }
            }
        }
    }
}
function Test-SktOwnedText([string]$Path,[string]$Pattern) {
    try {
        if (-not [IO.File]::Exists($Path)) { return $false }
        if ((Get-Item -LiteralPath $Path).Length -gt 8192) { return $false }
        return [IO.File]::ReadAllText($Path) -match $Pattern
    } catch { return $false }
}
function Remove-SktKnownFile([string]$Root,[string]$Relative,[switch]$Quiet) {
    $path=Get-SktPath $Root $Relative -Internal
    if (-not [IO.File]::Exists($path)) { return }
    try {
        [IO.File]::Delete($path)
        if (-not $Quiet) { Write-SktMessage (Get-SktText "已删除 $Relative" "Removed $Relative") }
    } catch {
        Write-SktMessage (Get-SktText "暂时无法删除，已保留：$Relative" "Could not delete this yet; kept: $Relative") -Color Yellow
    }
}
function Remove-SktEmptyOwnedTree([string]$Directory) {
    if (-not [IO.Directory]::Exists($Directory)) { return }
    if ([IO.File]::GetAttributes($Directory) -band [IO.FileAttributes]::ReparsePoint) { return }
    foreach ($child in @([IO.Directory]::EnumerateDirectories($Directory))) { Remove-SktEmptyOwnedTree $child }
    if (-not @([IO.Directory]::EnumerateFileSystemEntries($Directory)).Count) { [IO.Directory]::Delete($Directory) }
}
function Remove-SktUpdateStage([string]$Root) {
    $stage=Get-SktPath $Root 'skeeto_update' -Internal
    if (-not [IO.Directory]::Exists($stage)) { return }
    if ([IO.File]::Exists((Join-Path $stage 'READY'))) {
        Write-SktMessage (Get-SktText '检测到未完成的更新暂存，已保留 skeeto_update。' 'A pending update stage exists. skeeto_update was kept.') -Color Yellow
        return
    }
    foreach ($relative in $script:PayloadFiles) { Remove-SktKnownFile $Root ('skeeto_update\' + $relative) }
    foreach ($name in @('VERSION.txt','SKEETO_WORKSHOP.txt')) { Remove-SktKnownFile $Root ('skeeto_update\_scan\' + $name) }
    Remove-SktKnownFile $Root 'skeeto_update\Skeeto\Manifest.json'
    Remove-SktKnownFile $Root 'skeeto_update\apply_failed.txt'
    Remove-SktEmptyOwnedTree $stage
    if ([IO.Directory]::Exists($stage)) { Write-SktMessage (Get-SktText 'skeeto_update 里有未列入清理名单的文件，目录已保留。' 'skeeto_update contains a file outside the cleanup list. The directory was kept.') -Color Yellow }
}
function Remove-SktBackupTree([string]$Root) {
    $base=Get-SktPath $Root 'skeeto_migrate_backup' -Internal
    if (-not [IO.Directory]::Exists($base)) { return }
    if ([IO.File]::Exists((Join-Path $base 'active.json'))) {
        Write-SktMessage (Get-SktText '仍有未完成安装事务，已保留 skeeto_migrate_backup。' 'An unfinished install transaction remains. skeeto_migrate_backup was kept.') -Color Yellow
        return
    }
    $dirPattern='^(current|scratch-[a-f0-9]{32}|(update-)?\d{8}T\d{6}-[a-f0-9]{32})$'
    $filePattern='^(transaction\.json(\.tmp)?|result\.txt(\.tmp)?|original-[0-9]+\.bin|bridge\.json|pending\.txt)$'
    $unknown=$false
    foreach ($dir in @([IO.Directory]::EnumerateDirectories($base))) {
        if ([IO.File]::GetAttributes($dir) -band [IO.FileAttributes]::ReparsePoint) { $unknown=$true; continue }
        $owned=[IO.Path]::GetFileName($dir) -match $dirPattern
        if ($owned) {
            foreach ($file in @([IO.Directory]::EnumerateFiles($dir))) {
                if ([IO.Path]::GetFileName($file) -notmatch $filePattern) { $owned=$false; break }
            }
            if (@([IO.Directory]::EnumerateDirectories($dir)).Count) { $owned=$false }
        }
        if (-not $owned) { $unknown=$true; continue }
        foreach ($file in @([IO.Directory]::EnumerateFiles($dir))) { [IO.File]::Delete($file) }
        [IO.Directory]::Delete($dir)
    }
    foreach ($file in @([IO.Directory]::EnumerateFiles($base))) { $unknown=$true }
    if (-not $unknown -and -not @([IO.Directory]::EnumerateFileSystemEntries($base)).Count) {
        [IO.Directory]::Delete($base)
        Write-SktMessage (Get-SktText '已删除 skeeto_migrate_backup' 'Removed skeeto_migrate_backup')
    } elseif ($unknown) {
        Write-SktMessage (Get-SktText 'skeeto_migrate_backup 里有未识别内容，已保留。' 'skeeto_migrate_backup contains unrecognized contents and was kept.') -Color Yellow
    }
}
function Copy-SktChangelog([string]$Root,[string]$Payload,[switch]$Quiet) {
    # One menu document, copied after the DLL transaction. A lock keeps the
    # existing file and does not roll back the core or create a backup.
    $source=Get-SktPath $Payload 'Skeeto_更新日志.txt' -Internal
    if (-not [IO.File]::Exists($source)) { return }
    $target=Get-SktPath $Root 'Skeeto_更新日志.txt' -Internal
    try { [IO.File]::Copy($source,$target,$true) }
    catch {
        if (-not $Quiet) { Write-SktMessage (Get-SktText '更新日志暂时无法写入，已保留现有文件。' 'The changelog could not be written yet; the existing file was kept.') -Color Yellow }
    }
}
function Remove-SktInstalledExtras([string]$Root,[switch]$KeepLegacyEntry,[switch]$Quiet,[switch]$RemoveChangelog) {
    foreach ($relative in $script:PayloadFiles) {
        if ($relative -in @('skeeto.dll','bin\wininet.dll')) { continue }
        if ($relative -eq 'Skeeto_更新日志.txt' -and -not $RemoveChangelog) { continue }
        $path=Get-SktPath $Root $relative -Internal
        if ($relative -eq 'VERSION.txt' -and [IO.File]::Exists($path) -and -not (Test-SktOwnedText $path '(?im)^Skeeto\b')) {
            Write-SktMessage (Get-SktText '根目录 VERSION.txt 不属于 Skeeto，已保留。' 'The root VERSION.txt is not Skeeto and was kept.') -Color Yellow
            continue
        }
        if ($relative -eq 'SKEETO_WORKSHOP.txt' -and [IO.File]::Exists($path) -and -not (Test-SktOwnedText $path '(?m)^SkeetoWorkshopPackage=1\r?$')) {
            Write-SktMessage (Get-SktText '根目录 SKEETO_WORKSHOP.txt 无法确认，已保留。' 'The root SKEETO_WORKSHOP.txt could not be confirmed and was kept.') -Color Yellow
            continue
        }
        Remove-SktKnownFile $Root $relative -Quiet:$Quiet
    }
    # The ownership manifest is not a payload file, but older installs wrote it
    # into the game root. Automatic updates remove it here.
    $names=@('Skeeto\Manifest.json','skeeto_install.txt','skeeto_setting.ini.before-key-cleanup.bak','skeeto_ws_show_changelog.txt','skeeto_ws_changelog_shown.txt',
        'Skeeto_使用说明.txt','Skeeto_指令速查.txt','Skeeto_常见问题与解决办法_FAQ_and_Troubleshooting.txt','安装 Install Skeeto.bat','卸载 Uninstall Skeeto.bat')
    if (-not $KeepLegacyEntry) { $names=$script:Retired + $names }
    foreach ($name in $names) {
        Remove-SktKnownFile $Root $name -Quiet:$Quiet
    }
    $oldPlugin=Get-SktPath $Root 'bin\neko\plugins\skeeto.dll' -Internal
    try {
        if ([IO.File]::Exists($oldPlugin) -and (Test-SktKnownInstall $Root 'bin\neko\plugins\skeeto.dll' 'core')) {
            Remove-SktKnownFile $Root 'bin\neko\plugins\skeeto.dll' -Quiet:$Quiet
        }
    } catch {
        Write-SktMessage (Get-SktText '旧插件副本暂时无法确认，已保留。' 'The old plugin copy could not be confirmed yet and was kept.') -Color Yellow
    }
    $docs=Get-SktPath $Root 'Skeeto' -Internal
    if ([IO.Directory]::Exists($docs)) {
        Remove-SktEmptyOwnedTree $docs
        if ([IO.Directory]::Exists($docs)) { Write-SktMessage (Get-SktText 'Skeeto 目录中有未列入清理名单的文件，目录已保留。' 'The Skeeto directory contains a file outside the cleanup list and was kept.') -Color Yellow }
    }
}
function Remove-SktGameRootClutter([string]$Root) {
    Remove-SktInstalledExtras $Root
    Remove-SktUpdateStage $Root
    Remove-SktBackupTree $Root
}
function Get-SktDllSemver([string]$Root) {
    $path=Get-SktPath $Root 'skeeto.dll' -Internal
    if (-not [IO.File]::Exists($path)) { return $null }
    # Prepare runs while the game still has the core open. If it cannot be
    # read, the in-process DLL already compared versions; do not fail here.
    try { $bytes=[IO.File]::ReadAllBytes($path) }
    catch [IO.IOException] { return $null }
    catch [UnauthorizedAccessException] { return $null }
    $match=[regex]::Match([Text.Encoding]::ASCII.GetString($bytes),'SkeetoInstalledSemver=(\d+\.\d+\.\d+)')
    if (-not $match.Success) { return $null }
    return $match.Groups[1].Value
}

function Wait-SktGamesStopped {
    param([int]$TimeoutSeconds = 90, [int]$PollMilliseconds = 250)
    # After the accepted PID exits, a quick Steam relaunch can appear before
    # mutation. Poll with a deadline so Apply finishes when that short-lived
    # instance exits. Timeout is a deferral (READY kept, no apply_failed), not
    # a damaged-package failure.
    $deadline = [DateTime]::UtcNow.AddSeconds([Math]::Max(1, $TimeoutSeconds))
    while ($true) {
        if (-not (Test-SktGamesRunning)) { return }
        if ([DateTime]::UtcNow -ge $deadline) {
            throw (New-SktDeferException 'QUIET_WAIT' '请完全退出游戏及旧启动器后重试；脚本不会结束进程。' 'Exit the game and legacy launchers before retrying. This tool will not terminate them.')
        }
        Start-Sleep -Milliseconds ([Math]::Max(50, $PollMilliseconds))
    }
}
function Wait-SktGameInstance {
    if (-not $WaitPid) { return }
    if ($Mode -ne 'Apply' -or $WaitPid -lt 1 -or $WaitStartTime -lt 1) { throw (Get-SktText '等待游戏退出的参数无效。' 'Invalid update wait request.') }
    $process=$null
    try { $process=[Diagnostics.Process]::GetProcessById($WaitPid) }
    catch [ArgumentException] { $process=$null } # The accepted game has already exited.
    if ($process) {
        try {
            try { $started=$process.StartTime.ToUniversalTime().ToFileTimeUtc() }
            catch [InvalidOperationException] { $started=$null } # It exited between lookup and StartTime.
            if ($started -eq $WaitStartTime) { $process.WaitForExit() }
        } finally { $process.Dispose() }
    }
    # WaitPid is done; still require a quiet process list before mutation.
    Wait-SktGamesStopped
}

for ($attempt=0; $attempt -lt 4; ++$attempt) {
try {
    if ($InteractiveMenu) {
        Write-Host (Get-SktText 'Skeeto 安装与维护' 'Skeeto Setup') -ForegroundColor Cyan
        Write-Host (Get-SktText '正常首次安装：在游戏外运行本脚本。游戏里只放入核心、Steam 入口和更新日志。' 'First install: run this script outside the game. Only the core, Steam entry and changelog are placed in the game.')
        Write-Host (Get-SktText '1. 安装 / 修复（自动识别旧版迁移）' '1. Install / repair (detect legacy migration automatically)') -ForegroundColor White
        Write-Host (Get-SktText '2. 卸载 Skeeto（保留设置和其他平台）' '2. Uninstall Skeeto (keep settings and other platforms)')
        Write-Host (Get-SktText '3. 清理游戏根目录非必要文件（不卸载，保留核心、更新日志、设置和素材）' '3. Remove unnecessary game-root files (does not uninstall; keeps the core, changelog, settings and assets)')
        Write-Host (Get-SktText '0. 退出' '0. Exit')
        $selection=Read-Host (Get-SktText '请选择编号，然后回车' 'Choose a number, then press Enter')
        switch ($selection.Trim()) {
            '0' { exit 0 }
            '1' { $Mode='Migrate' }
            '2' { $Mode='Uninstall' }
            '3' { $Mode='Clean' }
            default { throw (Get-SktText '无效编号，请选择 0、1、2 或 3。' 'Invalid choice. Choose 0, 1, 2 or 3.') }
        }
    }
    $PayloadRoot=(Resolve-Path -LiteralPath $PayloadRoot).ProviderPath.TrimEnd('\')
    # Imported from this maintenance engine's directory, not arbitrary cwd.
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

    if ($Mode -eq 'Validate') { Get-SktPayload $PayloadRoot | Out-Null; exit 0 }
    if ($UpdateProtocol -notin @(0,1)) { Write-SktMessage (Get-SktText '更新协议不支持，请手动迁移。' 'Unsupported update protocol; manual migration required.'); exit 2 }
    $root=Find-SktGameRoot
    Enter-SktMaintenance $root
    if ($StartedEvent -and $attempt -eq 0) {
        if ($Mode -ne 'Apply' -or $StartedEvent -notmatch '^Local\\SkeetoUpdateStarted\.[0-9]+\.[0-9]+$') { throw 'Invalid update acknowledgement event.' }
        $ack=[Threading.EventWaitHandle]::OpenExisting($StartedEvent)
        try { $ack.Set() | Out-Null } finally { $ack.Dispose() }
    }
    Wait-SktGameInstance
    # Apply already quiet-waited. A duplicate Assert here only turned relaunch
    # races into failure receipts. Uninstall/Migrate/Clean still need a hard stop.
    if ($Mode -notin @('Prepare','Apply')) {
        Assert-SktStopped
    }
    if ($Mode -eq 'Clean') {
        Show-SktLaunchOptions $root
        try { Repair-SktInterrupted (Get-SktInterrupted $root) }
        catch { Write-SktMessage (Get-SktText '未完成事务无法恢复，继续清理可确认的残留。' 'Could not recover an unfinished transaction. Confirmed leftovers will still be removed.') -Color Yellow }
        Remove-SktGameRootClutter $root
        Write-SktMessage (Get-SktText '游戏根目录清理完成。核心、代理、更新日志、设置、自定义素材和其他平台已保留。' 'Game-root cleanup finished. The core, proxy, changelog, settings, custom assets and other platforms were kept.') -Color Green
        exit 0
    }
    if ($Mode -ne 'Migrate') {
        $action=Get-SktText $(switch($Mode){'Uninstall'{'卸载'}'Apply'{'应用更新'}'Prepare'{'准备更新'}default{$Mode}}) $Mode
        Write-SktMessage (Get-SktText "操作=$action；目标游戏目录=$root" "Operation=$action; game root=$root") -Color Cyan
    }
    $interrupted=Get-SktInterrupted $root
    if ($Mode -ne 'Prepare') { Repair-SktInterrupted $interrupted }
    elseif ($interrupted) {
        # Installed files may be a mixture after a killed helper. Do not reject
        # the old ownership table or write loaded files; Apply recovers at exit.
        $metadata=Get-SktPayload $PayloadRoot
        if ($metadata.requires_manual_migrate) { exit 2 }
        Write-SktControl (Get-SktPath $PayloadRoot 'READY' -Internal) ([pscustomobject]@{protocol=1;semver=$metadata.semver;manifestHash=$metadata.manifestHash} | ConvertTo-Json -Compress)
        # Prepare is a retry, not installation success or acknowledgement of
        # the previous failure. Leave its receipt for the DLL to report.
        Write-SktMessage 'Interrupted transaction retained for recovery after game exit.'
        exit 0
    }
    try {
        $state=if($Mode -eq 'Uninstall' -and -not [IO.File]::Exists((Get-SktPath $root 'Skeeto\Manifest.json'))){$null}else{Get-SktState $root}
    }
    catch [IO.IOException] { if ($Mode -ne 'Uninstall') { throw }; $state=$null }
    catch [UnauthorizedAccessException] { if ($Mode -ne 'Uninstall') { throw }; $state=$null }
    catch {
        if ($Mode -ne 'Uninstall') { Stop-SktConflict (Get-SktText "无法确认当前安装归属，请保留用户内容并手动处理：$($_.Exception.Message)" "Installed ownership cannot be confirmed; keep user content and handle manually: $($_.Exception.Message)") }
        $state=$null
        Write-SktMessage (Get-SktText '清单不可用，按 DLL 身份卸载核心，保留其他文件。' 'Manifest unavailable; identify the core DLLs directly and keep other files.') -Color Yellow
    }
    $scratch=$null
    $changes=@()
    if ($Mode -eq 'Uninstall') {
        # DLL-only installs have no ownership table. Inspect the fixed targets
        # statically; never execute a DLL to identify or uninstall it.
        $ownedDlls=@('skeeto.dll','bin\wininet.dll')
        $optionalRemovals=@()
        foreach ($file in @(Get-SktRuntimeFiles $root $state)) {
            $changes+=[pscustomobject]@{relative=$file.relative;before=$file.hash;after=$null;source=$null;backup=$null}
        }
        foreach ($entry in $(if($state){$state.files}else{@()})) {
            # Keep the currently running entry/engine. Deleting the active
            # BAT breaks CMD's return; neither is required inside the game.
            if ($entry.relative -in $ownedDlls + @('Setup_安装 维护 卸载.bat','Skeeto\Maintain.ps1') -or $entry.role -eq 'document') { continue }
            $optionalRemovals+=$entry
        }
        foreach ($name in $(if($state){@('skeeto_ws_show_changelog.txt','skeeto_ws_changelog_shown.txt')}else{@()})) {
            try {
                $path=Get-SktPath $root $name
                if ([IO.File]::Exists($path) -and (Get-Item -LiteralPath $path).Length -lt 1024 -and
                    [IO.File]::ReadAllText($path).Trim() -eq ('semver=' + $state.semver)) {
                    $optionalRemovals+=[pscustomobject]@{relative=$name;hash=(Get-SktHash $path)}
                }
            } catch { Write-SktMessage (Get-SktText "保留暂不可访问的附属文件：$name" "Keeping an unavailable support file: $name") -Color Yellow }
        }
        # Removal is idempotent; there is no old runtime to recover. Preflight
        # every lock, then remove identified files without making backups.
        foreach ($entry in $changes) {
            $stream=[IO.File]::Open((Get-SktPath $root $entry.relative),[IO.FileMode]::Open,[IO.FileAccess]::ReadWrite,[IO.FileShare]::None)
            $stream.Dispose()
        }
        foreach ($entry in $changes) {
            Assert-SktStopped
            $path=Get-SktPath $root $entry.relative
            if ((Get-SktHash $path) -cne $entry.before) { throw "File changed before removal: $path" }
            [IO.File]::Delete($path)
        }
        foreach ($entry in $optionalRemovals) {
            try {
                $path=Get-SktPath $root $entry.relative
                $current=Get-SktHash $path
                if (-not $current) { continue }
                if ($current -cne $entry.hash) {
                    Write-SktMessage (Get-SktText "保留已修改的用户文件：$($entry.relative)" "Keeping a modified user file: $($entry.relative)") -Color Yellow
                } else { [IO.File]::Delete($path) }
            } catch { Write-SktMessage (Get-SktText "核心已移除，保留占用或不可访问的附属文件：$($entry.relative)" "Core removed; keeping a locked or unavailable support file: $($entry.relative)") -Color Yellow }
        }
        Remove-SktOldUpdates $root ''
        Remove-SktInstalledExtras $root -RemoveChangelog
        Write-SktMessage (Get-SktText '卸载完成。设置、自定义资源和其他平台保留；未创建卸载备份。不再使用时请取消核心工坊订阅。' 'Uninstall completed. Settings, custom assets and other platforms were kept; no uninstall backups created. Unsubscribe from the core Workshop item if no longer used.') -Color Green
        exit 0
    }
    if ($Mode -eq 'Apply') {
        $PayloadRoot=Get-SktPath $root 'skeeto_update' -Internal
        if (-not [IO.File]::Exists((Get-SktPath $root 'skeeto_update\READY'))) { Write-SktMessage (Get-SktText '没有待应用更新。' 'No pending update.'); exit 0 }
    }
    $metadata=Get-SktPayload $PayloadRoot
    if ($Mode -in @('Prepare','Apply') -and $metadata.requires_manual_migrate) {
        Write-SktMessage (Get-SktText '该版本需要手动迁移，未应用任何安装文件。' 'This version requires manual migration. No installed files were applied.'); exit 2
    }
    if ($Mode -eq 'Prepare' -and (-not $state -or $state.install_layout -eq 1)) { Write-SktMessage (Get-SktText '旧入口需要先手动迁移，未准备自动更新。' 'Manual migration is required before automatic update.'); exit 2 }
    $installedVer=Get-SktDllSemver $root
    if (-not $installedVer -and $state -and $state.semver) { $installedVer=$state.semver }
    if ($installedVer) {
        # Equal-version preparation/application is a valid repair or retry.
        # The DLL filters fresh candidates; Apply still requires a matching
        # READY and fully validates the payload. Only downgrades are rejected.
        if ([version]$metadata.semver -lt [version]$installedVer) {
            throw (Get-SktText '包版本低于已安装版本；不回退文件，READY 保留供检查。' 'The package is older than the installed version. Files were not downgraded; READY was kept for inspection.')
        }
    }
    if ($Mode -eq 'Prepare') {
        # No installed-file snapshot or discarded transaction plan here.
        # Apply samples and plans the installation after the game exits.
        $receipt=[pscustomobject]@{protocol=1;semver=$metadata.semver;manifestHash=$metadata.manifestHash}
        Write-SktControl (Get-SktPath $PayloadRoot 'READY' -Internal) ($receipt|ConvertTo-Json -Compress)
        # The DLL consumes a failure receipt only after actually reporting it;
        # a successful Apply also removes it. Preparing alone must not erase it.
        Write-SktMessage (Get-SktText '已由新包更新器完成准备，当前安装文件未改变。' 'Prepared with this package engine; installed files unchanged.')
        exit 0
    }
    if ($Mode -eq 'Apply' -and $state -and $state.install_layout -ne 1) {
        $ready=Read-SktJson (Get-SktPath $root 'skeeto_update\READY')
        if ($ready.protocol -ne 1 -or $ready.semver -cne $metadata.semver -or $ready.manifestHash -cne $metadata.manifestHash) {
            throw (Get-SktText '暂存包在准备后改变，READY 保留，请重新准备。' 'Stage changed after preparation. READY kept; prepare it again.')
        }
    }
    $bridge=($Mode -eq 'Apply' -and (-not $state -or $state.install_layout -eq 1))
    $owned=@{}
    if ($state) { foreach ($entry in $state.files) { if ($entry.role -ne 'document') { $owned[$entry.relative]=$entry.hash } } }
    $proxy=Get-SktPath $root 'bin\wininet.dll'
    $currentProxy=if($Mode -eq 'Migrate' -or $bridge){Get-SktHash $proxy}else{$null}
    $incomingProxy=if($Mode -eq 'Migrate'){Get-SktHash (Get-SktPath $PayloadRoot 'bin\wininet.dll')}else{$null}
    $proxyKnown=($currentProxy -and $owned.ContainsKey('bin\wininet.dll') -and $owned['bin\wininet.dll'] -eq $currentProxy) -or
        ($currentProxy -and $currentProxy -ne $incomingProxy -and (Test-SktKnownInstall $root 'bin\wininet.dll' 'proxy'))
    if ($Mode -notin @('Prepare','Apply') -and $currentProxy -and $currentProxy -ne $incomingProxy -and -not $proxyKnown) {
        if ($Mode -ne 'Migrate' -or -not $AdoptExistingProxy) { throw (Get-SktText '已有未知/被修改的 bin\wininet.dll，停止覆盖。旧试验代理请先回退，或确认归属后使用 -AdoptExistingProxy 显式备份接管。' 'An unknown or modified bin\wininet.dll exists; overwrite stopped. Roll back the earlier experimental proxy, or verify ownership before explicitly backing it up and adopting it with -AdoptExistingProxy.') }
        Write-SktMessage (Get-SktText '已按显式参数接管现有代理；原文件将完整备份。' 'Explicit adoption requested. The existing proxy will be backed up in full.')
    }
    # A legacy entry must remain a bridge until the user explicitly migrates.
    if ($bridge -and $currentProxy) { throw (Get-SktText '已有代理但缺少有效完整包清单，停止自动更新。请核实代理归属并补齐完整包。' 'A proxy exists without a valid full-package manifest; automatic update stopped. Verify proxy ownership and restore the complete package.') }
    $incomingCore=@($metadata.files | Where-Object role -eq 'core')[0].relative
    $installedCore=if($state){@($state.files | Where-Object role -eq 'core')[0].relative}else{'skeeto.dll'}
    $oldCore=if($Mode -eq 'Migrate' -or $bridge){Get-SktHash (Get-SktPath $root $installedCore)}else{$null}
    if ($Mode -notin @('Prepare','Apply') -and $oldCore -and -not $state) {
        $names=@(Get-SktPeExports (Get-SktPath $root 'skeeto.dll'))
        if ('necola_export' -cnotin $names -and 'SkeetoBootstrapQuery' -cnotin $names) { throw (Get-SktText '现有 skeeto.dll 无法确认归属，未覆盖。' 'Ownership of the existing skeeto.dll could not be verified. It was not overwritten.') }
    }
    $aliasIsCore=$false
    if ($Mode -eq 'Migrate' -or $bridge) {
        $alias=Get-SktPath $root 'necola.dll'
        $aliasHash=Get-SktHash $alias
        $aliasIsCore=$aliasHash -and ($aliasHash -eq $oldCore -or $aliasHash -eq (Get-SktHash (Get-SktPath $PayloadRoot $incomingCore)))
        if ($aliasHash -and -not $aliasIsCore) {
            $aliasExports=@(Get-SktPeExports $alias)
            if ('necola_export' -cin $aliasExports -or 'SkeetoBootstrapQuery' -cin $aliasExports) {
                throw (Get-SktText 'necola.dll 是不同版本的 Skeeto 别名，未可靠确认对应核心。请先核对旧轨文件，不覆盖该别名。' 'necola.dll is a Skeeto alias from another version. Its matching core could not be verified; check the legacy installation first. The alias was not overwritten.')
            }
        }
    }
    if ($Mode -eq 'Migrate') {
        $legacyMigration=($state -and $state.install_layout -eq 1) -or $aliasIsCore
        foreach ($name in $script:Retired) { if ([IO.File]::Exists((Get-SktPath $root $name))) { $legacyMigration=$true; break } }
        $action=if($legacyMigration){Get-SktText '旧版迁移' 'Legacy migration'}elseif($oldCore -or $currentProxy){Get-SktText '维护 / 重装' 'Repair / reinstall'}else{Get-SktText '首次安装' 'First install'}
        Write-SktMessage (Get-SktText "操作=$action；目标游戏目录=$root" "Operation=$action; game root=$root") -Color Cyan
    }
    # Apply samples installed bytes after exit. OpenRead alone mislabels a
    # held core/proxy lock as APPLY_FAIL; wait the same exclusive gate first.
    if ($Mode -eq 'Apply') {
        foreach ($relative in @('skeeto.dll','bin\wininet.dll')) {
            $path = Get-SktPath $root $relative
            if ([IO.File]::Exists($path)) { Wait-SktExclusiveOpen $path $relative }
        }
    }
    foreach ($entry in $metadata.files) {
        # Package markers and tools stay in the package or update stage.
        # The game root receives the loading DLLs; the changelog is copied after.
        if ($entry.role -notin @('core','proxy','plugin')) { continue }
        if ($Mode -notin @('Prepare','Apply') -and $state -and -not $owned.ContainsKey($entry.relative)) {
            $unowned=Get-SktHash (Get-SktPath $root $entry.relative)
            if ($unowned -and $unowned -ne $entry.hash) { Stop-SktConflict (Get-SktText "新目标已有非受管文件，未覆盖：$($entry.relative)" "New target contains an unowned file; not overwritten: $($entry.relative)") }
        }
        if ($bridge -and $entry.role -in @('proxy','plugin')) { continue }
        $change=New-SktChange $root $entry.relative (Get-SktPath $PayloadRoot $entry.relative)
        if ($change.after -ne $entry.hash) { throw (Get-SktText "包在校验后已改变：$($entry.relative)" "Package changed after validation: $($entry.relative)") }
        $changes += $change
    }
    if ($state) {
        $incoming=@{}; foreach ($entry in $metadata.files) { $incoming[$entry.relative]=$true }
        foreach ($entry in $state.files) {
            if ($entry.relative -eq 'Skeeto\Manifest.json' -or $incoming.ContainsKey($entry.relative)) { continue }
            if ($entry.role -eq 'document') { continue } # Preserve retired user documents; no hash means no deletion authority.
            $current=Get-SktHash (Get-SktPath $root $entry.relative)
            if ($current -and $current -ne $entry.hash -and $entry.role -ne 'document') { Stop-SktConflict (Get-SktText "退役文件有用户修改，未覆盖：$($entry.relative)" "Owned retired file has user edits: $($entry.relative)") }
            if ($current) { $changes += New-SktChange $root $entry.relative $null }
        }
    }
    if ($aliasIsCore) {
        if ($bridge) { $changes += New-SktChange $root 'necola.dll' (Get-SktPath $PayloadRoot $incomingCore) }
        else {
            $native=Get-SktPath $root 'necola_orig.dll' -Internal
            if ([IO.File]::Exists($native)) {
                if (-not (Test-SktNativeNecola $native)) { throw (Get-SktText 'necola_orig.dll 未通过真实 Necola 静态导出核验，停止迁移。' 'necola_orig.dll failed the real Necola export check; migration stopped.') }
                $changes += New-SktChange $root 'necola.dll' $native
                Write-SktMessage (Get-SktText '将恢复真实 necola.dll，同时保留 necola_orig.dll。' 'Restoring the real necola.dll and keeping necola_orig.dll.')
            } else { $changes += New-SktChange $root 'necola.dll' $null }
        }
    }
    if ($Mode -eq 'Migrate') {
        # The user-authorized Steam migration retires Skeeto's old entry files.
        # Native Necola launchers/configuration are outside this exact list.
        if ($legacyMigration) { Show-SktLaunchOptions $root }
        foreach ($name in $script:Retired) {
            if ([IO.File]::Exists((Get-SktPath $root $name))) {
                $changes += New-SktChange $root $name $null
            }
        }

    }
    if ($Mode -eq 'Apply') {
        $changes += New-SktChange $root 'skeeto_update\READY' $null
    }
    $completed=Invoke-SktTransaction $root $changes $Mode
    # The changelog is one menu file and is independent of leftover cleanup.
    try { Copy-SktChangelog $root $PayloadRoot -Quiet:($Mode -eq 'Apply') }
    catch { Write-SktMessage (Get-SktText '核心已更新。更新日志暂时无法写入，已保留现有文件。' 'The core was updated. The changelog could not be written yet; the existing file was kept.') -Color Yellow }
    # Cleanup runs after the core transaction. A locked or unreadable leftover
    # must not turn a completed update into a failure or a retry.
    try {
        Remove-SktInstalledExtras $root -KeepLegacyEntry:$bridge -Quiet:($Mode -eq 'Apply')
        if ($Mode -eq 'Apply') { Remove-SktOldUpdates $root '' }
    } catch {
        Write-SktMessage (Get-SktText '核心已更新。部分非必要文件暂时无法清理，不影响本次安装。' 'The core was updated. Some unnecessary files could not be cleaned yet. This install is unaffected.') -Color Yellow
    }
    if ($Mode -eq 'Apply') {
        try {
            $failNote = Get-SktPath $root 'skeeto_update\apply_failed.txt' -Internal
            if ([IO.File]::Exists($failNote)) { [IO.File]::Delete($failNote) }
        } catch { Write-Warning 'apply_failed cleanup deferred.' }
        Write-SktApplyOutcome $root 'OK'
    }
    if ($bridge) { Write-SktMessage (Get-SktText '核心已更新，旧启动入口仍可用；尚未安装 Steam 代理。请从游戏外完整包运行“Setup_安装 维护 卸载.bat”完成迁移；迁移成功后会清理旧 Skeeto 启动文件。' 'Core updated; the legacy entry still works. The Steam proxy is not installed yet. Run Setup_安装 维护 卸载.bat from the complete package outside the game folder; successful migration removes the old Skeeto entry files.') }
    else {
        Write-SktMessage (Get-SktText '安装完成。请从 Steam 正常启动。设置和其他平台保留。' 'Installation completed. Launch normally through Steam. Settings and other platforms were kept.') -Color Green
        if ($Mode -eq 'Migrate' -and $legacyMigration) {
            Write-SktMessage (Get-SktText '旧 Skeeto 启动文件已清理；旧启动参数见上方，请自行转到 Steam 启动选项。Necola 本体及其配置保留。' 'Old Skeeto entry files were removed. Review the legacy options above and transfer them to Steam yourself. Native Necola and its configuration were kept.') -Color Green
        }
    }
    exit 0
} catch {
    $failure = $_.Exception.Message
    $isDefer = $failure.StartsWith('DEFER:')
    $isOccupancy = $failure.StartsWith('OCCUPANCY:')
    Write-SktMessage ((Get-SktText "操作失败：" "Operation failed: ") + $failure) -Failure:(-not $isDefer)
    if ($isDefer -and $Mode -eq 'Apply' -and $script:MaintenanceLocked -and (Get-Variable root -ErrorAction SilentlyContinue)) {
        # Package and READY are intact. Exit 0 so the live helper path does not
        # treat quiet-wait / relaunch overlap as BootstrapApplyFailed.
        $code,$detail = if ($failure.Contains('|')) { $failure.Split('|',2) } else { @($failure,'') }
        Write-SktApplyOutcome $root $code $detail
        exit 0
    }
    # Occupancy already polled; other Soft IO can still use short retries.
    if ($Mode -eq 'Apply' -and $script:MaintenanceLocked -and -not $isOccupancy -and $attempt -lt 3) {
        Write-SktMessage (Get-SktText '保留暂存，稍后自动重试。' 'Stage retained; retrying automatically shortly.')
        Start-Sleep -Seconds (2 * ($attempt+1))
        continue
    }
    # Staging-only failure receipt for the next game session (not game-root).
    # DLL shows the receipt text via NotifyRetryableError; do not clear READY.
    if ($Mode -eq 'Apply' -and $script:MaintenanceLocked -and (Get-Variable root -ErrorAction SilentlyContinue) -and [IO.File]::Exists((Get-SktPath $root 'skeeto_update\READY'))) {
        try {
            $code = if ($isOccupancy) { ($failure.Split('|',2)[0]) } elseif ($failure -match '哈希|hash|校验|verif') { 'VALIDATE' } else { 'APPLY_FAIL' }
            $detail = if ($failure.Contains('|')) { $failure.Split('|',2)[1] } else { $failure }
            $note = $code + '|' + (Get-SktText '自动安装未完成。' 'Automatic install did not finish.') + ' ' + $detail
            Write-SktControl (Get-SktPath $root 'skeeto_update\apply_failed.txt' -Internal) $note
            Write-SktApplyOutcome $root $code $detail
        } catch { Write-Warning 'apply_failed receipt deferred.' }
    }
    exit 1
} finally {
    # Scratch contains only files written by this invocation. Avoid recursive
    # deletion and keep user-added/unrecognized contents intact.
    if ((Get-Variable scratch -ErrorAction SilentlyContinue) -and [IO.Directory]::Exists($scratch)) {
        foreach ($name in @('bridge.json','pending.txt')) {
            try { $path=Get-SktPath $scratch $name -Internal; if ([IO.File]::Exists($path)) { [IO.File]::Delete($path) } } catch { Write-Warning 'Scratch cleanup deferred.' }
        }
        try { if (-not @([IO.Directory]::EnumerateFileSystemEntries($scratch)).Count) { [IO.Directory]::Delete($scratch) } } catch { Write-Warning 'Scratch cleanup deferred.' }
    }
    if ($script:MaintenanceLocked) {
        try { Remove-SktOldUpdates $root '' } catch { Write-Warning 'Recovery cleanup deferred.' }
        $script:MaintenanceMutex.ReleaseMutex()
    }
    if ($script:MaintenanceMutex) { $script:MaintenanceMutex.Dispose() }
    $script:MaintenanceLocked=$false; $script:MaintenanceMutex=$null
}
}
