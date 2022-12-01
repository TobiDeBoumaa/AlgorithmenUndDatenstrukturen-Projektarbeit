// stdafx.h : Include-Datei f�r Standard-System-Include-Dateien,
//  oder projektspezifische Include-Dateien, die h�ufig benutzt, aber
//      in unregelm��igen Abst�nden ge�ndert werden.
//

#pragma once

#include <cstdio>
#include "stdio.h"
#include <string>
#include <map>

#include <memory.h>

class CParser
{
public:
	std::string yytext; // input buffer
	struct tyylval
	{			  // value return
		std::string s; // structure
		int i;
	} yylval;
	FILE *IP_Input;						// Input File
	FILE *IP_Error;						// Error Output
	FILE *IP_List;						// List Output
	int IP_LineNumber;					// Line counter
	int ugetflag;						// checks ungets
	int prflag;							// controls printing
	std::map<std::string, int> IP_Token_table;	// Tokendefinitions
	std::map<int, std::string> IP_revToken_table; // reverse Tokendefinitions

	int yylex();					// lexial analyser
	void yyerror(char *ers);		// error reporter
	int IP_MatchToken(std::string &tok); // checks the token
	void InitParse(FILE *inp, FILE *err, FILE *lst);
	int yyparse();								 // parser
	void pr_tokentable();						 // test output for tokens
	void IP_init_token_table();					 // loads the tokens
	void Load_tokenentry(std::string str, int index); // load one token
	void PushString(char c);					 // Used for dtring assembly
	CParser()
	{
		IP_LineNumber = 1;
		ugetflag = 0;
		prflag = 0;
	}; // Constructor
};
//------------------------------------------------------------------------
