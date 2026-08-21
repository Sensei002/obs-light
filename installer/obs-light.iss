; obs-light Inno Setup installer script
; Produces obs-light-Setup-x64.exe

#define MyAppName "obs-light"
#define MyAppVersion "0.1.0"
#define MyAppPublisher "obs-light contributors"
#define MyAppURL "https://github.com/obs-light/obs-light"
#define MyAppExeName "obs-light.exe"

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
OutputBaseFilename=obs-light-Setup-x64
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
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
; Executable
Source: "..\build\rundir\Release\bin\64bit\{#MyAppExeName}"; DestDir: "{app}\bin\64bit"; Flags: ignoreversion

; Core OBS DLLs
Source: "..\build\rundir\Release\bin\64bit\obs*.dll"; DestDir: "{app}\bin\64bit"; Flags: ignoreversion
Source: "..\build\rundir\Release\bin\64bit\libobs*.dll"; DestDir: "{app}\bin\64bit"; Flags: ignoreversion

; Plugins
Source: "..\build\rundir\Release\obs-plugins\64bit\*.dll"; DestDir: "{app}\obs-plugins\64bit"; Flags: ignoreversion skipifsourcedoesntexist

; Plugin data (locale, etc.)
Source: "..\build\rundir\Release\data\obs-plugins\*"; DestDir: "{app}\data\obs-plugins"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist

; Graphics hook executables (win-capture)
Source: "..\build\rundir\Release\data\obs-plugins\win-capture\*"; DestDir: "{app}\data\obs-plugins\win-capture"; Flags: ignoreversion recursesubdirs skipifsourcedoesntexist
Source: "..\build\rundir\Release\obs-plugins\win-capture\*"; DestDir: "{app}\obs-plugins\win-capture"; Flags: ignoreversion recursesubdirs skipifsourcedoesntexist

; FFmpeg-mux helper
Source: "..\build\rundir\Release\data\obs-plugins\obs-ffmpeg\*"; DestDir: "{app}\data\obs-plugins\obs-ffmpeg"; Flags: ignoreversion recursesubdirs skipifsourcedoesntexist

; License
Source: "..\COPYING"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\bin\64bit\{#MyAppExeName}"
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\bin\64bit\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\bin\64bit\{#MyAppExeName}"; Description: "Launch {#MyAppName}"; Flags: nowait postinstall skipifsilent