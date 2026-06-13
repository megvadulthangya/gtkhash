[Setup]
AppName=GtkHash
AppVersion=1.5
DefaultDirName={autopf}\GtkHash
DefaultGroupName=GtkHash
SetupIconFile=dist\bin\gtkhash.ico
UninstallDisplayIcon={app}\bin\gtkhash.ico
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
; Start Menu shortcut with custom icon
Name: "{group}\GtkHash"; Filename: "{app}\bin\org.gtkhash.gtkhash.exe"; WorkingDir: "{app}\bin"; IconFilename: "{app}\bin\gtkhash.ico"
; Desktop shortcut (optional) with the same custom icon
Name: "{autodesktop}\GtkHash"; Filename: "{app}\bin\org.gtkhash.gtkhash.exe"; WorkingDir: "{app}\bin"; IconFilename: "{app}\bin\gtkhash.ico"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Run]
Filename: "{app}\bin\org.gtkhash.gtkhash.exe"; Description: "{cm:LaunchProgram,GtkHash}"; Flags: nowait postinstall skipifsilent