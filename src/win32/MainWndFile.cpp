// VisualBoyAdvance - Nintendo Gameboy/GameboyAdvance (TM) emulator.
// Copyright (C) 1999-2003 Forgotten
// Copyright (C) 2004 Forgotten and the VBA development team

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2, or(at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

#include "stdafx.h"
#include "MainWnd.h"

#include "ExportGSASnapshot.h"
#include "FileDlg.h"
#include "GSACodeSelect.h"
#include "RomInfo.h"
#include "Reg.h"
#include "WinResUtil.h"

#include "../GBA.h"
#include "../Globals.h"
#include "../NLS.h"
#include "../Sound.h"
#include "../gb/GB.h"
#include "../gb/gbCheats.h"
#include "../gb/gbGlobals.h"
/* Link 
---------------------*/
#include "../Link.h"
/* ---------------- */

/* Link 
---------------------*/
extern int linkid;
/* ------------------- */

extern int emulating;

extern void remoteCleanUp();

void MainWnd::OnFileOpen() 
{
  theApp.winCheckFullscreen();
  if(fileOpenSelect()) {
    FileRun();
  }
}

void MainWnd::OnFilePause() 
{
  theApp.paused = !theApp.paused;
  if(emulating) {
    if(theApp.paused) {
      theApp.wasPaused = true;
      soundPause();
    } else {
      soundResume();
    }
  }
}

void MainWnd::OnUpdateFilePause(CCmdUI* pCmdUI) 
{
  pCmdUI->SetCheck(theApp.paused);  
}

void MainWnd::OnFileReset() 
{
  if(emulating) {
    theApp.emulator.emuReset();
    systemScreenMessage(winResLoadString(IDS_RESET));
  }
}

void MainWnd::OnUpdateFileReset(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable(emulating);
}

void MainWnd::OnUpdateFileRecentFreeze(CCmdUI* pCmdUI) 
{
  pCmdUI->SetCheck(theApp.recentFreeze);

  if(pCmdUI->m_pMenu == NULL)
    return;
  
  CMenu *pMenu = pCmdUI->m_pMenu;

  int i;
  for(i = 0; i < 10; i++) {
    if(!pMenu->RemoveMenu(ID_FILE_MRU_FILE1+i, MF_BYCOMMAND))
      break;
  }

  for(i = 0; i < 10; i++) {
    CString p = theApp.recentFiles[i];
    if(p.GetLength() == 0)
      break;
    int index = p.ReverseFind('\\');

    if(index != -1)
      p = p.Right(p.GetLength()-index-1);

    pMenu->AppendMenu(MF_STRING, ID_FILE_MRU_FILE1+i, p);
  }
  theApp.winAccelMgr.UpdateMenu((HMENU)*pMenu);
}

BOOL MainWnd::OnFileRecentFile(UINT nID)
{
  if(theApp.recentFiles[(nID&0xFFFF)-ID_FILE_MRU_FILE1].GetLength()) {
    theApp.szFile = theApp.recentFiles[(nID&0xFFFF)-ID_FILE_MRU_FILE1];
    if(FileRun())
      emulating = true;
    else {
      emulating = false;
      soundPause();
    }
  }
  return TRUE;
}

void MainWnd::OnFileRecentReset() 
{
  int i = 0;
  for(i = 0; i < 10; i++)
    theApp.recentFiles[i] = "";
}

void MainWnd::OnFileRecentFreeze() 
{
  theApp.recentFreeze = !theApp.recentFreeze;
}

void MainWnd::OnFileExit() 
{
  SendMessage(WM_CLOSE);
}

void MainWnd::OnFileClose() 
{
  // save battery file before we change the filename...
  if(rom != NULL || gbRom != NULL) {
    if(theApp.autoSaveLoadCheatList)
      winSaveCheatListDefault();
    writeBatteryFile();
    soundPause();
    theApp.emulator.emuCleanUp();
    remoteCleanUp();
  }
  emulating = 0;
  RedrawWindow(NULL,NULL,RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN);
  systemSetTitle("VisualBoyAdvance");
}

void MainWnd::OnUpdateFileClose(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable(emulating);
}

void MainWnd::OnFileOpengameboy() 
{
  theApp.winCheckFullscreen();
  theApp.dir = "";
  CString initialDir = regQueryStringValue("gbromdir",".");
  if(!initialDir.IsEmpty())
    theApp.dir = initialDir;

  theApp.szFile = "";

  LPCTSTR exts[] = { "" };
  CString filter = winLoadFilter(IDS_FILTER_GBROM);
  CString title = winResLoadString(IDS_SELECT_ROM);

  FileDlg dlg(this, "", filter, 0, "", exts, initialDir, title, false);

  if(dlg.DoModal() == IDOK) {
    theApp.szFile = dlg.GetPathName();
    theApp.dir = theApp.szFile.Left(dlg.m_ofn.nFileOffset);
    if(theApp.dir.GetLength() > 3 && theApp.dir[theApp.dir.GetLength()-1] == '\\')
      theApp.dir = theApp.dir.Left(theApp.dir.GetLength()-1);
    regSetStringValue("gbromdir", theApp.dir);
    FileRun();
  }
}

void MainWnd::OnFileLoad() 
{
  theApp.winCheckFullscreen();
  CString buffer;
  CString filename;

  int index = theApp.filename.ReverseFind('\\');

  if(index != -1)
    buffer = theApp.filename.Right(theApp.filename.GetLength()-index-1);
  else
    buffer = theApp.filename;

  CString saveDir = regQueryStringValue("saveDir", NULL);

  if(saveDir.IsEmpty())
    saveDir = getDirFromFile(theApp.filename);

  if(isDriveRoot(saveDir))
    filename.Format("%s%s.sg%1d", saveDir, buffer,linkid+1);
  else
    filename.Format("%s\\%s.sg%1d", saveDir, buffer,linkid+1);

  LPCTSTR exts[] = { ".sgm" };
  CString filter = winLoadFilter(IDS_FILTER_SGM);
  CString title = winResLoadString(IDS_SELECT_SAVE_GAME_NAME);

  FileDlg dlg(this, filename, filter, 0, "", exts, saveDir, title, false);

  if(dlg.DoModal() == IDOK) {
    bool res = loadSaveGame(dlg.GetPathName());

    theApp.rewindCount = 0;
    theApp.rewindCounter = 0;
    theApp.rewindSaveNeeded = false;
  
    if(res)
      systemScreenMessage(winResLoadString(IDS_LOADED_STATE));  
  }
}

void MainWnd::OnUpdateFileLoad(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable(emulating);
}

BOOL MainWnd::OnFileLoadSlot(UINT nID)
{
  nID = nID + 1 - ID_FILE_LOADGAME_SLOT1;

  CString buffer;
  CString filename;

  int index = theApp.filename.ReverseFind('\\');

  if(index != -1)
    buffer = theApp.filename.Right(theApp.filename.GetLength()-index-1);
  else
    buffer = theApp.filename;

  CString saveDir = regQueryStringValue("saveDir", NULL);

  if(saveDir.IsEmpty())
    saveDir = getDirFromFile(theApp.filename);

  if(isDriveRoot(saveDir))
    filename.Format("%s%s%d.sg%1d", saveDir, buffer, nID, linkid+1);
  else
    filename.Format("%s\\%s%d.sg%1d", saveDir, buffer, nID, linkid+1);

  bool res = loadSaveGame(filename);

  theApp.rewindCount = 0;
  theApp.rewindCounter = 0;
  theApp.rewindSaveNeeded = false;
  
  CString format = winResLoadString(IDS_LOADED_STATE_N);
  buffer.Format(format, nID);
  
  systemScreenMessage(buffer);
  
  return res;
}

void MainWnd::OnFileSave() 
{
  theApp.winCheckFullscreen();
  CString buffer;
  CString filename;

  int index = theApp.filename.ReverseFind('\\');

  if(index != -1)
    buffer = theApp.filename.Right(theApp.filename.GetLength()-index-1);
  else
    buffer = theApp.filename;

  CString saveDir = regQueryStringValue("saveDir", NULL);

  if(saveDir.IsEmpty())
    saveDir = getDirFromFile(theApp.filename);

  if(isDriveRoot(saveDir))
    filename.Format("%s%s.sg%1d", saveDir, buffer, linkid+1);
  else
    filename.Format("%s\\%s.sg%1d", saveDir, buffer, linkid+1);

  LPCTSTR exts[] = { ".sgm" };
  CString filter = winLoadFilter(IDS_FILTER_SGM);
  CString title = winResLoadString(IDS_SELECT_SAVE_GAME_NAME);

  FileDlg dlg(this, filename, filter, 0, "", exts, saveDir, title, true);

  if(dlg.DoModal() == IDOK) {
    bool res = writeSaveGame(dlg.GetPathName());
    if(res)
      systemScreenMessage(winResLoadString(IDS_WROTE_STATE));  
  }
}

void MainWnd::OnUpdateFileSave(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable(emulating);
}

BOOL MainWnd::OnFileSaveSlot(UINT nID)
{
  nID = nID + 1 - ID_FILE_SAVEGAME_SLOT1;

  CString buffer;
  CString filename;

  int index = theApp.filename.ReverseFind('\\');

  if(index != -1)
    buffer = theApp.filename.Right(theApp.filename.GetLength()-index-1);
  else
    buffer = theApp.filename;

  CString saveDir = regQueryStringValue("saveDir", NULL);

  if(saveDir.IsEmpty())
    saveDir = getDirFromFile(theApp.filename);

  if(isDriveRoot(saveDir))
    filename.Format("%s%s%d.sg%1d", saveDir, buffer, nID,linkid+1);
  else
    filename.Format("%s\\%s%d.sg%1d", saveDir, buffer, nID,linkid+1);

  bool res = writeSaveGame(filename);

  CString format = winResLoadString(IDS_WROTE_STATE_N);
  buffer.Format(format, nID);
  
  systemScreenMessage(buffer);
  
  return res;
}

void MainWnd::OnFileImportBatteryfile() 
{
  theApp.winCheckFullscreen();
  LPCTSTR exts[] = { ".sav", ".sa1", ".sa2", ".sa3", ".sa4" };
  CString filter = winLoadFilter(IDS_FILTER_SAV);
  CString title = winResLoadString(IDS_SELECT_BATTERY_FILE);

  CString saveDir = regQueryStringValue("batteryDir", NULL);

  if(saveDir.IsEmpty())
    saveDir = getDirFromFile(theApp.filename);

  FileDlg dlg(this, "", filter, 0, "", exts, saveDir, title, false);
  
  if(dlg.DoModal() == IDCANCEL)
    return;
  
  CString str1 = winResLoadString(IDS_SAVE_WILL_BE_LOST);
  CString str2 = winResLoadString(IDS_CONFIRM_ACTION);

  if(MessageBox(str1,
                str2,
                MB_OKCANCEL) == IDCANCEL)
    return;
  
  bool res = false;

  res = theApp.emulator.emuReadBattery(dlg.GetPathName());
  
  if(!res)
    systemMessage(MSG_CANNOT_OPEN_FILE, "Cannot open file %s", dlg.GetPathName());
  else {
    theApp.emulator.emuReset();
  }
}

void MainWnd::OnUpdateFileImportBatteryfile(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable(emulating);  
}

void MainWnd::OnFileImportGamesharkcodefile() 
{
  theApp.winCheckFullscreen();
  LPCTSTR exts[] = { "" };
  CString filter = theApp.cartridgeType == 0 ? winLoadFilter(IDS_FILTER_SPC) : winLoadFilter(IDS_FILTER_GCF);
  CString title = winResLoadString(IDS_SELECT_CODE_FILE);

  FileDlg dlg(this, "", filter, 0, "", exts, "", title, false);
  
  if(dlg.DoModal() == IDCANCEL)
    return;

  CString str1 = winResLoadString(IDS_CODES_WILL_BE_LOST);
  CString str2 = winResLoadString(IDS_CONFIRM_ACTION);

  if(MessageBox(str1,
                str2,
                MB_OKCANCEL) == IDCANCEL)
    return;
  
  bool res = false;
  CString file = dlg.GetPathName();
  if(theApp.cartridgeType == 1)
    res = gbCheatReadGSCodeFile(file);
  else {
    res = fileImportGSACodeFile(file);
  }
}

void MainWnd::OnUpdateFileImportGamesharkcodefile(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable(emulating);  
}

void MainWnd::OnFileImportGamesharksnapshot() 
{
  theApp.winCheckFullscreen();
  LPCTSTR exts[] = { "" };
  CString filter = theApp.cartridgeType == 1 ? winLoadFilter(IDS_FILTER_GBS) : winLoadFilter(IDS_FILTER_SPS);
  CString title = winResLoadString(IDS_SELECT_SNAPSHOT_FILE);

  FileDlg dlg(this, "", filter, 0, "", exts, "", title, false);
  
  if(dlg.DoModal() == IDCANCEL)
    return;

  CString str1 = winResLoadString(IDS_SAVE_WILL_BE_LOST);
  CString str2 = winResLoadString(IDS_CONFIRM_ACTION);

  if(MessageBox(str1,
                str2,
                MB_OKCANCEL) == IDCANCEL)
    return;

  if(theApp.cartridgeType == 1)
    gbReadGSASnapshot(dlg.GetPathName());
  else
    CPUReadGSASnapshot(dlg.GetPathName());
}

void MainWnd::OnUpdateFileImportGamesharksnapshot(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable(emulating);  
}

void MainWnd::OnFileExportBatteryfile() 
{
  theApp.winCheckFullscreen();
  CString name;

  int index = theApp.filename.ReverseFind('\\');

  if(index != -1)
    name = theApp.filename.Right(theApp.filename.GetLength()-index-1);
  else
    name = theApp.filename;

  LPCTSTR exts[] = {".sav", ".sa1", ".sa2", ".sa3", ".sa4", ".dat" };

  CString filter = winLoadFilter(IDS_FILTER_SAV);
  CString title = winResLoadString(IDS_SELECT_BATTERY_FILE);

  CString saveDir = regQueryStringValue("batteryDir", NULL);

  if(saveDir.IsEmpty())
    saveDir = getDirFromFile(theApp.filename);

  FileDlg dlg(this,
              name,
              filter,
              1,
              "SAV",
              exts,
              saveDir,
              title,
              true);

  if(dlg.DoModal() == IDCANCEL) {
    return;
  }

  bool result = false;

  if(theApp.cartridgeType == 1) {
    result = gbWriteBatteryFile(dlg.GetPathName(), false);
  } else
    result = theApp.emulator.emuWriteBattery(dlg.GetPathName());

  if(!result)
    systemMessage(MSG_ERROR_CREATING_FILE, "Error creating file %s",
                  dlg.GetPathName());
}

void MainWnd::OnUpdateFileExportBatteryfile(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable(emulating);  
}

void MainWnd::OnFileExportGamesharksnapshot() 
{
  theApp.winCheckFullscreen();
  if(eepromInUse) {
    systemMessage(IDS_EEPROM_NOT_SUPPORTED, "EEPROM saves cannot be exported");
    return;
  }
  
  CString name;

  int index = theApp.filename.ReverseFind('\\');

  if(index != -1)
    name = theApp.filename.Right(theApp.filename.GetLength()-index-1);
  else
    name = theApp.filename;

  LPCTSTR exts[] = {".sps" };
  
  CString filter = winLoadFilter(IDS_FILTER_SPS);
  CString title = winResLoadString(IDS_SELECT_SNAPSHOT_FILE);

  FileDlg dlg(this,
              name,
              filter,
              1,
              "SPS",
              exts,
              "",
              title,
              true);

  if(dlg.DoModal() == IDCANCEL)
    return;

  char buffer[16];
  strncpy(buffer, (const char *)&rom[0xa0], 12);
  buffer[12] = 0;

  ExportGSASnapshot dlg2(dlg.GetPathName(), buffer, this);
  dlg2.DoModal();
}

void MainWnd::OnUpdateFileExportGamesharksnapshot(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable(emulating && theApp.cartridgeType == 0);  
}

void MainWnd::OnFileScreencapture() 
{
  theApp.winCheckFullscreen();
  CString name;
  CString filename;

  int index = theApp.filename.ReverseFind('\\');

  if(index != -1)
    name = theApp.filename.Right(theApp.filename.GetLength()-index-1);
  else
    name = theApp.filename;

  CString capdir = regQueryStringValue("captureDir", "");
  if(capdir.IsEmpty())
    capdir = getDirFromFile(name);

  CString ext = "png";

  if(theApp.captureFormat != 0)
    ext = "bmp";
  
  if(isDriveRoot(capdir))
    filename.Format("%s%s.%s", capdir, name, ext);
  else
    filename.Format("%s\\%s.%s", capdir, name, ext);

  LPCTSTR exts[] = {".png", ".bmp" };
    
  CString filter = winLoadFilter(IDS_FILTER_PNG);
  CString title = winResLoadString(IDS_SELECT_CAPTURE_NAME);

  FileDlg dlg(this,
              filename,
              filter,
              theApp.captureFormat ? 2 : 1,
              theApp.captureFormat ? "BMP" : "PNG",
              exts,
              capdir,
              title,
              true);

  if(dlg.DoModal() == IDCANCEL)
    return;
  
  if(dlg.getFilterIndex() == 2)
    theApp.emulator.emuWriteBMP(dlg.GetPathName());
  else
    theApp.emulator.emuWritePNG(dlg.GetPathName());

  systemScreenMessage(winResLoadString(IDS_SCREEN_CAPTURE));
}

void MainWnd::OnUpdateFileScreencapture(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable(emulating);  
}

void MainWnd::OnFileRominformation() 
{
  theApp.winCheckFullscreen();
  if(theApp.cartridgeType == 0) {
    RomInfoGBA dlg(rom);
    dlg.DoModal();
  } else {
    RomInfoGB dlg(gbRom);
    dlg.DoModal();
  }
}

void MainWnd::OnUpdateFileRominformation(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable(emulating);  
}

void MainWnd::OnFileTogglemenu() 
{
  if(theApp.videoOption <= VIDEO_4X)
    return;
  
  theApp.menuToggle = !theApp.menuToggle;

  if(theApp.menuToggle) {
    theApp.updateMenuBar();
    if(theApp.tripleBuffering) {
      if(theApp.display)
        theApp.display->checkFullScreen();
      DrawMenuBar();
    }    
  } else {
    SetMenu(NULL);
    DestroyMenu(theApp.menu);
  }

  theApp.adjustDestRect();
}

void MainWnd::OnUpdateFileTogglemenu(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable(theApp.videoOption > VIDEO_4X);  
}

bool MainWnd::fileImportGSACodeFile(CString& fileName)
{
  FILE *f = fopen(fileName, "rb");
  
  if(f == NULL) {
    systemMessage(MSG_CANNOT_OPEN_FILE, "Cannot open file %s", fileName);
    return false;
  }

  u32 len;
  fread(&len, 1, 4, f);
  if(len != 14) {
    fclose(f);
    systemMessage(MSG_UNSUPPORTED_CODE_FILE, "Unsupported code file %s",
                  fileName);
    return false;
  }
  char buffer[16];
  fread(buffer, 1, 14, f);
  buffer[14] = 0;
  if(memcmp(buffer, "SharkPortCODES", 14)) {
    fclose(f);
    systemMessage(MSG_UNSUPPORTED_CODE_FILE, "Unsupported code file %s",
                  fileName);
    return false;
  }    
  fseek(f, 0x1e, SEEK_SET);
  fread(&len, 1, 4, f);
  int game = 0;
  if(len > 1) {
    GSACodeSelect dlg(f);
    game = dlg.DoModal();
  }
  fclose(f);

  bool v3 = false;
  
  int index = fileName.ReverseFind('.');

  if(index != -1) {
    if(fileName.Right(3).CompareNoCase("XPC") == 0)
      v3 = true;
  }

  if(game != -1) {
    return cheatsImportGSACodeFile(fileName, game, v3);
  }
  
  return true;
}

void MainWnd::OnFileSavegameOldestslot() 
{
  if(!emulating)
    return;

  CString filename;

  int index = theApp.filename.ReverseFind('\\');

  if(index != -1)
    filename = theApp.filename.Right(theApp.filename.GetLength()-index-1);
  else
    filename = theApp.filename;

  CString saveDir = regQueryStringValue("saveDir", NULL);

  if(saveDir.IsEmpty())
    saveDir = getDirFromFile(theApp.filename);

  if(!isDriveRoot(saveDir))
    saveDir += "\\";

  CString name;
  CFileStatus status;
  CString str;
  time_t time = -1;
  int found = 0;
    
  for(int i = 0; i < 10; i++) {
    name.Format("%s%s%d.sg%1d", saveDir, filename, i+1,linkid+1);

    if(emulating && CFile::GetStatus(name, status)) {
      if(status.m_mtime.GetTime() < time) {
        time = status.m_mtime.GetTime();
        found = i;
      }
    } else {
      found = i;
      break;
    }
  }
  OnFileSaveSlot(ID_FILE_SAVEGAME_SLOT1+found);
}

void MainWnd::OnUpdateFileSavegameOldestslot(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable(emulating);
  if(pCmdUI->m_pSubMenu != NULL) {
    CMenu *pMenu = pCmdUI->m_pSubMenu;
    CString filename;

    int index = theApp.filename.ReverseFind('\\');

    if(index != -1)
      filename = theApp.filename.Right(theApp.filename.GetLength()-index-1);
    else
      filename = theApp.filename;

    CString saveDir = regQueryStringValue("saveDir", NULL);

    if(saveDir.IsEmpty())
      saveDir = getDirFromFile(theApp.filename);

    if(!isDriveRoot(saveDir))
      saveDir += "\\";

    CString name;
    CFileStatus status;
    CString str;

    for(int i = 0; i < 10; i++) {
      name.Format("%s%s%d.sg%1d", saveDir, filename, i+1,linkid+1);

      if(emulating && CFile::GetStatus(name, status)) {
        CString timestamp = status.m_mtime.Format("%Y/%m/%d %H:%M:%S");
        str.Format("%d %s", i+1, timestamp);
      } else {
        str.Format("%d ----/--/-- --:--:--", i+1);
      }
      pMenu->ModifyMenu(ID_FILE_SAVEGAME_SLOT1+i, MF_STRING|MF_BYCOMMAND, ID_FILE_SAVEGAME_SLOT1+i, str);
    }
    
    theApp.winAccelMgr.UpdateMenu(pMenu->GetSafeHmenu());
  }
}

void MainWnd::OnFileLoadgameMostrecent() 
{
  if(!emulating)
    return;

  CString filename;

  int index = theApp.filename.ReverseFind('\\');

  if(index != -1)
    filename = theApp.filename.Right(theApp.filename.GetLength()-index-1);
  else
    filename = theApp.filename;

  CString saveDir = regQueryStringValue("saveDir", NULL);

  if(saveDir.IsEmpty())
    saveDir = getDirFromFile(theApp.filename);

  if(!isDriveRoot(saveDir))
    saveDir += "\\";

  CString name;
  CFileStatus status;
  CString str;
  time_t time = 0;
  int found = -1;
    
  for(int i = 0; i < 10; i++) {
    name.Format("%s%s%d.sg%1d", saveDir, filename, i+1,linkid+1);

    if(emulating && CFile::GetStatus(name, status)) {
      if(status.m_mtime.GetTime() > time) {
        time = status.m_mtime.GetTime();
        found = i;
      }
    } 
  }
  if(found != -1) {
    OnFileLoadSlot(ID_FILE_LOADGAME_SLOT1+found);
  }
}

void MainWnd::OnUpdateFileLoadgameMostrecent(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable(emulating);

  if(pCmdUI->m_pSubMenu != NULL) {
    CMenu *pMenu = pCmdUI->m_pSubMenu;
    CString filename;

    int index = theApp.filename.ReverseFind('\\');

    if(index != -1)
      filename = theApp.filename.Right(theApp.filename.GetLength()-index-1);
    else
      filename = theApp.filename;

    CString saveDir = regQueryStringValue("saveDir", NULL);

    if(saveDir.IsEmpty())
      saveDir = getDirFromFile(theApp.filename);

    if(!isDriveRoot(saveDir))
      saveDir += "\\";

    CString name;
    CFileStatus status;
    CString str;
    
    for(int i = 0; i < 10; i++) {
      name.Format("%s%s%d.sg%1d", saveDir, filename, i+1,linkid+1);

      if(emulating && CFile::GetStatus(name, status)) {
        CString timestamp = status.m_mtime.Format("%Y/%m/%d %H:%M:%S");
        str.Format("%d %s", i+1, timestamp);
      } else {
        str.Format("%d ----/--/-- --:--:--", i+1);
      }
      pMenu->ModifyMenu(ID_FILE_LOADGAME_SLOT1+i, MF_STRING|MF_BYCOMMAND, ID_FILE_LOADGAME_SLOT1+i, str);
    }

    theApp.winAccelMgr.UpdateMenu(pMenu->GetSafeHmenu());
  }
}

void MainWnd::OnUpdateFileLoadGameSlot(CCmdUI *pCmdUI)
{
  pCmdUI->Enable(emulating);
}

void MainWnd::OnUpdateFileSaveGameSlot(CCmdUI *pCmdUI)
{
  pCmdUI->Enable(emulating);
}

void MainWnd::OnFileLoadgameAutoloadmostrecent() 
{
  theApp.autoLoadMostRecent = !theApp.autoLoadMostRecent;
}

void MainWnd::OnUpdateFileLoadgameAutoloadmostrecent(CCmdUI* pCmdUI) 
{
  pCmdUI->SetCheck(theApp.autoLoadMostRecent);
}
