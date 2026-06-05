; ============================================================
; vitte-installer.nsi
;
; NSIS Installation Script for Vitte Programming Language
; Platform: Windows x86_64 (64-bit)
; 
; Build with: makensis vitte-installer.nsi
; Installation: vitte-${VERSION}-x86_64-installer.exe
; ============================================================

; Include Modern UI
!include "MUI2.nsh"
!include "x64.nsh"

; ============================================================
; Configuration
; ============================================================

!define VERSION "2.1.1"
!define PRODUCT_NAME "Vitte"
!define PRODUCT_DESCRIPTION "Vitte Programming Language Compiler"
!define COMPANY_NAME "Vitte Authors"
!define COMPANY_WEBSITE "https://vitte-lang.io"
!define INSTALL_DIR "$PROGRAMFILES64\Vitte"

; Installer file
OutFile "pkgout\vitte-${VERSION}-x86_64-installer.exe"

; Default installation folder
InstallDir "${INSTALL_DIR}"

; Request application privileges for Windows Vista+
RequestExecutionLevel admin

; ============================================================
; MUI Settings
; ============================================================

!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_RIGHT
!define MUI_HEADERIMAGE_HEIGHT 150

!define MUI_BGCOLOR "FFFFFF"

!define MUI_ABORTWARNING

; ============================================================
; Pages
; ============================================================

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "LICENSE"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

!define MUI_FINISHPAGE_RUN "$INSTDIR\bin\vitte.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Launch Vitte"
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; ============================================================
; Language
; ============================================================

!insertmacro MUI_LANGUAGE "English"

; ============================================================
; Sections
; ============================================================

; Main installation section
Section "Vitte Compiler (required)" SEC_MAIN
  SectionIn RO
  SetOverwrite on
  
  ; Create directories
  CreateDirectory "$INSTDIR\bin"
  CreateDirectory "$INSTDIR\lib"
  CreateDirectory "$INSTDIR\include"
  CreateDirectory "$INSTDIR\docs"
  
  ; Copy compiler binary
  SetOutPath "$INSTDIR\bin"
  File "bin\vitte-x86_64.exe"
  
  ; Copy runtime libraries
  SetOutPath "$INSTDIR\lib"
  File /r "data\lib\*"
  
  ; Copy standard library
  SetOutPath "$INSTDIR\lib\vitte"
  File /r "data\stdlib\*"
  
  ; Copy documentation
  SetOutPath "$INSTDIR\docs"
  File "README.md"
  File "LICENSE"
  
  ; Create start menu shortcuts
  CreateDirectory "$SMPROGRAMS\Vitte"
  CreateShortCut "$SMPROGRAMS\Vitte\Vitte Compiler.lnk" "$INSTDIR\bin\vitte.exe"
  CreateShortCut "$SMPROGRAMS\Vitte\Uninstall.lnk" "$INSTDIR\uninstall.exe"
  
  ; Add to PATH
  Call AddToPath
  
  ; Create uninstaller
  WriteUninstaller "$INSTDIR\uninstall.exe"
  
  ; Write registry
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vitte" "DisplayName" "${PRODUCT_NAME} ${VERSION} (x86_64)"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vitte" "DisplayVersion" "${VERSION}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vitte" "Publisher" "${COMPANY_NAME}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vitte" "URLInfoAbout" "${COMPANY_WEBSITE}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vitte" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vitte" "UninstallString" "$INSTDIR\uninstall.exe"
  WriteRegDWord HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vitte" "NoModify" 1
  WriteRegDWord HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vitte" "NoRepair" 1
  
SectionEnd

; Editor integrations
Section /o "Editor Support" SEC_EDITORS
  SetOverwrite on
  CreateDirectory "$APPDATA\Vitte"
  File /r "editors\*"
SectionEnd

; Documentation
Section /o "Documentation & Examples" SEC_DOCS
  SetOverwrite on
  CreateDirectory "$INSTDIR\examples"
  File /r "examples\*"
SectionEnd

; Section descriptions
LangString DESC_SEC_MAIN ${LANG_ENGLISH} "Vitte compiler and runtime (required)"
LangString DESC_SEC_EDITORS ${LANG_ENGLISH} "Integration for VS Code, Vim, Notepad++"
LangString DESC_SEC_DOCS ${LANG_ENGLISH} "Documentation and example programs"

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_MAIN} $(DESC_SEC_MAIN)
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_EDITORS} $(DESC_SEC_EDITORS)
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_DOCS} $(DESC_SEC_DOCS)
!insertmacro MUI_FUNCTION_DESCRIPTION_END

; ============================================================
; Uninstall Section
; ============================================================

Section "Uninstall"
  
  Call un.RemoveFromPath
  
  ; Remove files
  RMDir /r "$INSTDIR"
  
  ; Remove shortcuts
  RMDir /r "$SMPROGRAMS\Vitte"
  
  ; Remove registry entries
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Vitte"
  
SectionEnd

; ============================================================
; Functions
; ============================================================

; Add Vitte to PATH
Function AddToPath
  ReadRegStr $0 HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "Path"
  ${If} $0 != ""
    StrCpy $0 "$0;$INSTDIR\bin"
  ${Else}
    StrCpy $0 "$INSTDIR\bin"
  ${EndIf}
  WriteRegStr HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "Path" $0
  SendMessage ${HWND_BROADCAST} ${WM_SETTINGCHANGE} 0 "STR:Environment" /TIMEOUT=5000
FunctionEnd

; Remove Vitte from PATH
Function un.RemoveFromPath
  ReadRegStr $0 HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "Path"
  ${StrReplace} $0 "$INSTDIR\bin;" "" $0
  ${StrReplace} $0 "$INSTDIR\bin" "" $0
  WriteRegStr HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "Path" $0
  SendMessage ${HWND_BROADCAST} ${WM_SETTINGCHANGE} 0 "STR:Environment" /TIMEOUT=5000
FunctionEnd

; ============================================================
; Installer Information
; ============================================================

VIProductVersion "2.1.1.0"
VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductName" "Vitte Programming Language"
VIAddVersionKey /LANG=${LANG_ENGLISH} "CompanyName" "${COMPANY_NAME}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductVersion" "${VERSION}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileVersion" "${VERSION}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileDescription" "${PRODUCT_DESCRIPTION} (x86_64)"
VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalCopyright" "© 2024 ${COMPANY_NAME}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalTrademarks" "Vitte"
