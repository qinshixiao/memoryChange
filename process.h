#ifndef PROCESS_H
#define PROCESS_H

#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <QGridLayout>
#include <QLineEdit>
#include <QListWidget>
#include <windows.h>
class Process : public QWidget
{
    Q_OBJECT

public:
    Process(QWidget *parent = 0);
    ~Process();
private slots:
    //void checkValue();//查询
    BOOL FindFirst();  // 在目标进程空间进行第一次查找
    BOOL FindNext();  // 在目标进程地址空间进行第2、3、4⋯⋯次查找
    BOOL WriteMemory();//将新值写进内存中

private:
    QTableWidget *table;
    QLabel *promptMessageLabel;
    QLabel *inputValueLabel;//输入要查找的值标签
    QLineEdit *inputValueLEt;//输入要查找的值
    QLabel *updateValueLabel;//要更新的新值标签
    QLineEdit *updateValueLEt;//要更新的新值
    QPushButton *firstScanBtn;//初次扫描
    QPushButton *nextScanBtn;//再次扫描
    QPushButton *updateBtn;//更新
    QListWidget *addressList;//查询到的值所在的地址列表，因为可能有多个地址处的值相同

    void createTabWidget();//创建显示进程的列表
    void showProcessList();//获取系统进程列表并显示
    BOOL SetPrivilege();//提升权限

    BOOL CompareAPage(DWORD dwBaseAddr, DWORD dwValue);//比较值是否是给定的值
    void ShowList();//

    DWORD g_arList[1024];  // 地址列表
    int g_nListCnt=0;  // 有效地址的个数
    HANDLE g_hProcess; // 目标进程句柄
};

#endif // PROCESS_H
