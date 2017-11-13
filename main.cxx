#include "mapeditor.hxx"
#include <QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	MapEditor w;
	QObject::connect(& a, SIGNAL(applicationStateChanged(Qt::ApplicationState)), & w, SLOT(applicationStateChanged(Qt::ApplicationState)));
	w.show();
	
	return a.exec();
}
