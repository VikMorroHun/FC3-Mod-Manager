#ifndef XMLREADER_H
#define XMLREADER_H

#include <QString>
#include <QDomDocument>
#include <QDomElement>
#include <QDomNode>
#include <QFileDialog>
#include <QFile>
#include <QMessageBox>

class XMLReader : public QObject
{
	Q_OBJECT
public:
	XMLReader(QDomDocument * document);
    ~XMLReader();
	void ReadXMLTest(QDomDocument * document, QString fileName);
	void ReadXML(QDomDocument * document, QString fileName);
	void WriteXML(QDomDocument * document, QString fileName);

private:
	QString str;
	QByteArray baData;

signals:
	void XMLSendStringBasic( QString str );
	void XMLStringPointerSender( QString *);
};


#endif // XMLREADER_H
