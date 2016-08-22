#include "process.h"
#include <QTableWidgetItem>
#include <windows.h>
#include <tlhelp32.h>//进程快照需要包含这个头文件
#include <QMessageBox>
#include <QDebug>
Process::Process(QWidget *parent)
    : QWidget(parent)
{
    this->setWindowTitle("内存修改器——秦时小——大秦皇家师范");
    this->setMaximumSize(500,500);
    this->setMinimumSize(500,500);

    createTabWidget();
    showProcessList();


    promptMessageLabel=new QLabel(tr("在下列进程中选择一个当前要修改内存的进程："));
    inputValueLabel=new QLabel("输入要查找的值：");
    inputValueLEt=new QLineEdit();
    firstScanBtn=new QPushButton("首次扫描");
    nextScanBtn=new QPushButton("再次扫描");
    updateBtn=new QPushButton("更新");
    updateValueLabel=new QLabel("新的值：");
    updateValueLEt=new QLineEdit();
    addressList=new QListWidget();

    QVBoxLayout *leftLayout=new QVBoxLayout;
    leftLayout->addWidget(promptMessageLabel);
    leftLayout->addWidget(table);
    QGridLayout *rightLayout=new QGridLayout;
    rightLayout->addWidget(inputValueLabel,0,0,1,2);
    rightLayout->addWidget(inputValueLEt,0,2);
    rightLayout->addWidget(firstScanBtn,1,0);
    rightLayout->setRowStretch(1,5);
    rightLayout->addWidget(nextScanBtn,1,2);
    rightLayout->addWidget(addressList,2,0,1,3);
    rightLayout->addWidget(updateValueLabel,3,0);
    rightLayout->addWidget(updateValueLEt,3,1);
    rightLayout->addWidget(updateBtn,3,2);

    QHBoxLayout *mainLayout=new QHBoxLayout(this);
    mainLayout->addLayout(leftLayout);
    mainLayout->addLayout(rightLayout);





   SetPrivilege();







    connect(firstScanBtn,SIGNAL(clicked()),this,SLOT(FindFirst()));
    connect(nextScanBtn,SIGNAL(clicked()),this,SLOT(FindNext()));
    connect(updateBtn,SIGNAL(clicked()),this,SLOT(WriteMemory()));
}

Process::~Process()
{
}

void Process::createTabWidget()
{
    table=new QTableWidget();
    table->setColumnCount(2);
    table->setColumnWidth(0,300);
    table->setColumnWidth(1,200);
    table->resize(this->size());
    QStringList list;
    list<<"进程名"<<"ID";
    table->setHorizontalHeaderLabels(list);
}

void Process::showProcessList()
{
    PROCESSENTRY32 pe32;//用来存放快照进程信息的一个结构体。（存放进程信息和调用成员输出进程信息）
    // 在使用这个结构之前，先设置它的大小
    pe32.dwSize = sizeof(pe32);
    // 给系统内的所有进程拍一个快照,返回快照的句柄
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if(hProcessSnap == INVALID_HANDLE_VALUE)
    {  QMessageBox::warning(this,"提示","CreateToolhelp32Snapshot调用失败！");
    }

    // 遍历进程快照，轮流显示每个进程的信息
    else
    {
        //获取快照中第一条进程信息，并将其放在
        BOOL bMore = Process32First(hProcessSnap, &pe32);//process32First 是一个进程获取函数,当我们利用函数
                                                        //CreateToolhelp32Snapshot()获得当前运行进程的快照后,
                                                        //我们可以利用process32First函数来获得第一个进程的句柄
        int i=0;
        while(bMore)
        {
            table->insertRow(i);
            //因为pe32.szExeFile的返回值是wchar_t，所以用fromWCharArray函数
            table->setItem(i,0,new QTableWidgetItem(QString::fromWCharArray(pe32.szExeFile)));
            table->setItem(i,1,new QTableWidgetItem(QString::number(pe32.th32ProcessID)));
            bMore = Process32Next(hProcessSnap, &pe32);//获取下一个进程句柄
            i++;
        }
    }
    // 不要忘记清除掉snapshot对象
    CloseHandle(hProcessSnap);
}

BOOL Process::CompareAPage(DWORD dwBaseAddr, DWORD dwValue)
{  // 读取1 页内存
    BYTE arBytes[4096];
    if(!ReadProcessMemory(g_hProcess, (LPVOID)dwBaseAddr, arBytes, 4096, NULL))
    {
        qDebug()<<"读取失败\n";
        return FALSE;  // 此页不可读
    }
    // 在这1 页内存中查找
    DWORD* pdw;
    int i;
    for(i=0; i<(int)4*1024-3; i++)
    {
        pdw = (DWORD*)&arBytes[i];
        if(pdw[0] == dwValue) // 等于要查找的值？
        {
            if(g_nListCnt >= 1024)
            return FALSE;
            // 添加到全局变量中
            g_arList[g_nListCnt++] = dwBaseAddr + i;
            qDebug()<<"查找到了值！\n";
        }
    }
    return TRUE;
}

BOOL Process::FindFirst()
{
    DWORD dwValue=inputValueLEt->text().toUInt();
    const DWORD dwOneGB = 1024*1024*1024; // 1GB
    const DWORD dwOnePage = 4*1024;  // 4KB
    g_hProcess=OpenProcess(PROCESS_ALL_ACCESS, FALSE, table->item(table->currentRow(),1)->text().toUInt());
    if(g_hProcess == NULL)
    {
        qDebug()<<"\n没有获取到进程句柄\n";
    return FALSE;
    }

    // 查看操作系统类型，以决定开始地址
    DWORD dwBase;
    OSVERSIONINFO vi = { sizeof(vi) };
    GetVersionEx(&vi);
    if (vi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
    dwBase = 4*1024*1024; // Windows 98系列，4MB
    else
    dwBase = 640*1024;  // Windows NT系列，64KB
    // 在开始地址到2GB的地址空间进行查找
    for(; dwBase < 2*dwOneGB; dwBase += dwOnePage)
    {
        // 比较1 页大小的内存
        CompareAPage(dwBase, dwValue);
    }
    ShowList();
    return TRUE;
}

BOOL Process::FindNext()
{
    addressList->clear();
    DWORD dwValue=inputValueLEt->text().toUInt();
    qDebug()<<"再次扫描\n";
    // 保存m_arList数组中有效地址的个数，初始化新的m_nListCnt值
    int nOrgCnt = g_nListCnt;
    qDebug()<<"listCnt:"<<g_nListCnt<<endl;
    g_nListCnt = 0;
    // 在m_arList数组记录的地址处查找
    BOOL bRet = FALSE; // 假设失败
    DWORD dwReadValue;
    int i;
    for(i=0; i<nOrgCnt; i++)
    {  if(ReadProcessMemory(g_hProcess, (LPVOID)g_arList[i], &dwReadValue, sizeof(DWORD), NULL))
        {
            if(dwReadValue == dwValue)
            {
                qDebug()<<"没有执行？\n";
                g_arList[g_nListCnt++] = g_arList[i];
                bRet = TRUE;
            }
        }
    }
    qDebug()<<"执行showList\n";
    ShowList();
    return bRet;
}

BOOL Process::WriteMemory()
{
    DWORD dwAddr=g_arList[0];
    DWORD dwValue=updateValueLEt->text().toUInt();
    return WriteProcessMemory(g_hProcess, (LPVOID)dwAddr, &dwValue, sizeof(DWORD), NULL);
}

void Process::ShowList()
{
    //printf("g_nListCnt:%d",g_nListCnt);
    int i;
    for(i=0; i < g_nListCnt; i++)
    {
        QString s;
        addressList->insertItem(i,s.number(g_arList[i]));
    }

}









BOOL Process::SetPrivilege()
{
HANDLE hProcess, hToken;
TOKEN_PRIVILEGES NewState;
LUID luidPrivilegeLUID;
hProcess = GetCurrentProcess();
if(!OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES, &hToken))
return FALSE;
if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luidPrivilegeLUID))
return FALSE;
NewState.PrivilegeCount = 1;
NewState.Privileges[0].Luid = luidPrivilegeLUID;
NewState.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
if(!AdjustTokenPrivileges(hToken, FALSE, &NewState, NULL, NULL, NULL))
return FALSE;
return TRUE;
}
