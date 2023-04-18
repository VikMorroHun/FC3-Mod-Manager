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
//#include "ui_FC3MMwidget.h"		//https://forum.qt.io/topic/20570/qtcreator-ui-designer-suddenly-fails-to-add-find-slots/5
#include <XMLReader.h>
#include <QDebug>
#include <QObject>

/****************************************************************************************************
 * An Element is something that does or could have attributes of its own or contain other elements. *
 * An Attribute is something that is self-contained, i.e., a color, an ID, a name.                  * 
 * An element can contain: text, attributes, other elements, or a mix of the above.                 *
 * Metadata (data about data) should be stored as attributes, and the data itself should be stored  *
 * as elements.  https://www.w3schools.com/xml/xml_attributes.asp                                   *
 ****************************************************************************************************/

XMLReader::XMLReader(QDomDocument * document)
{
	if ( document == NULL )
	{
		str = "XMLReader constructor without document pointer.  Exiting.";
		//ui.textEdit->append( "XMLReader constructor without document pointer.  Exiting." );
		return;
	}
}
// QString QDomNode::nodeName() const
// QDomNode::NodeType QDomNode::nodeType() const
// QString QDomNode::nodeValue() const
// https://stackoverflow.com/questions/49188597/how-to-parse-an-unknown-xml-in-qt-and-get-all-the-attributes-in-it

XMLReader::~XMLReader()
{

}

void XMLReader::ReadXMLTest(QDomDocument * document, QString fileName)
{
	QFile file(fileName);
	QMessageBox msgBox;
	if ( !file.open( QIODevice::ReadOnly ) )
	{
		msgBox.setText( QObject::tr("Opening file %1 failed.").arg(fileName) );
		msgBox.exec();
		return;
	}
	if ( !document->setContent(&file) )
	{
		file.close();
		msgBox.setText( QObject::tr("Parsing XML file %1 failed.").arg(fileName) );
		msgBox.exec();
		return;
	}
	file.close();
	str = "XMLReader test - file opened and closed.  Element names:\n";
	//emit XMLSendStringBasic(str);
	emit XMLStringPointerSender(&str);

	// print out the element names of all elements that are direct children of the outermost element.
	QDomElement docElem = document->documentElement();		// The root element is available using documentElement().

	QDomNode docNode = docElem.firstChild();
	while ( !docNode.isNull() )
	{
		QDomElement docElement = docNode.toElement();      // try to convert the node to an element.
		if ( !docElement.isNull() )
		{
			QDomNamedNodeMap attribVar = docElement.attributes();
			for ( int i = 0; i < attribVar.count(); ++i)
			{
				//qDebug() << "Attributes found: " << attribVar.item(i).nodeName();
				str = "attribute name: " + attribVar.item(i).nodeName() + ", attribute value: " + attribVar.item(i).nodeValue();
				//emit XMLSendStringBasic(str);
				emit XMLStringPointerSender(&str);
			}
			str = "element name: " + docElement.tagName() + ", element text: " + docElement.text();
			//qDebug() << str;
			//emit XMLSendStringBasic(str);
			emit XMLStringPointerSender(&str);
		}
		docNode = docNode.nextSibling();
	}
}

void XMLReader::ReadXML(QDomDocument * document, QString fileName)
{
	QFile file(fileName);
	if ( !file.open( QIODevice::ReadOnly ) )
	{
		str = QObject::tr("Opening file %1 failed.").arg(fileName);
		emit XMLStringPointerSender(&str);
		return;
	}
	if ( !document->setContent(&file) )
	{
		file.close();
		str = QObject::tr("Parsing XML file %1 failed.").arg(fileName);
		emit XMLStringPointerSender(&str);
		return;
	}
	file.close();
}

void XMLReader::WriteXML(QDomDocument * document, QString fileName)
{
	qint64 qi64Written;		//fSize

	QFile file(fileName);
	if ( !file.open( QIODevice::WriteOnly ) )
	{
		str = QObject::tr("Opening file %1 failed.").arg(fileName);
		emit XMLStringPointerSender(&str);
		return;
	}
	//fSize = file.size();
	baData = document->toByteArray();
	qi64Written = file.write(baData.data(), baData.size() );	//fSize + 500 = 500?!
	if ( qi64Written > -1 )
	{
		str = QObject::tr( "%1 updated successfully." ).arg(file.fileName());
	}
	else
	{
		str = QObject::tr( "Error writing data to %1." ).arg(file.fileName());
	}
	emit XMLStringPointerSender(&str);
	file.close();
}
