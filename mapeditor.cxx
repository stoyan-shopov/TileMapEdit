#include "mapeditor.hxx"
#include "ui_mapeditor.h"

MapEditor::MapEditor(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MapEditor)
{
	ui->setupUi(this);
}

MapEditor::~MapEditor()
{
	delete ui;
}
