[Setup]
AppName=GtkHash
AppVersion=1.5.2
DefaultDirName={autopf}\GtkHash
DefaultGroupName=GtkHash
SetupIconFile=dist\bin\gtkhash.ico
UninstallDisplayIcon={app}\GtkHash.exe
LicenseFile=dist\COPYING.txt
Compression=lzma2
SolidCompression=yes
OutputDir=.
OutputBaseFilename=gtkhash-installer
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64

[Files]
Source: "dist\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
; Start Menu shortcut targets the root launcher (carries the correct icon)
Name: "{group}\GtkHash"; Filename: "{app}\GtkHash.exe"; WorkingDir: "{app}"; IconFilename: "{app}\GtkHash.exe"
; Desktop shortcut (optional) also targets the root launcher
Name: "{autodesktop}\GtkHash"; Filename: "{app}\GtkHash.exe"; WorkingDir: "{app}"; IconFilename: "{app}\GtkHash.exe"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Run]
Filename: "{app}\GtkHash.exe"; Description: "{cm:LaunchProgram,GtkHash}"; Flags: nowait postinstall skipifsilent