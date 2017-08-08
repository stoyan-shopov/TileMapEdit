#ifndef MAPEDITOR_HXX
#define MAPEDITOR_HXX

#include <QMainWindow>
#include <QPainter>

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
protected:
	virtual void paintEvent(QPaintEvent * event)
	{
		int i;
		QPainter p(this);
		p.drawImage(0, 0, image);
		p.setPen(Qt::green);
		for (i = 0; i < image.width(); p.drawLine(i, 0, i, image.height()), i += tile_width);
		for (i = image.height() - tile_height; i >= 0; p.drawLine(0, i, image.width(), i), i -= tile_height);
	}
public:
	TileSheet(void) { tile_width = tile_height = MINIMUM_TILE_SIZE; }
	void setImage(const QImage & image) { this->image = image; repaint(); }
public slots:
	void setTileWidth(int width) { tile_width = width; repaint(); }
	void setTileHeight(int height) { tile_height = height; repaint(); }
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
	
private:
	Ui::MapEditor *ui;
	TileSheet * tileSheet;
};

#endif // MAPEDITOR_HXX
