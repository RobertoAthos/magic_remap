; Inno Setup script for MagicRemap
; Build with: iscc magic_remap.iss
; Pré-requisito: x64\Release\magic_remap.exe já compilado.

#define MyAppName       "MagicRemap"
#define MyAppVersion    "1.0.0"
#define MyAppPublisher  "MagicRemap"
#define MyAppURL        "https://github.com/robertoathos/magic_remap"
#define MyAppExeName    "magic_remap.exe"

[Setup]
AppId={{B5A1F742-3C8E-4D9A-A6F1-2B7C9D0E1F23}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
LicenseFile=..\LICENSE
OutputDir=Output
OutputBaseFilename=MagicRemapSetup-{#MyAppVersion}
Compression=lzma2/ultra
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
UninstallDisplayIcon={app}\{#MyAppExeName}

[Languages]
Name: "english";    MessagesFile: "compiler:Default.isl"
Name: "portuguese"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"

[Tasks]
Name: "autostart"; Description: "Iniciar automaticamente com o Windows"; GroupDescription: "Opções:"; Flags: checkedonce

[Files]
Source: "..\x64\Release\magic_remap.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\README.md";                   DestDir: "{app}"; Flags: ignoreversion isreadme
Source: "..\LICENSE";                     DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}";       Filename: "{app}\{#MyAppExeName}"
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"

[Registry]
; Autostart por usuário (HKCU). Removido junto na desinstalação.
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; \
    ValueType: string; ValueName: "MagicRemap"; ValueData: """{app}\{#MyAppExeName}"""; \
    Tasks: autostart; Flags: uninsdeletevalue

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Iniciar {#MyAppName} agora"; \
    Flags: nowait postinstall skipifsilent

[UninstallRun]
; Mata processo em execução antes de remover binários.
Filename: "{sys}\taskkill.exe"; Parameters: "/IM {#MyAppExeName} /F"; \
    Flags: runhidden; RunOnceId: "KillMagicRemap"
