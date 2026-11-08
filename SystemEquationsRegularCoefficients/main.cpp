#include "SERC.h"
#include "Terminals.h"

#include <iostream>
using namespace std;

int main()
{
    setlocale(LC_ALL, "Russian");

    //// Собираем выражение: (a + b)* c
    //NodePtr a = makeSymbol('a');
    //NodePtr b = makeSymbol('b');
    //NodePtr a_plus_b = makeUnion(a, b);
    //NodePtr star = makeStar(a_plus_b);

    //NodePtr c = makeSymbol('c');
    //NodePtr result = makeConcat(star, c);

    //// Выведет: (a + b)*c
    //std::cout << result->toString() << std::endl;

    try {
        std::string filename = "test2.txt";

        Matrix matrix = readMatrixFromFile(filename);
        int N = matrix.size();

        std::cout << "--- Исходная система уравнений ---\n";
        for (int i = 0; i < N; ++i) {
            std::cout << "X" << i + 1 << " = ";
            for (int j = 0; j < N; ++j) {
                if (!matrix[i][j]->isEmptySet()) {
                    std::cout << matrix[i][j]->toString() << "*X" << j + 1 << " + ";
                }
            }
            std::cout << matrix[i][N]->toString() << "\n";
        }
        std::cout << "----------------------------------\n\n";

        // Решаем систему
        std::vector<NodePtr> roots = solveRegexSystem(matrix);

        std::cout << "--- Решение системы ---\n";
        for (int i = 0; i < N; ++i) {
            std::cout << "X" << i + 1 << " = " << roots[i]->toString() << "\n";
        }

    }
    catch (const std::exception& e) {
        std::cerr << "КРИТИЧЕСКАЯ ОШИБКА: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
