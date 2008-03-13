#ifndef _QMC2_MAIN_H_
#define _QMC2_MAIN_H_

#include <QApplication>
#include <QtGui>
#include "ui_qmc2main.h"

#define LOG_QMC2	1
#define LOG_MAME	2

class MainWindow : public QMainWindow, public Ui::MainWindow
{
Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();
	
	QLineEdit *lineEditSearch;
	QLabel *labelProgress;
	QProgressBar *progressBarGamelist;
	QLabel *labelSnapshot;

public slots:
    // game menu
    void on_actionRefresh_activated();
    void on_actionReload_activated();
	void on_actionExitStop_activated();
	void on_actionDefaultOptions_activated();
    void log(char, QString);
	void init();
	void saveLayout();
	void loadLayout();
protected:
    void closeEvent(QCloseEvent *event);
};

#endif
