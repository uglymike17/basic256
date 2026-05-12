/** Copyright (C) 2012, J.M.Reneau.
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


// compile errors - returned by basicParse

#define COMPERR_NONE					0
#define COMPERR_SYNTAX					1
#define COMPERR_ENDIF					2
#define COMPERR_ELSE					3
#define COMPERR_ENDWHILE				4
#define COMPERR_UNTIL					5
#define COMPERR_NEXT					6
#define COMPERR_IFNOEND					7
#define COMPERR_ELSENOEND				8
#define COMPERR_WHILENOEND				9
#define COMPERR_DONOEND					10
#define COMPERR_FORNOEND				11
#define COMPERR_FUNCTIONNOEND			12
#define COMPERR_ENDFUNCTION				13
#define COMPERR_FUNCTIONNOTHERE			14
#define COMPERR_GLOBALNOTHERE			15
#define COMPERR_FUNCTIONGOTO			16
#define COMPERR_RETURNVALUE				19
#define COMPERR_CONTINUEDO				21
#define COMPERR_CONTINUEFOR				22
#define COMPERR_CONTINUEWHILE			23
#define COMPERR_EXITDO					24
#define COMPERR_EXITFOR					25
#define COMPERR_EXITWHILE				26
#define COMPERR_INCLUDEFILE				27
#define COMPERR_INCLUDEDEPTH			28
#define COMPERR_TRYNOEND				29
#define COMPERR_CATCH					30
#define COMPERR_CATCHNOEND				31
#define COMPERR_ENDTRY					32
#define COMPERR_NOTINTRY				33
#define COMPERR_ENDBEGINCASE			35
#define COMPERR_ENDENDCASEBEGIN			36
#define COMPERR_ENDENDCASE				37
#define COMPERR_BEGINCASENOEND			38
#define COMPERR_CASENOEND				39
#define COMPERR_LABELREDEFINED			40
#define COMPERR_NEXTWRONGFOR			41
#define COMPERR_SUBROUTINENOEND			42
#define COMPERR_ENDSUBROUTINE			43
#define COMPERR_INCLUDEMAX              44
#define COMPERR_INCLUDENOTALONE         45
#define COMPERR_INCLUDENOFILE           46
#define COMPERR_ONERRORCALL             47
#define COMPERR_NUMBERTOOLARGE          48




// WARNINGS - do not stop run
#define COMPWARNING_START				65536
#define COMPWARNING_MAXIMUMWARNINGS		65536
#define COMPWARNING_DEPRECATED_FORM		65537
#define COMPWARNING_DEPRECATED_REF		65538
