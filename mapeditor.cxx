#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>

#include "mapeditor.hxx"
#include "ui_mapeditor.h"

QStringList TileInfo::terrainTypeNames;

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
	ui->splitterTileData->restoreState(s.value("splitter-tile-data").toByteArray());
	
	QImage tileset_image = QImage("tile-set.png");
	if (tileset_image.isNull())
		tileset_image= QImage(QSize(2000, 2000), QImage::Format_RGB32);
	tileSet.setImage(tileset_image);
	tileSheet.setImage(QImage(last_map_image_filename = s.value("last-map-image").toString()));

	connect(ui->spinBoxTileWidth, SIGNAL(valueChanged(int)), & tileSheet, SLOT(setTileWidth(int)));
	connect(ui->spinBoxTileHeight, SIGNAL(valueChanged(int)), & tileSheet, SLOT(setTileHeight(int)));
	connect(ui->spinBoxZoomLevel, SIGNAL(valueChanged(int)), & tileSheet, SLOT(setZoomFactor(int)));
	connect(ui->spinBoxHorizontalOffset, SIGNAL(valueChanged(int)), & tileSheet, SLOT(setHorizontalOffset(int)));

	connect(ui->spinBoxTileWidth, SIGNAL(valueChanged(int)), & tileMap, SLOT(setTileWidth(int)));
	connect(ui->spinBoxTileHeight, SIGNAL(valueChanged(int)), & tileMap, SLOT(setTileHeight(int)));
	connect(ui->spinBoxZoomLevel, SIGNAL(valueChanged(int)), & tileMap, SLOT(setZoomFactor(int)));
	
	connect(ui->spinBoxTileWidth, SIGNAL(valueChanged(int)), & tileSet, SLOT(setTileWidth(int)));
	connect(ui->spinBoxTileHeight, SIGNAL(valueChanged(int)), & tileSet, SLOT(setTileHeight(int)));
	connect(ui->spinBoxZoomLevel, SIGNAL(valueChanged(int)), & tileSet, SLOT(setZoomFactor(int)));
	
	connect(ui->pushButtonClearMap, SIGNAL(pressed()), & tileMap, SLOT(clear()));
	connect(ui->checkBoxShowMapGrid, SIGNAL(clicked(bool)), & tileMap, SLOT(showGrid(bool)));

	connect(& tileSet, SIGNAL(tileSelected(int,int)), this, SLOT(tileSelected(int,int)));
	
	ui->spinBoxTileWidth->setValue(s.value("tile-width", MINIMUM_TILE_SIZE).toInt());
	ui->spinBoxTileHeight->setValue(s.value("tile-height", MINIMUM_TILE_SIZE).toInt());
	ui->spinBoxHorizontalOffset->setValue(s.value("horizontal-offset", 0).toInt());
	ui->spinBoxZoomLevel->setValue(s.value("zoom-level", 1).toInt());

	QFile f("tile-info.json");
	f.open(QFile::ReadOnly);
	QJsonDocument jdoc = QJsonDocument::fromJson(f.readAll());
	if (jdoc.isNull())
		resetTileData(tileSet.tileCountX(), tileSet.tileCountY());
	else
	{
		int x, y, i = 0;
		resetTileData(x = jdoc.object()["tiles-x"].toInt(), y = jdoc.object()["tiles-y"].toInt());
		auto terrains = jdoc.object()["terrains"].toArray();
		for (auto t : terrains)
		{
			TileInfo::terrainNames() << t.toObject()["name"].toString();
			terrain_checkboxes << new QCheckBox(TileInfo::terrainNames().last(), this);
			ui->verticalLayoutTerrain->addWidget(terrain_checkboxes.last());
		}

		auto tiles = jdoc.object()["tiles"].toArray();
		for (auto t : tiles)
			tile_info[i / x][i % x].read(t.toObject()), ++ i;
	}
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
	s.setValue("horizontal-offset", ui->spinBoxHorizontalOffset->value());
	s.setValue("zoom-level", ui->spinBoxZoomLevel->value());
	s.setValue("last-map-image", last_map_image_filename);
	s.setValue("splitter-tile-data", ui->splitterTileData->saveState());
	tileSet.getImage().save("tile-set.png");
	QJsonArray tiles;
	int x, y;
	y = 0;
	for (auto row : tile_info)
	{
		x = 0;
		for (auto tile : row)
		{
			QJsonObject t;
			tile.write(t);
			t["x"] = x ++;
			t["y"] = y;
			tiles.append(t);
		}
		y ++;
	}
	QJsonArray terrains;
	for (auto t : TileInfo::terrainNames())
	{
		QJsonObject x;
		x["name"] = t;
		terrains.append(x);
	}
	QJsonObject t;
	t["tiles-x"] = x;
	t["tiles-y"] = y;
	t["terrains"] = terrains;
	t["tiles"] = tiles;
	QFile f("tile-info.json");
	f.open(QFile::WriteOnly);
	QJsonDocument jdoc(t);
	f.write(jdoc.toJson());
}

void MapEditor::on_pushButtonResetTileData_clicked()
{
	if (QMessageBox::question(0, "confirm reset of tile map data", "Please, confirm that you want to destroy the current tile data, and start from scratch!",
				  QMessageBox::Yes, QMessageBox::Cancel) == QMessageBox::Yes)
		resetTileData(tileSet.tileCountX(), tileSet.tileCountY());
}

void MapEditor::tileSelected(int tileX, int tileY)
{
	last_tile_selected = & tile_info[tileY][tileX];
	ui->labelTileX->setText(QString("%1").arg(tileX));
	ui->labelTileY->setText(QString("%1").arg(tileY));
	ui->lineEditTileName->setText(last_tile_selected->name());
	auto t = last_tile_selected->terrain(), i = 1;
	for (auto c : terrain_checkboxes)
		c->setChecked(t & i), i <<= 1;
}

void MapEditor::on_pushButtonAddTerrain_clicked()
{
auto & t = TileInfo::terrainNames();
	if (!t.contains(ui->lineEditNewTerrain->text()) && !ui->lineEditNewTerrain->text().isEmpty())
	{
		t << ui->lineEditNewTerrain->text();
		terrain_checkboxes << new QCheckBox(t.last(), this);
		ui->verticalLayoutTerrain->addWidget(terrain_checkboxes.last());
	}
	ui->lineEditNewTerrain->clear();
}

void MapEditor::on_lineEditNewTerrain_returnPressed()
{
	on_pushButtonAddTerrain_clicked();
}

void MapEditor::on_pushButtonRemoveTerrain_clicked()
{
auto & t = TileInfo::terrainNames();
auto i = t.indexOf(ui->lineEditNewTerrain->text());
	if (i == -1)
	{
		QMessageBox::information(0, "terrain not found", "terrain not found");
		return;
	}
	t.removeAt(i);
	delete terrain_checkboxes.at(i);
	terrain_checkboxes.removeAt(i);
	for (auto tiles : tile_info)
		for (auto tile : tiles)
			tile.removeTerrain(i);
}

void MapEditor::on_pushButtonUpdateTile_clicked()
{
qint64 t = 0, i = 0;
	if (!last_tile_selected)
		return;
	last_tile_selected->setName(ui->lineEditTileName->text());
	for (auto c : terrain_checkboxes)
		t |= (c->isChecked() ? (1 << i) : 0), ++ i;
	last_tile_selected->setTerrain(t);
}
