
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
 
#ifndef __PANELITEM_H__
#define __PANELITEM_H__

#include <cairo/cairo.h>
#include <string>

/*! \class PanelItem
 *  \brief Brief Base class for items in panel.
 *
 *  This is a base class for items in panel. 
 */
class PanelItem
{
public:
  PanelItem();
  virtual ~PanelItem() = default;

  void set_pos(int x, int y);
  void set_width(int width);
  void set_height(int height);
  int get_x();
  int get_y();
  int get_width();
  int get_height();
  bool get_selected();
  void set_selected(bool selected);
  bool is_start_pos();
  void set_start_pos(bool pos);

  void repaint(cairo_t *cr);

  bool is_in(int x, int y);
  bool need_repaint();

  void on_mouse_enter(int x, int y); 
  void on_mouse_leave(int x, int y, bool leave);
  void on_mouse_clicked(int x, int y, int button);
  void on_mouse_released(int x, int y);
  void on_timeout(long now_in_msecs);

  void set_timeout(int timeout_msecs);
  long next_time_timeout(long now_in_msecs);

  void show_tooltip(std::string text);

  virtual void paint(cairo_t *cr);

  virtual void mouse_enter(); 
  virtual void mouse_leave();
  virtual void mouse_clicked(int button);
  virtual void mouse_released();
  virtual void timeout();

protected:
  int m_x; /*!< x postion*/ 
  int m_y; /*!< y position */
  int m_width;
  int m_height;

  bool m_mouse_over, m_mouse_clicked;
  bool m_need_repaint;
  bool m_has_focus;
  bool m_selected;

  bool m_start_position; /*!< If true, item is drawn at start posotions. If false, the item is drawn at end positions*/

  int m_timeout_msecs;
  long m_next_time_timeout;
};

#endif
