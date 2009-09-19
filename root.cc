/*
 * Copyright (c) 2000, Red Hat, Inc.
 *
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 *
 *     A copy of the GNU General Public License can be found at
 *     http://www.gnu.org/
 *
 * Written by DJ Delorie <dj@cygnus.com>
 *
 */

/* The purpose of this file is to ask the user where they want the
   root of the installation to be, and to ask whether the user prefers
   text or binary mounts. */

#if 0
static const char *cvsid =
  "\n%%% $Id$\n";
#endif

#include "root.h"

#include "LogSingleton.h"

#include "win32.h"
#include <shlobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "dialog.h"
#include "resource.h"
#include "state.h"
#include "msg.h"
#include "package_db.h"
#include "mount.h"

#include "getopt++/StringOption.h"

using namespace std;

StringOption RootOption ("", 'R', "root", "Root installation directory", false);

static ControlAdjuster::ControlInfo RootControlsInfo[] = {
  { IDC_ROOTDIR_GRP,              CP_STRETCH,           CP_TOP      },
  { IDC_ROOT_DIR,                 CP_STRETCH,           CP_TOP      },
  { IDC_ROOT_BROWSE,              CP_RIGHT,             CP_TOP      },

  { IDC_INSTALLFOR_GRP,           CP_STRETCH_LEFTHALF,  CP_STRETCH  },
  { IDC_ROOT_SYSTEM,              CP_LEFT,              CP_TOP      },
  { IDC_ALLUSERS_TEXT,            CP_STRETCH_LEFTHALF,  CP_TOP      },
  { IDC_ROOT_USER,                CP_LEFT,              CP_BOTTOM   },
  { IDC_JUSTME_TEXT,              CP_STRETCH_LEFTHALF,  CP_BOTTOM   },

  { IDC_MODE_GRP,                 CP_STRETCH_RIGHTHALF, CP_STRETCH  },
  { IDC_ROOT_BINARY,              CP_STRETCH_RIGHTHALF, CP_TOP      },
  { IDC_MODE_BIN,                 CP_STRETCH_RIGHTHALF, CP_TOP      },
  { IDC_ROOT_TEXT,                CP_STRETCH_RIGHTHALF, CP_BOTTOM   },
  { IDC_MODE_TEXT,                CP_STRETCH_RIGHTHALF, CP_BOTTOM   },
  { IDC_FILEMODES_LINK,           CP_RIGHT,             CP_BOTTOM   },
  {0, CP_LEFT, CP_TOP}
};

static int rb[] = { IDC_ROOT_TEXT, IDC_ROOT_BINARY, 0 };
static int su[] = { IDC_ROOT_SYSTEM, IDC_ROOT_USER, 0 };

static string orig_root_dir;

static void
check_if_enable_next (HWND h)
{
  EnableWindow (GetDlgItem (h, IDOK), root_text
		&& egetString (h, IDC_ROOT_DIR).size() && root_scope);
}

static inline void
GetDlgItemRect (HWND h, int item, LPRECT r)
{
  GetWindowRect (GetDlgItem (h, item), r);
  MapWindowPoints (HWND_DESKTOP, h, (LPPOINT) r, 2);
}

static inline void
SetDlgItemRect (HWND h, int item, LPRECT r)
{
  MoveWindow (GetDlgItem (h, item), r->left, r->top,
	      r->right - r->left, r->bottom - r->top, TRUE);
}

static void
load_dialog (HWND h)
{
  rbset (h, rb, root_text);
  rbset (h, su, root_scope);
  eset (h, IDC_ROOT_DIR, get_root_dir ());
  check_if_enable_next (h);
  if (IsWindowsNT ())
    {
      /* Stretch IDC_ROOTDIR_GRP content to full dialog width.  Use
         IDC_ROOTDIR_GRP rectangle as fix point. */
      RECT rect_base, rect;
      GetDlgItemRect (h, IDC_ROOTDIR_GRP, &rect_base);

      GetDlgItemRect (h, IDC_INSTALLFOR_GRP, &rect);
      rect.right = rect_base.right;
      SetDlgItemRect (h, IDC_INSTALLFOR_GRP, &rect);

      GetDlgItemRect (h, IDC_ALLUSERS_TEXT, &rect);
      rect.right = rect_base.right - 25;
      SetDlgItemRect (h, IDC_ALLUSERS_TEXT, &rect);

      GetDlgItemRect (h, IDC_JUSTME_TEXT, &rect);
      rect.right = rect_base.right - 25;
      SetDlgItemRect (h, IDC_JUSTME_TEXT, &rect);

      /* Change adjustment accordingly. */
      RootControlsInfo[3].horizontalPos = CP_STRETCH;
      RootControlsInfo[5].horizontalPos = CP_STRETCH;
      RootControlsInfo[7].horizontalPos = CP_STRETCH;

      SetWindowText (GetDlgItem (h, IDC_ALLUSERS_TEXT),
		     "Cygwin will be available to all users of the system.");
      SetWindowText (GetDlgItem (h, IDC_JUSTME_TEXT),
		     "Cygwin will still be available to all users, but "
		     "Desktop Icons, Cygwin Menu Entries, and important "
		     "Installer information are only available to the current "
		     "user.  Only select this if you lack Administrator "
		     "privileges or if you have specific needs.");

      ShowWindow (GetDlgItem (h, IDC_MODE_GRP), SW_HIDE);
      ShowWindow (GetDlgItem (h, IDC_ROOT_BINARY), SW_HIDE);
      ShowWindow (GetDlgItem (h, IDC_MODE_BIN), SW_HIDE);
      ShowWindow (GetDlgItem (h, IDC_ROOT_TEXT), SW_HIDE);
      ShowWindow (GetDlgItem (h, IDC_MODE_TEXT), SW_HIDE);
      ShowWindow (GetDlgItem (h, IDC_FILEMODES_LINK), SW_HIDE);
    }
}

static void
save_dialog (HWND h)
{
  root_text = IsWindowsNT () ? IDC_ROOT_BINARY : rbget (h, rb);
  root_scope = rbget (h, su);
  set_root_dir (egetString (h, IDC_ROOT_DIR));
}

static int CALLBACK
browse_cb (HWND h, UINT msg, LPARAM lp, LPARAM data)
{
  switch (msg)
    {
    case BFFM_INITIALIZED:
      if (get_root_dir ().size())
	SendMessage (h, BFFM_SETSELECTION, TRUE, (LPARAM) get_root_dir ().c_str());
      break;
    }
  return 0;
}

static void
browse (HWND h)
{
  BROWSEINFO bi;
  CHAR name[MAX_PATH];
  LPITEMIDLIST pidl;
  memset (&bi, 0, sizeof (bi));
  bi.hwndOwner = h;
  bi.pszDisplayName = name;
  bi.lpszTitle = "Select an installation root directory";
  bi.ulFlags = BIF_RETURNONLYFSDIRS;
  bi.lpfn = browse_cb;
  pidl = SHBrowseForFolder (&bi);
  if (pidl)
    {
      if (SHGetPathFromIDList (pidl, name))
	eset (h, IDC_ROOT_DIR, name);
    }
}

#define isslash(c) ((c) == '\\' || (c) == '/')

static int
directory_is_absolute ()
{
  
  const char *r = get_root_dir ().c_str();
  if (isalpha (r[0]) && r[1] == ':' && (r[2] == '\\' || r[2] == '/'))
    {
      return 1;
    }
  return 0;
}

static int
directory_is_rootdir ()
{
  
  for (const char *c = get_root_dir().c_str(); *c; c++)
    if (isslash (c[0]) && c[1] && !isslash (c[1]))
      return 0;
  return 1;
}

static int
directory_has_spaces ()
{
  if (std::string(get_root_dir()).find(' ') != std::string::npos)
    return 1;
  return 0;
}

bool
RootPage::OnMessageCmd (int id, HWND hwndctl, UINT code)
{
  switch (id)
    {

    case IDC_ROOT_DIR:
    case IDC_ROOT_TEXT:
    case IDC_ROOT_BINARY:
    case IDC_ROOT_SYSTEM:
    case IDC_ROOT_USER:
      check_if_enable_next (GetHWND ());
      break;

    case IDC_ROOT_BROWSE:
      browse (GetHWND ());
      break;
    default:
      return false;
    }
  return true;
}

RootPage::RootPage ()
{
  sizeProcessor.AddControlInfo (RootControlsInfo);
}

bool
RootPage::Create ()
{
  return PropertyPage::Create (IDD_ROOT);
}

void
RootPage::OnInit ()
{
  makeClickable (IDC_FILEMODES_LINK, 
        "http://cygwin.com/cygwin-ug-net/using-textbinary.html");
  if (((string)RootOption).size()) 
    set_root_dir((string)RootOption);
  if (!get_root_dir ().size())
    read_mounts (std::string ());
  orig_root_dir = get_root_dir();
  load_dialog (GetHWND ());
}

bool
RootPage::wantsActivation() const
{
  return (source != IDC_SOURCE_DOWNLOAD);
}

long
RootPage::OnNext ()
{
  HWND h = GetHWND ();

  save_dialog (h);

  if (!directory_is_absolute ())
    {
      note (h, IDS_ROOT_ABSOLUTE);
      return -1;
    }
  else if (get_root_dir() != orig_root_dir &&
           directory_is_rootdir () && (IDNO == yesno (h, IDS_ROOT_SLASH)))
    return -1;
  else if (directory_has_spaces () && (IDNO == yesno (h, IDS_ROOT_SPACE)))
    return -1;

  log (LOG_PLAIN) << "root: " << get_root_dir ()
    << (root_text == IDC_ROOT_TEXT ? " text" : " binary")
    << (root_scope == IDC_ROOT_USER ? " user" : " system") << endLog;

  /* Deferred initialization of packagedb *after* the root dir has been
     chosen. */
  packagedb db;
  db.task = chosen_db_task;

  return 0;
}

long
RootPage::OnBack ()
{
  HWND h = GetHWND ();

  save_dialog (h);
  return 0;
}

long
RootPage::OnUnattended ()
{
  return OnNext();
}
