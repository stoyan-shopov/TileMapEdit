#ifndef MAPEDITOR_HXX
#define MAPEDITOR_HXX

#include <QApplication>
#include <QMainWindow>
#include <QPainter>
#include <QClipboard>
#include <QMouseEvent>
#include <QDataStream>
#include <QJsonObject>

class TileInfo
{
private:
	static QStringList terrainTypeNames;
	/* this is an index in the 'terrainTypeNames' list above */
	int terrainType;
public:
	static QStringList & terrainNames(void) { return terrainTypeNames; }
	void read(const QJsonObject & json) { terrainType = json["terrain"].toInt(); }
	void write(QJsonObject & json) const { json["terrain"] = terrainType; }
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
		resize(w, h);
		setMinimumSize(w, h);
		QPainter p(this);
		p.drawImage(0, 0, image.scaled(w, h));
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
		resize(w, h);
		setMinimumSize(w, h);
		QPainter p(this);
		p.drawImage(0, 0, mapImage.scaled(w, h));
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
public slots:
	void setTileWidth(int width) { tile_width = width; resizeMap(); }
	void setTileHeight(int height) { tile_height = height; resizeMap(); }
	void setZoomFactor(int zoom_factor) { this->zoom_factor = zoom_factor; update(); }
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
public:
	int tileCountX(void) { return image.width() / tile_width; }
	int tileCountY(void) { return image.height() / tile_height; }
	TileSet(void) { setGridVerticalOrientation(false); setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding); }
	const QImage getImage(void) { return image; }
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
	void on_pushButtonAddTerrain_clicked();

private:
	Ui::MapEditor *ui;
	TileSheet tileSheet;
	TileMap tileMap;
	TileSet tileSet;
	QString last_map_image_filename;
	QVector<QVector<class TileInfo>> tile_info;
	void resetTileData(int tileCountX, int tileCountY) { int row; for (row = 0; row < tileCountY; row ++, tile_info << QVector<class TileInfo>(tileCountX)); }
protected:
	void closeEvent(QCloseEvent * event);
};

#endif // MAPEDITOR_HXX
