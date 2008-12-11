[Files]
Source: lib/bzip2.dll; DestDir: {app}
Source: ../.libs/libtty-0.dll; DestDir: {app}; Flags: ignoreversion
Source: ../.libs/proxyrec.exe; DestDir: {app}; Flags: ignoreversion
Source: ../.libs/termplay.exe; DestDir: {app}; Flags: ignoreversion
Source: ../.libs/termrec.exe; DestDir: {app}; Flags: ignoreversion
Source: lib/zlib1.dll; DestDir: {app}
[_ISTool]
UseAbsolutePaths=false
[Setup]
OutputBaseFilename=termrec-0.15
SolidCompression=true
PrivilegesRequired=none
DefaultDirName={pf}\termrec
DefaultGroupName=termrec
AppName=termrec
ChangesAssociations=true
AppVerName=termrec 0.15
[Registry]
Root: HKCR; SubKey: .ttyrec; ValueType: string; ValueData: ttyrec; Flags: uninsdeletekey
Root: HKCR; SubKey: ttyrec; ValueType: string; ValueData: ttyrec replay; Flags: uninsdeletekey
Root: HKCR; SubKey: ttyrec\Shell\Open\Command; ValueType: string; ValueData: """{app}\termplay.exe"" ""%1"""; Flags: uninsdeletevalue
Root: HKCR; Subkey: ttyrec\DefaultIcon; ValueType: string; ValueData: {app}\termplay.exe; Flags: uninsdeletevalue
Root: HKCR; SubKey: .ttyrec.bz2; ValueType: string; ValueData: ttyrec; Flags: uninsdeletekey
Root: HKCR; SubKey: ttyrec; ValueType: string; ValueData: ttyrec replay; Flags: uninsdeletekey
Root: HKCR; SubKey: ttyrec\Shell\Open\Command; ValueType: string; ValueData: """{app}\termplay.exe"" ""%1"""; Flags: uninsdeletevalue
Root: HKCR; Subkey: ttyrec\DefaultIcon; ValueType: string; ValueData: {app}\termplay.exe; Flags: uninsdeletevalue
Root: HKCR; SubKey: .ttyrec.gz; ValueType: string; ValueData: ttyrec; Flags: uninsdeletekey
Root: HKCR; SubKey: ttyrec; ValueType: string; ValueData: ttyrec replay; Flags: uninsdeletekey
Root: HKCR; SubKey: ttyrec\Shell\Open\Command; ValueType: string; ValueData: """{app}\termplay.exe"" ""%1"""; Flags: uninsdeletevalue
Root: HKCR; Subkey: ttyrec\DefaultIcon; ValueType: string; ValueData: {app}\termplay.exe; Flags: uninsdeletevalue
Root: HKCR; SubKey: .dm2; ValueType: string; ValueData: ttyrec; Flags: uninsdeletekey
Root: HKCR; SubKey: ttyrec; ValueType: string; ValueData: ttyrec replay; Flags: uninsdeletekey
Root: HKCR; SubKey: ttyrec\Shell\Open\Command; ValueType: string; ValueData: """{app}\termplay.exe"" ""%1"""; Flags: uninsdeletevalue
Root: HKCR; Subkey: ttyrec\DefaultIcon; ValueType: string; ValueData: {app}\termplay.exe; Flags: uninsdeletevalue
[Icons]
Name: {group}\ttyrec player; Filename: {app}\termplay.exe; WorkingDir: {userdocs}; IconIndex: 0
Name: {group}\ttyrec recorder; Filename: {app}\termrec.exe; WorkingDir: {userdocs}
Name: {group}\Network recorder proxy; Filename: {app}\proxyrec.exe; WorkingDir: {userdocs}
Name: {group}\{cm:UninstallProgram, termrec}; Filename: {uninstallexe}
