#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <cstdlib>
using namespace std;

enum TokenType { LET, PRINT, INPUT, IDENT, STRING, NUMBER, EQUALS, SEMICOLON, LPAREN, RPAREN, PLUS, END };

struct Token {
    TokenType type;
    string value;
};

map<string, string> variables;

vector<Token> tokenize(const string& src) {
    vector<Token> tokens;
    size_t i = 0;
    while (i < src.length()) {
        if (isspace(src[i])) { i++; continue; }
        if (src.compare(i, 3, "let") == 0) { tokens.push_back({LET, "let"}); i+=3; continue; }
        if (src.compare(i, 5, "print") == 0) { tokens.push_back({PRINT, "print"}); i+=5; continue; }
        if (src.compare(i, 5, "input") == 0) { tokens.push_back({INPUT, "input"}); i+=5; continue; }
        if (src[i] == '"') {
            i++;
            string str;
            while (i < src.length() && src[i] != '"') str += src[i++];
            i++;
            tokens.push_back({STRING, str});
            continue;
        }
        if (isalpha(src[i])) {
            string id;
            while (i < src.length() && isalnum(src[i])) id += src[i++];
            tokens.push_back({IDENT, id});
            continue;
        }
        if (isdigit(src[i])) {
            string num;
            while (i < src.length() && isdigit(src[i])) num += src[i++];
            tokens.push_back({NUMBER, num});
            continue;
        }
        switch(src[i]) {
            case '=': tokens.push_back({EQUALS, "="}); i++; break;
            case ';': tokens.push_back({SEMICOLON, ";"}); i++; break;
            case '(': tokens.push_back({LPAREN, "("}); i++; break;
            case ')': tokens.push_back({RPAREN, ")"}); i++; break;
            case '+': tokens.push_back({PLUS, "+"}); i++; break;
            default:
                cerr << "Unknown char: " << src[i] << endl;
                exit(1);
        }
    }
    tokens.push_back({END, ""});
    return tokens;
}

void generate_asm(const vector<Token>& tokens, const string& output_file) {
    ofstream out(output_file);

    out << "section .bss\n";
    out << "input_buf resb 256\n";
    out << "concat_buf resb 512\n";

    out << "section .data\n";
    out << "digit db 0\n";

    // Store string variables
    for (size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i].type == LET && tokens[i+1].type == IDENT && tokens[i+2].type == EQUALS) {
            string var = tokens[i+1].value;
            if (tokens[i+3].type == STRING) {
                string val = tokens[i+3].value;
                variables[var] = "str_" + var;
                out << "str_" << var << " db \"" << val << "\", 0xA\n";
                out << "str_" << var << "_len equ $ - str_" << var << "\n";
            } else if (tokens[i+3].type == INPUT) {
                // input() variable, store input_buf
                variables[var] = "input_buf";
            }
        }
    }

    out << "section .text\n";
    out << "global _start\n";
    out << "_start:\n";

    // Handle input calls
    for (size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i].type == LET && tokens[i+1].type == IDENT && tokens[i+2].type == EQUALS && tokens[i+3].type == INPUT) {
            // Optional prompt
            if (tokens[i+4].type == LPAREN && tokens[i+5].type == STRING) {
                string prompt = tokens[i+5].value;
                out << "    ; print prompt\n";
                out << "    mov eax, 4\n";
                out << "    mov ebx, 1\n";
                out << "    mov ecx, prompt_msg\n";
                out << "    mov edx, prompt_len\n";
                out << "    int 0x80\n";
            }
            out << "    ; read input\n";
            out << "    mov eax, 3\n";
            out << "    mov ebx, 0\n";
            out << "    mov ecx, input_buf\n";
            out << "    mov edx, 255\n";
            out << "    int 0x80\n";
        }
    }

    // Print statements handling
    for (size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i].type == PRINT) {
            size_t idx = i + 2; // skip print and '('

            // Case: print("string");
            if (tokens[idx].type == STRING && tokens[idx+1].type == RPAREN) {
                out << "    ; print string literal\n";
                out << "    mov eax, 4\n";
                out << "    mov ebx, 1\n";
                out << "    mov ecx, print_str\n";
                out << "    mov edx, print_str_len\n";
                out << "    int 0x80\n";
                continue;
            }

            // Case: print(var);
            if (tokens[idx].type == IDENT && tokens[idx+1].type == RPAREN) {
                string var = tokens[idx].value;
                if (variables.count(var) == 0) {
                    cerr << "Variable " << var << " not declared\n";
                    exit(1);
                }
                if (variables[var] == "input_buf") {
                    out << "    ; print user input\n";
                    out << "    mov eax, 4\n";
                    out << "    mov ebx, 1\n";
                    out << "    mov ecx, input_buf\n";
                    out << "    mov edx, 255\n";
                    out << "    int 0x80\n";
                } else if (variables[var].find("str_") == 0) {
                    out << "    ; print string variable\n";
                    out << "    mov eax, 4\n";
                    out << "    mov ebx, 1\n";
                    out << "    mov ecx, " << variables[var] << "\n";
                    out << "    mov edx, " << variables[var] << "_len\n";
                    out << "    int 0x80\n";
                }
                continue;
            }

            // Case: print("str" + var);
            if (tokens[idx].type == STRING && tokens[idx+1].type == PLUS && tokens[idx+2].type == IDENT && tokens[idx+3].type == RPAREN) {
                string strlit = tokens[idx].value;
                string var = tokens[idx+2].value;

                if (variables.count(var) == 0) {
                    cerr << "Variable " << var << " not declared\n";
                    exit(1);
                }

                // Declare string literal label
                out << "section .data\n";
                out << "hello_str db \"" << strlit << "\", 0\n";
                out << "hello_len equ $ - hello_str\n";

                // Assembly for concatenation
                out << "section .text\n";
                out << "    ; concat string literal and variable\n";
                out << "    mov esi, hello_str\n";
                out << "    mov edi, concat_buf\n";
                out << ".copy_loop:\n";
                out << "    mov al, [esi]\n";
                out << "    mov [edi], al\n";
                out << "    inc esi\n";
                out << "    inc edi\n";
                out << "    cmp al, 0\n";
                out << "    jne .copy_loop\n";

                if (variables[var] == "input_buf") {
                    out << "    mov esi, input_buf\n";
                } else {
                    out << "    mov esi, " << variables[var] << "\n";
                }

                out << ".append_loop:\n";
                out << "    mov al, [esi]\n";
                out << "    mov [edi], al\n";
                out << "    inc esi\n";
                out << "    inc edi\n";
                out << "    cmp al, 0\n";
                out << "    jne .append_loop\n";

                // Calculate length of concatenated string
                out << "    mov ecx, concat_buf\n";
                out << "    mov edx, 0\n";
                out << ".len_loop:\n";
                out << "    cmp byte [ecx + edx], 0\n";
                out << "    je .len_done\n";
                out << "    inc edx\n";
                out << "    jmp .len_loop\n";
                out << ".len_done:\n";

                // Print concatenated buffer
                out << "    mov eax, 4\n";
                out << "    mov ebx, 1\n";
                out << "    mov ecx, concat_buf\n";
                out << "    mov edx, edx\n";
                out << "    int 0x80\n";
            }
        }
    }

    // Exit syscall
    out << "    mov eax, 1\n";
    out << "    xor ebx, ebx\n";
    out << "    int 0x80\n";

    // Print string literal for print("string");
    for (size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i].type == PRINT && tokens[i+2].type == STRING) {
            out << "section .data\n";
            out << "print_str db \"" << tokens[i+2].value << "\", 0xA\n";
            out << "print_str_len equ $ - print_str\n";
            break;
        }
    }

    // Prompt message for input
    for (size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i].type == LET && tokens[i+3].type == INPUT && tokens[i+5].type == STRING) {
            out << "section .data\n";
            out << "prompt_msg db \"" << tokens[i+5].value << "\", 0xA\n";
            out << "prompt_len equ $ - prompt_msg\n";
            break;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "Usage: CCDash.exe source.cdash [-o output.exe]\n";
        return 1;
    }
    string input_file = argv[1];
    string output_file = "a.exe";

    for (int i=2; i<argc; i++) {
        string arg = argv[i];
        if (arg == "-o" && i+1 < argc) {
            output_file = argv[i+1];
            i++;
        }
    }

    ifstream in(input_file);
    if (!in) {
        cerr << "Failed to open " << input_file << "\n";
        return 1;
    }
    stringstream buffer;
    buffer << in.rdbuf();
    string src = buffer.str();

    auto tokens = tokenize(src);
    string asm_file = "temp.asm";

    generate_asm(tokens, asm_file);

    int nasm_status = system(("nasm -f elf " + asm_file + " -o temp.o").c_str());
    if (nasm_status != 0) {
        cerr << "NASM assembly failed\n";
        return 1;
    }
    int ld_status = system(("ld -m elf_i386 temp.o -o " + output_file).c_str());
    if (ld_status != 0) {
        cerr << "Linking failed\n";
        return 1;
    }

    cout << "Compiled " << input_file << " to " << output_file << "\n";
    return 0;
}
