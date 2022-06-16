#include "Lexer.hpp"

/**
 * @brief Function to return the next token from standard input
 *
 * the variable 'm_IdentifierStr' is set there in case of an identifier,
 * the variable 'm_NumVal' is set there in case of a number.
 */

void Lexer::readInput(void) {
    character = getchar();
    if ((character>='A' && character<='Z') || (character>='a' && character<='z') || character=='_')
        input = LETTER;
    else if (character>='0' && character<='9')
        input = NUMBER;
    else if (character == EOF)
        input = END;
    else if (character <= ' ')
        input = WHITE_SPACE;
    else
        input = NO_TYPE;
}


const char oneChSymbTable[13] = {
        '+','-','*','/','=','.',',',';', '(', ')', '[', ']', 0
};

const struct {std::string slovo; Token symb;} keyWordTable[] = {
        {"begin", tok_begin},
        {"end", tok_end},
        {"const", tok_const},
        {"procedure", tok_procedure},
        {"forward", tok_forward},
        {"function", tok_function},
        {"if", tok_if},
        {"then", tok_then},
        {"else", tok_else},
        {"program", tok_program},
        {"while", tok_while},
        {"exit", tok_exit},
        {"var", tok_var},
        {"integer", tok_integer},
        {"for", tok_for},
        {"do", tok_do},
        {"or", tok_or},
        {"mod", tok_mod},
        {"div", tok_div},
        {"not", tok_not},
        {"and", tok_and},
        {"xor", tok_xor},
        {"break", tok_break},
        {"to", tok_to},
        {"downto", tok_downto},
        {"", (Token) 0}
};

int keyWord(std::string id) {
    int i = 0;
    while (!keyWordTable[i].slovo.empty())
        if (id==keyWordTable[i].slovo)
            return keyWordTable[i].symb;
        else
            i++;
    return tok_identifier;
}
int isOneChTok(char character){
    int i = 0;
    while (oneChSymbTable[i])
        if(character==oneChSymbTable[i])
            return 1;
        else
            i++;
    return 0;
}
int Lexer::gettok()
{
//    std::cin >> m_NumVal;
//    return tok_number;

    int digit;
    q0:
    switch (character) {
        case '{':
            readInput();
            goto q1;
        case '<':
            readInput();
            goto q4;
        case '>':
            readInput();
            goto q6;
        case ':':
            readInput();
            goto q5;
        case '$':
            base=16;
            this->m_NumVal=0;
            readInput();
            goto q3;
        case '&':
            base=8;
            this->m_NumVal=0;
            readInput();
            goto q3;
        default:
            if(isOneChTok(character)) {
                char ch = character;
                readInput();
                return ch;
            }
    }
    switch (input) {
        case WHITE_SPACE:
            readInput();
            goto q0;
        case END:
            return tok_eof;
        case LETTER:
            this->m_IdentifierStr=(char)character;
            readInput();
            goto q2;
        case NUMBER:
            base=10;
            this->m_NumVal=character-'0';
            readInput();
            goto q3;
        default:
            //printf("Invalid symbol.");
            readInput();
            goto q0;
           // return tok_eof;
    }

    q1:
    switch(character) {
        case '}':
            readInput();
            goto q0;
        case '\\':
            readInput();
            goto q11;
        default:;
    }
    switch(input) {
        case END:
            printf("Uneexpected end of file in a comment.");
            return tok_eof;
        default:
            readInput();
            goto q1;
    }

    q11:
    switch(character) {
        case '}':
            readInput();
            goto q1;
        case '\\':
            readInput();
            goto q1;
        default:;
            printf("Unexpected escape symbol \\.");
            return tok_eof;
    }

    q2:
    switch(input) {
        case LETTER:
        case NUMBER:
            m_IdentifierStr+=character;
            readInput();
            goto q2;
        default:
            return keyWord(m_IdentifierStr);
    }

    q3:
    switch(input) {
        case NUMBER:
        case LETTER:
            if (character >= 'a') {
                digit = (character - 'a') + 10;
            } else if (character >= 'A') {
                digit = (character - 'A') + 10;
            } else {
                digit = (character - '0');
            }
            if (digit >= base) {
                printf("Digit %c not allowed in %d base!",character,base);
                exit(1);
            }

            m_NumVal = base * m_NumVal + digit;
            readInput();
            goto q3;
        default:
            return tok_number;
    }

    q4:
    switch(character) {
        case '=':
            readInput();
            return tok_lessequal;
        case '>':
            readInput();
            return tok_notequal;
        default:;
    }
    switch(input) {
        default:
            return '<';
    }

    q5:
    switch(character) {
        case '=':
            readInput();
            return tok_assign;
        default:;
    }
    switch(input) {
        default:
            return ':';
    }

    q6:
    switch(character) {
        case '=':
            readInput();
            return tok_greaterequal;
        default:;
    }
    switch(input) {
        default:
            return '>';
    }


}

