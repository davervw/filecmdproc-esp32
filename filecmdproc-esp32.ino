// github.com/davervw/filecmdproc-esp32
// File Command Processor for ESP32 - manage files stored in ESP32 sotrage
//
// MIT License
//
// Copyright(c) 2023 by David R. Van Wagner
// davevw.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#include <Arduino.h>
#include "FFat.h"

bool mount = false;
String path = "/";

void prompt()
{
  Serial.print(path);
  Serial.print(" > ");
}

void readLineAndExecute();
void execute(String cmd, String args);
void help(String args);
void format(String args);
void mkdir(String args);
void rmdir(String args);
void cd(String args);
void touch(String args);
void cat(String args);
void hex(String args);
void ls(String args);
void rm(String args);
void mv(String args);

void setup() {
  Serial.begin(115200);
  while (!Serial) {} // wait for USB Serial to connect
  mount = FFat.begin();
  Serial.println("github.com/davervw/filecmdproc-esp32");
  Serial.println("Copyright (c) 2023 by David R. Van Wagner");
  Serial.println("MIT LICENSE");
  if (!mount)
    Serial.println("WARNING: FFat Mount Failed");
  Serial.println("esp32 disk ready for commands");
  prompt();
}

void loop() {
  readLineAndExecute();  
}

void help(String args)
{
  Serial.println("We all need help now and again.");
  if (args.length() > 0)
  {
    Serial.print("no help available for ");
    Serial.println(args);
  }
  else
  {
    Serial.println("commands are format, mkdir, rmdir, cd, cat, ls, rm, mv, hex");
  }
}

void format(String args)
{
  if (args.length() != 0)
  {
    Serial.println("format arguments not supported");
    return;
  }
  FFat.end();
  path = "/";
  Serial.print("formatting...");
  FFat.format();
  Serial.println();
  mount = FFat.begin();
  if (!mount)
    Serial.println("WARNING: FFat Mount Failed");
}

String computePath(String local)
{
  if (local.length() == 0)
    return path;
  if (local.startsWith("/"))
    return local;
  if (local.equals(".."))
  {
    int i = path.lastIndexOf("/");
    if (i > 0)
      return path.substring(0, i);
    return "/";
  }
  if (path.equals("/"))
    return path + local;
  return path + "/" + local;
}

void mkdir(String args)
{
  if (args.length() == 0)
  {
    Serial.println("path expected");
    return;
  }
  String localpath = computePath(args);
  if (!FFat.mkdir(localpath))
  {
    Serial.print("mkdir failed ");
    Serial.println(localpath);
  }
}

void rmdir(String args)
{
  if (args.length() == 0)
  {
    Serial.println("path expected");
    return;
  }
  String localpath = computePath(args);
  if (!FFat.rmdir(localpath))
  {
    Serial.print("rmdir failed ");
    Serial.println(localpath);
  }
}

void cd(String args)
{
  if (args.length() == 0)
  {
    Serial.println("path expected");
    return;
  }
  String localpath = computePath(args);
  File root = FFat.open(localpath);
  if (!root)
  {
    Serial.print("path not found ");
    Serial.println(localpath);
    return;
  }
  if (!root.isDirectory())
  {
    Serial.print("path not a directory ");
    Serial.println(localpath);
    root.close();
    return;
  }
  root.close();
  path = localpath;
}

void touch(String args)
{
  if (args.length() == 0)
  {
    Serial.println("filename expected");
    return;
  }
  String localpath = computePath(args);
  File file = FFat.open(localpath);
  if (file)
  {
    Serial.print("file exists ");
    Serial.println(localpath);
    file.close();
    return;
  }
  file = FFat.open(localpath, FILE_WRITE);
  if (!file)
  {
    Serial.print("failed to create file ");
    Serial.println(localpath);
    return;
  }
  file.close();
}

void cat(String args)
{
  if (args.length() == 0)
  {
    Serial.println("filename expected");
    return;
  }
  String localpath = computePath(args);
  File file = FFat.open(localpath);
  if (!file)
  {
    Serial.print("Failed to open file ");
    Serial.println(localpath);
    return;
  }
  if (file.isDirectory())
  {
    Serial.println("viewing a directory as text is not supported");
    file.close();
    return;
  }
  char buffer[1024];
  while (true)
  {
    int bytes = file.readBytes(&buffer[0], sizeof(buffer));
    Serial.write(&buffer[0], bytes);
    if (bytes != sizeof(buffer))
      break;
  }
  file.close();
}

void hex(String args)
{
  if (args.length() == 0)
  {
    Serial.println("filename expected");
    return;
  }
  String localpath = computePath(args);
  File file = FFat.open(localpath);
  if (!file)
  {
    Serial.print("Failed to open file ");
    Serial.println(localpath);
    return;
  }
  if (file.isDirectory())
  {
    Serial.println("viewing a directory as hex is not supported");
    file.close();
    return;
  }
  char buffer[16];
  int addr = 0;
  while (true)
  {
    int bytes = file.readBytes(&buffer[0], sizeof(buffer));
    Serial.printf("%04X", addr);
    for (int i=0; i<bytes; ++i)
      Serial.printf(" %02X", buffer[i]);
    Serial.println();
    if (bytes != sizeof(buffer))
      break;
    addr += bytes;
  }
  file.close();
}

void ls(String args)
{
  if (args.length() > 0)
  {
    Serial.println("ls arguments not supported");
    return;
  }
  File root = FFat.open(path);
  if (!root)
  {
    Serial.print("Could not open directory ");
    Serial.println(path);
    return;
  }
  if (!root.isDirectory())
  {
    Serial.print("Unexpected path is not a directory ");
    Serial.println(path);
    root.close();
    return;
  }
  File file = root.openNextFile();
  int dirs = 0;
  int files = 0;
  int bytes = 0;
  while (file)
  {
    if (file.isDirectory())
    {
      Serial.printf("[%s]\n", file.name());
      ++dirs;
    }
    else
    {
      Serial.printf("%s %d bytes\n", file.name(), file.size());
      ++files;
      bytes += file.size();
    }
    file.close();
    file = root.openNextFile();
  }
  root.close();
  Serial.printf("%d bytes, %d files, %d directories\n", bytes, files, dirs);
}

void rm(String args)
{
  if (args.length() == 0)
  {
    Serial.println("filename expected");
    return;
  }
  String localpath = computePath(args);
  if (!FFat.remove(localpath))
  {
    Serial.print("Failed to remove ");
    Serial.println(args);
  }
}

void mv(String args)
{
  String source, dest;
  int first = args.indexOf(' ');
  int last = args.lastIndexOf(' ');
  if (first != last || first < 0)
  {
    Serial.println("expected source and destination");
    return;
  }
  source = computePath(args.substring(0, first));
  dest = computePath(args.substring(first+1));
  int sourceSlash = source.lastIndexOf('/');
  int destSlash = dest.lastIndexOf('/');
  if (!FFat.rename(source, dest))
  {
    Serial.print("failed to move ");
    Serial.print(source);
    Serial.print(" to ");
    Serial.println(dest);
  }
}

void execute(String cmd, String args)
{
  if (cmd.equals("help"))
    help(args);
  else if (cmd.equals("format"))
    format(args);
  else if (cmd.equals("mkdir"))
    mkdir(args);
  else if (cmd.equals("rmdir"))
    rmdir(args);
  else if (cmd.equals("cd"))
    cd(args);
  else if (cmd.equals("touch"))
    touch(args);
  else if (cmd.equals("cat"))
    cat(args);
  else if (cmd.equals("hex"))
    hex(args);
  else if (cmd.equals("ls"))
    ls(args);
  else if (cmd.equals("rm"))
    rm(args);
  else if (cmd.equals("mv"))
    mv(args);
  else {
    Serial.print("Unknown command ");
    Serial.println(cmd);
  }
}

void readLineAndExecute()
{
  if (!Serial.available())
    return;
  String line = Serial.readString();
  line.trim();
  if (line.length() > 0)
  {
    int space = line.indexOf(' ');
    String cmd, args;
    if (space > 0)
    {
      cmd = line.substring(0, space);
      cmd.toLowerCase();
      args = line.substring(space+1);
    }
    else
    {
      cmd = line;
      cmd.toLowerCase();
      args = "";
    }
    execute(cmd, args);
  }
  prompt();
}
