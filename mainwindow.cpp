#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QKeyEvent>
#include <QPushButton>
#include <QSizePolicy>
#include <QStringList>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setupButtons();
    applyStyleSheet();
    refreshDisplay();

    // 键盘焦点始终留在主窗口上：显示区是 NoFocus，按钮也都是 NoFocus，
    // 因此所有按键都会走到 keyPressEvent()。
    setFocusPolicy(Qt::StrongFocus);
    setFocus();
}

MainWindow::~MainWindow()
{
    delete ui;
}

// ---------------------------------------------------------------- 界面初始化

void MainWindow::setupButtons()
{
    // 所有按钮连到同一个槽：槽里通过 sender() 取按钮文字。
    // 这样新增按钮不需要再写新的槽函数，从根上避免大段重复代码。
    const QList<QPushButton *> buttons = findChildren<QPushButton *>();

    // 这些键按「功能键」样式显示（浅灰底），其余按数字键样式显示
    const QStringList functionKeys = QStringList()
            << QStringLiteral("%") << QStringLiteral("CE") << QStringLiteral("C")
            << QStringLiteral("⌫")
            << QStringLiteral("1/x") << QStringLiteral("x²") << QStringLiteral("√x");

    for (QPushButton *button : buttons) {
        button->setFocusPolicy(Qt::NoFocus);                          // 焦点不落到按钮上
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        // 把文字转换成计算核心认识的记号；顺带作为按钮的标识属性
        button->setProperty("token", tokenFromButtonText(button->text()));

        if (button->text() == QStringLiteral("="))
            button->setProperty("role", "equals");
        else if (functionKeys.contains(button->text()))
            button->setProperty("role", "function");
        else
            button->setProperty("role", "digit");

        connect(button, &QPushButton::clicked, this, &MainWindow::onButtonClicked);
    }
}

void MainWindow::applyStyleSheet()
{
    // 用样式表统一美化界面：显示区白底大字号，功能键浅灰，等号键高亮。
    // 属性选择器（[role="..."]）用来给不同作用的按钮上不同颜色。
    const QString style = QStringLiteral(R"(
        QMainWindow, #centralwidget {
            background: #f3f4f6;
        }

        #expressionLabel {
            color: #6b7280;
            font-size: 15px;
            padding-right: 14px;
        }

        #displayEdit {
            background: #ffffff;
            border: 1px solid #dcdfe4;
            border-radius: 10px;
            color: #111827;
            font-size: 30px;
            font-weight: 600;
            padding: 4px 14px;
            min-height: 66px;
        }

        QPushButton {
            background: #ffffff;
            border: 1px solid #e3e5e9;
            border-radius: 8px;
            color: #111827;
            font-size: 18px;
            min-height: 48px;
        }
        QPushButton:hover {
            background: #f0f6ff;
            border-color: #b9d6ff;
        }
        QPushButton:pressed {
            background: #dbeafe;
        }

        QPushButton[role="function"] {
            background: #f8f9fa;
            color: #374151;
            font-size: 15px;
        }

        QPushButton[role="equals"] {
            background: #2f7de1;
            border-color: #2f7de1;
            color: #ffffff;
            font-size: 22px;
            font-weight: 700;
        }
        QPushButton[role="equals"]:hover {
            background: #3d8bef;
        }
        QPushButton[role="equals"]:pressed {
            background: #2568c4;
        }
    )");
    setStyleSheet(style);
}

void MainWindow::refreshDisplay()
{
    ui->displayEdit->setText(m_calculator.displayText());
    ui->expressionLabel->setText(m_calculator.expressionText());
}

// ---------------------------------------------------------------- 输入处理

void MainWindow::onButtonClicked()
{
    QPushButton *button = qobject_cast<QPushButton *>(sender());
    if (!button)
        return;

    handleKey(button->property("token").toString());
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    const QString key = tokenFromKey(event->key());
    if (!key.isEmpty()) {
        handleKey(key);
        event->accept();
        return;
    }

    QMainWindow::keyPressEvent(event);   // 不认识的键交回基类处理
}

// ★ 鼠标与键盘唯一的共同入口：两条路径都在这里汇合，再交给 Calculator
void MainWindow::handleKey(const QString &key)
{
    if (key.isEmpty())
        return;

    m_calculator.enter(key);
    refreshDisplay();
}

// ---------------------------------------------------------------- 按键映射

QString MainWindow::tokenFromKey(int key)
{
    if (key >= Qt::Key_0 && key <= Qt::Key_9)
        return QString::number(key - Qt::Key_0);

    switch (key) {
    case Qt::Key_Period:    return QStringLiteral(".");
    case Qt::Key_Plus:      return QStringLiteral("+");
    case Qt::Key_Minus:     return QStringLiteral("-");
    case Qt::Key_Asterisk:
    case Qt::Key_multiply:  return QStringLiteral("×");   // 小键盘的 *
    case Qt::Key_Slash:     return QStringLiteral("÷");
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Equal:     return QStringLiteral("=");
    case Qt::Key_Backspace: return QStringLiteral("<-");
    case Qt::Key_Escape:    return QStringLiteral("C");   // Esc 等同全部清除
    case Qt::Key_Delete:    return QStringLiteral("CE");  // Delete 等同清除当前输入
    case Qt::Key_Percent:   return QStringLiteral("%");
    default:                return QString();
    }
}

QString MainWindow::tokenFromButtonText(const QString &text)
{
    // 退格键在界面上显示 ⌫，但计算核心内部统一用 "<-"
    if (text == QStringLiteral("⌫"))
        return QStringLiteral("<-");
    return text;
}
