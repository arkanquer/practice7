#include <clang-c/Index.h>
#include <iostream>
#include <string>
using namespace std;

bool isLoop(CXCursorKind kind) {
    return kind == CXCursor_ForStmt || kind == CXCursor_WhileStmt ||
           kind == CXCursor_DoStmt || kind == CXCursor_CXXForRangeStmt;
}

bool isMatchingVariable(CXCursor cursor) {
    if (clang_getCursorKind(cursor) != CXCursor_VarDecl) {
        return false;
    }
    CXType type = clang_getCanonicalType(clang_getCursorType(cursor));
    if (type.kind != CXType_Double) {
        return false;
    }

    CXString nameString = clang_getCursorSpelling(cursor);
    const char* rawName = clang_getCString(nameString);

    string name = rawName ? rawName : "";
    clang_disposeString(nameString);

    return !name.empty() && name[0] != 'x';
}

CXChildVisitResult findVariable(CXCursor cursor,CXCursor parent,CXClientData data) {
    bool* found = static_cast<bool*>(data);
    if (isLoop(clang_getCursorKind(cursor))) {
        return CXChildVisit_Continue;
    }
    if (isMatchingVariable(cursor)) {
        *found = true;
        return CXChildVisit_Break;
    }
    return CXChildVisit_Recurse;
}

CXChildVisitResult visitAST(CXCursor cursor, CXCursor parent, CXClientData data) {
    int* count = static_cast<int*>(data);
    if (isLoop(clang_getCursorKind(cursor)) &&
        clang_Location_isFromMainFile(clang_getCursorLocation(cursor))) {
        bool found = false;
        clang_visitChildren(cursor, findVariable, &found);
        if (found) {
            (*count)++;
        }
    }
    return CXChildVisit_Recurse;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Використовуйте " << argv[0] << " <file.cpp>\n";
        return 1;
    }
    CXIndex index = clang_createIndex(0, 0);
    const char* args[] = {"-std=c++17"};
    CXTranslationUnit unit = clang_parseTranslationUnit(index, argv[1],args, 1, nullptr, 0, CXTranslationUnit_None);
    if (unit == nullptr) {
        cerr << "Помилка парсингу\n";
        clang_disposeIndex(index);
        return 1;
    }
    int count = 0;

    CXCursor root = clang_getTranslationUnitCursor(unit);
    clang_visitChildren(root, visitAST, &count);
    cout << "К-сть циклів, у яких оголошуються змінні типу double та імена яких не починаються з x: "
         << count << endl;
    clang_disposeTranslationUnit(unit);
    clang_disposeIndex(index);
    return 0;
}
