[Setup]
AppName=KIWII Language
AppVersion=4.0
DefaultDirName={autopf}\Kiwii
LicenseFile=LICENSE
OutputBaseFilename=KiwiiSetup-v4.0
OutputDir=.
PrivilegesRequired=lowest
ChangesEnvironment=yes
Compression=lzma2
SolidCompression=yes


[Tasks]
Name: "addpath"; Description: "Agregar kiwii.exe al PATH de Windows (Recomendado)"
Name: "vscodeext"; Description: "Instalar extension de colores e icono para VS Code (.vsix)"

[Files]
Source: "kiwii.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "estandar.kiw"; DestDir: "{app}"; Flags: ignoreversion
Source: "motor.kiw"; DestDir: "{app}"; Flags: ignoreversion
Source: "lib\*"; DestDir: "{app}\lib"; Flags: ignoreversion recursesubdirs
Source: "pruebas\*"; DestDir: "{app}\pruebas"; Flags: ignoreversion recursesubdirs
Source: "kiwii-lang-1.1.0.vsix"; DestDir: "{app}"; Flags: ignoreversion
Source: "LICENSE"; DestDir: "{app}"; Flags: ignoreversion

[Registry]
Root: HKCU; Subkey: "Environment"; ValueType: expandsz; ValueName: "Path"; ValueData: "{olddata};{app}"; Tasks: addpath; Check: NeedsAddPath(ExpandConstant('{app}'))

[Run]
Filename: "powershell.exe"; Parameters: "-NoProfile -WindowStyle Hidden -Command ""code --install-extension '{app}\kiwii-lang-1.0.0.vsix'"""; StatusMsg: "Instalando extension en VS Code..."; Tasks: vscodeext; Flags: runhidden

[Code]
function NeedsAddPath(Param: string): boolean;
var
  OrigPath: string;
begin
  if not RegQueryStringValue(HKEY_CURRENT_USER, 'Environment', 'Path', OrigPath)
  then begin
    Result := True;
    exit;
  end;
  Result := Pos(';' + Param + ';', ';' + OrigPath + ';') = 0;
end;