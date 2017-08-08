#include <QFileDialog>
#include <QMessageBox>

#include "mapeditor.hxx"
#include "ui_mapeditor.h"

MapEditor::MapEditor(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MapEditor)
{
	ui->setupUi(this);
	/*! \todo	the scroll area is currently not working */
	tileSheet = new TileSheet;
	tileSheet->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	ui->scrollAreaTileSheet->setWidgetResizable(true);
	ui->scrollAreaTileSheet->setWidget(tileSheet);

	connect(ui->spinBoxTileWidth, SIGNAL(valueChanged(int)), tileSheet, SLOT(setTileWidth(int)));
	connect(ui->spinBoxTileHeight, SIGNAL(valueChanged(int)), tileSheet, SLOT(setTileHeight(int)));
	connect(ui->spinBoxZoomLevel, SIGNAL(valueChanged(int)), tileSheet, SLOT(setZoomFactor(int)));
	connect(ui->spinBoxTileWidth, SIGNAL(valueChanged(int)), this, SLOT(repaint()));
	connect(ui->spinBoxTileHeight, SIGNAL(valueChanged(int)), this, SLOT(repaint()));
}

MapEditor::~MapEditor()
{
	delete ui;
}

void MapEditor::on_pushButtonOpenImage_clicked()
{
	auto s = QFileDialog::getOpenFileName(0, "select image to open");
	QImage image(s);
	if (image.isNull())
	{
		QMessageBox::warning(0, "error opening image", "Error opening image!");
		return;
	}
	tileSheet->setImage(image);
}

void MapEditor::paintEvent(QPaintEvent *event)
{
	int i;
	QPainter p(this);
	for (i = 0; i < width(); p.drawLine(i, 0, i, height()), i += ui->spinBoxTileWidth->value());
	for (i = 0; i < height(); p.drawLine(0, i, width(), i), i += ui->spinBoxTileHeight->value());
}

void MapEditor::mousePressEvent(QMouseEvent *event)
{
	auto image = QApplication::clipboard()->image();
	if (image.width() == ui->spinBoxTileWidth->value() && image.height() == ui->spinBoxTileHeight->value())
		*(int*)0=0;
}
