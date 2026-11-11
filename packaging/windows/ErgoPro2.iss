; ErgoPro2 setup program for Windows 11 (Inno Setup 6), 1.7.4.
;
; Built by scripts\package-windows.ps1 -Installer (also in CI):
;   ISCC.exe /DAppVersion=<x.y.z> /DSourceDir=<deployed ErgoPro2 folder> /DOutputDir=<dist> ErgoPro2.iss
;
; Behavior (implementation_stages.md B.1.3):
;   - Per-user installation, never asks for an administrator password. Default
;     folder %LOCALAPPDATA%\Programs\ErgoPro2 (the same as the app's own Install
;     button); the user can choose another folder on the first installation.
;   - Upgrades install into the folder of the previous installation without
;     asking (UsePreviousAppDir + DisableDirPage=auto).
;   - A running ErgoPro2 is closed first: "ErgoPro2.exe --quit" (exits like the
;     Exit button, saves everything); if it is still running after that, the
;     process is ended.
;   - Start Menu entry, optional desktop icon, optional start at login (the same
;     registry value the app uses, so the General tab shows "Installed ... starts
;     at login").
;   - Settings, logs and statistics (%APPDATA%\techforge\ErgoPro2, %LOCALAPPDATA%
;     ...) are never touched, neither by the setup nor by the uninstaller.

#ifndef AppVersion
  #error AppVersion is not defined (ISCC /DAppVersion=x.y.z)
#endif
#ifndef SourceDir
  #error SourceDir is not defined (ISCC /DSourceDir=<folder with ErgoPro2.exe>)
#endif
#ifndef OutputDir
  #define OutputDir "."
#endif

[Setup]
; Never change the AppId: upgrades and the app (WinInstaller::setupRegistryKey)
; find the installation through it.
AppId={{6B0E2F3A-8C4D-4E1B-9A57-2D3C8E9F1A42}
AppName=ErgoPro2
AppVersion={#AppVersion}
AppVerName=ErgoPro2 {#AppVersion}
AppPublisher=TechForge
VersionInfoVersion={#AppVersion}
DefaultDirName={autopf}\ErgoPro2
DefaultGroupName=ErgoPro2
DisableProgramGroupPage=yes
DisableDirPage=auto
UsePreviousAppDir=yes
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
; ErgoPro2 is closed by the [Code] section below, not by the Restart Manager
; (the app only exits through its Exit button or --quit).
CloseApplications=no
RestartApplications=no
SetupIconFile=..\..\resources\icons\app\ErgoPro2.ico
UninstallDisplayIcon={app}\ErgoPro2.exe
UninstallDisplayName=ErgoPro2
WizardStyle=modern
Compression=lzma2/max
SolidCompression=yes
OutputDir={#OutputDir}
OutputBaseFilename=ErgoPro2-{#AppVersion}-windows-x64-setup

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "autostart"; Description: "Start ErgoPro2 automatically when I log in (recommended)"; GroupDescription: "Options:"
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\ErgoPro2"; Filename: "{app}\ErgoPro2.exe"
Name: "{autodesktop}\ErgoPro2"; Filename: "{app}\ErgoPro2.exe"; Tasks: desktopicon

[Registry]
; Same value the app writes for "start at login" (WinAutostart); removed on uninstall.
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "ErgoPro2"; \
    ValueData: """{app}\ErgoPro2.exe"" --autostart"; Flags: uninsdeletevalue; Tasks: autostart
; Without the task the value is still removed on uninstall (the app may have created it).
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: none; ValueName: "ErgoPro2"; \
    Flags: uninsdeletevalue dontcreatekey; Tasks: not autostart

[Run]
Filename: "{app}\ErgoPro2.exe"; Description: "{cm:LaunchProgram,ErgoPro2}"; Flags: nowait postinstall skipifsilent

[UninstallRun]
; Close a running ErgoPro2 before its files are removed.
Filename: "{app}\ErgoPro2.exe"; Parameters: "--quit"; Flags: runhidden waituntilterminated; RunOnceId: "QuitErgoPro2"
Filename: "{sys}\taskkill.exe"; Parameters: "/F /IM ErgoPro2.exe"; Flags: runhidden waituntilterminated; RunOnceId: "KillErgoPro2"

[Code]
function IsErgoPro2Running(): Boolean;
var
  ResultCode: Integer;
begin
  { tasklist /FI ... | find: exit code 0 when the process exists }
  Result := Exec(ExpandConstant('{cmd}'), '/C tasklist /FI "IMAGENAME eq ErgoPro2.exe" /NH | find /I "ErgoPro2.exe" >NUL',
                 '', SW_HIDE, ewWaitUntilTerminated, ResultCode) and (ResultCode = 0);
end;

function CloseRunningErgoPro2(): String;
var
  ResultCode, Waited: Integer;
  Installed: String;
begin
  Result := '';
  if not IsErgoPro2Running() then
    Exit;
  Log('ErgoPro2 is running; asking it to exit');
  { An installed ErgoPro2 >= 1.7.4 understands --quit (saves settings and statistics, releases a held mouse button). }
  Installed := ExpandConstant('{app}\ErgoPro2.exe');
  if FileExists(Installed) then
    Exec(Installed, '--quit', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
  Waited := 0;
  while IsErgoPro2Running() and (Waited < 10) do
  begin
    Sleep(500);
    Waited := Waited + 1;
  end;
  if IsErgoPro2Running() then
  begin
    { Older versions (portable zip) do not know --quit: end the process. }
    Log('ErgoPro2 did not exit; ending the process');
    Exec(ExpandConstant('{sys}\taskkill.exe'), '/F /IM ErgoPro2.exe', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
    Sleep(1000);
  end;
  if IsErgoPro2Running() then
    Result := 'ErgoPro2 is still running. Close it (tray icon > Exit) and run the setup again.';
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
begin
  Result := CloseRunningErgoPro2();
end;
