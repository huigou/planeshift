/*
 * hiresession.h  creator <andersr@pvv.org>
 *
 * Copyright (C) 2013 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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
#ifndef HIRE_SESSION_HEADER
#define HIRE_SESSION_HEADER
//====================================================================================
// Crystal Space Includes
//====================================================================================

//====================================================================================
// Project Includes
//====================================================================================
 
//====================================================================================
// Local Includes
//====================================================================================
#include <gem.h>

//------------------------------------------------------------------------------------
// Forward Declarations
//------------------------------------------------------------------------------------

/** The Hire Sessin will manage all aspects related to a specefic hiring of a NPC.
 *
 *  Players is allowed to Hire NPCs to do tasks for them. This can include
 *  selling of items, standing guard for the guild, etc. This session handle
 *  the details for the hire session. The main business logic for hiring is
 *  managed by the HireManager.
 */
class HireSession
{
public:
    /** Constructor.
     *
     * @param owner The player that start to hire a NPCs.
     */
    HireSession(gemActor* owner);

    /** Destructor.
     */
    virtual ~HireSession();

    /** Set pending hire type.
     *
     *  @param name The name of the NPC type to hire.
     *  @param npcType The NPC Type of the NPC type to hire type.
     */
    void SetHireType(const csString& name, const csString& npcType);

    /** Get hire type.
     *
     *  @return The NPC Type of the NPC type to hire type.
     */
    const csString& GetHireTypeName() const;

    /** Get hire type.
     *
     *  @return The NPC Type of the NPC type to hire type.
     */
    const csString& GetHireTypeNPCType() const;

    /** Set pending hire master NPC PID.
     *
     *  @param masterPID The PID of the master NPC.
     */
    void SetMasterPID(PID masterPID);

    /** Get Master PID.
     *
     *  @return The Master PID.
     */
    PID GetMasterPID() const;

    /** Set hired NPC.
     *
     *  @param hiredNPC The hired NPC.
     */
    void SetHiredNPC(gemNPC* hiredNPC);

    /** Verify if all requirments for hire is ok.
     *
     *  @return True if ok to confirm hire.
     */
    bool VerifyPendingHireConfigured();

protected:
private:
    PID                 ownerPID;    ///< The PID of the player that hire a NPC.
    PID                 hiredPID;    ///< The PID of the NPC that is hired.

    // Data for pending hires.
    csString            hireTypeName;    ///< The name of the type of hire.
    csString            hireTypeNPCType; ///< The NPC brain of the type of hire.
    PID                 masterPID;       ///< The master NPC PID.

    csWeakRef<gemActor> owner;       ///< Cached pointer to player actor when online.
    csWeakRef<gemNPC>   hiredNPC;    ///< Cached pointer to NPC actor when loaded.
};

#endif
