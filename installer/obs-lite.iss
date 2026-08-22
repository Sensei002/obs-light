; obs-lite Inno Setup installer script
; Produces obs-lite-Setup-x64.exe
;
; Build from CI:
;   ISCC.exe /DMyAppVersion=0.1.0 /DConfigName=RelWithDebInfo obs-lite.iss

#define MyAppName "obs-lite"
#ifndef MyAppVersion
#define MyAppVersion "0.1.1"
#endif
#define MyAppPublisher "obs-lite contributors"
#define MyAppURL "https://github.com/Sensei002/obs-light"
#define MyAppExeName "obs-lite.exe"
#ifndef ConfigName
#define ConfigName "Release"
#endif

[Setup]
AppId={{B0A1E2C3-D4E5-6789-ABCD-EF0123456789}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
AllowNoIcons=yes
OutputDir=..\build\installer
OutputBaseFilename=obs-lite-Setup-x64
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
SetupIconFile=..\frontend\resources\obs-lite.ico
UninstallDisplayIcon={app}\bin\64bit\{#MyAppExeName}
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
CloseApplications=yes
RestartApplications=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Additional icons:"; Flags: checkedonce

[Files]
; Executable and all runtime files: Qt DLLs and plugin folders (platforms/,
; styles/, ...), obs.dll, libobs-d3d11.dll, ffmpeg-mux, ...
Source: "..\build\rundir\{#ConfigName}\bin\64bit\*"; DestDir: "{app}\bin\64bit"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "*.pdb"

; Plugins
Source: "..\build\rundir\{#ConfigName}\obs-plugins\64bit\*.dll"; DestDir: "{app}\obs-plugins\64bit"; Flags: ignoreversion skipifsourcedoesntexist

; libobs data (shaders/effects, images, locale) - required for video init
Source: "..\build\rundir\{#ConfigName}\data\libobs\*"; DestDir: "{app}\data\libobs"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist

; Plugin data (locale files, graphics-hook executables, compatibility data)
Source: "..\build\rundir\{#ConfigName}\data\obs-plugins\*"; DestDir: "{app}\data\obs-plugins"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist

; License
Source: "..\COPYING"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\bin\64bit\{#MyAppExeName}"
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\bin\64bit\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\bin\64bit\{#MyAppExeName}"; Description: "Launch {#MyAppName}"; Flags: nowait postinstall skipifsilent