\
    ; NSIS stub (à compléter)
    Name "Vitte Studio"
    OutFile "VitteStudioSetup.exe"
    InstallDir "$PROGRAMFILES\Vitte Studio"
    Section
      SetOutPath "$INSTDIR"
      File /r "dist\*.*"
      CreateShortCut "$DESKTOP\Vitte Studio.lnk" "$INSTDIR\desktop-qt.exe"
    SectionEnd
