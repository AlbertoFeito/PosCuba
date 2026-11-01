; Instalador de PosCuba (Inno Setup 6.3 o superior).
; Lo compila scripts\deploy.ps1 a partir de dist\PosCuba:
;   ISCC.exe /DAppVersion=1.2.0 /DSourceDir=..\dist\PosCuba /DOutputDir=..\dist installer\PosCuba.iss
; No toca los datos: la base de datos (%APPDATA%\PosCuba) y los respaldos (Documentos\PosCuba)
; se conservan al actualizar y al desinstalar.

#ifndef AppVersion
  #define AppVersion "0.0.0"
#endif
#ifndef SourceDir
  #define SourceDir "..\dist\PosCuba"
#endif
#ifndef OutputDir
  #define OutputDir "..\dist"
#endif

[Setup]
AppId={{6E0C6B4A-3F1D-4C2B-9D57-5A9A1C2E7B11}
AppName=PosCuba
AppVersion={#AppVersion}
AppVerName=PosCuba {#AppVersion}
AppPublisher=PosCuba
VersionInfoVersion={#AppVersion}
DefaultDirName={autopf}\PosCuba
DefaultGroupName=PosCuba
DisableProgramGroupPage=yes
; Sin permisos de administrador por defecto (instala para el usuario); se puede elegir "todos los usuarios".
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
SetupIconFile=..\resources\icons\poscuba.ico
UninstallDisplayIcon={app}\PosCuba.exe
OutputDir={#OutputDir}
OutputBaseFilename=PosCuba-{#AppVersion}-Setup
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
CloseApplications=yes
RestartApplications=no

[Languages]
Name: "es"; MessagesFile: "compiler:Languages\Spanish.isl"

[Tasks]
Name: "desktopicon"; Description: "Crear un acceso directo en el escritorio"; GroupDescription: "Accesos directos:"

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\PosCuba"; Filename: "{app}\PosCuba.exe"
Name: "{group}\PosCuba (modo compatible)"; Filename: "{app}\PosCuba.exe"; Parameters: "--software"; \
    Comment: "Usar si la ventana sale en negro o parpadea"
Name: "{group}\Desinstalar PosCuba"; Filename: "{uninstallexe}"
Name: "{autodesktop}\PosCuba"; Filename: "{app}\PosCuba.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\PosCuba.exe"; Description: "Abrir PosCuba ahora"; Flags: nowait postinstall skipifsilent
