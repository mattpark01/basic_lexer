from enum import Enum
from dataclasses import dataclass

input_code = '''
"example" // STRING
{ // LEFT_BRACE
} // RIGHT_BRACE
( // LEFT_PAREN
) // RIGHT_PAREN
; // SEMICOLON
+ // PLUS
- // MINUS
* // STAR
/ // SLASH
= // EQUAL
== // EQUAL_EQUAL
! // BANG
!= // BANG_EQUAL
Hello // IDENTIFIER, assuming your lexer can recognize identifiers
, // Unspecified in TokenType, assuming part of general syntax or identifiers
world! // Unspecified in TokenType, assuming part of general syntax or identifiers
'''

TokenType = Enum(
    'TokenType',
    [
        # the first group of tokens are unambiguous single-characters
        'SEMICOLON',
        'LEFT_PAREN',
        'RIGHT_PAREN',
        'LEFT_BRACE', 'RIGHT_BRACE', 
        'PLUS', 'MINUS', 'STAR', 'SLASH',
        # the following tokesn are 1- or 2-characters with short prefix
        'EQUAL', 'EQUAL_EQUAL', # = vs ==
        'BANG', 'BANG_EQUAL', # ! vs !=
        # Identifiers and literals
        'IDENTIFIER', 'STRING'
    ]
)

@dataclass
class Token:
    token_type: TokenType
    lexeme: str
    line_num: int 

tokens = []

for line_num, line in enumerate(input_code.splitlines(), start=1):
    current = 0
    while current < len(line):
        def isEndOfLine():
            return current >= len(line)
        
        def peek(c):
            # Checks if the next character matches `c`, returns False if at end of line
            return current + 1 < len(line) and line[current + 1] == c
        
        # the next character to look at is: line[current]
        next_char = line[current]
        # determine_current_char(next_char)
        match next_char:
            case ";":
                token = Token(TokenType.SEMICOLON, next_char, line_num)
                tokens.append(token)
                current += 1
            case "(":
                token = Token(TokenType.LEFT_PAREN, next_char, line_num)
                tokens.append(token)
                current += 1    
            case ")":
                token = Token(TokenType.RIGHT_PAREN, next_char, line_num)
                tokens.append(token)
                current += 1
            case '=':
                # TODO: Factor out a peak(c) function  that returns True or False
                # is the next character 'c'?
                if current+1 >= len(line) or line[current+1] != '=':
                    token = Token(TokenType.EQUAL, '=', line_num)
                    current += 1
                else:
                    token = Token(TokenType.EQUAL_EQUAL, '==', line_num)
                    current += 2
                tokens.append(token)
            case '!':
                if peek('='):
                    token = Token(TokenType.BANG_EQUAL, '!=', line_num)
                    current += 2
                else:
                    token = Token(TokenType.BANG, '!', line_num)
                    current += 1
            case '"':
                start = current
                while not isEndOfLine():
                    if peek('"'):
                        current += 1
                        token = Token(TokenType.STRING, line[start:current+1], line_num)
                        tokens.append(token)
                    else:
                        current += 1
            case '{':
                token = Token(TokenType.LEFT_BRACE, next_char, line_num)
                tokens.append(token)
                current += 1
            case '}':
                token = Token(TokenType.RIGHT_BRACE, next_char, line_num)
                tokens.append(token)
                current += 1
            case '+':
                token = Token(TokenType.PLUS, next_char, line_num)
                tokens.append(token)
                current += 1
            case '-':
                token = Token(TokenType.MINUS, next_char, line_num)
                tokens.append(token)
                current += 1
            case '*':
                token = Token(TokenType.STAR, next_char, line_num)
                tokens.append(token)
                current += 1
            case '/':
                token = Token(TokenType.SLASH, next_char, line_num)
                tokens.append(token)
                current += 1
            case _:
                print(f"Unrecognized token on line {line_num}, character '{next_char}'. Context: '{line[current:current+10]}'.")
                break

def print_tokens(tokens):
    # Standard column widths
    scanning_width = 30
    token_type_width = 20
    lexeme_width = 20
    line_width = 20  # Adjust if it seems too wide for line numbers

    # Print header
    header_format = f"{'Scanning At':<{scanning_width}} | {'Found A':<{token_type_width}} | {'Lexeme':<{lexeme_width}} | {'Line':<{line_width}}"
    print(header_format)
    print("-" * len(header_format))  # Dynamically adjust based on header length

    # Print each token
    for token in tokens:
        scanning_str = f"Scanning at: {token.line_num}, index: ..." # Placeholder for index if available
        token_type_str = f"Found a: {token.token_type.name}"
        lexeme_str = f"'{token.lexeme}'"
        line_str = str(token.line_num)
        # Format the output string
        output_format = f"{scanning_str:<{scanning_width}} | {token_type_str:<{token_type_width}} | {lexeme_str:<{lexeme_width}} | {line_str:<{line_width}}"
        print(output_format)

# Example usage with the tokens list populated previously
print_tokens(tokens)
