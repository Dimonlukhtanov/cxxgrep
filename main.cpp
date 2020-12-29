#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "llvm/Support/CommandLine.h"
#include "clang-c/Index.h" // LibClang

using namespace std;
using namespace llvm;

enum Arguments {
    Function,
    I,
    Member,
    Parameter,
    Record,
    Variable,
};

struct ClientData {
    vector<CXCursorKind> kind;
    vector<string> lines;
    string name;
    bool isInsensitiveSearch;
};

vector<string> getLinesFile(string file) {
    ifstream fin(file);
    string str;
    vector<string> lines;
    while (getline(fin, str))
        lines.emplace_back(str);
    return lines;
}

string getCursorKindName(CXCursorKind cursorKind) {
    return clang_getCString(clang_getCursorKindSpelling(cursorKind));
}

string getCursorSpelling(CXCursor cursor){
    return clang_getCString(clang_getCursorSpelling(cursor));
}

// pointer to AST
// TODO: Нужно ли уменьшать счетчик указателей --> lang_getCString (++) x(--) и др. ???
//
std::string toString(CXString cxString) {
    return clang_getCString(cxString);
}

void match(CXSourceLocation location, CXCursor cursor, vector<string> lines) {
    CXFile file;
    unsigned lineNumber, columnNumber;

    clang_getSpellingLocation(location, &file, &lineNumber, &columnNumber, nullptr);
    std::cout << "\033[34;1;4m" << lineNumber << ':' << columnNumber << "\033[0m:\t";

    const auto spelling = toString(clang_getCursorSpelling(cursor));
    const auto& line = lines[lineNumber - 1];
    for (unsigned column = 1; column <= line.length(); ++column) {
        if (column == columnNumber) {
            auto lexeme = toString(clang_getCursorSpelling(cursor));
            std::cout << lexeme;
            column += spelling.length() - 1;
        } else {
            std::cout << line[column - 1];
        }
    }
    cout << endl;
}

bool checkEqualKind(CXCursorKind kind, const vector<CXCursorKind>& filteredKind) {
    for (auto & i : filteredKind) {
        if (kind == i)
            return true;
    }
    return false;
}

CXChildVisitResult visit(CXCursor cursor, CXCursor p, CXClientData clientData) {
    const CXSourceLocation location = clang_getCursorLocation(cursor);
    if (clang_Location_isInSystemHeader(location)) {
        return CXChildVisit_Break;
    }
    CXCursorKind kind = clang_getCursorKind(cursor);
    auto data = *reinterpret_cast<ClientData*>(clientData);
    auto lexeme = toString(clang_getCursorSpelling(cursor));

    if (data.isInsensitiveSearch) {
        transform(data.name.begin(), data.name.end(), data.name.begin(), (int (*)(int))tolower);
        transform(lexeme.begin(), lexeme.end(), lexeme.begin(), (int (*)(int))tolower);
    }

    if (checkEqualKind(kind, data.kind) && data.name == lexeme)
        match(location, cursor, data.lines);

    return CXChildVisit_Recurse;
}

// Enum CXCursorKind (1706 Index.h)
vector<CXCursorKind> enumOfArguments(Arguments argument) {
    vector<CXCursorKind> filter;
    switch (argument) {
        case Function:
            filter.push_back(CXCursor_FunctionDecl);
            break;
        case I:
            break;
        case Member:
            filter.push_back(CXCursor_CXXMethod);
            break;
        case Parameter:
            filter.push_back(CXCursor_ParmDecl);
            break;
        case Record:
            filter.push_back(CXCursor_StructDecl);
            filter.push_back(CXCursor_ClassDecl);
            break;
        case Variable:
            filter.push_back(CXCursor_VarDecl);
            break;
        default:
            break;
    }
    return filter;
}

int main(int argc, char **argv) {

    cl::opt<Arguments> Argument(cl::desc("[Options]:"), cl::values(
            clEnumVal(Function, "Filter by Functions"),
            clEnumVal(I, "Make the search case-insensitive"),
            clEnumVal(Member, "Filter by Members"),
            clEnumVal(Parameter, "Filter by Function parameter"),
            clEnumVal(Record, "Filter by Records (class/struct)"),
            clEnumVal(Variable, "Filter by Variables")
    ));

    cl::opt<bool> isInsensitiveSearch("i", cl::desc("Make the search case-insensitive"));
    cl::opt<string> Name(cl::Positional, cl::desc("Name by find"));
    cl::opt<string> Filepath(cl::Positional, cl::desc("Filepath"), cl::Required);

    cl::ParseCommandLineOptions(argc, argv);
    CXIndex index = clang_createIndex(false,true);
    CXTranslationUnit unit = clang_parseTranslationUnit(
            index,                           // CIdx
            Filepath.c_str(),                // source_filename
            nullptr,                         // command_line_args
            0,                               // num_command_line_args
            nullptr,                         // unsaved_files
            0,                               // num_unsaved_files
            0                                // options
    );

    if (unit == nullptr) {
        cerr << "Unable to parse translation unit" << endl;
        exit(EXIT_FAILURE);
    }
    // AST parsed OK

    ClientData clientData;
    clientData.kind = enumOfArguments(Argument.getValue());
    clientData.lines = getLinesFile(Filepath);
    clientData.isInsensitiveSearch = isInsensitiveSearch;
    clientData.name = Name;

    CXCursor cursor = clang_getTranslationUnitCursor(unit); // Start traversing the various declarations (in TU)
    clang_visitChildren(cursor, &visit, &clientData);

    clang_disposeTranslationUnit(unit); // Delete TU
    clang_disposeIndex(index); // Delete parsing context

    return EXIT_SUCCESS;
}