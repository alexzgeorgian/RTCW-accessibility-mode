/*
 * Parses a character's wolfanim.cfg and wolfanim.script with RTCW's own
 * reader (bg_animation.c), the way the game does on loading a level, and
 * says whether it got through. Built against iortcw's source on each
 * platform, so a platform where the game fails to read RTCW's own files
 * shows it here without the game.
 *
 *   anim_script_check <wolfanim.cfg> <wolfanim.script>
 */
#include "q_shared.h"
#include "bg_public.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

vmCvar_t g_gametype;

void QDECL Com_Error( int level, const char *fmt, ... ) {
	va_list args;

	va_start( args, fmt );
	printf( "ERROR: " );
	vprintf( fmt, args );
	printf( "\n" );
	va_end( args );
	exit( 1 );
}

void QDECL Com_Printf( const char *fmt, ... ) {
	va_list args;

	va_start( args, fmt );
	vprintf( fmt, args );
	va_end( args );
}

static char text[100000];
static animModelInfo_t modelInfo;
static animScriptData_t scriptData;

static void Load( const char *path ) {
	FILE *f = fopen( path, "rb" );
	size_t n;

	if ( !f ) {
		printf( "cannot open %s\n", path );
		exit( 2 );
	}
	n = fread( text, 1, sizeof( text ) - 1, f );
	text[n] = 0;
	fclose( f );
}

int main( int argc, char **argv ) {
	if ( argc != 3 ) {
		printf( "usage: anim_script_check <wolfanim.cfg> <wolfanim.script>\n" );
		return 2;
	}
	scriptData.modelInfo[0] = &modelInfo;
	scriptData.clientModels[0] = 1;
	Load( argv[1] );
	BG_AnimParseAnimConfig( &modelInfo, argv[1], text );
	Load( argv[2] );
	BG_AnimParseAnimScript( &modelInfo, &scriptData, 0, argv[2], text );
	printf( "parsed %s and %s\n", argv[1], argv[2] );
	return 0;
}
