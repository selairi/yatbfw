
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
 
#include "buttonruncommand.h"
#include <stdlib.h>
#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <linux/input-event-codes.h>

ButtonRunCommand::ButtonRunCommand() : Button() { }
ButtonRunCommand::ButtonRunCommand(char *icon_path) : Button(icon_path) { }
ButtonRunCommand::ButtonRunCommand(std::string text) : Button(text) { }
ButtonRunCommand::ButtonRunCommand(char *icon_path, std::string text) :  Button(icon_path, text) { }

void ButtonRunCommand::setCommand(std::string command)
{
  m_command = command;
}

void ButtonRunCommand::setFD(int fd)
{
  m_fd = fd;
}

void ButtonRunCommand::on_mouse_clicked(int button)
{
  if(button == BTN_LEFT) {
    std::cout << m_command  << " &>/dev/null &" << std::endl;
    if(fork() == 0) {
      close(m_fd);
      system((m_command + " &> /dev/null &").c_str());
      // Wait until child dies
      int status;
      wait(&status);
      exit(0);
    }
  }
}


