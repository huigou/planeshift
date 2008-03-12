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

#ifndef CS_PRIORITY_USE_CAMERA

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

#define DEF_PROX_DIST   100        // 100m is trial distance here
#define DEF_UPDATE_DIST   5        //  30m is trial (default) delta to update
#define PROX_LIST_ANY_RANGE 0.0      // range of 0 means all members of proxlist in multicast.
/*  Dynamic proxlist range settings.
 *   The dynamic proxlist shrinks range in steps (maximum of 1 step per proxlist update)
 *   if the number of player entities on the proxlist exceeds PROX_LIST_SHRINK_THRESHOLD.
 *   When the number of entities is below PROX_LIST_REGROW_THRESHOLD and the range is
 *   below gemObject::prox_distance_desired, the range is increased.
 */
#define PROX_LIST_SHRINK_THRESHOLD  50   // 50 players in range - start radius shrink
#define PROX_LIST_REGROW_THRESHOLD  30   // 30 players in range - start radius grow
#define PROX_LIST_STEP_SIZE         10   // grow by this much each attempt

#define DEFAULT_INSTANCE             0   // Instance 0 is where 99% of things happen
typedef uint32 INSTANCE_ID;
#define INSTANCE_ALL 0xffffffff

#define ASSIST_MAX_DIST 25   // Maximum distance that the /assist command will work

#define EXCHANGE_SLOT_COUNT         8
#define INVENTORY_BULK_COUNT        32
#define INVENTORY_EQUIP_COUNT       16

#define PSITEM_MAX_CONTAINER_SLOTS  16   // maximum number of items an object can contain

#define GLYPH_WAYS               6
#define GLYPH_ASSEMBLER_SLOTS    4

#define RANGE_TO_SEE_ACTOR_LABELS 14
#define RANGE_TO_SEE_ITEM_LABELS 7
#define RANGE_TO_SELECT 5
#define RANGE_TO_LOOT 3
#define RANGE_TO_RECV_LOOT 100
#define RANGE_TO_USE 2
#define RANGE_TO_STACK 0.5  // Range to stack like items when dropping/creating in the world
#define DROP_DISTANCE 0.55   // Distance in front of player to drop items (just more then RANGE_TO_STACK)

#define LONG_RANGE_PERCEPTION  30
#define SHORT_RANGE_PERCEPTION 10
#define PERSONAL_RANGE_PERCEPTION  4

#define IS_CONTAINER true

// Minimum guild requirements
#define GUILD_FEE 20000
#define GUILD_MIN_MEMBERS 5
#define GUILD_KICK_GRACE 5 // minutes

#define MAX_GUILD_LEVEL    9

#define SIZET_NOT_FOUND ((size_t)-1)


//Changed from unsigned long to uint32 since we assert the size of PS_ID is equal to uint32_t
//uint32 is defined by CS to magically be like uint32_t on systems that don't have it by default
typedef uint32 PS_ID;


// This is needed to 64bit code, some functions break on 32bit Linux, but we need them on 32bit windows
#ifdef _WIN32
#define _LP64
#endif

#define WEATHER_MAX_RAIN_DROPS  8000
#define WEATHER_MAX_SNOW_FALKES 6000

#endif
