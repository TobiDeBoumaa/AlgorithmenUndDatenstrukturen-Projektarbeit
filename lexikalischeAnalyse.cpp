// k7scan1.cpp : Definiert den Einsprungpunkt f�r die Konsolenanwendung.
//

#include "lexikalischeAnalyse.hpp"

using namespace std;

#define Getc(s) getc(s)
#define Ungetc(c)                                                              \
  {                                                                            \
    ungetc(c, IP_Input);                                                       \
    ugetflag = 1;                                                              \
  }

/*
 *	Lexical analyzer states.
 */
enum lexstate { L_START, L_END, L_MACADDRESS, L_DISPLAYOPTION, L_SEND };
const char* lexstateEnumNames[] =
{
    "L_START",
    "L_END",
    "L_MACADDRESS","L_DISPLAYOPTION","L_SEND"
};

const int MACADDRESS = 3;
const int DISPLAYOPTION = 4;
const int END = 5;
const int SEND = 100;

// Adds a character to the string value
void CParser::PushString(char c) { yylval.s += c; }
//------------------------------------------------------------------------
void CParser::Load_tokenentry(string str, int index) {
  IP_Token_table[str] = index;
  IP_revToken_table[index] = str;
}
void CParser::IP_init_token_table() {
  Load_tokenentry("MACADDRESS", MACADDRESS);
  Load_tokenentry("DISPLAYOPTION", DISPLAYOPTION);
  Load_tokenentry("End", END);
  Load_tokenentry("SEND", SEND);
}
//------------------------------------------------------------------------

void CParser::pr_tokentable() {

  typedef map<string, int>::const_iterator CI;
  const char *buf;

  printf("Symbol Table ---------------------------------------------\n");

  for (CI p = IP_Token_table.begin(); p != IP_Token_table.end(); ++p) {
    buf = p->first.c_str();
    printf(" key:%s", buf);
    printf(" val:%d\n", p->second);
    ;
  }
}
//------------------------------------------------------------------------

void CParser::yyparse(std::function<void(arp_paket)> func, std::function<void(std::string)> errorFunc,char* input) {
  int tok;
  yylval.s = "";
  errorFunction = errorFunc;
  stringstream inputStream;
  inputStream << input;
  if (prflag)
    fprintf(IP_List, "%5d ", (int)IP_LineNumber);
  /*
   *	Go parse things!
   */
  while ((tok = yylex(inputStream)) != 0) {
    printf("%d ", tok);
    switch (tok) {
    case SEND:
      printf("Sending Pseudo Arp Package");
      func({yylval.address, yylval.displayopt});
      break;
    case MACADDRESS: // Mac Address
      printf("Mac address: %s\n", yylval.address.macString.c_str());
      break;
    case DISPLAYOPTION: // Display Options
      printf("Display option: %d\n", yylval.displayopt);
      break;
    case END: // End
      printf("%s\n", IP_revToken_table[tok].c_str());
      exit(EXIT_SUCCESS);
      break;
    default:
      printf("unsupported id %c", tok);
      break;
    }
    if (!prflag)
      printf("\n");
  }
  func({ yylval.address, yylval.displayopt });
}
//------------------------------------------------------------------------

/*
 *	Parse Initfile:
 *
 *	  This builds the context tree and then calls the real parser.
 *	It is passed two file streams, the first is where the input comes
 *	from; the second is where error messages get printed.
 */
void CParser::InitParse(FILE *inp, FILE *err, FILE *lst)

{

  /*
   *	Set up the file state to something useful.
   */
  IP_Input = inp;
  IP_Error = err;
  IP_List = lst;

  IP_LineNumber = 1;
  ugetflag = 0;
  /*
   *	Define both the enabled token and keyword strings.
   */
  IP_init_token_table();
}
//------------------------------------------------------------------------

/*
 *	yyerror:
 *
 *	  Standard error reporter, it prints out the passed string
 *	preceeded by the current filename and line number.
 */
void CParser::yyerror(char *ers)

{
  fprintf(IP_Error, "line %d: %s\n", IP_LineNumber, ers);
}
//------------------------------------------------------------------------

int CParser::IP_MatchToken(string &tok) {
  int retval;
  if (IP_Token_table.find(tok) != IP_Token_table.end()) {
    retval = (IP_Token_table[tok]);
  } else {
    retval = (0);
  }
  return retval;
}

//------------------------------------------------------------------------

/*
 *	yylex:
 *
 */

int CParser::yylex(std::stringstream &inputStream) {
    char c;
    lexstate s;
    //printf("Starting cond %s;;%s\n", yytext.c_str(), yylval.s.c_str());
    for (s = L_START, yytext = ""; 1;) {
        inputStream.get(c);
    if(!inputStream.eof())
      yytext = yytext + (char)c;
    if (!ugetflag) {
      if (c != EOF)
        if (prflag)
          fprintf(IP_List, "%c", c);
    } else
      ugetflag = 0;
    printf("%i Switching on %c (current enum state:%s) entire string:%s\n", inputStream.eof(), c, lexstateEnumNames[s], inputStream.str().c_str());
    switch (s) {
      // Starting state, look for something resembling a token.
    case L_START: {
      if (inputStream.eof()) {
        return ('\0');
      }else if (isxdigit(c)) {
        s = L_MACADDRESS;
      } else if (c == '/') {
        s = L_DISPLAYOPTION;
      } else if (toupper(c) == 'E') {
        s = L_END;
      } else if (isspace(c)) {
          if (c == '\n') {
          s = L_SEND;
          IP_LineNumber += 1;
          if (prflag)
            fprintf(IP_List, "%5d ", (int)IP_LineNumber);
        }
        yytext = "";
      } else {
        return (c);
      }
      break;
    }
    case L_END: {
      if (toupper(c) == 'N' || toupper(c) == 'D')
        break;
      else if (isxdigit(c))
        s = L_MACADDRESS;
      else
        return (c);
      break;
    }
    case L_SEND: {

      return SEND;
      break;
    }
    case L_DISPLAYOPTION: {
      if ((isdigit(c) || c == ' ')&& !inputStream.eof()) {
        break;
      } else {
          
        //Ungetc(c);
        yylval.s = yytext.substr(1, yytext.size());
        yylval.displayopt = atoi(yylval.s.c_str());
        
        if (yylval.displayopt > 2) {
            errorFunction("There are no displayoptions over 2");
            yylval.displayopt = 0;
        }
        return (DISPLAYOPTION);
      }
    }
    case L_MACADDRESS: {
      if ((isxdigit(c) || c == ':') && !inputStream.eof()) {
        break;
      } else {
        if ((yytext.size() - yylval.s.length())+ inputStream.eof() != 18) {
            std::ostringstream errorStringStream;
            errorStringStream << "MAC not in correct form (lenght=" << (yytext.size() - yylval.s.length())<<", expected 18)";
            errorFunction(errorStringStream.str());
            return(0);
        }
        yylval.s = yytext.substr(yylval.s.length(), yytext.size() - 1 + inputStream.eof());
        yylval.address = yylval.s;
        return (MACADDRESS);
      }
    }
    }
  }
}
//------------------------------------------------------------------------
