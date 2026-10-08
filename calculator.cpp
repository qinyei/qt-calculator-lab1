#include "calculator.h"

#include <QtMath>

namespace {

// 单个数字最多允许输入这么多位，防止无限输入
const int kMaxDigits = 16;

// 把 double 转成适合显示的字符串。
// 用 12 位有效数字，避免 0.1 + 0.2 显示出 0.30000000000000004 这种浮点噪音。
QString formatNumber(double value)
{
    if (qIsNaN(value) || qIsInf(value))
        return QStringLiteral("0");

    QString text = QString::number(value, 'g', 12);

    // 数值过大/过小时 QString 会给出科学计数法（例如 1e+20）。
    // 计算器上用普通写法更直观，但太长时仍保留科学计数法。
    if (text.contains(QLatin1Char('e')) || text.contains(QLatin1Char('E'))) {
        QString plain = QString::number(value, 'f', 6);
        while (plain.contains(QLatin1Char('.')) && plain.endsWith(QLatin1Char('0')))
            plain.chop(1);
        if (plain.endsWith(QLatin1Char('.')))
            plain.chop(1);
        if (plain.length() <= kMaxDigits)
            text = plain;
    }
    return text;
}

int countDigits(const QString &text)
{
    int count = 0;
    for (const QChar &c : text) {
        if (c.isDigit())
            ++count;
    }
    return count;
}

} // namespace

Calculator::Calculator()
{
    clearAll();
}

// ---------------------------------------------------------------- 公共接口

void Calculator::enter(const QString &key)
{
    if (key.isEmpty())
        return;

    const bool isRecoveryKey = (key == QStringLiteral("C") || key == QStringLiteral("CE"));

    // 出错之后：C / CE 正常恢复，按其它键则先自动复位再继续。
    // （这样使用者不必先按 C 再重新输入，体验更顺畅）
    if (isError() && !isRecoveryKey)
        clearAll();

    if (key == QStringLiteral("C")) {
        clearAll();
    } else if (key == QStringLiteral("CE")) {
        clearEntry();
    } else if (key == QStringLiteral("<-")) {
        backspace();
    } else if (key == QStringLiteral("=")) {
        evaluate();
    } else if (key == QStringLiteral(".")) {
        inputDot();
    } else if (key == QStringLiteral("±")) {
        negate();
    } else if (key == QStringLiteral("+") || key == QStringLiteral("-")
               || key == QStringLiteral("×") || key == QStringLiteral("÷")) {
        inputOperator(key);
    } else if (key == QStringLiteral("1/x") || key == QStringLiteral("x²")
               || key == QStringLiteral("√x") || key == QStringLiteral("%")) {
        applyUnary(key);
    } else if (key.length() == 1 && key.at(0).isDigit()) {
        inputDigit(key.at(0));
    }
    // 其它按键一律忽略
}

QString Calculator::displayText() const
{
    if (isError())
        return m_errorMessage;

    // 刚按过 =：大字显示结果（结果就是算式里唯一的那个数字）
    if (m_justEvaluated && hasOpenNumber())
        return m_tokens.last().text;

    const QString expression = expressionString();
    return expression.isEmpty() ? QStringLiteral("0") : expression;
}

QString Calculator::expressionText() const
{
    if (isError() || !m_justEvaluated || m_finishedExpression.isEmpty())
        return QString();
    return m_finishedExpression + QStringLiteral(" =");
}

bool Calculator::isError() const
{
    return !m_errorMessage.isEmpty();
}

// ---------------------------------------------------------------- 清除类

void Calculator::clearAll()
{
    m_tokens.clear();
    m_justEvaluated = false;
    m_finishedExpression.clear();
    m_errorMessage.clear();
}

void Calculator::clearEntry()
{
    if (m_justEvaluated) {                 // 结果状态按 CE：整条清掉
        clearAll();
        return;
    }

    if (!hasOpenNumber())
        m_tokens.append(Token{Token::Number, QStringLiteral("0")});
    else
        openNumber() = QStringLiteral("0");
}

// ---------------------------------------------------------------- 输入类

void Calculator::inputDigit(QChar digit)
{
    // ★ 边界：按过 = 之后再输数字，应当开一条新算式，而不是接在结果后面
    if (m_justEvaluated)
        clearAll();

    if (!hasOpenNumber()) {
        m_tokens.append(Token{Token::Number, QString(digit)});
        return;
    }

    QString &number = openNumber();
    if (number == QStringLiteral("0")) {
        number = QString(digit);
        return;
    }
    if (number == QStringLiteral("-0")) {   // -0 之后继续输入
        number = QStringLiteral("-") + digit;
        return;
    }
    if (countDigits(number) >= kMaxDigits)
        return;                             // 到达位数上限

    number.append(digit);
}

void Calculator::inputDot()
{
    if (m_justEvaluated)
        clearAll();

    if (!hasOpenNumber()) {                 // 空位直接按小数点：从 "0." 开始
        m_tokens.append(Token{Token::Number, QStringLiteral("0.")});
        return;
    }

    // ★ 关键：同一个数里已经出现过小数点，就忽略这次输入，
    //   避免出现 "1.2.3" 这种非法数字。
    QString &number = openNumber();
    if (number.contains(QLatin1Char('.')))
        return;

    number.append(QLatin1Char('.'));
}

void Calculator::inputOperator(const QString &op)
{
    m_justEvaluated = false;                // 结果继续参与运算时，大字改回显示算式

    // ★ 连续按运算符（例如 "1 + ×"）：只替换运算符，不追加
    if (!m_tokens.isEmpty() && m_tokens.last().kind == Token::Operator) {
        m_tokens.last().text = op;
        return;
    }

    // 算式为空时先补一个 0（例如开机直接按 "+" 等价于 "0+"）
    if (m_tokens.isEmpty())
        m_tokens.append(Token{Token::Number, QStringLiteral("0")});

    m_tokens.append(Token{Token::Operator, op});
}

void Calculator::evaluate()
{
    if (m_tokens.isEmpty())
        return;

    // 末尾如果是运算符，按 = 时先把它去掉（"2+3+" 等价于 "2+3"）
    while (!m_tokens.isEmpty() && m_tokens.last().kind == Token::Operator)
        m_tokens.removeLast();
    if (m_tokens.isEmpty())
        return;

    const QString finished = expressionString();   // 定格「刚才那条算式」，供小字显示

    double  result = 0.0;
    QString error;
    if (!evaluateTokens(m_tokens, &result, &error)) {
        m_errorMessage = error;
        return;
    }

    m_finishedExpression = finished;
    m_tokens.clear();
    m_tokens.append(Token{Token::Number, formatNumber(result)});
    m_justEvaluated = true;
}

// ---------------------------------------------------------------- 编辑类

void Calculator::backspace()
{
    if (m_justEvaluated || m_tokens.isEmpty())   // 结果不做退格
        return;

    // 末尾是运算符：把运算符删掉
    if (m_tokens.last().kind == Token::Operator) {
        m_tokens.removeLast();
        return;
    }

    QString &number = openNumber();
    number.chop(1);
    if (number.isEmpty() || number == QStringLiteral("-"))
        number = QStringLiteral("0");            // ★ 退格退到空 -> 回到 0，而不是空字符串
}

void Calculator::negate()
{
    m_justEvaluated = false;

    if (!hasOpenNumber())
        m_tokens.append(Token{Token::Number, QStringLiteral("0")});

    QString &number = openNumber();
    if (number == QStringLiteral("0"))
        return;                                  // 0 取反还是 0

    if (number.startsWith(QLatin1Char('-')))
        number.remove(0, 1);
    else
        number.prepend(QLatin1Char('-'));
}

void Calculator::applyUnary(const QString &op)
{
    m_justEvaluated = false;

    if (!hasOpenNumber())
        m_tokens.append(Token{Token::Number, QStringLiteral("0")});

    const double value = openNumber().toDouble();
    double  result = value;
    QString error;

    if (op == QStringLiteral("1/x")) {
        if (value == 0.0)
            error = QStringLiteral("除数不能为 0");
        else
            result = 1.0 / value;
    } else if (op == QStringLiteral("x²")) {
        result = value * value;
    } else if (op == QStringLiteral("√x")) {
        if (value < 0.0)
            error = QStringLiteral("负数不能开平方");
        else
            result = qSqrt(value);
    } else if (op == QStringLiteral("%")) {
        result = value / 100.0;
    }

    if (!error.isEmpty()) {
        m_errorMessage = error;
        return;
    }

    openNumber() = formatNumber(result);
}

// ---------------------------------------------------------------- 工具

bool Calculator::hasOpenNumber() const
{
    return !m_tokens.isEmpty() && m_tokens.last().kind == Token::Number;
}

QString &Calculator::openNumber()
{
    return m_tokens.last().text;
}

QString Calculator::expressionString() const
{
    QString text;
    for (const Token &token : m_tokens)
        text += token.text;
    return text;
}

bool Calculator::evaluateTokens(const QVector<Token> &tokens, double *result, QString *error) const
{
    QVector<double>  values;
    QVector<QString> operators;

    for (const Token &token : tokens) {
        if (token.kind == Token::Number)
            values.append(token.text.toDouble());
        else
            operators.append(token.text);
    }

    if (values.isEmpty()) {
        *result = 0.0;
        return true;
    }

    // ---- 第一轮：先算乘除（× ÷ 优先级高于 + −）----
    int i = 0;
    while (i < operators.size() && i + 1 < values.size()) {
        const QString op = operators.at(i);
        if (op == QStringLiteral("×") || op == QStringLiteral("÷")) {
            const double left  = values.at(i);
            const double right = values.at(i + 1);

            if (op == QStringLiteral("÷") && right == 0.0) {   // ★ 边界：除数为 0
                *error = QStringLiteral("除数不能为 0");
                return false;
            }

            values[i] = (op == QStringLiteral("×")) ? left * right : left / right;
            values.removeAt(i + 1);
            operators.removeAt(i);
        } else {
            ++i;                                  // 加减先跳过，留给第二轮
        }
    }

    // ---- 第二轮：再算加减（同级从左到右）----
    double acc = values.at(0);
    for (int k = 0; k < operators.size() && k + 1 < values.size(); ++k) {
        if (operators.at(k) == QStringLiteral("+"))
            acc += values.at(k + 1);
        else
            acc -= values.at(k + 1);
    }

    *result = acc;
    return true;
}
