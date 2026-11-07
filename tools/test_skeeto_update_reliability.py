"""Fault injection against actual packaged engines, only in synthetic roots."""
from __future__ import annotations
import atexit
import ctypes
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import time
import os
import zipfile
from test_skeeto_package_layout import refresh_manifest

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'packages/Skeeto-dev'
SCRATCH = None
RESULTS = []

def stamp_dll_semver(path: Path, version: str) -> None:
    """Keep Prepare/Apply version gates aligned with fixture semver after a real build DLL is synced."""
    data = bytearray(path.read_bytes())
    match = re.search(br'SkeetoInstalledSemver=\d+\.\d+\.\d+', data)
    if not match:
        return
    replacement = f'SkeetoInstalledSemver={version}'.encode('ascii')
    if len(replacement) != match.end() - match.start():
        raise AssertionError(f'cannot stamp {version} over {match.group()!r}')
    data[match.start():match.end()] = replacement
    path.write_bytes(data)




def fixture(name):
    target = SCRATCH / name
    shutil.copytree(SOURCE, target)
    (target / 'left4dead2').mkdir()
    (target / 'left4dead2.exe').write_bytes(b'offline game fixture')
    (target / 'bin/engine.dll').write_bytes(b'offline engine fixture')
    (target / 'skeeto_setting.ini').write_bytes(b'user settings')
    (target / 'foreign-platform.dll').write_bytes(b'other platform fixture')
    stamp_dll_semver(target / 'skeeto.dll', '2.0.0')
    refresh_manifest(target, '2.0.0')
    return target


def stage(root, version='2.0.2', inject=None):
    target = root / 'skeeto_update'
    target.mkdir(exist_ok=True)
    shutil.copytree(SOURCE, target, dirs_exist_ok=True)
    binary = bytearray((target / 'skeeto.dll').read_bytes())
    binary[0x20] ^= int(version.split('.')[-1]) & 255
    (target / 'skeeto.dll').write_bytes(binary)
    stamp_dll_semver(target / 'skeeto.dll', version)
    if inject:
        engine = target / 'Skeeto/Maintain.ps1'
        engine.write_text(inject(engine.read_text(encoding='utf-8-sig')), encoding='utf-8-sig')
    refresh_manifest(target, version)
    return target


def command(root, mode, *extra):
    engine = root / 'skeeto_update/Skeeto/Maintain.ps1'
    return ['powershell.exe', '-NoLogo', '-NoProfile', '-ExecutionPolicy', 'Bypass',
            '-File', str(engine), '-Mode', mode, '-GameRoot', str(root),
            '-PayloadRoot', str(root / 'skeeto_update'), '-NonInteractive', *extra]


def invoke(root, mode, expected=0, *extra):
    result = subprocess.run(command(root, mode, *extra), capture_output=True, timeout=45)
    assert result.returncode == expected, (mode, result.returncode, (result.stdout + result.stderr).decode('utf-8', errors='replace'))
    return result


def prepared(root):
    invoke(root, 'Prepare', 0, '-UpdateProtocol', '1')


def passed(name):
    RESULTS.append(name)
    print('PASS:', name, flush=True)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def assert_applied(root):
    assert digest(root / 'skeeto.dll') == digest(root / 'skeeto_update/skeeto.dll')
    assert not (root / 'bin/neko/plugins/skeeto.dll').exists()
    assert not (root / 'skeeto_update/READY').exists()
    assert not (root / 'skeeto_migrate_backup/active.json').exists()
    assert (root / 'skeeto_setting.ini').read_bytes() == b'user settings'
    assert (root / 'foreign-platform.dll').read_bytes() == b'other platform fixture'


def injected_boundary(text, body):
    needle='if ((Get-SktHash $target) -ne $entry.after) { throw'
    at=text.index(needle); end=text.index('\n', at)
    return text[:end] + '\n            if ($entry.relative -eq "skeeto.dll") { ' + body + ' }' + text[end:]


def interrupted(name, inject, version='2.0.2', mode='Apply', installed_version=None):
    root=fixture(name)
    if installed_version:
        stamp_dll_semver(root/'skeeto.dll', installed_version)
        refresh_manifest(root,installed_version)
    stage(root, version, inject=inject)
    if mode=='Apply': prepared(root)
    child=subprocess.Popen(command(root, mode), stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    try:
        deadline=time.monotonic()+25
        while not (root/'fault-boundary.txt').exists() and time.monotonic()<deadline:
            assert child.poll() is None, 'helper exited before injected boundary'
            time.sleep(.1)
        assert (root/'fault-boundary.txt').exists(), 'boundary not reached'
    finally:
        if child.poll() is None: child.kill()  # Only our synthetic helper, never a game.
        child.wait(timeout=10)
    return root


def lock(path):
    kernel=ctypes.WinDLL('kernel32', use_last_error=True)
    kernel.CreateFileW.argtypes=[ctypes.c_wchar_p,ctypes.c_uint32,ctypes.c_uint32,ctypes.c_void_p,ctypes.c_uint32,ctypes.c_uint32,ctypes.c_void_p]
    kernel.CreateFileW.restype=ctypes.c_void_p
    kernel.CloseHandle.argtypes=[ctypes.c_void_p]
    handle=kernel.CreateFileW(str(path),0x80000000,0,None,3,0x80,None)
    assert handle != ctypes.c_void_p(-1).value, ctypes.get_last_error()
    return kernel,handle


def check_preparation_cleanup():
    root=fixture('failed-backup-preparation')
    def fail_backup(t):
        needle='            [IO.File]::Copy((Get-SktPath $Root $entry.relative),$backup,$false)'
        assert needle in t
        return t.replace(needle,needle+"\n            throw [IO.IOException]::new('injected backup copy/flush failure')",1)
    stage(root,inject=fail_backup);prepared(root);old=digest(root/'skeeto.dll')
    invoke(root,'Apply',1)
    assert digest(root/'skeeto.dll')==old and (root/'skeeto_update/READY').exists()
    assert not list((root/'skeeto_migrate_backup').glob('*/transaction.json')), 'pre-activation backups accumulated'
    passed('all four failed backup preparations leave no transaction folders or installed changes')


def check_manual_maintenance():
    def minimal(name, old_proxy=False):
        root=SCRATCH/name;(root/'bin').mkdir(parents=True);(root/'left4dead2').mkdir()
        (root/'left4dead2.exe').write_bytes(b'fixture');(root/'bin/engine.dll').write_bytes(b'fixture')
        (root/'skeeto_setting.ini').write_bytes(b'user settings')
        (root/'foreign-platform.dll').write_bytes(b'other platform fixture')
        for relative in ('skeeto.dll','bin/wininet.dll'):
            shutil.copyfile(SOURCE/relative,root/relative)
        stamp_dll_semver(root/'skeeto.dll', '2.0.0')
        if old_proxy:
            path=root/'bin/wininet.dll';data=path.read_bytes()
            assert b'SkeetoWininetProxyIdentity\0' in data
            path.write_bytes(data.replace(b'SkeetoWininetProxyIdentity\0',b'XeetoWininetProxyIdentity\0'))
        return root

    thin=SCRATCH/'external-tool/Skeeto';thin.mkdir(parents=True)
    shutil.copyfile(SOURCE/'Skeeto/Maintain.ps1',thin/'Maintain.ps1')
    def maintain(root, mode, tool=thin/'Maintain.ps1'):
        result=subprocess.run(['powershell.exe','-NoProfile','-ExecutionPolicy','Bypass','-File',str(tool),
            '-Mode',mode,'-GameRoot',str(root),'-NonInteractive','-Language','en'],capture_output=True,timeout=45)
        assert result.returncode==0,(mode,result.stdout,result.stderr)
        return result.stdout
    for name,old in [('dll-only-uninstall',False),('pre-marker-proxy-uninstall',True)]:
        root=minimal(name,old)
        maintain(root,'Uninstall');maintain(root,'Uninstall')
        assert not any((root/p).exists() for p in ('skeeto.dll','bin/wininet.dll'))
        assert not list((root/'skeeto_migrate_backup').glob('*/transaction.json'))
        assert not (root/'skeeto_migrate_backup').exists(), 'DLL-only uninstall created unnecessary storage'
        assert (root/'skeeto_setting.ini').read_bytes()==b'user settings'
        assert (root/'foreign-platform.dll').read_bytes()==b'other platform fixture'
        passed(name+': only an external PS engine needed; root core and proxy removed, no backups')

    root=minimal('foreign-proxy-uninstall');(root/'bin/wininet.dll').write_bytes(b'foreign platform DLL')
    maintain(root,'Uninstall')
    assert (root/'bin/wininet.dll').read_bytes()==b'foreign platform DLL' and not (root/'skeeto.dll').exists()
    passed('uninstall identifies each DLL independently and preserves an unknown proxy')

    root=minimal('damaged-manifest-uninstall');(root/'Skeeto').mkdir();(root/'Skeeto/Manifest.json').write_bytes(b'bad json')
    maintain(root,'Uninstall')
    assert not (root/'bin/wininet.dll').exists() and not (root/'Skeeto/Manifest.json').exists()
    passed('a damaged optional ownership manifest does not block DLL removal')

    root=fixture('uninstall-locked-document');tutorial=root/'Skeeto/DIY/HitFeedback_命中反馈.txt'
    expected=tutorial.read_bytes();kernel,handle=lock(tutorial)
    try: maintain(root,'Uninstall')
    finally: kernel.CloseHandle(handle)
    assert tutorial.read_bytes()==expected and not (root/'skeeto.dll').exists() and not (root/'bin/wininet.dll').exists()
    assert not list((root/'skeeto_migrate_backup').glob('*/original-*.bin'))
    passed('locked optional document is preserved without blocking core uninstall or creating backups')

    root=minimal('dll-only-auto-update');stage(root);prepared(root);invoke(root,'Apply');assert_applied(root)
    passed('manual root-and-proxy installation prepares and applies automatic update without registration')

    root=minimal('install-detect')
    maintain(root,'Uninstall')
    tool=SOURCE/'Skeeto/Maintain.ps1'
    for i in range(3):
        output=maintain(root,'Migrate',tool)
        assert b'Operation=First install' in output and b'Operation=Migrate' not in output
        assert b'Operation=Repair / reinstall' in maintain(root,'Migrate',tool)
        maintain(root,'Uninstall',tool)
        assert not list((root/'skeeto_migrate_backup').glob('*/transaction.json'))
        assert not list((root/'skeeto_migrate_backup').glob('*/original-*.bin'))
    retired=('left4dead2_skeeto.exe','start_skeeto.bat','start_skeeto_with_update.bat','start_necola_with_update.bat','skeeto.ini')
    for name in retired: (root/name).write_bytes(b'legacy entry')
    (root/'skeeto.ini').write_bytes(b'[config]\ncmdline=-novid +exec my_autoexec.cfg\n')
    native=(SOURCE/'bin/wininet.dll').read_bytes() # non-Skeeto-core PE; never execute it
    (root/'necola.dll').write_bytes(native)
    preserved={'kpatch.ini':b'cmdline=-hide_neko\n','necola.exe':b'native launcher','skeeto_setting.ini':b'user settings','my_autoexec.cfg':b'user commands'}
    for name,body in preserved.items(): (root/name).write_bytes(body)
    output=maintain(root,'Migrate',tool)
    assert b'Operation=Legacy migration' in output and b'-novid +exec my_autoexec.cfg' in output
    assert all(not (root/name).exists() for name in retired)
    assert (root/'necola.dll').read_bytes()==native
    assert all((root/name).read_bytes()==body for name,body in preserved.items())
    assert not list((root/'skeeto_migrate_backup').glob('*/original-*.bin'))
    passed('repeated install/uninstall creates no history; migration removes Skeeto entry files, prints old options, preserves Necola and settings')

    def colored(root,mode):
        quote=lambda p: "'"+str(p).replace("'","''")+"'"
        host="function global:Write-Host { param($Object,[ConsoleColor]$ForegroundColor='Gray') [Console]::WriteLine(('COLOR '+$ForegroundColor+' '+$Object)) }; "
        cmd=host+'& '+quote(tool)+' -Mode '+mode+' -GameRoot '+quote(root)+' -NonInteractive -Language en'
        return subprocess.run(['powershell.exe','-NoProfile','-ExecutionPolicy','Bypass','-Command',cmd],capture_output=True,timeout=45)
    root=minimal('console-colors')
    maintain(root,'Uninstall',tool)
    output=colored(root,'Migrate');assert output.returncode==0 and b'COLOR Cyan Operation=First install' in output.stdout and b'COLOR Green Installation completed' in output.stdout
    (root/'skeeto_apply_update.bat').write_bytes(b'@echo off\n')
    kernel,handle=lock(root/'skeeto_apply_update.bat')
    try:
        output=colored(root,'Uninstall');assert output.returncode==0 and b'COLOR Yellow Could not delete this yet' in output.stdout
    finally: kernel.CloseHandle(handle)
    output=colored(SCRATCH/'missing-game','Migrate');assert output.returncode==1 and b'COLOR Red Operation failed:' in output.stdout
    passed('console host receives cyan operation, green success, yellow optional-file warning and red failure colors')


def check_review_recovery():
    root=interrupted('same-version-recovery',lambda text:injected_boundary(text,"[IO.File]::WriteAllText((Join-Path $Root 'fault-boundary.txt'),'core replaced'); Start-Sleep -Seconds 60"),version='2.0.2',mode='Migrate',installed_version='2.0.2')
    pending=stage(root,'2.0.2');(pending/'apply_failed.txt').write_bytes(b'previous interrupted Apply')
    prepared(root);assert (pending/'apply_failed.txt').read_bytes()==b'previous interrupted Apply'
    invoke(root,'Apply');assert_applied(root)
    passed('equal installed/staged version survives interrupted migration recovery and completes without manual repair')
    installed=root/'skeeto.dll'
    raw=installed.read_bytes()
    match=re.search(br'SkeetoInstalledSemver=(\d+\.\d+\.\d+)', raw)
    assert match and len(match.group(1))==5
    installed.write_bytes(raw.replace(b'SkeetoInstalledSemver='+match.group(1), b'SkeetoInstalledSemver=9.9.9', 1))
    before=digest(installed);stage(root,'2.0.1');invoke(root,'Prepare',1,'-UpdateProtocol','1')
    assert not (root/'skeeto_update/READY').exists() and digest(root/'skeeto.dll')==before
    passed('allowing equal-version recovery still rejects an actual downgrade')

    def journal_first(text):
        old='foreach ($file in [IO.Directory]::EnumerateFiles($Directory)) {'
        new="foreach ($file in @([IO.Directory]::EnumerateFiles($Directory) | Sort-Object { if ([IO.Path]::GetFileName($_) -eq 'transaction.json') { 0 } else { 1 } })) {"
        assert old in text;return text.replace(old,new,1)
    root=fixture('closed-slot-cleanup-retry');stage(root,inject=journal_first);prepared(root)
    slot=root/'skeeto_migrate_backup/current';slot.mkdir(parents=True)
    old=bytearray((root/'skeeto.dll').read_bytes());old[0x20]^=3
    journal=dict(schema=1,gameRoot=str(root),operation='Migrate',entries=[dict(relative='skeeto.dll',before=hashlib.sha256(old).hexdigest(),after=digest(root/'skeeto.dll'),backup='original-0.bin')])
    (slot/'transaction.json').write_text(json.dumps(journal),encoding='utf-8')
    (slot/'original-0.bin').write_bytes(old);(slot/'result.txt').write_text('committed')
    kernel,handle=lock(slot/'original-0.bin')
    try:
        invoke(root,'Apply',1)
        assert (slot/'transaction.json').exists(), 'partial cleanup destroyed ownership evidence'
    finally: kernel.CloseHandle(handle)
    invoke(root,'Apply');assert_applied(root)
    assert not slot.exists()
    passed('journal-first filesystem order and locked old snapshot preserve ownership; next Apply clears slot and succeeds')



def check_document_refresh():
    # Old manifests may still contain document hashes. They are ignored, not
    # refreshed; changing bytes after Prepare must not change the receipt.
    root=fixture('edited-document-payload');pending=stage(root)
    documents=('Skeeto/README.txt','Skeeto/Commands.txt',
               'Skeeto/DIY/HitFeedback_命中反馈.txt',
               'Skeeto/DIY/Examples 命中反馈json示例/skeeto_ci_my_style.json')
    changelog='Skeeto_更新日志.txt'
    for folder in (root,pending):
        manifest=folder/'Skeeto/Manifest.json';metadata=json.loads(manifest.read_text(encoding='utf-8'))
        for entry in metadata['files']:
            if entry['role']=='document': entry['hash']=digest(folder/entry['relative'].replace('\\','/'))
        manifest.write_text(json.dumps(metadata,ensure_ascii=False),encoding='utf-8')
    prepared(root)
    manifest_before=(pending/'Skeeto/Manifest.json').read_bytes()
    for relative in documents:
        (pending/relative).write_bytes(('edited package document '+relative).encode('utf-8'))
    (pending/changelog).write_bytes(b'edited staged changelog')
    invoke(root,'Apply');assert_applied(root)
    for relative in documents: assert not (root/relative).exists(), relative
    assert (root/changelog).read_bytes()==b'edited staged changelog'
    assert (pending/'Skeeto/Manifest.json').read_bytes()==manifest_before
    assert not list((root/'skeeto_migrate_backup').glob('*/original-*.bin'))
    passed('README/Commands/tutorial/example edited after Prepare stay in the package; only the changelog is installed and the core still updates')

    root=fixture('unavailable-documents');pending=stage(root)
    missing=pending/'Skeeto/README.txt';missing.unlink();prepared(root)
    original=(root/'Skeeto/README.txt').read_bytes()
    blocked=root/'Skeeto/Commands.txt';blocked.unlink();blocked.mkdir()
    tutorial=root/'Skeeto/DIY/HitFeedback_命中反馈.txt';tutorial.write_bytes(b'locked user tutorial')
    other='Skeeto_更新日志.txt';(pending/other).write_bytes(b'other writable document still updated')
    kernel,handle=lock(tutorial)
    try: invoke(root,'Apply');assert_applied(root)
    finally: kernel.CloseHandle(handle)
    assert not (root/'Skeeto/README.txt').exists()
    assert not blocked.exists()
    assert tutorial.read_bytes()==b'locked user tutorial'
    assert (root/other).read_bytes()==b'other writable document still updated'
    assert not list((root/'skeeto_migrate_backup').glob('*/original-*.bin'))
    passed('missing package documents, a locked tutorial and an unwritable path do not block the core update or create backups')

    root=fixture('critical-payload-hash');pending=stage(root);prepared(root);old=digest(root/'skeeto.dll')
    binary=bytearray((pending/'skeeto.dll').read_bytes());binary[0x20]^=0x80;(pending/'skeeto.dll').write_bytes(binary)
    invoke(root,'Apply',1)
    assert digest(root/'skeeto.dll')==old and (pending/'READY').exists()
    passed('critical payload hash mismatch still rejects Apply and preserves installed core and READY')


def check_failure_receipt_ownership():
    root=fixture('receipt-survives-prepare');pending=stage(root);prepared(root)
    receipt=pending/'apply_failed.txt';receipt.write_bytes(b'previous Apply could not replace proxy')
    prepared(root)
    assert receipt.read_bytes()==b'previous Apply could not replace proxy', 'Prepare consumed an unreported Apply failure'
    invoke(root,'Apply');assert_applied(root)
    assert not receipt.exists()
    passed('Prepare preserves unreported failure; successful Apply clears it')

    root=fixture('duplicate-apply-no-failure');stage(root);prepared(root)
    sleeper=subprocess.Popen(['powershell.exe','-NoProfile','-Command','Start-Sleep -Seconds 12'])
    query=subprocess.check_output(['powershell.exe','-NoProfile','-Command',f'(Get-Process -Id {sleeper.pid}).StartTime.ToUniversalTime().ToFileTimeUtc()'],text=True).strip()
    owner=subprocess.Popen(command(root,'Apply','-WaitPid',str(sleeper.pid),'-WaitStartTime',query),stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
    try:
        time.sleep(2);assert owner.poll() is None
        ready=(root/'skeeto_update/READY').read_bytes()
        invoke(root,'Apply',1)
        assert not (root/'skeeto_update/apply_failed.txt').exists(), 'Non-owner published an installation failure'
        assert (root/'skeeto_update/READY').read_bytes()==ready
        assert owner.wait(timeout=30)==0;assert_applied(root)
    finally:
        for proc in (sleeper,owner):
            if proc.poll() is None:proc.kill()
            proc.wait(timeout=10)
    passed('duplicate Apply leaves READY untouched and publishes no false failure')


def check_worker(modes=None):
    """Compile the entire unchanged update module; mock only UI/root/clock."""
    native=SCRATCH/'worker-native';native.mkdir()
    cpp=r'''
#include <Windows.h>
#include <shellapi.h>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <chrono>
#include <cstdarg>
static char testRoot[MAX_PATH]{};
static DWORD clockTick=2000;
static std::atomic_uint processStarts{0}, updateHelpers{0}, dialogs{0}, documentDialogs{0}, changelogs{0};
static unsigned prepareStarts=0, applyStarts=0, fullExtractions=0, preparesBeforeConsent=0;
static bool declineUpdate=false;
static bool changelogBeforeApply=false, changelogPrepared=false;
static std::chrono::steady_clock::time_point consentAt, changelogAt;
static bool failOneLaunch=false;
static bool slowLaunch=false;
static bool missingTar=false;
static HANDLE acceptedEvent=nullptr;
static DWORD TestTick(){return clockTick;}
static DWORD WINAPI TestSearch(LPCSTR a,LPCSTR b,LPCSTR c,DWORD d,LPSTR e,LPSTR* f){if(missingTar && !strcmp(b,"tar.exe"))return 0;return SearchPathA(a,b,c,d,e,f);}
static int WINAPI TestMessage(HWND,LPCWSTR body,LPCWSTR title,UINT flags){++dialogs;if(title && !wcscmp(title,L"Skeeto"))++documentDialogs;if(flags & MB_YESNO){preparesBeforeConsent=prepareStarts;consentAt=std::chrono::steady_clock::now();return declineUpdate?IDNO:IDYES;}if(acceptedEvent && wcsstr(body,L"更新已校验并暂存"))SetEvent(acceptedEvent);return IDOK;}
static HINSTANCE WINAPI TestShell(HWND,LPCWSTR,LPCWSTR,LPCWSTR,LPCWSTR,INT){++changelogs;changelogAt=std::chrono::steady_clock::now();changelogBeforeApply=applyStarts==0;char ready[MAX_PATH]{};sprintf_s(ready,"%s\\skeeto_update\\READY",testRoot);changelogPrepared=GetFileAttributesA(ready)!=INVALID_FILE_ATTRIBUTES;return reinterpret_cast<HINSTANCE>(33);}
static BOOL WINAPI TestCreateA(LPCSTR a,LPSTR b,LPSECURITY_ATTRIBUTES c,LPSECURITY_ATTRIBUTES d,BOOL e,DWORD f,LPVOID g,LPCSTR h,LPSTARTUPINFOA i,LPPROCESS_INFORMATION j){++processStarts;if(strstr(b," -xf ") && !strstr(b,"SKEETO_WORKSHOP.txt VERSION.txt"))++fullExtractions;return CreateProcessA(a,b,c,d,e,f,g,h,i,j);}
static BOOL WINAPI TestCreateW(LPCWSTR a,LPWSTR b,LPSECURITY_ATTRIBUTES c,LPSECURITY_ATTRIBUTES d,BOOL e,DWORD f,LPVOID g,LPCWSTR h,LPSTARTUPINFOW i,LPPROCESS_INFORMATION j){++processStarts;if(wcsstr(b,L"-Mode Prepare"))++prepareStarts;if(wcsstr(b,L"-Mode Apply"))++applyStarts;if(wcsstr(b,L"-Mode Prepare") || wcsstr(b,L"-Mode Apply"))++updateHelpers;if(failOneLaunch && (!slowLaunch || wcsstr(b,L"-Mode Apply"))){failOneLaunch=false;if(slowLaunch)clockTick+=60000;SetLastError(ERROR_ACCESS_DENIED);return FALSE;}return CreateProcessW(a,b,c,d,e,f,g,h,i,j);}
#define GetTickCount TestTick
#define MessageBoxW TestMessage
#define ShellExecuteW TestShell
#define CreateProcessA TestCreateA
#define CreateProcessW TestCreateW
#define SearchPathA TestSearch
// Force the harness below the fixture package (2.0.3). #pragma once keeps
// skeeto_workshop.cpp from restoring the repo VERSION.txt semver.
#include "skeeto_version.generated.h"
#undef SKEETO_VERSION
#define SKEETO_VERSION "2.0.2"
bool g_optWorkshopUpdate=true;
char g_workshopId[24]="3787239937";
HMODULE g_self=nullptr;
std::atomic_bool g_run{true};
void Log(const char* fmt,...){va_list args;va_start(args,fmt);vprintf(fmt,args);puts("");va_end(args);}
void SkeetoGameRoot(char* out,size_t n){strcpy_s(out,n,testRoot);}
#include "skeeto_workshop.cpp"
static void Pump(){clockTick+=1000;SkeetoWorkshopUpdateWorkerPump();}
int main(int argc,char**argv){
 if(argc!=3)return 90;strcpy_s(testRoot,argv[1]);const char* mode=argv[2];g_wsUpdateEnabled=true;
 slowLaunch=!strcmp(mode,"slow-launch-retry");
 failOneLaunch=!strcmp(mode,"launch-retry") || slowLaunch;
 missingTar=!strcmp(mode,"no-tar");
 declineUpdate=!strcmp(mode,"declined-update");
 if(!strcmp(mode,"host-nonblocking")){
  acceptedEvent=CreateEventW(nullptr,TRUE,FALSE,nullptr);
  SkeetoWorkshopUpdatePump();
  for(unsigned i=0;i<200 && processStarts.load()<2;++i)Sleep(100);
  if(processStarts.load()<2)return 97;
  auto start=std::chrono::steady_clock::now();
  for(unsigned i=0;i<100000;++i)SkeetoWorkshopUpdatePump();
  auto us=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-start).count();
  printf("shared_worker_calls=100000 elapsed_us=%lld during_slow_Prepare\n",us);
  if(us>500000)return 98;
  if(WaitForSingleObject(acceptedEvent,40000)!=WAIT_OBJECT_0)return 104;
  SkeetoWorkshopUpdateShutdown();g_run=false;return 0;
 }
 if(!strcmp(mode,"current-version")){SkeetoWorkshopTryOpenChangelog();SkeetoWorkshopTryOpenChangelog();}
 if(!strcmp(mode,"apply-failed-budget"))SkeetoWorkshopNotifyRetryableError(L"Earlier retryable error","test error budget");
 Pump();
 if(!strcmp(mode,"pending-silent")||!strcmp(mode,"apply-failed-start")||!strcmp(mode,"apply-failed-budget")){
  char fail[MAX_PATH]{},ready[MAX_PATH]{};sprintf_s(fail,"%s\\skeeto_update\\apply_failed.txt",testRoot);sprintf_s(ready,"%s\\skeeto_update\\READY",testRoot);
  const bool budget=!strcmp(mode,"apply-failed-budget"),failed=strcmp(mode,"pending-silent")!=0;
  if(dialogs!=(failed?1u:0u)||!g_wsApplyProcess||GetFileAttributesA(ready)==INVALID_FILE_ATTRIBUTES)return 112;
  if((GetFileAttributesA(fail)!=INVALID_FILE_ATTRIBUTES)!=budget)return 113;
  Pump();Pump();if(dialogs!=(failed?1u:0u))return 114;
  printf("pending notification dialogs=%u receipt_retained=%u\n",dialogs.load(),budget?1u:0u);
 }
 if(slowLaunch){unsigned before=processStarts;Pump();Pump();if(g_wsApplyProcess || processStarts!=before)return 105;printf("slow failed helper preserves retry delay after completion\n");clockTick+=60000;}
 Pump();Pump();
 if(!strcmp(mode,"download-retry")){
  if(g_wsApplyProcess)return 99;
  char zip[MAX_PATH]{},good[MAX_PATH]{};sprintf_s(zip,"%s\\left4dead2\\addons\\workshop\\3787239937.vpk",testRoot);sprintf_s(good,"%s\\test-valid-download.vpk",testRoot);
  if(!CopyFileA(good,zip,FALSE))return 100;
  Pump();Pump();Pump();
 }
 if(!strcmp(mode,"launch-retry")){if(g_wsApplyProcess)return 101;clockTick+=60000;Pump();}
 if(!strcmp(mode,"ack-failure")){return !g_wsApplyProcess && !g_wsSessionSettled?0:102;}
 if(!strcmp(mode,"idle")){
  unsigned before=processStarts;auto start=std::chrono::steady_clock::now();
  for(unsigned i=0;i<100000;++i)Pump();
  auto us=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-start).count();
  printf("idle_metadata_observations=100000 helper_starts_after_initial=%u elapsed_us=%lld\n",processStarts-before,us);
  return processStarts==before?0:91;
 }
 if(!strcmp(mode,"current-version")){
  for(unsigned i=0;i<100000;++i)Pump();
  printf("same_version_observations=100000 helper_starts=%u dialogs=%u changelogs=%u\n",updateHelpers.load(),dialogs.load(),changelogs.load());
  return updateHelpers==0 && dialogs==0 && changelogs==0?0:108;
 }
 if(!strcmp(mode,"manual-retry")){
  if(g_wsSessionSettled || g_wsApplyProcess || !g_wsManualRequired)return 92;
  char marker[MAX_PATH]{},backup[MAX_PATH]{};sprintf_s(marker,"%s\\left4dead2\\addons\\workshop\\3787239937.vpk",testRoot);sprintf_s(backup,"%s\\test-valid-download.vpk",testRoot);
  if(!CopyFileA(backup,marker,FALSE))return 93;
  clockTick+=600000;Pump();Pump();
 }
 if(declineUpdate){printf("declined: Prepare=%u full_extractions=%u dialogs=%u changelogs=%u\n",prepareStarts,fullExtractions,dialogs.load(),changelogs.load());return prepareStarts==0 && fullExtractions==0 && dialogs==1 && changelogs==0 && !g_wsApplyProcess?0:109;}
 if(!strcmp(mode,"one-preparation")){
  auto ms=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-consentAt).count();
  auto logMs=std::chrono::duration_cast<std::chrono::milliseconds>(changelogAt-consentAt).count();
  printf("Prepare_before_consent=%u total_Prepare=%u full_extractions=%u changelog_ms=%lld helper_ack_ms=%lld\n",preparesBeforeConsent,prepareStarts,fullExtractions,logMs,ms);
  if(preparesBeforeConsent!=0 || prepareStarts!=1 || fullExtractions!=1 || changelogs!=1 || !changelogBeforeApply || !changelogPrepared)return 110;
 }
 if(!g_wsApplyProcess || g_wsSessionSettled)return 94;
 if(!strcmp(mode,"missing-changelog")){
  printf("missing_optional_log document_dialogs=%u changelog_opens=%u updater_armed=1\n",documentDialogs.load(),changelogs.load());
  if(documentDialogs!=0 || changelogs!=0)return 111;
 }
 unsigned before=processStarts;
 for(unsigned i=0;i<100000;++i)Pump();
 if(processStarts!=before)return 95;
 printf("waiting_helper_observations=100000 duplicate_helper_starts=0\n");
 if(!strcmp(mode,"helper-retry")){
  TerminateProcess(g_wsApplyProcess,123);WaitForSingleObject(g_wsApplyProcess,5000);Pump();
  if(!g_wsApplyProcess || WaitForSingleObject(g_wsApplyProcess,0)!=WAIT_TIMEOUT || processStarts<=before)return 96;
  printf("dead acknowledged helper was rearmed\n");
 }
 return 0;
}
'''
    (native/'worker.cpp').write_text(cpp,encoding='utf-8')
    # A synthetic tiny future core tests archive discovery only; it is never
    # loaded or shipped as a game DLL and does not represent feature acceptance.
    (native/'tiny_core.cpp').write_text('extern "C" __declspec(dllexport) void necola_export(){}\nextern "C" __declspec(dllexport) const char* SkeetoBootstrapQuery(){return "Skeeto\\\\Maintain.ps1";}\n',encoding='utf-8')
    vswhere=Path(os.environ['ProgramFiles(x86)'])/'Microsoft Visual Studio/Installer/vswhere.exe'
    vs=Path(subprocess.check_output([str(vswhere),'-latest','-products','*','-requires','Microsoft.VisualStudio.Component.VC.Tools.x86.x64','-property','installationPath'],text=True).strip())
    build='@echo off\ncall "'+str(vs/'VC/Auxiliary/Build/vcvarsall.bat')+'" x86 >nul\n'
    for exe,flag in [('worker.exe',''),('current_worker.exe','/DTEST_CURRENT_CORE=1')]:
        build+='cl /nologo /MT /O2 /EHsc /std:c++17 /utf-8 '+flag+' /I "'+str(ROOT/'src/skeeto/core')+'" /Fe:'+exe+' worker.cpp bcrypt.lib user32.lib shell32.lib\nif errorlevel 1 exit /b 1\n'
    # Trivial exported constants need no CRT or entry point. Keep this valid
    # synthetic PE genuinely small rather than measuring incidental CRT size.
    build+='cl /nologo /LD /MT /O1 /Fe:tiny_core.dll tiny_core.cpp /link /NOENTRY /NODEFAULTLIB\n'
    (native/'build.bat').write_text(build,encoding='utf-8')
    subprocess.run(['cmd','/d','/c',str(native/'build.bat')],cwd=native,check=True)
    for mode in modes or ('idle','current-version','one-preparation','missing-changelog','declined-update','helper-retry','damaged-stage','manual-retry','launch-retry','slow-launch-retry','download-retry','ack-failure','no-tar','small-package','host-nonblocking','current-core-recovery','same-version-ready','pending-silent','apply-failed-start','apply-failed-budget'):
        root=fixture('worker-'+mode+("'s" if mode=='no-tar' else ''))
        if mode in ('current-core-recovery','same-version-ready','current-version'):
            stamp_dll_semver(root/'skeeto.dll', '2.0.2')
            refresh_manifest(root,'2.0.2')
        payload=native/('payload-'+mode);shutil.copytree(SOURCE,payload)
        if mode=='missing-changelog': (payload/'Skeeto_更新日志.txt').unlink()
        if mode=='small-package':
            shutil.copyfile(native/'tiny_core.dll',payload/'skeeto.dll')
            meta=json.loads((payload/'Skeeto/Manifest.json').read_text(encoding='utf-8-sig'))
            for entry in meta['files']:
                if entry['role']=='document': (payload/entry['relative'].replace('\\','/')).write_bytes(b'Optional future fixture documentation\n')
        if mode=='host-nonblocking':
            engine=payload/'Skeeto/Maintain.ps1'
            text=engine.read_text(encoding='utf-8-sig').replace("    $metadata=Get-SktPayload $PayloadRoot\n    if ($Mode -in", "    if ($Mode -eq 'Prepare') { Start-Sleep -Seconds 6 }\n    $metadata=Get-SktPayload $PayloadRoot\n    if ($Mode -in",1)
            assert 'Start-Sleep -Seconds 6' in text
            engine.write_text(text,encoding='utf-8-sig')
        if mode=='ack-failure':
            engine=payload/'Skeeto/Maintain.ps1'
            engine.write_text(engine.read_text(encoding='utf-8-sig').replace('$ack.Set() | Out-Null','$null = $ack'),encoding='utf-8-sig')
        same_version = mode in ('current-core-recovery', 'same-version-ready', 'current-version')
        version = '1.7.4' if mode == 'idle' else ('2.0.2' if same_version else '2.0.3')
        refresh_manifest(payload,version)
        workshop=root/'left4dead2/addons/workshop';workshop.mkdir(parents=True)
        with zipfile.ZipFile(workshop/'3787239937.vpk','w',zipfile.ZIP_DEFLATED) as packed:
            for file in payload.rglob('*'):
                if file.is_file():packed.write(file,file.relative_to(payload).as_posix())
        if mode=='small-package': assert (workshop/'3787239937.vpk').stat().st_size<200000
        if mode=='download-retry':
            complete=(workshop/'3787239937.vpk').read_bytes()
            (root/'test-valid-download.vpk').write_bytes(complete)
            (workshop/'3787239937.vpk').write_bytes(complete[:len(complete)//2])
        if mode=='damaged-stage':
            pending=stage(root,'2.0.2');prepared(root)
            (pending/'Skeeto/Maintain.ps1').write_bytes(b'broken staged engine')
        if mode=='current-core-recovery':
            pending=stage(root,'2.0.2',inject=lambda t:injected_boundary(t,"[IO.File]::WriteAllText((Join-Path $Root 'fault-boundary.txt'),'core replaced'); Start-Sleep -Seconds 60"))
            child=subprocess.Popen(command(root,'Migrate'),stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
            try:
                deadline=time.monotonic()+20
                while not (root/'fault-boundary.txt').exists() and time.monotonic()<deadline:
                    assert child.poll() is None;time.sleep(.1)
                assert (root/'fault-boundary.txt').exists()
            finally:
                if child.poll() is None:child.kill()
                child.wait(timeout=10)
            (pending/'VERSION.txt').unlink();(pending/'Skeeto/Maintain.ps1').unlink()
        if mode=='manual-retry':
            archive=workshop/'3787239937.vpk'
            (root/'test-valid-download.vpk').write_bytes(archive.read_bytes())
            meta_path=payload/'Skeeto/Manifest.json'
            meta=json.loads(meta_path.read_text(encoding='utf-8-sig'));meta['requires_manual_migrate']=True
            meta_path.write_text(json.dumps(meta,ensure_ascii=False),encoding='utf-8')
            with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as packed:
                for file in payload.rglob('*'):
                    if file.is_file():packed.write(file,file.relative_to(payload).as_posix())
        if mode=='current-version':
            stage(root,'2.0.2')  # Completed updates can leave stage files without READY.
            (root/'skeeto_ws_changelog_shown.txt').write_text('semver=2.0.2\n',encoding='ascii')
            (root/'skeeto_ws_show_changelog.txt').write_text('semver=2.0.2\n',encoding='ascii')
        if mode in ('pending-silent','apply-failed-start','apply-failed-budget'):
            pending=stage(root,'2.0.3');prepared(root)
            if mode!='pending-silent':(pending/'apply_failed.txt').write_bytes(b'previous Apply failed')
        if mode=='same-version-ready':
            pending=stage(root,'2.0.2');prepared(root)
            assert not (root/'skeeto_migrate_backup/active.json').exists()
        actor=native/('current_worker.exe' if mode in ('current-core-recovery','same-version-ready','current-version') else 'worker.exe')
        result=subprocess.run([str(actor),str(root),mode],capture_output=True,timeout=60)
        (native/(mode+'.log')).write_bytes(result.stdout+result.stderr)
        assert result.returncode==0,(mode,result.returncode,result.stdout+result.stderr)
        if mode not in ('idle','current-version','ack-failure','declined-update'):
            deadline=time.monotonic()+30
            while time.monotonic()<deadline:
                core, staged = root/'skeeto.dll', root/'skeeto_update/skeeto.dll'
                if staged.is_file() and core.is_file() and digest(core)==digest(staged) and not (root/'skeeto_update/READY').exists() and not (root/'skeeto_migrate_backup/active.json').exists() and not (root/'Skeeto/Manifest.json').exists() and not (root/'VERSION.txt').exists(): break
                time.sleep(.2)
            assert_applied(root)
            assert not (root/'Skeeto/Manifest.json').exists()
            assert not (root/'VERSION.txt').exists()
        elif mode=='ack-failure':
            assert (root/'skeeto_update/READY').exists()
            assert json.loads((root/'Skeeto/Manifest.json').read_text(encoding='utf-8-sig'))['semver']=='2.0.0'
        # Save full diagnostics, print only the measurement/result summary.
        measured=[line for line in result.stdout.decode(errors='replace').splitlines() if 'observations=' in line or 'shared_worker_calls=' in line or 'rearmed' in line]
        passed('actual C++ update worker: '+mode+'; '+'; '.join(measured))


def main():
    global SCRATCH
    SCRATCH = Path(tempfile.mkdtemp(prefix='skeeto-update-reliability-'))
    atexit.register(lambda: shutil.rmtree(SCRATCH, ignore_errors=True))
    check_failure_receipt_ownership()
    check_document_refresh()
    check_review_recovery()
    check_manual_maintenance()
    check_preparation_cleanup()
    root=fixture('prepare-locked-installed-core');stage(root);old=digest(root/'skeeto.dll')
    kernel,handle=lock(root/'skeeto.dll')
    try:
        prepared(root)
        assert (root/'skeeto_update/READY').exists() and not (root/'skeeto_migrate_backup/active.json').exists()
    finally: kernel.CloseHandle(handle)
    assert digest(root/'skeeto.dll')==old
    invoke(root,'Apply');assert_applied(root)
    passed('Prepare succeeds while installed core is exclusively locked; only exit Apply reads/plans installed bytes')
    root=fixture('locked-document'); stage(root); prepared(root)
    tutorial=root/'Skeeto/DIY/HitFeedback_命中反馈.txt'; tutorial.write_bytes(b'edited tutorial')
    kernel,handle=lock(tutorial)
    try: invoke(root,'Apply'); assert_applied(root)
    finally: kernel.CloseHandle(handle)
    assert tutorial.read_bytes()==b'edited tutorial'
    passed('locked/edited optional tutorial does not block core; no tutorial backup')

    root=fixture('bad-optional-payload');pending=stage(root)
    (pending/'Skeeto/DIY/HitFeedback_命中反馈.txt').write_bytes(b'corrupted optional document')
    prepared(root);invoke(root,'Apply');assert_applied(root)
    assert not (root/'Skeeto/DIY/HitFeedback_命中反馈.txt').exists()
    assert (pending/'Skeeto/DIY/HitFeedback_命中反馈.txt').read_bytes()==b'corrupted optional document'
    passed('a corrupted optional package document is not installed and does not block the critical payload')

    root=fixture('critical-lock');stage(root);prepared(root);old=digest(root/'skeeto.dll')
    kernel,handle=lock(root/'skeeto.dll')
    try: invoke(root,'Apply',1)
    finally: kernel.CloseHandle(handle)
    assert digest(root/'skeeto.dll')==old and (root/'skeeto_update/READY').exists()
    assert (root/'skeeto_update/apply_failed.txt').is_file()
    invoke(root,'Apply');assert_applied(root)
    assert not (root/'skeeto_update/apply_failed.txt').exists()
    passed('critical file lock retains READY and succeeds after lock release')

    # Honesty red line: locked proxy must fail the whole Apply (both critical
    # hashes unchanged), keep READY, and leave a staging apply_failed receipt.
    root=fixture('critical-lock-proxy');stage(root);prepared(root)
    old_core=digest(root/'skeeto.dll'); old_proxy=digest(root/'bin/wininet.dll')
    kernel,handle=lock(root/'bin/wininet.dll')
    try: invoke(root,'Apply',1)
    finally: kernel.CloseHandle(handle)
    assert digest(root/'skeeto.dll')==old_core and digest(root/'bin/wininet.dll')==old_proxy
    assert (root/'skeeto_update/READY').exists() and (root/'skeeto_update/apply_failed.txt').is_file()
    invoke(root,'Apply');assert_applied(root)
    assert not (root/'skeeto_update/apply_failed.txt').exists()
    passed('locked proxy fails whole Apply, keeps READY + apply_failed; succeeds after unlock')

    root=fixture('transient-lock-auto-retry');stage(root);prepared(root)
    kernel,handle=lock(root/'skeeto.dll')
    child=subprocess.Popen(command(root,'Apply'),stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
    try:
        time.sleep(3);assert child.poll() is None
        kernel.CloseHandle(handle);handle=None
        assert child.wait(timeout=30)==0;assert_applied(root)
    finally:
        if handle is not None: kernel.CloseHandle(handle)
        if child.poll() is None:child.kill()
        child.wait(timeout=10)
    passed('same Apply helper retries automatically and succeeds when a transient core lock clears')

    root=fixture('missing-owned');stage(root);(root/'SKEETO_WORKSHOP.txt').unlink()
    prepared(root);invoke(root,'Apply');assert_applied(root)
    passed('missing owned marker is repaired by a newer valid package')

    root=fixture('write-error');stage(root,inject=lambda t:injected_boundary(t,"throw [IO.IOException]::new('injected disk/write failure')"));prepared(root);old=digest(root/'skeeto.dll')
    invoke(root,'Apply',1);assert digest(root/'skeeto.dll')==old and (root/'skeeto_update/READY').exists()
    stage(root);prepared(root);invoke(root,'Apply');assert_applied(root)
    passed('write exception after first replacement rolls back and retries successfully')

    root=interrupted('killed-mid-write',lambda t:injected_boundary(t,"[IO.File]::WriteAllText((Join-Path $Root 'fault-boundary.txt'),'replaced'); Start-Sleep -Seconds 60"))
    journal=json.loads((root/'skeeto_migrate_backup/current/transaction.json').read_text(encoding='utf-8-sig'))
    old_hashes={e['before'] for e in journal['entries'] if e['before']}
    assert len(list((root/'skeeto_migrate_backup/current').glob('original-*.bin')))==len(old_hashes)
    passed('unfinished transaction stores identical old core bytes once, not one copy per DLL path')
    before=digest(root/'skeeto.dll');stage(root);prepared(root);assert digest(root/'skeeto.dll')==before
    invoke(root,'Apply');assert_applied(root)
    passed('killed helper midway: Prepare leaves installed bytes alone; Apply recovers and completes')

    def during_temp_copy(t):
        needle='        [IO.File]::Copy($Source,$temp,$true)'
        assert needle in t
        return t.replace(needle,needle+"\n        [IO.File]::WriteAllText((Join-Path $GameRoot 'fault-boundary.txt'),'temp copied'); Start-Sleep -Seconds 60",1)
    root=interrupted('killed-during-temp-copy',during_temp_copy)
    assert list(root.glob('*.skt-update.tmp'))
    stage(root);prepared(root);invoke(root,'Apply');assert_applied(root)
    assert not list(root.rglob('*.skt-update.tmp')) and not list((root/'skeeto_migrate_backup').glob('scratch-*'))
    passed('killed during temporary copy: retry reuses/cleans one reserved temp and old scratch')

    def during_journal_write(t):
        needle='    if ([IO.File]::Exists($Path)) { [IO.File]::Replace($temporary,$Path,[NullString]::Value) }'
        assert needle in t
        return t.replace(needle,"    if ([IO.Path]::GetFileName($Path) -eq 'transaction.json') { [IO.File]::WriteAllText((Join-Path $GameRoot 'fault-boundary.txt'),'journal temp flushed'); Start-Sleep -Seconds 60 }\n"+needle,1)
    root=interrupted('killed-during-journal-temp',during_journal_write)
    assert list((root/'skeeto_migrate_backup').glob('current/transaction.json.tmp'))
    assert not (root/'skeeto_migrate_backup/active.json').exists()
    stage(root);prepared(root);invoke(root,'Apply');assert_applied(root)
    assert not list((root/'skeeto_migrate_backup').glob('current/transaction.json.tmp'))
    assert not (root/'skeeto_migrate_backup/current').exists()
    passed('killed after journal temp flush: next successful update reclaims the inactive directory')

    def after_commit(t):
        return t.replace('        # The prepared journal is immutable',"        [IO.File]::WriteAllText((Join-Path $Root 'fault-boundary.txt'),'all applied'); Start-Sleep -Seconds 60\n        # The prepared journal is immutable",1)
    root=interrupted('killed-after-files',after_commit)
    assert not (root/'skeeto_update/READY').exists()
    # The same staged engine resumes without entering the injected transaction.
    invoke(root,'Apply');assert_applied(root)
    passed('killed after all targets: recognizes committed files, clears active record without downgrade')

    def game_restart(t):
        t=t.replace('function Assert-SktStopped {',"function Assert-SktStopped {\n    if ([IO.File]::Exists((Join-Path $GameRoot 'test-game-active.txt'))) { throw [IO.IOException]::new('injected game restart') }",1)
        return injected_boundary(t,"[IO.File]::WriteAllText((Join-Path $Root 'test-game-active.txt'),'game running')")
    root=fixture('game-restart');stage(root,inject=game_restart);prepared(root)
    invoke(root,'Apply',1)
    assert (root/'skeeto_migrate_backup/active.json').exists() and (root/'skeeto_update/READY').exists()
    (root/'test-game-active.txt').unlink();stage(root);prepared(root);invoke(root,'Apply');assert_applied(root)
    passed('game restart during commit: further writes stop, pending recovery completes after exit')

    root=fixture('repeat-updates')
    for version in ('2.0.2','2.0.3','2.0.4','2.0.5','2.0.6','2.0.7'):
        stage(root,version);prepared(root);invoke(root,'Apply');assert_applied(root)
        journals=list((root/'skeeto_migrate_backup').glob('*/transaction.json'))
        assert len(journals)==0, 'completed recovery data accumulated'
        assert not list((root/'skeeto_migrate_backup').glob('scratch-*'))
    passed('six consecutive updates retain zero completed recovery transactions and zero scratch folders')

    root=fixture('manual-record-budget');stage(root);prepared(root);invoke(root,'Apply')
    previous=root/'skeeto_migrate_backup/update-20200101T000000-ffffffffffffffffffffffffffffffff'
    previous.mkdir(parents=True)
    (previous/'transaction.json').write_text(json.dumps(dict(schema=1,gameRoot=str(root),operation='Apply',entries=[])),encoding='utf-8')
    (previous/'result.txt').write_bytes(b'committed')
    (previous/'result.txt.tmp').write_bytes(b'interrupted outcome write')
    for i in range(140):
        directory=root/'skeeto_migrate_backup'/('20200101T000000-'+format(i,'032x'));directory.mkdir()
        (directory/'manual-user-sentinel.txt').write_bytes(b'keep manual records')
    stage(root,'2.0.2');prepared(root);invoke(root,'Apply');assert_applied(root)
    assert not previous.exists()
    assert len(list((root/'skeeto_migrate_backup').glob('*/transaction.json')))==0
    assert len(list((root/'skeeto_migrate_backup').glob('*/manual-user-sentinel.txt')))==140
    passed('140 manual records cannot starve cleanup; old automatic record/control temp removed, manual files intact')

    # Keep one accepted synthetic process alive while two Apply processes start.
    root=fixture('two-helpers');stage(root);prepared(root)
    sleeper=subprocess.Popen(['powershell.exe','-NoProfile','-Command','Start-Sleep -Seconds 15'])
    query=subprocess.check_output(['powershell.exe','-NoProfile','-Command',f'(Get-Process -Id {sleeper.pid}).StartTime.ToUniversalTime().ToFileTimeUtc()'],text=True).strip()
    owner=subprocess.Popen(command(root,'Apply','-WaitPid',str(sleeper.pid),'-WaitStartTime',query),stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
    try:
        time.sleep(2);assert owner.poll() is None
        invoke(root,'Apply',1)
        assert (root/'skeeto_update/READY').exists()
        assert owner.wait(timeout=30)==0
        assert_applied(root)
    finally:
        if sleeper.poll() is None: sleeper.kill()
        sleeper.wait(timeout=10)
        if owner.poll() is None: owner.kill()
        owner.wait(timeout=10)
    passed('only one helper can wait/write; duplicate exits retryably without touching the stage')

    # WaitPid exits while another left4dead2* is still alive (quick relaunch overlap).
    # Apply must poll-wait that process out instead of Assert-failing and keeping READY.
    root = fixture('apply-waits-outlasting-relaunch')
    stage(root); prepared(root)
    bins = SCRATCH / 'fake-l4d2-bin'
    bins.mkdir(parents=True, exist_ok=True)
    fake_game = bins / 'left4dead2.exe'
    import sys as _sys
    shutil.copyfile(_sys.executable, fake_game)
    # Overlap: relaunch outlives WaitPid so the post-PID poll must observe it.
    relaunch = subprocess.Popen([str(fake_game), '-c', 'import time; time.sleep(8)'], cwd=str(bins))
    wait_pid = subprocess.Popen(['powershell.exe', '-NoProfile', '-Command', 'Start-Sleep -Seconds 2'])
    query = subprocess.check_output(
        ['powershell.exe', '-NoProfile', '-Command',
         f'(Get-Process -Id {wait_pid.pid}).StartTime.ToUniversalTime().ToFileTimeUtc()'],
        text=True).strip()
    owner = subprocess.Popen(command(root, 'Apply', '-WaitPid', str(wait_pid.pid), '-WaitStartTime', query),
                             stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    try:
        assert wait_pid.wait(timeout=10) == 0
        assert relaunch.poll() is None, 'relaunch should still be alive when WaitPid exits'
        assert owner.wait(timeout=60) == 0, (owner.stdout.read() if owner.stdout else b'')
        assert_applied(root)
        assert relaunch.wait(timeout=15) == 0
    finally:
        for proc in (wait_pid, relaunch, owner):
            if proc and proc.poll() is None:
                proc.kill()
                proc.wait(timeout=10)
    passed('Apply polls until a quick left4dead2 relaunch exits, then clears READY')

    # Apply holds the maintenance mutex. A concurrent Prepare (same lock name the
    # DLL probes) must leave READY and stage bytes alone — no ClearDir/re-extract.
    root = fixture('busy-helper-leaves-stage-untouched')
    stage(root); prepared(root)
    sentinel = root / 'skeeto_update' / 'clear-dir-sentinel.bin'
    sentinel.write_bytes(b'stage must survive busy helper')
    ready_before = (root / 'skeeto_update' / 'READY').read_bytes()
    maintain_before = digest(root / 'skeeto_update' / 'Skeeto' / 'Maintain.ps1')
    sleeper = subprocess.Popen(['powershell.exe', '-NoProfile', '-Command', 'Start-Sleep -Seconds 20'])
    query = subprocess.check_output(
        ['powershell.exe', '-NoProfile', '-Command',
         f'(Get-Process -Id {sleeper.pid}).StartTime.ToUniversalTime().ToFileTimeUtc()'],
        text=True).strip()
    owner = subprocess.Popen(command(root, 'Apply', '-WaitPid', str(sleeper.pid), '-WaitStartTime', query),
                             stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    try:
        time.sleep(2)
        assert owner.poll() is None
        invoke(root, 'Prepare', 1, '-UpdateProtocol', '1')
        assert sentinel.read_bytes() == b'stage must survive busy helper'
        assert (root / 'skeeto_update' / 'READY').read_bytes() == ready_before
        assert digest(root / 'skeeto_update' / 'Skeeto' / 'Maintain.ps1') == maintain_before
        sleeper.wait(timeout=30)
        assert owner.wait(timeout=60) == 0
        assert_applied(root)
        # Unknown sentinel is outside the managed payload list; Apply need not delete it.
    finally:
        for proc in (sleeper, owner):
            if proc.poll() is None:
                proc.kill()
                proc.wait(timeout=10)
    passed('Apply holding lock: concurrent Prepare leaves READY/stage untouched (no ClearDir)')
    check_worker()


if __name__=='__main__': main()
