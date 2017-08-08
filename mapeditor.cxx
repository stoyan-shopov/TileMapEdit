#include <QFileDialog>
#include <QMessageBox>

#include "mapeditor.hxx"
#include "ui_mapeditor.h"

MapEditor::MapEditor(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MapEditor)
{
	ui->setupUi(this);
	ui->dockWidgetTileSheetContents->layout()->addWidget(tileSheet = new TileSheet);
}

MapEditor::~MapEditor()
{
	delete ui;
}

void MapEditor::on_pushButtonOpenImage_clicked()
{
	auto s = QFileDialog::getOpenFileName(0, "select image to open");
	QImage image(s);
	if (image.isNull())
	{
		QMessageBox::warning(0, "error opening image", "Error opening image!");
		return;
	}
	tileSheet->setImage(image);
}
