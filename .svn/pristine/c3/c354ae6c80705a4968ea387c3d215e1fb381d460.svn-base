/** Copyright (C) 2006, Ian Paul Larsen.
 **
 **  This program is free software; you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation; either version 2 of the License, or
 **  (at your option) any later version.
 **
 **  This program is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License along
 **  with this program; if not, write to the Free Software Foundation, Inc.,
 **  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 **/

/** For statements without arguments you NEED to create rules like {stmt}":" that push back the
 ** colon.  this is so that if the stmt with a : and no space between them is not interperted
 ** as a label- flex is greedy and will match the stmt: first
 **/
 
%{
#include "basicParse.tab.h"
#include "../CompileErrors.h"
#include "../WordCodes.h"


#ifdef __cplusplus
	extern "C" {
#endif

#include <string.h>
#include <stdlib.h>


extern int numifs;
extern void clearLabelTable();
extern void clearIfTable();
extern int testIfOnTable(int);
extern int testIfOnTableError(int);
extern void newLineOffsets(int);

extern int getSymbol(char *);
extern void addIntOp(int, int);
extern void addOp(int);
extern void addString(char*);
extern void addStringOp(int, char*);


extern int yyparse();
//extern YYLTYPE yylloc;
extern int errorcode;
extern int wordOffset;
extern int functionDefSymbol;
extern int subroutineDefSymbol;

// arrays to return warnings from compiler
// defined in basicParse.y and also used in interperter.cpp
extern int parsewarningtable[];
extern int parsewarningtablesourceline[];
extern int parsewarningtablecolumn[];
extern char *parsewarningcolumnlexingfilename[];
extern int numparsewarnings;

extern int numargs; //need to be set to 0 when basicParse() start otherwise after a COMPILER_ERROR, the next rul will use the last value

int linenumber;
int column;
char *lexingfilename;

#define MAX_INCLUDE_DEPTH 10
YY_BUFFER_STATE include_stack[MAX_INCLUDE_DEPTH];
int include_linenumber[MAX_INCLUDE_DEPTH];
int include_filenumber[MAX_INCLUDE_DEPTH];
int numincludes=0;
#define MAX_INCLUDE_FILES 100
char* include_filenames[MAX_INCLUDE_FILES];
int include_filenames_counter=0;
char* include_exec_path;
int filenumber=0;
char *filename;
#ifdef __cplusplus
}
#endif

void count();
void unputcolon();

%}

%x INCLUDE
%x INCLUDE_FILE

constinteger ([0-9]{1,9}|1[0-9]{9}|2(0[0-9]{8}|1([0-3][0-9]{7}|4([0-6][0-9]{6}|7([0-3][0-9]{5}|4([0-7][0-9]{4}|8([0-2][0-9]{3}|3([0-5][0-9]{2}|6([0-3][0-9]|4[0-7])))))))))
constdecimal [0-9]*\.?[0-9]+([eE][-+]?[0-9]+)?
constbinary 0[bB][01]+
consthex 0[xX][0-9a-fA-F]+
constoctal 0[oO][0-7]+
conststring (\"[^\"\n]*\")|(\'[^\'\n]*\')
constfalse [Ff][Aa][Ll][Ss][Ee]
consttrue [Tt][Rr][Uu][Ee]


blankline ^[ \t]*[\n]
nws [^ \t\n]+
variable [a-zA-Z][a-zA-Z0-9_\$]*
whitespace [ \t]+


abs [Aa][Bb][Ss]
acos [Aa][Cc][Oo][Ss]
alert [Aa][Ll][Ee][Rr][Tt]
and [Aa][Nn][Dd]
arc [Aa][Rr][Cc]
asc [Aa][Ss][Cc]
arraybase [Aa][Rr][Rr][Aa][Yy][ \t]*[Bb][Aa][Ss][Ee]
asin [Aa][Ss][Ii][Nn]
assigned [Aa][Ss][Ss][Ii][Gg][Nn][Ee][Dd]
atan [Aa][Tt][Aa][Nn]
badinclude [Ii][Nn][Cc][Ll][Uu][Dd][Ee]
begincase [Bb][Ee][Gg][Ii][Nn][ \t]*[Cc][Aa][Ss][Ee]
black [Bb][Ll][Aa][Cc][Kk]
blue [Bb][Ll][Uu][Ee]
booleof [Ee][Oo][Ff]
call [Cc][Aa][Ll][Ll]
case [Cc][Aa][Ss][Ee]
catch [Cc][Aa][Tt][Cc][Hh]
ceil [Cc][Ee][Ii][Ll]
changedir [Cc][Hh][Aa][Nn][Gg][Ee][Dd][Ii][Rr]
chord [Cc][Hh][Oo][Rr][Dd]
chr [Cc][Hh][Rr]
circle [Cc][Ii][Rr][Cc][Ll][Ee]
clear [Cc][Ll][Ee][Aa][Rr]
clg [Cc][Ll][Gg]
clickb [Cc][Ll][Ii][Cc][Kk][Bb]
clickclear [Cc][Ll][Ii][Cc][Kk][Cc][Ll][Ee][Aa][Rr]
clickx [Cc][Ll][Ii][Cc][Kk][Xx]
clicky [Cc][Ll][Ii][Cc][Kk][Yy]
close [Cc][Ll][Oo][Ss][Ee]
cls [Cc][Ll][Ss]
color [Cc][Oo][Ll][Oo][Uu]?[Rr]
confirm [Cc][Oo][Nn][Ff][Ii][Rr][Mm]
continuedo [Cc][Oo][Nn][Tt][Ii][Nn][Uu][Ee][ \t]*[Dd][Oo]
continuefor [Cc][Oo][Nn][Tt][Ii][Nn][Uu][Ee][ \t]*[Ff][Oo][Rr]
continuewhile [Cc][Oo][Nn][Tt][Ii][Nn][Uu][Ee][ \t]*[Ww][Hh][Ii][Ll][Ee]
cos [Cc][Oo][Ss]
count [Cc][Oo][Uu][Nn][Tt]
countx [Cc][Oo][Uu][Nn][Tt][Xx]
currentdir [Cc][Uu][Rr][Rr][Ee][Nn][Tt][Dd][Ii][Rr]
cyan [Cc][Yy][Aa][Nn]
dark [Dd][Aa][Rr][Kk]
day [Dd][Aa][Yy]
dbclose [Dd][Bb][Cc][Ll][Oo][Ss][Ee]
dbcloseset [Dd][Bb][Cc][Ll][Oo][Ss][Ee][Ss][Ee][Tt]
dbexecute [Dd][Bb][Ee][Xx][Ee][Cc][Uu][Tt][Ee]
dbfloat [Dd][Bb][Ff][Ll][Oo][Aa][Tt]
dbint [Dd][Bb][Ii][Nn][Tt]
dbnull [Dd][Bb][Nn][Uu][Ll][Ll]
dbopen [Dd][Bb][Oo][Pp][Ee][Nn]
dbopenset [Dd][Bb][Oo][Pp][Ee][Nn][Ss][Ee][Tt]
dbrow [Dd][Bb][Rr][Oo][Ww]
dbstring [Dd][Bb][Ss][Tt][Rr][Ii][Nn][Gg]
debuginfo [Dd][Ee][Bb][Uu][Gg][Ii][Nn][Ff][Oo]
degrees [Dd][Ee][Gg][Rr][Ee][Ee][Ss]
dim [Dd][Ii][Mm]
dir [Dd][Ii][Rr]
do [Dd][Oo]
editvisible [Ee][Dd][Ii][Tt][Vv][Ii][Ss][Ii][Bb][Ll][Ee]
ellipse [Ee][Ll][Ll][Ii][Pp][Ss][Ee]
else [Ee][Ll][Ss][Ee]
end [Ee][Nn][Dd]
endcase [Ee][Nn][Dd][ \t]*[Cc][Aa][Ss][Ee]
endfunction [Ee][Nn][Dd][ \t]*[Ff][Uu][Nn][Cc][Tt][Ii][Oo][Nn]
endif [Ee][Nn][Dd][ \t]*[Ii][Ff]
endsubroutine [Ee][Nn][Dd][ \t]*[Ss][Uu][Bb][Rr][Oo][Uu][Tt][Ii][Nn][Ee]
endtry [Ee][Nn][Dd][ \t]*[Tt][Rr][Yy]
endwhile [Ee][Nn][Dd][ \t]*[Ww][Hh][Ii][Ll][Ee]
error_argumentcount [Ee][Rr][Rr][Oo][Rr][_][Aa][Rr][Gg][Uu][Mm][Ee][Nn][Tt][Cc][Oo][Uu][Nn][Tt]
error_arrayelement [Ee][Rr][Rr][Oo][Rr][_][Aa][Rr][Rr][Aa][Yy][Ee][Ll][Ee][Mm][Ee][Nn][Tt]
error_arrayeven [Ee][Rr][Rr][Oo][Rr][_][Aa][Rr][Rr][Aa][Yy][Ee][Vv][Ee][Nn]
error_arrayexpr [Ee][Rr][Rr][Oo][Rr][_][Aa][Rr][Rr][Aa][Yy][Ee][Xx][Pp][Rr]
error_arrayindex [Ee][Rr][Rr][Oo][Rr][_][Aa][Rr][Rr][Aa][Yy][Ii][Nn][Dd][Ee][Xx]
error_arrayindexmissing [Ee][Rr][Rr][Oo][Rr][_][Aa][Rr][Rr][Aa][Yy][Ii][Nn][Dd][Ee][Xx][Mm][Ii][Ss][Ss][Ii][Nn][Gg]
error_arraylength2d [Ee][Rr][Rr][Oo][Rr][_][Aa][Rr][Rr][Aa][Yy][Ll][Ee][Nn][Gg][Tt][Hh][2][Dd]
error_arraynitems [Ee][Rr][Rr][Oo][Rr][_][Aa][Rr][Rr][Aa][Yy][Nn][Ii][Tt][Ee][Mm][Ss]
error_arraysizelarge [Ee][Rr][Rr][Oo][Rr][_][Aa][Rr][Rr][Aa][Yy][Ss][Ii][Zz][Ee][Ll][Aa][Rr][Gg][Ee]
error_arraysizesmall [Ee][Rr][Rr][Oo][Rr][_][Aa][Rr][Rr][Aa][Yy][Ss][Ii][Zz][Ee][Ss][Mm][Aa][Ll][Ll]
error_asinacosrange [Ee][Rr][Rr][Oo][Rr][_][Aa][Ss][Ii][Nn][Aa][Cc][Oo][Ss][Rr][Aa][Nn][Gg][Ee]
error_booleanconv [Ee][Rr][Rr][Oo][Rr][_][Bb][Oo][Oo][Ll][Ee][Aa][Nn][Cc][Oo][Nn][Vv]
error_dbcolno [Ee][Rr][Rr][Oo][Rr][_][Dd][Bb][Cc][Oo][Ll][Nn][Oo]
error_dbconnnumber [Ee][Rr][Rr][Oo][Rr][_][Dd][Bb][Cc][Oo][Nn][Nn][Nn][Uu][Mm][Bb][Ee][Rr]
error_dbnotopen [Ee][Rr][Rr][Oo][Rr][_][Dd][Bb][Nn][Oo][Tt][Oo][Pp][Ee][Nn]
error_dbnotset [Ee][Rr][Rr][Oo][Rr][_][Dd][Bb][Nn][Oo][Tt][Ss][Ee][Tt]
error_dbnotsetrow [Ee][Rr][Rr][Oo][Rr][_][Dd][Bb][Nn][Oo][Tt][Ss][Ee][Tt][Rr][Oo][Ww]
error_dbopen [Ee][Rr][Rr][Oo][Rr][_][Dd][Bb][Oo][Pp][Ee][Nn]
error_dbquery [Ee][Rr][Rr][Oo][Rr][_][Dd][Bb][Qq][Uu][Ee][Rr][Yy]
error_dbsetnumber [Ee][Rr][Rr][Oo][Rr][_][Dd][Bb][Ss][Ee][Tt][Nn][Uu][Mm][Bb][Ee][Rr]
error_divzero [Ee][Rr][Rr][Oo][Rr][_][Dd][Ii][Vv][Zz][Ee][Rr][Oo]
error_download [Ee][Rr][Rr][Oo][Rr][_][Dd][Oo][Ww][Nn][Ll][Oo][Aa][Dd]
error_envelopemax [Ee][Rr][Rr][Oo][Rr][_][Ee][Nn][Vv][Ee][Ll][Oo][Pp][Ee][Mm][Aa][Xx]
error_envelopeodd [Ee][Rr][Rr][Oo][Rr][_][Ee][Nn][Vv][Ee][Ll][Oo][Pp][Ee][Oo][Dd][Dd]
error_expectedarray [Ee][Rr][Rr][Oo][Rr][_][Ee][Xx][Pp][Ee][Cc][Tt][Ee][Dd][Aa][Rr][Rr][Aa][Yy]
error_expectedsound [Ee][Rr][Rr][Oo][Rr][_][Ee][Xx][Pp][Ee][Cc][Tt][Ee][Dd][Ss][Oo][Uu][Nn][Dd]
error_filenotopen [Ee][Rr][Rr][Oo][Rr][_][Ff][Ii][Ll][Ee][Nn][Oo][Tt][Oo][Pp][Ee][Nn]
error_filenumber [Ee][Rr][Rr][Oo][Rr][_][Ff][Ii][Ll][Ee][Nn][Uu][Mm][Bb][Ee][Rr]
error_fileopen [Ee][Rr][Rr][Oo][Rr][_][Ff][Ii][Ll][Ee][Oo][Pp][Ee][Nn]
error_fileoperation [Ee][Rr][Rr][Oo][Rr][_][Ff][Ii][Ll][Ee][Oo][Pp][Ee][Rr][Aa][Tt][Ii][Oo][Nn]
error_filereset [Ee][Rr][Rr][Oo][Rr][_][Ff][Ii][Ll][Ee][Rr][Ee][Ss][Ee][Tt]
error_filewrite [Ee][Rr][Rr][Oo][Rr][_][Ff][Ii][Ll][Ee][Ww][Rr][Ii][Tt][Ee]
error_folder [Ee][Rr][Rr][Oo][Rr][_][Ff][Oo][Ll][Dd][Ee][Rr]
error_freedb [Ee][Rr][Rr][Oo][Rr][_][Ff][Rr][Ee][Ee][Dd][Bb]
error_freedbset [Ee][Rr][Rr][Oo][Rr][_][Ff][Rr][Ee][Ee][Dd][Bb][Ss][Ee][Tt]
error_freefile [Ee][Rr][Rr][Oo][Rr][_][Ff][Rr][Ee][Ee][Ff][Ii][Ll][Ee]
error_freenet [Ee][Rr][Rr][Oo][Rr][_][Ff][Rr][Ee][Ee][Nn][Ee][Tt]
error_harmoniclist [Ee][Rr][Rr][Oo][Rr][_][Hh][Aa][Rr][Mm][Oo][Nn][Ii][Cc][Ll][Ii][Ss][Tt]
error_harmonicnumber [Ee][Rr][Rr][Oo][Rr][_][Hh][Aa][Rr][Mm][Oo][Nn][Ii][Cc][Nn][Uu][Mm][Bb][Ee][Rr]
error_imagefile [Ee][Rr][Rr][Oo][Rr][_][Ii][Mm][Aa][Gg][Ee][Ff][Ii][Ll][Ee]
error_imageresource [Ee][Rr][Rr][Oo][Rr][_][Ii][Mm][Aa][Gg][Ee][Rr][Ee][Ss][Oo][Uu][Rr][Cc][Ee]
error_imagesavetype [Ee][Rr][Rr][Oo][Rr][_][Ii][Mm][Aa][Gg][Ee][Ss][Aa][Vv][Ee][Tt][Yy][Pp][Ee]
error_imagescale [Ee][Rr][Rr][Oo][Rr][_][Ii][Mm][Aa][Gg][Ee][Ss][Cc][Aa][Ll][Ee]
error_infinity [Ee][Rr][Rr][Oo][Rr][_][Ii][Nn][Ff][Ii][Nn][Ii][Tt][Yy]
error_integerrange [Ee][Rr][Rr][Oo][Rr][_][Ii][Nn][Tt][Ee][Gg][Ee][Rr][Rr][Aa][Nn][Gg][Ee]
error_invalidkeyname [Ee][Rr][Rr][Oo][Rr][_][Ii][Nn][Vv][Aa][Ll][Ii][Dd][Kk][Ee][Yy][Nn][Aa][Mm][Ee]
error_invalidprogname [Ee][Rr][Rr][Oo][Rr][_][Ii][Nn][Vv][Aa][Ll][Ii][Dd][Pp][Rr][Oo][Gg][Nn][Aa][Mm][Ee]
error_invalidresource [Ee][Rr][Rr][Oo][Rr][_][Ii][Nn][Vv][Aa][Ll][Ii][Dd][Rr][Ee][Ss][Oo][Uu][Rr][Cc][Ee]
error_logrange [Ee][Rr][Rr][Oo][Rr][_][Ll][Oo][Gg][Rr][Aa][Nn][Gg][Ee]
error_longrange [Ee][Rr][Rr][Oo][Rr][_][Ll][Oo][Nn][Gg][Rr][Aa][Nn][Gg][Ee]
error_maxrecurse [Ee][Rr][Rr][Oo][Rr][_][Mm][Aa][Xx][Rr][Ee][Cc][Uu][Rr][Ss][Ee]
error_netaccept [Ee][Rr][Rr][Oo][Rr][_][Nn][Ee][Tt][Aa][Cc][Cc][Ee][Pp][Tt]
error_netbind [Ee][Rr][Rr][Oo][Rr][_][Nn][Ee][Tt][Bb][Ii][Nn][Dd]
error_netconn [Ee][Rr][Rr][Oo][Rr][_][Nn][Ee][Tt][Cc][Oo][Nn][Nn]
error_nethost [Ee][Rr][Rr][Oo][Rr][_][Nn][Ee][Tt][Hh][Oo][Ss][Tt]
error_netnone [Ee][Rr][Rr][Oo][Rr][_][Nn][Ee][Tt][Nn][Oo][Nn][Ee]
error_netread [Ee][Rr][Rr][Oo][Rr][_][Nn][Ee][Tt][Rr][Ee][Aa][Dd]
error_netsock [Ee][Rr][Rr][Oo][Rr][_][Nn][Ee][Tt][Ss][Oo][Cc][Kk]
error_netsocknumber [Ee][Rr][Rr][Oo][Rr][_][Nn][Ee][Tt][Ss][Oo][Cc][Kk][Nn][Uu][Mm][Bb][Ee][Rr]
error_netsockopt [Ee][Rr][Rr][Oo][Rr][_][Nn][Ee][Tt][Ss][Oo][Cc][Kk][Oo][Pp][Tt]
error_netwrite [Ee][Rr][Rr][Oo][Rr][_][Nn][Ee][Tt][Ww][Rr][Ii][Tt][Ee]
error_nextnofor [Ee][Rr][Rr][Oo][Rr][_][Nn][Ee][Xx][Tt][Nn][Oo][Ff][Oo][Rr]
error_none [Ee][Rr][Rr][Oo][Rr][_][Nn][Oo][Nn][Ee]
error_nosuchfunction [Ee][Rr][Rr][Oo][Rr][_][Nn][Oo][Ss][Uu][Cc][Hh][Ff][Uu][Nn][Cc][Tt][Ii][Oo][Nn]
error_nosuchlabel [Ee][Rr][Rr][Oo][Rr][_][Nn][Oo][Ss][Uu][Cc][Hh][Ll][Aa][Bb][Ee][Ll]
error_nosuchsubroutine [Ee][Rr][Rr][Oo][Rr][_][Nn][Oo][Ss][Uu][Cc][Hh][Ss][Uu][Bb][Rr][Oo][Uu][Tt][Ii][Nn][Ee]
error_notarray [Ee][Rr][Rr][Oo][Rr][_][Nn][Oo][Tt][Aa][Rr][Rr][Aa][Yy]
error_notimplemented [Ee][Rr][Rr][Oo][Rr][_][Nn][Oo][Tt][Ii][Mm][Pp][Ll][Ee][Mm][Ee][Nn][Tt][Ee][Dd]
error_numberconv [Ee][Rr][Rr][Oo][Rr][_][Nn][Uu][Mm][Bb][Ee][Rr][Cc][Oo][Nn][Vv]
error_numberexpr [Ee][Rr][Rr][Oo][Rr][_][Nn][Uu][Mm][Bb][Ee][Rr][Ee][Xx][Pp][Rr]
error_onedimensional [Ee][Rr][Rr][Oo][Rr][_][Oo][Nn][Ee][Dd][Ii][Mm][Ee][Nn][Ss][Ii][Oo][Nn][Aa][Ll]
error_onerrorsub [Ee][Rr][Rr][Oo][Rr][_][Oo][Nn][Ee][Rr][Rr][Oo][Rr][Ss][Uu][Bb]
error_penwidth [Ee][Rr][Rr][Oo][Rr][_][Pp][Ee][Nn][Ww][Ii][Dd][Tt][Hh]
error_permission [Ee][Rr][Rr][Oo][Rr][_][Pp][Ee][Rr][Mm][Ii][Ss][Ss][Ii][Oo][Nn]
error_polypoints [Ee][Rr][Rr][Oo][Rr][_][Pp][Oo][Ll][Yy][Pp][Oo][Ii][Nn][Tt][Ss]
error_printernotoff [Ee][Rr][Rr][Oo][Rr][_][Pp][Rr][Ii][Nn][Tt][Ee][Rr][Nn][Oo][Tt][Oo][Ff][Ff]
error_printernoton [Ee][Rr][Rr][Oo][Rr][_][Pp][Rr][Ii][Nn][Tt][Ee][Rr][Nn][Oo][Tt][Oo][Nn]
error_printeropen [Ee][Rr][Rr][Oo][Rr][_][Pp][Rr][Ii][Nn][Tt][Ee][Rr][Oo][Pp][Ee][Nn]
error_radix [Ee][Rr][Rr][Oo][Rr][_][Rr][Aa][Dd][Ii][Xx]
error_radixstring [Ee][Rr][Rr][Oo][Rr][_][Rr][Aa][Dd][Ii][Xx][Ss][Tt][Rr][Ii][Nn][Gg]
error_refnotassigned [Ee][Rr][Rr][Oo][Rr][_][Rr][Ee][Ff][Nn][Oo][Tt][Aa][Ss][Ss][Ii][Gg][Nn][Ee][Dd]
error_rgb [Ee][Rr][Rr][Oo][Rr][_][Rr][Gg][Bb]
error_serialparameter [Ee][Rr][Rr][Oo][Rr][_][Ss][Ee][Rr][Ii][Aa][Ll][Pp][Aa][Rr][Aa][Mm][Ee][Tt][Ee][Rr]
error_settingmaxkeys [Ee][Rr][Rr][Oo][Rr][_][Ss][Ee][Tt][Tt][Ii][Nn][Gg][Mm][Aa][Xx][Kk][Ee][Yy][Ss]
error_settingmaxlen [Ee][Rr][Rr][Oo][Rr][_][Ss][Ee][Tt][Tt][Ii][Nn][Gg][Mm][Aa][Xx][Ll][Ee][Nn]
error_settingsgetaccess [Ee][Rr][Rr][Oo][Rr][_][Ss][Ee][Tt][Tt][Ii][Nn][Gg][Ss][Gg][Ee][Tt][Aa][Cc][Cc][Ee][Ss][Ss]
error_settingssetaccess [Ee][Rr][Rr][Oo][Rr][_][Ss][Ee][Tt][Tt][Ii][Nn][Gg][Ss][Ss][Ee][Tt][Aa][Cc][Cc][Ee][Ss][Ss]
error_slicesize [Ee][Rr][Rr][Oo][Rr][_][Ss][Ll][Ii][Cc][Ee][Ss][Ii][Zz][Ee]
error_sounderror [Ee][Rr][Rr][Oo][Rr][_][Ss][Oo][Uu][Nn][Dd][Ee][Rr][Rr][Oo][Rr]
error_soundfile [Ee][Rr][Rr][Oo][Rr][_][Ss][Oo][Uu][Nn][Dd][Ff][Ii][Ll][Ee]
error_soundfileformat [Ee][Rr][Rr][Oo][Rr][_][Ss][Oo][Uu][Nn][Dd][Ff][Ii][Ll][Ee][Ff][Oo][Rr][Mm][Aa][Tt]
error_soundlength [Ee][Rr][Rr][Oo][Rr][_][Ss][Oo][Uu][Nn][Dd][Ll][Ee][Nn][Gg][Tt][Hh]
error_soundnotseekable [Ee][Rr][Rr][Oo][Rr][_][Ss][Oo][Uu][Nn][Dd][Nn][Oo][Tt][Ss][Ee][Ee][Kk][Aa][Bb][Ll][Ee]
error_soundresource [Ee][Rr][Rr][Oo][Rr][_][Ss][Oo][Uu][Nn][Dd][Rr][Ee][Ss][Oo][Uu][Rr][Cc][Ee]
error_spritena [Ee][Rr][Rr][Oo][Rr][_][Ss][Pp][Rr][Ii][Tt][Ee][Nn][Aa]
error_spritenumber [Ee][Rr][Rr][Oo][Rr][_][Ss][Pp][Rr][Ii][Tt][Ee][Nn][Uu][Mm][Bb][Ee][Rr]
error_spriteslice [Ee][Rr][Rr][Oo][Rr][_][Ss][Pp][Rr][Ii][Tt][Ee][Ss][Ll][Ii][Cc][Ee]
error_sqrrange [Ee][Rr][Rr][Oo][Rr][_][Ss][Qq][Rr][Rr][Aa][Nn][Gg][Ee]
error_stackunderflow [Ee][Rr][Rr][Oo][Rr][_][Ss][Tt][Aa][Cc][Kk][Uu][Nn][Dd][Ee][Rr][Ff][Ll][Oo][Ww]
error_string2note [Ee][Rr][Rr][Oo][Rr][_][Ss][Tt][Rr][Ii][Nn][Gg][2][Nn][Oo][Tt][Ee]
error_stringconv [Ee][Rr][Rr][Oo][Rr][_][Ss][Tt][Rr][Ii][Nn][Gg][Cc][Oo][Nn][Vv]
error_stringexpr [Ee][Rr][Rr][Oo][Rr][_][Ss][Tt][Rr][Ii][Nn][Gg][Ee][Xx][Pp][Rr]
error_stringmaxlen [Ee][Rr][Rr][Oo][Rr][_][Ss][Tt][Rr][Ii][Nn][Gg][Mm][Aa][Xx][Ll][Ee][Nn]
error_strstart [Ee][Rr][Rr][Oo][Rr][_][Ss][Tt][Rr][Ss][Tt][Aa][Rr][Tt]
error_toomanysounds [Ee][Rr][Rr][Oo][Rr][_][Tt][Oo][Oo][Mm][Aa][Nn][Yy][Ss][Oo][Uu][Nn][Dd][Ss]
error_unexpectedreturn [Ee][Rr][Rr][Oo][Rr][_][Uu][Nn][Ee][Xx][Pp][Ee][Cc][Tt][Ee][Dd][Rr][Ee][Tt][Uu][Rr][Nn]
error_unserializeformat [Ee][Rr][Rr][Oo][Rr][_][Uu][Nn][Ss][Ee][Rr][Ii][Aa][Ll][Ii][Zz][Ee][Ff][Oo][Rr][Mm][Aa][Tt]
error_varcircular [Ee][Rr][Rr][Oo][Rr][_][Vv][Aa][Rr][Cc][Ii][Rr][Cc][Uu][Ll][Aa][Rr]
error_varnotassigned [Ee][Rr][Rr][Oo][Rr][_][Vv][Aa][Rr][Nn][Oo][Tt][Aa][Ss][Ss][Ii][Gg][Nn][Ee][Dd]
error_varnull [Ee][Rr][Rr][Oo][Rr][_][Vv][Aa][Rr][Nn][Uu][Ll][Ll]
error_waveformlogical [Ee][Rr][Rr][Oo][Rr][_][Ww][Aa][Vv][Ee][Ff][Oo][Rr][Mm][Ll][Oo][Gg][Ii][Cc][Aa][Ll]
error_wavobsolete [Ee][Rr][Rr][Oo][Rr][_][Ww][Aa][Vv][Oo][Bb][Ss][Oo][Ll][Ee][Tt][Ee]
exists [Ee][Xx][Ii][Ss][Tt][Ss]
exitdo [Ee][Xx][Ii][Tt][ \t]*[Dd][Oo]
exitfor [Ee][Xx][Ii][Tt][ \t]*[Ff][Oo][Rr]
exitwhile [Ee][Xx][Ii][Tt][ \t]*[Ww][Hh][Ii][Ll][Ee]
exp [Ee][Xx][Pp]
explode [Ee][Xx][Pp][Ll][Oo][Dd][Ee]
explodex [Ee][Xx][Pp][Ll][Oo][Dd][Ee][Xx]
fastgraphics [Ff][Aa][Ss][Tt][Gg][Rr][Aa][Pp][Hh][Ii][Cc][Ss]
fill [Ff][Ii][Ll][Ll]
floor [Ff][Ll][Oo][Oo][Rr]
font [Ff][Oo][Nn][Tt]
for [Ff][Oo][Rr]
foreach [Ff][Oo][Rr][ \t]*[Ee][Aa][Cc][Hh]
freedb [Ff][Rr][Ee][Ee][Dd][Bb]
freedbset [Ff][Rr][Ee][Ee][Dd][Bb][Ss][Ee][Tt]
freefile [Ff][Rr][Ee][Ee][Ff][Ii][Ll][Ee]
freenet [Ff][Rr][Ee][Ee][Nn][Ee][Tt]
frombinary [Ff][Rr][Oo][Mm][Bb][Ii][Nn][Aa][Rr][Yy]
fromhex [Ff][Rr][Oo][Mm][Hh][Ee][Xx]
fromoctal [Ff][Rr][Oo][Mm][Oo][Cc][Tt][Aa][Ll]
fromradix [Ff][Rr][Oo][Mm][Rr][Aa][Dd][Ii][Xx]
function [Ff][Uu][Nn][Cc][Tt][Ii][Oo][Nn]
getarraybase [Gg][Ee][Tt][Aa][Rr][Rr][Aa][Yy][Bb][Aa][Ss][Ee]
getbrushcolor [Gg][Ee][Tt][Bb][Rr][Uu][Ss][Hh][Cc][Oo][Ll][Oo][Rr]
getclipboardimage [Gg][Ee][Tt][Cc][Ll][Ii][Pp][Bb][Oo][Aa][Rr][Dd][Ii][Mm][Aa][Gg][Ee]
getclipboardstring [Gg][Ee][Tt][Cc][Ll][Ii][Pp][Bb][Oo][Aa][Rr][Dd][Ss][Tt][Rr][Ii][Nn][Gg]
getcolor [Gg][Ee][Tt][Cc][Oo][Ll][Oo][Rr]
getpenwidth [Gg][Ee][Tt][Pp][Ee][Nn][Ww][Ii][Dd][Tt][Hh]
getsetting [Gg][Ee][Tt][Ss][Ee][Tt][Tt][Ii][Nn][Gg]
getslice [Gg][Ee][Tt][Ss][Ll][Ii][Cc][Ee]
global [Gg][Ll][Oo][Bb][Aa][Ll]
gosub [Gg][Oo][Ss][Uu][Bb]
goto [Gg][Oo][Tt][Oo]
graphheight [Gg][Rr][Aa][Pp][Hh][Hh][Ee][Ii][Gg][Hh][Tt]
graphsize [Gg][Rr][Aa][Pp][Hh][Ss][Ii][Zz][Ee]
graphvisible [Gg][Rr][Aa][Pp][Hh][Vv][Ii][Ss][Ii][Bb][Ll][Ee]
graphtoolbarvisible [Gg][Rr][Aa][Pp][Hh][Tt][Oo][Oo][Ll][Bb][Aa][Rr][Vv][Ii][Ss][Ii][Bb][Ll][Ee]
graphwidth [Gg][Rr][Aa][Pp][Hh][Ww][Ii][Dd][Tt][Hh]
gray [Gg][Rr][AaEe][Yy]
green [Gg][Rr][Ee][Ee][Nn]
hour [Hh][Oo][Uu][Rr]
if [Ii][Ff]
in [Ii][Nn]
imageautocrop [Ii][Mm][Aa][Gg][Ee][Aa][Uu][Tt][Oo][Cc][Rr][Oo][Pp]
imagecentered [Ii][Mm][Aa][Gg][Ee][Cc][Ee][Nn][Tt][Ee][Rr][Ee][Dd]
imagecopy [Ii][Mm][Aa][Gg][Ee][Cc][Oo][Pp][Yy]
imagecrop [Ii][Mm][Aa][Gg][Ee][Cc][Rr][Oo][Pp]
imagedraw [Ii][Mm][Aa][Gg][Ee][Dd][Rr][Aa][Ww]
imageflip [Ii][Mm][Aa][Gg][Ee][Ff][Ll][Ii][Pp]
imageheight [Ii][Mm][Aa][Gg][Ee][Hh][Ee][Ii][Gg][Hh][Tt]
imageload [Ii][Mm][Aa][Gg][Ee][Ll][Oo][Aa][Dd]
imagenew [Ii][Mm][Aa][Gg][Ee][Nn][Ee][Ww]
imagepixel [Ii][Mm][Aa][Gg][Ee][Pp][Ii][Xx][Ee][Ll]
imageresize [Ii][Mm][Aa][Gg][Ee][Rr][Ee][Ss][Ii][Zz][Ee]
imagerotate [Ii][Mm][Aa][Gg][Ee][Rr][Oo][Tt][Aa][Tt][Ee]
imagesetpixel [Ii][Mm][Aa][Gg][Ee][Ss][Ee][Tt][Pp][Ii][Xx][Ee][Ll]
imagesmooth [Ii][Mm][Aa][Gg][Ee][Ss][Mm][Oo][Oo][Tt][Hh]
imagetransformed [Ii][Mm][Aa][Gg][Ee][Tt][Rr][Aa][Nn][Ss][Ff][Oo][Rr][Mm][Ee][Dd]
imagetype_bmp [Ii][Mm][Aa][Gg][Ee][Tt][Yy][Pp][Ee][_][Bb][Mm][Pp]
imagetype_jpg [Ii][Mm][Aa][Gg][Ee][Tt][Yy][Pp][Ee][_][Jj][Pp][Gg]
imagetype_png [Ii][Mm][Aa][Gg][Ee][Tt][Yy][Pp][Ee][_][Pp][Nn][Gn]
imagewidth [Ii][Mm][Aa][Gg][Ee][Ww][Ii][Dd][Tt][Hh]
imgload [Ii][Mm][Gg][Ll][Oo][Aa][Dd]
imgsave [Ii][Mm][Gg][Ss][Aa][Vv][Ee]
implode [Ii][Mm][Pp][Ll][Oo][Dd][Ee]
include ^[ \t]*[Ii][Nn][Cc][Ll][Uu][Dd][Ee][ \t]*
input [Ii][Nn][Pp][Uu][Tt]
inputfloat [Ii][Nn][Pp][Uu][Tt][ \t]*[Ff][Ll][Oo][Aa][Tt]
inputint [Ii][Nn][Pp][Uu][Tt][ \t]*[iI][nN][tT]([Ee][Gg][Ee][Rr])?
inputstring [Ii][Nn][Pp][Uu][Tt][ \t]*[Ss][Tt][Rr][Ii][Nn][Gg]
instr [Ii][Nn][Ss][Tt][Rr]
instrx [Ii][Nn][Ss][Tt][Rr][Xx]
isnumeric [iI][Ss][Nn][Uu][Mm][Ee][Rr][Ii][Cc]
key [Kk][Ee][Yy]
keypressed [Kk][Ee][Yy][Pp][Rr][Ee][Ss][Ss][Ee][Dd]
kill [Kk][Ii][Ll][Ll]
label ^[ \t]*{variable}:
lasterror [Ll][Aa][Ss][Tt][Ee][Rr][Rr][Oo][Rr]
lasterrorextra [Ll][Aa][Ss][Tt][Ee][Rr][Rr][Oo][Rr][Ee][Xx][Tt][Rr][Aa]
lasterrorline [Ll][Aa][Ss][Tt][Ee][Rr][Rr][Oo][Rr][Ll][Ii][Nn][Ee]
lasterrormessage [Ll][Aa][Ss][Tt][Ee][Rr][Rr][Oo][Rr][Mm][Ee][Ss][Ss][Aa][Gg][Ee]
left [Ll][Ee][Ff][Tt]
length [Ll][Ee][Nn][Gg][Tt][Hh]
let [Ll][Ee][Tt]
line [Ll][Ii][Nn][Ee]
ljust [Ll][Jj][Uu][Ss][Tt]
log [Ll][Oo][Gg]
logten [Ll][Oo][Gg][1][0]
lower [Ll][Oo][Ww][Ee][Rr]
ltrim [Ll][Tt][Rr][Ii][Mm]
maintoolbarvisible [Mm][Aa][Ii][Nn][Tt][Oo][Oo][Ll][Bb][Aa][Rr][Vv][Ii][Ss][Ii][Bb][Ll][Ee]
map [Mm][Aa][Pp]
md5 [Mm][Dd][5]
mid [Mm][Ii][Dd]
midx [Mm][Ii][Dd][Xx]
minute [Mm][Ii][Nn][Uu][Tt][Ee]
mod ([Mm][Oo][Dd])
month [Mm][Oo][Nn][Tt][Hh]
mouseb [Mm][Oo][Uu][Ss][Ee][Bb]
mousebutton_center [Mm][Oo][Uu][Ss][Ee][Bb][Uu][Tt][Tt][Oo][Nn][_][Cc][Ee][Nn][Tt][Ee][Rr]
mousebutton_doubleclick [Mm][Oo][Uu][Ss][Ee][Bb][Uu][Tt][Tt][Oo][Nn][_][Dd][Oo][Uu][Bb][Ll][Ee][Cc][Ll][Ii][Cc][Kk]
mousebutton_left [Mm][Oo][Uu][Ss][Ee][Bb][Uu][Tt][Tt][Oo][Nn][_][Ll][Ee][Ff][Tt]
mousebutton_none [Mm][Oo][Uu][Ss][Ee][Bb][Uu][Tt][Tt][Oo][Nn][_][Nn][Oo][Nn][Ee]
mousebutton_right [Mm][Oo][Uu][Ss][Ee][Bb][Uu][Tt][Tt][Oo][Nn][_][Rr][Ii][Gg][Hh][Tt]
mousex [Mm][Oo][Uu][Ss][Ee][Xx]
mousey [Mm][Oo][Uu][Ss][Ee][Yy]
msec [Mm][Ss][Ee][Cc]
netaddress [Nn][Ee][Tt][Aa][Dd][Dd][Rr][Ee][Ss][Ss]
netclose [Nn][Ee][Tt][Cc][Ll][Oo][Ss][Ee]
netconnect [Nn][Ee][Tt][Cc][Oo][Nn][Nn][Ee][Cc][Tt]
netdata [Nn][Ee][Tt][Dd][Aa][Tt][Aa]
netlisten [Nn][Ee][Tt][Ll][Ii][Ss][Tt][Ee][Nn]
netread [Nn][Ee][Tt][Rr][Ee][Aa][Dd]
netwrite [Nn][Ee][Tt][Ww][Rr][Ii][Tt][Ee]
next [Nn][Ee][Xx][Tt]
not [Nn][Oo][Tt]
offerror [Oo][Ff][Ff][Ee][Rr][Rr][Oo][Rr]
onerror [Oo][Nn][Ee][Rr][Rr][Oo][Rr]
open [Oo][Pp][Ee][Nn]
openb [Oo][Pp][Ee][Nn][Bb]
openserial [Oo][Pp][Ee][Nn][Ss][Ee][Rr][Ii][Aa][Ll]
openfiledialog [Oo][Pp][Ee][Nn][Ff][Ii][Ll][Ee][Dd][Ii][Aa][Ll][Oo][Gg]
or [Oo][Rr]
orange [Oo][Rr][Aa][Nn][Gg][Ee]
ostype [Oo][Ss][Tt][Yy][Pp][Ee]
ostype_android [Oo][Ss][Tt][Yy][Pp][Ee][_][Aa][Nn][Dd][Rr][Oo][Ii][Dd]
ostype_linux [Oo][Ss][Tt][Yy][Pp][Ee][_][Ll][Ii][Nn][Uu][Xx]
ostype_macintosh [Oo][Ss][Tt][Yy][Pp][Ee][_][Mm][Aa][Cc][Ii][Nn][Tt][Oo][Ss][Hh]
ostype_windows [Oo][Ss][Tt][Yy][Pp][Ee][_][Ww][Ii][Nn][Dd][Oo][Ww][Ss]
outputvisible [Oo][Uu][Tt][Pp][Uu][Tt][Vv][Ii][Ss][Ii][Bb][Ll][Ee]
outputtoolbarvisible [Oo][Uu][Tt][Pp][Uu][Tt][Tt][Oo][Oo][Ll][Bb][Aa][Rr][Vv][Ii][Ss][Ii][Bb][Ll][Ee]
pause [Pp][Aa][Uu][Ss][Ee]
penwidth [Pp][Ee][Nn][Ww][Ii][Dd][Tt][Hh]
pi [Pp][Ii]
pie [Pp][Ii][Ee]
pixel [Pp][Ii][Xx][Ee][Ll]
plot [Pp][Ll][Oo][Tt]
poly [Pp][Oo][Ll][Yy]
portin [Pp][Oo][Rr][Tt][Ii][Nn]
portout [Pp][Oo][Rr][Tt][Oo][Uu][Tt]
print ([Pp][Rr][Ii][Nn][Tt])
printercancel [Pp][Rr][Ii][Nn][Tt][Ee][Rr][ \t]*[Cc][Aa][Nn][Cc][Ee][Ll]
printeroff [Pp][Rr][Ii][Nn][Tt][Ee][Rr][ \t]*[Oo][Ff][Ff]
printeron [Pp][Rr][Ii][Nn][Tt][Ee][Rr][ \t]*[Oo][Nn]
printerpage [Pp][Rr][Ii][Nn][Tt][Ee][Rr][ \t]*[Pp][Aa][Gg][Ee]
prompt [Pp][Rr][Oo][Mm][Pp][Tt]
purple [Pp][Uu][Rr][Pp][Ll][Ee]
putslice [Pp][Uu][Tt][Ss][Ll][Ii][Cc][Ee]
radians [Rr][Aa][Dd][Ii][Aa][Nn][Ss]
rand [Rr][Aa][Nn][Dd]
read [Rr][Ee][Aa][Dd]
readbyte [Rr][Ee][Aa][Dd][Bb][Yy][Tt][Ee]
readline [Rr][Ee][Aa][Dd][Ll][Ii][Nn][Ee]
rect [Rr][Ee][Cc][Tt]
red [Rr][Ee][Dd]
redim [Rr][Ee][Dd][Ii][Mm]
ref [Rr][Ee][Ff]
refresh [Rr][Ee][Ff][Rr][Ee][Ss][Hh]
regexminimal [Rr][Ee][Gg][Ee][Xx][Mm][Ii][Nn][Ii][Mm][Aa][Ll]
rem (:[ \t]*)?[Rr][Ee][Mm](([ \t:].+)|([ \t:]*))$
remalt (:[ \t]*)?#.*$
replace [Rr][Ee][Pp][Ll][Aa][Cc][Ee]
replacex [Rr][Ee][Pp][Ll][Aa][Cc][Ee][Xx]
reset [Rr][Ee][Ss][Ee][Tt]
return [Rr][Ee][Tt][Uu][Rr][Nn]
rgb [Rr][Gg][Bb]
right [Rr][Ii][Gg][Hh][Tt]
rjust [Rr][Jj][Uu][Ss][Tt]
round [Rr][Oo][Uu][Nn][Dd]
rtrim [Rr][Tt][Rr][Ii][Mm]
savefiledialog [Ss][Aa][Vv][Ee][Ff][Ii][Ll][Ee][Dd][Ii][Aa][Ll][Oo][Gg]
say [Ss][Aa][Yy]
second [Ss][Ee][Cc][Oo][Nn][Dd]
seed [Ss][Ee][Ee][Dd]
seek [Ss][Ee][Ee][Kk]
serialize [Ss][Ee][Rr][Ii][Aa][Ll][Ii][Zz][Ee]
setclipboardimage [Ss][Ee][Tt][Cc][Ll][Ii][Pp][Bb][Oo][Aa][Rr][Dd][Ii][Mm][Aa][Gg][Ee]
setclipboardstring [Ss][Ee][Tt][Cc][Ll][Ii][Pp][Bb][Oo][Aa][Rr][Dd][Ss][Tt][Rr][Ii][Nn][Gg]
setgraph [Ss][Ee][Tt][Gg][Rr][Aa][Pp][Hh]
setsetting [Ss][Ee][Tt][Ss][Ee][Tt][Tt][Ii][Nn][Gg]
sin [Ss][Ii][Nn]
size [Ss][Ii][Zz][Ee]
slice_all [Ss][Ll][Ii][Cc][Ee]_[Aa][Ll][Ll]
slice_paint [Ss][Ll][Ii][Cc][Ee]_[Pp][Aa][Ii][Nn][Tt]
slice_sprite [Ss][Ll][Ii][Cc][Ee]_[Ss][Pp][Rr][Ii][Tt][Ee]
sound [Ss][Oo][Uu][Nn][Dd]
soundenvelope [Ss][Oo][Uu][Nn][Dd][Ee][Nn][Vv][Ee][Ll][Oo][Pp][Ee]
soundfade [Ss][Oo][Uu][Nn][Dd][Ff][Aa][Dd][Ee]
soundharmonics [Ss][Oo][Uu][Nn][Dd][Hh][Aa][Rr][Mm][Oo][Nn][Ii][Cc][Ss]
soundid [Ss][Oo][Uu][Nn][Dd][Ii][Dd]
soundlength [Ss][Oo][Uu][Nn][Dd][Ll][Ee][Nn][Gg][Tt][Hh]
soundload [Ss][Oo][Uu][Nn][Dd][Ll][Oo][Aa][Dd]
soundloadraw [Ss][Oo][Uu][Nn][Dd][Ll][Oo][Aa][Dd][Rr][Aa][Ww]
soundloop [Ss][Oo][Uu][Nn][Dd][Ll][Oo][Oo][Pp]
soundpause [Ss][Oo][Uu][Nn][Dd][Pp][Aa][Uu][Ss][Ee]
soundplay [Ss][Oo][Uu][Nn][Dd][Pp][Ll][Aa][Yy]
soundplayer [Ss][Oo][Uu][Nn][Dd][Pp][Ll][Aa][Yy][Ee][Rr]
soundplayeroff [Ss][Oo][Uu][Nn][Dd][Pp][Ll][Aa][Yy][Ee][Rr][Oo][Ff][Ff]
soundposition [Ss][Oo][Uu][Nn][Dd][Pp][Oo][Ss][Ii][Tt][Ii][Oo][Nn]
soundsamplerate [Ss][Oo][Uu][Nn][Dd][Ss][Aa][Mm][Pp][Ll][Ee][Rr][Aa][Tt][Ee]
soundseek [Ss][Oo][Uu][Nn][Dd][Ss][Ee][Ee][Kk]
soundstate [Ss][Oo][Uu][Nn][Dd][Ss][Tt][Aa][Tt][Ee]
soundstop [Ss][Oo][Uu][Nn][Dd][Ss][Tt][Oo][Pp]
soundsystem [Ss][Oo][Uu][Nn][Dd][Ss][Yy][Ss][Tt][Ee][Mm]
soundvolume [Ss][Oo][Uu][Nn][Dd][Vv][Oo][Ll][Uu][Mm][Ee]
soundwait [Ss][Oo][Uu][Nn][Dd][Ww][Aa][Ii][Tt]
soundwaveform [Ss][Oo][Uu][Nn][Dd][Ww][Aa][Vv][Ee][Ff][Oo][Rr][Mm]
spritecollide [Ss][Pp][Rr][Ii][Tt][Ee][Cc][Oo][Ll][Ll][Ii][Dd][Ee]
spritedim [Ss][Pp][Rr][Ii][Tt][Ee][Dd][Ii][Mm]
spriteh [Ss][Pp][Rr][Ii][Tt][Ee][Hh]
spritehide [Ss][Pp][Rr][Ii][Tt][Ee][Hh][Ii][Dd][Ee]
spriteload [Ss][Pp][Rr][Ii][Tt][Ee][Ll][Oo][Aa][Dd]
spritemove [Ss][Pp][Rr][Ii][Tt][Ee][Mm][Oo][Vv][Ee]
spriteo [Ss][Pp][Rr][Ii][Tt][Ee][Oo]
spriteplace [Ss][Pp][Rr][Ii][Tt][Ee][Pp][Ll][Aa][Cc][Ee]
spritepoly [Ss][Pp][Rr][Ii][Tt][Ee][Pp][Oo][Ll][Yy]
spriter [Ss][Pp][Rr][Ii][Tt][Ee][Rr]
sprites [Ss][Pp][Rr][Ii][Tt][Ee][Ss]
spriteshow [Ss][Pp][Rr][Ii][Tt][Ee][Ss][Hh][Oo][Ww]
spriteslice [Ss][Pp][Rr][Ii][Tt][Ee][Ss][Ll][Ii][Cc][Ee]
spritev [Ss][Pp][Rr][Ii][Tt][Ee][Vv]
spritew [Ss][Pp][Rr][Ii][Tt][Ee][Ww]
spritex [Ss][Pp][Rr][Ii][Tt][Ee][Xx]
spritey [Ss][Pp][Rr][Ii][Tt][Ee][Yy]
sqr [Ss][Qq][Rr]
sqrt [Ss][Qq][Rr][Tt]
stamp [Ss][Tt][Aa][Mm][Pp]
step [Ss][Tt][Ee][Pp]
subroutine [Ss][Uu][Bb][Rr][Oo][Uu][Tt][Ii][Nn][Ee]
system [Ss][Yy][Ss][Tt][Ee][Mm]
tan [Tt][Aa][Nn]
text [Tt][Ee][Xx][Tt]
textheight [Tt][Ee][Xx][Tt][Hh][Ee][Ii][Gg][Hh][Tt]
textwidth [Tt][Ee][Xx][Tt][Ww][Ii][Dd][Tt][Hh]
then [Tt][Hh][Ee][Nn]
throwerror [Tt][Hh][Rr][Oo][Ww][Ee][Rr][Rr][Oo][Rr]
to [Tt][Oo]
tobinary [Tt][Oo][Bb][Ii][Nn][Aa][Rr][Yy]
tofloat [Ff][Ll][Oo][Aa][Tt]
tohex [Tt][Oo][Hh][Ee][Xx]
toint [Ii][Nn][Tt]
tooctal [Tt][Oo][Oo][Cc][Tt][Aa][Ll]
toradix [Tt][Oo][Rr][Aa][Dd][Ii][Xx]
tostring [Ss][Tt][Rr][Ii][Nn][Gg]
trim [Tt][Rr][Ii][Mm]
try [Tt][Rr][Yy]
type_array [Tt][Yy][Pp][Ee][_][Aa][Rr][Rr][Aa][Yy]
type_float [Tt][Yy][Pp][Ee][_][Ff][Ll][Oo][Aa][Tt]
type_int [Tt][Yy][Pp][Ee][_][Ii][Nn][Tt]([Ee][Gg][Ee][Rr])*
type_map [Tt][Yy][Pp][Ee][_][Mm][Aa][Pp]
type_ref [Tt][Yy][Pp][Ee][_][Rr][Ee][Ff]
type_string [Tt][Yy][Pp][Ee][_][Ss][Tt][Rr][Ii][Nn][Gg]
type_unassigned [Tt][Yy][Pp][Ee][_][Uu][Nn][Aa][Ss][Ss][Ii][Gg][Nn][Ee][Dd]
typeof [Tt][Yy][Pp][Ee][Oo][Ff]
unassign [Uu][Nn][Aa][Ss][Ss][Ii][Gg][Nn]
unload [Uu][Nn][Ll][Oo][Aa][Dd]
unserialize [Uu][Nn][Ss][Ee][Rr][Ii][Aa][Ll][Ii][Zz][Ee]
until [Uu][Nn][Tt][Ii][Ll]
upper [Uu][Pp][Pp][Ee][Rr]
variablewatch [Vv][Aa][Rr][Ii][Aa][Bb][Ll][Ee][Ww][Aa][Tt][Cc][Hh]
version [Vv][Ee][Rr][Ss][Ii][Oo][Nn]
volume [Vv][Oo][Ll][Uu][Mm][Ee]
warning_arrayelement [Ww][Aa][Rr][Nn][Ii][Nn][Gg][_][Aa][Rr][Rr][Aa][Yy][Ee][Ll][Ee][Mm][Ee][Nn][Tt]
warning_booleanconv [Ww][Aa][Rr][Nn][Ii][Nn][Gg][_][Bb][Oo][Oo][Ll][Ee][Aa][Nn][Cc][Oo][Nn][Vv]
warning_integerrange [Ww][Aa][Rr][Nn][Ii][Nn][Gg][_][Ii][Nn][Tt][Ee][Gg][Ee][Rr][Rr][Aa][Nn][Gg][Ee]
warning_longrange [Ww][Aa][Rr][Nn][Ii][Nn][Gg][_][Ll][Oo][Nn][Gg][Rr][Aa][Nn][Gg][Ee]
warning_numberconv [Ww][Aa][Rr][Nn][Ii][Nn][Gg][_][Nn][Uu][Mm][Bb][Ee][Rr][Cc][Oo][Nn][Vv]
warning_refnotassigned [Ww][Aa][Rr][Nn][Ii][Nn][Gg][_][Rr][Ee][Ff][Nn][Oo][Tt][Aa][Ss][Ss][Ii][Gg][Nn][Ee][Dd]
warning_sounderror [Ww][Aa][Rr][Nn][Ii][Nn][Gg][_][Ss][Oo][Uu][Nn][Dd][Ee][Rr][Rr][Oo][Rr]
warning_soundfileformat [Ww][Aa][Rr][Nn][Ii][Nn][Gg][_][Ss][Oo][Uu][Nn][Dd][Ff][Ii][Ll][Ee][Ff][Oo][Rr][Mm][Aa][Tt]
warning_soundlength [Ww][Aa][Rr][Nn][Ii][Nn][Gg][_][Ss][Oo][Uu][Nn][Dd][Ll][Ee][Nn][Gg][Tt][Hh]
warning_soundnotseekable [Ww][Aa][Rr][Nn][Ii][Nn][Gg][_][Ss][Oo][Uu][Nn][Dd][Nn][Oo][Tt][Ss][Ee][Ee][Kk][Aa][Bb][Ll][Ee]
warning_start [Ww][Aa][Rr][Nn][Ii][Nn][Gg][_][Ss][Tt][Aa][Rr][Tt]
warning_string2note [Ww][Aa][Rr][Nn][Ii][Nn][Gg][_][Ss][Tt][Rr][Ii][Nn][Gg][2][Nn][Oo][Tt][Ee]
warning_stringconv [Ww][Aa][Rr][Nn][Ii][Nn][Gg][_][Ss][Tt][Rr][Ii][Nn][Gg][Cc][Oo][Nn][Vv]
warning_varnotassigned [Ww][Aa][Rr][Nn][Ii][Nn][Gg][_][Vv][Aa][Rr][Nn][Oo][Tt][Aa][Ss][Ss][Ii][Gg][Nn][Ee][Dd]
warning_wavobsolete [Ww][Aa][Rr][Nn][Ii][Nn][Gg][_][Ww][Aa][Vv][Oo][Bb][Ss][Oo][Ll][Ee][Tt][Ee]
wavlength [Ww][Aa][Vv][Ll][Ee][Nn][Gg][Tt][Hh]
wavpause [Ww][Aa][Vv][Pp][Aa][Uu][Ss][Ee]
wavplay [Ww][Aa][Vv][Pp][Ll][Aa][Yy]
wavpos [Ww][Aa][Vv][Pp][Oo][Ss]
wavseek [Ww][Aa][Vv][Ss][Ee][Ee][Kk]
wavstate [Ww][Aa][Vv][Ss][Tt][Aa][Tt][Ee]
wavstop [Ww][Aa][Vv][Ss][Tt][Oo][Pp]
wavwait [Ww][Aa][Vv][Ww][Aa][Ii][Tt]
while [Ww][Hh][Ii][Ll][Ee]
white [Ww][Hh][Ii][Tt][Ee]
write [Ww][Rr][Ii][Tt][Ee]
writebyte [Ww][Rr][Ii][Tt][Ee][Bb][Yy][Tt][Ee]
writeline [Ww][Rr][Ii][Tt][Ee][Ll][Ii][Nn][Ee]
xor [Xx][Oo][Rr]
year [Yy][Ee][Aa][Rr]
yellow [Yy][Ee][Ll][Ll][Oo][Ww]
zfill [Zz][Ff][Ii][Ll][Ll]
%%

 /**********************************************************************/
 /* data type rules                                                    */
 /**********************************************************************/

{constfalse}				{ count(); return B256BOOLFALSE; }
{consttrue}					{ count(); return B256BOOLTRUE; }
{constinteger}				{ count(); yylval.number = atoi(yytext); return B256INTEGER; }
{constdecimal}				{ count(); yylval.floatnum = atof(yytext); return B256FLOAT; }
{constbinary}				{
								count();
								yylval.string = strdup(yytext + 2);
								return B256BINCONST;
}
{consthex}				{
								count();
								yylval.string = strdup(yytext + 2);
								return B256HEXCONST;
}
{constoctal}				{
								count();
								yylval.string = strdup(yytext + 2);
								return B256OCTCONST;
}
{conststring}				{
								int len;
								count();
								len = strlen(yytext);
								yylval.string = strdup(yytext + 1);
								yylval.string[len - 2] = 0;
								return B256STRING;
}

 /**********************************************************************/
 /* compile time rules */
 /**********************************************************************/

{include}   {
            count();
            BEGIN(INCLUDE);
}
{badinclude} {
            //include command must be alone in line
            count();
            errorcode = COMPERR_INCLUDENOTALONE;
            return -1;
            //printf("include command must be alone in line %i\n", column);
}

 /**********************************************************************/
 /* functions and constants rules                                      */
 /**********************************************************************/

{abs}						{ count(); return B256ABS; }
{acos}						{ count(); return B256ACOS; }
{arc}						{ count(); return B256ARC; }
{arraybase}					{ count(); return B256ARRAYBASE; }
{asc}						{ count(); return B256ASC; }
{assigned}					{ count(); return B256ASSIGNED; }
{asin}						{ count(); return B256ASIN; }
{atan}						{ count(); return B256ATAN; }
{black}						{ count(); return B256BLACK; }
{blue}						{ count(); return B256BLUE; }
{booleof}					{ count(); return B256BOOLEOF; }
{ceil}						{ count(); return B256CEIL; }
{chr}						{ count(); return B256CHR; }
{clear}						{ count(); return B256CLEAR; }
{clickb}					{ count(); return B256CLICKB; }
{clickx}					{ count(); return B256CLICKX; }
{clicky}					{ count(); return B256CLICKY; }
{confirm}					{ count(); return B256CONFIRM; }
{cos}						{ count(); return B256COS; }
{countx}					{ count(); return B256COUNTX; }
{count}						{ count(); return B256COUNT; }
{currentdir}				{ count(); return B256CURRENTDIR; }
{cyan}						{ count(); return B256CYAN; }
{dark}{blue}				{ count(); return B256DARKBLUE; }
{dark}{cyan}				{ count(); return B256DARKCYAN; }
{dark}{gray}				{ count(); return B256DARKGREY; }
{dark}{green}				{ count(); return B256DARKGREEN; }
{dark}{orange}				{ count(); return B256DARKORANGE; }
{dark}{purple}				{ count(); return B256DARKPURPLE; }
{dark}{red}					{ count(); return B256DARKRED; }
{dark}{yellow}				{ count(); return B256DARKYELLOW; }
{day}						{ count(); return B256DAY; }
{dbfloat}					{ count(); return B256DBFLOAT; }
{dbint}						{ count(); return B256DBINT; }
{dbnull}					{ count(); return B256DBNULL; }
{dbrow}						{ count(); return B256DBROW; }
{dbstring}					{ count(); return B256DBSTRING; }
{debuginfo}					{ count(); return B256DEBUGINFO; }
{degrees}					{ count(); return B256DEGREES; }
{dir}						{ count(); return B256DIR; }
{error_argumentcount}			{ count(); return B256ERROR_ARGUMENTCOUNT; }
{error_arrayelement}			{ count(); return B256ERROR_ARRAYELEMENT; }
{error_arrayeven}			{ count(); return B256ERROR_ARRAYEVEN; }
{error_arrayexpr}			{ count(); return B256ERROR_ARRAYEXPR; }
{error_arrayindexmissing}			{ count(); return B256ERROR_ARRAYINDEXMISSING; }
{error_arrayindex}			{ count(); return B256ERROR_ARRAYINDEX; }
{error_arraylength2d}			{ count(); return B256ERROR_ARRAYLENGTH2D; }
{error_arraynitems}			{ count(); return B256ERROR_ARRAYNITEMS; }
{error_arraysizelarge}			{ count(); return B256ERROR_ARRAYSIZELARGE; }
{error_arraysizesmall}			{ count(); return B256ERROR_ARRAYSIZESMALL; }
{error_asinacosrange}			{ count(); return B256ERROR_ASINACOSRANGE; }
{error_booleanconv}			{ count(); return B256ERROR_BOOLEANCONV; }
{error_dbcolno}			{ count(); return B256ERROR_DBCOLNO; }
{error_dbconnnumber}			{ count(); return B256ERROR_DBCONNNUMBER; }
{error_dbnotopen}			{ count(); return B256ERROR_DBNOTOPEN; }
{error_dbnotsetrow}			{ count(); return B256ERROR_DBNOTSETROW; }
{error_dbnotset}			{ count(); return B256ERROR_DBNOTSET; }
{error_dbopen}			{ count(); return B256ERROR_DBOPEN; }
{error_dbquery}			{ count(); return B256ERROR_DBQUERY; }
{error_dbsetnumber}			{ count(); return B256ERROR_DBSETNUMBER; }
{error_divzero}			{ count(); return B256ERROR_DIVZERO; }
{error_download}			{ count(); return B256ERROR_DOWNLOAD; }
{error_envelopemax}			{ count(); return B256ERROR_ENVELOPEMAX; }
{error_envelopeodd}			{ count(); return B256ERROR_ENVELOPEODD; }
{error_expectedarray}			{ count(); return B256ERROR_EXPECTEDARRAY; }
{error_expectedsound}			{ count(); return B256ERROR_EXPECTEDSOUND; }
{error_filenotopen}			{ count(); return B256ERROR_FILENOTOPEN; }
{error_filenumber}			{ count(); return B256ERROR_FILENUMBER; }
{error_fileopen}			{ count(); return B256ERROR_FILEOPEN; }
{error_fileoperation}			{ count(); return B256ERROR_FILEOPERATION; }
{error_filereset}			{ count(); return B256ERROR_FILERESET; }
{error_filewrite}			{ count(); return B256ERROR_FILEWRITE; }
{error_folder}			{ count(); return B256ERROR_FOLDER; }
{error_freedbset}			{ count(); return B256ERROR_FREEDBSET; }
{error_freedb}			{ count(); return B256ERROR_FREEDB; }
{error_freefile}			{ count(); return B256ERROR_FREEFILE; }
{error_freenet}			{ count(); return B256ERROR_FREENET; }
{error_harmoniclist}			{ count(); return B256ERROR_HARMONICLIST; }
{error_harmonicnumber}			{ count(); return B256ERROR_HARMONICNUMBER; }
{error_imagefile}			{ count(); return B256ERROR_IMAGEFILE; }
{error_imageresource}			{ count(); return B256ERROR_IMAGERESOURCE; }
{error_imagesavetype}			{ count(); return B256ERROR_IMAGESAVETYPE; }
{error_imagescale}			{ count(); return B256ERROR_IMAGESCALE; }
{error_infinity}			{ count(); return B256ERROR_INFINITY; }
{error_integerrange}			{ count(); return B256ERROR_INTEGERRANGE; }
{error_invalidkeyname}			{ count(); return B256ERROR_INVALIDKEYNAME; }
{error_invalidprogname}			{ count(); return B256ERROR_INVALIDPROGNAME; }
{error_invalidresource}			{ count(); return B256ERROR_INVALIDRESOURCE; }
{error_logrange}			{ count(); return B256ERROR_LOGRANGE; }
{error_longrange}			{ count(); return B256ERROR_LONGRANGE; }
{error_maxrecurse}			{ count(); return B256ERROR_MAXRECURSE; }
{error_netaccept}			{ count(); return B256ERROR_NETACCEPT; }
{error_netbind}			{ count(); return B256ERROR_NETBIND; }
{error_netconn}			{ count(); return B256ERROR_NETCONN; }
{error_nethost}			{ count(); return B256ERROR_NETHOST; }
{error_netnone}			{ count(); return B256ERROR_NETNONE; }
{error_netread}			{ count(); return B256ERROR_NETREAD; }
{error_netsocknumber}			{ count(); return B256ERROR_NETSOCKNUMBER; }
{error_netsockopt}			{ count(); return B256ERROR_NETSOCKOPT; }
{error_netsock}			{ count(); return B256ERROR_NETSOCK; }
{error_netwrite}			{ count(); return B256ERROR_NETWRITE; }
{error_nextnofor}			{ count(); return B256ERROR_NEXTNOFOR; }
{error_none}			{ count(); return B256ERROR_NONE; }
{error_nosuchfunction}			{ count(); return B256ERROR_NOSUCHFUNCTION; }
{error_nosuchlabel}			{ count(); return B256ERROR_NOSUCHLABEL; }
{error_nosuchsubroutine}			{ count(); return B256ERROR_NOSUCHSUBROUTINE; }
{error_notarray}			{ count(); return B256ERROR_NOTARRAY; }
{error_notimplemented}			{ count(); return B256ERROR_NOTIMPLEMENTED; }
{error_numberconv}			{ count(); return B256ERROR_NUMBERCONV; }
{error_numberexpr}			{ count(); return B256ERROR_NUMBEREXPR; }
{error_onedimensional}			{ count(); return B256ERROR_ONEDIMENSIONAL; }
{error_onerrorsub}			{ count(); return B256ERROR_ONERRORSUB; }
{error_penwidth}			{ count(); return B256ERROR_PENWIDTH; }
{error_permission}			{ count(); return B256ERROR_PERMISSION; }
{error_polypoints}			{ count(); return B256ERROR_POLYPOINTS; }
{error_printernotoff}			{ count(); return B256ERROR_PRINTERNOTOFF; }
{error_printernoton}			{ count(); return B256ERROR_PRINTERNOTON; }
{error_printeropen}			{ count(); return B256ERROR_PRINTEROPEN; }
{error_radixstring}			{ count(); return B256ERROR_RADIXSTRING; }
{error_radix}			{ count(); return B256ERROR_RADIX; }
{error_refnotassigned}			{ count(); return B256ERROR_REFNOTASSIGNED; }
{error_rgb}			{ count(); return B256ERROR_RGB; }
{error_serialparameter}			{ count(); return B256ERROR_SERIALPARAMETER; }
{error_settingmaxkeys}			{ count(); return B256ERROR_SETTINGMAXKEYS; }
{error_settingmaxlen}			{ count(); return B256ERROR_SETTINGMAXLEN; }
{error_settingsgetaccess}			{ count(); return B256ERROR_SETTINGSGETACCESS; }
{error_settingssetaccess}			{ count(); return B256ERROR_SETTINGSSETACCESS; }
{error_slicesize}			{ count(); return B256ERROR_SLICESIZE; }
{error_sounderror}			{ count(); return B256ERROR_SOUNDERROR; }
{error_soundfileformat}			{ count(); return B256ERROR_SOUNDFILEFORMAT; }
{error_soundfile}			{ count(); return B256ERROR_SOUNDFILE; }
{error_soundlength}			{ count(); return B256ERROR_SOUNDLENGTH; }
{error_soundnotseekable}			{ count(); return B256ERROR_SOUNDNOTSEEKABLE; }
{error_soundresource}			{ count(); return B256ERROR_SOUNDRESOURCE; }
{error_spritena}			{ count(); return B256ERROR_SPRITENA; }
{error_spritenumber}			{ count(); return B256ERROR_SPRITENUMBER; }
{error_spriteslice}			{ count(); return B256ERROR_SPRITESLICE; }
{error_sqrrange}			{ count(); return B256ERROR_SQRRANGE; }
{error_stackunderflow}			{ count(); return B256ERROR_STACKUNDERFLOW; }
{error_string2note}			{ count(); return B256ERROR_STRING2NOTE; }
{error_stringconv}			{ count(); return B256ERROR_STRINGCONV; }
{error_stringexpr}			{ count(); return B256ERROR_STRINGEXPR; }
{error_stringmaxlen}			{ count(); return B256ERROR_STRINGMAXLEN; }
{error_strstart}			{ count(); return B256ERROR_STRSTART; }
{error_toomanysounds}			{ count(); return B256ERROR_TOOMANYSOUNDS; }
{error_unexpectedreturn}			{ count(); return B256ERROR_UNEXPECTEDRETURN; }
{error_unserializeformat}			{ count(); return B256ERROR_UNSERIALIZEFORMAT; }
{error_varcircular}			{ count(); return B256ERROR_VARCIRCULAR; }
{error_varnotassigned}			{ count(); return B256ERROR_VARNOTASSIGNED; }
{error_varnull}			{ count(); return B256ERROR_VARNULL; }
{error_waveformlogical}			{ count(); return B256ERROR_WAVEFORMLOGICAL; }
{error_wavobsolete}			{ count(); return B256ERROR_WAVOBSOLETE; }
{exists}					{ count(); return B256EXISTS; }
{explodex}					{ count(); return B256EXPLODEX; }
{explode}					{ count(); return B256EXPLODE; }
{exp}						{ count(); return B256EXP; }
{fill}						{ count(); return B256FILL; }
{floor}						{ count(); return B256FLOOR; }
{freedbset}					{ count(); return B256FREEDBSET; }
{freedb}					{ count(); return B256FREEDB; }
{freefile}					{ count(); return B256FREEFILE; }
{freenet}					{ count(); return B256FREENET; }
{frombinary}				{ count(); return B256FROMBINARY; }
{fromhex}					{ count(); return B256FROMHEX; }
{fromoctal}					{ count(); return B256FROMOCTAL; }
{fromradix}					{ count(); return B256FROMRADIX; }
{getarraybase}				{ count(); return B256GETARRAYBASE; }
{getbrushcolor}				{ count(); return B256GETBRUSHCOLOR; }
{getclipboardimage}			{ count(); return B256GETCLIPBOARDIMAGE; }
{getclipboardstring}		{ count(); return B256GETCLIPBOARDSTRING; }
{getcolor}					{ count(); return B256GETCOLOR; }
{getpenwidth}				{ count(); return B256GETPENWIDTH; }
{getsetting}				{ count(); return B256GETSETTING; }
{getslice}					{ count(); return B256GETSLICE; }
{graphheight}				{ count(); return B256GRAPHHEIGHT; }
{graphwidth}				{ count(); return B256GRAPHWIDTH; }
{gray}						{ count(); return B256GREY; }
{green}						{ count(); return B256GREEN; }
{hour}						{ count(); return B256HOUR; }
{imagecopy}					{ count(); return B256IMAGECOPY; }
{imageheight}					{ count(); return B256IMAGEHEIGHT; }
{imageload}					{ count(); return B256IMAGELOAD; }
{imagenew}					{ count(); return B256IMAGENEW; }
{imagepixel}					{ count(); return B256IMAGEPIXEL; }
{imagetype_bmp}				{ count(); return B256IMAGETYPE_BMP; }
{imagetype_jpg}				{ count(); return B256IMAGETYPE_JPG; }
{imagetype_png}				{ count(); return B256IMAGETYPE_PNG; }
{imagewidth}					{ count(); return B256IMAGEWIDTH; }
{implode}					{ count(); return B256IMPLODE; }
{instrx}					{ count(); return B256INSTRX; }
{instr}						{ count(); return B256INSTR; }
{isnumeric}					{ count(); return B256ISNUMERIC; }
{keypressed}				{ count(); return B256KEYPRESSED; }
{key}						{ count(); return B256KEY; }
{lasterrorextra} 			{ count(); return B256LASTERROREXTRA; }
{lasterrorline}				{ count(); return B256LASTERRORLINE; }
{lasterrormessage}			{ count(); return B256LASTERRORMESSAGE; }
{lasterror}					{ count(); return B256LASTERROR; }
{left}						{ count(); return B256LEFT; }
{length}					{ count(); return B256LENGTH; }
{ljust}						{ count(); return B256LJUST; }
{logten}					{ count(); return B256LOGTEN; }
{log}						{ count(); return B256LOG; }
{lower}						{ count(); return B256LOWER; }
{ltrim}						{ count(); return B256LTRIM; }
{md5}						{ count(); return B256MD5; }
{midx}						{ count(); return B256MIDX; }
{mid}						{ count(); return B256MID; }
{minute}					{ count(); return B256MINUTE; }
{month}						{ count(); return B256MONTH; }
{mousebutton_center}		{ count(); return B256MOUSEBUTTON_CENTER; }
{mousebutton_doubleclick}	{ count(); return B256MOUSEBUTTON_DOUBLECLICK; }
{mousebutton_left}			{ count(); return B256MOUSEBUTTON_LEFT; }
{mousebutton_none}			{ count(); return B256MOUSEBUTTON_NONE; }
{mousebutton_right}			{ count(); return B256MOUSEBUTTON_RIGHT; }
{mouseb}					{ count(); return B256MOUSEB; }
{mousex}					{ count(); return B256MOUSEX; }
{mousey}					{ count(); return B256MOUSEY; }
{msec}						{ count(); return B256MSEC; }
{netaddress}				{ count(); return B256NETADDRESS; }
{netdata}					{ count(); return B256NETDATA; }
{netread}					{ count(); return B256NETREAD; }
{openfiledialog}			{ count(); return B256OPENFILEDIALOG; }
{orange}					{ count(); return B256ORANGE; }
{ostype_android}			{ count(); return B256OSTYPE_ANDROID; }
{ostype_linux}				{ count(); return B256OSTYPE_LINUX; }
{ostype_macintosh}			{ count(); return B256OSTYPE_MACINTOSH; }
{ostype_windows}			{ count(); return B256OSTYPE_WINDOWS; }
{ostype}					{ count(); return B256OSTYPE; }
{pixel}						{ count(); return B256PIXEL; }
{pi}						{ count(); return B256PI; }
{portin}					{ count(); return B256PORTIN; }
{purple}					{ count(); return B256PURPLE; }
{radians}					{ count(); return B256RADIANS; }
{rand}						{ count(); return B256RAND; }
{readbyte}					{ count(); return B256READBYTE; }
{readline}					{ count(); return B256READLINE; }
{read}						{ count(); return B256READ; }
{red}						{ count(); return B256RED; }
{ref}						{ count(); return B256REF; }
{replacex}					{ count(); return B256REPLACEX; }
{replace}					{ count(); return B256REPLACE; }
{rgb}						{ count(); return B256RGB; }
{right}						{ count(); return B256RIGHT; }
{rjust}						{ count(); return B256RJUST; }
{round}						{ count(); return B256ROUND; }
{rtrim}						{ count(); return B256RTRIM; }
{savefiledialog}			{ count(); return B256SAVEFILEDIALOG; }
{second}					{ count(); return B256SECOND; }
{serialize}					{ count(); return B256SERIALIZE; }
{sin}						{ count(); return B256SIN; }
{size}						{ count(); return B256SIZE; }
{slice_all}					{ count(); return B256SLICE_ALL; }
{slice_paint}				{ count(); return B256SLICE_PAINT; }
{slice_sprite}				{ count(); return B256SLICE_SPRITE; }
{soundid}					{ count(); return B256SOUNDID; }
{soundlength}				{ count(); return B256SOUNDLENGTH; }
{soundloadraw}				{ count(); return B256SOUNDLOADRAW; }
{soundload}					{ count(); return B256SOUNDLOAD; }
{soundplayer}				{ count(); return B256SOUNDPLAYER; }
{soundplay}					{ count(); return B256SOUNDPLAY; }
{soundposition}				{ count(); return B256SOUNDPOSITION; }
{soundsamplerate}			{ count(); return B256SOUNDSAMPLERATE; }
{soundstate}				{ count(); return B256SOUNDSTATE; }
{spritecollide}				{ count(); return B256SPRITECOLLIDE; }
{spriteh}					{ count(); return B256SPRITEH; }
{spriteo}					{ count(); return B256SPRITEO; }
{spriter}					{ count(); return B256SPRITER; }
{sprites}					{ count(); return B256SPRITES; }
{spritev}					{ count(); return B256SPRITEV; }
{spritew}					{ count(); return B256SPRITEW; }
{spritex}					{ count(); return B256SPRITEX; }
{spritey}					{ count(); return B256SPRITEY; }
{sqrt}						{ count(); return B256SQR; }
{sqr}						{ count(); return B256SQR; }
{tan}						{ count(); return B256TAN; }
{textheight}				{ count(); return B256TEXTHEIGHT; }
{textwidth}					{ count(); return B256TEXTWIDTH; }
{tobinary}					{ count(); return B256TOBINARY; }
{tofloat}					{ count(); return B256TOFLOAT; }
{tohex}						{ count(); return B256TOHEX; }
{toint}						{ count(); return B256TOINT; }
{tooctal}					{ count(); return B256TOOCTAL; }
{toradix}					{ count(); return B256TORADIX; }
{tostring}					{ count(); return B256TOSTRING; }
{trim}						{ count(); return B256TRIM; }
{type_array}				{ count(); return B256TYPE_ARRAY; }
{type_float}				{ count(); return B256TYPE_FLOAT; }
{type_int}					{ count(); return B256TYPE_INT; }
{type_map}					{ count(); return B256TYPE_MAP; }
{type_ref}					{ count(); return B256TYPE_REF; }
{type_string}				{ count(); return B256TYPE_STRING; }
{type_unassigned}			{ count(); return B256TYPE_UNASSIGNED; }
{typeof}					{ count(); return B256TYPEOF; }
{unserialize}				{ count(); return B256UNSERIALIZE; }
{upper}						{ count(); return B256UPPER; }
{variablewatch}				{ count(); return B256VARIABLEWATCH; }
{version}					{ count(); return B256VERSION; }
{warning_arrayelement}			{ count(); return B256WARNING_ARRAYELEMENT; }
{warning_booleanconv}			{ count(); return B256WARNING_BOOLEANCONV; }
{warning_integerrange}			{ count(); return B256WARNING_INTEGERRANGE; }
{warning_longrange}			{ count(); return B256WARNING_LONGRANGE; }
{warning_numberconv}			{ count(); return B256WARNING_NUMBERCONV; }
{warning_refnotassigned}			{ count(); return B256WARNING_REFNOTASSIGNED; }
{warning_sounderror}			{ count(); return B256WARNING_SOUNDERROR; }
{warning_soundfileformat}			{ count(); return B256WARNING_SOUNDFILEFORMAT; }
{warning_soundlength}			{ count(); return B256WARNING_SOUNDLENGTH; }
{warning_soundnotseekable}			{ count(); return B256WARNING_SOUNDNOTSEEKABLE; }
{warning_start}			{ count(); return B256WARNING_START; }
{warning_string2note}			{ count(); return B256WARNING_STRING2NOTE; }
{warning_stringconv}			{ count(); return B256WARNING_STRINGCONV; }
{warning_varnotassigned}			{ count(); return B256WARNING_VARNOTASSIGNED; }
{warning_wavobsolete}			{ count(); return B256WARNING_WAVOBSOLETE; }
{wavlength}					{ count(); return B256WAVLENGTH; }
{wavpos}					{ count(); return B256WAVPOS; }
{wavstate}					{ count(); return B256WAVSTATE; }
{white}						{ count(); return B256WHITE; }
{year}						{ count(); return B256YEAR; }
{yellow}					{ count(); return B256YELLOW; }
{zfill}						{ count(); return B256ZFILL; }

 /**********************************************************************/
 /* statement rules                                                    */
 /* For statements with no arguments (or optionally with no arguments) */
 /* you NEED to include a version with a ':' on the end that unputs    */
 /* the ':' - multiple statements on a line and confusion with labels  */
 /* require this.                                                      */
 /**********************************************************************/

{alert}						{ count(); return B256ALERT; }
{begincase}					{ count(); return B256BEGINCASE; }
{begincase}":"				{ count(); unputcolon(); return B256BEGINCASE; }
{call}						{ count(); return B256CALL; }
{case}						{ count(); return B256CASE; }
{catch}						{ count(); return B256CATCH; }
{catch}":"					{ count(); unputcolon(); return B256CATCH; }
{changedir}					{ count(); return B256CHANGEDIR; }
{chord}						{ count(); return B256CHORD; }
{circle}					{ count(); return B256CIRCLE; }
{clg}						{ count(); return B256CLG; }
{clg}":"					{ count(); unputcolon(); return B256CLG; }
{clickclear}				{ count(); return B256CLICKCLEAR; }
{clickclear}":"				{ count(); unputcolon(); return B256CLICKCLEAR; }
{close}						{ count(); return B256CLOSE; }
{close}":"					{ count(); unputcolon(); return B256CLOSE; }
{cls}						{ count(); return B256CLS; }
{cls}":"					{ count(); unputcolon(); return B256CLS; }
{color}						{ count(); return B256SETCOLOR; }
{continuedo}				{ count(); return B256CONTINUEDO; }
{continuedo}":"				{ count(); unputcolon(); return B256CONTINUEDO; }
{continuefor}				{ count(); return B256CONTINUEFOR; }
{continuefor}":"			{ count(); unputcolon(); return B256CONTINUEFOR; }
{continuewhile}				{ count(); return B256CONTINUEWHILE; }
{continuewhile}":"			{ count(); unputcolon(); return B256CONTINUEWHILE; }
{dbcloseset}				{ count(); return B256DBCLOSESET; }
{dbcloseset}":"				{ count(); unputcolon(); return B256DBCLOSESET; }
{dbclose}					{ count(); return B256DBCLOSE; }
{dbclose}":"				{ count(); unputcolon(); return B256DBCLOSE; }
{dbexecute}					{ count(); return B256DBEXECUTE; }
{dbopenset}					{ count(); return B256DBOPENSET; }
{dbopen}					{ count(); return B256DBOPEN; }
{dim}						{ count(); return B256DIM; }
{do}						{ count(); return B256DO; }
{do}":"						{ count(); unputcolon(); return B256DO; }
{editvisible}				{ count(); return B256EDITVISIBLE; }
{ellipse}					{ count(); return B256ELLIPSE; }
{else}						{ count(); return B256ELSE; }
{else}":"					{ count(); unputcolon(); return B256ELSE; }
{endcase}					{ count(); return B256ENDCASE; }
{endcase}":"				{ count(); unputcolon(); return B256ENDCASE; }
{endfunction}				{ count(); return B256ENDFUNCTION; }
{endfunction}":"			{ count(); unputcolon(); return B256ENDFUNCTION; }
{endif}						{ count(); return B256ENDIF; }
{endif}":"					{ count(); unputcolon(); return B256ENDIF; }
{endsubroutine}				{ count(); return B256ENDSUBROUTINE; }
{endsubroutine}":"			{ count(); unputcolon(); return B256ENDSUBROUTINE; }
{endtry}					{ count(); return B256ENDTRY; }
{endtry}":"					{ count(); unputcolon(); return B256ENDTRY; }
{endwhile}					{ count(); return B256ENDWHILE; }
{endwhile}":"				{ count(); unputcolon(); return B256ENDWHILE; }
{end}						{ count(); return B256END; }
{end}":"					{ count(); unputcolon(); return B256END; }
{exitdo}					{ count(); return B256EXITDO; }
{exitdo}":"					{ count(); unputcolon(); return B256EXITDO; }
{exitfor}					{ count(); return B256EXITFOR; }
{exitfor}":"				{ count(); unputcolon(); return B256EXITFOR; }
{exitwhile}					{ count(); return B256EXITWHILE; }
{exitwhile}":"				{ count(); unputcolon(); return B256EXITWHILE; }
{fastgraphics}				{ count(); return B256FASTGRAPHICS; }
{fastgraphics}":"			{ count(); unputcolon(); return B256FASTGRAPHICS; }
{font}						{ count(); return B256FONT; }
{for}						{ count(); return B256FOR; }
{foreach}						{ count(); return B256FOREACH; }
{function}					{ count(); return B256FUNCTION; }
{global}					{ count(); return B256GLOBAL; }
{gosub}						{ count(); return B256GOSUB; }
{goto}						{ count(); return B256GOTO; }
{graphsize}					{ count(); return B256GRAPHSIZE; }
{graphvisible}				{ count(); return B256GRAPHVISIBLE; }
{graphtoolbarvisible}		{ count(); return B256GRAPHTOOLBARVISIBLE; }
{if}						{ count(); return B256IF; }
{in}						{ count(); return B256IN; }
{imageautocrop}					{ count(); return B256IMAGEAUTOCROP; }
{imagecentered}					{ count(); return B256IMAGECENTERED; }
{imagecrop}					{ count(); return B256IMAGECROP; }
{imagedraw}					{ count(); return B256IMAGEDRAW; }
{imageflip}					{ count(); return B256IMAGEFLIP; }
{imageresize}					{ count(); return B256IMAGERESIZE; }
{imagerotate}					{ count(); return B256IMAGEROTATE; }
{imagesetpixel}					{ count(); return B256IMAGESETPIXEL; }
{imagesmooth}					{ count(); return B256IMAGESMOOTH; }
{imagetransformed}				{ count(); return B256IMAGETRANSFORMED; }
{imgload}					{ count(); return B256IMGLOAD; }
{imgsave}					{ count(); return B256IMGSAVE; }
{inputfloat}				{ count(); return B256INPUTFLOAT; }
{inputint}					{ count(); return B256INPUTINT; }
{inputstring}				{ count(); return B256INPUTSTRING; }
{input}						{ count(); return B256INPUT; }
{kill}						{ count(); return B256KILL; }
{let}						{ count(); return B256LET; }
{line}						{ count(); return B256LINE; }
{maintoolbarvisible}		{ count(); return B256MAINTOOLBARVISIBLE; }
{map}						{ count(); return B256MAP; }
{netclose}					{ count(); return B256NETCLOSE; }
{netclose}":"				{ count(); unputcolon(); return B256NETCLOSE; }
{netconnect}				{ count(); return B256NETCONNECT; }
{netlisten}					{ count(); return B256NETLISTEN; }
{netwrite}					{ count(); return B256NETWRITE; }
{next}						{ count(); return B256NEXT; }
{offerror}					{ count(); return B256OFFERROR; }
{offerror}":"				{ count(); unputcolon(); return B256OFFERROR; }
{onerror}					{ count(); return B256ONERROR; }
{openb}						{ count(); return B256OPENB; }
{openserial}				{ count(); return B256OPENSERIAL; }
{open}						{ count(); return B256OPEN; }
{outputvisible}				{ count(); return B256OUTPUTVISIBLE; }
{outputtoolbarvisible}		{ count(); return B256OUTPUTTOOLBARVISIBLE; }
{pause}						{ count(); return B256PAUSE; }
{penwidth}					{ count(); return B256PENWIDTH; }
{pie}						{ count(); return B256PIE; }
{plot}						{ count(); return B256PLOT; }
{poly}						{ count(); return B256POLY; }
{portout}					{ count(); return B256PORTOUT; }
{printercancel}				{ count(); return B256PRINTERCANCEL; }
{printercancel}":"			{ count(); unputcolon(); return B256PRINTERCANCEL; }
{printeroff}				{ count(); return B256PRINTEROFF; }
{printeroff}":"				{ count(); unputcolon(); return B256PRINTEROFF; }
{printeron}					{ count(); return B256PRINTERON; }
{printeron}":"				{ count(); unputcolon(); return B256PRINTERON; }
{printerpage}				{ count(); return B256PRINTERPAGE; }
{printerpage}":"			{ count(); unputcolon(); return B256PRINTERPAGE; }
{print}						{ count(); return B256PRINT; }
{print}":"					{ count(); unputcolon(); return B256PRINT; }
{prompt}					{ count(); return B256PROMPT; }
{putslice}					{ count(); return B256PUTSLICE; }
{rect}						{ count(); return B256RECT; }
{redim}						{ count(); return B256REDIM; }
{refresh}					{ count(); return B256REFRESH; }
{refresh}":"				{ count(); unputcolon(); return B256REFRESH; }
{regexminimal}				{ count(); return B256REGEXMINIMAL; }
{remalt}					{ count(); return B256REM; }
{rem}						{ count(); return B256REM; }
{reset}						{ count(); return B256RESET; }
{reset}":"					{ count(); unputcolon(); return B256RESET; }
{return}					{ count(); return B256RETURN; }
{return}":"					{ count(); unputcolon(); return B256RETURN; }
{say}						{ count(); return B256SAY; }
{seed}						{ count(); return B256SEED; }
{seek}						{ count(); return B256SEEK; }
{setclipboardimage}			{ count(); return B256SETCLIPBOARDIMAGE; }
{setclipboardstring}		{ count(); return B256SETCLIPBOARDSTRING; }
{setgraph}						{ count(); return B256SETGRAPH; }
{setgraph}":"					{ count(); unputcolon(); return B256SETGRAPH; }
{setsetting}				{ count(); return B256SETSETTING; }
{soundenvelope}					{ count(); return B256SOUNDENVELOPE; }
{soundenvelope}":"				{ count(); unputcolon(); return B256SOUNDENVELOPE; }
{soundfade}					{ count(); return B256SOUNDFADE; }
{soundharmonics}				{ count(); return B256SOUNDHARMONICS; }
{soundharmonics}":"				{ count(); unputcolon(); return B256SOUNDHARMONICS; }
{soundloop}					{ count(); return B256SOUNDLOOP; }
{soundpause}					{ count(); return B256SOUNDPAUSE; }
{soundpause}":"					{ count(); unputcolon(); return B256SOUNDPAUSE; }
{soundplayeroff}				{ count(); return B256SOUNDPLAYEROFF; }
{soundplayeroff}":"				{ count(); unputcolon(); return B256SOUNDPLAYEROFF; }
{soundseek}					{ count(); return B256SOUNDSEEK; }
{soundstop}					{ count(); return B256SOUNDSTOP; }
{soundstop}":"					{ count(); unputcolon(); return B256SOUNDSTOP; }
{soundsystem}					{ count(); return B256SOUNDSYSTEM; }
{soundvolume}					{ count(); return B256SOUNDVOLUME; }
{soundwait}					{ count(); return B256SOUNDWAIT; }
{soundwait}":"					{ count(); unputcolon(); return B256SOUNDWAIT; }
{soundwaveform}					{ count(); return B256SOUNDWAVEFORM; }
{sound}						{ count(); return B256SOUND; }
{spritedim}					{ count(); return B256SPRITEDIM; }
{spritehide}				{ count(); return B256SPRITEHIDE; }
{spriteload}				{ count(); return B256SPRITELOAD; }
{spritemove}				{ count(); return B256SPRITEMOVE; }
{spriteplace}				{ count(); return B256SPRITEPLACE; }
{spritepoly}				{ count(); return B256SPRITEPOLY; }
{spriteshow}				{ count(); return B256SPRITESHOW; }
{spriteslice}				{ count(); return B256SPRITESLICE; }
{stamp}						{ count(); return B256STAMP; }
{step}						{ count(); return B256STEP; }
{subroutine}				{ count(); return B256SUBROUTINE; }
{system}					{ count(); return B256SYSTEM; }
{text}						{ count(); return B256TEXT; }
{then}						{ count(); return B256THEN; }
{then}":"					{ count(); unputcolon(); return B256THEN; }
{throwerror}				{ count(); return B256THROWERROR; }
{to}						{ count(); return B256TO; }
{try}						{ count(); return B256TRY; }
{try}":"					{ count(); unputcolon(); return B256TRY; }
{unassign}					{ count(); return B256UNASSIGN; }
{unload}					{ count(); return B256UNLOAD; }
{until}						{ count(); return B256UNTIL; }
{volume}					{ count(); return B256VOLUME; }
{wavpause}					{ count(); return B256WAVPAUSE; }
{wavpause}":"				{ count(); unputcolon(); return B256WAVPAUSE; }
{wavplay}					{ count(); return B256WAVPLAY; }
{wavplay}":"				{ count(); unputcolon(); return B256WAVPLAY; }
{wavseek}					{ count(); return B256WAVSEEK; }
{wavstop}					{ count(); return B256WAVSTOP; }
{wavstop}":"				{ count(); unputcolon(); return B256WAVSTOP; }
{wavwait}					{ count(); return B256WAVWAIT; }
{wavwait}":"				{ count(); unputcolon(); return B256WAVWAIT; }
{while}						{ count(); return B256WHILE; }
{writebyte}					{ count(); return B256WRITEBYTE; }
{writeline}					{ count(); return B256WRITELINE; }
{write}						{ count(); return B256WRITE; }

 /**********************************************************************/
 /* operator rules                                                     */
 /**********************************************************************/

{and}						{ count(); return B256AND; }
{not}						{ count(); return B256NOT; }
{or}						{ count(); return B256OR; }
{xor}						{ count(); return B256XOR; }
">="						{ count(); return B256GTE; }
"<="						{ count(); return B256LTE; }
"<>"						{ count(); return B256NE; }
"+"							{ count(); return '+'; }
"++"						{ count(); return B256ADD1; }
"+="						{ count(); return B256ADDEQUAL; }
"-"							{ count(); return '-'; }
"--"						{ count(); return B256SUB1; }
"-="						{ count(); return B256SUBEQUAL; }
"*"							{ count(); return '*'; }
"*="						{ count(); return B256MULEQUAL; }
"\\"						{ count(); return B256INTDIV; }
mod							{ count(); return B256MOD; }
"|"							{ count(); return B256BINARYOR; }
"&"							{ count(); return B256AMP; }
"&="						{ count(); return B256AMPEQUAL; }
"~"							{ count(); return B256BINARYNOT; }
"/"							{ count(); return '/'; }
"/="						{ count(); return B256DIVEQUAL; }
"^"							{ count(); return '^'; }
"="							{ count(); return '='; }
"<"							{ count(); return '<'; }
">"							{ count(); return '>'; }
","							{ count(); return ','; }
";"							{ count(); return B256SEMICOLON; }
";="						{ count(); return B256SEMICOLONEQUAL; }
":"							{ count(); return ':'; }
"("							{ count(); return '('; }
")"							{ count(); return ')'; }
"{"							{ count(); return '{'; }
"}"							{ count(); return '}'; }
"["							{ count(); return '['; }
"]"							{ count(); return ']'; }
"<<"						{ count(); return B256BITSHIFTL; }
">>"						{ count(); return B256BITSHIFTR; }

{label} {
	char *temp;
	char *c, *d;

	count();
	temp = strdup(yytext);
	// strip whitespace
	c = temp;
	while(*c == ' ' || *c == '\t') {
		c++;
	}
	if (c!=temp) {
		d = temp;
		while(*c!=0x00) {
			*d++ = *c++;
		}
		*d = 0x00;
	}
	// strip :
	c = temp;
	while (*c != ':') c++;
	*c = 0x0;
	//
	yylval.number = getSymbol(temp); // get existing or create new
	free(temp);
	//
	return B256LABEL;
}
	
{variable} {
	count();
	yylval.number = getSymbol(yytext);
	return B256VARIABLE;
}


"\n"	{ return '\n'; }

{whitespace}	{
	/* eat the whitespace at the begining of a token - but count */
	count();
}

.	{
	/* we have a single character - count it and send it through */
 	column++;
 	return yytext[0];
}

<INCLUDE>((\"[^\"\n]*\")|(\'[^\'\n]*\'))  {
	// got the include file name (with quotes) and store it
	//printf("<INCLUDE> nume \"%s\"\n", yytext);
	if ( numincludes >= MAX_INCLUDE_DEPTH ) {
			BEGIN(INITIAL);
			errorcode = COMPERR_INCLUDEDEPTH;
			return -1;
	}
	if ( yyleng == 2 ) {
			BEGIN(INITIAL);
			errorcode = COMPERR_INCLUDENOFILE;
			return -1;
	}
	filename = strdup(yytext+1);
	filename[strlen(filename)-1] = 0x00;
	//printf("\"%s\"\n", filename);
	BEGIN(INCLUDE_FILE);
}
<INCLUDE>. {
	//fail to grab include file name
	//printf("<INCLUDE>. \"%s\"\n", yytext);
	BEGIN(INITIAL);
	errorcode = COMPERR_INCLUDENOFILE;
	return -1;
}

<INCLUDE><<EOF>> {
	 //EOF before file name
	 //printf("<INCLUDE><<EOF>> \"%s\"\n", yytext);
	 BEGIN(INITIAL);
	 errorcode = COMPERR_INCLUDENOFILE;
	 return -1;
}

<INCLUDE_FILE>(([ \t:]*)|([ \t:]*#.*)|([ \t:]*[Rr][Ee][Mm](([ \t:].+)|([ \t:]*))))$  {
	//include line is ok - eat the rest of the line (after file name)
	//otherwise the remaining chars will be processed just after the included file (errors)
	//This rest is valid only if it is an empty line or a REM statement
	//printf("<INCLUDE_FILE> rest \"%s\"\n", yytext);

	include_stack[numincludes] = YY_CURRENT_BUFFER;

	yyin = fopen( filename, "r" );
	if (!yyin) {
		// check in the executable folder
		char str[512];
		strcpy(str, include_exec_path);
		strcat(str, "/");
		strcat(str, filename);
		yyin = fopen( str, "r" );
		//fprintf(stderr, "real include %s\n", str);
	}
	if ( ! yyin ) {
			BEGIN(INITIAL);
			free(filename);
			filename=NULL;
			errorcode = COMPERR_INCLUDEFILE;
			return -1;
	}

	include_linenumber[numincludes] = linenumber;	// save last level's line number
	include_filenumber[numincludes] = filenumber;	// save the previous file name to the stack
	numincludes++;

	int addnewfilename = 1;

	int f;
	for(f=1;f<include_filenames_counter;f++){
		if(strcmp(filename, include_filenames[f])==0){
			filenumber = f;
			free(filename);
			filename=NULL;
			addnewfilename=0;
			break;
		}
	}

	if(addnewfilename==1){
		include_filenames_counter++;
		if ( include_filenames_counter >= MAX_INCLUDE_FILES ) {
				BEGIN(INITIAL);
				include_filenames_counter--; //to clean up correctly in freeBasicParse()
				free(filename);
				filename=NULL;
				errorcode = COMPERR_INCLUDEMAX;
				return -1;
		}
		filenumber = include_filenames_counter;
		include_filenames[filenumber] = filename;
	}

	lexingfilename = include_filenames[filenumber];
	linenumber = 1;
	column=0;
	addIntOp(OP_CURRLINE, filenumber * 0x1000000 + linenumber);

	yy_switch_to_buffer( yy_create_buffer( yyin, YY_BUF_SIZE ) );
	BEGIN(INITIAL);
}

<INCLUDE_FILE>.   {
	//include line contains other chars - not ok
	//printf("<INCLUDE_FILE>. \"%s\"\n", yytext);
	BEGIN(INITIAL);
	errorcode = COMPERR_INCLUDENOTALONE;
	return -1;
}



<<EOF>> {
	if ( numincludes == 0 ) {
		// eof of the top level program
		yyterminate();
		//fclose(YY_CURRENT_BUFFER->yy_input_file);
		//YY_CURRENT_BUFFER->yy_input_file = NULL;
		//yy_delete_buffer( YY_CURRENT_BUFFER );
	} else {

		// check for unclosed if and other structures
		if (testIfOnTable(numincludes)!=-1) {
			linenumber = testIfOnTable(numincludes);
			return testIfOnTableError(numincludes);
		}

		// eof of an include - return to the includer
		numincludes--;
		linenumber = include_linenumber[numincludes];	// restore last level's line number
		filenumber = include_filenumber[numincludes];	// restore the previous file name/number from the stack
		lexingfilename=include_filenames[filenumber];

		fclose(YY_CURRENT_BUFFER->yy_input_file);
		YY_CURRENT_BUFFER->yy_input_file = NULL;
		yy_delete_buffer( YY_CURRENT_BUFFER );
		yy_switch_to_buffer(include_stack[numincludes] );
	}
}

%%

int
yywrap(void) {
	return 1;
}

void unputcolon() {
	unput(':');
	if (column>0) column--;
}

void count() {
	int i=0;
	while(yytext[i] != '\0'){
		i++;
		column++;
	}
	//printf("\"%s\" %i\n", yytext, column);
}

int basicParse(char *code) {
	BEGIN(INITIAL);
	numargs=0;
	YY_BUFFER_STATE bp;
	
	numparsewarnings=0;
	
	clearIfTable();

	numincludes=0;
	linenumber = 1;
	column = 0;
	errorcode = COMPERR_NONE;
	functionDefSymbol = -1;
	subroutineDefSymbol = -1;
	
	filenumber=0;
	numincludes=0;
	include_filenames[0] = strdup("");
	lexingfilename = include_filenames[0];
	include_filenames_counter = 0;
	filename = NULL;

	bp = yy_scan_string(code);
	yy_switch_to_buffer(bp);
	yyparse();
	yy_delete_buffer(bp);

	if((numincludes>0)){
		//close current include file
		fclose(YY_CURRENT_BUFFER->yy_input_file);
		YY_CURRENT_BUFFER->yy_input_file = NULL;
		yy_delete_buffer( YY_CURRENT_BUFFER );
		while(numincludes>1) {
			numincludes--;
			fclose(include_stack[numincludes]->yy_input_file);
			include_stack[numincludes]->yy_input_file = NULL;
			yy_delete_buffer(include_stack[numincludes]);
		}
	}

	if (testIfOnTable(0)!=-1 && errorcode==COMPERR_NONE) {
		linenumber = testIfOnTable(0);
		errorcode = testIfOnTableError(0);
	}

	return errorcode;
}
