from enum import Enum
from dataclasses import dataclass

input_code = '''
if (temp < 32) {
   message = "It's freezing!";
}
'''

# input_code = '''
# x  ^ 2   message = "Example
# '''

TokenType = Enum(
    'TokenType',
    [
        # The first group of tokens are unambiguous single-characters
        'SEMICOLON',
        'LEFT_PAREN', 'RIGHT_PAREN',
        'LEFT_BRACE', 'RIGHT_BRACE',
        'PLUS', 'MINUS', 'STAR', 'SLASH',
        # The following tokens are 1- or 2-characters with shared prefix
        'EQUAL', 'EQUAL_EQUAL',  # = vs ==
        'BANG', 'BANG_EQUAL',  # ! vs !=
        'LESS', 'LESS_EQUAL',  # < vs <=
        # Identifiers and literals
        'IDENTIFIER', 'STRING', 'NUMBER',
        # Keywords
        'AND', 'CLASS', 'ELSE', 'FALSE', 'FOR', 'IF', 'NIL', 'OR',
        # Exponentiation operator (choose the appropriate one based on your syntax preference)
        'POWER',  # For '^' or '**' as exponentiation operator
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
        char = line[current]
        if char.isspace():
            current += 1
            continue  # Skip to the next iteration of the loop to bypass whitespace

        def isEndOfLine():
            return current >= len(line)
        
        def peek(c):
            # Checks if the next character matches `c`, returns False if at end of line
            return current + 1 < len(line) and line[current + 1] == c
        
        # the next character to look at is: line[current]
        next_char = line[current]
        # determine_current_char(next_char)

        while current < len(line) and line[current].isspace():
            current += 1
        if current >= len(line):
            break  # Exit the loop if we've reached the end of the line after skipping whitespace

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
            case _ if next_char.isdigit():
                start = current
                current += 1
                while not isEndOfLine() and line[current].isdigit():
                    current += 1
                token = Token(TokenType.NUMBER, line[start:current], line_num)
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
                current += 1
                while not isEndOfLine():
                    if peek('"'):
                        current += 1
                        token = Token(TokenType.STRING, line[start:current+1], line_num)
                        tokens.append(token)
                        break
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
                if peek('*'):
                    tokens.append(Token(TokenType.POWER, '**', line_num))
                    current += 2
                else:
                    token = Token(TokenType.STAR, '*', line_num)
                    tokens.append(token)
                    current += 1


            case '/':
                token = Token(TokenType.SLASH, next_char, line_num)
                tokens.append(token)
                current += 1
            case _ if next_char.isalpha() or next_char == '_':
                start = current
                while current < len(line) and (line[current].isalnum() or line[current] == '_'):
                    current += 1
                lexeme = line[start:current]
                if lexeme.upper() in TokenType.__members__:
                    token_type = TokenType[lexeme.upper()]
                else:
                    token_type = TokenType.IDENTIFIER
                tokens.append(Token(token_type, lexeme, line_num))
            case '^':
                tokens.append(Token(TokenType.POWER, next_char, line_num))
                current += 1
            case '<':
                token = Token(TokenType.LESS, '<', line_num)
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
