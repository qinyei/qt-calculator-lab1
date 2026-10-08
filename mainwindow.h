#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "calculator.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    // 键盘事件：把按键翻译成与按钮文字相同的记号，转交 handleKey()
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    // 所有按钮共用这一个槽（槽内用 sender() 区分是哪个按钮）
    void onButtonClicked();

private:
    // ★ 鼠标与键盘唯一的共同处理入口，内部只调用 Calculator::enter()
    void handleKey(const QString &key);

    void refreshDisplay();
    void setupButtons();
    void applyStyleSheet();

    // 把 Qt 的按键码翻译成输入记号；不认识的键返回空串
    static QString tokenFromKey(int key);
    // 把按钮文字翻译成输入记号（只有退格键的显示文字与记号不同）
    static QString tokenFromButtonText(const QString &text);

    Ui::MainWindow *ui;
    Calculator      m_calculator;
};

#endif // MAINWINDOW_H
