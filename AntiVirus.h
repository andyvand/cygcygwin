#ifndef CINSTALL_ANTIVIRUS_H
#define CINSTALL_ANTIVIRUS_H

/*
 * Copyright (c) 2001, Gary R. Van Sickle.
 *
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 *
 *     A copy of the GNU General Public License can be found at
 *     http://www.gnu.org/
 *
 * Written by Robert Collisn  <rbtcollins@hotmail.com>
 *
 */

// This is the header for the AntiVirusPage class, which lets the user
// disable their virus scanner if needed.


#include "proppage.h"

class AntiVirus {
public:
    static void AtExit();
    static bool Show();
};

class AntiVirusPage:public PropertyPage
{
public:
  AntiVirusPage ()
  {
  };
  virtual ~ AntiVirusPage ()
  {
  };

  bool Create ();

  virtual void OnActivate ();
  virtual void OnDeactivate ();
  virtual long OnNext ();
  virtual long OnBack ();
  virtual long OnUnattended ();
};

#endif /* CINSTALL_ANTIVIRUS_H */