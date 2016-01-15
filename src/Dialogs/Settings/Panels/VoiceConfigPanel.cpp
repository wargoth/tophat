/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "VoiceConfigPanel.hpp"
#include "Profile/Profile.hpp"
#include "Language/Language.hpp"
#include "Interface.hpp"
#include "Widget/RowFormWidget.hpp"
#include "UIGlobals.hpp"
#include "Audio/VoiceRecogSettings.hpp"

enum ControlIndex {
  RepeatVoice,
};

class VoiceConfigPanel final : public RowFormWidget {
public:
  VoiceConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

public:
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc) override;
  virtual bool Save(bool &changed) override;
};

void
VoiceConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  const ComputerSettings &settings_computer = CommonInterface::GetComputerSettings();
  const VoiceRecogSettings &voice = settings_computer.voice_recog;
  AddBoolean(_("Repeat voice"), _("Repeat what you said."),
             voice.repeat_voice);
  SetExpertRow(RepeatVoice);
}

bool
VoiceConfigPanel::Save(bool &changed)
{
  ComputerSettings &settings_computer = CommonInterface::SetComputerSettings();
  VoiceRecogSettings &voice = settings_computer.voice_recog;
  if (SaveValue(RepeatVoice, ProfileKeys::RepeatVoice,
                voice.repeat_voice)) {
    changed = true;
  }
  return true;
}

Widget *
CreateVoiceConfigPanel()
{
  return new VoiceConfigPanel();
}
