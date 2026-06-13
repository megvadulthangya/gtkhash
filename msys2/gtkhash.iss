[Setup]
AppName=GtkHash
AppVersion=1.5
DefaultDirName={autopf}\GtkHash
DefaultGroupName=GtkHash
UninstallDisplayIcon={app}\bin\gtkhash.ico
Compression=lzma2
SolidCompression=yes
OutputDir=.
OutputBaseFilename=gtkhash-installer
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64

[Files]
Source: "dist\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\GtkHash"; Filename: "{app}\bin\org.gtkhash.gtkhash.exe"; WorkingDir: "{app}\bin"; IconFilename: "{app}\bin\gtkhash.ico"
Name: "{autodesktop}\GtkHash"; Filename: "{app}\bin\org.gtkhash.gtkhash.exe"; WorkingDir: "{app}\bin"; IconFilename: "{app}\bin\gtkhash.ico"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Run]
Filename: "{app}\bin\org.gtkhash.gtkhash.exe"; Description: "{cm:LaunchProgram,GtkHash}"; Flags: nowait postinstall skipifsilent