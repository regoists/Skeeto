$ErrorActionPreference = 'Stop'
$Repo = Split-Path -Parent $PSScriptRoot
$Root = Join-Path $Repo '_tmp_exit_waiter'
if (Test-Path -LiteralPath $Root) {
  $resolved = (Resolve-Path -LiteralPath $Root).Path
  if ($resolved -ne $Root) { throw "Unsafe test root: $resolved" }
  Remove-Item -LiteralPath $resolved -Recurse -Force
}
New-Item -ItemType Directory -Path $Root | Out-Null
$Updater = Join-Path $Root 'skeeto_apply_update.bat'
$Marker = Join-Path $Root 'applied.txt'
[IO.File]::WriteAllText($Updater, "@echo off`r`necho applied> `"$Marker`"`r`n", (New-Object Text.ASCIIEncoding))
$parent = Start-Process -FilePath 'powershell.exe' -ArgumentList @('-NoProfile','-Command','Start-Sleep -Milliseconds 800') -PassThru -WindowStyle Hidden
$escaped = $Updater.Replace("'", "''")
$command = '$p=Get-Process -Id ' + $parent.Id + ' -ErrorAction SilentlyContinue;if($p){$p.WaitForExit()}; & ''' + $escaped + ''''
$waiter = Start-Process -FilePath 'powershell.exe' -ArgumentList @('-NoProfile','-NonInteractive','-WindowStyle','Hidden','-Command',$command) -PassThru -WindowStyle Hidden
$waiter.WaitForExit(5000) | Out-Null
if ($waiter.ExitCode -ne 0 -or !(Test-Path -LiteralPath $Marker)) { throw 'Exit waiter failed' }
'EXIT_WAITER=PASS'
