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

/* The purpose of this file is to get and parse the setup.ini file
   from the mirror site.  A few support routines for the bison and
   flex parsers are provided also.  We check to see if this setup.ini
   is older than the one we used last time, and if so, warn the user. */

#if 0
static const char *cvsid =
  "\n%%% $Id$\n";
#endif

#include "win32.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "ini.h"
#include "resource.h"
#include "concat.h"
#include "state.h"
#include "geturl.h"
#include "dialog.h"
#include "msg.h"
#include "log.h"
#include "version.h"
#include "mount.h"
#include "site.h"
#include "rfc1738.h"
#include "find.h"

#include "io_stream.h"

unsigned int setup_timestamp = 0;
char *setup_version = 0;

extern int yyparse ();
/*extern int yydebug;*/

static char *error_buf = 0;
static int error_count = 0;

static int local_ini;

static void
find_routine (char *path, unsigned int fsize)
{
  if (!strstr (path, "/setup.ini") )
    return;
  io_stream *ini_file = io_stream::open (concat ("file://", local_dir,"/", path, 0), "rb");
  if (!ini_file)
    {
    note (IDS_SETUPINI_MISSING, path);
    return;
    }

  /* FIXME: only use most recent copy */
  setup_timestamp = 0;
  setup_version = 0;

  ini_init (ini_file, concat ("file://", local_dir,"/", path, 0));

  /*yydebug = 1; */

  if (yyparse () || error_count > 0)
    MessageBox (0, error_buf, error_count == 1 ? "Parse Error" : "Parse Errors", 0);
  else
    local_ini++;
}

static int
do_local_ini ()
{
  local_ini = 0;
  find (local_dir, find_routine);
  return local_ini; 
}

static int
do_remote_ini ()
{
  size_t ini_count = 0;
  for (size_t n = 1; n <= site_list.number (); n++)
    {
      io_stream *ini_file =
	get_url_to_membuf (concat (site_list[n]->url, "/setup.ini", 0));
      dismiss_url_status_dialog ();

      if (!ini_file)
	{
	  note (IDS_SETUPINI_MISSING, site_list[n]->url);
	  continue;
	}

      /* FIXME: only use most recent copy */
      setup_timestamp = 0;
      setup_version = 0;

      ini_init (ini_file, site_list[n]->url);

      /*yydebug = 1; */

      if (yyparse () || error_count > 0)
	MessageBox (0, error_buf,
		    error_count == 1 ? "Parse Error" : "Parse Errors", 0);
      else
	{
	  /* save known-good setup.ini locally */
	  char const *fp = concat ("file://", local_dir, "/",
				   rfc1738_escape_part (site_list[n]->url),
				   "/setup.ini", 0);
	  io_stream::mkpath_p (PATH_TO_FILE, fp);
	  io_stream *inistream = io_stream::open (fp, "wb");
	  if (inistream && !ini_file->seek (0, IO_SEEK_SET))
	    {
	      if (io_stream::copy (ini_file, inistream))
		io_stream::remove (fp);
	      delete ini_file;
	      delete inistream;
	    }
	  free ((void *) fp);
	  ini_count++;
	}
    }
  return ini_count;
}

void
do_ini (HINSTANCE h)
{
  size_t ini_count = 0;
  if (source == IDC_SOURCE_CWD)
    ini_count = do_local_ini ();
  else
    ini_count = do_remote_ini ();

  if (ini_count == 0)
    {
      next_dialog = source == IDC_SOURCE_CWD ? IDD_S_FROM_CWD : IDD_SITE;
      return;
    }

  if (get_root_dir ())
    {
      io_stream::mkpath_p (PATH_TO_DIR, "cygfile:///etc/setup");

      unsigned int old_timestamp = 0;
      io_stream *ots =
	io_stream::open ("cygfile:///etc/setup/timestamp", "rt");
      if (ots)
	{
	  char temp[20];
	  memset (temp, '\0', 20);
	  if (ots->read (temp, 19))
	    sscanf (temp, "%u", &old_timestamp);
	  delete ots;
	  if (old_timestamp && setup_timestamp
	      && (old_timestamp > setup_timestamp))
	    {
	      int yn = yesno (IDS_OLD_SETUPINI);
	      if (yn == IDNO)
		exit_setup (1);
	    }
	}
      if (setup_timestamp)
	{
	  io_stream *nts =
	    io_stream::open ("cygfile:///etc/setup/timestamp", "wt");
	  if (nts)
	    {
	      char temp[20];
	      sprintf (temp, "%u", setup_timestamp);
	      nts->write (temp, strlen (temp));
	      delete nts;
	    }
	}
    }

  msg ("setup_version is %s, our_version is %s", setup_version ? : "(null)",
       version);
  if (setup_version)
    {
      char *ini_version = canonicalize_version (setup_version);
      char *our_version = canonicalize_version (version);
      if (strcmp (our_version, ini_version) < 0)
	note (IDS_OLD_SETUP_VERSION, version, setup_version);
    }

  next_dialog = IDD_CHOOSE;
}

extern int yylineno;

extern "C" int
yyerror (char *s, ...)
{
  char buf[1000];
  int len;
  sprintf (buf, "setup.ini line %d: ", yylineno);
  va_list args;
  va_start (args, s);
  vsprintf (buf + strlen (buf), s, args);
  OutputDebugString (buf);
  if (error_buf)
    {
      strcat (error_buf, "\n");
      len = strlen (error_buf) + strlen (buf) + 5;
      error_buf = (char *) realloc (error_buf, len);
      strcat (error_buf, buf);
    }
  else
    {
      len = strlen (buf) + 5;
      error_buf = (char *) malloc (len);
      strcpy (error_buf, buf);
    }
  error_count++;
  /* TODO: is return 0 correct? */
  return 0;
}

extern "C" int fprintf (FILE * f, const char *s, ...);

static char stderrbuf[1000];

int
fprintf (FILE * f, const char *fmt, ...)
{
  char buf[1000];
  int rv;
  va_list args;
  va_start (args, fmt);
  if (f == stderr)
    {
      rv = vsprintf (buf, fmt, args);
      strcat (stderrbuf, buf);
      if (char *nl = strchr (stderrbuf, '\n'))
	{
	  *nl = 0;
	  /*OutputDebugString (stderrbuf); */
	  MessageBox (0, buf, "Cygwin Setup", 0);
	  stderrbuf[0] = 0;
	}

    }
  else
    {
      rv = vfprintf (f, fmt, args);
    }
  return rv;
}
