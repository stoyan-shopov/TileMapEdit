#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonArray>
#include <QGradient>
#include <QScrollBar>
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

	restoreGeometry(s.value("window-geometry").toByteArray());
	restoreState(s.value("window-state").toByteArray());
	ui->splitterTileData->restoreState(s.value("splitter-tile-data").toByteArray());
	ui->splitterMain->restoreState(s.value("splitter-main").toByteArray());
	
	QImage tileset_image = QImage(":/tile-set.png");
	if (tileset_image.isNull())
		tileset_image= QImage(QSize(2000, 2000), QImage::Format_RGB32);
	tileSet.setImage(tileset_image);
	tileSheet.setImage(QImage(last_map_image_filename = s.value("last-map-image").toString()));
	
	connect(ui->spinBoxZoomLevel, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this] (int s) -> void { auto x = QTransform(); ui->graphicsViewTileSet->setTransform(x.scale(s, s)); });
	connect(ui->pushButtonClearMap, & QPushButton::clicked, [=]{clearMap();});

	connect(ui->spinBoxTileWidth, SIGNAL(valueChanged(int)), & tileSheet, SLOT(setTileWidth(int)));
	connect(ui->spinBoxTileHeight, SIGNAL(valueChanged(int)), & tileSheet, SLOT(setTileHeight(int)));
	connect(ui->spinBoxZoomLevel, SIGNAL(valueChanged(int)), & tileSheet, SLOT(setZoomFactor(int)));
	connect(ui->spinBoxHorizontalOffset, SIGNAL(valueChanged(int)), & tileSheet, SLOT(setHorizontalOffset(int)));

	connect(ui->spinBoxTileWidth, SIGNAL(valueChanged(int)), & tileSet, SLOT(setTileWidth(int)));
	connect(ui->spinBoxTileHeight, SIGNAL(valueChanged(int)), & tileSet, SLOT(setTileHeight(int)));
	connect(ui->spinBoxZoomLevel, SIGNAL(valueChanged(int)), & tileSet, SLOT(setZoomFactor(int)));
	
	connect(& tileSet, SIGNAL(tileSelected(int,int)), this, SLOT(tileSelected(int,int)));
	connect(& tileSet, SIGNAL(tileShiftSelected(int,int)), this, SLOT(tileShiftSelected(int,int)));

	ui->spinBoxTileWidth->setValue(s.value("tile-width", MINIMUM_TILE_SIZE).toInt());
	ui->spinBoxTileHeight->setValue(s.value("tile-height", MINIMUM_TILE_SIZE).toInt());
	ui->spinBoxHorizontalOffset->setValue(s.value("horizontal-offset", 0).toInt());
	ui->spinBoxZoomLevel->setValue(s.value("zoom-level", 1).toInt());
	ui->spinBoxMapWidth->setValue(s.value("map-width", 2).toInt());
	ui->spinBoxMapHeight->setValue(s.value("map-height", 2).toInt());

	connect(ui->spinBoxGlobalZoom, static_cast<void(QSpinBox::*)(int)>(& QSpinBox::valueChanged), this,
		[=] (int s){auto x = QTransform(); ui->graphicsViewTileSet->setTransform(x.scale(s, s)); ui->graphicsViewTileMap->setTransform(x); });
	ui->dockWidgetTileSet->setHidden(MINIMALISTIC_INTERFACE);
	ui->groupBoxMapControls->setHidden(MINIMALISTIC_INTERFACE);
	ui->groupBoxTileSetControls->setHidden(MINIMALISTIC_INTERFACE);
	if (MINIMALISTIC_INTERFACE)
	{
		ui->spinBoxTileWidth->setValue(24);
		ui->spinBoxTileHeight->setValue(28);
		ui->graphicsViewTileMap->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		ui->graphicsViewTileMap->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		//ui->spinBoxGlobalZoom->setValue(4);
	}

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
			ui->groupBoxTerrain->layout()->addWidget(terrain_checkboxes.last());
		}

		auto tiles = jdoc.object()["tiles"].toArray();
		for (auto t : tiles)
			tileInfo[i / x][i % x].read(t.toObject()), ++ i;
	}
	auto tx = tileSet.tileCountX(), ty = tileSet.tileCountY(), w = tileSet.tileWidth(), h = tileSet.tileHeight();
	for (auto y = 0; y < ty; y ++)
		for (auto x = 0; x < tx; x ++)
		{
			Tile * tile = new Tile(tileSet.getTilePixmap(x, y));
			graphicsSceneTiles << tile;
			tile->setXY(x, y);
			tile->setTileInfoPointer(& tileInfo[y][x]);
			connect(tile, SIGNAL(tileSelected(Tile*)), this, SLOT(tileSelected(Tile*)));
			connect(tile, SIGNAL(tileShiftSelected(Tile*)), this, SLOT(tileShiftSelected(Tile*)));
			tileSetGraphicsScene.addItem(tile);
			tile->setPos(x * w, y * h);
		}
	auto pen = QPen(Qt::green);
	for (auto y = 0; y < ty; y ++)
	{
		auto l = new QGraphicsLineItem(0, y * h, tx * w - 1, y * h);
		l->setPen(pen);
		tileSetGraphicsScene.addItem(l);
	}
	for (auto x = 0; x < tx; x ++)
	{
		auto l = new QGraphicsLineItem(x * w, 0, x * w, ty * h - 1);
		l->setPen(pen);
		tileSetGraphicsScene.addItem(l);
	}
	
	ui->graphicsViewTileSet->setScene(& tileSetGraphicsScene);
	ui->graphicsViewFilteredTiles->setScene(& filteredTilesGraphicsScene);

	if (!loadMap("map.json"))
		clearMap();
	ui->graphicsViewTileMap->setScene(& tileMapGraphicsScene);
	connect(ui->spinBoxRotateMap, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), [=](int angle){auto r = QTransform(); ui->graphicsViewTileMap->setTransform(r.rotate(-angle));});

	ui->graphicsViewTileSet->setInteractive(true);
	ui->graphicsViewTileSet->setDragMode(QGraphicsView::RubberBandDrag);

	connect(& tileSetGraphicsScene, & QGraphicsScene::selectionChanged, [=] { qDebug() << "tiles selected:" << tileSetGraphicsScene.selectedItems().size(); });

	tileMapGraphicsScene.setPlayer(player = new Player());
	player->setPos(100, 100);
	tileMapGraphicsScene.addItem(player);
	connect(& tileMapGraphicsScene, & GameScene::playerObjectPositionChanged, [=]{ui->graphicsViewTileMap->ensureVisible(player);});

	connect(ui->graphicsViewTileMap->horizontalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(gameSceneViewportMoved()));
	connect(ui->graphicsViewTileMap->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(gameSceneViewportMoved()));

	Animation * a;
	tileMapGraphicsScene.addItem(a = new Animation(0, ":/red-gemstone.png", 12, 30, true, true));
	connect(a, & Animation::animationFinished, [=](Animation * a){ tileMapGraphicsScene.removeItem(a); delete a; });
	a->setPos(240, 280);
	a->start();

	upArrowOverlayButton = new Tile(QPixmap(":/arrow-up.png"));
	upArrowOverlayButton->setOpacity(.2);
	tileMapGraphicsScene.addItem(upArrowOverlayButton);
	connect(upArrowOverlayButton, & Tile::tileSelected, [=]{qDebug() << "forward"; tileMapGraphicsScene.forwardPressed(); });
	connect(upArrowOverlayButton, & Tile::tileReleased, [=]{qDebug() << "forward released"; tileMapGraphicsScene.forwardReleased(); });
}

MapEditor::~MapEditor()
{
	/* for some reason, the 'gameSceneViewportMoved()' slot gets invoked while destruction of the user interface object is in progress,
	 * and that causes a segmentation violation */
	ui->graphicsViewTileMap->horizontalScrollBar()->disconnect();
	ui->graphicsViewTileMap->verticalScrollBar()->disconnect();
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
	s.setValue("map-width", ui->spinBoxMapWidth->value());
	s.setValue("map-height", ui->spinBoxMapHeight->value());
	s.setValue("horizontal-offset", ui->spinBoxHorizontalOffset->value());
	s.setValue("zoom-level", ui->spinBoxZoomLevel->value());
	s.setValue("last-map-image", last_map_image_filename);
	s.setValue("splitter-tile-data", ui->splitterTileData->saveState());
	s.setValue("splitter-main", ui->splitterMain->saveState());
	tileSet.getImage().save("tile-set.png");
	QJsonArray tiles;
	int x, y;
	y = 0;
	for (auto row : tileInfo)
	{
		x = 0;
		for (auto tile : row)
		{
			QJsonObject t;
			tile.write(t);
			++ x;
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

	saveMap("map.json");
}

void MapEditor::on_pushButtonResetTileData_clicked()
{
	if (QMessageBox::question(0, "confirm reset of tile map data", "Please, confirm that you want to destroy the current tile data, and start from scratch!",
				  QMessageBox::Yes, QMessageBox::Cancel) == QMessageBox::Yes)
		resetTileData(tileSet.tileCountX(), tileSet.tileCountY());
}

void MapEditor::tileSelected(int tileX, int tileY)
{
	last_tile_selected = & tileInfo[tileY][tileX];
	ui->labelTileX->setText(QString("%1").arg(tileX));
	ui->labelTileY->setText(QString("%1").arg(tileY));
	ui->lineEditTileName->setText(last_tile_selected->name());
	ui->spinBoxTerrainLayer->setValue(tileInfo[tileY][tileX].getLayer());
	if (!ui->checkBoxLockTerrain->isChecked())
	{
	auto t = last_tile_selected->terrain(), i = 1;
		for (auto c : terrain_checkboxes)
			c->setChecked(t & i), i <<= 1;
	}
}

void MapEditor::mapTileSelected(Tile *tile)
{
	auto tiles = tileSetGraphicsScene.selectedItems();
	qDebug() << tiles.size();
	std::sort(tiles.begin(), tiles.end(), [](QGraphicsItem * & a, QGraphicsItem * & b)->bool { Tile * a1 = dynamic_cast<Tile*>(a), * b1 = dynamic_cast<Tile*>(b); return (a1->getY() << 16) + a1->getX() < (b1->getY() << 16) + b1->getX();});
	if (tiles.isEmpty() && lastTileFromMapSelected)
	{
		tile = tileMap[lastTileFromMapSelected->getTileInfo()->getLayer()][tile->getY()][tile->getX()];
		tile->setPixmap(lastTileFromMapSelected->pixmap());
		tile->setTileInfoPointer(lastTileFromMapSelected->getTileInfo());
	}
	else if (!tiles.isEmpty())
	{
		int row = dynamic_cast<Tile*>(tiles.at(0))->getY(), mapy = tile->getY(), mapx = tile->getX(), map_start_x = mapx;
		for (auto g : tiles)
		{
			Tile * t = dynamic_cast<Tile *>(g);
			if (t->getY() != row)
				row = t->getY(), mapy ++, mapx = map_start_x;
			if (mapy >= tileMap[0].size() || mapx >= tileMap[0][mapy].size())
				continue;
			auto map_tile = tileMap[t->getTileInfo()->getLayer()][mapy][mapx ++];
			map_tile->setPixmap(t->pixmap());
			map_tile->setTileInfoPointer(t->getTileInfo());
		}
	}
}

void MapEditor::tileSelected(Tile *tile)
{
	lastTileFromMapSelected = tile;
	QApplication::clipboard()->setImage(tile->pixmap().toImage());
	tileSelected(tile->getTileInfo()->getX(), tile->getTileInfo()->getY());
}

void MapEditor::tileShiftSelected(int tileX, int tileY)
{
	tileInfo[tileY][tileX].setTerrain(terrainBitmap());
	tileInfo[tileY][tileX].setLayer(ui->spinBoxTerrainLayer->value());
}

void MapEditor::tileShiftSelected(Tile *tile)
{
	tileShiftSelected(tile->getX(), tile->getY());
}

void MapEditor::gameSceneViewportMoved()
{
	qDebug() << "game viewport movement" << ui->graphicsViewTileMap->viewport()->rect() << ui->graphicsViewTileMap->mapToScene(0, 0);
	//upArrowOverlayButton->setPos(ui->graphicsViewTileMap->mapToScene(0, 0));
	upArrowOverlayButton->setPos(ui->graphicsViewTileMap->mapToScene(
					     ui->graphicsViewTileMap->viewport()->rect().bottomLeft() - upArrowOverlayButton->pixmap().rect().bottomLeft())
				     );
}

void MapEditor::on_pushButtonAddTerrain_clicked()
{
auto & t = TileInfo::terrainNames();
	if (!t.contains(ui->lineEditNewTerrain->text()) && !ui->lineEditNewTerrain->text().isEmpty())
	{
		t << ui->lineEditNewTerrain->text();
		terrain_checkboxes << new QCheckBox(t.last(), this);
		ui->groupBoxTerrain->layout()->addWidget(terrain_checkboxes.last());
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
	for (auto & tiles : tileInfo)
		for (auto & tile : tiles)
			tile.removeTerrain(i);
}

void MapEditor::on_pushButtonUpdateTile_clicked()
{
	if (!last_tile_selected)
		return;
	last_tile_selected->setName(ui->lineEditTileName->text());
	last_tile_selected->setTerrain(terrainBitmap());
}

void MapEditor::on_pushButtonAnimate_clicked()
{
	animation = tileSet.reapTiles([=] (int x, int y) -> bool { return tileInfo[y][x].terrain() == terrainBitmap(); });
	animation_index = 0;
}

void MapEditor::displayFilteredTiles(bool exactTerrainMatch)
{
	auto terrain = terrainBitmap();
	auto last_row = -1;
	QVector<QVector<Tile *>> tiles;
	for (auto tile : graphicsSceneTiles)
	{
		TileInfo * t = tile->getTileInfo();
		if (!t)
			continue;
		if ((exactTerrainMatch && t->terrain() == terrain)
				|| (!exactTerrainMatch && t->terrain() & terrain))
		{
			if (t->getY() != last_row)
				last_row = t->getY(), tiles << QVector<Tile *>();
			tiles.last() << tile;
		}
	}
	filteredTilesGraphicsScene.clear();
	int x = 0, y = 0, maxx = 0, maxy;
	for (auto tileRow : tiles)
	{
		x = 0;
		for (auto tile : tileRow)
		{
			auto t = new Tile(* tile);
			filteredTilesGraphicsScene.addItem(t);
			t->setPos(x ++ * tileSet.tileWidth(), y * tileSet.tileHeight());
			connect(t, SIGNAL(tileSelected(Tile*)), this, SLOT(tileSelected(Tile*)));
		}
		maxx = std::max(maxx, x);
		y ++;
	}
	maxy = y;
	for (x = 0; x < maxx; x++)
		filteredTilesGraphicsScene.addLine(x * tileSet.tileWidth(), 0, x * tileSet.tileWidth(), maxy * tileSet.tileHeight(), QPen(Qt::cyan));
	for (y = 0; y < maxy; y++)
		filteredTilesGraphicsScene.addLine(0, y * tileSet.tileHeight(), maxx * tileSet.tileWidth(), y * tileSet.tileHeight(), QPen(Qt::cyan));
}

void MapEditor::on_pushButtonMarkTerrain_clicked()
{
auto t = terrainBitmap();

	for (auto m : tileMarks)
		tileSetGraphicsScene.removeItem(m);
	tileMarks.clear();
	for (auto tile : graphicsSceneTiles)
	{
		if (tileInfo.at(tile->getY()).at(tile->getX()).terrain() & t)
		{
			auto mark = new QGraphicsEllipseItem(tileSet.tileRect());
			mark->setPos(tile->pos());
			mark->setPen(QPen(Qt::cyan));
			tileMarks << mark;
			tileSetGraphicsScene.addItem(mark);
		}
	}
}

void MapEditor::saveMap(const QString &fileName)
{
	QJsonArray map_layers;
	for (auto layer : tileMap)
	{
		int x, y;
		QJsonArray layer_tiles;
		y = 0;
		for (auto row : layer)
		{
			x = 0;
			for (auto tile : row)
			{
				QJsonObject t;
				t["x"] = x;
				t["y"] = y;
				if (tile->getTileInfo())
				{
					t["tile-set-x"] = tile->getTileInfo()->getX();
					t["tile-set-y"] = tile->getTileInfo()->getY();
				}
				else
				{
					t["tile-set-x"] = -1;
					t["tile-set-y"] = -1;
				}
				++ x;
				layer_tiles.append(t);
			}
			y ++;
		}
		map_layers.append(layer_tiles);
	}

	QJsonObject t;
	t["map-size-x"] = tileMap[0].at(0).size();
	t["map-size-y"] = tileMap[0].size();
	t["layers"] = map_layers;
	QFile f(fileName);
	f.open(QFile::WriteOnly);
	QJsonDocument jdoc(t);
	f.write(jdoc.toJson());
}

bool MapEditor::loadMap(const QString &fileName)
{
	QFile f(fileName);
	if (!f.open(QFile::ReadOnly))
		return false;

	QJsonDocument jdoc = QJsonDocument::fromJson(f.readAll());
	if (jdoc.isNull())
		return false;
	else
	{
		QLinearGradient gradient(QPointF(0, 0), QPointF(tileSet.tileWidth(), tileSet.tileHeight()));
		gradient.setColorAt(0, Qt::black);
		gradient.setColorAt(1, Qt::white);

		tileMapGraphicsScene.clear();
		auto obj = jdoc.object();
		int columns = obj["map-size-x"].toInt(-1), rows = obj["map-size-y"].toInt(-1), x, y;
		QJsonArray map_layers = obj["layers"].toArray();
		for (int layer = 0; layer < MAP_LAYERS; layer ++)
		{
			tileMap[layer].clear();
			auto map_layer = map_layers.at(layer).toArray();
			for (y = 0; y < rows; y ++)
			{
				tileMap[layer] << QVector<Tile *>();
				for (x = 0; x < columns; x ++)
				{
					int tx = map_layer.at(y * columns + x).toObject()["tile-set-x"].toInt(-1), ty = map_layer.at(y * columns + x).toObject()["tile-set-y"].toInt(-1);
					QPixmap px(tileSet.tileRect().size());
					QPainter p(&px);
					p.setPen(Qt::magenta);
					p.drawLine(0, 0, 28, 24);

					if ((tx == -1 || ty == -1) && layer)
					{
						p.end();
						px = QPixmap();
					}
					else
						p.fillRect(px.rect(), QBrush(gradient));
					if (tx != -1 && ty != -1)
						p.drawPixmap(0, 0, tileSet.getTilePixmap(tx, ty));
					Tile * t = new Tile(px);

					t->setXY(x, y);
					t->setPos(x * tileSet.tileWidth(), y * tileSet.tileHeight());
					t->setZValue(layer);
					if (tx != -1 && ty != -1)
						t->setTileInfoPointer(& tileInfo[ty][tx]);
					tileMapGraphicsScene.addItem(t);
					connect(t, SIGNAL(tileSelected(Tile*)), this, SLOT(mapTileSelected(Tile*)));
					connect(t, & Tile::tileControlSelected, [=] (Tile * tile) { for (auto i = 1; i < MAP_LAYERS; i ++) tileMap[i][tile->getY()][tile->getX()]->setPixmap(QPixmap()); });
					tileMap[layer].last() << t;
				}
			}
		}
	}
	return true;
}

void MapEditor::clearMap()
{
	/* create map layers */
	auto rows = ui->spinBoxMapHeight->value(), columns = ui->spinBoxMapWidth->value();

	QLinearGradient gradient(QPointF(0, 0), QPointF(tileSet.tileWidth(), tileSet.tileHeight()));
	gradient.setColorAt(0, Qt::black);
	gradient.setColorAt(1, Qt::white);

	tileMapGraphicsScene.clear();

	for (int layer = 0; layer < MAP_LAYERS; layer ++)
	{
		tileMap[layer].clear();
		for (auto y = 0; y < rows; y++)
		{
			QVector<Tile *> v;
			for (auto x = 0; x < columns; x++)
			{
				QPixmap px;
				if (!layer)
				{
					px = QPixmap(tileSet.tileRect().size());
					QPainter p(&px);
					p.fillRect(px.rect(), QBrush(gradient));
				}
				Tile * t = new Tile(px);

				t->setXY(x, y);
				t->setPos(x * tileSet.tileWidth(), y * tileSet.tileHeight());
				tileMapGraphicsScene.addItem(t);
				connect(t, SIGNAL(tileSelected(Tile*)), this, SLOT(mapTileSelected(Tile*)));
				connect(t, & Tile::tileControlSelected, [=] (Tile * tile) { for (auto i = 1; i < MAP_LAYERS; i ++) tileMap[i][tile->getY()][tile->getX()]->setPixmap(QPixmap()); });
				v << t;
			}
			tileMap[layer] << v;
		}
	}
}

void MapEditor::on_pushButtonFillMap_clicked()
{
	clearMap();
	if (last_tile_selected)
	{
		auto px = tileSet.getTilePixmap(last_tile_selected->getX(), last_tile_selected->getY());
		for (auto row : tileMap[0])
			for (auto tile : row)
			{
				tile->setTileInfoPointer(last_tile_selected);
				tile->setPixmap(px);
			}
	}
}
