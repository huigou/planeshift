/*
 * autoexec.h
 *
 * Author: Fabian Stock (Aiwendil)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation (version 2 of the License)
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#ifndef AUTOEXEC_HEADER
#define AUTOEXEC_HEADER

// Crystal Space Includes
#include <csutil/csstring.h>
#include <csutil/array.h>

struct AutoexecCommand
{
  csString name;
  csString cmd;
};

// class handling autoexecution of commands at the startup.
class Autoexec
{
  public:
    Autoexec();
    ~Autoexec();
    void execute();
    bool GetEnabled() { return enabled; };
  protected:
    void LoadCommands(const char * fileName);
    void SaveCommands();
    void addCommand(csString name, csString cmd);
    bool enabled;
    csArray<AutoexecCommand> cmds;
};


#endif // AUTOEXEC_HEADER

