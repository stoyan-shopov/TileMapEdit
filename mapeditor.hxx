#ifndef MAPEDITOR_HXX
#define MAPEDITOR_HXX

#include <QMainWindow>
#include <QPainter>

class TileSheet : public QWidget
{
	Q_OBJECT
private:
	QImage image;
protected:
	virtual void paintEvent(QPaintEvent * event)
	{
		QPainter p(this);
		p.drawImage(0, 0, image);
	}
public:
	void setImage(const QImage & image) { this->image = image; }
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
