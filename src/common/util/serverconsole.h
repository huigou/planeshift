/*
 * serverconsole.h - author: Matze Braun <matze@braunis.de>
 *
 * Copyright (C) 2001 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
 *
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
#ifndef __SERVERCONSOLE_H__
#define __SERVERCONSOLE_H__

#include "util/consoleout.h"
#include "util/gameevent.h"

#ifdef    USE_ANSI_ESCAPE
#define COL_NORMAL  "\033[m\017"
#define COL_RED        "\033[31m" 
#define COL_BLUE    "\033[34m"
#define COL_YELLOW  "\033[33m"
#define    COL_CYAN    "\033[36m"
#define COL_GREEN   "\033[32m"
#else
#define COL_NORMAL  ""
#define COL_RED        ""
#define COL_BLUE    ""
#define COL_CYAN    ""
#define COL_GREEN   ""
#define COL_YELLOW  ""
#endif

struct COMMAND;

const COMMAND *find_command(const char *name);
int execute_line(const char *line,csString *buffer);


/**
 * This defines an interface for intercepting commands instead of handling them
 * locally in the server console.
 */
class iCommandCatcher
{
public:
    virtual void CatchCommand(const char *cmd) = 0;
    virtual ~iCommandCatcher() {}
};

/**
 * This class is implements the user input and output console for the server.
 */
class ServerConsole : public ConsoleOut
{
public:

    enum Status
    {
        STOPPED,
        RUNNING,
        ABORTED
    };
    /**
     * Initializes the console. 
     * appname is the resulting name for this server application.
     */
    static void Init(const char *appname, const char *command_prompt);

    /**
     * Executes a script of commands. The script is passed
     * in as a char array buffer (NOT a filename). This function
     * goes through the char array script line at a time and executes
     * the given server command. It also allows for commenting the
     * script using the #comment...
     */
    static void ExecuteScript(const char* script);

    /**
     * The main server console loop. This waits for a user
     * to enter a line of data and executes the command
     * that the user entered.
     */
    static void InputRun();

    /**
     * Is the loop that runs until Stop() has been called.
     * This just calls InputRun() which contains the real loop.
     */
    static int MainLoop ();

    /**
     * Halts any execution of the server console. Sets running to RUNNING.
     */
    static void Stop();    

    /**
     * Aborts any execution of the server console. Sets running to ABORT.
     */
    static void Abort();

    static void SetCommandCatcher(iCommandCatcher *cmdcatch)
    {
        cmdcatcher = cmdcatch;
    }

    /// Holds the prompt.
    static const char *prompt;
protected:

    /// A boolean variable that gives the current status of the server console.
    static volatile Status running;

    /// Holds the name of the current application.
    static const char *appname;
    //

    /// CommandCatcher intercepts typed commands without processing them here
    static iCommandCatcher *cmdcatcher;
};

// Allows the console commands to be thread-safe by inserting them into the main event queue
class psServerConsoleCommand : public psGameEvent
{
    csString command;

public:
    // 0 offset for highest priority in the game event queue
    psServerConsoleCommand(const char* command) : psGameEvent(0, 0, "psServerStatusRunEvent"), command(command) {};
    void Trigger()
    {
        execute_line(command,NULL);
        CPrintf (CON_CMDOUTPUT, COL_BLUE "%s: " COL_NORMAL, ServerConsole::prompt);
    };
};

#endif

