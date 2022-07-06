//------------
// locations.c
//------------
// Original format by JP Grossman for Proquake -- woods / rook #pqteam

#include "quakedef.h"

#define VectorSet(v, x, y, z) ((v)[0] = (x), (v)[1] = (y), (v)[2] = (z))
#define VectorClear(a) ((a)[0] = (a)[1] = (a)[2] = 0)

location_t	*locations, *temploc;

extern	cvar_t	r_drawlocs;

void LOC_PQ_Init (void) // woods added PQ to name #pqteam
{
	temploc = Z_Malloc(sizeof(location_t));
	locations = Z_Malloc(sizeof(location_t));
}

void LOC_Delete(location_t* loc)
{
	location_t** ptr, ** next;
	for (ptr = &locations; *ptr; ptr = next)
	{
		next = &(*ptr)->next_loc;
		if (*ptr == loc)
		{
			*ptr = loc->next_loc;
			Z_Free(loc);
		}
	}
}

void LOC_Clear_f (void)
{
	location_t *l;
	for (l = locations;l;l = l->next_loc)
	{
		LOC_Delete(l);
	}
}

void LOC_SetLoc (vec3_t mins, vec3_t maxs, char *name)
{
	location_t *l, **ptr;
	int namelen;
	float	temp;
	int		n;
	vec_t	sum;

	if (!(name))
		return;

	namelen = strlen(name);
	l = Z_Malloc(sizeof(location_t));

	sum = 0;
	for (n = 0 ; n < 3 ; n++)
	{
		if (mins[n] > maxs[n])
		{
			temp = mins[n];
			mins[n] = maxs[n];
			maxs[n] = temp;
		}
		sum += maxs[n] - mins[n];
	}
	
	l->sum = sum;

	VectorSet(l->mins, mins[0], mins[1], mins[2]-32);	//ProQuake Locs are extended +/- 32 units on the z-plane...
	VectorSet(l->maxs, maxs[0], maxs[1], maxs[2]+32);
	q_snprintf (l->name, namelen + 1, name);	
	Con_DPrintf("Location %s assigned.\n", name);
	for (ptr = &locations;*ptr;ptr = &(*ptr)->next_loc);
		*ptr = l;
}

qboolean LOC_LoadLocations (void)
{
	char	filename[MAX_OSPATH];
	vec3_t	mins, maxs;	
	char	name[32];

	int		i, linenumber, limit, len;
	char	*filedata, *text, *textend, *linestart, *linetext, *lineend;
	int		filesize;

	if (cls.state != ca_connected || !cl.worldmodel)
	{
		Con_Printf("!LOC_LoadLocations ERROR: No map loaded!\n");
		return false;
	}

	//LOC_Init();

	LOC_Clear_f();

	q_snprintf(filename, sizeof(filename), "locs/%s.loc", cl.mapname);

	if (COM_FileExists(filename, NULL) == false)
	{
		//Con_Printf("%s could not be found.\n", filename);
		return false;
	}

	filedata = (char*)COM_LoadTempFile(filename, NULL);

	if (!filedata)
	{
		Con_Printf("%s contains empty or corrupt data.\n", filename);
		return false;
	}

	filesize = strlen(filedata);

	text = filedata;
	textend = filedata + filesize;
	for (linenumber = 1;text < textend;linenumber++)
	{
		linestart = text;
		for (;text < textend && *text != '\r' && *text != '\n';text++)
			;
		lineend = text;
		if (text + 1 < textend && *text == '\r' && text[1] == '\n')
			text++;
		if (text < textend)
			text++;
		// trim trailing whitespace
		while (lineend > linestart && lineend[-1] <= ' ')
			lineend--;
		// trim leading whitespace
		while (linestart < lineend && *linestart <= ' ')
			linestart++;
		// check if this is a comment
		if (linestart + 2 <= lineend && !strncmp(linestart, "//", 2))
			continue;
		linetext = linestart;
		limit = 3;
		for (i = 0;i < limit;i++)
		{
			if (linetext >= lineend)
				break;
			// note: a missing number is interpreted as 0
			if (i < 3)
				mins[i] = atof(linetext);
			else
				maxs[i - 3] = atof(linetext);
			// now advance past the number
			while (linetext < lineend && *linetext > ' ' && *linetext != ',')
				linetext++;
			// advance through whitespace
			if (linetext < lineend)
			{
				if (*linetext == ',')
				{
					linetext++;
					limit = 6;
					// note: comma can be followed by whitespace
				}
				if (*linetext <= ' ')
				{
					// skip whitespace
					while (linetext < lineend && *linetext <= ' ')
						linetext++;
				}
			}
		}
		// if this is a quoted name, remove the quotes
		if (i == 6)
		{
			if (linetext >= lineend || *linetext != '"')
				continue; // proquake location names are always quoted
			lineend--;
			linetext++;
			len = q_min(lineend - linetext, (int)sizeof(name) - 1);
			memcpy(name, linetext, len);
			name[len] = 0;
			
			LOC_SetLoc(mins, maxs, name);// add the box to the list
		}
		else
			continue;
	}
	return true;
}


char *LOC_GetLocation (vec3_t p)
{
	location_t *loc;
	location_t *bestloc;
	float dist, bestdist;

	bestloc = NULL;

	bestdist = 999999;

	for (loc = locations;loc;loc = loc->next_loc)
	{
		dist =	fabs(loc->mins[0] - p[0]) + fabs(loc->maxs[0] - p[0]) + 
				fabs(loc->mins[1] - p[1]) + fabs(loc->maxs[1] - p[1]) +
				fabs(loc->mins[2] - p[2]) + fabs(loc->maxs[2] - p[2]) - loc->sum;

		if (dist < .01)
		{
			return loc->name;
		}

		if (dist < bestdist)
		{
			bestdist = dist;
			bestloc = loc;
		}
	}
	
	if (bestloc)
	{
		return bestloc->name;
	}
	return "no .loc";
}
