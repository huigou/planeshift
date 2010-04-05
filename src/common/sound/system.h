/*
 * system.h
 *
 * Copyright (C) 2001-2010 Atomic Blue (info@planeshift.it, http://www.planeshift.it)
 *
 * Credits : Saul Leite <leite@engineer.com>
 *           Mathias 'AgY' Voeroes <agy@operswithoutlife.net>
 *           and all past and present planeshift coders
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation (version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#ifndef _SOUND_SYSTEM_H_
#define _SOUND_SYSTEM_H_

class SoundSystem
{
    public:
    bool Initialize(iObjectRegistry* objectReg);

    bool CreateStream (csRef<iSndSysData> &snddata, int loop, int type,
                       csRef<iSndSysStream> &sndstream);

    void RemoveStream(csRef<iSndSysStream> &sndstream);

    bool CreateSource (csRef<iSndSysStream> &sndstream,
                       csRef<iSndSysSource> &sndsource);

    void RemoveSource (csRef<iSndSysSource> &sndsource);

    void Create3dSource (csRef<iSndSysSource> &sndsource,
                         csRef<iSndSysSource3D> &sndsource3d,
                         float mindist, float maxdist, csVector3 pos);

    void CreateDirectional3dSource (csRef<iSndSysSource3D> &sndsource3d,
                                    csRef<iSndSysSource3DDirectionalSimple> &sndsourcedir,
                                    csVector3 direction, float rad);

    void UpdateListener( csVector3 v, csVector3 f, csVector3 t );

    private:
    csRef<iSndSysRenderer> sndrenderer;
    csRef<iSndSysListener> listener;
};

#endif /*_SOUND_SYSTEM_H_*/
