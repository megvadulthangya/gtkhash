[Setup]
AppName=GtkHash
AppVersion=1.0-git
DefaultDirName={autopf}\GtkHash
DefaultGroupName=GtkHash
UninstallDisplayIcon={app}\bin\gtkhash.exe
Compression=lzma2/max
SolidCompression=yes
OutputDir=.
OutputBaseFilename=gtkhash-installer
ArchitecturesInstallIn64BitMode=x64
DisableWelcomePage=no
DisableDirPage=no

[Files]
; Beolvassa a bundle.sh által tökéletesen felépített dist mappa teljes tartalmát
Source: "dist\*"; DestDir: "{app}"; Flags: recursesubdirs createallsubdirs

[Icons]
; Parancsikonok létrehozása a Start menüben és az Asztalon a megfelelő munkakönyvtárral
Name: "{group}\GtkHash"; Filename: "{app}\bin\gtkhash.exe"; WorkingDir: "{app}\bin"
Name: "{autodesktop}\GtkHash"; Filename: "{app}\bin\gtkhash.exe"; WorkingDir: "{app}\bin"

[Run]
; Opció a telepítés végén a program azonnali elindítására
Filename: "{app}\bin\gtkhash.exe"; Description: "GtkHash indítása most"; Flags: nowait postinstall skipifsilent