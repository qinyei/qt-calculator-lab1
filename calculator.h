#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <QString>
#include <QVector>

/**
 * 计算器核心：负责「输入 -> 算式 -> 求值」的全部规则，与界面完全无关。
 *
 * ★ 采用「整式求值」模型（与手机计算器一致，区别于 Windows 标准计算器的「即时运算」）：
 *   - 每按一次键，就把内容追加到算式里，显示区实时显示整条算式；
 *   - 按 = 时一次性求值，并且遵守运算优先级：先 × ÷，后 + −，同级从左到右。
 *   例：2 + 3 × 4 = 14（即时运算会算成 20）
 *
 * 内部用一个 token 列表保存算式，数字与运算符严格交替：
 *   [数字 "2"] [运算符 "+"] [数字 "3"] [运算符 "×"] [数字 "4"]
 * 列表末尾如果是数字，它就代表「正在输入的那个数」。
 *
 * 之所以把逻辑单独抽出来：
 *   1. 鼠标点击和键盘按键都调用同一个入口 enter()，两套输入行为天然一致；
 *   2. 可以脱离 GUI 单独写测试（见 tests/），边界情况能被真实验证。
 */
class Calculator
{
public:
    Calculator();

    /**
     * 唯一输入入口。key 的取值集合（按钮文字与键盘按键都翻译成这些记号）：
     *   数字      "0" ~ "9"
     *   小数点    "."
     *   运算符    "+"  "-"  "×"  "÷"
     *   等号      "="
     *   退格      "<-"                 （键盘 Backspace）
     *   清除      "C"（全部清除）      "CE"（只清除当前输入的数）
     *   正负号    "±"
     *   一元运算  "1/x"  "x²"  "√x"  "%"
     */
    void enter(const QString &key);

    QString displayText() const;      // 大字：输入过程中是整条算式；按 = 之后是结果
    QString expressionText() const;   // 小字：按 = 之后显示「刚才那条算式 =」
    bool    isError() const;

private:
    // 算式里的一个记号：要么是数字，要么是运算符
    struct Token {
        enum Kind { Number, Operator };
        Kind    kind;
        QString text;
    };

    void clearAll();                        // C
    void clearEntry();                      // CE
    void inputDigit(QChar digit);           // 0~9
    void inputDot();                        // .
    void inputOperator(const QString &op);  // + - × ÷
    void evaluate();                        // =
    void backspace();                       // <-
    void negate();                          // ±
    void applyUnary(const QString &op);     // 1/x  x²  √x  %

    bool     hasOpenNumber() const;         // 末尾是不是「正在输入的数字」
    QString &openNumber();                  // 取该数字的引用（调用前需 hasOpenNumber 为真）
    QString  expressionString() const;      // 把 token 拼成 "2+3×4"

    // 按优先级求值：先 × ÷，再 + −
    bool evaluateTokens(const QVector<Token> &tokens, double *result, QString *error) const;

    QVector<Token> m_tokens;
    bool           m_justEvaluated;       // 刚按过 =：大字显示结果
    QString        m_finishedExpression;  // 按 = 时定格的算式，供小字显示
    QString        m_errorMessage;
};

#endif // CALCULATOR_H
