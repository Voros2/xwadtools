/*
 *	This is the multigen program to compile the state tables.
 *	It was written by John Carmack, id Software.
 *
 *	Copyright (C) 1994 by id Software.
 *
 *	I have modified this program to get it integrated into the
 *	tool collection and to generate state tables, which compile
 *	ok together with XDoom. Some changes were necessary because
 *	this state compiler was abandoned unfortuately, and someone
 *	hacked the generated files manualy, instead of using this
 *	compiler :( The sources were published by id Software to
 *	the public, so it wasn't necessary to break this.
 *
 *	Please don't bother John Carmack or anyone at id Software
 *	about this. I'm sure this is working fine by doing compares
 *	between the generated sources and those included in the
 *	public released sources.
 *
 *	24 June 1998, Udo Munk	um@compuserve.com
 */

#define VERSION "1.1"


/*
=============================================================================

						DOOM STATESCR

						by John Carmack

=============================================================================
*/


#include "strfunc.h"
#include "cmdlib.h"
#include "scriplib.h"


typedef struct
{
	long	sprite;
	long	frame;
	long	tics;
	long	action;
	long	nextstate;
	long	misc1, misc2;
} state_t;

#define MAXSTATES	2048
#define	MAXTHINKS	256

char	spritename[MAXSTATES][5];
int	numsprites;
char	actionname[MAXTHINKS][32];
int	numactions;


char	statename[MAXSTATES][32];
char	nextname[MAXSTATES][32];
state_t	states[MAXSTATES];
int	numstates;

state_t	*st;



#define		MAXTYPES	256
#define		MAXINFO		64


typedef struct
{
	char	*name;
	char	*base;
} info_t;

info_t	baseinfo[MAXINFO];

char	typename[MAXTYPES][32];
char	*info[MAXTYPES][MAXINFO];
int	numtypes;
int	numinfo;
int	nummisc;


/*
===============
=
= ParseState
=
===============
*/

void ParseState (void)
{
	int j;
	
	for (j=0 ; j<numstates ; j++)
		if (!strcmpi(token,statename[j]))
			Error ("line %i: duplicate state name (%s)",scriptline,token);
	strcpy (statename[numstates],token);

	GetToken (false);		// sprite
	for (j=0 ; j<numsprites ; j++)
		if (!strcmpi(token,spritename[j]))
			break;
	if (j==numsprites)
	{
		numsprites++;
		strcpy (spritename[j], token);
	}
	st->sprite = j;

	GetToken (false);		// frame
	st->frame = toupper(token[0])-'A';
	if (token[1] == '*')
	{
		st->frame |= 0x8000;	// full bright flag
		if (strlen (token) != 2)
			Error ("line %i: bad frame",scriptline);
	}
	else if (strlen (token) != 1)
		Error ("line %i: bad frame",scriptline);

	GetToken (false);		// tics
	st->tics = atoi (token);

	GetToken (false);		// action
	for (j=0 ; j<numactions ; j++)
		if (!strcmpi(token,actionname[j]))
			break;
	if (j==numactions)
	{
		numactions++;
		strcpy (actionname[j], token);
	}
	st->action = j;

	GetToken (false);		// next
	strcpy (nextname[numstates],token);	// resolved after reading all states

	if (TokenAvailable ())
	{
		GetToken (false);	// misc1
		st->misc1 = atoi (token);
		if (TokenAvailable ())
		{
			GetToken (false);	// misc2
			st->misc2 = atoi (token);
		}
	}

	numstates++;
	st++;

}


/*
===============
=
= ParseInfo
=
===============
*/

void ParseInfo (void)
{
	int i;
	
	if (token[0] == '$')
	{
		GetToken (false);
		if (token[0] == '+')
		{
			sprintf (typename[numtypes],"MT_MISC%i",nummisc);
			nummisc++;
		}
		else
			strcpy (typename[numtypes], token);
		numtypes++;
		return;
	}
	if (!numtypes)
		Error ("A type mnust be defined before declaring info");
	// find which field name
	for (i=0 ; i<numinfo ; i++)
		if (!strcmp (token, baseinfo[i].name))
			break;
	if (i==numinfo)
		Error ("Unknown info type %s",token);
	GetToken (false);
	info[numtypes-1][i] = malloc(strlen(token)+1);
	strcpy (info[numtypes-1][i], token);

}


/*
===================
=
= main
=
===================
*/

int main (void)
{
	FILE	*stenum;
	int	i, j;

	printf ("\nMULTIGEN "VERSION" by John Carmack, copyright (c) 1994 Id Software\n");
	
	printf ("processing multigen.txt\n");
	LoadScriptFile ("multigen.txt");


//
// init state definitions
//
	strcpy (actionname[0], "NULL");
	numactions = 1;
	numstates = numsprites = 0;
	st = states;

//
// parse info defaults
//
	numtypes = 0;

	GetToken (true);
	if (token[0] != '$')
		Error ("first command must be $ DEFAULT");
	GetToken (false);
	if (strcmp (token,"DEFAULT"))
		Error ("first command must be $ DEFAULT");
	numinfo = 0;
	do
	{
		GetToken (true);
		if (endofscript)
			Error ("end of script in defaults");
		if (token[0] == '$' || (token[0] == 'S' && token[1] == '_') )
		{
			UnGetToken ();
			break;
		}
		baseinfo[numinfo].name = malloc (strlen(token)+1);
		strcpy (baseinfo[numinfo].name, token);
		GetToken (false);
		baseinfo[numinfo].base = malloc (strlen(token)+1);
		strcpy (baseinfo[numinfo].base, token);
		numinfo++;
	} while (1);
		
	
//
// parse state and info definitions
//
	do
	{
		GetToken (true);
		if (endofscript)
			break;
		if (token[0] == 'S' && token[1] == '_')
			ParseState ();
		else
			ParseInfo ();	
	} while (1);

//
// resolve next state numbers
//
	for (i=0 ; i<numstates ; i++)
	{
		for (j=0 ; j<numstates ; j++)
			if (!strcmpi(nextname[i],statename[j]))
				break;
		if (j==numstates)
			printf ("Unresolved nextstate: %s\n",nextname[i]);
		states[i].nextstate = j;
	}


//=============================================
//
// write info.h file
//
//=============================================

	stenum = fopen ("info.h","w");
	fprintf (stenum, "// generated by multigen, do not edit!\n\n");
	fprintf (stenum, "#ifndef __INFO__\n");
	fprintf (stenum, "#define __INFO__\n\n");
	fprintf (stenum, "// Needed for action function pointer handling\n");
	fprintf (stenum, "#include \"d_think.h\"\n\n");

//
// write sprite names
//
	fprintf (stenum,"typedef enum {\n");
	for (i=0 ; i<numsprites ; i++)
		fprintf (stenum,"SPR_%s,\n",spritename[i]);
	fprintf (stenum,"NUMSPRITES\n} spritenum_t;\n\n");

//
// write state names
//
	fprintf (stenum,"typedef enum {\n");
	for (i=0 ; i<numstates ; i++)
		fprintf (stenum,"%s,\n",statename[i]);
	fprintf (stenum,"NUMSTATES\n} statenum_t;\n\n");

	fprintf (stenum, "typedef struct\n"
					 "{\n"
					 "	 spritenum_t	sprite;\n"
					 "	 long		frame;\n"
					 "	 long		tics;\n"
					 "	 actionf_t	action;\n"
					 "	 statenum_t	nextstate;\n"
					 "	 long		misc1, misc2;\n"
					 "} state_t;\n\n");

	fprintf (stenum, "extern state_t	states[NUMSTATES];\n");
	fprintf (stenum, "extern char	*sprnames[NUMSPRITES];\n\n");
	
//
// write info def
//
	fprintf (stenum, "\n\ntypedef enum {\n");
	for (i=0 ; i<numtypes ; i++)
		fprintf (stenum,"%s,\n",typename[i]);
	fprintf (stenum, "NUMMOBJTYPES} mobjtype_t;\n\n");
	fprintf (stenum, "typedef struct {\n");
	for (j=0 ; j<numinfo ; j++)
	{
		if ( !strncmp(baseinfo[j].name,"str_",4) )
			fprintf (stenum, "	char *%s;\n", baseinfo[j].name );
		else if (!strcmp(baseinfo[j].name, "flags"))
			fprintf (stenum, "	long %s;\n", baseinfo[j].name );
		else
			fprintf (stenum, "	int  %s;\n", baseinfo[j].name );
	}
	fprintf (stenum, "} mobjinfo_t;\n\n");
	fprintf (stenum, "extern mobjinfo_t mobjinfo[NUMMOBJTYPES];\n\n");
	
	fprintf (stenum, "#endif\n");

	fclose (stenum);

//===========================================
//
// write states.c file
//
//===========================================
	stenum = fopen ("info.c","w");
	fprintf (stenum, "// generated by multigen, do not edit!\n\n");
	fprintf (stenum, "#include \"sounds.h\"\n");
	fprintf (stenum, "#include \"m_fixed.h\"\n\n");
	fprintf (stenum, "#ifdef __GNUG__\n");
	fprintf (stenum, "#pragma implementation \"info.h\"\n");
	fprintf (stenum, "#endif\n\n");
	fprintf (stenum, "#include \"info.h\"\n");
	fprintf (stenum, "#include \"p_mobj.h\"\n\n");

//
// write sprite names for initial sprite loading
//
	fprintf (stenum, "char *sprnames[NUMSPRITES] = {\n");
	for (i=0 ; i<numsprites ; i++)
	{
		fprintf (stenum, "\"%s\"", spritename[i]);
		if (i != numsprites-1)
		{
			fprintf (stenum, ",");
			if (i%10 == 9)
				fprintf (stenum, "\n");
		}
	}
	fprintf (stenum, "\n};\n\n");

//
// write action names
//
	for (i=1 ; i<numactions ; i++)	// skip NULL at 0
		fprintf (stenum,"void %s ();\n", actionname[i]);
	fprintf (stenum,"\n");

//
// write state structures
//
	fprintf (stenum, "state_t	states[NUMSTATES] = {\n");
	for (i=0, st=states ; i<numstates ; i++,st++)
	{
		fprintf (stenum, "{SPR_%s,%li,%li,{%s},%s,%li,%li}",
		spritename[st->sprite], st->frame,
		st->tics, actionname[st->action], statename[st->nextstate],
		st->misc1, st->misc2);
		if (i != numstates-1)
			fprintf (stenum, ",");
		fprintf (stenum, "\t// %s\n",statename[i]);
	}
	fprintf (stenum, "};\n\n");

//
// write info declarations
//
	fprintf (stenum, "\nmobjinfo_t mobjinfo[NUMMOBJTYPES] = {\n");
	for (i=0 ; i<numtypes ; i++)
	{
		fprintf (stenum,"\n{		// %s\n", typename[i]);
		for (j=0 ; j<numinfo ; j++)
		{
			if (info[i][j])
				fprintf (stenum, "%s", info[i][j] );
			else
				fprintf (stenum, "%s", baseinfo[j].base );
			
			if (j != numinfo-1)
				fprintf (stenum, ",");
			fprintf (stenum,"		// %s\n", baseinfo[j].name);
		}
		fprintf (stenum, " }");
		if (i != numtypes-1)
			fprintf (stenum, ",");
		fprintf (stenum, "\n");
	}
	fprintf (stenum, "};\n\n");
	
	fclose (stenum);


	printf ("%u states parsed\n",numstates);
	printf ("%i types parsed\n", numtypes);

	return 0;
}
