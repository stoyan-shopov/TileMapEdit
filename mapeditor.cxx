#include <QFileDialog>
#include <QMessageBox>

#include "mapeditor.hxx"
#include "ui_mapeditor.h"

MapEditor::MapEditor(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MapEditor)
{
	ui->setupUi(this);
	ui->scrollAreaTileSheet->setWidget(& tileSheet);
	ui->scrollAreaTileMap->setWidget(& tileMap);

	connect(ui->spinBoxTileWidth, SIGNAL(valueChanged(int)), & tileSheet, SLOT(setTileWidth(int)));
	connect(ui->spinBoxTileHeight, SIGNAL(valueChanged(int)), & tileSheet, SLOT(setTileHeight(int)));
	connect(ui->spinBoxZoomLevel, SIGNAL(valueChanged(int)), & tileSheet, SLOT(setZoomFactor(int)));
	connect(ui->spinBoxTileWidth, SIGNAL(valueChanged(int)), this, SLOT(repaint()));
	connect(ui->spinBoxTileHeight, SIGNAL(valueChanged(int)), this, SLOT(repaint()));
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
	tileSheet.setImage(image);
}
