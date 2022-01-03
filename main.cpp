
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
 
#include "panel.h"
#include "settings.h"
#include "configure.h"
#include <string.h>
#include <iostream>
#include <filesystem>
#include <execinfo.h>


void printstacktrace(int sig) {
  void *array[10];
  size_t size;

  // get void*'s for all entries on the stack
  size = backtrace(array, 10);

  // print out all the frames to stderr
  backtrace_symbols_fd(array, size, STDERR_FILENO);
}

int main(int argn, char *argv[])
{
  signal(SIGSEGV, printstacktrace);
  Panel panel;

  Settings *settings = Settings::get_settings();

  {
    // Parse command line
    bool settings_file = false;

    for(int i = 1; i < argn; i++) {
      if(argn > (i+1) && !strcmp(argv[i], "--settings")) { 
        settings->load_settings(std::string(argv[++i]), &panel);
        settings_file = true;
      }
    }
    if(!settings_file) {
      std::string path(settings->get_env("XDG_CONFIG_HOME") + "/yatbfw.json");
      std::filesystem::directory_entry settings_entry(path);
      if(settings_entry.exists())
        settings->load_settings(path, &panel);
      else {
        std::filesystem::directory_entry settings_orig(SHARE_PATH + std::string("/yatbfw.json"));
        std::filesystem::copy_file(settings_orig, settings_entry);
        settings->load_settings(path, &panel);
      }
    }
  }

  try {
    panel.init();
    // Run events loop
    panel.run();
    
	delete settings;
  } catch(const std::exception& e) {
    std::cout << "Exception launched:" << std::endl;
    std::cout << e.what() << std::endl;
    printstacktrace(0);
  }
  return 0;
}
