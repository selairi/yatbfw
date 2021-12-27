
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
 
#ifndef __TOPLEVEL_BUTTON_H__
#define __TOPLEVEL_BUTTON_H__

#include <string>
#include <wayland-client.hpp>
#include <wayland-client-protocol-extra.hpp>
#include "button.h"
#include "toplevel.h"


/*! \class ToplevelButton
 *  \brief Button that lets focus, maximize, minimize,... a window.
 *
 *  Button that lets focus, maximize, minimize,... a window.
 */
class ToplevelButton : public Button
{
public:
  ToplevelButton(wayland::zwlr_foreign_toplevel_handle_v1_t toplevel_handle, wayland::seat_t seat, std::vector<ToplevelButton*> *toplevels);

  virtual void on_mouse_clicked(int button) override;

  std::function<void(bool)> repaint_main_interface;

  bool is_fullscreen();

private:
  wayland::zwlr_foreign_toplevel_handle_v1_t m_toplevel_handle;
  std::string m_title, m_id;
  wayland::output_t m_output;
  wayland::array_t m_state;
  wayland::seat_t m_seat;
  std::vector<ToplevelButton*> *m_toplevels; 
  bool m_maximized, m_activated, m_minimized, m_fullscreen;
  //std::string m_icon_path;

  void update_states(wayland::array_t toplevel_states);
};

#endif
