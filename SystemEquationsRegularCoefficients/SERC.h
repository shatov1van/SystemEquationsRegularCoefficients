#pragma once
#include "Terminals.h"

#include <vector>
#include <memory>
#include <fstream>

using Matrix = std::vector<std::vector<NodePtr>>;

// Функция решения системы уравнений
std::vector<NodePtr> solveRegexSystem(Matrix matrix) {
    int N = matrix.size();

    // Прямой и обратный ход
    for (int k = 0; k < N; ++k) {

        // 1. Применяем лемму Ардена к k-му уравнению
        // Коэффициент при X_k превращается в (A_kk)*
        NodePtr akk_star = makeStar(matrix[k][k])->simplify();

        // Умножаем (A_kk)* на все остальные элементы строки k
        for (int j = 0; j <= N; ++j) {
            if (j != k) {
                matrix[k][j] = makeConcat(akk_star, matrix[k][j])->simplify();
            }
        }
        // Теперь X_k не зависит от самого себя
        matrix[k][k] = makeEmptySet();

        // 2. Подставляем выраженный X_k во все остальные уравнения
        for (int i = 0; i < N; ++i) {
            if (i == k) continue; // Пропускаем текущую строку

            NodePtr aik = matrix[i][k]; // Запоминаем коэффициент при X_k

            // Пропускаем подстановку, если коэффициент — пустое множество (оптимизация)
            if (aik->isEmptySet()) continue;

            for (int j = 0; j <= N; ++j) {
                if (j != k) {
                    // Новое значение: A_ij = A_ij + A_ik * A_kj
                    NodePtr substitution = makeConcat(aik, matrix[k][j]);
                    matrix[i][j] = makeUnion(matrix[i][j], substitution)->simplify();
                }
            }
            // Переменная X_k полностью исключена из i-го уравнения
            matrix[i][k] = makeEmptySet();
        }
    }

    // После завершения цикла матрица приведена к диагональному виду (точнее, все элементы 
    // кроме последнего столбца равны пустым множествам).
    // Последний столбец содержит искомые регулярные выражения.

    std::vector<NodePtr> results(N);
    for (int i = 0; i < N; ++i) {
        results[i] = matrix[i][N]->simplify();
    }

    return results;
}

// Парсер строк с регулярными коэффициентами
class RegexParser {
    std::string s;
    size_t pos = 0;

    char peek() const { return pos < s.length() ? s[pos] : '\0'; }
    char get() { return pos < s.length() ? s[pos++] : '\0'; }

    bool match(char c) {
        if (peek() == c) {
            pos++;
            return true;
        }
        return false;
    }

public:
    explicit RegexParser(std::string str) : s(std::move(str)) {
        // Удаляем все пробелы, чтобы они не мешали парсингу
        s.erase(std::remove_if(s.begin(), s.end(), ::isspace), s.end());
    }

    NodePtr parse() {
        if (s.empty()) return makeEmptySet();
        NodePtr result = parseExpr();
        if (pos < s.length()) {
            throw std::runtime_error("Ошибка синтаксиса: неожиданные символы в конце строки");
        }
        return result;
    }

private:
    // Expr -> Term ( '+' Term )*
    NodePtr parseExpr() {
        NodePtr node = parseTerm();
        while (match('+') || match('|')) {
            node = makeUnion(node, parseTerm());
        }
        return node;
    }

    // Term -> Factor ( Factor )*   [Неявная конкатенация]
    NodePtr parseTerm() {
        NodePtr node = parseFactor();
        while (pos < s.length() && peek() != '+' && peek() != '|' && peek() != ')') {
            node = makeConcat(node, parseFactor());
        }
        return node;
    }

    // Factor -> Base ( '*' | '(^*)' )*
    NodePtr parseFactor() {
        NodePtr node = parseBase();

        while (true) {
            // Поддержка твоего кастомного формата (^*)
            if (pos + 3 < s.length() && s.substr(pos, 4) == "(^*)") {
                pos += 4;
                node = makeStar(node);
            }
            // Поддержка классической звездочки *
            else if (match('*')) {
                node = makeStar(node);
            }
            else {
                break;
            }
        }
        return node;
    }

    // Base -> 'a'-'z' | '\e' | '\0' | '(' Expr ')'
    NodePtr parseBase() {
        if (match('(')) {
            NodePtr node = parseExpr();
            if (!match(')')) throw std::runtime_error("Пропущена закрывающая скобка ')'");
            return node;
        }

        if (match('\\')) {
            char next = get();
            if (next == 'e') return makeEpsilon();
            if (next == '0') return makeEmptySet();
            throw std::runtime_error(std::string("Неизвестный спецсимвол: \\") + next);
        }

        char c = get();
        if (c == '\0') throw std::runtime_error("Неожиданный конец строки");

        return makeSymbol(c);
    }
};

// Функция чтения СЛАУ из текстового файла
Matrix readMatrixFromFile(const std::string& filename) {
    std::ifstream file(filename);

    // Проверка, открылся ли файл
    if (!file.is_open()) {
        throw std::runtime_error("Не удалось открыть файл: " + filename);
    }

    int N;
    // Читаем размерность матрицы (первое число в файле)
    if (!(file >> N)) {
        throw std::runtime_error("Ошибка чтения: файл пуст или отсутствует размер матрицы N");
    }

    // Создаем матрицу
    Matrix matrix(N, std::vector<NodePtr>(N + 1));
    std::string token;

    // Заполняем матрицу
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j <= N; ++j) {
            // Читаем очередное выражение до пробела или переноса строки
            if (!(file >> token)) {
                throw std::runtime_error("Недостаточно коэффициентов в файле. Проверьте формат.");
            }

            // Пропускаем прочитанную строку через парсер
            RegexParser parser(token);
            matrix[i][j] = parser.parse();
        }
    }

    return matrix;
}