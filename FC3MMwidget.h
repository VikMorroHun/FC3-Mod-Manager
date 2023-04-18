#ifndef WIDGET_H
#define WIDGET_H

#include "XMLReader.h"
#include <QWidget>
#include <QMessageBox>
#include <QShortcut>
#include <QDomDocument>
#include <QProcess>
#include <QListWidget>
#include <QListWidgetItem>
#include <QDir>
#include <QScrollBar>

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; class XMLReader; }
QT_END_NAMESPACE

class Widget : public QWidget
{
	Q_OBJECT

public:
	Widget(QWidget *parent = nullptr);
	~Widget();
	void BackupCheck(QString sBackup);
	QDomDocument * pDOMDoc;
	QString fileName, str, sShort, appPath, gameBin;
	QFile fileCheck, targetFile;
	XMLReader * pXMLReader;
	QDir dir;
	QMessageBox msgBox;
	QStringList modList, argList, tempList;
	QProcess * pProcess;
	QByteArray baData, baDataFC3DLLFrom, baDataFC3DLLTo, * pBA;
	bool bChangedLO;
	qint32 iProcState, iModIndex;

private slots:
    void on_exitButtonClicked();

    void on_readXMLButtonClicked();

    void on_shortcutTriggered();

	void on_pushButtonGameDirClicked();

	void on_pushButtonWorkDirClicked();

	void on_pushButtonGamerProfileClicked();

	void on_XMLReceiveStringBasic( QString s);

	void on_ListWidgetCurrItemChanged( QListWidgetItem * pNewItem);

	void on_XMLStringPointerReceiver( QString *);

	void on_deployButtonClicked();

	void on_loadModButtonClicked();

	void on_removeModButtonClicked();

	void on_readyReadSTD();

	void on_processFinished(int, QProcess::ExitStatus);

	void on_uninstallButtonClicked();

	void on_applySettingsButtonClicked();

	void on_checkBoxDX11Clicked();

private:
	Ui::Widget *ui;
    QShortcut shortcut;
	void ProcessStartExtract();
	void ProcessStartExtractLevel1(QString s);
};
#endif // WIDGET_H
