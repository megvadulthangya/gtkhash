[Setup]
AppName=GtkHash
AppVersion=1.5
DefaultDirName={autopf}\GtkHash
DefaultGroupName=GtkHash
UninstallDisplayIcon={app}\bin\gtkhash.exe
Compression=lzma2
SolidCompression=yes
OutputDir=.
OutputBaseFilename=gtkhash-installer
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64

[Files]
Source: "dist\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\GtkHash"; Filename: "{app}\bin\gtkhash.exe"; WorkingDir: "{app}\bin"
Name: "{autodesktop}\GtkHash"; Filename: "{app}\bin\gtkhash.exe"; WorkingDir: "{app}\bin"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Run]
Filename: "{app}\bin\gtkhash.exe"; Description: "{cm:LaunchProgram,GtkHash}"; Flags: nowait postinstall skipifsilent