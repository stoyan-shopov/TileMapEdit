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
	
	connect(ui->spinBoxTileWidth, SIGNAL(valueChanged(int)), & tileMap, SLOT(setTileWidth(int)));
	connect(ui->spinBoxTileHeight, SIGNAL(valueChanged(int)), & tileMap, SLOT(setTileHeight(int)));
	connect(ui->spinBoxZoomLevel, SIGNAL(valueChanged(int)), & tileMap, SLOT(setZoomFactor(int)));
	
	connect(ui->pushButtonClearMap, SIGNAL(pressed()), & tileMap, SLOT(clear()));
	connect(ui->checkBoxShowMapGrid, SIGNAL(clicked(bool)), & tileMap, SLOT(showGrid(bool)));
}

MapEditor::~MapEditor()
{
	delete ui;
}

void MapEditor::on_pushButtonOpenImage_clicked()
{
	auto s = QFileDialog::getOpenFileName(0, tr("select image to open"));
	QImage image(s);
	if (image.isNull())
	{
		QMessageBox::warning(0, tr("error opening image"), tr("Error opening image!"));
		return;
	}
	tileSheet.setImage(image);
}
