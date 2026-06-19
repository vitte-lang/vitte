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
!define MUI_LANGDLL_ALLLANGUAGES

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
!insertmacro MUI_LANGUAGE "French"
!insertmacro MUI_LANGUAGE "Spanish"
!insertmacro MUI_LANGUAGE "German"
!insertmacro MUI_LANGUAGE "Italian"
!insertmacro MUI_LANGUAGE "PortugueseBR"
!insertmacro MUI_LANGUAGE "Dutch"
!insertmacro MUI_LANGUAGE "Polish"
!insertmacro MUI_LANGUAGE "Russian"
!insertmacro MUI_LANGUAGE "Ukrainian"
!insertmacro MUI_LANGUAGE "SimpChinese"
!insertmacro MUI_LANGUAGE "Japanese"
!insertmacro MUI_LANGUAGE "Korean"
!insertmacro MUI_LANGUAGE "Turkish"
!insertmacro MUI_LANGUAGE "Arabic"

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
  CreateDirectory "$INSTDIR\share\vitte\src"
  CreateDirectory "$INSTDIR\share\locales"
  
  ; Copy compiler binary
  SetOutPath "$INSTDIR\bin"
  File "bin\vitte-x86_64.exe"
  
  ; Copy runtime libraries
  SetOutPath "$INSTDIR\lib"
  File /r "data\lib\*"
  
  ; Copy source packages, standard library and compiler sources
  SetOutPath "$INSTDIR\share\vitte\src"
  File /r "data\src\*"
  
  ; Copy documentation
  SetOutPath "$INSTDIR\docs"
  File "README.md"
  File "LICENSE"

  ; Copy Fluent diagnostic catalogs
  SetOutPath "$INSTDIR\share\locales"
  File /r "locales\*"
  
  ; Create start menu shortcuts
  CreateDirectory "$SMPROGRAMS\Vitte"
  CreateShortCut "$SMPROGRAMS\Vitte\Vitte Compiler.lnk" "$INSTDIR\bin\vitte.exe"
  CreateShortCut "$SMPROGRAMS\Vitte\Uninstall.lnk" "$INSTDIR\uninstall.exe"
  
  ; Add to PATH
  Call AddToPath

  ; Persist the selected installer language for CLI diagnostics.
  Call WriteLanguageConfig
  
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
LangString DESC_SEC_MAIN ${LANG_FRENCH} "Compilateur et runtime Vitte (requis)"
LangString DESC_SEC_EDITORS ${LANG_FRENCH} "Integration pour VS Code, Vim, Notepad++"
LangString DESC_SEC_DOCS ${LANG_FRENCH} "Documentation et exemples"
LangString DESC_SEC_MAIN ${LANG_SPANISH} "Compilador y runtime de Vitte (obligatorio)"
LangString DESC_SEC_EDITORS ${LANG_SPANISH} "Integracion para VS Code, Vim, Notepad++"
LangString DESC_SEC_DOCS ${LANG_SPANISH} "Documentacion y ejemplos"
LangString DESC_SEC_MAIN ${LANG_GERMAN} "Vitte-Compiler und Runtime (erforderlich)"
LangString DESC_SEC_EDITORS ${LANG_GERMAN} "Integration fuer VS Code, Vim, Notepad++"
LangString DESC_SEC_DOCS ${LANG_GERMAN} "Dokumentation und Beispiele"
LangString DESC_SEC_MAIN ${LANG_ITALIAN} "Compilatore e runtime Vitte (richiesto)"
LangString DESC_SEC_EDITORS ${LANG_ITALIAN} "Integrazione per VS Code, Vim, Notepad++"
LangString DESC_SEC_DOCS ${LANG_ITALIAN} "Documentazione ed esempi"
LangString DESC_SEC_MAIN ${LANG_PORTUGUESEBR} "Compilador e runtime Vitte (obrigatorio)"
LangString DESC_SEC_EDITORS ${LANG_PORTUGUESEBR} "Integracao para VS Code, Vim, Notepad++"
LangString DESC_SEC_DOCS ${LANG_PORTUGUESEBR} "Documentacao e exemplos"
LangString DESC_SEC_MAIN ${LANG_DUTCH} "Vitte compiler en runtime (vereist)"
LangString DESC_SEC_EDITORS ${LANG_DUTCH} "Integratie voor VS Code, Vim, Notepad++"
LangString DESC_SEC_DOCS ${LANG_DUTCH} "Documentatie en voorbeelden"
LangString DESC_SEC_MAIN ${LANG_POLISH} "Kompilator i runtime Vitte (wymagane)"
LangString DESC_SEC_EDITORS ${LANG_POLISH} "Integracja z VS Code, Vim, Notepad++"
LangString DESC_SEC_DOCS ${LANG_POLISH} "Dokumentacja i przyklady"
LangString DESC_SEC_MAIN ${LANG_RUSSIAN} "Kompilyator i sreda vypolneniya Vitte (obyazatelno)"
LangString DESC_SEC_EDITORS ${LANG_RUSSIAN} "Integratsiya s VS Code, Vim, Notepad++"
LangString DESC_SEC_DOCS ${LANG_RUSSIAN} "Dokumentatsiya i primery"
LangString DESC_SEC_MAIN ${LANG_UKRAINIAN} "Kompiliator i seredovyshche vykonannia Vitte (oboviazkovo)"
LangString DESC_SEC_EDITORS ${LANG_UKRAINIAN} "Intehratsiia z VS Code, Vim, Notepad++"
LangString DESC_SEC_DOCS ${LANG_UKRAINIAN} "Dokumentatsiia ta pryklady"
LangString DESC_SEC_MAIN ${LANG_SIMPCHINESE} "Vitte compiler and runtime (required)"
LangString DESC_SEC_EDITORS ${LANG_SIMPCHINESE} "Integration for VS Code, Vim, Notepad++"
LangString DESC_SEC_DOCS ${LANG_SIMPCHINESE} "Documentation and examples"
LangString DESC_SEC_MAIN ${LANG_JAPANESE} "Vitte compiler and runtime (required)"
LangString DESC_SEC_EDITORS ${LANG_JAPANESE} "Integration for VS Code, Vim, Notepad++"
LangString DESC_SEC_DOCS ${LANG_JAPANESE} "Documentation and examples"
LangString DESC_SEC_MAIN ${LANG_KOREAN} "Vitte compiler and runtime (required)"
LangString DESC_SEC_EDITORS ${LANG_KOREAN} "Integration for VS Code, Vim, Notepad++"
LangString DESC_SEC_DOCS ${LANG_KOREAN} "Documentation and examples"
LangString DESC_SEC_MAIN ${LANG_TURKISH} "Vitte derleyici ve runtime (gerekli)"
LangString DESC_SEC_EDITORS ${LANG_TURKISH} "VS Code, Vim, Notepad++ entegrasyonu"
LangString DESC_SEC_DOCS ${LANG_TURKISH} "Belgeler ve ornekler"
LangString DESC_SEC_MAIN ${LANG_ARABIC} "Vitte compiler and runtime (required)"
LangString DESC_SEC_EDITORS ${LANG_ARABIC} "Integration for VS Code, Vim, Notepad++"
LangString DESC_SEC_DOCS ${LANG_ARABIC} "Documentation and examples"

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

Function .onInit
  !insertmacro MUI_LANGDLL_DISPLAY
FunctionEnd

Function SelectedLocale
  StrCpy $0 "en"
  ${If} $LANGUAGE == ${LANG_FRENCH}
    StrCpy $0 "fr"
  ${ElseIf} $LANGUAGE == ${LANG_SPANISH}
    StrCpy $0 "es"
  ${ElseIf} $LANGUAGE == ${LANG_GERMAN}
    StrCpy $0 "de"
  ${ElseIf} $LANGUAGE == ${LANG_ITALIAN}
    StrCpy $0 "it"
  ${ElseIf} $LANGUAGE == ${LANG_PORTUGUESEBR}
    StrCpy $0 "pt-BR"
  ${ElseIf} $LANGUAGE == ${LANG_DUTCH}
    StrCpy $0 "nl"
  ${ElseIf} $LANGUAGE == ${LANG_POLISH}
    StrCpy $0 "pl"
  ${ElseIf} $LANGUAGE == ${LANG_RUSSIAN}
    StrCpy $0 "ru"
  ${ElseIf} $LANGUAGE == ${LANG_UKRAINIAN}
    StrCpy $0 "uk"
  ${ElseIf} $LANGUAGE == ${LANG_SIMPCHINESE}
    StrCpy $0 "zh-CN"
  ${ElseIf} $LANGUAGE == ${LANG_JAPANESE}
    StrCpy $0 "ja"
  ${ElseIf} $LANGUAGE == ${LANG_KOREAN}
    StrCpy $0 "ko"
  ${ElseIf} $LANGUAGE == ${LANG_TURKISH}
    StrCpy $0 "tr"
  ${ElseIf} $LANGUAGE == ${LANG_ARABIC}
    StrCpy $0 "ar"
  ${EndIf}
FunctionEnd

Function WriteLanguageConfig
  Call SelectedLocale
  CreateDirectory "$APPDATA\Vitte"
  FileOpen $1 "$APPDATA\Vitte\config" w
  FileWrite $1 "VITTE_LANG=$0$\r$\n"
  FileWrite $1 "VITTE_FLUENT=$INSTDIR\share\locales\$0\diagnostics.ftl$\r$\n"
  FileClose $1
  WriteRegStr HKCU "Software\Vitte" "Language" "$0"
  WriteRegStr HKCU "Software\Vitte" "FluentCatalog" "$INSTDIR\share\locales\$0\diagnostics.ftl"
FunctionEnd

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
