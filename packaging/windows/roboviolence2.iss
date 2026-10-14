; Windows installer (Inno Setup 6), built by packaging/make-package.py (ADR 0018). Unsigned (PNS-49).
; ISCC /DSourceDir=<package folder> /DOutputDir=<dir> /DOutputName=<file name> /DAppVersion=<version> roboviolence2.iss

[Setup]
; AppId identifies the installed app for upgrades and uninstall: never change it
AppId={{14FF5586-3A63-4D11-8996-60EAD6BAEFCD}
AppName=Robo Violence 2
AppVersion={#AppVersion}
AppPublisher=Robo Violence 2 contributors
DefaultDirName={autopf}\Robo Violence 2
DefaultGroupName=Robo Violence 2
; Per-user install, no admin prompt: the folder stays writable for the servers' databases
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
; Windows 10 22H2 (ADR 0012)
MinVersion=10.0.19045
; GPLv3 needs no acceptance to run the program (section 9): shown, not agreed to
InfoBeforeFile={#SourceDir}\LICENSE.txt
OutputDir={#OutputDir}
OutputBaseFilename={#OutputName}
UninstallDisplayIcon={app}\bv2.exe
Compression=lzma2
SolidCompression=yes
WizardStyle=modern

[Tasks]
Name: desktopicon; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Excludes: "master.db,web.db"; Flags: recursesubdirs ignoreversion
; The master's databases hold bans and accounts: never overwritten by an upgrade or removed on uninstall
Source: "{#SourceDir}\master.db"; DestDir: "{app}"; Flags: onlyifdoesntexist uninsneveruninstall
Source: "{#SourceDir}\web.db"; DestDir: "{app}"; Flags: onlyifdoesntexist uninsneveruninstall

[Icons]
Name: "{group}\Robo Violence 2"; Filename: "{app}\bv2.exe"; WorkingDir: "{app}"
Name: "{group}\Uninstall Robo Violence 2"; Filename: "{uninstallexe}"
Name: "{autodesktop}\Robo Violence 2"; Filename: "{app}\bv2.exe"; WorkingDir: "{app}"; Tasks: desktopicon

[Run]
Filename: "{app}\bv2.exe"; Description: "{cm:LaunchProgram,Robo Violence 2}"; WorkingDir: "{app}"; Flags: nowait postinstall skipifsilent
