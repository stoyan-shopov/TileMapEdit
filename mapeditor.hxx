#ifndef MAPEDITOR_HXX
#define MAPEDITOR_HXX

#include <QApplication>
#include <QMainWindow>
#include <QPainter>
#include <QClipboard>
#include <QMouseEvent>

class TileSheet : public QWidget
{
	Q_OBJECT
private:
	enum
	{
		MINIMUM_TILE_SIZE	=	8,
		MAXIMUM_TILE_SIZE	=	1024,
	};
	QImage image;
	int tile_width, tile_height;
	int zoom_factor;
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
		for (i = 0; i < w; p.drawLine(i, 0, i, h), i += tile_width * zoom_factor);
		for (i = (image.height() - tile_height) * zoom_factor; i >= 0; p.drawLine(0, i, w, i), i -= tile_height * zoom_factor);
	}
	virtual void mousePressEvent(QMouseEvent *event)
	{
		int x, y;
		if (image.isNull())
			return;
		if ((x = event->x()) <= image.width() && (y = event->y()) < image.height())
			QApplication::clipboard()->setImage(image.copy((x / tile_width) * tile_width, (x / tile_height) * tile_height, tile_width, tile_height));
	}
public:
	TileSheet(void) { tile_width = tile_height = MINIMUM_TILE_SIZE; zoom_factor = 1; }
	void setImage(const QImage & image) { this->image = image; repaint(); }
public slots:
	void setTileWidth(int width) { tile_width = width; repaint(); }
	void setTileHeight(int height) { tile_height = height; repaint(); }
	void setZoomFactor(int zoom_factor) { this->zoom_factor = zoom_factor; repaint(); }
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

protected:
	virtual void paintEvent(QPaintEvent * event);
	virtual void mousePressEvent(QMouseEvent *event);
	
private:
	Ui::MapEditor *ui;
	TileSheet * tileSheet;
};

#endif // MAPEDITOR_HXX
