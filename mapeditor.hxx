#ifndef MAPEDITOR_HXX
#define MAPEDITOR_HXX

#include <QApplication>
#include <QMainWindow>
#include <QPainter>
#include <QClipboard>
#include <QMouseEvent>
#include <QDataStream>
#include <QJsonObject>
#include <QJsonArray>
#include <QCheckBox>
#include <QTimer>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QScrollBar>
#include <QMetaMethod>
#include <QDebug>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsBlurEffect>

#include <functional>
#include <math.h>

enum Z_AXIS_ORDER_ENUM
{
	PLAYER_Z_VALUE		= 5,
	NPC_Z_VALUE		= 5,
	EXPLOSIONS_Z_VALUE	= 6,
};

class Util
{
public:
	static double rad(double angle) { return angle * 2 * M_PI / 360.; }
	static double degrees(double angle) { return angle * 360. / (2 * M_PI); }
	static int bound(int low, int value, int high) { if (low > high) std::swap(low, high); return std::min(std::max(low, value), high); }
	static double bound(double low, double value, double high) { if (low > high) std::swap(low, high); return std::min(std::max(low, value), high); }
	/*! \todo	this is buggy */
	static double angleForVector(const QVector2D & v)
	{
		auto a = (v.x() != .0) ? atan(v.y() / v.x()) : /* not exactly correct, but good enough for this engine */
					 (v.y() >= .0 ? M_PI_2 : 3. * M_PI_2);
		if (v.x() < .0)
			a += M_PI;
		return a;
	}
	static double angleForVector(const QPointF & p) { return angleForVector(QVector2D(p.x(), p.y())); }
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


class JoypadFire : public QObject, public QGraphicsEllipseItem
{
	Q_OBJECT
protected:
private:
	bool sceneEvent(QEvent *event) override
	{
		bool result = false;
		switch (event->type())
		{
			case QEvent::TouchUpdate:
			case QEvent::TouchBegin:
				result =  true;
		{
			QTouchEvent * e = static_cast<QTouchEvent *>(event);
			QPointF p;
			for (auto t : e->touchPoints())
			{
				if (contains(t.pos()))
					p = t.pos();
			}
			if (p.isNull())
				break;
			/* check joypad zones */
			auto angle = fmod(360. - Util::degrees(Util::angleForVector(p - boundingRect().center())), 360.);
			if (angle < 180.)
				emit pressed(FIRE_UP);
			else
				emit pressed(FIRE_DOWN);
		}
				break;
			case QEvent::TouchEnd:
			emit released();
			result = true;
		}
		return result;
	}
	int x, y, r;
	int zoomLevel = 1;
	QGraphicsView * view;
public slots:
	void adjustPosition(void)
	{

	QRectF visible_scene_rect = view->mapToScene(view->viewport()->rect()).boundingRect();
		setPos(
				visible_scene_rect.center() - QPointF(r / zoomLevel, (y + r) / zoomLevel - visible_scene_rect.height() * .5)
			);

		setRect(0, 0, (2 * r) / zoomLevel, (2 * r) / zoomLevel);
	}
	void setZoomLevel(int zoomLevel) { this->zoomLevel = zoomLevel; adjustPosition(); }
signals:
	void released(void);
	void pressed(int);
public:
	enum { FIRE_UP, FIRE_DOWN, };
	enum { Type = UserType + __COUNTER__ + 1, };
	int type(void) const {return Type;}
	JoypadFire(int x, int y, int radius, QGraphicsView * view, QGraphicsItem * parent = 0)
		: QGraphicsEllipseItem(0, 0, 2 * radius, 2 * radius, parent)
	{ setPen(QPen(Qt::green)); this->x = x, this->y = y, r = radius; this->view= view; setAcceptTouchEvents(true); setTransformOriginPoint(boundingRect().center()); adjustPosition(); }
};


class JoypadLeftRight : public QObject, public QGraphicsEllipseItem
{
	Q_OBJECT
protected:
	bool sceneEvent(QEvent *event) override
	{
		bool result = false;
		switch (event->type())
		{
			case QEvent::TouchUpdate:
			case QEvent::TouchBegin:
				result =  true;
		{
			QTouchEvent * e = static_cast<QTouchEvent *>(event);
			QPointF p;
			for (auto t : e->touchPoints())
			{
				if (contains(t.pos()))
					p = t.pos();
			}
			if (p.isNull())
				break;
			/* check joypad zones */
			auto angle = fmod(360. - Util::degrees(Util::angleForVector(p - boundingRect().center())), 360.);
			if (angle > 270. || angle <= 90.)
				emit pressed(RIGHT);
			else
				emit pressed(LEFT);
		}
				break;
			case QEvent::TouchEnd:
			emit released();
			result = true;
		}
		return result;
	}
private:
	int x, y, r;
	int zoomLevel = 1;
	QGraphicsView * view;
public slots:
	void adjustPosition(void)
	{

	QRectF visible_scene_rect = view->mapToScene(view->viewport()->rect()).boundingRect();
		setPos(
				visible_scene_rect.bottomRight() - QPointF((x + r) / zoomLevel, (y + r) / zoomLevel)
			);

		//qDebug() <<"joypad position" << pos() << "visible scene rect:" << visible_scene_rect;
		//qDebug() << "scroll bar values" << view->horizontalScrollBar()->value() << view->verticalScrollBar()->value() << "viewport:" << view->viewport()->rect();
		setRect(0, 0, (2 * r) / zoomLevel, (2 * r) / zoomLevel);
	}
	void setZoomLevel(int zoomLevel) { this->zoomLevel = zoomLevel; adjustPosition(); }
signals:
	void released(void);
	void pressed(int);
public:
	enum { LEFT, RIGHT, };
	enum { Type = UserType + __COUNTER__ + 1, };
	int type(void) const {return Type;}
	JoypadLeftRight(int x, int y, int radius, QGraphicsView * view, QGraphicsItem * parent = 0)
		: QGraphicsEllipseItem(0, 0, 2 * radius, 2 * radius, parent)
	{ setPen(QPen(Qt::cyan)); this->x = x, this->y = y, r = radius; this->view= view; setAcceptTouchEvents(true); setTransformOriginPoint(boundingRect().center()); adjustPosition(); }
};

class JoypadUpDown : public QObject, public QGraphicsEllipseItem
{
	Q_OBJECT
	int lastTouchpointId;
protected:
	bool sceneEvent(QEvent *event) override
	{
		bool result = false;
		QTouchEvent * e = static_cast<QTouchEvent *>(event);
		QPointF p;
		double angle;
		switch (event->type())
		{
			case QEvent::GraphicsSceneMousePress:
qCritical() << "touch mouse PRESS:";
			if (0)
			case QEvent::GraphicsSceneMouseMove:
qCritical() << "touch mouse MOVE:";
				p = (static_cast<QGraphicsSceneMouseEvent *>(event))->pos();
				qDebug() << "joypad mouse event at:" << p;
				if (0)
		{
			case QEvent::TouchBegin:
qCritical() << "touch BEGIN:";
				lastTouchpointId = e->touchPoints().at(0).id();
				/* fall out */
				if (0)
			case QEvent::TouchUpdate:
qCritical() << "touch UPDATE:";
qCritical() << e->touchPoints();
			for (auto t : e->touchPoints())
			{
				if (t.id() == lastTouchpointId)
				{
					p = t.pos();
					goto process_event;
				}
			}
					break;
		}
process_event:
			result =  true;
			if (p.isNull())
				break;
			/* check joypad zones */
			angle = fmod(360. - Util::degrees(Util::angleForVector(p - boundingRect().center())), 360.);
			emit requestAngle(angle);
			emit pressed(UP);
			/*
			if (angle < 180.)
				emit pressed(UP);
			else
				emit pressed(DOWN);
				*/
				break;
			case QEvent::TouchEnd:
			qCritical() << "touch END:";
			qCritical() << e->touchPoints();
			if (0)
			case QEvent::GraphicsSceneMouseRelease:
qCritical() << "touch mouse END:";
			emit released();
			result = true;
		}
		return result;
	}
	/*
	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR) override
	{
		QGraphicsEllipseItem::paint(painter, option, widget);
		painter->drawRect(rect());
	}
	*/
private:
	int x, y, r;
	QGraphicsView * view;
	int zoomLevel = 1;
public slots:
	void adjustPosition(void)
	{

	QRectF visible_scene_rect = view->mapToScene(view->viewport()->rect()).boundingRect();
		setPos(
				visible_scene_rect.bottomLeft() + QPointF((x - r) / zoomLevel, - (y + r) / zoomLevel)
			);

		setRect(0, 0, (2 * r) / zoomLevel, (2 * r) / zoomLevel);
	}
	void setZoomLevel(int zoomLevel) { this->zoomLevel = zoomLevel; adjustPosition(); }
signals:
	void released(void);
	void pressed(int);
	void requestAngle(double angle);
public:
	enum { UP, DOWN, };
	enum { Type = UserType + __COUNTER__ + 1, };
	int type(void) const {return Type;}
	//virtual QPainterPath shape(void) const override { QPainterPath p; p.addRect(rect()); return p; }
	JoypadUpDown(int x, int y, int radius, QGraphicsView * view, QGraphicsItem * parent = 0)
		: QGraphicsEllipseItem(0, 0, 2 * radius, 2 * radius, parent)
	{ setPen(QPen(Qt::magenta)); this->x = x, this->y = y, r = radius; this->view= view; setAcceptTouchEvents(true); setTransformOriginPoint(boundingRect().center()); adjustPosition();
		setFlag(QGraphicsItem::ItemIsMovable); }
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
	int type(void) const {return Type;}
	Animation(QGraphicsItem * parent, const QPixmap pixmap, int frameWidth, int framePeriod, bool loop = false, bool playForwardAndBackward = false) : QGraphicsPixmapItem(QPixmap(), parent)
	{
		this->pixmap = pixmap;
		this->loop = loop;
		this->playForwardAndBackward = playForwardAndBackward;
		this->frameWidth = frameWidth;
		setTransformOriginPoint(boundingRect().center());
		connect(& timer, SIGNAL(timeout()), this, SLOT(timerElapsed()));
		setPixmap(pixmap.copy(0, 0, frameWidth, pixmap.height()));
		timer.setInterval(framePeriod);
	}

	Animation(QGraphicsItem * parent, QString pixmapFileName, int frameWidth, int framePeriod, bool loop = false, bool playForwardAndBackward = false) :
	      Animation(parent, QPixmap(pixmapFileName), frameWidth, framePeriod, loop, playForwardAndBackward) { }
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
		QRect r;
		while (1)
		{
			frameIndex += isPlayingForward ? 1 : -1;
			r = QRect((frameIndex) * frameWidth, 0, frameWidth, pixmap.height());
			if (r.left() >= pixmap.width() || r.left() < 0)
			{
				if (loop)
				{
					if (!playForwardAndBackward)
					{
						frameIndex = -1;
					}
					else
					{
						if (isPlayingForward)
							frameIndex = pixmap.width() / frameWidth;
						else
							frameIndex = -1;
						isPlayingForward = ! isPlayingForward;
					}
					continue;
				}
				else
				{
					timer.stop();
					emit animationFinished(this);
				}
				return;
			}
			break;
		}
		setPixmap(pixmap.copy(r));
	}

signals:
	void animationFinished(Animation * a);
};


class Player : public QGraphicsPixmapItem
{
private:
	int rotation_angle = 0;
public:
	enum { Type = UserType + __COUNTER__ + 1, };
	int type(void) const {return Type;}
	Player(QGraphicsItem * parent = 0) : QGraphicsPixmapItem(parent)
	{
		setPixmap(QPixmap(":/stalker-ship.png"));
		setTransformOriginPoint(boundingRect().center());
		setZValue(PLAYER_Z_VALUE);
	}
	void setRotation(int angle) { if (angle < 0) angle += 360; rotation_angle = angle % 360; QGraphicsPixmapItem::setRotation(- rotation_angle); }
	int getRotationAngle(void) { return rotation_angle; }
};



class Projectile : public QObject, public QGraphicsPixmapItem
{
	Q_OBJECT
	enum
	{
		MAX_PROJECTILE_RANGE	= 200,
	};
	QVector2D velocity;
	QPointF launchPoint;
	QTimer timer;
public:
	Projectile(QGraphicsItem * parent, QString pixmapFileName, const QVector2D & velocity, const QPointF & launchPoint) : QGraphicsPixmapItem(parent)
	{
		this->velocity = velocity;
		setPixmap(QPixmap(pixmapFileName));
		setTransformOriginPoint(.0, boundingRect().height() * .5);
		setPos(this->launchPoint = launchPoint - QPointF(0, .5 * pixmap().height()));
		setRotation(Util::degrees(Util::angleForVector(velocity)));
		timer.setInterval(30);
		connect(&timer, SIGNAL(timeout()), this, SLOT(timeout()));
	}
	void start(void) { timer.start(); }
private slots:
	void timeout(void)
	{
		setPos(pos() + velocity.toPointF());
		auto items = scene()->collidingItems(this);
		for (auto i : items)
		{
			if (auto a = qgraphicsitem_cast<Animation *>(i))
				emit a->animationFinished(a);
		}
		if (QVector2D(launchPoint - pos()).length() > MAX_PROJECTILE_RANGE)
		{
			setPixmap(QPixmap());
			timer.stop();
			emit projectileDeactivated(this);
		}
	}
signals:
	void projectileDeactivated(Projectile * p);
};


class GameScene : public QGraphicsScene
{
	Q_OBJECT
private:
	enum
	{
		TIMER_POLL_INTERVAL_MS		=	100,
		MAX_ROTATION_SPEED_DEGREES	=	20,
	};
	static constexpr double AFTERBURNER_DISTANCE_CHANGE_ANIMATION = 2.;
	Player * player = 0;
	QVector2D lastAfterburnAnimationPosition;
	int rotationSpeed = 0;
	QTimer timer;
	double speed = 0;
	const double MAX_SPEED_UNITS = 5.;
	const double acceleration = .2;
	const double max_rotation_speed_degrees = 15.;
	double requestedPlayerAngle = INFINITY;
	QVector2D playerForwardVector(void)
	{
		auto angle = Util::rad(player->getRotationAngle());
		return QVector2D(cos(angle), sin(- angle));
	}
	struct
	{
		union
		{
			unsigned		rotationKeys;
			struct
			{
				unsigned	isLeftPressed	: 1;
				unsigned	isRightPressed	: 1;
			};
		};
		union
		{
			unsigned		movementKeys;
			struct
			{
				unsigned	isForwardPressed	: 1;
				unsigned	isBackwardPressed	: 1;
			};
		};
	}
	keypresses;
	QPixmap backgroundPixmap;
protected:
	void keyReleaseEvent(QKeyEvent *keyEvent) override
	{
		if (keyEvent->isAutoRepeat() || !player)
			return;
		switch (keyEvent->key())
		{
		case Qt::Key_Left: keypresses.isLeftPressed = 0; break;
		case Qt::Key_Right: keypresses.isRightPressed = 0; break;
		case Qt::Key_Up: keypresses.isForwardPressed = 0; break;
		case Qt::Key_Down: keypresses.isBackwardPressed = 0; break;
		default: QGraphicsScene::keyReleaseEvent(keyEvent); break;
		}
	}
#if 1
	virtual void drawBackground(QPainter *painter, const QRectF &rect) override
	{
	QRectF visible_scene_rect = views().at(0)->mapToScene(views().at(0)->rect()).boundingRect();
		QRect pf = QRect(rect.x() - visible_scene_rect.x(), rect.y() - visible_scene_rect.y(), rect.width(), rect.height());
		//qDebug() << "original rect:" << rect << "visible scene rect:" << views().at(0)->mapToScene(views().at(0)->rect()).boundingRect() << "pixmap rect:" << pf << views().at(0)->verticalScrollBar()->singleStep();
		painter->drawPixmap(rect.topLeft(), backgroundPixmap.copy(pf));
	}
#endif
	void keyPressEvent(QKeyEvent *keyEvent) override
	{
		if (keyEvent->isAutoRepeat() || !player)
			return;
		QPointF pos = player->pos();
		switch (keyEvent->key()) {
		case Qt::Key_W: pos += QPoint(0, -1); break;
		case Qt::Key_A: pos += QPoint(-1, 0); break;
		case Qt::Key_S: pos += QPoint(0, 1); break;
		case Qt::Key_D: pos += QPoint(1, 0); break;
		case Qt::Key_Left: keypresses.isLeftPressed = 1; break;
		case Qt::Key_Right: keypresses.isRightPressed = 1; break;
		case Qt::Key_Up: keypresses.isForwardPressed = 1; break;
		case Qt::Key_Down: keypresses.isBackwardPressed = 1; break;
		case Qt::Key_Space: fireProjectile(); break;
		default: QGraphicsScene::keyPressEvent(keyEvent); return;
		}
		player->setPos(pos);
	}
private slots:
	void pollKeyboard(void)
	{
		if (keypresses.isLeftPressed)
			rotationSpeed += (rotationSpeed < 0) ? +3 : +1;
		if (keypresses.isRightPressed)
			rotationSpeed += (rotationSpeed > 0) ? -3 : -1;
		if (!keypresses.rotationKeys && rotationSpeed)
			rotationSpeed += (rotationSpeed > 0) ? -1 : 1;
		rotationSpeed = Util::bound(- MAX_ROTATION_SPEED_DEGREES, rotationSpeed, MAX_ROTATION_SPEED_DEGREES);
		player->setRotation(player->getRotationAngle() + rotationSpeed); 

		/* handle joypad rotation - ease rotation */
		if (!keypresses.rotationKeys && requestedPlayerAngle != INFINITY)
		{
			auto r = fabs(requestedPlayerAngle - player->getRotationAngle());
			QVector2D rp = playerForwardVector(), rq = QVector2D(cos(Util::rad(requestedPlayerAngle)), - sin(Util::rad(requestedPlayerAngle)));
			r = std::min(max_rotation_speed_degrees, r);
			/* use cross product to determine rotation direction from the current player forward vector to the requested player forward vector */
			auto s = std::signbit(rp.x() * rq.y() - rp.y() * rq.x());
			if (!s)
				r *= -1.;
			player->setRotation(player->getRotationAngle() + r);
			if (fabs(player->getRotationAngle() - requestedPlayerAngle) < 1.)
				requestedPlayerAngle = INFINITY;
		}

		if (keypresses.isForwardPressed)
			speed += (speed < .0) ? 2 * acceleration : 1 * acceleration;
		if (keypresses.isBackwardPressed)
			speed += (speed > .0) ? -2 * acceleration : -1 * acceleration;
		if (!keypresses.movementKeys && fabs(speed) > acceleration)
			speed += (speed > .0) ? -1 * acceleration : 1 * acceleration;
		else if (!keypresses.movementKeys && fabs(speed) < acceleration)
			speed = .0;
		speed = Util::bound(- MAX_SPEED_UNITS, speed, MAX_SPEED_UNITS);
		auto oldPosition = player->pos();
		player->setPos(player->pos() + speed * playerForwardVector().toPointF());
		if (oldPosition != player->pos())
			emit playerObjectPositionChanged();
		if ((lastAfterburnAnimationPosition - QVector2D(player->pos()) + 50 * playerForwardVector()).length() > AFTERBURNER_DISTANCE_CHANGE_ANIMATION)
		{
			auto a = new Animation(0, ":/afterburn-white.png", 12, 100, false);
			connect(a, & Animation::animationFinished, [=](Animation * a){ removeItem(a); delete a; });
			a->setPos(player->pos() - 50 * playerForwardVector().toPointF());
			a->setZValue(EXPLOSIONS_Z_VALUE);
			addItem(a);
			a->start();
			lastAfterburnAnimationPosition = QVector2D(player->pos() - 50 * playerForwardVector().toPointF());
		}
		for (auto item : player->collidingItems())
		{
			if (auto p = qgraphicsitem_cast<Animation *>(item))
				emit p->animationFinished(p);
		}
	}
signals:
	void playerObjectPositionChanged(void);
public slots:
	void joypadUpDownReleased(void) { keypresses.movementKeys = 0; }
	void joypadUpDownPressed(int direction)
	{
		joypadUpDownReleased();
		switch (direction) {
		case JoypadUpDown::UP:
			forwardPressed();
			break;
		case JoypadUpDown::DOWN:
			backwardPressed();
			break;
		}
	}
	void joypadLeftRightReleased(void) { keypresses.rotationKeys = 0; }
	void joypadLeftRightPressed(int direction)
	{
		joypadLeftRightReleased();
		switch (direction) {
		case JoypadLeftRight::LEFT:
			leftPressed();
			break;
		case JoypadLeftRight::RIGHT:
			rightPressed();
			break;
		}
	}

	void joypadFirePressed(void) { fireProjectile(); }
	void joypadSetAngle(double angle) { requestedPlayerAngle = angle; if (angle < .0) angle += 360.; }

public:
	void fireProjectile(void)
	{

		QPointF c = playerForwardVector().toPointF();
		auto playerLength = player->pixmap().width();
		auto pos = player->pos();
		auto a = new Animation(0, ":/explosion-1.png", 24, 30, false);
		connect(a, & Animation::animationFinished, [=](Animation * a){ removeItem(a); delete a; });
		a->setPos(pos + 10 * playerLength * c);
		a->setZValue(EXPLOSIONS_Z_VALUE);
		addItem(a);
		a->start();

		a = new Animation(0, ":/explosion-2.png", 24, 30, false);
		connect(a, & Animation::animationFinished, [=](Animation * a){ removeItem(a); delete a; });
		a->setPos(pos + 10 * playerLength * c + playerLength * QPointF(c.y(), - c.x()));
		a->setZValue(EXPLOSIONS_Z_VALUE);
		addItem(a);
		a->start();

		a = new Animation(0, ":/explosion-3.png", 12, 30, false);
		connect(a, & Animation::animationFinished, [=](Animation * a){ removeItem(a); delete a; });
		a->setPos(pos + 10 * playerLength * c + playerLength * QPointF(- c.y(), c.x()));
		a->setZValue(EXPLOSIONS_Z_VALUE);
		addItem(a);
		a->start();

		a = new Animation(0, ":/explosion-4.png", 12, 100, false);
		connect(a, & Animation::animationFinished, [=](Animation * a){ removeItem(a); delete a; });
		a->setPos(pos + 10 * playerLength * c + 2 * playerLength * QPointF(- c.y(), c.x()));
		a->setZValue(EXPLOSIONS_Z_VALUE);
		addItem(a);
		a->start();

		Projectile * p = new Projectile(0, ":/projectile.png", playerForwardVector() * 2,
						pos
						+ player->boundingRect().center()
						+ .5 * playerForwardVector().toPointF() * playerLength);
		connect(p, & Projectile::projectileDeactivated, [=](Projectile * a){ removeItem(a); delete a; });
		addItem(p);
		p->start();
	}

	GameScene(QObject * parent = 0) : QGraphicsScene(parent)
	{
		memset(& keypresses, 0, sizeof keypresses);
		timer.setInterval(TIMER_POLL_INTERVAL_MS);
		connect(& timer, SIGNAL(timeout()), this, SLOT(pollKeyboard()));
		backgroundPixmap = QPixmap(":/PIA06909-1920x1200.jpg");
	}
	void forwardPressed(void) { keypresses.isForwardPressed = 1; }
	void forwardReleased(void) { keypresses.isForwardPressed = 0; }

	void backwardPressed(void) { keypresses.isBackwardPressed = 1; }
	void backwardReleased(void) { keypresses.isBackwardPressed = 0; }

	void leftPressed(void) { keypresses.isLeftPressed = 1; }
	void leftReleased(void) { keypresses.isLeftPressed = 0; }

	void rightPressed(void) { keypresses.isRightPressed = 1; }
	void rightReleased(void) { keypresses.isRightPressed = 0; }

	void setPlayer(Player * player) { this->player = player; timer.start(); }
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
	int type(void) const {return Type;}
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
	virtual QPainterPath shape(void) const override { QPainterPath p; if (isCollisionEnabled) p = QGraphicsPixmapItem::shape(); return p; }
signals:
	void tileSelected(Tile * tile);
	void tileShiftSelected(Tile * tile);
	void tileControlSelected(Tile * tile);
	void tileReleased(Tile * tile);
protected:
	virtual void mousePressEvent(QGraphicsSceneMouseEvent * event) override
	{ if (event->modifiers() & Qt::ShiftModifier) emit tileShiftSelected(this); else if (event->modifiers() & Qt::ControlModifier) emit tileControlSelected(this);
		else emit tileSelected(this);
		/* ugly, ugly, ugly... */
		static const QMetaMethod tileReleasedSignal = QMetaMethod::fromSignal(&Tile::tileReleased);
		isSignalConnected(tileReleasedSignal) ? event->accept() : event->ignore(); }
	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent * event) override { emit tileReleased(this); }
};

class TileButton : public Tile
{
	Q_OBJECT
signals:
	void pressed(void);
public:
	TileButton(const QPixmap &pixmap, QGraphicsItem *parent = Q_NULLPTR) : Tile(pixmap, parent) { }
	virtual void mousePressEvent(QGraphicsSceneMouseEvent * event) override { event->accept(); }
	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent * event) override { event->accept(); if (contains(event->pos())) emit pressed(); }
};

class TouchTile : public Tile
{
	Q_OBJECT
public:
	TouchTile(const QPixmap &pixmap, QGraphicsItem *parent = Q_NULLPTR) : Tile(pixmap, parent) { setAcceptTouchEvents(true); }
signals:
	void tileTouchStarted(TouchTile *);
	void tileTouchEnded(TouchTile *);
protected:
	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR) override
	{
		Tile::paint(painter, option, widget);
		painter->drawRect(pixmap().rect());
	}
	virtual QPainterPath shape(void) const override { QPainterPath p; p.addRect(pixmap().rect()); return p; }
	bool sceneEvent(QEvent *event) override
	{
		if (event->type() == QEvent::TouchBegin)
		{
			qCritical() << "touch begin at" << getX() << getY();
			emit tileTouchStarted(this);
			return true;
		}
		else if (event->type() == QEvent::TouchEnd)
		{
			qCritical() << "touch end at" << getX() << getY();
			emit tileTouchEnded(this);
			return true;
		}
		return false;
	}

};

enum
{
	MINIMUM_TILE_SIZE	=	8,
	MINIMUM_MAP_SIZE	=	8,
};

class TileSet : public QObject
{
	Q_OBJECT
protected:
	QPixmap pixmap;
	int tile_width = MINIMUM_TILE_SIZE, tile_height = MINIMUM_TILE_SIZE;
public:
	QRect tileRect(void) { return QRect(0, 0, tile_width, tile_height); }
	void setPixmap(const QPixmap & pixmap) { this->pixmap = pixmap; }
	int tileWidth(void) { return tile_width; }
	int tileHeight(void) { return tile_height; }
	const QPixmap getPixmap(void) { return pixmap; }

	int tileCountX(void) { return pixmap.width() / tile_width; }
	int tileCountY(void) { return pixmap.height() / tile_height; }
	QPixmap getTilePixmap(int x, int y) { return pixmap.copy(tileRect().adjusted(x * tile_width, y * tile_height, x * tile_width, y * tile_height)); }
public slots:
	void setTileWidth(int width) { tile_width = width; }
	void setTileHeight(int height) { tile_height = height; }
};

struct AnimatedTile
{
	struct TileCoordinates { int x = 0, y = 0; TileCoordinates(int x, int y) { this->x = x, this->y = y; } TileCoordinates(void) { x = y = 0; } };
	QString name;
	bool playForwardAndBackward = false, isPlayingForward = true;
	int frameIndex = 0;
	QVector<struct TileCoordinates> animationFrames;

	void advanceFrame(void) { ++ frameIndex; frameIndex %= animationFrames.size(); }

	void read(const QJsonObject & json)
	{
		name = json["name"].toString("<<<n/a>>>");
		playForwardAndBackward = json["play-back-and-forth"].toBool(false);
		QJsonArray frames = json["animation-frames"].toArray();
		for (auto f : frames)
			animationFrames << AnimatedTile::TileCoordinates(f.toObject()["tile-x"].toInt(-1), f.toObject()["tile-y"].toInt(-1));
	}
	void write(QJsonObject & json) const
	{
		QJsonArray frames;
		for (auto f : animationFrames)
		{
			QJsonObject x;
			x["tile-x"] = f.x;
			x["tile-y"] = f.y;
			frames.append(x);
		}
		json["name"] = name;
		json["play-back-and-forth"] = playForwardAndBackward;
		json["animation-frames"] = frames;
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
public slots:
	void applicationStateChanged(Qt::ApplicationState state);
private slots:
	void on_pushButtonOpenImage_clicked();
	void on_pushButtonResetTileData_clicked();
	void tileSelected(int tileX, int tileY);
	void mapTileSelected(Tile * tile);
	void tileSelected(Tile * tile);
	void tileShiftSelected(int tileX, int tileY);
	void tileShiftSelected(Tile * tile);
	void timeoutTileAnimationTimer(void);
	void on_pushButtonAddTerrain_clicked();

	void on_lineEditNewTerrain_returnPressed();

	void on_pushButtonRemoveTerrain_clicked();

	void on_pushButtonUpdateTile_clicked();

	void on_pushButtonReapTilesExact_clicked() { displayFilteredTiles(true); }

	void on_pushButtonReapTilesAny_clicked() { displayFilteredTiles(false); }

	void on_pushButtonMarkTerrain_clicked();

	void on_pushButtonFillMap_clicked();

	void on_pushButtonAnimationSequence_clicked();

private:
	QTimer timerTileAnimation;
	void updateTileAnimationList();
	bool isDefiningAnimationSequence = false;
	QVector<struct AnimatedTile::TileCoordinates> currentTileAnimation;
	/* This vector holds all 'Tile' items in the tile map that must be animated.
	 * the number of items in the vector must equal the number of different tile
	 * animations in the 'tileAnimations' vector. Each element of the vector
	 * is itself a vector containing all tiles for the corresponding animation type
	 */
	QVector<QVector<Tile *>> tileMapAnimatedItems;
	QVector<AnimatedTile> tileAnimations;
	void scanGameSceneForAnimatedTiles(void);

	JoypadUpDown	* joypadUpDown;
	JoypadLeftRight	* joypadLeftRight;
	JoypadFire	* joypadFire;
	void saveProgramData(void);
	void saveMap(const QString & fileName);
	bool loadMap(const QString & fileName);
	void clearMap(void);
	QVector<QCheckBox*> terrain_checkboxes;
	TileInfo	* last_tile_selected = 0;
	Tile		* lastTileFromMapSelected = 0;
	Ui::MapEditor *ui;
	TileSet tileSet;
	QString last_map_image_filename;
	QVector<QVector<class TileInfo>> tileInfo;
	void resetTileData(int tileCountX, int tileCountY)
	{ int row; for (row = 0; row < tileCountY; row ++) { tileInfo << QVector<class TileInfo>(tileCountX); int column = 0; for (auto & t : tileInfo.last()) t.setXY(column ++, row); } }
	qint64 terrainBitmap(void) { qint64 t = 0, i = 0; for (auto c : terrain_checkboxes) t |= (c->isChecked() ? (1 << i) : 0), ++ i; return t; }
	QGraphicsScene tileSetGraphicsScene, filteredTilesGraphicsScene, touchControlsGraphicsScene;
	GameScene tileMapGraphicsScene;
	QVector<Tile *> graphicsSceneTiles;
	void displayFilteredTiles(bool exactTerrainMatch);
	QVector<QVector<Tile *>> tileMap[MAP_LAYERS];
	QVector<QGraphicsEllipseItem *> tileMarks;

	Player * player;
	QGraphicsDropShadowEffect * ds;
	QGraphicsBlurEffect * blur;

	TouchTile	* upArrowOverlayButton, * downArrowOverlayButton, * leftArrowOverlayButton, * rightArrowOverlayButton, * fireOverlayButton;
	QGraphicsScene buttonUpScene, buttonDownScene, buttonLeftScene, buttonRightScene, buttonFireScene, buttonsScene;
	void setupTouchButton(QGraphicsView * graphicsView, QGraphicsScene & graphicsScene, TouchTile * touchTileItem,
		std::function<void()> const & touchStartLambda, std::function<void()> const & touchEndLambda);
protected:
	void closeEvent(QCloseEvent * event);
	bool eventFilter(QObject * watched, QEvent * event);
};

#endif // MAPEDITOR_HXX
