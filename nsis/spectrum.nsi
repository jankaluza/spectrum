  SetCompressor /SOLID lzma

  !define MULTIUSER_EXECUTIONLEVEL Highest
  !define MULTIUSER_MUI
  !define MULTIUSER_INSTALLMODE_COMMANDLINE
  !define MULTIUSER_INSTALLMODE_DEFAULT_REGISTRY_KEY "Software\Spectrum"
  !define MULTIUSER_INSTALLMODE_DEFAULT_REGISTRY_VALUENAME ""
  !define MULTIUSER_INSTALLMODE_INSTDIR_REGISTRY_KEY "Software\Spectrum"
  !define MULTIUSER_INSTALLMODE_INSTDIR_REGISTRY_VALUENAME ""
  !define MULTIUSER_INSTALLMODE_INSTDIR "Spectrum"
  !include "MultiUser.nsh"
  !include "MUI2.nsh"
  !include "nsDialogs.nsh"
  !include "LogicLib.nsh"

;--------------------------------
;General

  ;Name and file
  Name "Spectrum 0.2"
  OutFile "spectrum-0.2-win32.exe"

;--------------------------------
;Variables

  Var StartMenuFolder
  !define UserdataDir Spectrum
  Var UserdataFlags

;Userdata page variables

  Var Dialog
  Var UserdataMyDocumentsRadioButton
  Var UserdataMyDocumentsRadioButtonState
  Var UserdataInstalldirRadioButton
  Var UserdataInstalldirRadioButtonState

;--------------------------------
;Interface Settings

;  !define MUI_WELCOMEFINISHPAGE_BITMAP packaging\windows\WindowsInstallerGraphic.bmp
  !define MUI_ABORTWARNING

;--------------------------------
;Language Selection Dialog Settings

  ;Remember the installer language
  !define MUI_LANGDLL_REGISTRY_ROOT "SHCTX" 
  !define MUI_LANGDLL_REGISTRY_KEY "Software\Spectrum" 
  !define MUI_LANGDLL_REGISTRY_VALUENAME "Installer Language"

;--------------------------------
;Pages

  !insertmacro MUI_PAGE_WELCOME
  !insertmacro MUI_PAGE_LICENSE "../COPYING"
  !insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MULTIUSER_PAGE_INSTALLMODE
  !insertmacro MUI_PAGE_DIRECTORY

  ;Start Menu Folder Page Configuration
  !define MUI_STARTMENUPAGE_REGISTRY_ROOT "SHCTX" 
  !define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\Spectrum" 
  !define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start Menu Folder"
  !define MUI_STARTMENUPAGE_DEFAULTFOLDER "Spectrum"
  
  !insertmacro MUI_PAGE_STARTMENU Application $StartMenuFolder

  !insertmacro MUI_PAGE_INSTFILES

;  !define MUI_FINISHPAGE_RUN $INSTDIR\spectrum.exe
;  !define MUI_FINISHPAGE_RUN_PARAMETERS "$UserdataFlags"
  !insertmacro MUI_PAGE_FINISH
  
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES

;--------------------------------
;Languages

  !insertmacro MUI_LANGUAGE "English" ;first language is the default language

;--------------------------------
;Reserve Files
  
  ;If you are using solid compression, files that are required before
  ;the actual installation should be stored first in the data block,
  ;because this will make your installer start faster.
  
  !insertmacro MUI_RESERVEFILE_LANGDLL
  ReserveFile "${NSISDIR}\Plugins\*.dll"

;--------------------------------
;Installer Sections

Section "Spectrum" SpectrumSection

  SetOutPath "$INSTDIR"
  File /r ..\spectrum-git\*.*
  
  ;Store installation folder
  WriteRegStr SHCTX "Software\Spectrum" "" $INSTDIR
  
  ;Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  
  ;Add uninstall information
  WriteRegStr SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\Spectrum" "DisplayName" "Spectrum 0.2" 
  WriteRegStr SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\Spectrum" "DisplayIcon" "$\"$INSTDIR\spectrum.exe$\""
  WriteRegStr SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\Spectrum" "UninstallString" "$\"$INSTDIR\Uninstall.exe$\""
  WriteRegStr SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\Spectrum" "QuietUninstallString" "$\"$INSTDIR\Uninstall.exe$\" /S"
  WriteRegStr SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\Spectrum" "URLInfoAbout" "www.spectrum.im"
  WriteRegStr SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\Spectrum" "DisplayVersion" "0.2"
  WriteRegDWORD SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\Spectrum" "NoModify" 1
  WriteRegDWORD SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\Spectrum" "NoRepair" 1
  
  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
    
  ${If} $UserdataMyDocumentsRadioButtonState == ${BST_CHECKED}
    StrCpy $UserdataFlags "--config-dir ${UserdataDir}"
  ${Else}
    StrCpy $UserdataFlags ""
  ${EndIf}
    ;Create shortcuts
    CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
    CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Spectrum.lnk" "$INSTDIR\runallinstances.bat"
    CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Edit default config.lnk" "$INSTDIR\editconfig.bat"
    CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Show config folder.lnk" "$INSTDIR\showconfigs.bat"
    CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
  
  !insertmacro MUI_STARTMENU_WRITE_END

SectionEnd

;--------------------------------
;Installer Functions

Function .onInit

  !insertmacro MULTIUSER_INIT
  !insertmacro MUI_LANGDLL_DISPLAY

  StrCpy $UserdataMyDocumentsRadioButtonState ${BST_CHECKED}

FunctionEnd


;--------------------------------
;Descriptions

  ;USE A LANGUAGE STRING IF YOU WANT YOUR DESCRIPTIONS TO BE LANGAUGE SPECIFIC

  ;Assign descriptions to sections
  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SpectrumSection} "Spectrum executable and data."
  !insertmacro MUI_FUNCTION_DESCRIPTION_END

 
;--------------------------------
;Uninstaller Section

Section "Uninstall"

  Delete "$INSTDIR\asprintf.dll"
  Delete "$INSTDIR\charset.dll"
  Delete "$INSTDIR\cyggloox-8.dll"
  Delete "$INSTDIR\editconfig.bat"
  Delete "$INSTDIR\freebl3.dll"
  Delete "$INSTDIR\iconv.dll"
  Delete "$INSTDIR\intl.dll"
  Delete "$INSTDIR\libatk-1.0-0.dll"
  Delete "$INSTDIR\libexpat.dll"
  Delete "$INSTDIR\libgio-2.0-0.dll"
  Delete "$INSTDIR\libglib-2.0-0.dll"
  Delete "$INSTDIR\libgmodule-2.0-0.dll"
  Delete "$INSTDIR\libgobject-2.0-0.dll"
  Delete "$INSTDIR\libgthread-2.0-0.dll"
  Delete "$INSTDIR\libjabber.dll"
  Delete "$INSTDIR\libmeanwhile-1.dll"
  Delete "$INSTDIR\liboscar.dll"
  Delete "$INSTDIR\libpurple.dll"
  Delete "$INSTDIR\libsasl.dll"
  Delete "$INSTDIR\libsilc-1-1-2.dll"
  Delete "$INSTDIR\libsilcclient-1-1-2.dll"
  Delete "$INSTDIR\libxml2.dll"
  Delete "$INSTDIR\libymsg.dll"
  Delete "$INSTDIR\nspr4.dll"
  Delete "$INSTDIR\nss3.dll"
  Delete "$INSTDIR\nssckbi.dll"
  Delete "$INSTDIR\plc4.dll"
  Delete "$INSTDIR\plds4.dll"
  Delete "$INSTDIR\runallinstances.bat"
  Delete "$INSTDIR\showconfigs.bat"
  Delete "$INSTDIR\smime3.dll"
  Delete "$INSTDIR\softokn3.dll"
  Delete "$INSTDIR\spectrum.bat"
  Delete "$INSTDIR\spectrum.cfg"
  Delete "$INSTDIR\spectrum.exe"
  Delete "$INSTDIR\ssl3.dll"
  Delete "$INSTDIR\xmlparse.dll"
  Delete "$INSTDIR\xmltok.dll"

  Delete "$INSTDIR\plugins\autoaccept.dll"
  Delete "$INSTDIR\plugins\buddynote.dll"
  Delete "$INSTDIR\plugins\idle.dll"
  Delete "$INSTDIR\plugins\joinpart.dll"
  Delete "$INSTDIR\plugins\libaim.dll"
  Delete "$INSTDIR\plugins\libbonjour.dll"
  Delete "$INSTDIR\plugins\libgg.dll"
  Delete "$INSTDIR\plugins\libicq.dll"
  Delete "$INSTDIR\plugins\libirc.dll"
  Delete "$INSTDIR\plugins\libmsn.dll"
  Delete "$INSTDIR\plugins\libmxit.dll"
  Delete "$INSTDIR\plugins\libmyspace.dll"
  Delete "$INSTDIR\plugins\libnovell.dll"
  Delete "$INSTDIR\plugins\libqq.dll"
  Delete "$INSTDIR\plugins\libsametime.dll"
  Delete "$INSTDIR\plugins\libsilc.dll"
  Delete "$INSTDIR\plugins\libsimple.dll"
  Delete "$INSTDIR\plugins\libxmpp.dll"
  Delete "$INSTDIR\plugins\libyahoo.dll"
  Delete "$INSTDIR\plugins\libyahoojp.dll"
  Delete "$INSTDIR\plugins\log_reader.dll"
  Delete "$INSTDIR\plugins\newline.dll"
  Delete "$INSTDIR\plugins\offlinemsg.dll"
  Delete "$INSTDIR\plugins\perl.dll"
  Delete "$INSTDIR\plugins\psychic.dll"
  Delete "$INSTDIR\plugins\ssl-nss.dll"
  Delete "$INSTDIR\plugins\ssl.dll"
  Delete "$INSTDIR\plugins\statenotify.dll"
  Delete "$INSTDIR\plugins\tcl.dll"

  RMDir "$INSTDIR\plugins"

  Delete "$INSTDIR\Uninstall.exe"

  RMDir /r "$INSTDIR"

  !insertmacro MUI_STARTMENU_GETFOLDER Application $StartMenuFolder

  Delete "$SMPROGRAMS\$StartMenuFolder\Spectrum.lnk"
  Delete "$SMPROGRAMS\$StartMenuFolder\Edit default config.lnk"
  Delete "$SMPROGRAMS\$StartMenuFolder\Show config folder.lnk"
  Delete "$SMPROGRAMS\$StartMenuFolder\Uninstall.lnk"
  RMDir "$SMPROGRAMS\$StartMenuFolder"

  DeleteRegKey SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\Spectrum"
  DeleteRegKey /ifempty SHCTX "Spectrum"

SectionEnd

;--------------------------------
;Uninstaller Functions

Function un.onInit

  !insertmacro MULTIUSER_UNINIT
  !insertmacro MUI_UNGETLANGUAGE
  
FunctionEnd
