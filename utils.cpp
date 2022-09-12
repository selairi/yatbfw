
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
#include "utils.h"
#include <stdio.h>
#include <vector>

std::string Utils::read_command(std::string command)
{
  FILE *in = popen(command.c_str(), "r");
  char buffer[256];
  if(in == NULL)
    return std::string();
  std::string output;
  while(fgets(buffer, sizeof(buffer), in) != NULL)
    output += buffer;
  pclose(in);
  return output;
}

std::string Utils::read_command(const char *command)
{
  return read_command(std::string(command)); 
}

// Get lines of text
std::vector<std::string> get_lines(std::string text)
{
  std::vector<std::string> lines;
  size_t start = 0;
  size_t end = text.find("\n");
  if(end == std::string::npos)
    lines.push_back(text);
  else { 
    while (end != std::string::npos) {
      lines.push_back(text.substr(start, end - start));
      start = end + 1;
      end = text.find("\n", start);
    }
    lines.push_back(text.substr(start));
  }

  return lines;
}

