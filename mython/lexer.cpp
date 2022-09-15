#include "lexer.h"

#include <algorithm>
#include <charconv>

using namespace std;

namespace parse {

bool operator==(const Token& lhs, const Token& rhs) {
    using namespace token_type;

    if (lhs.index() != rhs.index()) {
        return false;
    }
    if (lhs.Is<Char>()) {
        return lhs.As<Char>().value == rhs.As<Char>().value;
    }
    if (lhs.Is<Number>()) {
        return lhs.As<Number>().value == rhs.As<Number>().value;
    }
    if (lhs.Is<String>()) {
        return lhs.As<String>().value == rhs.As<String>().value;
    }
    if (lhs.Is<Id>()) {
        return lhs.As<Id>().value == rhs.As<Id>().value;
    }
    return true;
}

bool operator!=(const Token& lhs, const Token& rhs) {
    return !(lhs == rhs);
}

ostream& operator<<(ostream& os, const Token& rhs) {
    using namespace token_type;

#define VALUED_OUTPUT(type) \
    if (auto p = rhs.TryAs<type>()) return os << #type << '{' << p->value << '}';

    VALUED_OUTPUT(Number);
    VALUED_OUTPUT(Id);
    VALUED_OUTPUT(String);
    VALUED_OUTPUT(Char);

#undef VALUED_OUTPUT

#define UNVALUED_OUTPUT(type) \
    if (rhs.Is<type>()) return os << #type;

    UNVALUED_OUTPUT(Class);
    UNVALUED_OUTPUT(Return);
    UNVALUED_OUTPUT(If);
    UNVALUED_OUTPUT(Else);
    UNVALUED_OUTPUT(Def);
    UNVALUED_OUTPUT(Newline);
    UNVALUED_OUTPUT(Print);
    UNVALUED_OUTPUT(Indent);
    UNVALUED_OUTPUT(Dedent);
    UNVALUED_OUTPUT(And);
    UNVALUED_OUTPUT(Or);
    UNVALUED_OUTPUT(Not);
    UNVALUED_OUTPUT(Eq);
    UNVALUED_OUTPUT(NotEq);
    UNVALUED_OUTPUT(LessOrEq);
    UNVALUED_OUTPUT(GreaterOrEq);
    UNVALUED_OUTPUT(None);
    UNVALUED_OUTPUT(True);
    UNVALUED_OUTPUT(False);
    UNVALUED_OUTPUT(Eof);

#undef UNVALUED_OUTPUT

    return os << "Unknown token :("sv;
}

Lexer::Lexer(istream& input) : input_{input} {
    ReadNextToken();
}

const Token& Lexer::CurrentToken() const {
    return current_token_;
}

Token Lexer::NextToken() {
    ReadNextToken();
    
    return current_token_;
}

void Lexer::ReadNextToken() {
    char ch = input_.peek();
    
    if (ch == EOF) { 
        ParseEOF();
    } else if (ch == '\n') { 
        ParseLineEnd();
    } else if (ch == '#') { 
        ParseComment();
    } else if (ch == ' ') { 
        ParseSpaces();
    } else if (line_start_ && (indent_ != next_indent_)) {
        ParseIndent();
    }  else {
        ParseToken();
        line_start_ = false; 
    }
}

void Lexer::NewLine() {
    detail::ReadLine(input_);
    
    line_start_ = true;
    
    next_indent_ = 0;
}

void Lexer::ParseComment() {
    char c;
    
    while (input_.get(c)) {
        if (c == '\n') {
            input_.putback(c);
            break;
        }
    }
    
    ReadNextToken();
}

void Lexer::ParseEOF() {
    if (!line_start_) { 
        NewLine();
        current_token_ = token_type::Newline{};
    } else { 
        if (indent_ > 0) {
            --indent_;
            current_token_ = token_type::Dedent{};
        } else {
            current_token_ = token_type::Eof{};
        }
    }
}

void Lexer::ParseLineEnd() {
    if (line_start_) { 
        NewLine();
        ReadNextToken();
    } else {
        NewLine();
        current_token_ = token_type::Newline{};
    }
}

void Lexer::ParseSpaces() {
    size_t spaces = detail::ReadSpaces(input_);
    
    if (line_start_) { 
        next_indent_ = spaces / 2;
    }
    
    ReadNextToken(); 
}

void Lexer::ParseIndent() {
    if (indent_ < next_indent_) {
        ++indent_;
        current_token_ = token_type::Indent{};
    } else if (indent_ > next_indent_) {
        --indent_;
        current_token_ = token_type::Dedent{};
    }
}

void Lexer::ParseToken() {
    char ch = input_.peek();
    
    if (detail::IsDigit(ch)) {  
        current_token_ = token_type::Number{detail::ReadNumber(input_)};
    } else if (detail::IsNameChar(ch)) {    
        ParseName();
    } else if (ch == '\"' || ch == '\'') { 
        current_token_ = token_type::String{detail::ReadString(input_)};
    } else { 
        ParseChar();
    }
}

void Lexer::ParseName() {
    auto name = detail::ReadName(input_);
    
    if (Lexer::key_words.count(name) != 0) { 
        current_token_ = Lexer::key_words.at(name);
    } else { 
        current_token_ = token_type::Id{name};
    }
}

void Lexer::ParseChar() {
    string char_pair;
    
    char_pair += input_.get();
    char_pair += input_.peek();
    
    if (Lexer::double_char_ops.count(char_pair)) { 
        current_token_ = Lexer::double_char_ops.at(char_pair);
        input_.get();
    } else { 
      current_token_ = token_type::Char{char_pair[0]};
    }
}

namespace detail {

bool IsDigit(char ch) {
    return isdigit(static_cast<unsigned char>(ch));
}

bool IsNameChar(char ch) {
    return isalnum(static_cast<unsigned char>(ch)) || ch == '_';
}

string ReadString(istream& input) {
    string line;

    char first = input.get(); 
    char c;
    
    while (input.get(c)) {
        if (c == '\\') {
            char next;
            input.get(next);
            if (next == '\"') {
                line += '\"';
            } else if (next == '\'') {
                line += '\'';
            } else if (next == 'n') {
                line += '\n';
            } else if (next == 't') {
                line += '\t';
            }
        } else {
            if (c == first) {
                break;
            }
            line += c;
        }
    }
    

    if (c != first) {
        throw runtime_error("No end quote in string: "s + line);
    }
    
    return line;
}

string ReadName(istream& input) {
    string line;
    char c;
    
    while (input.get(c)) {
        if (detail::IsNameChar(c)) {
            line += c;
        } else {
            input.putback(c);
            break;
        }
    }
    
    return line;
}

size_t ReadSpaces(istream& input) {
    size_t result = 0;
    char c;
    
    while (input.get(c)) {
        if (c == ' ') {
            ++result;
        } else {
            input.putback(c);
            break;
        }
    }
    
    return result;
}

string ReadLine(istream& input) {
    string s;
    
    getline(input, s);
    
    return s;
}

int ReadNumber(istream& input) {
    string num;
    char c;
    
    while(input.get(c)) {
        if (IsDigit(c)) {
            num += c;
        } else {
            input.putback(c);
            break;
        }
    }
    
    if (num.empty()) {
        return 0;
    }
    
    return stoi(num);
}

} // namespace detail

}  // namespace parse