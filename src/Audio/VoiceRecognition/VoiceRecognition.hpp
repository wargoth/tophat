/*
Copyright_License {

  Copyright (C) 2000-2015 Caz Yokoyama
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef VOICE_RECOGNITION_HPP
#define VOICE_RECOGNITION_HPP

#include "Thread/Thread.hpp"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
/* include top Julius library header */
#include <julius/juliuslib.h>

class EventQueue;

class VoiceRecognition final: protected Thread
{
private:
  EventQueue &queue;
  /**
   * configuration parameter holder
   * 
   */
  Jconf *jconf;

  /**
   * Recognition instance
   * 
   */
  Recog *recog;

  void PushWords(const char *words);

public:
  VoiceRecognition(EventQueue &_queue);
  ~VoiceRecognition();

  /**
   * Start the thread.  This method should be called after creating
   * this object.
   */
  bool Start();

  /**
   * Stop the thread.  This method must be called before the
   * destructor.
   */
  void Stop();

protected:
  /* virtual methods from Thread */
  void Run() override;
};

#endif
