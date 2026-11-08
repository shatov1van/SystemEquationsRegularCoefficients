#pragma once
#include <iostream>
#include <memory>
#include <string>

// Определение приоритетов для расстановки скобок
enum class Precedence {
    UNION = 1,   // Самый низкий приоритет: +
    CONCAT = 2,  // Средний приоритет: конкатенация
    STAR = 3,    // Высокий приоритет: *
    ATOM = 4     // Терминалы (буквы, эпсилон, пустое множество)
};

// Базовый класс узла дерева
class RegexNode: public std::enable_shared_from_this<RegexNode> {
public:
    virtual ~RegexNode() = default;

    // Возвращает строковое представление регулярного выражения
    virtual std::string toString() const = 0;

    // Возвращает приоритет текущего узла
    virtual Precedence getPrecedence() const = 0;

    // Упрощение выражения
    virtual std::shared_ptr<RegexNode> simplify() = 0;

    // Флаги для быстрой идентификации
    virtual bool isEmptySet() const { return false; }
    virtual bool isEpsilon() const { return false; }
};

using NodePtr = std::shared_ptr<RegexNode>;
NodePtr makeEmptySet();
NodePtr makeEpsilon();
NodePtr makeSymbol(char c);
NodePtr makeUnion(NodePtr l, NodePtr r);
NodePtr makeConcat(NodePtr l, NodePtr r);
NodePtr makeStar(NodePtr c);

// ---------------- ТЕРМИНАЛЫ ----------------

// Символ алфавита (например, 'a' или 'b')
class SymbolNode : public RegexNode {
    char symbol;
public:
    explicit SymbolNode(char s) : symbol(s) {}
    std::string toString() const override { return std::string(1, symbol); }
    Precedence getPrecedence() const override { return Precedence::ATOM; }

    NodePtr simplify() override { return shared_from_this(); }
};

// Пустое слово (Epsilon, ε)
class EpsilonNode : public RegexNode {
public:
    std::string toString() const override { return "\\e"; }
    Precedence getPrecedence() const override { return Precedence::ATOM; }

    NodePtr simplify() override { return shared_from_this(); }
    bool isEpsilon() const override { return true; }
};

// Пустое множество (Empty Set)
class EmptySetNode : public RegexNode {
public:
    std::string toString() const override { return "\\0"; }
    Precedence getPrecedence() const override { return Precedence::ATOM; }

    NodePtr simplify() override { return shared_from_this(); }
    bool isEmptySet() const override { return true; }
};

// ---------------- ОПЕРАЦИИ ----------------

// Объединение (Union, +)
class UnionNode : public RegexNode {
    NodePtr left, right;
public:
    UnionNode(NodePtr l, NodePtr r) : left(std::move(l)), right(std::move(r)) {}

    std::string toString() const override {
        return left->toString() + " + " + right->toString();
    }
    Precedence getPrecedence() const override { return Precedence::UNION; }

    NodePtr simplify() override {
        // Сначала рекурсивно упрощаем потомков
        auto l = left->simplify();
        auto r = right->simplify();

        // 1. Ø + A = A
        if (l->isEmptySet()) return r;
        // 2. A + Ø = A
        if (r->isEmptySet()) return l;
        // 3. A + A = A (Идемпотентность). Сравниваем через строки для простоты
        if (l->toString() == r->toString()) return l;

        return makeUnion(l, r);
    }
};

// Конкатенация (A * B)
class ConcatNode : public RegexNode {
    NodePtr left, right;

    // Вспомогательная функция для добавления скобок дочерним узлам с низким приоритетом
    std::string wrapIfNeeded(const NodePtr& child) const {
        if (child->getPrecedence() < Precedence::CONCAT) {
            return "(" + child->toString() + ")";
        }
        return child->toString();
    }
public:
    ConcatNode(NodePtr l, NodePtr r) : left(std::move(l)), right(std::move(r)) {}

    std::string toString() const override {
        return wrapIfNeeded(left) + wrapIfNeeded(right);
    }
    Precedence getPrecedence() const override { return Precedence::CONCAT; }

    NodePtr simplify() override {
        auto l = left->simplify();
        auto r = right->simplify();

        // 1. ØA = Ø  и  AØ = Ø
        if (l->isEmptySet() || r->isEmptySet()) return makeEmptySet();
        // 2. εA = A
        if (l->isEpsilon()) return r;
        // 3. Aε = A
        if (r->isEpsilon()) return l;

        return makeConcat(l, r);
    }
};

// Звезда Клини (Kleene Star, *)
class StarNode : public RegexNode {
    NodePtr child;
public:
    explicit StarNode(NodePtr c) : child(std::move(c)) {}

    std::string toString() const override {
        // Если под узлом операция объединения или конкатенации, нужны скобки
        if (child->getPrecedence() < Precedence::STAR) {
            return "(" + child->toString() + ")(^*)";
        }
        return child->toString() + "(^*)";
    }
    Precedence getPrecedence() const override { return Precedence::STAR; }

    NodePtr simplify() override {
        auto c = child->simplify();

        // 1. Ø* = ε
        if (c->isEmptySet()) return makeEpsilon();
        // 2. ε* = ε
        if (c->isEpsilon()) return makeEpsilon();

        return makeStar(c);
    }
};

// ---------------- ФАБРИЧНЫЕ ФУНКЦИИ ----------------
// Позволяют собирать AST без явного вызова std::make_shared в коде

NodePtr makeEmptySet() { return std::make_shared<EmptySetNode>(); }
NodePtr makeEpsilon() { return std::make_shared<EpsilonNode>(); }
NodePtr makeSymbol(char c) { return std::make_shared<SymbolNode>(c); }
NodePtr makeUnion(NodePtr l, NodePtr r) { return std::make_shared<UnionNode>(l, r); }
NodePtr makeConcat(NodePtr l, NodePtr r) { return std::make_shared<ConcatNode>(l, r); }
NodePtr makeStar(NodePtr c) { return std::make_shared<StarNode>(c); }
