; Windows installer (Inno Setup 6), built by packaging/make-package.py (ADR 0020). Unsigned (PNS-49).
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
; The installer exe shows the game icon (relative to this script, ADR 0030)
SetupIconFile=..\icons\roboviolence2.ico
Compression=lzma2
SolidCompression=yes
WizardStyle=modern

[Tasks]
Name: desktopicon; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
; The client asks master.roboviolence.com for the server list (ADR 0028); this turns that off (SignPath Foundation terms)
Name: nomaster; Description: "Don't contact the public server list at master.roboviolence.com (join servers by address only)"; GroupDescription: "Privacy:"; Flags: unchecked

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Excludes: "master.db,web.db"; Flags: recursesubdirs ignoreversion
; The master's databases hold bans and accounts: never overwritten by an upgrade or removed on uninstall
Source: "{#SourceDir}\master.db"; DestDir: "{app}"; Flags: onlyifdoesntexist uninsneveruninstall
Source: "{#SourceDir}\web.db"; DestDir: "{app}"; Flags: onlyifdoesntexist uninsneveruninstall

[Icons]
Name: "{group}\Robo Violence 2"; Filename: "{app}\bv2.exe"; WorkingDir: "{app}"
Name: "{group}\Uninstall Robo Violence 2"; Filename: "{uninstallexe}"
Name: "{autodesktop}\Robo Violence 2"; Filename: "{app}\bv2.exe"; WorkingDir: "{app}"; Tasks: desktopicon

[UninstallDelete]
; Written by the nomaster task (CurStepChanged), so not in the uninstall log
Type: files; Name: "{app}\config\local\no-master.cfg"

[Run]
Filename: "{app}\bv2.exe"; Description: "{cm:LaunchProgram,Robo Violence 2}"; WorkingDir: "{app}"; Flags: nowait postinstall skipifsilent

[Code]
const
  PrivacyURL = 'https://roboviolence.com/privacy.html';
  NoMasterFile = '{app}\config\local\no-master.cfg'; // a local config layer, never saved over (config/README.md)

procedure PrivacyLinkClick(Sender: TObject);
var
  ErrorCode: Integer;
begin
  ShellExecAsOriginalUser('open', PrivacyURL, '', '', SW_SHOWNORMAL, ewNoWait, ErrorCode);
end;

// The privacy policy, as a link at the bottom left of every wizard page (SignPath Foundation: shown during installation)
procedure InitializeWizard();
var
  Link: TNewStaticText;
begin
  Link := TNewStaticText.Create(WizardForm);
  Link.Parent := WizardForm;
  Link.Caption := 'Privacy policy';
  Link.Cursor := crHand;
  Link.OnClick := @PrivacyLinkClick;
  Link.Font.Style := Link.Font.Style + [fsUnderline];
  Link.Font.Color := clHotLight;
  Link.Left := ScaleX(20);
  Link.Top := WizardForm.CancelButton.Top + (WizardForm.CancelButton.Height - Link.Height) div 2;
end;

// The nomaster task writes the opt-out file; installing again without it removes the file
procedure CurStepChanged(CurStep: TSetupStep);
var
  Path: String;
begin
  if CurStep <> ssPostInstall then Exit;
  Path := ExpandConstant(NoMasterFile);
  if WizardIsTaskSelected('nomaster') then
  begin
    ForceDirectories(ExtractFileDir(Path));
    SaveStringToFile(Path, 'cl_masterServers "none"' + #13#10, False);
  end
  else
    DeleteFile(Path);
end;
