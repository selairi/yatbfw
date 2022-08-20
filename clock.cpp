
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
  
#include "debug.h"
#include "clock.h"
#include <time.h>
#include <vector>

static std::string get_time(std::string timeformat)
{
  time_t rawtime;
  struct tm *timeinfo;
  char buffer[80];

  time(&rawtime);
  timeinfo = localtime(&rawtime);

  strftime(buffer, 80, timeformat.c_str(), timeinfo);

  return std::string(buffer);
}

/** Clock is updated each minute. If contains seconds, it must be updated each second.
 */
static int get_timeout_from_timeformat(const std::string &timeformat)
{
  std::vector<std::string> seconds_formats = {"%s", "%S", "%T"};
  int timeout = 60000; // Update clock each 30 seconds
  for(std::string item : seconds_formats)
    if(timeformat.find(item) != std::string::npos)
      timeout = 1000;
  return timeout;
}

Clock::Clock(const std::string & icon, const std::string & timeformat) : ButtonRunCommand(icon, std::string(), std::string()) 
{
  m_timeformat = timeformat;
  set_text(get_time(timeformat));
  set_timeout(get_timeout_from_timeformat(timeformat));
  debug << "Time format: " << timeformat.c_str() << std::endl;
}

void Clock::timeout()
{
  std::string time = get_time(m_timeformat);
  if(time != get_text()) {
    set_text(time);
    send_repaint();
  }
}

void Clock::mouse_enter()
{
  std::string time = get_time("%A %d\n%B %Y\n%x");
  show_tooltip(time);
}
