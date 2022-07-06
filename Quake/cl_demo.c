/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2010-2014 QuakeSpasm developers

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "time.h"
#include "quakedef.h"

static void CL_FinishTimeDemo (void);

typedef struct framepos_s // woods #demorewind (Baker Fitzquake Mark V)
{
	long				baz;
	struct framepos_s* next;
} framepos_t;

framepos_t* dem_framepos = NULL;
qboolean	start_of_demo = false;
qboolean	bumper_on = false;

/*
==============================================================================

DEMO CODE

When a demo is playing back, all NET_SendMessages are skipped, and
NET_GetMessages are read from the demo file.

Whenever cl.time gets past the last received message, another message is
read from the demo file.
==============================================================================
*/

/*
==============
CL_StopPlayback

Called when a demo file runs out, or the user starts a game
==============
*/
void CL_StopPlayback (void)
{
	if (!cls.demoplayback)
		return;

	fclose (cls.demofile);
	cls.demoplayback = false;
	cls.demopaused = false;
	cls.demofile = NULL;
	cls.state = ca_disconnected;

	if (cls.timedemo)
		CL_FinishTimeDemo ();
}

/*
====================
CL_WriteDemoMessage

Dumps the current net message, prefixed by the length and view angles
====================
*/
static void CL_WriteDemoMessage (void)
{
	int	len;
	int	i;
	float	f;

	len = LittleLong (net_message.cursize);
	fwrite (&len, 4, 1, cls.demofile);
	for (i = 0; i < 3; i++)
	{
		f = LittleFloat (cl.viewangles[i]);
		fwrite (&f, 4, 1, cls.demofile);
	}
	fwrite (net_message.data, net_message.cursize, 1, cls.demofile);
	fflush (cls.demofile);
}

void PushFrameposEntry(long fbaz) // woods #demorewind (Baker Fitzquake Mark V)
{
	framepos_t* newf;

	newf = malloc(sizeof(framepos_t)); // Demo rewind
	newf->baz = fbaz;

	if (!dem_framepos)
	{
		newf->next = NULL;
		start_of_demo = false;
	}
	else
	{
		newf->next = dem_framepos;
	}
	dem_framepos = newf;
}

static void EraseTopEntry(void) // woods #demorewind (Baker Fitzquake Mark V)
{
	framepos_t* top;

	top = dem_framepos;
	dem_framepos = dem_framepos->next;
	free(top);
}

static int CL_GetDemoMessage (void)
{
	int	r, i;
	float	f;

	if (cls.demopaused)
		return 0;

	if (start_of_demo && cls.demorewind) // woods #demorewind (Baker Fitzquake Mark V)
		return 0;

	if (cls.signon < SIGNONS)	// clear stuffs if new demo 
		while (dem_framepos)
			EraseTopEntry(); // end woods #demorewind (Baker Fitzquake Mark V)

	// decide if it is time to grab the next message
	if (cls.signon == SIGNONS)	// always grab until fully connected
	{
		if (cls.timedemo)
		{
			if (host_framecount == cls.td_lastframe)
				return 0;	// already read this frame's message
			cls.td_lastframe = host_framecount;
		// if this is the second frame, grab the real td_starttime
		// so the bogus time on the first frame doesn't count
			if (host_framecount == cls.td_startframe + 1)
				cls.td_starttime = realtime;
		}

		else if (!cls.demorewind && cl.ctime <= cl.mtime[0]) // woods #demorewind (Baker Fitzquake Mark V)
			return 0;		// don't need another message yet
		else if (cls.demorewind && cl.ctime >= cl.mtime[0])
			return 0;

		// joe: fill in the stack of frames' positions
		// enable on intermission or not...?
		// NOTE: it can't handle fixed intermission views!
		if (!cls.demorewind /*&& !cl.intermission*/)
			PushFrameposEntry(ftell(cls.demofile)); 

	//	else if (/* cl.time > 0 && */ cl.time <= cl.mtime[0])
	//	{
	//		return 0;	// don't need another message yet
	//	} // end woods #demorewind (Baker Fitzquake Mark V)
	}

// get the next message

	cls.demo_offset_current = ftell(cls.demofile); // woods #demopercent (Baker Fitzquake Mark V)

	fread (&net_message.cursize, 4, 1, cls.demofile);
	VectorCopy (cl.mviewangles[0], cl.mviewangles[1]);
	for (i = 0 ; i < 3 ; i++)
	{
		r = fread (&f, 4, 1, cls.demofile);
		cl.mviewangles[0][i] = LittleFloat (f);
	}

	net_message.cursize = LittleLong (net_message.cursize);
	if (net_message.cursize > MAX_MSGLEN)
		Sys_Error ("Demo message > MAX_MSGLEN");
	r = fread (net_message.data, net_message.cursize, 1, cls.demofile);
	if (r != 1)
	{
		CL_StopPlayback ();
		return 0;
	}

	// woods #demorewind (Baker Fitzquake Mark V)
	// joe: get out framestack's top entry
	if (cls.demorewind /*&& !cl.intermission*/)
	{
		if (dem_framepos/* && dem_framepos->baz*/)	// Baker: in theory, if this occurs we ARE at the start of the demo with demo rewind on
		{
			fseek(cls.demofile, dem_framepos->baz, SEEK_SET);
			EraseTopEntry(); // Baker: we might be able to improve this better but not right now.
		}
		if (!dem_framepos)
			bumper_on = start_of_demo = true;
	}

	return 1;
}

/*
====================
CL_GetMessage

Handles recording and playback of demos, on top of NET_ code
====================
*/
int CL_GetMessage (void)
{
	int	r;

	if (cls.demoplayback)
		return CL_GetDemoMessage ();

	while (1)
	{
		r = NET_GetMessage (cls.netcon);

		if (r != 1 && r != 2)
			return r;

	// discard nop keepalive message
		if (net_message.cursize == 1 && net_message.data[0] == svc_nop)
			Con_Printf ("<-- server to client keepalive\n");
		else
			break;
	}

	if (cls.demorecording)
		CL_WriteDemoMessage ();

	return r;
}


/*
====================
CL_Stop_f

stop recording a demo
====================
*/
void CL_Stop_f (void)
{
	if (cmd_source != src_command)
		return;

	if (!cls.demorecording)
	{
		Con_Printf ("Not recording a demo.\n");
		return;
	}

// write a disconnect message to the demo file
	SZ_Clear (&net_message);
	MSG_WriteByte (&net_message, svc_disconnect);
	CL_WriteDemoMessage ();

// finish up
	fclose (cls.demofile);
	cls.demofile = NULL;
	cls.demorecording = false;
	Con_Printf ("completed demo\n");

	Cvar_SetROM(cl_recordingdemo.name, "");
	
// ericw -- update demo tab-completion list
	DemoList_Rebuild ();
}

static void CL_Record_Serverdata(void)
{
	size_t i;
	MSG_WriteByte(&net_message, svc_serverinfo);
	if (cl.protocol_pext2)
	{
		MSG_WriteLong (&net_message, PROTOCOL_FTE_PEXT2);
		MSG_WriteLong (&net_message, cl.protocol_pext2);
	}
	MSG_WriteLong (&net_message, cl.protocol);
	if (cl.protocol == PROTOCOL_RMQ)
		MSG_WriteLong (&net_message, cl.protocolflags);
	if (cl.protocol_pext2 & PEXT2_PREDINFO)
		MSG_WriteString(&net_message, COM_SkipPath(com_gamedir));
	MSG_WriteByte (&net_message, cl.maxclients);
	MSG_WriteByte (&net_message, cl.gametype);
	MSG_WriteString (&net_message, cl.levelname);
	for (i=1; cl.model_precache[i]; i++)
		MSG_WriteString (&net_message, cl.model_precache[i]->name);
	MSG_WriteByte (&net_message, 0);
	for (i=1; cl.sound_precache[i]; i++)	//FIXME: might not send any if nosound is set
		MSG_WriteString (&net_message, cl.sound_precache[i]->name);
	MSG_WriteByte (&net_message, 0);
	//FIXME: cd track (current rather than initial?)
	//FIXME: initial view entity (for clients that don't want to mess up scoreboards)
	MSG_WriteByte (&net_message, svc_signonnum);
	MSG_WriteByte (&net_message, 1);
	CL_WriteDemoMessage();
	SZ_Clear (&net_message);
}

//spins out a baseline(idx>=0) or static entity(idx<0) into net_message
void CL_Record_Prespawn(void)
{
	int idx, i;

	//baselines
	for (idx = 0; idx < cl.num_entities; idx++)
	{
		entity_state_t *state = &cl.entities[idx].baseline;
		if (!memcmp(state, &nullentitystate, sizeof(entity_state_t)))
			continue;	//no need
		MSG_WriteStaticOrBaseLine(&net_message, idx, state, cl.protocol_pext2, cl.protocol, cl.protocolflags);

		if (net_message.cursize > 4096)
		{	//periodically flush so that large maps don't need larger than vanilla limits
			CL_WriteDemoMessage();
			SZ_Clear (&net_message);
		}
	}

	//static ents
	for (idx = 1; idx < cl.num_statics; idx++)
	{
		MSG_WriteStaticOrBaseLine(&net_message, -1, &cl.static_entities[idx]->baseline, cl.protocol_pext2, cl.protocol, cl.protocolflags);

		if (net_message.cursize > 4096)
		{	//periodically flush so that large maps don't need larger than vanilla limits
			CL_WriteDemoMessage();
			SZ_Clear (&net_message);
		}
	}

	//static sounds
	for (i = NUM_AMBIENTS; i < total_channels; i++)
	{
		channel_t	*ss = &snd_channels[i];
		sfxcache_t		*sc;

		if (!ss->sfx)
			continue;
		if (ss->entnum || ss->entchannel)
			continue;	//can't have been a static sound
		sc = S_LoadSound(ss->sfx);
		if (!sc || sc->loopstart == -1)
			continue;	//can't have been a (valid) static sound

		for (idx = 1; idx < MAX_SOUNDS && cl.sound_precache[idx]; idx++)
			if (cl.sound_precache[idx] == ss->sfx)
				break;
		if (idx == MAX_SOUNDS)
			continue;	//can't figure out which sound it was

		MSG_WriteByte(&net_message, (idx > 255)?svc_spawnstaticsound2:svc_spawnstaticsound);
		MSG_WriteCoord(&net_message, ss->origin[0], cl.protocolflags);
		MSG_WriteCoord(&net_message, ss->origin[1], cl.protocolflags);
		MSG_WriteCoord(&net_message, ss->origin[2], cl.protocolflags);
		if (idx > 255)
			MSG_WriteShort(&net_message, idx);
		else
			MSG_WriteByte(&net_message, idx);
		MSG_WriteByte(&net_message, ss->master_vol);
		MSG_WriteByte(&net_message, ss->dist_mult*1000*64);

		if (net_message.cursize > 4096)
		{	//periodically flush so that large maps don't need larger than vanilla limits
			CL_WriteDemoMessage();
			SZ_Clear (&net_message);
		}
	}

#ifdef PSET_SCRIPT
	//particleindexes
	for (idx = 0; idx < MAX_PARTICLETYPES; idx++)
	{
		if (!cl.particle_precache[idx].name)
			continue;
		MSG_WriteByte(&net_message, svcdp_precache);
		MSG_WriteShort(&net_message, 0x4000 | idx);
		MSG_WriteString(&net_message, cl.particle_precache[idx].name);

		if (net_message.cursize > 4096)
		{	//periodically flush so that large maps don't need larger than vanilla limits
			CL_WriteDemoMessage();
			SZ_Clear (&net_message);
		}
	}
#endif

	MSG_WriteByte (&net_message, svc_signonnum);
	MSG_WriteByte (&net_message, 2);
	CL_WriteDemoMessage();
	SZ_Clear (&net_message);
}

void CL_Record_Spawn(void)
{
	const char *cmd;
	int i, c;

	// player names, colors, and frag counts
	for (i = 0; i < cl.maxclients; i++)
	{
		MSG_WriteByte (&net_message, svc_updatename);
		MSG_WriteByte (&net_message, i);
		MSG_WriteString (&net_message, cl.scores[i].name);
		MSG_WriteByte (&net_message, svc_updatefrags);
		MSG_WriteByte (&net_message, i);
		MSG_WriteShort (&net_message, cl.scores[i].frags);
		MSG_WriteByte (&net_message, svc_updatecolors);
		MSG_WriteByte (&net_message, i);
		c = 0;
		if (cl.scores[i].shirt.type == 1)
			c |= (cl.scores[i].shirt.rgb[0]<<4)&0xf;
		if (cl.scores[i].pants.type == 1)
			c |= (cl.scores[i].pants.rgb[0]<<0)&0xf;
		MSG_WriteByte (&net_message, c);
	}

	// send all current light styles
	for (i = 0; i < MAX_LIGHTSTYLES; i++)
	{
		if (*cl_lightstyle[i].map)
		{
			MSG_WriteByte (&net_message, svc_lightstyle);
			MSG_WriteByte (&net_message, i);
			MSG_WriteString (&net_message, cl_lightstyle[i].map);
		}

		if (net_message.cursize > 4096)
		{	//periodically flush so that large maps don't need larger than vanilla limits
			CL_WriteDemoMessage();
			SZ_Clear (&net_message);
		}
	}

	// what about the current CD track... future consideration.

	//if this mod is using dynamic fog, make sure we start with the right values.
	cmd = Fog_GetFogCommand();
	if (cmd)
	{
		MSG_WriteByte (&net_message, svc_stufftext);
		MSG_WriteString (&net_message, cmd);
	}

	//stats
	for (i = 0; i < MAX_CL_STATS; i++)
	{
		if (!cl.stats[i] && !cl.statsf[i])
			continue;

		if (net_message.cursize > 4096)
		{	//periodically flush so that large maps don't need larger than vanilla limits
			CL_WriteDemoMessage();
			SZ_Clear (&net_message);
		}

		if ((double)cl.stats[i] != cl.statsf[i] && (unsigned int)cl.stats[i] <= 0x00ffffff)
		{	//if the float representation seems to have more precision then use that, unless its getting huge in which case we're probably getting fpu truncation, so go back to more compatible ints
			MSG_WriteByte (&net_message, svcfte_updatestatfloat);
			MSG_WriteByte (&net_message, i);
			MSG_WriteFloat (&net_message, cl.statsf[i]);
		}
		else if (cl.stats[i] >= 0 && cl.stats[i] <= 255 && (cl.protocol_pext2 & PEXT2_PREDINFO))
		{
			MSG_WriteByte (&net_message, svcdp_updatestatbyte);
			MSG_WriteByte (&net_message, i);
			MSG_WriteByte (&net_message, cl.stats[i]);
		}
		else
		{
			MSG_WriteByte (&net_message, svc_updatestat);
			MSG_WriteByte (&net_message, i);
			MSG_WriteLong (&net_message, cl.stats[i]);
		}
	}

	// view entity
	MSG_WriteByte (&net_message, svc_setview);
	MSG_WriteShort (&net_message, cl.viewentity);

	// signon
	MSG_WriteByte (&net_message, svc_signonnum);
	MSG_WriteByte (&net_message, 3);

	CL_WriteDemoMessage();
	SZ_Clear (&net_message);

	//ask the server to reset entity deltas. yes this means playback will wait a couple of frames before it actually starts playing but oh well.
	if (cl.protocol_pext2 & PEXT2_REPLACEMENTDELTAS)
	{
		cl.ackframes_count = 0;
		cl.ackframes[cl.ackframes_count++] = -1;
	}
}

/*
====================
CL_Record_f -- -- woods changed alot: removed cd track, and map selections (just use autodemo) | sourced from Qrack #autodemo

record <demoname> <map> [cd track]
====================
*/
void CL_Record_f (void)
{
	int		c;
	char	name[MAX_OSPATH];
	int		track;

	if (cmd_source != src_command)
		return;

	if (cls.demoplayback)
	{
		Con_Printf ("Can't record during demo playback\n");
		return;
	}

	c = Cmd_Argc();
	if (c > 3)
	{
		Con_Printf("record or record <demoname> [<map>]\n");
		return;
	}

	if (c == 1 || c == 2)
	{
		if (c == 1)
		{
			// woods added time for demo output
			char str[24];
			time_t systime = time(0);
			struct tm loct =*localtime(&systime);

			q_snprintf(name, sizeof(name), "%s/demos", com_gamedir); //  create demos folder if not there
			Sys_mkdir(name); 

			strftime(str, 24, "%m-%d-%Y-%H%M%S", &loct);
			q_snprintf(name, sizeof(name), "%s/demos/%s_%s", com_gamedir, cl.mapname, str);  // woods added demos folder, added args for demo output info
		}
		else if (c == 2)
		{
			if (strstr(Cmd_Argv(1), ".."))
			{
				Con_Printf("Relative pathnames are not allowed.\n");
				return;
			}

			if (c == 2 && cls.state == ca_connected)
			{
#if 0
				Con_Printf("Can not record - already connected to server\nClient demo recording must be started before connecting\n");
				return;
#endif
				if (cls.signon < 2)
				{
					Con_Printf("Can't record - try again when connected\n");
					return;
				}
				switch (cl.protocol)
				{
				case PROTOCOL_NETQUAKE:
				case PROTOCOL_FITZQUAKE:
				case PROTOCOL_RMQ:
				case PROTOCOL_VERSION_BJP3:
					break;
					//case PROTOCOL_VERSION_NEHD:
					//case PROTOCOL_VERSION_DP5:
					//case PROTOCOL_VERSION_DP6:
				case PROTOCOL_VERSION_DP7:
					//case PROTOCOL_VERSION_BJP1:
					//case PROTOCOL_VERSION_BJP2:
				default:
					Con_Printf("Can not record - protocol not supported for recording mid-map\nClient demo recording must be started before connecting\n");
					return;
				}
			}

		}
	}

	if (cls.demorecording)
		CL_Stop_f();

	// write the forced cd track number, or -1
	if (c == 4)
	{
		track = atoi(Cmd_Argv(3));
		Con_Printf("Forcing CD track to %i\n", cls.forcetrack);
	}
	else
	{
		track = -1;
	}

	if (c == 2)
	{
		q_snprintf(name, sizeof(name), "%s/demos", com_gamedir); //  create demos folder if not there
		Sys_mkdir(name);
		q_snprintf(name, sizeof(name), "%s/demos/%s", com_gamedir, Cmd_Argv(1));  // added demos

	}

	// start the map up
	if (c > 2)
	{
		//Cmd_ExecuteString(va("map %s", Cmd_Argv(2)), src_command);
		//if (cls.state != ca_connected)
			//return;

		Con_Printf("enable autodemo to record at map start\n");
		return;

	}

// open the demo file
	COM_AddExtension (name, ".dem", sizeof(name));

	Cvar_SetROM(cl_recordingdemo.name, name);

	Con_Printf ("demo recording\n");
	cls.demofile = fopen (name, "wb");
	if (!cls.demofile)
	{
		Con_Printf ("ERROR: couldn't create %s\n", name);
		Cvar_SetROM(cl_recordingdemo.name, "");
		return;
	}

	cls.forcetrack = track;
	fprintf (cls.demofile, "%i\n", cls.forcetrack);

	cls.demorecording = true;

	// from ProQuake: initialize the demo file if we're already connected
	if (c < 3 && cls.state == ca_connected)
	{
		byte *data = net_message.data;
		int cursize = net_message.cursize;
		byte weirdaltbufferthatprobablyisntneeded[NET_MAXMESSAGE];

		net_message.data = weirdaltbufferthatprobablyisntneeded;
		SZ_Clear (&net_message);

		CL_Record_Serverdata();
		CL_Record_Prespawn();
		CL_Record_Spawn();

		// restore net_message
		net_message.data = data;
		net_message.cursize = cursize;
	}
}


/*
====================
CL_PlayDemo_f

play [demoname]
====================
*/
void CL_PlayDemo_f (void)
{
	char	name[MAX_OSPATH], name2[MAX_OSPATH]; // woods #demosfolder

	if (cmd_source != src_command)
		return;

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("playdemo <demoname> : plays a demo\n");
		return;
	}

// disconnect from server
	CL_Disconnect (); // woods #demorewind (Baker Fitzquake Mark V)

	// Revert
	cls.demorewind = false;
	cls.demospeed = 0; // 0 = Don't use
	bumper_on = false;

// open the demo file
	q_strlcpy (name, Cmd_Argv(1), sizeof(name));
	COM_AddExtension (name, ".dem", sizeof(name));

	q_snprintf(name2, sizeof(name2), "demos/%s", name); // woods #demosfolder

	Con_Printf ("Playing demo from %s.\n", name2); // woods #demosfolder

	COM_FOpenFile (name2, &cls.demofile, NULL); // check demos folder

	if (!cls.demofile)
		COM_FOpenFile(name, &cls.demofile, NULL); // check gamedir too

	if (!cls.demofile)
	{
		Con_Printf ("ERROR: couldn't open %s\n", name); // woods #demosfolder
		cls.demonum = -1;	// stop demo loop
		return;
	}

	// woods #demopercent (Baker Fitzquake Mark V)

	strcpy(cls.demoname, name); 
	cls.demo_offset_start = ftell(cls.demofile);	// qfs_lastload.offset instead?
	cls.demo_file_length = com_filesize;
	cls.demo_hosttime_start = cls.demo_hosttime_elapsed = 0; // Fill this in ... host_time;
	cls.demo_cltime_start = cls.demo_cltime_elapsed = 0; // Fill this in

	// end #demopercent (Baker Fitzquake Mark V)
	// 
// ZOID, fscanf is evil
// O.S.: if a space character e.g. 0x20 (' ') follows '\n',
// fscanf skips that byte too and screws up further reads.
//	fscanf (cls.demofile, "%i\n", &cls.forcetrack);
	if (fscanf (cls.demofile, "%i", &cls.forcetrack) != 1 || fgetc (cls.demofile) != '\n')
	{
		fclose (cls.demofile);
		cls.demofile = NULL;
		cls.demonum = -1;	// stop demo loop
		Con_Printf ("ERROR: demo \"%s\" is invalid\n", name);
		return;
	}

	cls.demoplayback = true;
	cls.demopaused = false;
	cls.state = ca_connected;

// get rid of the menu and/or console
	key_dest = key_game;
}

/*
====================
CL_FinishTimeDemo

====================
*/
static void CL_FinishTimeDemo (void)
{
	int	frames;
	float	time;

	cls.timedemo = false;

// the first frame didn't count
	frames = (host_framecount - cls.td_startframe) - 1;
	time = realtime - cls.td_starttime;
	if (!time)
		time = 1;
	Con_Printf ("%i frames %5.1f seconds %5.1f fps\n", frames, time, frames/time);
}

/*
====================
CL_TimeDemo_f

timedemo [demoname]
====================
*/
void CL_TimeDemo_f (void)
{
	if (cmd_source != src_command)
		return;

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("timedemo <demoname> : gets demo speeds\n");
		return;
	}

	CL_PlayDemo_f ();
	if (!cls.demofile)
		return;

// cls.td_starttime will be grabbed at the second frame of the demo, so
// all the loading time doesn't get counted

	cls.timedemo = true;
	cls.td_startframe = host_framecount;
	cls.td_lastframe = -1;	// get a new message this frame
}

