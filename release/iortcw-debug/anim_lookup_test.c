/*
 * Looks up the weapon names a player model's animation script lists, the
 * way the game does as it loads one, and parses a script line naming
 * them. Built and run on a Mac by .github/workflows/iortcw-mac-debug.yml,
 * where the campaign stopped with "BG_IndexForString: unknown token".
 */
#include "q_shared.h"
#include "bg_public.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct { int handle; int modificationCount; float value; int integer; char string[256]; } stubCvar_t;
stubCvar_t g_gametype;

void QDECL Com_Error( int level, const char *error, ... ) {
	va_list a;
	va_start( a, error );
	printf( "COM_ERROR: " );
	vprintf( error, a );
	printf( "\n" );
	va_end( a );
	exit( 1 );
}

void QDECL Com_Printf( const char *msg, ... ) {
	va_list a;
	va_start( a, msg );
	vprintf( msg, a );
	va_end( a );
}

extern animStringItem_t weaponStrings[WP_NUM_WEAPONS];
void BG_InitWeaponStrings( void );

static int snd( const char *n ) { (void)n; return 0; }

static animModelInfo_t info;
static animScriptData_t data;

int main( void ) {
	static const char *names[] = { "SP5 Pistol", "luger", "colt", "mp40", "thompson", "sten",
								   "flamethrower", "mauser rifle", "snooper rifle", NULL };
	char script[] =
		"// a script of our own, in the shape of the game's\r\n"
		"DEFINES\r\n"
		"{\r\n"
		"set weapons pistols = SP5 Pistol AND luger AND colt\r\n"
		"set weapons machineguns = mp40 AND thompson AND sten AND flamethrower \r\n"
		"set weapons rifles = mauser rifle AND snooper rifle\r\n"
		"}\r\n";
	int i, failed = 0;

	printf( "sizeof(long)=%d sizeof(int)=%d char signed=%d\n", (int)sizeof( long ), (int)sizeof( int ), (char)0xff < 0 );

	BG_InitWeaponStrings();
	for ( i = 0; i < WP_NUM_WEAPONS; i++ ) {
		printf( "weapon %2d: %-20s hash %d\n", i, weaponStrings[i].string ? weaponStrings[i].string : "(null)", weaponStrings[i].hash );
	}
	for ( i = 0; names[i]; i++ ) {
		char token[64];
		int index;

		Q_strncpyz( token, names[i], sizeof( token ) );
		index = BG_IndexForString( token, weaponStrings, qtrue );
		printf( "lookup %-16s -> %d\n", names[i], index );
		if ( index < 0 ) {
			failed = 1;
		}
	}

	memset( &data, 0, sizeof( data ) );
	data.modelInfo[0] = &info;
	data.clientModels[0] = 1;
	data.soundIndex = snd;
	BG_AnimParseAnimScript( &info, &data, 0, "test.script", script );
	printf( "script parsed\n" );

	return failed;
}
