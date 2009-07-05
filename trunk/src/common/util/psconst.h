/*
 * psconst.h
 *
 * Copyright (C) 2003 PlaneShift Team (info@planeshift.it,
 * http://www.planeshift.it)
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
 *
 */

#ifndef PSCONST_HEADER
#define PSCONST_HEADER

// FIXME: This is not so great, but we need to be able to show and compute
// hashes for our new integral types.  Those probably shouldn't be here either.
#include <csutil/csstring.h>
#include <csutil/hash.h>
#include "psstdint.h" //Added to fix msvc build

#define CS_PRIORITY_USE_CAMERA    true

#define CEL_CAMERA_USE_CD  true
#define CEL_CAMERA_NO_CD   false

#define CEL_MESHSELECT_CAMERA_SPACE  true
#define CEL_MESHSELECT_WORLD_SPACE   false

#define CS_TEXTURE_CREATE_MATERIAL true

#define CS_MUTEX_RECURSIVE  true

#define SOCKET_CLOSE_FORCED  true

#define CS_FOLLOW_ONLY_PORTALS true

#define CS_LIGHT_STATIC CS_LIGHT_DYNAMICTYPE_STATIC
#define CS_LIGHT_PSEUDO CS_LIGHT_DYNAMICTYPE_PSEUDO

#define CS_LOADER_CLEAR_WORLD true
#define CS_LOADER_KEEP_WORLD  false

#define CS_LOADER_ONLY_REGION    true
#define CS_LOADER_ACROSS_REGIONS false

#define DEF_PROX_DIST   100        ///< 100m is trial distance here
#define DEF_UPDATE_DIST   5        ///<  30m is trial (default) delta to update
#define PROX_LIST_ANY_RANGE 0.0      ///< range of 0 means all members of proxlist in multicast.
/** @name Dynamic proxlist range settings.
 *   The dynamic proxlist shrinks range in steps (maximum of 1 step per proxlist update)
 *   if the number of player entities on the proxlist exceeds PROX_LIST_SHRINK_THRESHOLD.
 *   When the number of entities is below PROX_LIST_REGROW_THRESHOLD and the range is
 *   below gemObject::prox_distance_desired, the range is increased.
 */
//@{
#define PROX_LIST_SHRINK_THRESHOLD  50   ///< 50 players in range - start radius shrink
#define PROX_LIST_REGROW_THRESHOLD  30   ///< 30 players in range - start radius grow
#define PROX_LIST_STEP_SIZE         10   ///< grow by this much each attempt
//@}

#define DEFAULT_INSTANCE             0   ///< Instance 0 is where 99% of things happen
typedef uint32 InstanceID;
#define INSTANCE_ALL 0xffffffff

#define ASSIST_MAX_DIST 25   ///< Maximum distance that the /assist command will work

#define EXCHANGE_SLOT_COUNT         8
#define INVENTORY_BULK_COUNT        32
#define INVENTORY_EQUIP_COUNT       16

#define PSITEM_MAX_CONTAINER_SLOTS  16   ///< maximum number of items an object can contain

#define GLYPH_WAYS               6
#define GLYPH_ASSEMBLER_SLOTS    4

#define RANGE_TO_SEE_ACTOR_LABELS 14
#define RANGE_TO_SEE_ITEM_LABELS 7
#define RANGE_TO_SELECT 5
#define RANGE_TO_LOOT 3
#define RANGE_TO_RECV_LOOT 100
#define RANGE_TO_USE 2
#define RANGE_TO_STACK 0.5  ///< Range to stack like items when dropping/creating in the world
#define DROP_DISTANCE 0.55   ///< Distance in front of player to drop items (just more then RANGE_TO_STACK)
#define RANGE_TO_GUARD 5 ///< Range into which you can guard an item

#define LONG_RANGE_PERCEPTION  30
#define SHORT_RANGE_PERCEPTION 10
#define PERSONAL_RANGE_PERCEPTION  4

#define IS_CONTAINER true

/** @name Minimum guild requirements
 */
//@{
#define GUILD_FEE 20000
#define GUILD_MIN_MEMBERS 5
#define GUILD_KICK_GRACE 5 ///< minutes

#define MAX_GUILD_LEVEL    9
//@}

#define SIZET_NOT_FOUND ((size_t)-1)

// This is needed to 64bit code, some functions break on 32bit Linux, but we need them on 32bit windows
#ifdef _WIN32
#define _LP64
#endif

#define WEATHER_MAX_RAIN_DROPS  8000
#define WEATHER_MAX_SNOW_FALKES 6000

enum SPELL_TYPE
{
    BUFF,
    DEBUFF,
};

/** Make unique integer types for various types of IDs.  This allows the
  * compiler to statically check for the right kind of ID, preventing various
  * kinds of mistakes.  It also documents which kind of ID is required.
  */
#define MAKE_ID_TYPE(name)                                               \
class name                                                               \
{                                                                        \
public:                                                                  \
    name()           : id(0) {}                                          \
    name(uint32_t i) : id(i) {}                                          \
    uint32_t Unbox() const { return id; }                                \
    bool IsValid() const { return id != 0; }                             \
    csString Show() const                                                \
    {                                                                    \
        csString str(#name":");                                          \
        str.Append((unsigned int) id);                                         \
        return str;                                                      \
    }                                                                    \
    bool operator==(const name & other) const { return id == other.id; } \
    bool operator!=(const name & other) const { return id != other.id; } \
    bool operator< (const name & other) const { return id <  other.id; } \
private:                                                                 \
    uint32_t id;                                                         \
};                                                                       \
template<>                                                               \
class csHashComputer<name>                                               \
{                                                                        \
public:                                                                  \
    static uint ComputeHash(name key)                                    \
    {                                                                    \
        return key.Unbox();                                              \
    }                                                                    \
};
// Note: The < operator allows us to use these as keys for sets and trees.
//       The csHashComputer allows us to use them as keys in csHash.

/** Convenience wrapper so we don't have to write ugly things like
  * actor->GetEID().Show().GetData() all over the place.
  */
#define ShowID(id) id.Show().GetData()

MAKE_ID_TYPE(EID);         ///< GEM Entity IDs
MAKE_ID_TYPE(PID);         ///< Player IDs
MAKE_ID_TYPE(AccountID);   ///< Account IDs

/** Container IDs are either EIDs (if > 100) or inventory slot IDs.
  * This is quite ugly; we don't try to add proper typing.
  */
typedef uint32_t ContainerID;

#endif
