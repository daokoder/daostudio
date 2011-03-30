//=============================================================================
/*
   This file is a part of Dao Studio
   Copyright (C) 2009-2011, Fu Limin
Email: limin.fu@yahoo.com, phoolimin@gmail.com

Dao Studio is free software; you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation;
either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.
 */
//=============================================================================

#include<QFile>
#include<QFileInfo>
#include<QTextStream>
#include<QMouseEvent>
#include<QPaintEvent>
#include<QPainter>
#include<QVBoxLayout>
#include<QScrollBar>
#include<QMessageBox>
#include<QApplication>
#include<QTextCodec>
#include<QLocalSocket>

#include<daoCodeSHL.h>
#include<daoStudioMain.h>

enum
{
	DAO_SHL_COLOR1 ,
	DAO_SHL_COLOR2 ,
	DAO_SHL_COLOR3 ,
	DAO_SHL_COLOR4 ,
	DAO_SHL_COLOR5 ,
	DAO_SHL_COLOR6 ,
	DAO_SHL_COLOR7 ,
	DAO_SHL_COLOR8 ,
	DAO_SHL_COLOR9
};


DaoBasicSyntax::DaoBasicSyntax( const QString & lang )
{
	language = lang;
	keyStruct = DHash_New(D_STRING,0);
	keyStorage = DHash_New(D_STRING,0);
	keyStatement = DHash_New(D_STRING,0);
	keyConstant = DHash_New(D_STRING,0);
	keyOthers = DHash_New(D_STRING,0);
	mbs = DString_New(1);
	wcs = DString_New(0);
	cmtLine1 = NULL;
	cmtLine2 = NULL;
	cmtOpen1 = NULL;
	cmtOpen2 = NULL;
	cmtClose1 = NULL;
	cmtClose2 = NULL;
	func_regex = NULL;
	class_regex = NULL;
	hasDaoLineComment = false;
	hasDaoBlockComment = false;
	singleQuotation = true;
	doubleQuotation = true;
	shortQuotation = false;
	caseInsensitive = false;
	isLatex = false;
	tokens = DArray_New( D_TOKEN );
	DString_SetSharing( mbs, 0 );
	DString_SetSharing( wcs, 0 );
}
DaoBasicSyntax::~DaoBasicSyntax()
{
	DArray_Delete( tokens );
	DMap_Delete( keyStruct );
	DMap_Delete( keyStorage );
	DMap_Delete( keyStatement );
	DMap_Delete( keyConstant );
	DMap_Delete( keyOthers );
	DString_Delete( mbs );
	DString_Delete( wcs );
	if( cmtLine1 ) DString_Delete( cmtLine1 );
	if( cmtLine2 ) DString_Delete( cmtLine2 );
	if( cmtOpen1 ) DString_Delete( cmtOpen1 );
	if( cmtOpen2 ) DString_Delete( cmtOpen2 );
	if( cmtClose1 ) DString_Delete( cmtClose1 );
	if( cmtClose2 ) DString_Delete( cmtClose2 );
	if( func_regex ) free( func_regex );
	if( class_regex ) free( class_regex );
}

void DaoBasicSyntax::AddKeywordStruct( const char *keyword )
{
	DString_SetMBS( mbs, keyword );
	DMap_Insert( keyStruct, mbs, 0 );
}
void DaoBasicSyntax::AddKeywordStorage( const char *keyword )
{
	DString_SetMBS( mbs, keyword );
	DMap_Insert( keyStorage, mbs, 0 );
}
void DaoBasicSyntax::AddKeywordStatement( const char *keyword )
{
	DString_SetMBS( mbs, keyword );
	DMap_Insert( keyStatement, mbs, 0 );
}
void DaoBasicSyntax::AddKeywordConstant( const char *keyword )
{
	DString_SetMBS( mbs, keyword );
	DMap_Insert( keyConstant, mbs, 0 );
}
void DaoBasicSyntax::AddKeywordOthers( const char *keyword )
{
	DString_SetMBS( mbs, keyword );
	DMap_Insert( keyOthers, mbs, 0 );
}
void DaoBasicSyntax::AddSingleLineComment( const char *tok )
{
	if( strcmp( tok, "#" ) ==0 ) hasDaoLineComment = true;
	DString_SetMBS( mbs, tok );
	if( cmtLine1 == NULL ){
		cmtLine1 = DString_Copy( mbs );
	}else if( cmtLine2 == NULL ){
		cmtLine2 = DString_Copy( mbs );
	}
}
void DaoBasicSyntax::AddMultiLineComment( const char *open, const char *close )
{
	if( strcmp( open, "#{" ) ==0 and strcmp( close, "#}" ) ==0 )
		hasDaoBlockComment = true;
	DString_SetMBS( mbs, open );
	if( cmtOpen1 == NULL ){
		cmtOpen1 = DString_Copy( mbs );
		DString_SetMBS( mbs, close );
		cmtClose1 = DString_Copy( mbs );
	}else if( cmtOpen2 == NULL ){
		cmtOpen2 = DString_Copy( mbs );
		DString_SetMBS( mbs, close );
		cmtClose2 = DString_Copy( mbs );
	}
}
void DaoBasicSyntax::AddPattern( const char *pat, int group, int color )
{
	DaoSyntaxPattern sp = { NULL, group, color };
	DString_SetMBS( wcs, pat );
	sp.pattern = DaoRegex_New( wcs );
	patterns.append( sp );
}
void DaoBasicSyntax::SetClassPattern( const char *pat )
{
	if( class_regex ) free( class_regex );
	DString_SetMBS( wcs, pat );
	class_regex = DaoRegex_New( wcs );
}
void DaoBasicSyntax::SetMethodPattern( const char *pat )
{
	if( func_regex ) free( func_regex );
	DString_SetMBS( wcs, pat );
	func_regex = DaoRegex_New( wcs );
}
void DaoBasicSyntax::AddNoneIndentPattern( const char *pat )
{
	DString mbs = DString_WrapMBS( pat );
	DaoRegex *noneIndent = DaoRegex_New( & mbs );
	noneIndents.append( noneIndent );
}
void DaoBasicSyntax::AddLessIndentPattern( const char *pat )
{
	DString mbs = DString_WrapMBS( pat );
	DaoRegex *lessIndent = DaoRegex_New( & mbs );
	lessIndents.append( lessIndent );
}
void DaoBasicSyntax::AddMoreIndentPattern( const char *pat )
{
	DString mbs = DString_WrapMBS( pat );
	DaoRegex *moreIndent = DaoRegex_New( & mbs );
	moreIndents.append( moreIndent );
}
void DaoBasicSyntax::AddMoreIndentPattern2( const char *pat )
{
	DString mbs = DString_WrapMBS( pat );
	DaoRegex *moreIndent = DaoRegex_New( & mbs );
	moreIndents2.append( moreIndent );
}
bool DaoBasicSyntax::IndentNone( const QString & codes )
{
	int i;
	DString mbs = DString_WrapMBS( codes.toUtf8().data() );
	for(i=0; i<noneIndents.size(); i++){
		if( DaoRegex_Match( noneIndents[i], & mbs, NULL, NULL ) ) return true;
	}
	return false;
}
bool DaoBasicSyntax::IndentLess( const QString & codes )
{
	int i;
	DString mbs = DString_WrapMBS( codes.toUtf8().data() );
	for(i=0; i<lessIndents.size(); i++){
		if( DaoRegex_Match( lessIndents[i], & mbs, NULL, NULL ) ) return true;
	}
	return false;
}
bool DaoBasicSyntax::IndentMore( const QString & codes )
{
	int i;
	DString mbs = DString_WrapMBS( codes.toUtf8().data() );
	for(i=0; i<moreIndents.size(); i++){
		if( DaoRegex_Match( moreIndents[i], & mbs, NULL, NULL ) ) return true;
	}
	return false;
}
bool DaoBasicSyntax::IndentMore2( const QString & codes )
{
	int i;
	DString mbs = DString_WrapMBS( codes.toUtf8().data() );
	for(i=0; i<moreIndents2.size(); i++){
		if( DaoRegex_Match( moreIndents2[i], & mbs, NULL, NULL ) ) return true;
	}
	return false;
}
int DaoBasicSyntax::Tokenize( DArray *tokens, const char *source )
{
	DArray *toks = this->tokens;
	DaoToken one = {0,0,0,0,0,NULL};
	const char *src = source;
	int i, j, k = 0, n = strlen( src );
	int cmtype = 0;
	int LC1 = cmtLine1 ? cmtLine1->size : 0;
	int LC2 = cmtLine2 ? cmtLine2->size : 0;
	int BC1 = cmtOpen1 ? cmtOpen1->size : 0;
	int BC2 = cmtOpen2 ? cmtOpen2->size : 0;

	one.string = mbs;
	//		DString_SetDataMBS( mbs, tk->string->mbs+1, tk->string->size-1 );
	//		DString_Erase( tk->string, 1, tk->string->size-1 );
	//		DArray_Insert( tokens, & one, i+1 );

	//printf( "==============\n%s\n\n\n", src );
	DArray_Clear( tokens );
	DString ds = DString_WrapMBS( src );

	while( n ){
		//printf( "%5i:  %s\n\n\n\n", n, src );
		//if( n < -100 ) break;
		if( n <0 ){
			printf( "%5i:  %s\n\n\n\n", n, src );
			break;
		}
		ds.mbs = (char*)src;
		ds.size = n;
		one.type = one.name = DTOK_COMMENT;
		//DString_Clear( mbs );
		mbs->size = 0;
		cmtype = 0;
		if( LC1 and strncmp( src, cmtLine1->mbs, LC1 ) ==0 ) cmtype = 1;
		if( LC2 and strncmp( src, cmtLine2->mbs, LC2 ) ==0 ) cmtype = 2;
		if( BC1 and strncmp( src, cmtOpen1->mbs, BC1 ) ==0 ) cmtype = 3;
		if( BC2 and strncmp( src, cmtOpen2->mbs, BC2 ) ==0 ) cmtype = 4;
		if( cmtype ==1 or cmtype ==2 ){
			while( n ){
				DString_AppendChar( one.string, *src );
				src ++;
				n --;
				if( *src == '\n' ) break;
			}
		}else if( cmtype ==3 ){
			k = DString_Find( & ds, cmtClose1, cmtOpen1->size );
			if( k == MAXSIZE ){
				DString_Assign( one.string, & ds );
				one.type = one.name = DTOK_CMT_OPEN;
			}else{
				k += cmtClose1->size;
				DString_SetDataMBS( one.string, ds.mbs, k );
				src += k;
				n -= k;
			}
		}else if( cmtype ==4 ){
			k = DString_Find( & ds, cmtClose2, cmtOpen2->size );
			if( k == MAXSIZE ){
				DString_Assign( one.string, & ds );
				one.type = one.name = DTOK_CMT_OPEN;
			}else{
				k += cmtClose2->size;
				DString_SetDataMBS( one.string, ds.mbs, k );
				src += k;
				n -= k;
			}
		}
		if( cmtype ){
			DArray_Append( tokens, & one );
			if( one.type == DTOK_CMT_OPEN ) return cmtype - 2;
			continue;
		}
		if( *src == '#' ){
			one.type = DTOK_NONE2;
			DString_AppendChar( mbs, '#' );
			if( n >1 and src[1] == '{' and not hasDaoBlockComment ){
				src += 2;
				n -= 2;
				DArray_Append( tokens, & one );
				one.type = DTOK_LCB;
				DString_SetMBS( mbs, "{" );
				DArray_Append( tokens, & one );
				continue;
			}else if( not hasDaoLineComment ){
				src ++;
				n --;
				DArray_Append( tokens, & one );
				continue;
			}
			//DString_Clear( mbs );
			mbs->size = 0;
		}
		if( shortQuotation && (*src == '\'' || *src == '\"') ){
			one.type = DaoToken_Check( src, n, & k );
			if( k < 16 && (one.type == DTOK_MBS || one.type == DTOK_WCS) ){
				DString_SetDataMBS( mbs, src, k );
				DArray_Append( tokens, & one );
				src += k;
				n -= k;
				if( k == 0 ) break;
				continue;
			}
		}
		if( *src == '\'' and not singleQuotation ){
			one.type = DTOK_MBS_OPEN;
			DString_AppendChar( mbs, *src );
			DArray_Append( tokens, & one );
			src += 1;
			n -= 1;
			continue;
		}else if( *src == '\"' and not doubleQuotation ){
			one.type = DTOK_WCS_OPEN;
			DString_AppendChar( mbs, *src );
			DArray_Append( tokens, & one );
			src += 1;
			n -= 1;
			continue;
		}
		one.type = DaoToken_Check( src, n, & k );
		//printf( "n = %i,  %i,  %i=======%s\n", n, k, one.type, src );
		DString_SetDataMBS( mbs, src, k );
		DArray_Append( tokens, & one );
		if( n < k ){
			printf( "n = %i,  %i,  %i=======%s\n", n, k, one.type, src );
		}
		src += k;
		n -= k;
		if( k == 0 ) break;
	}
	return 0;
}

const char *pat_cxx_method = 
"^( ((inline|static|virtual) %s+|) (%w+::|) ( %w [:%w]* (%b<>|) ) (%s*|) ([%*%&]+ %s*|) )"
"( (%w+) :: | ) ((( %w+ | operator %W+ ))) %s* %b() %s* [^%S;]* $";

const char *pat_dao_method =
"^(routine|function|sub|operator) %s+ (%w+ %s* :: %s* |) %w+ %s* %b() %s* [^%S;]* $";

const char *pat_python_method =
"def %s+ %w+ %s* %b() %s* :";

const char *pat_perl_method =
"^sub %s+ %w+";

const char *pat_lua_method =
"^(local %s+|) function %s+ %w+ %s* %b()";

DaoBasicSyntax* DaoBasicSyntax::dao = NULL;
DaoBasicSyntax* DaoBasicSyntax::python = NULL;

DaoLanguages::DaoLanguages()
{
	DaoBasicSyntax *dao = new DaoBasicSyntax( "dao" );
	DaoBasicSyntax::dao = dao;
	DaoCodeSHL::languages[ "dao" ] = dao;
	dao->hasDaoLineComment = true;
	dao->hasDaoBlockComment = true;
	dao->AddKeywordOthers( "std" );
	dao->AddKeywordOthers( "io" );
	dao->SetClassPattern( "^class %s* %w [%w:]* %s* (:|$)" );
	dao->SetMethodPattern( pat_dao_method );
	dao->AddLessIndentPattern( "^ %s* (public | protected | private) %s* :? %s* $" );
	dao->AddLessIndentPattern( "^ %s* case %s+ %S+ %s* (: | - | , | %. )" );
	dao->AddLessIndentPattern( "^ %s* default %s* :" );
	dao->AddMoreIndentPattern2( "^ %s* (public | protected | private) %s* :? %s* $" );
	dao->AddMoreIndentPattern2( "^ %s* case %s+ %S+ %s* (: | - | , | %. )" );
	dao->AddMoreIndentPattern2( "^ %s* default %s* :" );
	dao->AddMoreIndentPattern( "(^ | %W) ( if | for | while | switch ) %s* %b() %s* $" );
	dao->AddMoreIndentPattern( "(^ | %W) ( else %s+ if | rescue ) %s* %b() %s* $" );
	dao->AddMoreIndentPattern( "(^ | %W) (|%}%s*) else %s* $" );
	dao->AddMoreIndentPattern( "(^ | %W) ( try | raise | do ) %s* $" );


	DaoBasicSyntax *cpp = new DaoBasicSyntax( "cpp" );
	DaoCodeSHL::languages[ "h" ] = cpp;
	DaoCodeSHL::languages[ "hpp" ] = cpp;
	DaoCodeSHL::languages[ "hxx" ] = cpp;
	DaoCodeSHL::languages[ "c" ] = cpp;
	DaoCodeSHL::languages[ "cxx" ] = cpp;
	DaoCodeSHL::languages[ "cpp" ] = cpp;
	DaoCodeSHL::languages[ "cxx" ] = cpp;
	DaoCodeSHL::languages[ "c++" ] = cpp;
	cpp->AddKeywordStruct( "namespace" );
	cpp->AddKeywordStruct( "typename" );
	cpp->AddKeywordStruct( "virtual" );
	cpp->AddKeywordStruct( "inline" );
	cpp->AddKeywordStruct( "template" );
	cpp->AddKeywordStruct( "class" );
	cpp->AddKeywordStruct( "struct" );
	cpp->AddKeywordStruct( "enum" );
	cpp->AddKeywordStruct( "bool" );
	cpp->AddKeywordStruct( "char" );
	cpp->AddKeywordStruct( "wchar_t" );
	cpp->AddKeywordStruct( "signed" );
	cpp->AddKeywordStruct( "unsigned" );
	cpp->AddKeywordStruct( "int" );
	cpp->AddKeywordStruct( "short" );
	cpp->AddKeywordStruct( "long" );
	cpp->AddKeywordStruct( "float" );
	cpp->AddKeywordStruct( "double" );
	cpp->AddKeywordStruct( "void" );
	cpp->AddKeywordStruct( "operator" );
	cpp->AddKeywordStruct( "size_t" );
	cpp->AddKeywordStruct( "FILE" );
	cpp->AddKeywordStruct( "int8_t" );
	cpp->AddKeywordStruct( "int16_t" );
	cpp->AddKeywordStruct( "int32_t" );
	cpp->AddKeywordStruct( "int64_t" );
	cpp->AddKeywordStruct( "uint8_t" );
	cpp->AddKeywordStruct( "uint16_t" );
	cpp->AddKeywordStruct( "uint32_t" );
	cpp->AddKeywordStruct( "uint64_t" );

	cpp->AddKeywordStorage( "const" );
	cpp->AddKeywordStorage( "static" );
	cpp->AddKeywordStorage( "extern" );
	cpp->AddKeywordStorage( "private" );
	cpp->AddKeywordStorage( "protected" );
	cpp->AddKeywordStorage( "public" );
	cpp->AddKeywordStorage( "new" );
	cpp->AddKeywordStorage( "delete" );
	cpp->AddKeywordStorage( "this" );
	cpp->AddKeywordStorage( "self" );
	cpp->AddKeywordStorage( "typedef" );
	cpp->AddKeywordStorage( "sizeof" );

	cpp->AddKeywordStatement( "do" );
	cpp->AddKeywordStatement( "for" );
	cpp->AddKeywordStatement( "while" );
	cpp->AddKeywordStatement( "until" );
	cpp->AddKeywordStatement( "switch" );
	cpp->AddKeywordStatement( "case" );
	cpp->AddKeywordStatement( "default" );
	cpp->AddKeywordStatement( "if" );
	cpp->AddKeywordStatement( "else" );
	cpp->AddKeywordStatement( "break" );
	cpp->AddKeywordStatement( "continue" );
	cpp->AddKeywordStatement( "return" );
	cpp->AddKeywordStatement( "goto" );

	cpp->AddKeywordConstant( "false" );
	cpp->AddKeywordConstant( "true" );
	cpp->AddKeywordConstant( "NULL" );
	cpp->AddKeywordConstant( "stdin" );
	cpp->AddKeywordConstant( "stdout" );

	cpp->AddKeywordOthers( "using" );
	cpp->AddKeywordOthers( "pragma" );
	cpp->AddKeywordOthers( "if" );
	cpp->AddKeywordOthers( "ifdef" );
	cpp->AddKeywordOthers( "ifndef" );
	cpp->AddKeywordOthers( "else" );
	cpp->AddKeywordOthers( "endif" );
	cpp->AddKeywordOthers( "undef" );
	cpp->AddKeywordOthers( "define" );
	cpp->AddKeywordOthers( "defined" );
	cpp->AddKeywordOthers( "include" );

	cpp->AddKeywordOthers( "not" );
	cpp->AddKeywordOthers( "or" );
	cpp->AddKeywordOthers( "and" );
	cpp->AddKeywordOthers( "friend" );

	cpp->AddSingleLineComment( "//" );
	cpp->AddMultiLineComment( "/*", "*/" );
	//cpp->AddMultiLineComment( "#if 0", "#else" );
	//cpp->AddMultiLineComment( "#if 0", "#endif" );

	cpp->SetClassPattern( "^(class|struct) %s* %w [%w:]* %s* (:|$)" );
	cpp->SetMethodPattern( pat_cxx_method );
	cpp->AddPattern( "# %s* %w+", 1<<0, DAO_SHL_COLOR1 );
	cpp->AddPattern( "# %s* include %s* (%b<>)", 1<<1, DAO_SHL_COLOR2 );
	//cpp->AddPattern( "^ %s* %w+ %s*:", 1<<0, DAO_SHL_COLOR4 );
	cpp->AddPattern( "^ %s* (%w{1,6} | %w{8,} | [^d]%w{6} | %w[^e]%w{5}"
			"| %w{2}[^f]%w{4} | %w{3}[^a]%w{3} | %w{4}[^u]%w{2} "
			"| %w{5}[^l]%w{1} | %w{6}[^t]) %s* : [^:]", 1<<0, DAO_SHL_COLOR4 );

	cpp->AddNoneIndentPattern( "^ %s* # %s* %w+" );
	cpp->AddNoneIndentPattern( "^ %s* (%w{1,6} | %w{8,} | [^d]%w{6} | %w[^e]%w{5}"
			"| %w{2}[^f]%w{4} | %w{3}[^a]%w{3} | %w{4}[^u]%w{2} "
			"| %w{5}[^l]%w{1} | %w{6}[^t]) %s* : [^:]" );
	cpp->AddLessIndentPattern( "^ %s* (public | protected | private) %s* :? %s* $" );
	cpp->AddLessIndentPattern( "^ %s* case %s+ %S+ %s* (: | - | , | %. )" );
	cpp->AddLessIndentPattern( "^ %s* default %s* :" );
	cpp->AddMoreIndentPattern2( "^ %s* (public | protected | private) %s* :? %s* $" );
	cpp->AddMoreIndentPattern2( "^ %s* case %s+ %S+ %s* (: | - | , | %. )" );
	cpp->AddMoreIndentPattern2( "^ %s* default %s* :" );
	cpp->AddMoreIndentPattern( "(^ | %W) ( if | for | while | switch ) %s* %b() %s* $" );
	cpp->AddMoreIndentPattern( "(^ | %W) ( else %s+ if | rescue ) %s* %b() %s* $" );
	cpp->AddMoreIndentPattern( "(^ | %W) (|%}%s*) else %s* $" );
	cpp->AddMoreIndentPattern( "(^ | %W) ( try | raise | do ) %s* $" );

	
	DaoBasicSyntax *lua = new DaoBasicSyntax( "lua" );
	DaoCodeSHL::languages[ "lua" ] = lua;
	lua->SetMethodPattern( pat_lua_method );
	lua->AddSingleLineComment( "--" );
	lua->AddKeywordStruct( "function" );
	lua->AddKeywordStorage( "local" );
	lua->AddKeywordConstant( "nil" );
	lua->AddKeywordConstant( "false" );
	lua->AddKeywordConstant( "true" );

	lua->AddKeywordStatement( "do" );
	lua->AddKeywordStatement( "end" );
	lua->AddKeywordStatement( "for" );
	lua->AddKeywordStatement( "while" );
	lua->AddKeywordStatement( "until" );
	lua->AddKeywordStatement( "switch" );
	lua->AddKeywordStatement( "case" );
	lua->AddKeywordStatement( "default" );
	lua->AddKeywordStatement( "if" );
	lua->AddKeywordStatement( "then" );
	lua->AddKeywordStatement( "elseif" );
	lua->AddKeywordStatement( "else" );
	lua->AddKeywordStatement( "break" );
	lua->AddKeywordStatement( "continue" );
	lua->AddKeywordStatement( "repeat" );
	lua->AddKeywordStatement( "return" );

	lua->AddKeywordOthers( "not" );
	lua->AddKeywordOthers( "or" );
	lua->AddKeywordOthers( "and" );

	lua->AddMoreIndentPattern2( "(^ | %W) function %s+ %w+ %s* %b()%s* $" );
	lua->AddMoreIndentPattern2( "(^ | %W) for %W+ .* %W+ do %s* $" );
	lua->AddMoreIndentPattern2( "(^ | %W) do %s* $" );
	lua->AddMoreIndentPattern2( "(^ | %W) if %W+ .* %W+ then %s* $" );
	lua->AddMoreIndentPattern2( "^ %s* elseif %W+ .* %W+ then %s* $" );
	lua->AddMoreIndentPattern2( "^ %s* else %s* $" );
	lua->AddLessIndentPattern( "^ %s* elseif %W+ .* %W+ then %s* $" );
	lua->AddLessIndentPattern( "^ %s* else %s* $" );
	lua->AddLessIndentPattern( "^ %s* end (%W | $)" );

	DaoBasicSyntax *py = new DaoBasicSyntax( "python" );
	DaoBasicSyntax::python = py;
	DaoCodeSHL::languages[ "py" ] = py;
	DaoCodeSHL::languages[ "python" ] = py;
	py->SetClassPattern( "^class %s* %w [%w:]* %s* (%b()|) %s* :" );
	py->SetMethodPattern( pat_python_method );
	py->AddSingleLineComment( "#" );
	py->AddKeywordStruct( "class" );
	py->AddKeywordStruct( "def" );
	py->AddKeywordStruct( "map" );
	py->AddKeywordStruct( "lambda" );
	py->AddKeywordStruct( "int" );
	py->AddKeywordStruct( "float" );
	py->AddKeywordStruct( "string" );
	py->AddKeywordStorage( "self" );
	py->AddKeywordStorage( "global" );
	py->AddKeywordConstant( "nil" );
	py->AddKeywordConstant( "false" );
	py->AddKeywordConstant( "true" );

	py->AddKeywordStatement( "do" );
	py->AddKeywordStatement( "end" );
	py->AddKeywordStatement( "for" );
	py->AddKeywordStatement( "in" );
	py->AddKeywordStatement( "while" );
	py->AddKeywordStatement( "until" );
	py->AddKeywordStatement( "switch" );
	py->AddKeywordStatement( "case" );
	py->AddKeywordStatement( "default" );
	py->AddKeywordStatement( "if" );
	py->AddKeywordStatement( "then" );
	py->AddKeywordStatement( "else" );
	py->AddKeywordStatement( "elif" );
	py->AddKeywordStatement( "break" );
	py->AddKeywordStatement( "pass" );
	py->AddKeywordStatement( "return" );
	py->AddKeywordStatement( "yield" );
	py->AddKeywordStatement( "from" );
	py->AddKeywordStatement( "import" );
	py->AddKeywordStatement( "try" );
	py->AddKeywordStatement( "except" );
	
	py->AddKeywordOthers( "not" );
	py->AddKeywordOthers( "or" );
	py->AddKeywordOthers( "and" );
	
	py->AddPattern( "__%w+__", 1<<0, DAO_SHL_COLOR3 );
	
	py->AddMoreIndentPattern2( "(^ | %W) (class|def|if|for|while|switch) %s+ .* : %s* $" );
	py->AddMoreIndentPattern2( "^ %s* elif %W+ .* : %s* $" );
	py->AddMoreIndentPattern2( "^ %s* ( else | rescue ) %s* : %s* $" );
	py->AddLessIndentPattern( "^ %s* elif %W+ .* : %s* $" );
	py->AddLessIndentPattern( "^ %s* ( else | rescue ) %s* : %s* $" );
	
	DaoBasicSyntax *perl = new DaoBasicSyntax( "perl" );
	DaoCodeSHL::languages[ "pl" ] = perl;
	DaoCodeSHL::languages[ "pm" ] = perl;
	DaoCodeSHL::languages[ "perl" ] = perl;
	perl->SetMethodPattern( pat_perl_method );
	perl->AddSingleLineComment( "#" );
	perl->AddKeywordStorage( "my" );
	perl->AddKeywordStorage( "our" );
	perl->AddKeywordStorage( "sub" );
	perl->AddKeywordStorage( "use" );
	perl->AddKeywordStorage( "require" );
	perl->AddKeywordStatement( "for" );
	perl->AddKeywordStatement( "foreach" );
	perl->AddKeywordStatement( "while" );
	perl->AddKeywordStatement( "until" );
	perl->AddKeywordStatement( "continue" );
	perl->AddKeywordStatement( "if" );
	perl->AddKeywordStatement( "elsif" );
	perl->AddKeywordStatement( "else" );
	perl->AddKeywordStatement( "unless" );
	perl->AddKeywordStatement( "do" );
	perl->AddKeywordStatement( "goto" );
	perl->AddKeywordStatement( "next" );
	perl->AddKeywordStatement( "last" );
	perl->AddKeywordStatement( "die" );
	perl->AddKeywordStatement( "return" );
	perl->AddKeywordStatement( "exit" );
	perl->AddKeywordOthers( "and" );
	perl->AddKeywordOthers( "or" );
	perl->AddKeywordOthers( "not" );
	perl->AddKeywordOthers( "eq" );
	perl->AddKeywordOthers( "no" );
	perl->AddKeywordOthers( "shift" );
	perl->AddKeywordOthers( "strict" );
	
	perl->AddKeywordConstant( "fail" );
	
	DaoBasicSyntax *tex = new DaoBasicSyntax( "tex" );
	DaoCodeSHL::languages[ "tex" ] = tex;
	tex->AddPattern( "%\\ %s* (chapter|section|subsection) %s* %{ %s* ( %w+ ) %s*  %}", (1<<2), 0 );
	tex->isLatex = true;
	tex->singleQuotation = false;
	tex->doubleQuotation = false;
	tex->AddSingleLineComment( "%" );
	tex->AddKeywordStruct( "begin" );
	tex->AddKeywordStruct( "end" );
	tex->AddKeywordStruct( "document" );
	tex->AddKeywordStruct( "title" );
	tex->AddKeywordStruct( "author" );
	tex->AddKeywordStruct( "chapter" );
	tex->AddKeywordStruct( "section" );
	tex->AddKeywordStruct( "subsection" );
	tex->AddKeywordStruct( "subsubsection" );

	tex->AddKeywordStatement( "documentclass" );
	tex->AddKeywordStatement( "usepackage" );
	tex->AddKeywordStatement( "addtolength" );
	tex->AddKeywordStatement( "setlength" );
	tex->AddKeywordStatement( "linespread" );
	tex->AddKeywordStatement( "maketitle" );

	DaoBasicSyntax *txt = new DaoBasicSyntax( "txt" );
	DaoCodeSHL::languages[ "txt" ] = txt;
	txt->singleQuotation = false;
	txt->doubleQuotation = false;
	txt->shortQuotation = true;

	DaoBasicSyntax *bnf = new DaoBasicSyntax( "bnf" );
	DaoCodeSHL::languages[ "bnf" ] = bnf;
	bnf->AddPattern( "::=", 1<<0, DAO_SHL_COLOR3 );
	bnf->AddPattern( "%.%.%.", 1<<0, DAO_SHL_COLOR4 );
	bnf->AddPattern( "[%*%+%?]", 1<<0, DAO_SHL_COLOR4 );

	DaoBasicSyntax *html = new DaoBasicSyntax( "html" );
	DaoCodeSHL::languages[ "htm" ] = html;
	DaoCodeSHL::languages[ "html" ] = html;
	html->caseInsensitive = true;
	html->singleQuotation = false;
	html->doubleQuotation = false;
	html->AddKeywordStruct( "html" );
	html->AddKeywordStruct( "head" );
	html->AddKeywordStruct( "body" );
	html->AddKeywordStruct( "style" );
	html->AddKeywordStruct( "div" );
	html->AddKeywordStruct( "span" );
	html->AddKeywordStruct( "table" );

	html->AddKeywordStorage( "class" );

	DaoBasicSyntax *sdml = new DaoBasicSyntax( "sdml" );
	DaoCodeSHL::languages[ "sdml" ] = sdml;
	sdml->caseInsensitive = true;
	sdml->singleQuotation = false;
	sdml->doubleQuotation = false;
	sdml->AddKeywordStruct( "report" );
	sdml->AddKeywordStruct( "title" );
	sdml->AddKeywordStruct( "author" );
	sdml->AddKeywordStruct( "list" );
	sdml->AddKeywordStruct( "table" );
	sdml->AddKeywordStruct( "cite" );
	sdml->AddKeywordStruct( "code" );
	sdml->AddKeywordStruct( "demo" );
	sdml->AddKeywordStruct( "link" );
	sdml->AddPattern( "(%<(=+)%>).*(%<%/%2%>)", 1<<0, 0 );

	DaoBasicSyntax *mkfile = new DaoBasicSyntax( "makefile" );
	DaoCodeSHL::languages[ "makefile" ] = mkfile;
	mkfile->caseInsensitive = true;
	mkfile->singleQuotation = false;
	mkfile->AddSingleLineComment( "#" );
	mkfile->AddKeywordStatement( "ifeq" );
	mkfile->AddKeywordStatement( "ifneq" );
	mkfile->AddKeywordStatement( "endif" );
	mkfile->AddPattern( "^%w+%s*:", 1<<0, DAO_SHL_COLOR3 );
	mkfile->AddPattern( "^%s*(%w+)%s*%+?=", 1<<1, DAO_SHL_COLOR5 );
	mkfile->AddPattern( "%$%(%s*%w+%s*%)", 1<<0, DAO_SHL_COLOR6 );

	DaoBasicSyntax *pld = new DaoBasicSyntax( "pld" );
	DaoCodeSHL::languages[ "pld" ] = pld;
	pld->caseInsensitive = true;
	pld->singleQuotation = true;
	pld->doubleQuotation = true;
	pld->AddSingleLineComment( "#" );
	pld->AddKeywordStruct( "pipeline" );
	pld->AddKeywordStruct( "task" );
	pld->AddKeywordStruct( "program" );
	pld->AddKeywordStruct( "inputs" );
	pld->AddKeywordStruct( "outputs" );
	pld->AddKeywordStruct( "options" );
	pld->AddKeywordStruct( "flags" );
	pld->AddKeywordStruct( "tasks" );
	pld->AddKeywordStruct( "extends" );
	pld->AddKeywordStruct( "help" );
	pld->AddKeywordStatement( "include" );
	pld->AddKeywordStatement( "as" );
	pld->AddKeywordConstant( "on" );
	pld->AddKeywordConstant( "off" );
	pld->AddKeywordConstant( "default" );
	pld->AddKeywordConstant( "stdout" );
	pld->AddKeywordConstant( "stderr" );
}

QMap<QString,DaoBasicSyntax*> DaoCodeSHL::languages;

DaoTokenFormat::DaoTokenFormat()
{
	setFont( DaoStudioSettings::codeFont );
#if 0
	QTextOption::Tab tab;
	QList<QVariant> tabs;
	QFontMetrics fm( DaoStudioSettings::codeFont );
	int tw = 4 * fm.width( 'a' );
	tab.position = tw - 2; 
	tabs.append( tw - 4 );
	tabs.append( 2*tw - 4 );
	tabs.append( 3*tw - 4 );
	tabs.append( 4*tw - 4 );
	tabs.append( 5*tw - 4 );
	tabs.append( 6*tw - 4 );
	tabs.append( 7*tw - 4 );
	setProperty( QTextFormat::TabPositions, QVariant( tabs ) );
#endif
}


DaoCodeSHL::DaoCodeSHL( QTextDocument * parent ) : QSyntaxHighlighter( parent )
{
	plainColor = Qt::black;
	tabColor[0] = QColor( 240, 255, 240 );
	tabColor[1] = QColor( 240, 255, 240 );
	formatComment.setFontItalic( true );
	formatOutput.setProperty( 10000, 1 );
	
	formatOutput.setObjectType( TXT_CHAR_OUTPUT );
	formatPrompt.setObjectType( TXT_CHAR_PROMPT );
	formatComment.setObjectType( TXT_CHAR_COMMENT );
	formatConstant.setObjectType( TXT_CHAR_CONSTANT );
	formatStorage.setObjectType( TXT_CHAR_IDENTIFIER );
	formatTypeStruct.setObjectType( TXT_CHAR_IDENTIFIER );
	formatStmtKey.setObjectType( TXT_CHAR_IDENTIFIER );
	formatStdobj.setObjectType( TXT_CHAR_IDENTIFIER );
	formatInitype.setObjectType( TXT_CHAR_IDENTIFIER );
	formatSymbol.setObjectType( TXT_CHAR_IDENTIFIER );
	formatBracket.setObjectType( TXT_CHAR_SYMBOL );
	formatSBracket.setObjectType( TXT_CHAR_SYMBOL );
	formatCBracket.setObjectType( TXT_CHAR_SYMBOL );
	
	fontSize = 16;
	tabVisibility = 1;
	scheme = 0;
	toktype = 0;
	hlstate = 0;
	textSkip = 0;
	pattern = NULL;
	language = NULL;
	mbs = DString_New(1);
	wcs = DString_New(0);
	tokens = DArray_New( D_TOKEN );
	toks = DArray_New( D_TOKEN );
	SetColorScheme( 0 );
	setCurrentBlockState(0);
}
DaoCodeSHL::~DaoCodeSHL()
{
	DArray_Delete( tokens );
	DArray_Delete( toks );
	DString_Delete( mbs );
	DString_Delete( wcs );
}
void DaoCodeSHL::SetFontSize( int size )
{
	fontSize = size;
	formatOutput.setFontPointSize( size );
	formatPrompt.setFontPointSize( size );
	formatComment.setFontPointSize( size );
	formatConstant.setFontPointSize( size );
	formatStorage.setFontPointSize( size );
	formatTypeStruct.setFontPointSize( size );
	formatStmtKey.setFontPointSize( size );
	formatStdobj.setFontPointSize( size );
	formatBracket.setFontPointSize( size );
	formatSBracket.setFontPointSize( size );
	formatCBracket.setFontPointSize( size );
	formatInitype.setFontPointSize( size );
	formatSymbol.setFontPointSize( size );
}
void DaoCodeSHL::SetFontFamily( const QString & family )
{
	formatOutput.setFontFamily( family );
	formatPrompt.setFontFamily( family );
	formatComment.setFontFamily( family );
	formatConstant.setFontFamily( family );
	formatStorage.setFontFamily( family );
	formatTypeStruct.setFontFamily( family );
	formatStmtKey.setFontFamily( family );
	formatStdobj.setFontFamily( family );
	formatBracket.setFontFamily( family );
	formatSBracket.setFontFamily( family );
	formatCBracket.setFontFamily( family );
	formatInitype.setFontFamily( family );
	formatSymbol.setFontFamily( family );
}
void DaoCodeSHL::SetColorScheme( int scheme )
{
	this->scheme = scheme;
	if( scheme <= 1 ){
		plainColor = Qt::black;
		searchColor = QColor( 150, 150, 250 );
		formatPrompt.setForeground( QColor( 160, 120, 160 ) );
		formatComment.setForeground( QColor( 100, 80, 250 ) );
		formatConstant.setForeground( QColor( 200, 50, 50 ) );
		formatStorage.setForeground( QColor( 120, 150, 0 ) );
		formatTypeStruct.setForeground( QColor( 0, 150, 0 ) );
		formatStmtKey.setForeground( QColor( 180, 120, 0 ) );
		formatStdobj.setForeground( QColor( 0, 140, 120 ) );
		formatBracket.setForeground( QColor(  0, 100, 150 ) );
		formatSBracket.setForeground( QColor( 100, 150, 0 ) );
		formatCBracket.setForeground( QColor( 150, 0, 100 ) );
		formatInitype.setForeground( QColor( 40, 40, 0 ) );
		formatSymbol.setForeground( QColor( 0, 40, 40 ) );
	}else{
		plainColor = QColor( 220, 220, 220 );;
		plainColor = Qt::white;
		searchColor = QColor( 0, 0, 100 );
		formatPrompt.setForeground( QColor( 180, 120, 180 ) );
		formatComment.setForeground( QColor( 150, 120, 250 ) );
		formatConstant.setForeground( QColor( 200, 50, 50 ) );
		formatStorage.setForeground( QColor( 120, 150, 0 ) );
		formatTypeStruct.setForeground( QColor( 0, 180, 0 ) );
		formatStmtKey.setForeground( QColor( 200, 150, 0 ) );
		formatStdobj.setForeground( QColor( 0, 150, 130 ) );
		formatBracket.setForeground( QColor(  0, 130, 170 ) );
		formatSBracket.setForeground( QColor( 130, 170, 0 ) );
		formatCBracket.setForeground( QColor( 170, 0, 130 ) );
		formatInitype.setForeground( QColor( 255, 255, 170 ) );
		formatSymbol.setForeground( QColor( 180, 255, 255 ) );
	}
	formatOutput.setForeground( plainColor );
	SetTabVisibility( tabVisibility );
}
void DaoCodeSHL::SetTabVisibility( int vid )
{
	tabVisibility = vid;
	int bgcolor = 255;
	switch( scheme ){
	case 0 : bgcolor = 255; break;
	case 1 : bgcolor = 221; break;
	case 2 : bgcolor = 68; break;
	case 3 : bgcolor = 0; break;
	default : break;
	}
	int vis[] = { 0, 16, 32, 64 };
	int dif[] = { 0, 4, 6, 8 };
	int base = 0;
	if( scheme <=1 ){
		base = bgcolor - vis[vid];
	}else{
		base = bgcolor + vis[vid];
	}
	tabColor[0] = QColor( base, base+dif[vid], base );
	tabColor[1] = QColor( base, base, base+dif[vid] );
}
DaoTokenFormat DaoCodeSHL::ColorGroup( int group )
{
	switch( group ){
	case DAO_SHL_COLOR1 : return formatComment;
	case DAO_SHL_COLOR2 : return formatConstant;
	case DAO_SHL_COLOR3 : return formatTypeStruct;
	case DAO_SHL_COLOR4 : return formatStmtKey;
	case DAO_SHL_COLOR5 : return formatStorage;
	case DAO_SHL_COLOR6 : return formatStdobj;
	case DAO_SHL_COLOR7 : return formatBracket;
	case DAO_SHL_COLOR8 : return formatSBracket;
	case DAO_SHL_COLOR9 : return formatCBracket;
	}
	return DaoTokenFormat();
}
void DaoCodeSHL::SetState( int state )
{
	//printf( "SetState: %i %i\n", state, toktype );
	if( state >= 0 ){
		hlstate = state;
		if( state <= DAO_HLSTATE_NORMAL ) setCurrentBlockState(0);
		return;
	}
	switch( toktype ){
	case DTOK_CMT_OPEN : hlstate = DAO_HLSTATE_COMMENT; break;
	case DTOK_MBS_OPEN : hlstate = DAO_HLSTATE_MBS; break;
	case DTOK_WCS_OPEN : hlstate = DAO_HLSTATE_WCS; break;
	default : hlstate = DAO_HLSTATE_NORMAL; break;
	}
}
DaoRegex* DaoCodeSHL::SetPattern( DaoRegex *pat )
{
	//bool redo = pat != pattern;
	DaoRegex *oldpat = pattern;
	pattern = pat;
	return oldpat;
	//if( redo ) rehighlight(); // problem: it will mark text being changed
}
QString DaoCodeSHL::SetLanguage( const QString &lang )
{
	if( languages.contains( lang ) ){
		language = languages[lang];
		return language->language;
	}
	return "";
}
void DaoCodeSHL::HighlightSearch( const QString & text )
{
	if( pattern ){
		QByteArray bytes = text.toUtf8();
		DString_SetDataMBS( wcs, bytes.data(), bytes.size() );
		size_t start = 0;
		size_t end = wcs->size;
		size_t p1 = start, p2 = end;
		size_t i;
		while( DaoRegex_Match( pattern, wcs, & p1, & p2 ) ){
			for(i=p1; i<=p2; i++){
				QTextCharFormat fmt = format( i );
				QColor fore = fmt.foreground().color();
				QColor back = plainColor;
				fore = QColor( 255-fore.red(), 255-fore.green(), 255-fore.blue() );
				int red = back.red() + 100;
				int green = back.green() + 100;
				int blue = back.blue() - 150;
				if( red >= 255 ) red = 255;
				if( green >= 255 ) green = 255;
				if( blue <0 ) blue = 0;
				back = QColor( red, green, blue );
				fmt.setForeground( fore );
				fmt.setBackground( back );
				if( i == p1 ) fmt.setProperty( 0x100001, 1 );
				setFormat( i, 1, fmt );
			}
			p1 = p2 + 1;
			p2 = end;
		}
	}
}
void DaoCodeSHL::SetIndentationData( DaoCodeLineData *ud, DArray *tokens )
{
	int i, j;
	bool leading = true;
	ud->leadSpaces = ud->leadTabs = 0;
	ud->firstToken = 0;
	for(i=0; i<tokens->size; i++){
		DaoToken *tk = tokens->items.pToken[i];
		if( leading ){
			ud->leadSpaces += (tk->type == DTOK_BLANK);
			ud->leadTabs += (tk->type == DTOK_TAB);
			leading = (tk->type == DTOK_BLANK or tk->type == DTOK_TAB);
			if( leading == false and tk->type != DTOK_NEWLN ){
				ud->firstToken = tk->type;
				break;
			}
		}
	}
}
void DaoCodeSHL::HighlightNormal( const QString & text )
{
	QTextBlock block = currentBlock();
	DaoTokenFormat format2;
	int start = block.position();
	if( start + block.length() < textSkip ) return;
	int cmt, offset = 0;
	size_t i, j, k;
	QString src = "";
	switch( previousBlockState() ){
	case DAO_HLSTATE_MBS : src = "'"; break;
	case DAO_HLSTATE_WCS : src = "\""; break;
	case DAO_HLSTATE_CMT_OPEN1 : src = language->cmtOpen1->mbs; break;
	case DAO_HLSTATE_CMT_OPEN2 : src = language->cmtOpen2->mbs; break;
	}
	offset = src.size();
	src += text;
	cmt = language->Tokenize( tokens, src.toLocal8Bit().data() );

	if( offset ) DString_Erase( tokens->items.pToken[0]->string, 0, offset );

	toktype = 0;
	if( tokens->size ) toktype = tokens->items.pToken[tokens->size-1]->type;
	//printf( "--------- %i  %i  %i\n", previousBlockState(), tokens->size, toktype );
	switch( toktype ){
	case DTOK_CMT_OPEN :
		setCurrentBlockState( DAO_HLSTATE_CMT_OPEN1 + (cmt-1) ); break;
	case DTOK_MBS_OPEN : setCurrentBlockState( DAO_HLSTATE_MBS ); break;
	case DTOK_WCS_OPEN : setCurrentBlockState( DAO_HLSTATE_WCS ); break;
	default : setCurrentBlockState(0);
	}
	DaoCodeLineData *ud = (DaoCodeLineData*) currentBlockUserData();
	if( ud == NULL ) setCurrentBlockUserData( new DaoCodeLineData(false, CLS_COMMAND ) );
	ud = (DaoCodeLineData*) currentBlockUserData();

	bool def_class = ud->def_class;
	bool def_method = ud->def_method;
	DString_Resize( wcs, src.size() );
	src.toWCharArray( wcs->wcs );
	ud->def_class = ud->def_method = false;
	if( language->func_regex && DaoRegex_Match( language->func_regex, wcs, NULL, NULL ) )
		ud->def_method = true;
	if( language->class_regex && DaoRegex_Match( language->class_regex, wcs, NULL, NULL ) )
		ud->def_class = true;

	if( def_class != ud->def_class or def_method != ud->def_method ) emit signalUpdateOutline();

	int oldFontSize = fontSize;
	if( fontSize < 10 and (ud->def_class or ud->def_method) ){
		//ud->font_size = 15;
		//SetFontSize( 15 );
	}
	format2.setFontPointSize( fontSize );
	format2.setForeground( plainColor );
	setFormat( 0, text.size(), format2 );

	int pos = 0;
	for(i=0; i<tokens->size; i++){
		DaoToken *tk = tokens->items.pToken[i];
		DaoTokenFormat format;
		//printf( "%3i: %s\n", i, tk->string->mbs );
		format.setFontPointSize( fontSize );
		format.setForeground( plainColor );
		DString_SetDataMBS( wcs, tk->string->mbs, tk->string->size );
		//printf( "%4i:  %3i  %s\n", i, tk->type, tk->string->mbs );
		switch( tk->type ){
		case DTOK_BLANK : case DTOK_TAB :
			format.setObjectType( 0 );
			if( tk->type == DTOK_TAB ) format.setBackground( tabColor[i%2] );
			break;
		case DTOK_MBS : case DTOK_WCS :
		case DTOK_MBS_OPEN : case DTOK_WCS_OPEN :
		case DTOK_DIGITS_HEX : case DTOK_DIGITS_DEC :
		case DTOK_NUMBER_HEX : case DTOK_NUMBER_DEC : case DTOK_NUMBER_SCI :
			format = formatConstant;
			break;
		case DTOK_CMT_OPEN : case DTOK_COMMENT :
			format = formatComment;
			break;
		case DTOK_LB : case DTOK_RB :
		case DTOK_COLON : case DTOK_SEMCO : case DTOK_COMMA :
		case DTOK_ASSN : case DTOK_FIELD :
			format = formatBracket;
			break;
		case DTOK_LCB : case DTOK_RCB :
			format = formatCBracket;
			break;
		case DTOK_LSB : case DTOK_RSB :
		case DTOK_DOT : case DTOK_ARROW : case DTOK_COLON2 :
			format = formatSBracket;
			break;
		case DTOK_DOLLAR : case DTOK_XOR : 
			format = formatBracket;
			break;
		case DTOK_AMAND :
			format = formatTypeStruct;
			break;
		case DTOK_ID_INITYPE :
			format = formatInitype;
			break;
		case DTOK_ID_SYMBOL :
			format = formatSymbol;
			break;
		case DTOK_IDENTIFIER :
			format.setObjectType( TXT_CHAR_IDENTIFIER );
			if( language->isLatex ){
				if( i ==0 ){
					if( tk->string->size >= 5 )
						words[ QString::fromUtf8( tk->string->mbs ) ] = true;
					break;
				}
				if( strcmp( tokens->items.pToken[i-1]->string->mbs, "\\" ) !=0 ){
					if( tk->string->size >= 5 )
						words[ QString::fromUtf8( tk->string->mbs ) ] = true;
					break;
				}else{
					if( tk->string->size >= 3 )
						words[ "\\" + QString::fromUtf8( tk->string->mbs ) ] = true;
				}
			}
			if( language->caseInsensitive ) DString_ToLower( tk->string );
			if( DMap_Find( language->keyStruct, tk->string ) ){
				format = formatTypeStruct;
			}else if( DMap_Find( language->keyStorage, tk->string ) ){
				format = formatStorage;
			}else if( DMap_Find( language->keyStatement, tk->string ) ){
				format = formatStmtKey;
			}else if( DMap_Find( language->keyConstant, tk->string ) ){
				format = formatConstant;
			}else if( DMap_Find( language->keyOthers, tk->string ) ){
				format = formatPrompt;
			}else if( language->isLatex ){
				format = formatStorage;
			}
			if( language->isLatex ) setFormat( pos-1, 1, format );
			break;
		default:
			format.setObjectType( TXT_CHAR_SYMBOL );
			break;
		}
		if( ud->font_size ) format.setFontPointSize( ud->font_size );
		setFormat( pos, wcs->size, format );
		pos += wcs->size;
	}
	SetIndentationData( ud, tokens );

	DString_SetMBS( wcs, src.toLocal8Bit().data() );
	for(i=0; i<language->patterns.size(); i++){
		DaoSyntaxPattern sp = language->patterns[i];
		int pos, len, type;
		size_t start = 0;
		size_t end = wcs->size-1;
		while( DaoRegex_Match( sp.pattern, wcs, &start, &end ) ){
			size_t s1 = 0, s2 = 0;
			for(j=0; j<8*sizeof(sp.groups); j++){
				if( not (sp.groups & (1<<j) ) ) continue;
				if( DaoRegex_SubMatch( sp.pattern, j, &s1, &s2 ) ==0 ) continue;
				pos = s1 - offset;
				len = s2 - s1 + 1;
				type = format( pos ).objectType();
				while( len && (type == TXT_CHAR_COMMENT || type == TXT_CHAR_CONSTANT) ){
					pos += 1;
					len -= 1;
					type = format( pos ).objectType();
				}
				if( len ) setFormat( pos, len, ColorGroup( sp.color ) );
			}
			start = end + 1;
			end = wcs->size-1;
		}
	}
	SetFontSize( oldFontSize );
}
void DaoCodeSHL::highlightBlock ( const QString & text )
{
	DaoTokenFormat format2;
	format2.setFontPointSize( fontSize );
	format2.setForeground( plainColor );
	setFormat( 0, text.size(), format2 );
	
	if( language and language != DaoBasicSyntax::dao ){
		HighlightNormal( text );
		HighlightSearch( text );
		return;
	}
	DaoCodeLineData *ud = (DaoCodeLineData*) currentBlockUserData();
	if( ud == NULL && hlstate == DAO_HLSTATE_REDO ){
		setFormat( 0, text.size(), formatOutput );
		return;
	}
	if( ud && ud->state == CLS_OUTPUT && language == NULL ){
		setFormat( 0, text.size(), formatOutput );
		return;
	}
	if( hlstate ==0 && language == NULL ){
		setCurrentBlockUserData( NULL );
		setFormat( 0, text.size(), formatOutput );
		return;
	}
	if( ud == NULL ) setCurrentBlockUserData( new DaoCodeLineData(false, CLS_COMMAND ) );
	ud = (DaoCodeLineData*) currentBlockUserData();
	QTextBlock block = currentBlock();
	int start = block.position();
	if( start + block.length() < textSkip ) return;
	int offset = 0;
	QString src = "";
	switch( previousBlockState() ){
	case DAO_HLSTATE_MBS : src = "'"; break;
	case DAO_HLSTATE_WCS : src = "\""; break;
	case DAO_HLSTATE_COMMENT : src = "#{"; break;
	}
	int pos = 0;
	QRegExp regex( "^\\s*\\d+:" );
	if( text.indexOf( regex ) ==0 ){
		DaoTokenFormat format;
		// there is a problem to pring background color over multiple pages:
		// format.setForeground( plainColor );
		// format.setBackground( QColor( 200, 200, 200 ) );
		format.setForeground( QColor( 150, 150, 150 ) );
		pos = regex.matchedLength();
		setFormat( 0, pos, format );
	}
	offset = src.size();
	src += text.mid( pos );
	DaoToken_Tokenize( tokens, src.toLocal8Bit().data(), 0, 1, 1 );
	if( offset ) DString_Erase( tokens->items.pToken[0]->string, 0, offset );
	toktype = 0;
	if( tokens->size ) toktype = tokens->items.pToken[tokens->size-1]->type;
	//printf( "--------- %i  %i  %i\n", previousBlockState(), tokens->size, toktype );
	switch( toktype ){
	case DTOK_CMT_OPEN : setCurrentBlockState( DAO_HLSTATE_COMMENT ); break;
	case DTOK_MBS_OPEN : setCurrentBlockState( DAO_HLSTATE_MBS ); break;
	case DTOK_WCS_OPEN : setCurrentBlockState( DAO_HLSTATE_WCS ); break;
	default : setCurrentBlockState(0);
	}
	bool def_class = ud->def_class;
	bool def_method = ud->def_method;
	DaoBasicSyntax *lang = language;
	if( lang == NULL ) lang = DaoBasicSyntax::dao;
	DString_Resize( wcs, src.size() );
	src.toWCharArray( wcs->wcs );
	ud->def_class = ud->def_method = false;
	if( lang->func_regex && DaoRegex_Match( lang->func_regex, wcs, NULL, NULL ) )
		ud->def_method = true;
	if( lang->class_regex && DaoRegex_Match( lang->class_regex, wcs, NULL, NULL ) )
		ud->def_class = true;

	if( def_class != ud->def_class or def_method != ud->def_method ) emit signalUpdateOutline();

	int oldFontSize = fontSize;
	if( fontSize < 10 and (ud->def_class or ud->def_method) ){
		//ud->font_size = 15;
		//SetFontSize( 15 );
	}
	format2.setFontPointSize( fontSize );
	format2.setForeground( plainColor );
	setFormat( 0, text.size(), format2 );

	for(size_t i=0; i<tokens->size; i++){
		DaoToken *tk = tokens->items.pToken[i];
		DaoTokenFormat format;
		format.setFontPointSize( fontSize );
		format.setForeground( plainColor );
		DString_SetDataMBS( wcs, tk->string->mbs, tk->string->size );
		//printf( "%4i:  %3i  %s\n", i, tk->name, tk->string->mbs );
		switch( tk->name ){
		case DTOK_BLANK : case DTOK_TAB :
			format.setObjectType( 0 );
			if( tk->type == DTOK_TAB ) format.setBackground( tabColor[i%2] );
			break;
		case DTOK_MBS : case DTOK_WCS :
		case DTOK_MBS_OPEN : case DTOK_WCS_OPEN :
		case DTOK_DIGITS_HEX : case DTOK_DIGITS_DEC :
		case DTOK_NUMBER_HEX : case DTOK_NUMBER_DEC : case DTOK_NUMBER_SCI :
			format = formatConstant;
			break;
		case DTOK_CMT_OPEN : case DTOK_COMMENT :
			format = formatComment;
			break;
		case DTOK_LB : case DTOK_RB :
		case DTOK_COLON : case DTOK_SEMCO : case DTOK_COMMA :
		case DTOK_ASSN : case DTOK_FIELD :
			format = formatBracket;
			break;
		case DTOK_LCB : case DTOK_RCB :
			format = formatCBracket;
			break;
		case DTOK_LSB : case DTOK_RSB :
		case DTOK_DOT : case DTOK_ARROW : case DTOK_COLON2 :
			format = formatSBracket;
			break;
		case DKEY_USE : case DKEY_LOAD : case DKEY_IMPORT : case DKEY_BIND :
		case DKEY_REQUIRE : case DKEY_BY : case DKEY_AS : case DKEY_SYNTAX : 
		case DKEY_AND : case DKEY_OR : case DKEY_NOT :
		case DKEY_VIRTUAL :
			format = formatStmtKey;
			break;
		case DKEY_VAR : case DKEY_CONST : case DKEY_STATIC : case DKEY_GLOBAL :
		case DKEY_PRIVATE : case DKEY_PROTECTED : case DKEY_PUBLIC :
			format = formatStorage;
			break;
		case DKEY_IF : case DKEY_ELSE :
		case DKEY_WHILE : case DKEY_DO : case DKEY_UNTIL :
		case DKEY_FOR : case DKEY_IN :
		case DKEY_SKIP : case DKEY_BREAK : case DKEY_CONTINUE :
		case DKEY_TRY : case DKEY_RESCUE :
		case DKEY_RETRY : case DKEY_RAISE :
		case DKEY_RETURN : case DKEY_YIELD :
		case DKEY_SWITCH : case DKEY_CASE : case DKEY_DEFAULT :
			format = formatStmtKey;
			break;
		case DKEY_ANY : case DKEY_ENUM :
		case DKEY_INT : case DKEY_LONG :
		case DKEY_FLOAT : case DKEY_DOUBLE :
		case DKEY_STRING : case DKEY_COMPLEX :
		case DKEY_LIST : case DKEY_MAP : case DKEY_TUPLE : case DKEY_ARRAY :
		case DKEY_CLASS : case DKEY_FUNCTION : case DKEY_ROUTINE : case DKEY_SUB :
		case DKEY_OPERATOR :
		case DKEY_FOLD : case DKEY_UNFOLD : case DKEY_REDUCE : 
		case DKEY_INDEX : case DKEY_SELECT : case DKEY_COUNT :
		case DKEY_EACH : case DKEY_REPEAT : 
		case DKEY_SORT : case DKEY_APPLY :
			format = formatTypeStruct;
			break;
		case DKEY_SELF :
			format = formatStdobj;
			break;
		case DTOK_ID_INITYPE :
			format = formatInitype;
			break;
		case DTOK_ID_SYMBOL :
			format = formatSymbol;
			break;
		case DTOK_IDENTIFIER :
			format.setObjectType( TXT_CHAR_IDENTIFIER );
			if( tk->string->size == 3 && strcmp( tk->string->mbs, "dao" ) ==0 )
				format = formatPrompt;
			if( lang and DMap_Find( lang->keyOthers, tk->string ) ) format = formatPrompt;
			break;
		default:
			format.setObjectType( TXT_CHAR_SYMBOL );
			break;
		}
		if( ud->font_size ) format.setFontPointSize( ud->font_size );
		setFormat( pos, wcs->size, format );
		pos += wcs->size;
	}
	SetIndentationData( ud, tokens );
	//printf( "%3i%3i%3i%3i: %s\n", ud->leadSpaces, ud->leadTabs,
	//	ud->lastToken, ud->lastNoComment, src.toLocal8Bit().data() );
	HighlightSearch( text );
	SetFontSize( oldFontSize );
}


