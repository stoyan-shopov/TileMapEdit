#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>

#include "mapeditor.hxx"
#include "ui_mapeditor.h"

MapEditor::MapEditor(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MapEditor)
{
	QCoreApplication::setOrganizationName("shopov instruments");
	QCoreApplication::setApplicationName("tile map editor");
	QSettings s("tile-edit.rc", QSettings::IniFormat);
	
	ui->setupUi(this);
	ui->scrollAreaTileSheet->setWidget(& tileSheet);
	ui->scrollAreaTileMap->setWidget(& tileMap);
	ui->scrollAreaTileSet->setWidget(& tileSet);
	
	restoreGeometry(s.value("window-geometry").toByteArray());
	restoreState(s.value("window-state").toByteArray());
	
	QImage tileset_image = QImage("tile-set.png");
	if (tileset_image.isNull())
		tileset_image= QImage(QSize(2000, 2000), QImage::Format_RGB32);
	tileSet.setImage(tileset_image);
	tileSheet.setImage(QImage(last_map_image_filename = s.value("last-map-image").toString()));

	connect(ui->spinBoxTileWidth, SIGNAL(valueChanged(int)), & tileSheet, SLOT(setTileWidth(int)));
	connect(ui->spinBoxTileHeight, SIGNAL(valueChanged(int)), & tileSheet, SLOT(setTileHeight(int)));
	connect(ui->spinBoxZoomLevel, SIGNAL(valueChanged(int)), & tileSheet, SLOT(setZoomFactor(int)));

	connect(ui->spinBoxTileWidth, SIGNAL(valueChanged(int)), & tileMap, SLOT(setTileWidth(int)));
	connect(ui->spinBoxTileHeight, SIGNAL(valueChanged(int)), & tileMap, SLOT(setTileHeight(int)));
	connect(ui->spinBoxZoomLevel, SIGNAL(valueChanged(int)), & tileMap, SLOT(setZoomFactor(int)));
	
	connect(ui->spinBoxTileWidth, SIGNAL(valueChanged(int)), & tileSet, SLOT(setTileWidth(int)));
	connect(ui->spinBoxTileHeight, SIGNAL(valueChanged(int)), & tileSet, SLOT(setTileHeight(int)));
	connect(ui->spinBoxZoomLevel, SIGNAL(valueChanged(int)), & tileSet, SLOT(setZoomFactor(int)));
	
	connect(ui->pushButtonClearMap, SIGNAL(pressed()), & tileMap, SLOT(clear()));
	connect(ui->checkBoxShowMapGrid, SIGNAL(clicked(bool)), & tileMap, SLOT(showGrid(bool)));
	
	ui->spinBoxTileWidth->setValue(s.value("tile-width", MINIMUM_TILE_SIZE).toInt());
	ui->spinBoxTileHeight->setValue(s.value("tile-height", MINIMUM_TILE_SIZE).toInt());
	ui->spinBoxZoomLevel->setValue(s.value("zoom-level", 1).toInt());
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
	last_map_image_filename = s;
	tileSheet.setImage(image);
}

void MapEditor::closeEvent(QCloseEvent *event)
{
	QSettings s("tile-edit.rc", QSettings::IniFormat);
	s.setValue("window-geometry", saveGeometry());
	s.setValue("window-state", saveState());
	s.setValue("tile-width", ui->spinBoxTileWidth->value());
	s.setValue("tile-height", ui->spinBoxTileHeight->value());
	s.setValue("zoom-level", ui->spinBoxZoomLevel->value());
	s.setValue("last-map-image", last_map_image_filename);
	tileSet.getImage().save("tile-set.png");
}
