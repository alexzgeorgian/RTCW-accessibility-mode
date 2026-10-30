/*
 * Linked with the game module's own object files, as the Makefile built
 * them for the Mac: the item and weapon-name lookups the campaign stopped
 * on there, run on script lines of our own. No game data is needed.
 */
#include "g_local.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

void dllEntry( intptr_t ( QDECL *syscallptr )( intptr_t arg, ... ) );
extern animStringItem_t weaponStrings[WP_NUM_WEAPONS];
void BG_InitWeaponStrings( void );

static intptr_t QDECL fake_syscall( intptr_t arg, ... ) {
	va_list ap;
	const char *text;

	va_start( ap, arg );
	text = va_arg( ap, const char * );
	va_end( ap );
	if ( arg == G_PRINT ) {
		printf( "PRINT: %s", text );
	} else if ( arg == G_ERROR ) {
		printf( "G_ERROR: %s\n", text );
		exit( 3 );
	}
	return 0;
}

static int snd( const char *n ) { (void)n; return 0; }
static animModelInfo_t info;
static animScriptData_t data;

int main( void ) {
	static const char *weapons[] = { "SP5 Pistol", "luger", "colt", "mp40", "thompson", "sten", "flamethrower", NULL };
	static const char *items[] = { "weapon_luger", "weapon_mp40", "weapon_knife", "weapon_mauserrifle", "Luger", NULL };
	char script[] =
		"// a script of our own, in the shape of the game's\r\n"
		"DEFINES\r\n"
		"{\r\n"
		"set weapons pistols = SP5 Pistol AND luger AND colt\r\n"
		"set weapons machineguns = mp40 AND thompson AND sten AND flamethrower \r\n"
		"}\r\n";
	int i, failed = 0;

	dllEntry( fake_syscall );

	BG_InitWeaponStrings();
	for ( i = 0; weapons[i]; i++ ) {
		char token[64];
		int index;

		Q_strncpyz( token, weapons[i], sizeof( token ) );
		index = BG_IndexForString( token, weaponStrings, qtrue );
		printf( "weapon %-12s -> %d\n", weapons[i], index );
		failed |= index < 0;
	}
	for ( i = 0; items[i]; i++ ) {
		gitem_t *item = BG_FindItem2( items[i] );

		printf( "item %-20s -> %s\n", items[i], item ? item->classname : "NULL" );
		failed |= !item;
	}
	printf( "Med Health -> %s\n", BG_FindItem( "Med Health" ) ? "found" : "NULL" );
	printf( "Q_stricmp(Luger, luger) = %d, Q_strcasecmp(weapon_luger, WEAPON_LUGER) = %d\n",
			Q_stricmp( "Luger", "luger" ), Q_strcasecmp( "weapon_luger", "WEAPON_LUGER" ) );

	memset( &data, 0, sizeof( data ) );
	data.modelInfo[0] = &info;
	data.clientModels[0] = 1;
	data.soundIndex = snd;
	BG_AnimParseAnimScript( &info, &data, 0, "test.script", script );
	printf( "script parsed; %s\n", failed ? "SOME LOOKUPS FAILED" : "all lookups found" );
	return failed;
}
