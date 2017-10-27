#ifndef MAPEDITOR_HXX
#define MAPEDITOR_HXX

#include <QApplication>
#include <QMainWindow>
#include <QPainter>
#include <QClipboard>
#include <QMouseEvent>
#include <QDataStream>
#include <QJsonObject>
#include <QCheckBox>
#include <QTimer>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QDebug>

#include <functional>

class TileInfo
{
private:
	static QStringList terrainTypeNames;
	/* this is a bitmap with elements in the 'terrainTypeNames' list above */
	qint32 terrainBitmap = 0;
	QString _name = "unassigned";
	int x = -1, y = -1;
public:
	static QStringList & terrainNames(void) { return terrainTypeNames; }
	void read(const QJsonObject & json)
	{
		terrainBitmap = json["terrain"].toInt(-1) & ((1 << terrainTypeNames.size()) - 1);
		_name = json["name"].toString("unassigned");
		x = json["x"].toInt(-1);
		y = json["y"].toInt(-1);
	}
	void write(QJsonObject & json) const
	{
		json["terrain"] = terrainBitmap;
		json["name"] = _name;
		json["x"] = x;
		json["y"] = y;
	}
	void setXY(int x, int y) { this->x = x, this->y = y; }
	int getX(void) { return x; }
	int getY(void) { return y; }
	void setName(const QString & name) { _name = name; }
	const QString & name(void) { return _name; }
	void setTerrain(int terrain) { terrainBitmap = terrain; }
	void removeTerrain(int index) { qint64 x = (1 << index) - 1; terrainBitmap = (terrainBitmap & x) | ((terrainBitmap >> 1) & ~ x); }
	qint32 terrain(void) const { return terrainBitmap; }
};

Q_DECLARE_METATYPE(TileInfo *)

class Tile : public QObject, public QGraphicsPixmapItem
{
	Q_OBJECT
	int x = -1, y = -1;
	TileInfo * tileInfo = 0;
public:
	Tile(const QPixmap &pixmap, QGraphicsItem *parent = Q_NULLPTR) : QGraphicsPixmapItem(pixmap, parent) {}
	Tile(QGraphicsItem *parent = Q_NULLPTR) : QObject(0), QGraphicsPixmapItem(parent) {}
	Tile(const Tile & tile) : QObject(0), QGraphicsPixmapItem(0)
	{
		setPixmap(tile.pixmap());
		setTileInfoPointer(tile.getTileInfo());
	}
	TileInfo * getTileInfo(void) const { return tileInfo; }
	void setTileInfoPointer(TileInfo * tileInfo) { this->tileInfo = tileInfo; }
	void setXY(int x, int y) { this->x = x, this->y = y; }
	int getX(void) { return x; }
	int getY(void) { return y; }
signals:
	void tileSelected(Tile * tile);
	void tileShiftSelected(Tile * tile);
protected:
	void mousePressEvent(QGraphicsSceneMouseEvent * event) { if (event->modifiers() & Qt::ShiftModifier) emit tileShiftSelected(this); else emit tileSelected(this); }
};

enum
{
	MINIMUM_TILE_SIZE	=	8,
	MINIMUM_MAP_SIZE	=	8,
};

class TileSheet : public QWidget
{
	Q_OBJECT
protected:
	QImage image;
	int tile_width = MINIMUM_TILE_SIZE, tile_height = MINIMUM_TILE_SIZE;
	int zoom_factor = 1;
	QRect tileRect(int x, int y) { return QRect(x * tile_width, y * tile_height, tile_width, tile_height); }
private:
	bool isBottomUpGrid = true;
	int horizontalOffset = 0;
protected:
	virtual void paintEvent(QPaintEvent * event)
	{
		int i, w, h;
		if (image.isNull())
			return;
		w = image.width() * zoom_factor;
		h = image.height() * zoom_factor;
		/*! \todo	kludge... redo this, maybe */
		if (w != width() || h != height())
		{
			qDebug() << "XXX";
			resize(w, h);
			setMinimumSize(w, h);
		}
		QPainter p(this);
		auto r = event->rect(), zr = QRect(r.x() / zoom_factor, r.y() / zoom_factor, r.width() / zoom_factor, r.height() / zoom_factor);
		p.drawImage(r, image, zr);
		p.setPen(Qt::green);
		for (i = horizontalOffset * zoom_factor; i < w; p.drawLine(i, 0, i, h), i += tile_width * zoom_factor);
		if (isBottomUpGrid)
			for (i = (image.height() - tile_height) * zoom_factor; i >= 0; p.drawLine(0, i, w, i), i -= tile_height * zoom_factor);
		else
			for (i = tile_height * zoom_factor; i < image.height() * zoom_factor; p.drawLine(0, i, w, i), i += tile_height * zoom_factor);
	}
	void setGridVerticalOrientation(bool isBottomUp) { isBottomUpGrid = isBottomUp; update(); }
	virtual void mousePressEvent(QMouseEvent *event)
	{
		int x, y;
		if (image.isNull())
			return;
		if ((x = event->x()) <= image.width() * zoom_factor && (y = event->y()) < image.height() * zoom_factor)
			QApplication::clipboard()->setImage(image.copy((x / (tile_width * zoom_factor)) * tile_width + horizontalOffset, (y / (tile_height * zoom_factor)) * tile_height, tile_width, tile_height));
	}
public:
	TileSheet(void) { setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding); }
	void setImage(const QImage & image) { this->image = image; update(); }
	int tileWidth(void) { return tile_width; }
	int tileHeight(void) { return tile_height; }
public slots:
	void setTileWidth(int width) { tile_width = width; update(); }
	void setTileHeight(int height) { tile_height = height; update(); }
	void setZoomFactor(int zoom_factor) { this->zoom_factor = zoom_factor; update(); }
	void setHorizontalOffset(int offset) { horizontalOffset = offset; update(); }
};

class TileMap : public QWidget
{
	Q_OBJECT
private:
	QImage mapImage;
	int tile_width = MINIMUM_TILE_SIZE, tile_height = MINIMUM_TILE_SIZE;
	int map_width = MINIMUM_MAP_SIZE, map_height = MINIMUM_MAP_SIZE;
	int zoom_factor = 1;
	bool isGridShown = true;
	void resizeMap(void)
	{
		mapImage = QImage(tile_width * map_width, tile_height * tile_height, QImage::Format_RGB16);
		int w = mapImage.width() * zoom_factor, h = mapImage.height() * zoom_factor;
		resize(w, h);
		setMinimumSize(w, h);
		update();
	}
protected:
	virtual void paintEvent(QPaintEvent * event)
	{
		int i, w, h;
		if (mapImage.isNull())
			return;
		w = mapImage.width() * zoom_factor;
		h = mapImage.height() * zoom_factor;
		QPainter p(this);
		auto r = event->rect(), zr = QRect(r.x() / zoom_factor, r.y() / zoom_factor, r.width() / zoom_factor, r.height() / zoom_factor);
		p.drawImage(r, mapImage, zr);
		p.setPen(Qt::green);
		if (isGridShown)
			for (i = 0; i < w; p.drawLine(i, 0, i, h), i += tile_width * zoom_factor);
		if (isGridShown)
			for (i = 0; i < h; p.drawLine(0, i, w, i), i += tile_height * zoom_factor);
	}
	virtual void mousePressEvent(QMouseEvent *event)
	{
		auto image = QApplication::clipboard()->image();
		if (!image.isNull() && image.width() == tile_width && image.height() == tile_height)
		{
			int x, y;
			if ((x = event->x()) <= mapImage.width() * zoom_factor && (y = event->y()) < mapImage.height() * zoom_factor)
			{
				QPainter p(& mapImage);
				p.drawImage((x / (tile_width * zoom_factor)) * tile_width, (y / (tile_height * zoom_factor)) * tile_height, image);
				update();
			}
		}
	}
public:
	TileMap(void)
	{
		setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		resizeMap();
		clear();
	}
	void injectImage(const QImage & image, int x, int y) { QPainter p(&mapImage); p.drawImage(x * tile_width, y * tile_height, image); update(); }
public slots:
	void setTileWidth(int width) { tile_width = width; resizeMap(); }
	void setTileHeight(int height) { tile_height = height; resizeMap(); }
	void setZoomFactor(int zoom_factor) { this->zoom_factor = zoom_factor; resizeMap(); }
	void clear(void) { mapImage.fill(Qt::black); update(); }
	void showGrid(bool show) { isGridShown = show; update(); }
};

class TileSet : public TileSheet
{
	Q_OBJECT
protected:
	virtual void mousePressEvent(QMouseEvent *event) override
	{
		int x = event->x(), y = event->y(), tx = (x / (tile_width * zoom_factor)), ty = (y / (tile_height * zoom_factor));
		if (event->modifiers() & Qt::ShiftModifier)
			emit tileShiftSelected(tx, ty);
		else
			emit tileSelected(tx, ty);
		if (event->button() == Qt::LeftButton)
			TileSheet::mousePressEvent(event);
		else
		{
			auto clipboardImage = QApplication::clipboard()->image();
			if (!clipboardImage.isNull() && clipboardImage.width() == tile_width && clipboardImage.height() == tile_height)
			{
				if ((x = event->x()) <= image.width() * zoom_factor && (y = event->y()) < image.height() * zoom_factor)
				{
					QPainter p(& image);
					p.drawImage(tx * tile_width, ty * tile_height, clipboardImage);
					update();
				}
			}
		}
	}
signals:
	void tileSelected(int x, int y);
	void tileShiftSelected(int x, int y);
public:
	int tileCountX(void) { return image.width() / tile_width; }
	int tileCountY(void) { return image.height() / tile_height; }
	TileSet(void) { setGridVerticalOrientation(false); setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding); }
	const QImage getImage(void) { return image; }
	QPixmap getTilePixmap(int x, int y) { return QPixmap::fromImage(image.copy(tileRect(x, y))); }
	QVector<QImage> reapTiles(std::function<bool(int, int)> predicate)
	{
		QVector<QImage> tiles;
		int rows = image.height() / tile_height, columns = image.width() / tile_width, x, y;
		for (y = 0; y < rows; y ++)
			for (x = 0; x < columns; x ++)
				if (predicate(x, y))
					tiles << image.copy(tileRect(x, y));
		return tiles;
	}
};

namespace Ui {
class MapEditor;
}

class MapEditor : public QMainWindow
{
	Q_OBJECT
	
public:
	explicit MapEditor(QWidget *parent = 0);
	~MapEditor();
	
private slots:
	void on_pushButtonOpenImage_clicked();
	void on_pushButtonResetTileData_clicked();
	void tileSelected(int tileX, int tileY);
	void mapTileSelected(Tile * tile);
	void tileSelected(Tile * tile);
	void tileShiftSelected(int tileX, int tileY);
	void tileShiftSelected(Tile * tile);
	void on_pushButtonAddTerrain_clicked();

	void on_lineEditNewTerrain_returnPressed();

	void on_pushButtonRemoveTerrain_clicked();

	void on_pushButtonUpdateTile_clicked();

	void on_pushButtonReapTilesExact_clicked() { displayFilteredTiles(true); }

	void on_pushButtonReapTilesAny_clicked() { displayFilteredTiles(false); }

	void on_pushButtonAnimate_clicked();

private:
	QVector<QCheckBox*> terrain_checkboxes;
	TileInfo	* last_tile_selected = 0;
	Tile		* lastTileFromMapSelected = 0;
	Ui::MapEditor *ui;
	TileSheet tileSheet;
	TileSet tileSet;
	QString last_map_image_filename;
	QVector<QVector<class TileInfo>> tile_info;
	void resetTileData(int tileCountX, int tileCountY)
	{ int row; for (row = 0; row < tileCountY; row ++) { tile_info << QVector<class TileInfo>(tileCountX); int column = 0; for (auto & t : tile_info.last()) t.setXY(column ++, row); } }
	qint64 terrainBitmap(void) { qint64 t = 0, i = 0; for (auto c : terrain_checkboxes) t |= (c->isChecked() ? (1 << i) : 0), ++ i; return t; }
	QVector<QImage> animation;
	int animation_index = 0;
	QGraphicsScene tileSetGraphicsScene, filteredTilesGraphicsScene, tileMapGraphicsScene;
	QVector<Tile *> graphicsSceneTiles;
	void displayFilteredTiles(bool exactTerrainMatch);
	QVector<QVector<Tile *>> tileMap;

protected:
	void closeEvent(QCloseEvent * event);
};

#endif // MAPEDITOR_HXX
