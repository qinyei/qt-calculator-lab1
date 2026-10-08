/*
 * 计算核心的自动化测试（不依赖界面）。
 *
 * 因为 Calculator 与 GUI 完全解耦，所以可以在这里逐条验证：
 *   - 运算优先级（先 × ÷ 后 + −）
 *   - 实验要求里的边界情况：连续小数点、连续运算符、除数为 0、
 *     算完后继续输入、输入过程中退格、退格退到空……
 *
 * 运行方式见同目录的 test_calculator.pro（qmake + make），也可以由 Qt Creator 直接运行。
 */
#include <QCoreApplication>
#include <QString>
#include <QStringList>
#include <QTextStream>

#include "../calculator.h"

static int g_pass = 0;
static int g_fail = 0;

// 模拟连续按键：把 "2 + 3 =" 这样的字符串按空格拆开逐次送入
static void press(Calculator &calc, const QString &keys)
{
    const QStringList list = keys.split(QLatin1Char(' '), QString::SkipEmptyParts);
    for (const QString &key : list)
        calc.enter(key);
}

static void report(QTextStream &out, bool ok, const QString &name,
                   const QString &actual, const QString &expected)
{
    if (ok) {
        ++g_pass;
        out << QStringLiteral("[PASS] ") << name << QStringLiteral("  ->  \"") << actual << QStringLiteral("\"\n");
    } else {
        ++g_fail;
        out << QStringLiteral("[FAIL] ") << name
            << QStringLiteral("  ->  实际 \"") << actual
            << QStringLiteral("\"，期望 \"") << expected << QStringLiteral("\"\n");
    }
    out.flush();
}

// 检查按键序列后「大字」显示的内容
static void check(QTextStream &out, const QString &name,
                  const QString &keys, const QString &expected)
{
    Calculator calc;
    press(calc, keys);
    report(out, calc.displayText() == expected, name, calc.displayText(), expected);
}

// 检查按键序列后「小字」显示的内容
static void checkExpression(QTextStream &out, const QString &name,
                            const QString &keys, const QString &expected)
{
    Calculator calc;
    press(calc, keys);
    report(out, calc.expressionText() == expected, name, calc.expressionText(), expected);
}

// 检查是否进入错误状态
static void checkError(QTextStream &out, const QString &name,
                       const QString &keys, const QString &expectedError)
{
    Calculator calc;
    press(calc, keys);
    const bool ok = calc.isError() && calc.displayText() == expectedError;
    report(out, ok, name, calc.displayText(), expectedError);
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QTextStream out(stdout);
    out.setCodec("UTF-8");

    out << QStringLiteral("========== 计算器核心测试（整式求值） ==========\n\n");

    out << QStringLiteral("--- 1. 基本四则运算 ---\n");
    check(out, QStringLiteral("2 + 3 ="), QStringLiteral("2 + 3 ="), QStringLiteral("5"));
    check(out, QStringLiteral("9 - 4 ="), QStringLiteral("9 - 4 ="), QStringLiteral("5"));
    check(out, QStringLiteral("3 × 5 ="), QStringLiteral("3 × 5 ="), QStringLiteral("15"));
    check(out, QStringLiteral("8 ÷ 2 ="), QStringLiteral("8 ÷ 2 ="), QStringLiteral("4"));
    check(out, QStringLiteral("减法得负数 2 - 5 ="), QStringLiteral("2 - 5 ="), QStringLiteral("-3"));

    out << QStringLiteral("\n--- 2. 运算优先级（先 × ÷，后 + −）---\n");
    check(out, QStringLiteral("2 + 3 × 4 =  → 应先算乘法，得 14（不是 20）"),
          QStringLiteral("2 + 3 × 4 ="), QStringLiteral("14"));
    check(out, QStringLiteral("2 × 3 + 4 ="), QStringLiteral("2 × 3 + 4 ="), QStringLiteral("10"));
    check(out, QStringLiteral("10 - 2 × 3 ="), QStringLiteral("1 0 - 2 × 3 ="), QStringLiteral("4"));
    check(out, QStringLiteral("2 + 3 × 4 - 6 ÷ 3 =  → 2+12-2"),
          QStringLiteral("2 + 3 × 4 - 6 ÷ 3 ="), QStringLiteral("12"));
    check(out, QStringLiteral("同级从左到右 8 ÷ 4 ÷ 2 ="),
          QStringLiteral("8 ÷ 4 ÷ 2 ="), QStringLiteral("1"));
    check(out, QStringLiteral("同级从左到右 10 - 4 - 3 ="),
          QStringLiteral("1 0 - 4 - 3 ="), QStringLiteral("3"));
    check(out, QStringLiteral("小数参与 1.5 × 2 + 0.5 ="),
          QStringLiteral("1 . 5 × 2 + 0 . 5 ="), QStringLiteral("3.5"));

    out << QStringLiteral("\n--- 3. 输入过程中显示整条算式 ---\n");
    check(out, QStringLiteral("输入 2 + 3 × 4（还没按 =）"),
          QStringLiteral("2 + 3 × 4"), QStringLiteral("2+3×4"));
    checkExpression(out, QStringLiteral("按 = 后小字显示原式"),
                    QStringLiteral("2 + 3 × 4 ="), QStringLiteral("2+3×4 ="));

    out << QStringLiteral("\n--- 4. 连续运算（中途不再提前出结果）---\n");
    check(out, QStringLiteral("按到 2 + 3 + 时仍显示算式"),
          QStringLiteral("2 + 3 +"), QStringLiteral("2+3+"));
    check(out, QStringLiteral("2 + 3 + 4 ="), QStringLiteral("2 + 3 + 4 ="), QStringLiteral("9"));
    check(out, QStringLiteral("2 + 3 × 4 + 5 =  → 2+12+5"),
          QStringLiteral("2 + 3 × 4 + 5 ="), QStringLiteral("19"));

    out << QStringLiteral("\n--- 5. 小数与浮点噪音 ---\n");
    check(out, QStringLiteral("1.5 + 2.5 ="), QStringLiteral("1 . 5 + 2 . 5 ="), QStringLiteral("4"));
    check(out, QStringLiteral("0.1 + 0.2 =（不应出现 0.30000000000000004）"),
          QStringLiteral("0 . 1 + 0 . 2 ="), QStringLiteral("0.3"));

    out << QStringLiteral("\n--- 6. 边界：连续输入小数点 ---\n");
    check(out, QStringLiteral("1.2.3（第二个小数点应被忽略）"),
          QStringLiteral("1 . 2 . 3"), QStringLiteral("1.23"));
    check(out, QStringLiteral("连按两个小数点 . ."), QStringLiteral(". ."), QStringLiteral("0."));
    check(out, QStringLiteral(". . 5"), QStringLiteral(". . 5"), QStringLiteral("0.5"));
    check(out, QStringLiteral("1 . . 5"), QStringLiteral("1 . . 5"), QStringLiteral("1.5"));
    check(out, QStringLiteral("1.2 + 3.4.5（第二个数里也忽略）"),
          QStringLiteral("1 . 2 + 3 . 4 . 5"), QStringLiteral("1.2+3.45"));

    out << QStringLiteral("\n--- 7. 边界：连续输入运算符 ---\n");
    check(out, QStringLiteral("1 + × 2 =（中间换运算符）"),
          QStringLiteral("1 + × 2 ="), QStringLiteral("2"));
    check(out, QStringLiteral("5 + + + 3 ="), QStringLiteral("5 + + + 3 ="), QStringLiteral("8"));
    check(out, QStringLiteral("1 + - 2 =（换成减号）"),
          QStringLiteral("1 + - 2 ="), QStringLiteral("-1"));
    check(out, QStringLiteral("开机直接按 + 3 =（补 0）"),
          QStringLiteral("+ 3 ="), QStringLiteral("3"));

    out << QStringLiteral("\n--- 8. 边界：除数为 0 ---\n");
    checkError(out, QStringLiteral("5 ÷ 0 ="), QStringLiteral("5 ÷ 0 ="), QStringLiteral("除数不能为 0"));
    checkError(out, QStringLiteral("0 ÷ 0 ="), QStringLiteral("0 ÷ 0 ="), QStringLiteral("除数不能为 0"));
    checkError(out, QStringLiteral("算式中间的除零 1 + 5 ÷ 0 ="),
               QStringLiteral("1 + 5 ÷ 0 ="), QStringLiteral("除数不能为 0"));
    checkError(out, QStringLiteral("0 的倒数 1/x"), QStringLiteral("0 1/x"), QStringLiteral("除数不能为 0"));
    check(out, QStringLiteral("出错后按 7 应自动复位"), QStringLiteral("5 ÷ 0 = 7"), QStringLiteral("7"));
    check(out, QStringLiteral("出错后按 C 应清零"), QStringLiteral("5 ÷ 0 = C"), QStringLiteral("0"));

    out << QStringLiteral("\n--- 9. 边界：算完后继续输入 ---\n");
    check(out, QStringLiteral("2 + 3 = 7（应开新算式，不是 57）"),
          QStringLiteral("2 + 3 = 7"), QStringLiteral("7"));
    check(out, QStringLiteral("2 + 3 = . 5"), QStringLiteral("2 + 3 = . 5"), QStringLiteral("0.5"));
    check(out, QStringLiteral("算完连按 = 不应出错"), QStringLiteral("5 = = ="), QStringLiteral("5"));
    check(out, QStringLiteral("用结果继续算 2 + 3 = + 4 ="),
          QStringLiteral("2 + 3 = + 4 ="), QStringLiteral("9"));
    check(out, QStringLiteral("用结果继续算 2 + 3 = × 2 ="),
          QStringLiteral("2 + 3 = × 2 ="), QStringLiteral("10"));

    out << QStringLiteral("\n--- 10. 边界：输入过程中退格 ---\n");
    check(out, QStringLiteral("1 2 3 <-"), QStringLiteral("1 2 3 <-"), QStringLiteral("12"));
    check(out, QStringLiteral("5 <-（退到空应回到 0）"), QStringLiteral("5 <-"), QStringLiteral("0"));
    check(out, QStringLiteral("1 . <-"), QStringLiteral("1 . <-"), QStringLiteral("1"));
    check(out, QStringLiteral("连续退格到底"), QStringLiteral("1 2 <- <- <- <- <-"), QStringLiteral("0"));
    check(out, QStringLiteral("负数退格"), QStringLiteral("4 2 ± <-"), QStringLiteral("-4"));
    check(out, QStringLiteral("退格删掉运算符"), QStringLiteral("2 + <-"), QStringLiteral("2"));
    check(out, QStringLiteral("算式中途退格到底"), QStringLiteral("2 + 3 <- <-"), QStringLiteral("2+0"));

    out << QStringLiteral("\n--- 11. 清除 ---\n");
    check(out, QStringLiteral("1 2 3 C"), QStringLiteral("1 2 3 C"), QStringLiteral("0"));
    check(out, QStringLiteral("2 + 3 C（C 清掉整条算式）"), QStringLiteral("2 + 3 C"), QStringLiteral("0"));
    check(out, QStringLiteral("5 + 3 CE =（CE 只清当前输入的数）"),
          QStringLiteral("5 + 3 CE ="), QStringLiteral("5"));
    check(out, QStringLiteral("5 + 3 CE 4 ="), QStringLiteral("5 + 3 CE 4 ="), QStringLiteral("9"));

    out << QStringLiteral("\n--- 12. 正负号 ---\n");
    check(out, QStringLiteral("5 ±"), QStringLiteral("5 ±"), QStringLiteral("-5"));
    check(out, QStringLiteral("5 ± ±"), QStringLiteral("5 ± ±"), QStringLiteral("5"));
    check(out, QStringLiteral("0 ±"), QStringLiteral("0 ±"), QStringLiteral("0"));
    check(out, QStringLiteral("-3 × -4 ="), QStringLiteral("3 ± × 4 ± ="), QStringLiteral("12"));
    check(out, QStringLiteral("2 + 3 ± ="), QStringLiteral("2 + 3 ± ="), QStringLiteral("-1"));

    out << QStringLiteral("\n--- 13. 一元运算 ---\n");
    check(out, QStringLiteral("25 √x"), QStringLiteral("2 5 √x"), QStringLiteral("5"));
    check(out, QStringLiteral("4 x²"), QStringLiteral("4 x²"), QStringLiteral("16"));
    check(out, QStringLiteral("4 1/x"), QStringLiteral("4 1/x"), QStringLiteral("0.25"));
    check(out, QStringLiteral("50 %"), QStringLiteral("5 0 %"), QStringLiteral("0.5"));
    checkError(out, QStringLiteral("9 ± √x（负数开平方）"),
               QStringLiteral("9 ± √x"), QStringLiteral("负数不能开平方"));
    check(out, QStringLiteral("算式中一元运算 2 + 9 √x ="),
          QStringLiteral("2 + 9 √x ="), QStringLiteral("5"));

    out << QStringLiteral("\n--- 14. 输入上限 ---\n");
    {
        Calculator calc;
        press(calc, QStringLiteral("1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0"));
        report(out, calc.displayText().length() == 16,
               QStringLiteral("连按 20 个数字只保留 16 位"),
               calc.displayText(), QStringLiteral("16 位数字"));
    }

    out << QStringLiteral("\n========== 通过 %1 项，失败 %2 项 ==========\n")
           .arg(g_pass).arg(g_fail);
    out.flush();

    return g_fail == 0 ? 0 : 1;
}
