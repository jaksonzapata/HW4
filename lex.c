/*
Assignment:
HW4 - Complete Parser and Code Generator for PL/0
(with Procedures, Call, and Else)
Author(s): <Full Name 1>, <Full Name 2>
Language: C (only)
To Compile:
Scanner:
gcc -O2 -std=c11 -o lex lex.c
Parser/Code Generator:
gcc -O2 -std=c11 -o parsercodegen_complete parsercodegen_complete.c
Virtual Machine:
gcc -O2 -std=c11 -o vm vm.c
To Execute (on Eustis):
./lex <input_file.txt>
./parsercodegen_complete
./vm elf.txt
where:
<input_file.txt> is the path to the PL/0 source program
Notes:
- lex.c accepts ONE command-line argument (input PL/0 source file)
- parsercodegen_complete.c accepts NO command-line arguments
- Input filename is hard-coded in parsercodegen_complete.c
- Implements recursive-descent parser for extended PL/0 grammar
- Supports procedures, call statements, and if-then-else
- Generates PM/0 assembly code (see Appendix A for ISA)
- VM must support EVEN instruction (OPR 0 11)
- All development and testing performed on Eustis
Class: COP3402 - System Software - Fall 2025
Instructor: Dr. Jie Lin
Due Date: Friday, November 21, 2025 at 11:59 PM ET
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define MAX_TOKENS 1000
#define MAX_IDENT_LEN 11
#define MAX_NUM_LEN 5
#define MAX_SOURCE_SIZE 100000
#define MAX_LEXEME_LEN 256

// TokenType Enumeration
typedef enum
{
    skipsym = 1,
    identsym,
    numbersym,
    plussym,
    minussym,
    multsym,
    slashsym,
    eqsym,
    neqsym,
    lessym,
    leqsym,
    gtrsym,
    geqsym,
    lparentsym,
    rparentsym,
    commasym,
    semicolonsym,
    periodsym,
    becomessym,
    beginsym,
    endsym,
    ifsym,
    fisym,
    thensym,
    whilesym,
    dosym,
    callsym,
    constsym,
    varsym,
    procsym,
    writesym,
    readsym,
    elsesym,
    evensym
} TokenType;

// Token structure
typedef struct
{
    char lexeme[MAX_LEXEME_LEN];
    TokenType type;
    char error[50];
} Token;

// Globals
Token tokens[MAX_TOKENS];
int tokenCount = 0;
char sourceProgram[MAX_SOURCE_SIZE];
int sourceLen = 0;

// Prototypes
void lexicalAnalyzer(FILE *source);
TokenType isKeyword(const char *word);
void addToken(const char *lexeme, TokenType type, const char *error);
void printOutput();
void readSourceProgram(FILE *source);
const char *tokenName(TokenType type);

// Main
int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: ./lex <input file>\n");
        return 1;
    }

    FILE *source = fopen(argv[1], "r");
    if (source == NULL)
    {
        perror("Error opening file");
        return 1;
    }

    readSourceProgram(source);
    fseek(source, 0, SEEK_SET);

    lexicalAnalyzer(source);
    fclose(source);

    printOutput();

    return 0;
}

// Read entire source program
void readSourceProgram(FILE *source)
{
    int ch;
    while ((ch = fgetc(source)) != EOF && sourceLen < MAX_SOURCE_SIZE - 1)
    {
        sourceProgram[sourceLen++] = ch;
    }
    sourceProgram[sourceLen] = '\0';
}

// Lexical analysis
void lexicalAnalyzer(FILE *source)
{
    int ch;
    char buffer[MAX_LEXEME_LEN];
    int bufferIndex = 0;
    int inComment = 0;

    while ((ch = fgetc(source)) != EOF)
    {
        // Handle comments
        if (!inComment && ch == '/')
        {
            int next = fgetc(source);
            if (next == '*')
            {
                // Add /* delimiters as tokens
                addToken("/", slashsym, NULL);
                addToken("*", multsym, NULL);
                inComment = 1;
                continue;
            }
            else
            {
                if (next != EOF)
                    ungetc(next, source); // guard
                addToken("/", slashsym, NULL);
                continue;
            }
        }

        if (inComment)
        {
            if (ch == '*')
            {
                int next = fgetc(source);
                if (next == '/')
                {
                    // Add */ delimiters as tokens
                    addToken("*", multsym, NULL);
                    addToken("/", slashsym, NULL);
                    inComment = 0;
                }
                else
                {
                    if (next != EOF)
                        ungetc(next, source); // guard
                }
            }
            continue;
        }

        // Skip whitespace
        if (isspace(ch))
            continue;

        // Identifiers and keywords
        if (isalpha(ch))
        {
            bufferIndex = 0;
            buffer[bufferIndex++] = ch;
            while ((ch = fgetc(source)) != EOF && (isalnum(ch)))
            {
                if (bufferIndex < MAX_LEXEME_LEN - 1)
                    buffer[bufferIndex++] = ch;
            }
            buffer[bufferIndex] = '\0';
            if (ch != EOF)
                ungetc(ch, source); // guard

            if (bufferIndex > MAX_IDENT_LEN)
                addToken(buffer, skipsym, "Identifier too long");
            else
            {
                TokenType keywordType = isKeyword(buffer);
                if (keywordType != identsym)
                    addToken(buffer, keywordType, NULL);
                else
                    addToken(buffer, identsym, NULL);
            }
        }
        // Numbers
        else if (isdigit(ch))
        {
            bufferIndex = 0;
            buffer[bufferIndex++] = ch;
            while ((ch = fgetc(source)) != EOF && isdigit(ch))
            {
                if (bufferIndex < MAX_LEXEME_LEN - 1)
                    buffer[bufferIndex++] = ch;
            }
            buffer[bufferIndex] = '\0';
            if (ch != EOF)
                ungetc(ch, source); // guard

            if (bufferIndex > MAX_NUM_LEN)
                addToken(buffer, skipsym, "Number too long");
            else
                addToken(buffer, numbersym, NULL);
        }
        // Special symbols
        else if (ch == '+')
            addToken("+", plussym, NULL);
        else if (ch == '-')
            addToken("-", minussym, NULL);
        else if (ch == '*')
            addToken("*", multsym, NULL);
        else if (ch == '=')
            addToken("=", eqsym, NULL);
        else if (ch == '<')
        {
            int next = fgetc(source);
            if (next == '>')
                addToken("<>", neqsym, NULL);
            else if (next == '=')
                addToken("<=", leqsym, NULL);
            else
            {
                if (next != EOF)
                    ungetc(next, source); // guard
                addToken("<", lessym, NULL);
            }
        }
        else if (ch == '>')
        {
            int next = fgetc(source);
            if (next == '=')
                addToken(">=", geqsym, NULL);
            else
            {
                if (next != EOF)
                    ungetc(next, source); // guard
                addToken(">", gtrsym, NULL);
            }
        }
        else if (ch == ':')
        {
            int next = fgetc(source);
            if (next == '=')
                addToken(":=", becomessym, NULL);
            else
            {
                if (next != EOF)
                    ungetc(next, source); // guard
                char invalidChar[2] = {ch, '\0'};
                addToken(invalidChar, skipsym, "Invalid symbol");
            }
        }
        else if (ch == '(')
            addToken("(", lparentsym, NULL);
        else if (ch == ')')
            addToken(")", rparentsym, NULL);
        else if (ch == ',')
            addToken(",", commasym, NULL);
        else if (ch == ';')
            addToken(";", semicolonsym, NULL);
        else if (ch == '.')
            addToken(".", periodsym, NULL);
        else
        {
            char invalidChar[2] = {ch, '\0'};
            addToken(invalidChar, skipsym, "Invalid symbol");
        }
    }
}

// Keywords
TokenType isKeyword(const char *word)
{
    if (strcmp(word, "begin") == 0)
        return beginsym;
    if (strcmp(word, "end") == 0)
        return endsym;
    if (strcmp(word, "if") == 0)
        return ifsym;
    if (strcmp(word, "fi") == 0)
        return fisym;
    if (strcmp(word, "then") == 0)
        return thensym;
    if (strcmp(word, "while") == 0)
        return whilesym;
    if (strcmp(word, "do") == 0)
        return dosym;
    if (strcmp(word, "call") == 0)
        return callsym;
    if (strcmp(word, "const") == 0)
        return constsym;
    if (strcmp(word, "var") == 0)
        return varsym;
    if (strcmp(word, "procedure") == 0)
        return procsym;
    if (strcmp(word, "write") == 0)
        return writesym;
    if (strcmp(word, "read") == 0)
        return readsym;
    if (strcmp(word, "else") == 0)
        return elsesym;
    if (strcmp(word, "even") == 0)
        return evensym;
    return identsym;
}

// Add a token
void addToken(const char *lexeme, TokenType type, const char *error)
{
    if (tokenCount < MAX_TOKENS)
    {
        strcpy(tokens[tokenCount].lexeme, lexeme);
        tokens[tokenCount].type = type;
        if (error != NULL)
            strcpy(tokens[tokenCount].error, error);
        else
            tokens[tokenCount].error[0] = '\0';
        tokenCount++;
    }
}

// Token name mapping
const char *tokenName(TokenType type)
{
    switch (type)
    {
    case skipsym:
        return "skipsym";
    case identsym:
        return "identsym";
    case numbersym:
        return "numbersym";
    case plussym:
        return "plussym";
    case minussym:
        return "minussym";
    case multsym:
        return "multsym";
    case slashsym:
        return "slashsym";
    case eqsym:
        return "eqsym";
    case neqsym:
        return "neqsym";
    case lessym:
        return "lessym";
    case leqsym:
        return "leqsym";
    case gtrsym:
        return "gtrsym";
    case geqsym:
        return "geqsym";
    case lparentsym:
        return "lparentsym";
    case rparentsym:
        return "rparentsym";
    case commasym:
        return "commasym";
    case semicolonsym:
        return "semicolonsym";
    case periodsym:
        return "periodsym";
    case becomessym:
        return "becomessym";
    case beginsym:
        return "beginsym";
    case endsym:
        return "endsym";
    case ifsym:
        return "ifsym";
    case fisym:
        return "fisym";
    case thensym:
        return "thensym";
    case whilesym:
        return "whilesym";
    case dosym:
        return "dosym";
    case callsym:
        return "callsym";
    case constsym:
        return "constsym";
    case varsym:
        return "varsym";
    case procsym:
        return "procsym";
    case writesym:
        return "writesym";
    case readsym:
        return "readsym";
    case elsesym:
        return "elsesym";
    case evensym:
        return "evensym";
    default:
        return "UNKNOWN";
    }
}

void printOutput()
{
    printf("\nSource Program:\n\n");
    printf("%s\n", sourceProgram);

    printf("\nLexeme Table:\n\n");
    printf("lexeme\t\ttoken type\n");

    for (int i = 0; i < tokenCount; i++)
    {
        printf("%s\t\t", tokens[i].lexeme);
        if (tokens[i].error[0] != '\0')
            printf("%s", tokens[i].error);
        else
            printf("%d", tokens[i].type); // print numeric code
        printf("\n");
    }

    printf("\nToken List:\n\n");
    for (int i = 0; i < tokenCount; i++)
    {
        printf("%d", tokens[i].type);
        if ((tokens[i].type == identsym || tokens[i].type == numbersym) && tokens[i].error[0] == '\0')
        {
            printf(" %s", tokens[i].lexeme);
        }
        if (i < tokenCount - 1)
            printf(" ");
    }
    printf("\n");

    /* Also write the numeric token stream to a hard-coded tokens.txt file
       so parsercodegen can open it without requiring redirection. */
    FILE *tokf = fopen("tokens.txt", "w");
    if (tokf != NULL)
    {
        for (int i = 0; i < tokenCount; i++)
        {
            fprintf(tokf, "%d", tokens[i].type);
            if ((tokens[i].type == identsym || tokens[i].type == numbersym) && tokens[i].error[0] == '\0')
            {
                fprintf(tokf, " %s", tokens[i].lexeme);
            }
            if (i < tokenCount - 1)
                fprintf(tokf, " ");
        }
        fprintf(tokf, "\n");
        fclose(tokf);
    }
    else
    {
        perror("Error creating tokens.txt");
    }
}