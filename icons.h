
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
 
#ifndef __ICONS_H__
#define __ICONS_H__

#include <string>
#include <cairo/cairo.h>
#include <librsvg/rsvg.h>
#include <memory>
#include <unordered_map>

/*! \class Icon
 *  \brief Icon to draw in a cairo surface.
 *
 *  A icon from icons resources are loaded with get_icon method
 *  and can be paint with paint method.
 *  Icons are stored in a map and are they reused.
 */
class Icon
{
public:
  /*! \brief Don't use this constructor to load icons.
   * Instead, use get_icon to load an icon.
   * \param path is used as key of map.
   * \param icon_path real path to icon image in disk.
   */
  Icon(const std::string & path, const std::string & icon_path);
  virtual ~Icon();

  void paint(cairo_t *cr, uint32_t x, uint32_t y, uint32_t width, uint32_t height);

  std::string get_icon_path();

  /*! \brief Load an icon from disk. The icon theme is used to
   * choose the icon.
   * \param path path to icon on disk or icon name.
   * Example:
   * auto firefox = Icon::get_icon("firefox");
   */
  static std::shared_ptr<Icon> get_icon(const std::string & path);
  static std::string suggested_icon_for_id(std::string id);

private:
  cairo_surface_t *m_icon;
  RsvgHandle *m_svg_icon;
  int m_icon_width, m_icon_height;
  std::string m_path; // Icon id name
  std::string m_icon_path;
  uint32_t m_ref_count;

  // Map of all loaded icons
  static std::unordered_map<std::string, std::weak_ptr<Icon> > icons;
};

#endif
