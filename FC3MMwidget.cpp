/***************************************************************************
 *   Copyright (C) 2023 by Viktor Dózsa                                    *
 *   luke2135@gmail.com                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; version 3 of the License.               *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses>.   *
 ***************************************************************************/

#include "FC3MMwidget.h"
#include "ui_FC3MMwidget.h"
#define TEST 0				// 0 - release, 1 - normal test, 2 - listWidget test data
#define VERSION "1.0.29"		// majorversion.minorversion.build

Widget::Widget(QWidget *parent)
	: QWidget(parent)
    , ui(new Ui::Widget), shortcut (QKeySequence(tr("Esc")), this, SLOT(on_shortcutTriggered()))
{
	ui->setupUi(this);
	QString s3 = "QProgressBar { background-color: grey; color: white; border: 0.5px solid black;} QProgressBar::chunk {background: green} ";	// works this way only!
	ui->progressBar->setAlignment(Qt::AlignRight);									//lehet transparent; is
	ui->progressBar->setStyleSheet(s3);
	ui->progressBar->setValue(0);
	pDOMDoc = NULL;pXMLReader = NULL;pProcess = NULL;			// to avoid possible segmentation fault
	ui->textEdit->setReadOnly(true);setWindowTitle( tr( "Far Cry 3 Mod Manager v" ) + VERSION );
	connect( ui->exitButton, SIGNAL(clicked()), this, SLOT( on_exitButtonClicked() ) );
	connect( ui->readXMLButton, SIGNAL(clicked()), this, SLOT( on_readXMLButtonClicked() ) );     //variation: https://doc.qt.io/qt-6/qobject.html#connect-2
	connect( ui->gameDirButton, SIGNAL(clicked()), this, SLOT( on_pushButtonGameDirClicked() ) );
	connect( ui->workDirButton, SIGNAL( clicked()), this, SLOT( on_pushButtonWorkDirClicked() ) );
	connect( ui->profileButton, SIGNAL( clicked() ), this, SLOT( on_pushButtonGamerProfileClicked() ) );
	connect( ui->loadModButton, SIGNAL( clicked(bool)), this, SLOT( on_loadModButtonClicked()) );
	connect( ui->deployButton, SIGNAL( clicked(bool)), this, SLOT( on_deployButtonClicked()) );
	connect( ui->removeModButton, SIGNAL( clicked(bool)), this, SLOT( on_removeModButtonClicked()) );
	connect( ui->uninstallButton, SIGNAL( clicked()), this, SLOT( on_uninstallButtonClicked() ) );
	connect( ui->applySettingsButton, SIGNAL( clicked(bool)), this, SLOT( on_applySettingsButtonClicked() ) );
	connect( ui->checkBoxDX11, SIGNAL( clicked(bool)), this, SLOT( on_checkBoxDX11Clicked()) );
	connect( ui->listWidget, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)), this, SLOT(on_ListWidgetCurrItemChanged(QListWidgetItem*)));
	pDOMDoc = new QDomDocument();iProcState = iModIndex = 0;
	ui->lineEditGamerProfile->setText( QDir::homePath() + "/Documents/My Games/Far Cry 3/GamerProfile.xml" );
	ui->lineEditWorkDir->setText( QDir::tempPath() + "/patch_unpack" );
	if ( TEST == 2 )
	{
		QStringList numbers;
		numbers << "One" << "Two" << "Three" << "Four" << "Five" << "Six";
		ui->listWidget->addItems(numbers);
	}
	ui->listWidget->setDragDropMode(QAbstractItemView::InternalMove);
	if ( pDOMDoc != NULL )
		pXMLReader = new XMLReader(pDOMDoc);
	pProcess = new QProcess(this);
	if ( pProcess == NULL )
		ui->textEdit->append( tr( "Warning!  Can't start new process - can't install mods." ) );
	else
	{
		connect(pProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(on_readyReadSTD()));
		connect(pProcess, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(on_processFinished(int,QProcess::ExitStatus)));
	}
	if ( pXMLReader != NULL )
	{
		connect( pXMLReader, SIGNAL( XMLSendStringBasic(QString)), this, SLOT(on_XMLReceiveStringBasic(QString)));			//sender, signal(Shared * s) didn't work.  neither did (&shared).
		connect( pXMLReader, SIGNAL( XMLStringPointerSender(QString*)), this, SLOT( on_XMLStringPointerReceiver(QString*) ));
	}
	appPath = QDir::current().path();bChangedLO = false;gameBin = "";
	if ( TEST >= 1 )
		ui->textEdit->append( tr("*** TEST MODE ***") );
	if ( !TEST )
		ui->readXMLButton->setVisible(false);
	/*	QTimer *timer = new QTimer(this);
		connect(timer, &QTimer::timeout, this, &Foo::processOneThing);
		timer->start();
	*/
}

/***************************************************************************
 *   Destructor                                                            *
 ***************************************************************************/
Widget::~Widget()
{
	if ( pDOMDoc != NULL )
		delete pDOMDoc;
	if ( pXMLReader != NULL )
		delete pXMLReader;
	if ( pProcess != NULL )
		delete pProcess;
	delete ui;
}
//https://stackoverflow.com/questions/4774291/q-object-throwing-undefined-reference-to-vtable-error
//					https://forum.qt.io/topic/20570/qtcreator-ui-designer-suddenly-fails-to-add-find-slots/5
//+------------------------------------------------------------------+
//| Exit program                                                     |
//| INPUT: none                                                      |
//| OUTPUT: none                                                     |
//| REMARK: by button click                                          |
//+------------------------------------------------------------------+
void Widget::on_exitButtonClicked()             //exitButton_clicked() -> clazy connect warning, https://forum.qt.io/topic/24237/creating-a-slot-with-the-same-name-as-the-auto-generated-one
{
	/*if ( pDOMDoc != NULL )
	{
		QDomElement docElem = pDOMDoc->documentElement();		// The root element is available using documentElement().
		QDomNode docNode = docElem.firstChild();
		qDebug() << "Exit button clicked, here's an element from XML:" << docNode.toElement().text();
	}*/
	if ( pProcess != NULL )
		if ( pProcess->state() == QProcess::Running )
		{
			msgBox.setWindowTitle(tr("Early exit"));
			msgBox.setText(tr("Are you sure you want to quit?"));
			msgBox.setDetailedText( tr( "The install process is still running." ) );
			msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
			msgBox.setDefaultButton(QMessageBox::Cancel);
			int ret = msgBox.exec();
			if ( ret == QMessageBox::Ok )
			{
				if ( pProcess->state() == QProcess::Running )
					pProcess->kill();
				bChangedLO = false;
			}
			else return;
		}
	if ( bChangedLO )
	{
		msgBox.setText( tr("Do you really want to quit?") );
		msgBox.setDetailedText( tr("You've made changes in the Mod Manager.") );
		msgBox.setWindowTitle( tr("Confirm exit") );
		msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
		msgBox.setDefaultButton(QMessageBox::Cancel);
		int iRet = msgBox.exec();
		if ( iRet == QMessageBox::Cancel )
			return;
	}
    close();
}

void Widget::on_readXMLButtonClicked()          //https://forum.qt.io/topic/129896/what-does-slots-named-on_foo_bar-are-error-prone-mean
{
	/*fileName = QFileDialog::getOpenFileName(this, tr("Open XML file to load"), "", "*.xml");
	if ( fileName.isEmpty() )
		return;	*/
	if ( iProcState )
		return;
	fileName = ui->lineEditGamerProfile->text();
	fileCheck.setFileName(fileName);
	if ( !fileCheck.exists() )
	{
		ui->textEdit->append( tr( "%1 does not exist.").arg(fileName) );
		return;
	}
	pXMLReader->ReadXMLTest(pDOMDoc, fileName);
}

//+------------------------------------------------------------------+
//| Exit program                                                     |
//| INPUT: none                                                      |
//| OUTPUT: none                                                     |
//| REMARK: by Esc button press                                      |
//+------------------------------------------------------------------+
void Widget::on_shortcutTriggered()
{
    //qDebug() << "Esc";
	if ( pProcess != NULL )
		if ( pProcess->state() == QProcess::Running )
		{
			msgBox.setWindowTitle(tr("Early exit"));
			msgBox.setText(tr("Are you sure you want to quit?"));
			msgBox.setDetailedText( tr( "The install process is still running." ) );
			msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
			msgBox.setDefaultButton(QMessageBox::Cancel);
			int ret = msgBox.exec();
			if ( ret == QMessageBox::Ok )
			{
				if ( pProcess->state() == QProcess::Running )
					pProcess->kill();
				bChangedLO = false;
			}
			else return;
		}
	if ( bChangedLO )
	{
		msgBox.setText( tr("Do you really want to quit?") );
		msgBox.setDetailedText( tr("You've made changes in the Mod Manager.") );
		msgBox.setWindowTitle( tr("Confirm exit") );
		msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
		msgBox.setDefaultButton(QMessageBox::Cancel);
		int iRet = msgBox.exec();
		if ( iRet == QMessageBox::Cancel )
			return;
	}
    close();
}

void Widget::on_pushButtonGameDirClicked()
{
	if ( iProcState )
		return;
	fileName = QFileDialog::getOpenFileName(this, tr("Select Far Cry 3 directory"), "", "farcry*.exe patch.?at");
	if ( fileName.isEmpty() )
		return;
	if ( fileName.contains( ".exe", Qt::CaseInsensitive ) )
	{
		gameBin = fileName.remove( fileName.lastIndexOf("/"), fileName.size() );
		//ui->textEdit->append( "game bin: " + gameBin );
		fileName = fileName.remove( fileName.lastIndexOf( "bin", Qt::CaseInsensitive ), fileName.size() );
		fileName += "data_win32";
	}
	if ( fileName.contains( ".dat", Qt::CaseInsensitive ) || fileName.contains( ".fat", Qt::CaseInsensitive ) )
	{
		fileName = fileName.remove( fileName.lastIndexOf( "/", Qt::CaseInsensitive ), fileName.size() );
		gameBin = fileName.remove( fileName.lastIndexOf( "/", Qt::CaseInsensitive ), fileName.size() );
		gameBin += "/bin";
		fileName += "/data_win32";
		//ui->textEdit->append( "game bin: " + gameBin );
	}
	ui->lineEditGameDir->setText( fileName );
}


void Widget::on_pushButtonWorkDirClicked()
{
	if ( iProcState )
		return;
	fileName = QFileDialog::getExistingDirectory( this, tr( "Select deployment directory" ), ui->lineEditWorkDir->text() );
	if ( fileName.isEmpty() )
		return;
	//qDebug() << "selected dir: " << fileName;		//selected dir:  "C:/Users/luke2/AppData/Local/Temp", OK
	if ( !fileName.endsWith("/patch_unpack", Qt::CaseInsensitive) )
		fileName += "/patch_unpack";
	ui->lineEditWorkDir->setText( fileName );
}


void Widget::on_pushButtonGamerProfileClicked()
{
	if ( iProcState )
		return;
	fileName = QFileDialog::getOpenFileName(this, tr("Select user profile"), ui->lineEditGamerProfile->text(), "GamerProfile.xml");
	if ( fileName.isEmpty() )
		return;
	ui->lineEditGamerProfile->setText( fileName );
}

void Widget::on_XMLReceiveStringBasic( QString s)
{
	ui->textEdit->append(s);
}

void Widget::on_ListWidgetCurrItemChanged(QListWidgetItem * pNewItem)			// not necessary
{
	fileName = pNewItem->text();
	//ui->textEdit->append(pNewItem->text());
}

void Widget::on_XMLStringPointerReceiver( QString * pStr )
{
	ui->textEdit->append(*pStr);
}

//+------------------------------------------------------------------+
//| Install mods according to load order                             |
//| INPUT: modList, work dir                                         |
//| OUTPUT: none                                                     |
//| REMARK: none                                                     |
//+------------------------------------------------------------------+
void Widget::on_deployButtonClicked()
{
	qint32 i, j;

	if ( iProcState )
		return;
	if ( pProcess == NULL )
	{
		ui->textEdit->append( tr( "Cannot start new process.  Aborting." ) );
		return;
	}
	if ( !ui->listWidget->count() )
	{
		ui->textEdit->append( tr( "There are no mods to install." ) );
		return;
	}
	if ( TEST == 0 )
		if ( ui->lineEditGameDir->text().isEmpty() )
		{
			ui->textEdit->append( tr( "Game directory unknown.  Aborting." ) );
			return;
		}
	if ( TEST >= 1 )
	{
		ui->lineEditWorkDir->setText( "d:/_Vik_munka/Far Cry 3/FC3 Mod Manager/patch_unpack" );
		ui->textEdit->append( tr( "Work directory changed to " ) + ui->lineEditWorkDir->text() );
		ui->lineEditGameDir->setText( "e:/Games/Ubisoft Game Launcher/Far Cry 3/data_win32" );
		ui->textEdit->append( tr( "Game directory changed to " ) + ui->lineEditGameDir->text() );
	}
	if ( !dir.exists(ui->lineEditWorkDir->text()) )
		if ( !dir.mkdir( ui->lineEditWorkDir->text() ) )
		{
			msgBox.setText( tr("Cannot create work directory %1.").arg(ui->lineEditWorkDir->text()) );
			msgBox.setWindowTitle( tr("Error") );
			msgBox.exec();
			return;
		}
	if ( !dir.exists( ui->lineEditGameDir->text() ) )
	{
		ui->textEdit->append( tr( "Game directory does not exist.  Aborting." ) );
		return;
	}
	str = ui->lineEditWorkDir->text();i = str.lastIndexOf("/");
	str = str.remove( i, str.size() );
	fileName = str + "/patch_unpack.dat";
	fileCheck.setFileName(fileName);
	if ( fileCheck.exists() )
		fileCheck.remove();
	fileName = str + "/patch_unpack.fat";
	fileCheck.setFileName(fileName);
	if ( fileCheck.exists() )
		fileCheck.remove();
	BackupCheck(ui->lineEditGameDir->text() + "/patch.dat");
	BackupCheck(ui->lineEditGameDir->text() + "/patch.fat");
	fileName = appPath + "/7z.exe";
	if ( !QFile::exists( fileName ) )
	{
		ui->textEdit->append( tr( "%1 missing - cannot unpack mods.  Installation aborted." ).arg( fileName ) );
		return;
	}
	ui->textEdit->append("\nParsing current load order:");tempList.clear();
	dir.setCurrent(ui->lineEditWorkDir->text());
	for ( i = 0; i < ui->listWidget->count(); i++ )					// synchronize modList with listWidget
	{
		str = ui->listWidget->item(i)->text();
		for ( j = 0; j < modList.size(); j++ )						// search for this QString in modList
		{
			sShort = modList.at(j).mid( modList.at(j).lastIndexOf("/") + 1 );
			if ( sShort.contains(str) )		// modList.at(j) - at the end!
			{
				//ui->textEdit->append( "mod: " + str + ", path: " + modList.at(j));
				tempList << modList.at(j);
			}
		}
	}
	if ( modList.size() != tempList.size() )
	{
		ui->textEdit->append( tr("Error!  Size mismatch during preparation.  Installation aborted.") );
		return;
	}
	ui->progressBar->setValue(0);iProcState = 1;iModIndex = 0;
	modList.clear();modList = tempList;
	//for ( auto it = modList.begin(); it != modList.end(); it++ )		// STL-style iterator
		//ui->textEdit->append( *it );
	ProcessStartExtract();			// INPUT: fileName contains program to run!
}

//+------------------------------------------------------------------+
//| Load mod(s)                                                      |
//| INPUT: none                                                      |
//| OUTPUT: add it to load order list                                |
//| REMARK: none                                                     |
//+------------------------------------------------------------------+
void Widget::on_loadModButtonClicked()
{
	qint32 i, iList;
	bool bBreak;

	if ( iProcState )
		return;
	tempList.clear();
	tempList = QFileDialog::getOpenFileNames(this, tr( "Select mods to load"), "", "*.a3 *.a4 *.zip *.7z *.rar");
	if ( tempList.isEmpty() )
		return;
	for ( iList = 0; iList < tempList.size(); iList++ )
	{
		fileName = tempList.at(iList);bBreak = false;
		sShort = fileName.mid( fileName.lastIndexOf("/") + 1 );
		for ( i = 0; i < modList.size(); i++ )
			if ( modList.at(i).contains(sShort, Qt::CaseInsensitive) )
			{
				ui->textEdit->append( tr("The selected mod (%1) is already in the list.").arg(sShort) );bBreak = true;
				break;
			}
		if ( bBreak )
			continue;
		modList.append(fileName);bChangedLO = true;
		ui->listWidget->addItem(sShort);
	}
}

//+------------------------------------------------------------------+
//| Remove a mod                                                     |
//| INPUT: none                                                      |
//| OUTPUT: mod deleted from listWidget/modList                      |
//| REMARK: except the last (CTD)                                    |
//+------------------------------------------------------------------+	//https://forum.qt.io/topic/76049/qlistwidget-delete-selected-item
void Widget::on_removeModButtonClicked()
{
	QListWidgetItem * pItemRemoved;
	qint32 j;

	if ( iProcState )
		return;
	if ( !ui->listWidget->count() )
	{
		ui->textEdit->append( tr("Uhm... which one?") );
		return;
	}
	if ( ui->listWidget->currentItem() == NULL )
	{
		ui->textEdit->append( tr("Uhm... which one?") );
		return;
	}
	if ( ui->listWidget->count() == 1 )
	{
		//ui->textEdit->append( QString::number(ui->listWidget->currentRow() ));		// currentRow(): 0
		/*pItemRemoved = ui->listWidget->takeItem(0);		//clear(); - CTD
		if ( pItemRemoved == NULL )
		{
			ui->textEdit->append( tr( "List empty." ) );
			return;
		}
		delete pItemRemoved;modList.clear();	*/
		ui->textEdit->append( tr( "For some unknown reason deleting the last mod from the load order causes crash to desktop.  So... you can't do that.") );
		return;
	}
	pItemRemoved = ui->listWidget->takeItem( ui->listWidget->currentRow() );
	if ( pItemRemoved == NULL )
	{
		ui->textEdit->append( tr( "List empty." ) );
		return;
	}
	str = pItemRemoved->text();
	for ( j = 0; j < modList.size(); j++ )						// search for this QString in modList
	{
		sShort = modList.at(j).mid( modList.at(j).lastIndexOf("/") + 1 );
		if ( sShort.contains(str) )
		{
			modList.removeAt(j);delete pItemRemoved;
			ui->textEdit->append( tr("Mod: %1 has been removed from load order.").arg(str) );
			return;
		}
	}
	ui->textEdit->append( tr("Couldn't find mod in list which is supposed to be removed.  This shouldn't have happened.  List corrupted?") );
}

void Widget::on_readyReadSTD()
{
	baData = pProcess->readAllStandardOutput();
	ui->textEdit->insertPlainText( baData.data() );
	ui->textEdit->verticalScrollBar()->setValue(ui->textEdit->verticalScrollBar()->maximum());
}

//+------------------------------------------------------------------+
//| Process finished, should we continue?                            |
//| INPUT: none                                                      |
//| OUTPUT: message box in case sg went wrong                        |
//| REMARK: none                                                     |
//+------------------------------------------------------------------+
void Widget::on_processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
	float f1, f2;
	qint32 i;
	bool bDel;
	QString sTarget;

	//qDebug() << "on_processFinished(), iProcState == " << iProcState;
	str = tr( "Process exited with code %1" ).arg(exitCode);		//+ QString::number(exitCode)
	if ( exitStatus == QProcess::CrashExit )
		str += ".  Process crashed.";
	if ( exitCode != 0 )
	{
		ui->textEdit->append( str );
	}
	if ( exitCode != 0 )
	{
		msgBox.setText( tr("Do you wish to continue?") );
		msgBox.setDetailedText( tr("There has been a problem.  Process exited with error code %1").arg(exitCode) );
		msgBox.setWindowTitle( tr("Warning") );
		msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
		msgBox.setDefaultButton(QMessageBox::Cancel);
		int iRet = msgBox.exec();
		if ( iRet == QMessageBox::Cancel )
			return;
	}
	if ( fileName.endsWith("7z.exe", Qt::CaseInsensitive) )			// unpack phase
	{
		f2 = ui->listWidget->count();
		do
		{
			bDel = false;
			tempList.clear();
			tempList << "*.a3" << "*.a4" << "*.zip" << "*.7z" << "*.rar";
			tempList = dir.entryList(tempList, QDir::Files);
			if ( tempList.size() )
			{
				if ( ( iProcState & 0x10000 ) != 0 )			// level 1 uncompress finished
				{
					ui->textEdit->append( tr("Level 1 unpack finished.") );
					fileCheck.setFileName( tempList.at(0) );
					fileCheck.remove();bDel = true;
					iProcState &= ~0x10000;
				}
				else
				{
					if ( pProcess->state() == QProcess::Running )
					{
						//qDebug() << "unpack, level 1, process still running.";
						return;
					}
					ui->textEdit->append( tr("Found %1 file in work directory.  Level 1 unpack initiated.").arg( tempList.at(0) ) );
					if ( ( iProcState & 0x10000 ) == 0 )
						iProcState |= 0x10000;
					ProcessStartExtractLevel1( tempList.at(0) );
					return;
				}
			}
		} while ( bDel );
		if ( ++iModIndex < modList.size() )
		{
			f1 = iModIndex / f2 * 100;i = (int) f1;
			ui->progressBar->setValue(i);
			if ( pProcess->state() == QProcess::Running )
			{
				//qDebug() << "unpack, normal, process still running.";
				iModIndex--;
				return;
			}
			ProcessStartExtract();
		}
		else
		{
			ui->textEdit->append( tr( "Unpack phase finished." ) );
			bChangedLO = false;
		}
		f1 = ( iModIndex / f2 * 100) / 3;i = (int) f1;
		ui->progressBar->setValue(i);
	}	// if ( fileName.endsWith("7z.exe", Qt::CaseInsensitive) )			// unpack phase
	if ( ++iProcState == 2 )			// packaging phase
	{
		if ( pProcess->state() == QProcess::Running )
		{
			//qDebug() << "package, process still running.";
			--iProcState;
			return;
		}
		ui->textEdit->append( tr( "Packaging started...") );
		dir.setCurrent(ui->lineEditWorkDir->text());
		fileName = appPath + "/Dunia2/bin/Gibbed.Dunia2.Pack.exe";		// import.bat: Gibbed.Dunia2.Pack.exe "d:\_Vik_munka\Far Cry 3\FC3 Mod Manager\patch_unpack"
		fileCheck.setFileName(fileName);
		if ( !fileCheck.exists() )
		{
			ui->textEdit->append( tr( "Critical Error!  Can't find %1.  Packaging failed.").arg(fileCheck.fileName()) );
			return;
		}
		argList.clear();
		str = ui->lineEditWorkDir->text();	// + "/patch_unpack";		- oopsie
		str.replace('/', '\\');
		//qDebug() << fileName << str;
		argList << str;
		pProcess->start( fileName, argList );
	}
	if ( iProcState == 3 )
	{
		ui->textEdit->append( tr( "Packaging finished.") );ui->progressBar->setValue(66);bChangedLO = false;iProcState = 0;
		str = ui->lineEditWorkDir->text();i = str.lastIndexOf("/");
		str = str.remove( i, str.size() );
		fileName = str + "/patch_unpack.dat";						// move files from work_dir/.. to game_dir - prep
		fileCheck.setFileName(fileName);
		if ( !fileCheck.exists() )
		{
			ui->textEdit->append( tr("%1 missing.  Installation failed.").arg(fileCheck.fileName()) );
			return;
		}
		fileName = str + "/patch_unpack.fat";
		fileCheck.setFileName(fileName);
		if ( !fileCheck.exists() )
		{
			ui->textEdit->append( tr("%1 missing.  Installation failed.").arg(fileCheck.fileName()) );
			return;
		}
		sTarget = ui->lineEditGameDir->text() + "/patch.dat";
		targetFile.setFileName(sTarget);
		if ( targetFile.exists() )
			if ( !targetFile.remove() )
			{
				ui->textEdit->append( tr( "Cannot overwrite %1.  Installation failed.").arg(targetFile.fileName()));		//, fileCheck.fileName()
				return;
			}
		sTarget = ui->lineEditGameDir->text() + "/patch.fat";
		targetFile.setFileName(sTarget);
		if ( targetFile.exists() )
			if ( !targetFile.remove() )
			{
				ui->textEdit->append( tr( "Cannot overwrite %1.  Installation failed.").arg(targetFile.fileName()));		//, fileCheck.fileName()
				return;
			}
		fileName = str + "/patch_unpack.dat";						// move files from work_dir/.. to game_dir - execute
		fileCheck.setFileName(fileName);
		sTarget = ui->lineEditGameDir->text() + "/patch.dat";
		if ( fileCheck.copy(sTarget) )
		{
			ui->textEdit->append( tr( "Installation of %1 successfully completed.").arg(sTarget) );ui->progressBar->setValue(99);
			fileCheck.remove();
		}
		else ui->textEdit->append( tr( "Installation of %1 failed.").arg(fileCheck.fileName()) );
		fileName = str + "/patch_unpack.fat";
		fileCheck.setFileName(fileName);
		sTarget = ui->lineEditGameDir->text() + "/patch.fat";
		if ( fileCheck.copy(sTarget) )
		{
			ui->textEdit->append( tr( "Installation of %1 successfully completed.").arg(sTarget) );ui->progressBar->setValue(ui->progressBar->value() + 1);
			fileCheck.remove();
		}
		else ui->textEdit->append( tr( "Installation of %1 failed.").arg(fileCheck.fileName()) );
		dir.setPath(ui->lineEditWorkDir->text());
		if ( dir.removeRecursively() )
			ui->textEdit->append( tr("Work directory %1 deleted.").arg(dir.path()) );
		else ui->textEdit->append( tr("Work directory %1 emptied (more or less).").arg(dir.path()) );
	}
}

//+------------------------------------------------------------------+
//| Start extracting/loop through load order                         |
//| INPUT: fileName contains program to run                          |
//| OUTPUT: files extracted to work directory                        |
//| REMARK: none                                                     |
//+------------------------------------------------------------------+
void Widget::ProcessStartExtract()
{
	str = "-o" + ui->lineEditWorkDir->text();				// output directory
	argList.clear();
	argList << "x" << str << "-y" << modList.at(iModIndex);		// eXtract with full path, -y assume yes on all queries			iProcState
	//argList << "l" << modList.at(iModIndex);			// list contents of archive
	ui->textEdit->append( ui->listWidget->item(iModIndex)->text() );						//iProcState
	pProcess->start( fileName, argList );
}

//+------------------------------------------------------------------+
//| Start extracting level1 compressed file                          |
//| INPUT: fileName contains program to run                          |
//| OUTPUT: files extracted to work directory                        |
//| REMARK: delete level1 compressed file once finished              |
//+------------------------------------------------------------------+
void Widget::ProcessStartExtractLevel1(QString s)
{
	str = "-o" + ui->lineEditWorkDir->text();				// output directory
	argList.clear();
	argList << "x" << str << "-y" << s;		// eXtract with full path, -y assume yes on all queries
	//argList << "l" << modList.at(iModIndex);			// list contents of archive
	//ui->textEdit->append( ui->listWidget->item(iModIndex)->text() );
	pProcess->start( fileName, argList );
}

//+------------------------------------------------------------------+
//| Backup patch.?at files                                           |
//| INPUT: file to backup (full path)                                |
//| OUTPUT: <input_file>.bak                                         |
//| REMARK: none                                                     |
//+------------------------------------------------------------------+
void Widget::BackupCheck(QString sBackup)
{
	fileCheck.setFileName(sBackup);		//ui->lineEditGameDir->text() + "/patch.fat"
	if ( fileCheck.exists() )
	{
		targetFile.setFileName(sBackup + ".bak");		//ui->lineEditGameDir->text() + "/patch.fat.bak"
		if ( !targetFile.exists() )
			if ( fileCheck.copy(targetFile.fileName()) )
				ui->textEdit->append( tr( "Backup created: %1.").arg(targetFile.fileName()) );
	}
}

//+------------------------------------------------------------------+
//| Restore patch.?at files from backup                              |
//| INPUT: none                                                      |
//| OUTPUT: patch.dat, patch.fat                                     |
//| REMARK: none                                                     |
//+------------------------------------------------------------------+
void Widget::on_uninstallButtonClicked()
{
	if ( iProcState )
		return;
	if ( ui->lineEditGameDir->text().isEmpty() )
	{
		ui->textEdit->append( tr( "Game directory unknown.  Aborting." ) );
		return;
	}
	if ( !dir.exists(ui->lineEditGameDir->text()) )
	{
		ui->textEdit->append( tr( "Game directory does not exist.  Aborting." ) );
		return;
	}
	fileCheck.setFileName(ui->lineEditGameDir->text() + "/patch.fat.bak");
	if ( fileCheck.exists() )
	{
		targetFile.setFileName(ui->lineEditGameDir->text() + "/patch.fat");
		if ( targetFile.exists() )
		{
			if ( targetFile.remove() )
			{
				if ( fileCheck.copy(targetFile.fileName()) )
					ui->textEdit->append( tr( "Backup restored: %1.").arg(targetFile.fileName()) );
				else ui->textEdit->append( tr( "Cannot restore backup: %1.").arg(targetFile.fileName()) );
			}
			else ui->textEdit->append( tr( "Cannot overwrite %1!").arg(targetFile.fileName()) );
		}
		else if ( fileCheck.copy(targetFile.fileName()) )
				ui->textEdit->append( tr( "Backup restored: %1.").arg(targetFile.fileName()) );
			else ui->textEdit->append( tr( "Cannot restore backup: %1.").arg(targetFile.fileName()) );
	}
	else ui->textEdit->append( tr( "Backup file %1 does not exist!").arg(fileCheck.fileName()) );
	fileCheck.setFileName(ui->lineEditGameDir->text() + "/patch.dat.bak");
	if ( fileCheck.exists() )
	{
		targetFile.setFileName(ui->lineEditGameDir->text() + "/patch.dat");
		if ( targetFile.exists() )
		{
			if ( targetFile.remove() )
			{
				if ( fileCheck.copy(targetFile.fileName()) )
					ui->textEdit->append( tr( "Backup restored: %1.").arg(targetFile.fileName()) );
				else ui->textEdit->append( tr( "Cannot restore backup: %1.").arg(targetFile.fileName()) );
			}
			else ui->textEdit->append( tr( "Cannot overwrite %1!").arg(targetFile.fileName()) );
		}
		else if ( fileCheck.copy(targetFile.fileName()) )
				ui->textEdit->append( tr( "Backup restored: %1.").arg(targetFile.fileName()) );
			else ui->textEdit->append( tr( "Cannot restore backup: %1.").arg(targetFile.fileName()) );
	}
	else ui->textEdit->append( tr( "Backup file %1 does not exist!").arg(fileCheck.fileName()) );
}

//+------------------------------------------------------------------+
//| Apply game settings                                              |
//| INPUT: none                                                      |
//| OUTPUT: none                                                     |
//| REMARK: could be improved - not now                              |
//+------------------------------------------------------------------+
void Widget::on_applySettingsButtonClicked()
{
	qint64 fPos, fSize;
	bool bPatched, bCantFindMsg, bPatchSuccess1, bPatchSuccess2;

	bPatched = bCantFindMsg = bPatchSuccess1 = bPatchSuccess2 = false;
	//ui->textEdit->append("Moo");
	pBA = NULL;					// segmentation fault without it!
	if ( iProcState )
		return;
	//ui->textEdit->append("I said Moo");		// for senseless segmentation fault test
	if ( ui->checkBoxOffline->isChecked() )		// offline mode
	{
		if ( !gameBin.isEmpty() )
		{
			fileName = gameBin + "/fc3.dll";
			BackupCheck(fileName);
			fileCheck.setFileName(fileName);
			pBA = new QByteArray(fileCheck.size(), 0);
			if ( pBA == NULL )
				ui->textEdit->append( tr( "Null pointer error.  Cannot modify patch DLL files." ) );
			else
			{
				if ( fileCheck.open(QIODevice::ReadWrite) )
				{
					fSize = fileCheck.size();
					if ( ( fileCheck.read(pBA->data(), fSize) ) < fSize )
					{
						ui->textEdit->append( tr( "Error reading data from %1.").arg(fileCheck.fileName()) );
						fileCheck.close();
					}
					else
					{
						baDataFC3DLLFrom = QByteArray(QByteArrayLiteral("\xE8\x1C\xFE\x80\x00\x8A\xD8\x80\xE3\x01"));
						baDataFC3DLLTo = QByteArray(QByteArrayLiteral("\xB3\x00\x90\x90\x90\x90\x90\x90\x90\x90"));
						fPos = pBA->indexOf(QByteArrayView(baDataFC3DLLFrom));
						if ( fPos == -1 )
						{
							fPos = pBA->indexOf(QByteArrayView(baDataFC3DLLTo));
							if ( !bCantFindMsg )
							{
								ui->textEdit->append( tr( "Can't find byte sequence to be patched in %1.").arg(fileCheck.fileName()) );bCantFindMsg = true;
							}
							if ( fPos > -1 )
								bPatched = true;		//ui->textEdit->append("found byte sequence in ba at " + QString::number(fPos));
						}
						else
						{
							pBA->replace(baDataFC3DLLFrom, baDataFC3DLLTo);bPatchSuccess1 = true;
						}
						baDataFC3DLLFrom = QByteArray(QByteArrayLiteral("\x2D\x6F\x66\x66\x6C\x69\x6E\x65"));
						baDataFC3DLLTo = QByteArray(QByteArrayLiteral("\x00\x00\x00\x00\x00\x00\x00\x00"));
						fPos = pBA->indexOf(QByteArrayView(baDataFC3DLLFrom));
						if ( fPos == -1 )
						{
							fPos = pBA->indexOf(QByteArrayView(baDataFC3DLLTo));
							if ( !bCantFindMsg )
							{
								ui->textEdit->append( tr( "Can't find byte sequence to be patched in %1.").arg(fileCheck.fileName()) );//bCantFindMsg = true;
							}
							if ( fPos > -1 )
								bPatched = true;
						}
						else
						{
							pBA->replace(baDataFC3DLLFrom, baDataFC3DLLTo);bPatchSuccess1 = true;
						}
						if ( bPatched )
							ui->textEdit->append( tr("%1 has been patched before.\n").arg(fileCheck.fileName()) );
						if ( bPatchSuccess1 )
						{
							fileCheck.seek(0);
							if ( ( fileCheck.write(pBA->data(), fSize ) ) < fSize )
								ui->textEdit->append( tr( "Error writing to file %1.  File is probably corrupted.  You should restore it from backup.").arg(fileCheck.fileName()) );
							else ui->textEdit->append( tr("%1 has been patched successfully.").arg(fileCheck.fileName()) );
						}
						fileCheck.close();bCantFindMsg = false;
					}	//if ( ( fileCheck.read(pBA->data()
				}	//if ( fileCheck.open
				else ui->textEdit->append( tr( "Error.  Cannot open %1.").arg(fileCheck.fileName()) );
				fileName = gameBin + "/fc3_d3d11.dll";
				BackupCheck(fileName);
				fileCheck.setFileName(fileName);
				pBA->resize( fileCheck.size() );
				if ( fileCheck.open(QIODevice::ReadWrite) )
				{
					fSize = fileCheck.size();
					if ( ( fileCheck.read(pBA->data(), fSize) ) < fSize )
					{
						ui->textEdit->append( tr( "Error reading data from %1.").arg(fileCheck.fileName()) );
						fileCheck.close();
					}
					else
					{
						baDataFC3DLLFrom = QByteArray(QByteArrayLiteral("\xE8\x76\x03\x81\x00\x8A\xD8\x80\xE3\x01"));
						baDataFC3DLLTo = QByteArray(QByteArrayLiteral("\xB3\x00\x90\x90\x90\x90\x90\x90\x90\x90"));
						fPos = pBA->indexOf(QByteArrayView(baDataFC3DLLFrom));
						if ( fPos == -1 )
						{
							fPos = pBA->indexOf(QByteArrayView(baDataFC3DLLTo));
							if ( !bCantFindMsg )
							{
								ui->textEdit->append( tr( "Can't find byte sequence to be patched in %1.").arg(fileCheck.fileName()) );bCantFindMsg = true;
							}
							if ( fPos > -1 )
								bPatched = true;
						}
						else
						{
							pBA->replace(baDataFC3DLLFrom, baDataFC3DLLTo);bPatchSuccess2 = true;
						}
						baDataFC3DLLFrom = QByteArray(QByteArrayLiteral("\x2D\x6F\x66\x66\x6C\x69\x6E\x65"));
						baDataFC3DLLTo = QByteArray(QByteArrayLiteral("\x00\x00\x00\x00\x00\x00\x00\x00"));
						fPos = pBA->indexOf(QByteArrayView(baDataFC3DLLFrom));
						if ( fPos == -1 )
						{
							fPos = pBA->indexOf(QByteArrayView(baDataFC3DLLTo));
							if ( !bCantFindMsg )
							{
								ui->textEdit->append( tr( "Can't find byte sequence to be patched in %1.").arg(fileCheck.fileName()) );//bCantFindMsg = true;
							}
							if ( fPos > -1 )
								bPatched = true;
						}
						else
						{
							pBA->replace(baDataFC3DLLFrom, baDataFC3DLLTo);bPatchSuccess2 = true;
						}
						if ( bPatched )
							ui->textEdit->append( tr("%1 has been patched before.\n").arg(fileCheck.fileName()) );
						if ( bPatchSuccess2 )
						{
							fileCheck.seek(0);
							if ( ( fileCheck.write(pBA->data(), fSize ) ) < fSize )
								ui->textEdit->append( tr( "Error writing to file %1.  File is probably corrupted.  You should restore it from backup.").arg(fileCheck.fileName()) );
							else
							{
								if ( bPatchSuccess1 )
									ui->textEdit->append( tr("%1 has been patched successfully.  Offline mode enabled.").arg(fileCheck.fileName()) );
								else ui->textEdit->append( tr("%1 has been patched successfully.").arg(fileCheck.fileName()) );
							}
						}
						fileCheck.close();
					}	//if ( ( fileCheck.read
				}	//if ( fileCheck.open
				else ui->textEdit->append( tr( "Error.  Cannot open %1.").arg(fileCheck.fileName()) );
			}	//if ( pBA == NULL ), else
		}	//if ( !gameBin.isEmpty() )
		else ui->textEdit->append( tr( "Game directory unknown.  Cannot comply with offline mode request." ) );
	}	// if ( ui->checkBoxOffline->isChecked() )		// offline mode
	fileName = ui->lineEditGamerProfile->text();
	fileCheck.setFileName(fileName);
	if ( !fileCheck.exists() )
	{
		ui->textEdit->append( tr( "%1 does not exist.").arg(fileName) );
		if ( pBA != NULL )
			delete pBA;
		return;
	}
	if ( pXMLReader != NULL )					// Game settings in GamerProfile.xml
	{
		pXMLReader->ReadXML(pDOMDoc, fileName);
		QDomElement docElem = pDOMDoc->documentElement();		// The root element is available using documentElement().

		QDomNode docNode = docElem.firstChild();
		while ( !docNode.isNull() )
		{
			QDomElement docElement = docNode.toElement();      // try to convert the node to an element.
			if ( !docElement.isNull() )
			{
				QDomNamedNodeMap attribVar = docElement.attributes();
				for ( int i = 0; i < attribVar.count(); ++i)
				{
					//str = "attribute name: " + attribVar.item(i).nodeName() + ", attribute value: " + attribVar.item(i).nodeValue();
					if ( ui->checkBoxDX11->isChecked() )
					{
						if ( attribVar.item(i).nodeName() == "LockString" )
							if ( docElement.tagName() == "UplayProfile" )
								attribVar.item(i).setNodeValue("8yC48AvoM9/EPLarMLcyFZ+ojHCrcWqEmbb89G/v6qU=");			// possibly not needed - but it won't do any harm
						if ( attribVar.item(i).nodeName() == "DisableMip0Loading" )
							if ( docElement.tagName() == "RenderProfile" )
								attribVar.item(i).setNodeValue("1");
					}
					else
					{
						if ( attribVar.item(i).nodeName() == "LockString" )
							if ( docElement.tagName() == "UplayProfile" )
								attribVar.item(i).setNodeValue("");
						if ( !ui->checkBoxDisableMip0->isChecked() )
							if ( attribVar.item(i).nodeName() == "DisableMip0Loading" )
								if ( docElement.tagName() == "RenderProfile" )
									attribVar.item(i).setNodeValue("0");
					}
					if ( ui->checkBoxDisableMip0->isChecked() )
						if ( attribVar.item(i).nodeName() == "DisableMip0Loading" )
							if ( docElement.tagName() == "RenderProfile" )
								attribVar.item(i).setNodeValue("1");
					if ( !ui->checkBoxDisableMip0->isChecked() )
						if ( !ui->checkBoxDX11->isChecked() )
							if ( attribVar.item(i).nodeName() == "DisableMip0Loading" )
								if ( docElement.tagName() == "RenderProfile" )
									attribVar.item(i).setNodeValue("0");
					if ( attribVar.item(i).nodeName() == "HelpCrosshair" )
						if ( docElement.tagName() == "ProfileSpecificGameProfile" )
						{
							if ( ui->checkBoxCrosshair->isChecked() )
								attribVar.item(i).setNodeValue("1");
							else attribVar.item(i).setNodeValue("0");
						}
				}
				//str = "element name: " + docElement.tagName() + ", element text: " + docElement.text();
			}
			docNode = docNode.nextSibling();
		}	//while ( !docNode.isNull() )
		pXMLReader->WriteXML(pDOMDoc, fileName);
	}
	else ui->textEdit->append( tr( "XML reader failed.") );
	if ( pBA != NULL )
		delete pBA;
}


void Widget::on_checkBoxDX11Clicked()
{
	if ( ui->checkBoxDX11->isChecked() )
		ui->checkBoxDisableMip0->setChecked(true);
}

