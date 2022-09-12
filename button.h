
/*
 * Copyright 2021 P.L. Lucas <selairi@gmail.com>
 * 
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
 
#ifndef __BUTTON_H__
#define __BUTTON_H__

#include <string>
#include "panelitem.h"
#include "icons.h"
#include <librsvg/rsvg.h>

/*! \class Button
 *  \brief Simple button to add to panel.
 */
class Button : public PanelItem
{
public:
  Button();
  Button(const std::string & icon_path, const std::string & text);
  virtual ~Button();

  void set_text(const std::string & text);
  std::string get_text();
  void set_icon(const std::string & icon_path);
  std::string get_icon();
  void set_tooltip(const std::string & tooltip);
  std::string get_tooltip();

  virtual void paint(cairo_t *cr) override;
  virtual void update_size(cairo_t *cr) override;

  virtual void mouse_enter() override;
private:
  std::shared_ptr<Icon> m_icon_ref;
  std::string m_text, m_tooltip;

  void draw_text(cairo_t *cr, int x_offset, int y_offset, std::string text);

protected:
  void init(const std::string & icon_path, const std::string & text);
};

#endif
