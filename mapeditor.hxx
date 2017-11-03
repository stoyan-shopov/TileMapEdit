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

class Util
{
public:
	static double rad(double angle) { return angle * 2 * M_PI / 360.; }
};

enum
{
	MAP_LAYERS = 4,
};

class TileInfo
{
private:
	static QStringList terrainTypeNames;
	/* this is a bitmap with elements in the 'terrainTypeNames' list above */
	qint32 terrainBitmap = 0;
	QString _name = "unassigned";
	int layer = 0;
	int x = -1, y = -1;
public:
	static QStringList & terrainNames(void) { return terrainTypeNames; }
	void read(const QJsonObject & json)
	{
		terrainBitmap = json["terrain"].toInt(-1) & ((1 << terrainTypeNames.size()) - 1);
		_name = json["name"].toString("unassigned");
		x = json["x"].toInt(-1);
		y = json["y"].toInt(-1);
		layer = json["layer"].toInt(0);
	}
	void write(QJsonObject & json) const
	{
		json["terrain"] = terrainBitmap;
		json["name"] = _name;
		json["x"] = x;
		json["y"] = y;
		json["layer"] = layer;
	}
	void setXY(int x, int y) { this->x = x, this->y = y; }
	int getLayer(void) { return layer; }
	void setLayer(int layer) { this->layer = layer; }
	int getX(void) { return x; }
	int getY(void) { return y; }
	void setName(const QString & name) { _name = name; }
	const QString & name(void) { return _name; }
	void setTerrain(int terrain) { terrainBitmap = terrain; }
	void removeTerrain(int index) { qint64 x = (1 << index) - 1; terrainBitmap = (terrainBitmap & x) | ((terrainBitmap >> 1) & ~ x); }
	qint32 terrain(void) const { return terrainBitmap; }
};

class Player : public QGraphicsPixmapItem
{
private:
	int rotation_angle = 0;
public:
	Player(QGraphicsItem * parent = 0) : QGraphicsPixmapItem(parent)
	{
		setPixmap(QPixmap("stalker-ship.png"));
		setTransformOriginPoint(boundingRect().center());
		qDebug() << transformOriginPoint();
	}
	void setRotation(int angle) { rotation_angle = angle % 360; QGraphicsPixmapItem::setRotation(- rotation_angle); }
	int getRotationAngle(void) { return rotation_angle; }
};

class Projectile : public QObject, public QGraphicsPixmapItem
{
	Q_OBJECT
	enum
	{
		MAX_PROJECTILE_DISTANCE	= 200,
	};
	QVector2D velocity;
	QPointF startPoint;
	QTimer timer;
public:
	Projectile(QGraphicsItem * parent, QString pixmapFileName, const QVector2D & velocity, const QPointF & startPoint) : QGraphicsPixmapItem(parent)
	{
		this->velocity = velocity;
		this->startPoint = startPoint;
		setPixmap(QPixmap(pixmapFileName));
		timer.setInterval(30);
		connect(&timer, SIGNAL(timeout()), this, SLOT(timeout()));
	}
	void start(void) { timer.start(); }
private slots:
	void timeout(void)
	{
		setPos(pos() + velocity.toPointF());
		if (QVector2D(startPoint - pos()).length() > MAX_PROJECTILE_DISTANCE)
		{
			setPixmap(QPixmap());
			timer.stop();
			emit projectileDeactivated(this);
		}
	}
signals:
	void projectileDeactivated(Projectile * p);
};

class Animation : public QObject, public QGraphicsPixmapItem
{
	Q_OBJECT
	QPixmap pixmap;
	QTimer timer;
	int frameWidth, frameIndex = 0;
	bool loop, playForwardAndBackward, isPlayingForward = true;
public:
	enum { Type = UserType + __COUNTER__ + 1, };
	int type(void){return Type;}
	Animation(QGraphicsItem * parent, QString pixmapFileName, int frameWidth, int framePeriod, bool loop = false, bool playForwardAndBackward = false) : QGraphicsPixmapItem(QPixmap(), parent)
	{
		pixmap = QPixmap(pixmapFileName);
		this->loop = loop;
		this->playForwardAndBackward = playForwardAndBackward;
		this->frameWidth = frameWidth;
		setTransformOriginPoint(boundingRect().center());
		connect(& timer, SIGNAL(timeout()), this, SLOT(timerElapsed()));
		setPixmap(pixmap.copy(0, 0, frameWidth, pixmap.height()));
		timer.setInterval(framePeriod);
	}
	void start(void)
	{
		if (pixmap.isNull())
		{
			emit animationFinished(this);
			return;
		}
		timer.start();
	}
private slots:
	void timerElapsed(void)
	{
		frameIndex += isPlayingForward ? 1 : -1;
		QRect r((frameIndex) * frameWidth, 0, frameWidth, pixmap.height());
		if (r.left() >= pixmap.width() || r.left() < 0)
		{
			if (loop)
			{
				if (!playForwardAndBackward)
					frameIndex = 0;
				else
				{
					if (isPlayingForward)
						frameIndex = pixmap.width() / frameWidth - 1;
					else
						frameIndex = 1;
					isPlayingForward = ! isPlayingForward;
				}
			}
			else
			{
				timer.stop();
				emit animationFinished(this);
			}
			return;
		}
		setPixmap(pixmap.copy(r));
	}

signals:
	void animationFinished(Animation * a);
};

class GameScene : public QGraphicsScene
{
private:
	Player * player = 0;
protected:
	void keyPressEvent(QKeyEvent *keyEvent) override
	{
		if (!player)
			return;
		QPointF pos = player->pos();
		switch (keyEvent->key()) {
		case Qt::Key_W: pos += QPoint(0, -1); break;
		case Qt::Key_A: pos += QPoint(-1, 0); break;
		case Qt::Key_S: pos += QPoint(0, 1); break;
		case Qt::Key_D: pos += QPoint(1, 0); break;
		case Qt::Key_Left: player->setRotation(player->getRotationAngle() + 1); break;
		case Qt::Key_Right: player->setRotation(player->getRotationAngle() - 1); break;
		case Qt::Key_Up: { auto angle = Util::rad(player->getRotationAngle()); pos += QPointF(cos(angle), sin(-angle)); break; }
		case Qt::Key_Down: { auto angle = Util::rad(player->getRotationAngle()); pos -= QPointF(cos(angle), sin(-angle)); break; }
		case Qt::Key_Space: { auto angle = Util::rad(player->getRotationAngle()); auto a = new Animation(0, "explosion-1.png", 24, 30, false);
			connect(a, & Animation::animationFinished, [=](Animation * a){ removeItem(a); delete a; });
			a->setPos(pos + QPointF(2 * 28 * cos(angle), 2 * 28 * sin(-angle)));
			addItem(a);
			a->start();
			Projectile * p = new Projectile(0, "projectile.png", QVector2D(2, 2), player->pos());
			connect(p, & Projectile::projectileDeactivated, [=](Projectile * a){ removeItem(a); delete a; });
			addItem(p);
			p->start();
		}
		default:
			break;
		}
		player->setPos(pos);
		qDebug() << "ship collides with " << player->collidingItems().size() << "items";
	}
public:
	void setPlayer(Player * player) { this->player = player; }
};

Q_DECLARE_METATYPE(TileInfo *)

class Tile : public QObject, public QGraphicsPixmapItem
{
	Q_OBJECT
	int x = -1, y = -1;
	TileInfo * tileInfo = 0;
	bool isCollisionEnabled = true;
public:
	enum { Type = UserType + __COUNTER__ + 1, };
	int type(void){return Type;}
	Tile(const QPixmap &pixmap, QGraphicsItem *parent = Q_NULLPTR) : QGraphicsPixmapItem(pixmap, parent) { setFlag(QGraphicsItem::ItemIsSelectable); }
	Tile(QGraphicsItem *parent = Q_NULLPTR) : QObject(0), QGraphicsPixmapItem(parent) { setFlag(QGraphicsItem::ItemIsSelectable); }
	Tile(const Tile & tile) : QObject(0), QGraphicsPixmapItem(0)
	{
		setPixmap(tile.pixmap());
		setTileInfoPointer(tile.getTileInfo());
		setFlag(QGraphicsItem::ItemIsSelectable);
	}
	TileInfo * getTileInfo(void) const { return tileInfo; }
	void setTileInfoPointer(TileInfo * tileInfo) { this->tileInfo = tileInfo; }
	void setXY(int x, int y) { this->x = x, this->y = y; }
	int getX(void) { return x; }
	int getY(void) { return y; }
	void setCollisionEnabled(bool f) { isCollisionEnabled = f; }
	QPainterPath shape(void) const override { QPainterPath p; if (isCollisionEnabled) p = QGraphicsPixmapItem::shape(); return p; }
signals:
	void tileSelected(Tile * tile);
	void tileShiftSelected(Tile * tile);
	void tileControlSelected(Tile * tile);
protected:
	void mousePressEvent(QGraphicsSceneMouseEvent * event)
	{ if (event->modifiers() & Qt::ShiftModifier) emit tileShiftSelected(this); else if (event->modifiers() & Qt::ControlModifier) emit tileControlSelected(this);

		else emit tileSelected(this); event->ignore(); }
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
	QRect tileRect(void) { return QRect(0, 0, tile_width, tile_height); }
	void setImage(const QImage & image) { this->image = image; update(); }
	int tileWidth(void) { return tile_width; }
	int tileHeight(void) { return tile_height; }
public slots:
	void setTileWidth(int width) { tile_width = width; update(); }
	void setTileHeight(int height) { tile_height = height; update(); }
	void setZoomFactor(int zoom_factor) { this->zoom_factor = zoom_factor; update(); }
	void setHorizontalOffset(int offset) { horizontalOffset = offset; update(); }
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

	void on_pushButtonMarkTerrain_clicked();

	void on_pushButtonFillMap_clicked();

private:
	void saveMap(const QString & fileName);
	bool loadMap(const QString & fileName);
	void clearMap(void);
	QVector<QCheckBox*> terrain_checkboxes;
	TileInfo	* last_tile_selected = 0;
	Tile		* lastTileFromMapSelected = 0;
	Ui::MapEditor *ui;
	TileSheet tileSheet;
	TileSet tileSet;
	QString last_map_image_filename;
	QVector<QVector<class TileInfo>> tileInfo;
	void resetTileData(int tileCountX, int tileCountY)
	{ int row; for (row = 0; row < tileCountY; row ++) { tileInfo << QVector<class TileInfo>(tileCountX); int column = 0; for (auto & t : tileInfo.last()) t.setXY(column ++, row); } }
	qint64 terrainBitmap(void) { qint64 t = 0, i = 0; for (auto c : terrain_checkboxes) t |= (c->isChecked() ? (1 << i) : 0), ++ i; return t; }
	QVector<QImage> animation;
	int animation_index = 0;
	QGraphicsScene tileSetGraphicsScene, filteredTilesGraphicsScene;
	GameScene tileMapGraphicsScene;
	QVector<Tile *> graphicsSceneTiles;
	void displayFilteredTiles(bool exactTerrainMatch);
	QVector<QVector<Tile *>> tileMap[MAP_LAYERS];
	QVector<QGraphicsEllipseItem *> tileMarks;
	Player * player;
protected:
	void closeEvent(QCloseEvent * event);
};

#endif // MAPEDITOR_HXX
