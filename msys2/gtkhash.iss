[Setup]
AppName=GtkHash
AppVersion=1.5
DefaultDirName={autopf}\GtkHash
DefaultGroupName=GtkHash
UninstallDisplayIcon={app}\gtkhash.exe
Compression=lzma2
SolidCompression=yes
OutputDir=.
OutputBaseFilename=gtkhash-installer
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64

[Files]
Source: "dist\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\GtkHash"; Filename: "{app}\gtkhash.exe"; WorkingDir: "{app}"
Name: "{autodesktop}\GtkHash"; Filename: "{app}\gtkhash.exe"; WorkingDir: "{app}"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Run]
Filename: "{app}\gtkhash.exe"; Description: "{cm:LaunchProgram,GtkHash}"; Flags: nowait postinstall skipifsilent