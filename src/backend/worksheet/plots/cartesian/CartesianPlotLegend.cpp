/***************************************************************************
    File                 : CartesianPlotLegend.cpp
    Project              : LabPlot
    Description          : Legend for the cartesian plot
    --------------------------------------------------------------------
    Copyright            : (C) 2013-2018 Alexander Semke (alexander.semke@web.de)
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 2 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/

/*!
  \class CartesianPlotLegend
  \brief Legend for the cartesian plot.

  \ingroup kdefrontend
*/

#include "CartesianPlotLegend.h"
#include "backend/worksheet/plots/cartesian/CartesianPlotLegendPrivate.h"
#include "backend/worksheet/plots/cartesian/CartesianPlot.h"
#include "backend/worksheet/plots/cartesian/XYCurve.h"
#include "backend/worksheet/Worksheet.h"
#include "backend/lib/XmlStreamReader.h"
#include "backend/worksheet/TextLabel.h"
#include "backend/lib/commandtemplates.h"

#include <QPainterPath>
#include <QPainter>
#include <QGraphicsSceneContextMenuEvent>
#include <QMenu>

#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>

CartesianPlotLegend::CartesianPlotLegend(CartesianPlot* plot, const QString &name)
		: WorksheetElement(name), d_ptr(new CartesianPlotLegendPrivate(this)), m_plot(plot) {
	init();
}

CartesianPlotLegend::CartesianPlotLegend(CartesianPlot* plot, const QString &name, CartesianPlotLegendPrivate *dd)
		: WorksheetElement(name), d_ptr(dd), m_plot(plot) {
	init();
}

//no need to delete the d-pointer here - it inherits from QGraphicsItem
//and is deleted during the cleanup in QGraphicsScene
CartesianPlotLegend::~CartesianPlotLegend() = default;

void CartesianPlotLegend::init() {
	Q_D(CartesianPlotLegend);

	KConfig config;
	KConfigGroup group = config.group( "CartesianPlotLegend" );

	d->labelFont = group.readEntry("LabelsFont", QFont());
	d->labelFont.setPixelSize( Worksheet::convertToSceneUnits( 10, Worksheet::Point ) );

	d->labelColor = Qt::black;
	d->labelColumnMajor = true;
	d->lineSymbolWidth = group.readEntry("LineSymbolWidth", Worksheet::convertToSceneUnits(1, Worksheet::Centimeter));
	d->rowCount = 0;
	d->columnCount = 0;

	d->position.horizontalPosition = CartesianPlotLegend::hPositionRight;
	d->position.verticalPosition = CartesianPlotLegend::vPositionBottom;

	d->rotationAngle = group.readEntry("Rotation", 0.0);

	//Title
 	d->title = new TextLabel(this->name(), TextLabel::PlotLegendTitle);
	addChild(d->title);
	d->title->setHidden(true);
	d->title->setParentGraphicsItem(graphicsItem());
	d->title->graphicsItem()->setFlag(QGraphicsItem::ItemIsMovable, false);
	connect(d->title, &TextLabel::changed, this, &CartesianPlotLegend::retransform);

	//Background
	d->backgroundType = (PlotArea::BackgroundType) group.readEntry("BackgroundType", (int) PlotArea::Color);
	d->backgroundColorStyle = (PlotArea::BackgroundColorStyle) group.readEntry("BackgroundColorStyle", (int) PlotArea::SingleColor);
	d->backgroundImageStyle = (PlotArea::BackgroundImageStyle) group.readEntry("BackgroundImageStyle", (int) PlotArea::Scaled);
	d->backgroundBrushStyle = (Qt::BrushStyle) group.readEntry("BackgroundBrushStyle", (int) Qt::SolidPattern);
	d->backgroundFileName = group.readEntry("BackgroundFileName", QString());
	d->backgroundFirstColor = group.readEntry("BackgroundFirstColor", QColor(Qt::white));
	d->backgroundSecondColor = group.readEntry("BackgroundSecondColor", QColor(Qt::black));
	d->backgroundOpacity = group.readEntry("BackgroundOpacity", 1.0);

	//Border
	d->borderPen = QPen(group.readEntry("BorderColor", QColor(Qt::black)),
										 group.readEntry("BorderWidth", Worksheet::convertToSceneUnits(1.0, Worksheet::Point)),
										 (Qt::PenStyle) group.readEntry("BorderStyle", (int)Qt::SolidLine));
	d->borderCornerRadius = group.readEntry("BorderCornerRadius", 0.0);
	d->borderOpacity = group.readEntry("BorderOpacity", 1.0);

	//Layout
	d->layoutTopMargin =  group.readEntry("LayoutTopMargin", Worksheet::convertToSceneUnits(0.2f, Worksheet::Centimeter));
	d->layoutBottomMargin = group.readEntry("LayoutBottomMargin", Worksheet::convertToSceneUnits(0.2f, Worksheet::Centimeter));
	d->layoutLeftMargin = group.readEntry("LayoutLeftMargin", Worksheet::convertToSceneUnits(0.2f, Worksheet::Centimeter));
	d->layoutRightMargin = group.readEntry("LayoutRightMargin", Worksheet::convertToSceneUnits(0.2f, Worksheet::Centimeter));
	d->layoutVerticalSpacing = group.readEntry("LayoutVerticalSpacing", Worksheet::convertToSceneUnits(0.1f, Worksheet::Centimeter));
	d->layoutHorizontalSpacing = group.readEntry("LayoutHorizontalSpacing", Worksheet::convertToSceneUnits(0.1f, Worksheet::Centimeter));
	d->layoutColumnCount = group.readEntry("LayoutColumnCount", 1);

	graphicsItem()->setFlag(QGraphicsItem::ItemIsSelectable, true);
	graphicsItem()->setFlag(QGraphicsItem::ItemIsMovable);
	graphicsItem()->setFlag(QGraphicsItem::ItemSendsGeometryChanges);

	this->initActions();
}

void CartesianPlotLegend::initActions() {
	visibilityAction = new QAction(i18n("Visible"), this);
	visibilityAction->setCheckable(true);
	connect(visibilityAction, &QAction::triggered, this, &CartesianPlotLegend::visibilityChanged);
}

QMenu* CartesianPlotLegend::createContextMenu() {
	QMenu *menu = WorksheetElement::createContextMenu();
	QAction* firstAction = menu->actions().at(1); //skip the first action because of the "title-action"

	visibilityAction->setChecked(isVisible());
	menu->insertAction(firstAction, visibilityAction);

	return menu;
}

/*!
	Returns an icon to be used in the project explorer.
*/
QIcon CartesianPlotLegend::icon() const{
	return QIcon::fromTheme("text-field");
}

STD_SWAP_METHOD_SETTER_CMD_IMPL(CartesianPlotLegend, SetVisible, bool, swapVisible)
void CartesianPlotLegend::setVisible(bool on) {
	Q_D(CartesianPlotLegend);
	exec(new CartesianPlotLegendSetVisibleCmd(d, on, on ? ki18n("%1: set visible") : ki18n("%1: set invisible")));
}

bool CartesianPlotLegend::isVisible() const{
	Q_D(const CartesianPlotLegend);
	return d->isVisible();
}

void CartesianPlotLegend::setPrinting(bool on) {
	Q_D(CartesianPlotLegend);
	d->m_printing = on;
}

QGraphicsItem *CartesianPlotLegend::graphicsItem() const{
	return d_ptr;
}

void CartesianPlotLegend::retransform() {
	d_ptr->retransform();
}

void CartesianPlotLegend::handleResize(double horizontalRatio, double verticalRatio, bool pageResize) {
	Q_UNUSED(horizontalRatio);
	Q_UNUSED(verticalRatio);
	Q_UNUSED(pageResize);
	//TODO
// 	Q_D(const CartesianPlotLegend);
}

//##############################################################################
//################################  getter methods  ############################
//##############################################################################
CLASS_SHARED_D_READER_IMPL(CartesianPlotLegend, QFont, labelFont, labelFont)
CLASS_SHARED_D_READER_IMPL(CartesianPlotLegend, QColor, labelColor, labelColor)
BASIC_SHARED_D_READER_IMPL(CartesianPlotLegend, bool, labelColumnMajor, labelColumnMajor)
CLASS_SHARED_D_READER_IMPL(CartesianPlotLegend, CartesianPlotLegend::PositionWrapper, position, position)
BASIC_SHARED_D_READER_IMPL(CartesianPlotLegend, qreal, rotationAngle, rotationAngle)
BASIC_SHARED_D_READER_IMPL(CartesianPlotLegend, float, lineSymbolWidth, lineSymbolWidth)

//Title
TextLabel* CartesianPlotLegend::title() {
	return d_ptr->title;
}

//Background
BASIC_SHARED_D_READER_IMPL(CartesianPlotLegend, PlotArea::BackgroundType, backgroundType, backgroundType)
BASIC_SHARED_D_READER_IMPL(CartesianPlotLegend, PlotArea::BackgroundColorStyle, backgroundColorStyle, backgroundColorStyle)
BASIC_SHARED_D_READER_IMPL(CartesianPlotLegend, PlotArea::BackgroundImageStyle, backgroundImageStyle, backgroundImageStyle)
BASIC_SHARED_D_READER_IMPL(CartesianPlotLegend, Qt::BrushStyle, backgroundBrushStyle, backgroundBrushStyle)
CLASS_SHARED_D_READER_IMPL(CartesianPlotLegend, QColor, backgroundFirstColor, backgroundFirstColor)
CLASS_SHARED_D_READER_IMPL(CartesianPlotLegend, QColor, backgroundSecondColor, backgroundSecondColor)
CLASS_SHARED_D_READER_IMPL(CartesianPlotLegend, QString, backgroundFileName, backgroundFileName)
BASIC_SHARED_D_READER_IMPL(CartesianPlotLegend, float, backgroundOpacity, backgroundOpacity)

//Border
CLASS_SHARED_D_READER_IMPL(CartesianPlotLegend, QPen, borderPen, borderPen)
BASIC_SHARED_D_READER_IMPL(CartesianPlotLegend, float, borderCornerRadius, borderCornerRadius)
BASIC_SHARED_D_READER_IMPL(CartesianPlotLegend, float, borderOpacity, borderOpacity)

//Layout
BASIC_SHARED_D_READER_IMPL(CartesianPlotLegend, float, layoutTopMargin, layoutTopMargin)
BASIC_SHARED_D_READER_IMPL(CartesianPlotLegend, float, layoutBottomMargin, layoutBottomMargin)
BASIC_SHARED_D_READER_IMPL(CartesianPlotLegend, float, layoutLeftMargin, layoutLeftMargin)
BASIC_SHARED_D_READER_IMPL(CartesianPlotLegend, float, layoutRightMargin, layoutRightMargin)
BASIC_SHARED_D_READER_IMPL(CartesianPlotLegend, float, layoutHorizontalSpacing, layoutHorizontalSpacing)
BASIC_SHARED_D_READER_IMPL(CartesianPlotLegend, float, layoutVerticalSpacing, layoutVerticalSpacing)
BASIC_SHARED_D_READER_IMPL(CartesianPlotLegend, int, layoutColumnCount, layoutColumnCount)

//##############################################################################
//######################  setter methods and undo commands  ####################
//##############################################################################
STD_SETTER_CMD_IMPL_F_S(CartesianPlotLegend, SetLabelFont, QFont, labelFont, retransform)
void CartesianPlotLegend::setLabelFont(const QFont& font) {
	Q_D(CartesianPlotLegend);
	if (font!= d->labelFont)
		exec(new CartesianPlotLegendSetLabelFontCmd(d, font, ki18n("%1: set font")));
}

STD_SETTER_CMD_IMPL_F_S(CartesianPlotLegend, SetLabelColor, QColor, labelColor, update)
void CartesianPlotLegend::setLabelColor(const QColor& color) {
	Q_D(CartesianPlotLegend);
	if (color!= d->labelColor)
		exec(new CartesianPlotLegendSetLabelColorCmd(d, color, ki18n("%1: set font color")));
}

STD_SETTER_CMD_IMPL_F_S(CartesianPlotLegend, SetLabelColumnMajor, bool, labelColumnMajor, retransform)
void CartesianPlotLegend::setLabelColumnMajor(bool columnMajor) {
	Q_D(CartesianPlotLegend);
	if (columnMajor != d->labelColumnMajor)
		exec(new CartesianPlotLegendSetLabelColumnMajorCmd(d, columnMajor, ki18n("%1: change column order")));
}

STD_SETTER_CMD_IMPL_F_S(CartesianPlotLegend, SetLineSymbolWidth, float, lineSymbolWidth, retransform)
void CartesianPlotLegend::setLineSymbolWidth(float width) {
	Q_D(CartesianPlotLegend);
	if (width != d->lineSymbolWidth)
		exec(new CartesianPlotLegendSetLineSymbolWidthCmd(d, width, ki18n("%1: change line+symbol width")));
}

STD_SETTER_CMD_IMPL_F_S(CartesianPlotLegend, SetPosition, CartesianPlotLegend::PositionWrapper, position, updatePosition);
void CartesianPlotLegend::setPosition(const PositionWrapper& pos) {
	Q_D(CartesianPlotLegend);
	if (pos.point != d->position.point
		|| pos.horizontalPosition != d->position.horizontalPosition
		|| pos.verticalPosition != d->position.verticalPosition)
		exec(new CartesianPlotLegendSetPositionCmd(d, pos, ki18n("%1: set position")));
}

STD_SETTER_CMD_IMPL_F_S(CartesianPlotLegend, SetRotationAngle, qreal, rotationAngle, retransform)
void CartesianPlotLegend::setRotationAngle(qreal angle) {
	Q_D(CartesianPlotLegend);
	if (angle != d->rotationAngle) {
		exec(new CartesianPlotLegendSetRotationAngleCmd(d, angle, ki18n("%1: set rotation angle")));
		d->title->setRotationAngle(angle);
	}
}

//Background
STD_SETTER_CMD_IMPL_F_S(CartesianPlotLegend, SetBackgroundType, PlotArea::BackgroundType, backgroundType, update)
void CartesianPlotLegend::setBackgroundType(PlotArea::BackgroundType type) {
	Q_D(CartesianPlotLegend);
	if (type != d->backgroundType)
		exec(new CartesianPlotLegendSetBackgroundTypeCmd(d, type, ki18n("%1: background type changed")));
}

STD_SETTER_CMD_IMPL_F_S(CartesianPlotLegend, SetBackgroundColorStyle, PlotArea::BackgroundColorStyle, backgroundColorStyle, update)
void CartesianPlotLegend::setBackgroundColorStyle(PlotArea::BackgroundColorStyle style) {
	Q_D(CartesianPlotLegend);
	if (style != d->backgroundColorStyle)
		exec(new CartesianPlotLegendSetBackgroundColorStyleCmd(d, style, ki18n("%1: background color style changed")));
}

STD_SETTER_CMD_IMPL_F_S(CartesianPlotLegend, SetBackgroundImageStyle, PlotArea::BackgroundImageStyle, backgroundImageStyle, update)
void CartesianPlotLegend::setBackgroundImageStyle(PlotArea::BackgroundImageStyle style) {
	Q_D(CartesianPlotLegend);
	if (style != d->backgroundImageStyle)
		exec(new CartesianPlotLegendSetBackgroundImageStyleCmd(d, style, ki18n("%1: background image style changed")));
}

STD_SETTER_CMD_IMPL_F_S(CartesianPlotLegend, SetBackgroundBrushStyle, Qt::BrushStyle, backgroundBrushStyle, update)
void CartesianPlotLegend::setBackgroundBrushStyle(Qt::BrushStyle style) {
	Q_D(CartesianPlotLegend);
	if (style != d->backgroundBrushStyle)
		exec(new CartesianPlotLegendSetBackgroundBrushStyleCmd(d, style, ki18n("%1: background brush style changed")));
}

STD_SETTER_CMD_IMPL_F_S(CartesianPlotLegend, SetBackgroundFirstColor, QColor, backgroundFirstColor, update)
void CartesianPlotLegend::setBackgroundFirstColor(const QColor &color) {
	Q_D(CartesianPlotLegend);
	if (color!= d->backgroundFirstColor)
		exec(new CartesianPlotLegendSetBackgroundFirstColorCmd(d, color, ki18n("%1: set background first color")));
}

STD_SETTER_CMD_IMPL_F_S(CartesianPlotLegend, SetBackgroundSecondColor, QColor, backgroundSecondColor, update)
void CartesianPlotLegend::setBackgroundSecondColor(const QColor &color) {
	Q_D(CartesianPlotLegend);
	if (color!= d->backgroundSecondColor)
		exec(new CartesianPlotLegendSetBackgroundSecondColorCmd(d, color, ki18n("%1: set background second color")));
}

STD_SETTER_CMD_IMPL_F_S(CartesianPlotLegend, SetBackgroundFileName, QString, backgroundFileName, update)
void CartesianPlotLegend::setBackgroundFileName(const QString& fileName) {
	Q_D(CartesianPlotLegend);
	if (fileName!= d->backgroundFileName)
		exec(new CartesianPlotLegendSetBackgroundFileNameCmd(d, fileName, ki18n("%1: set background image")));
}

STD_SETTER_CMD_IMPL_F_S(CartesianPlotLegend, SetBackgroundOpacity, float, backgroundOpacity, update)
void CartesianPlotLegend::setBackgroundOpacity(float opacity) {
	Q_D(CartesianPlotLegend);
	if (opacity != d->backgroundOpacity)
		exec(new CartesianPlotLegendSetBackgroundOpacityCmd(d, opacity, ki18n("%1: set opacity")));
}

//Border
STD_SETTER_CMD_IMPL_F_S(CartesianPlotLegend, SetBorderPen, QPen, borderPen, update)
void CartesianPlotLegend::setBorderPen(const QPen &pen) {
	Q_D(CartesianPlotLegend);
	if (pen != d->borderPen)
		exec(new CartesianPlotLegendSetBorderPenCmd(d, pen, ki18n("%1: set border style")));
}

STD_SETTER_CMD_IMPL_F_S(CartesianPlotLegend, SetBorderCornerRadius, qreal, borderCornerRadius, update)
void CartesianPlotLegend::setBorderCornerRadius(float radius) {
	Q_D(CartesianPlotLegend);
	if (radius != d->borderCornerRadius)
		exec(new CartesianPlotLegendSetBorderCornerRadiusCmd(d, radius, ki18n("%1: set border corner radius")));
}

STD_SETTER_CMD_IMPL_F_S(CartesianPlotLegend, SetBorderOpacity, qreal, borderOpacity, update)
void CartesianPlotLegend::setBorderOpacity(float opacity) {
	Q_D(CartesianPlotLegend);
	if (opacity != d->borderOpacity)
		exec(new CartesianPlotLegendSetBorderOpacityCmd(d, opacity, ki18n("%1: set border opacity")));
}

//Layout
STD_SETTER_CMD_IMPL_F_S(CartesianPlotLegend, SetLayoutTopMargin, float, layoutTopMargin, retransform)
void CartesianPlotLegend::setLayoutTopMargin(float margin) {
	Q_D(CartesianPlotLegend);
	if (margin != d->layoutTopMargin)
		exec(new CartesianPlotLegendSetLayoutTopMarginCmd(d, margin, ki18n("%1: set layout top margin")));
}

STD_SETTER_CMD_IMPL_F_S(CartesianPlotLegend, SetLayoutBottomMargin, float, layoutBottomMargin, retransform)
void CartesianPlotLegend::setLayoutBottomMargin(float margin) {
	Q_D(CartesianPlotLegend);
	if (margin != d->layoutBottomMargin)
		exec(new CartesianPlotLegendSetLayoutBottomMarginCmd(d, margin, ki18n("%1: set layout bottom margin")));
}

STD_SETTER_CMD_IMPL_F_S(CartesianPlotLegend, SetLayoutLeftMargin, float, layoutLeftMargin, retransform)
void CartesianPlotLegend::setLayoutLeftMargin(float margin) {
	Q_D(CartesianPlotLegend);
	if (margin != d->layoutLeftMargin)
		exec(new CartesianPlotLegendSetLayoutLeftMarginCmd(d, margin, ki18n("%1: set layout left margin")));
}

STD_SETTER_CMD_IMPL_F_S(CartesianPlotLegend, SetLayoutRightMargin, float, layoutRightMargin, retransform)
void CartesianPlotLegend::setLayoutRightMargin(float margin) {
	Q_D(CartesianPlotLegend);
	if (margin != d->layoutRightMargin)
		exec(new CartesianPlotLegendSetLayoutRightMarginCmd(d, margin, ki18n("%1: set layout right margin")));
}

STD_SETTER_CMD_IMPL_F_S(CartesianPlotLegend, SetLayoutVerticalSpacing, float, layoutVerticalSpacing, retransform)
void CartesianPlotLegend::setLayoutVerticalSpacing(float spacing) {
	Q_D(CartesianPlotLegend);
	if (spacing != d->layoutVerticalSpacing)
		exec(new CartesianPlotLegendSetLayoutVerticalSpacingCmd(d, spacing, ki18n("%1: set layout vertical spacing")));
}

STD_SETTER_CMD_IMPL_F_S(CartesianPlotLegend, SetLayoutHorizontalSpacing, float, layoutHorizontalSpacing, retransform)
void CartesianPlotLegend::setLayoutHorizontalSpacing(float spacing) {
	Q_D(CartesianPlotLegend);
	if (spacing != d->layoutHorizontalSpacing)
		exec(new CartesianPlotLegendSetLayoutHorizontalSpacingCmd(d, spacing, ki18n("%1: set layout horizontal spacing")));
}

STD_SETTER_CMD_IMPL_F_S(CartesianPlotLegend, SetLayoutColumnCount, int, layoutColumnCount, retransform)
void CartesianPlotLegend::setLayoutColumnCount(int count) {
	Q_D(CartesianPlotLegend);
	if (count != d->layoutColumnCount)
		exec(new CartesianPlotLegendSetLayoutColumnCountCmd(d, count, ki18n("%1: set layout column count")));
}

//##############################################################################
//#################################  SLOTS  ####################################
//##############################################################################


//##############################################################################
//######  SLOTs for changes triggered via QActions in the context menu  ########
//##############################################################################
void CartesianPlotLegend::visibilityChangedSlot() {
	Q_D(const CartesianPlotLegend);
	this->setVisible(!d->isVisible());
}

//##############################################################################
//######################### Private implementation #############################
//##############################################################################
CartesianPlotLegendPrivate::CartesianPlotLegendPrivate(CartesianPlotLegend *owner) : q(owner) {
	setAcceptHoverEvents(true);
}

QString CartesianPlotLegendPrivate::name() const {
	return q->name();
}

QRectF CartesianPlotLegendPrivate::boundingRect() const {
	if (rotationAngle != 0) {
		QMatrix matrix;
		matrix.rotate(-rotationAngle);
		return matrix.mapRect(rect);
	} else
		return rect;
}

void CartesianPlotLegendPrivate::contextMenuEvent(QGraphicsSceneContextMenuEvent* event) {
	q->createContextMenu()->exec(event->screenPos());
}

/*!
  Returns the shape of the CartesianPlotLegend as a QPainterPath in local coordinates
*/
QPainterPath CartesianPlotLegendPrivate::shape() const {
	QPainterPath path;
	if ( qFuzzyIsNull(borderCornerRadius) )
		path.addRect(rect);
	else
		path.addRoundedRect(rect, borderCornerRadius, borderCornerRadius);

	if (rotationAngle != 0) {
		QTransform trafo;
		trafo.rotate(-rotationAngle);
		path = trafo.map(path);
	}

	return path;
}

bool CartesianPlotLegendPrivate::swapVisible(bool on) {
	bool oldValue = isVisible();
	setVisible(on);
	emit q->visibilityChanged(on);
	return oldValue;
}

/*!
  recalculates the rectangular of the legend.
*/
void CartesianPlotLegendPrivate::retransform() {
	if (suppressRetransform)
		return;

	prepareGeometryChange();

	curvesList.clear();

	//add xy-curves
	for (auto* curve : q->m_plot->children<XYCurve>()) {
		if (curve && curve->isVisible())
			curvesList.push_back(curve);
	}

	//add histograms
	for (auto* hist : q->m_plot->children<Histogram>()) {
		if (hist && hist->isVisible())
			curvesList.push_back(hist);
	}

	int curveCount = curvesList.size();
	columnCount = (curveCount<layoutColumnCount) ? curveCount : layoutColumnCount;
	if (columnCount == 0) //no curves available
		rowCount = 0;
	else
		rowCount = ceil(double(curveCount)/double(columnCount));

	maxColumnTextWidths.clear();

	//determine the width of the legend
	QFontMetrics fm(labelFont);
	float w;
	float h = fm.ascent();

	float maxTextWidth = 0;
	float legendWidth = 0;
	int index;
	for (int c = 0; c < columnCount; ++c) {
		for (int r = 0; r < rowCount; ++r) {
			if (labelColumnMajor)
				index = c*rowCount + r;
			else
				index = r*columnCount + c;

			if ( index >= curveCount )
				break;

			const WorksheetElement* curve = dynamic_cast<WorksheetElement*>(curvesList.at(index));
			if (curve) {
				if (!curve->isVisible())
					continue;

				w = fm.width(curve->name());
				if (w>maxTextWidth)
					maxTextWidth = w;
			}
		}
		maxColumnTextWidths.append(maxTextWidth);
		legendWidth += maxTextWidth;
	}
	legendWidth += layoutLeftMargin + layoutRightMargin; //margins
	legendWidth += columnCount*lineSymbolWidth + layoutHorizontalSpacing; //width of the columns without the text
	legendWidth += (columnCount-1)*2*layoutHorizontalSpacing; //spacings between the columns
	if (title->isVisible() && !title->text().text.isEmpty()) {
		float titleWidth;
		if (rotationAngle == 0.0)
			titleWidth = title->graphicsItem()->boundingRect().width();
		else {
			QRectF rect = title->graphicsItem()->boundingRect();
			QMatrix matrix;
			matrix.rotate(-rotationAngle);
			rect = matrix.mapRect(rect);
			titleWidth = rect.width();
		}

		if (titleWidth > legendWidth)
			legendWidth = titleWidth;
	}

	//determine the height of the legend
	float legendHeight = layoutTopMargin + layoutBottomMargin; //margins
	legendHeight += rowCount*h; //height of the rows
	legendHeight += (rowCount-1)*layoutVerticalSpacing; //spacing between the rows
	if (title->isVisible() && !title->text().text.isEmpty()) {
		if (rotationAngle == 0.0)
			legendHeight += title->graphicsItem()->boundingRect().height(); //legend title
		else {
			QRectF rect = title->graphicsItem()->boundingRect();
			QMatrix matrix;
			matrix.rotate(-rotationAngle);
			rect = matrix.mapRect(rect);
			legendHeight += rect.height(); //legend title
		}
	}

	rect.setX(-legendWidth/2);
	rect.setY(-legendHeight/2);
	rect.setWidth(legendWidth);
	rect.setHeight(legendHeight);

	updatePosition();
}

/*!
	calculates the position of the legend, when the position relative to the parent was specified (left, right, etc.)
*/
void CartesianPlotLegendPrivate::updatePosition() {
	//position the legend relative to the actual plot size minus small offset
	//TODO: make the offset dependent on the size of axis ticks.
	const QRectF parentRect = q->m_plot->dataRect();
	float hOffset = Worksheet::convertToSceneUnits(10, Worksheet::Point);
	float vOffset = Worksheet::convertToSceneUnits(10, Worksheet::Point);

	if (position.horizontalPosition != CartesianPlotLegend::hPositionCustom) {
		if (position.horizontalPosition == CartesianPlotLegend::hPositionLeft)
			position.point.setX(parentRect.x() + rect.width()/2 + hOffset);
		else if (position.horizontalPosition == CartesianPlotLegend::hPositionCenter)
			position.point.setX(parentRect.x() + parentRect.width()/2);
		else if (position.horizontalPosition == CartesianPlotLegend::hPositionRight)
			position.point.setX(parentRect.x() + parentRect.width() - rect.width()/2 - hOffset);
	}

	if (position.verticalPosition != CartesianPlotLegend::vPositionCustom) {
		if (position.verticalPosition == CartesianPlotLegend::vPositionTop)
			position.point.setY(parentRect.y() + rect.height()/2 + vOffset);
		else if (position.verticalPosition == CartesianPlotLegend::vPositionCenter)
			position.point.setY(parentRect.y() + parentRect.height()/2);
		else if (position.verticalPosition == CartesianPlotLegend::vPositionBottom)
			position.point.setY(parentRect.y() + parentRect.height() -  rect.height()/2 -vOffset);
	}

	suppressItemChangeEvent = true;
	setPos(position.point);
	suppressItemChangeEvent = false;
	emit q->positionChanged(position);

	suppressRetransform = true;
	title->retransform();
	suppressRetransform = false;
}

/*!
  Reimplementation of QGraphicsItem::paint(). This function does the actual painting of the legend.
  \sa QGraphicsItem::paint().
*/
void CartesianPlotLegendPrivate::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
	Q_UNUSED(option);
	Q_UNUSED(widget);
	if (!isVisible())
		return;

	painter->save();

	painter->rotate(-rotationAngle);

	//draw the area
	painter->setOpacity(backgroundOpacity);
	painter->setPen(Qt::NoPen);
	if (backgroundType == PlotArea::Color) {
		switch (backgroundColorStyle) {
			case PlotArea::SingleColor:{
				painter->setBrush(QBrush(backgroundFirstColor));
				break;
			}
			case PlotArea::HorizontalLinearGradient:{
				QLinearGradient linearGrad(rect.topLeft(), rect.topRight());
				linearGrad.setColorAt(0, backgroundFirstColor);
				linearGrad.setColorAt(1, backgroundSecondColor);
				painter->setBrush(QBrush(linearGrad));
				break;
			}
			case PlotArea::VerticalLinearGradient:{
				QLinearGradient linearGrad(rect.topLeft(), rect.bottomLeft());
				linearGrad.setColorAt(0, backgroundFirstColor);
				linearGrad.setColorAt(1, backgroundSecondColor);
				painter->setBrush(QBrush(linearGrad));
				break;
			}
			case PlotArea::TopLeftDiagonalLinearGradient:{
				QLinearGradient linearGrad(rect.topLeft(), rect.bottomRight());
				linearGrad.setColorAt(0, backgroundFirstColor);
				linearGrad.setColorAt(1, backgroundSecondColor);
				painter->setBrush(QBrush(linearGrad));
				break;
			}
			case PlotArea::BottomLeftDiagonalLinearGradient:{
				QLinearGradient linearGrad(rect.bottomLeft(), rect.topRight());
				linearGrad.setColorAt(0, backgroundFirstColor);
				linearGrad.setColorAt(1, backgroundSecondColor);
				painter->setBrush(QBrush(linearGrad));
				break;
			}
			case PlotArea::RadialGradient:{
				QRadialGradient radialGrad(rect.center(), rect.width()/2);
				radialGrad.setColorAt(0, backgroundFirstColor);
				radialGrad.setColorAt(1, backgroundSecondColor);
				painter->setBrush(QBrush(radialGrad));
				break;
			}
		}
	} else if (backgroundType == PlotArea::Image) {
		if ( !backgroundFileName.trimmed().isEmpty() ) {
			QPixmap pix(backgroundFileName);
			switch (backgroundImageStyle) {
				case PlotArea::ScaledCropped:
					pix = pix.scaled(rect.size().toSize(),Qt::KeepAspectRatioByExpanding,Qt::SmoothTransformation);
					painter->drawPixmap(rect.topLeft(),pix);
					break;
				case PlotArea::Scaled:
					pix = pix.scaled(rect.size().toSize(),Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
					painter->drawPixmap(rect.topLeft(),pix);
					break;
				case PlotArea::ScaledAspectRatio:
					pix = pix.scaled(rect.size().toSize(),Qt::KeepAspectRatio,Qt::SmoothTransformation);
					painter->drawPixmap(rect.topLeft(),pix);
					break;
				case PlotArea::Centered:
					painter->drawPixmap(QPointF(rect.center().x()-pix.size().width()/2,rect.center().y()-pix.size().height()/2),pix);
					break;
				case PlotArea::Tiled:
					painter->drawTiledPixmap(rect,pix);
					break;
				case PlotArea::CenterTiled:
					painter->drawTiledPixmap(rect,pix,QPoint(rect.size().width()/2,rect.size().height()/2));
			}
		}
	} else if (backgroundType == PlotArea::Pattern) {
		painter->setBrush(QBrush(backgroundFirstColor,backgroundBrushStyle));
	}

	if ( qFuzzyIsNull(borderCornerRadius) )
		painter->drawRect(rect);
	else
		painter->drawRoundedRect(rect, borderCornerRadius, borderCornerRadius);

	//draw the border
	if (borderPen.style() != Qt::NoPen) {
		painter->setPen(borderPen);
		painter->setBrush(Qt::NoBrush);
		painter->setOpacity(borderOpacity);
		if ( qFuzzyIsNull(borderCornerRadius) )
			painter->drawRect(rect);
		else
			painter->drawRoundedRect(rect, borderCornerRadius, borderCornerRadius);
	}

	//draw curve's line+symbol and the names
	int curveCount = curvesList.size();
	QFontMetrics fm(labelFont);
	float h = fm.ascent();
	painter->setFont(labelFont);

	//translate to left upper conner of the bounding rect plus the layout offset and the height of the title
	painter->translate(-rect.width()/2+layoutLeftMargin, -rect.height()/2+layoutTopMargin);
	if (title->isVisible() && !title->text().text.isEmpty())
		painter->translate(0, title->graphicsItem()->boundingRect().height());

	painter->save();

	int index;
	for (int c = 0; c < columnCount; ++c) {
		for (int r = 0; r < rowCount; ++r) {
			if (labelColumnMajor)
				index = c*rowCount + r;
			else
				index = r*columnCount + c;

			if ( index >= curveCount )
				break;

			//draw the legend item for histogram (simple rectangular with the sizes of the ascent)
			const Histogram* hist = dynamic_cast<Histogram*>(curvesList.at(index));
			if (hist) {
				//use line's pen (histogram bars, envelope or drop lines) if available,
				if (hist->lineType() != Histogram::NoLine && hist->linePen() != Qt::NoPen) {
					painter->setOpacity(hist->lineOpacity());
					painter->setPen(hist->linePen());
				}

				//for the brush, use the histogram filling or symbols filling or no brush
				if (hist->fillingEnabled())
					painter->setBrush(QBrush(hist->fillingFirstColor(), hist->fillingBrushStyle()));
				else if (hist->symbolsStyle() != Symbol::NoSymbols)
					painter->setBrush(hist->symbolsBrush());
				else
					painter->setBrush(Qt::NoBrush);

				painter->translate(QPointF(lineSymbolWidth/2, h/2));
				painter->drawRect(QRectF(-h/2, -h/2, h, h));
				painter->translate(-QPointF(lineSymbolWidth/2, h/2));

				//curve's name
				painter->setPen(QPen(labelColor));
				painter->setOpacity(1.0);
				//TODO: support HTML text?
				painter->drawText(QPoint(lineSymbolWidth + layoutHorizontalSpacing, h), hist->name());
				painter->translate(0, layoutVerticalSpacing + h);
				continue;
			}

			//draw the legend item for histogram
			const XYCurve* curve = dynamic_cast<XYCurve*>(curvesList.at(index));

			//curve's line (painted at the half of the ascent size)
			if (curve->lineType() != XYCurve::NoLine) {
				painter->setPen(curve->linePen());
				painter->setOpacity(curve->lineOpacity());
				painter->drawLine(0, h/2, lineSymbolWidth, h/2);
			}

			//error bars
			if ( (curve->xErrorType() != XYCurve::NoError && curve->xErrorPlusColumn()) || (curve->yErrorType() != XYCurve::NoError && curve->yErrorPlusColumn()) ) {
				painter->setOpacity(curve->errorBarsOpacity());
				painter->setPen(curve->errorBarsPen());

				//curve's error bars for x
				float errorBarsSize = Worksheet::convertToSceneUnits(10, Worksheet::Point);
				if (curve->symbolsStyle()!=Symbol::NoSymbols && errorBarsSize<curve->symbolsSize()*1.4)
					errorBarsSize = curve->symbolsSize()*1.4;

				switch (curve->errorBarsType()) {
				case XYCurve::ErrorBarsSimple:
					//horiz. line
					if (curve->xErrorType() != XYCurve::NoError)
						painter->drawLine(lineSymbolWidth/2-errorBarsSize/2, h/2,
										lineSymbolWidth/2+errorBarsSize/2, h/2);
					//vert. line
					if (curve->yErrorType() != XYCurve::NoError)
						painter->drawLine(lineSymbolWidth/2, h/2-errorBarsSize/2,
										lineSymbolWidth/2, h/2+errorBarsSize/2);
					break;
				case XYCurve::ErrorBarsWithEnds:
					//horiz. line
					if (curve->xErrorType() != XYCurve::NoError) {
						painter->drawLine(lineSymbolWidth/2-errorBarsSize/2, h/2,
										lineSymbolWidth/2+errorBarsSize/2, h/2);

						//caps for the horiz. line
						painter->drawLine(lineSymbolWidth/2-errorBarsSize/2, h/2-errorBarsSize/4,
										lineSymbolWidth/2-errorBarsSize/2, h/2+errorBarsSize/4);
						painter->drawLine(lineSymbolWidth/2+errorBarsSize/2, h/2-errorBarsSize/4,
										lineSymbolWidth/2+errorBarsSize/2, h/2+errorBarsSize/4);
					}

					//vert. line
					if (curve->yErrorType() != XYCurve::NoError) {
						painter->drawLine(lineSymbolWidth/2, h/2-errorBarsSize/2,
										lineSymbolWidth/2, h/2+errorBarsSize/2);


						//caps for the vert. line
						painter->drawLine(lineSymbolWidth/2-errorBarsSize/4, h/2-errorBarsSize/2,
										lineSymbolWidth/2+errorBarsSize/4, h/2-errorBarsSize/2);
						painter->drawLine(lineSymbolWidth/2-errorBarsSize/4, h/2+errorBarsSize/2,
										lineSymbolWidth/2+errorBarsSize/4, h/2+errorBarsSize/2);
					}
					break;
				}
			}

			//curve's symbol
			if (curve->symbolsStyle()!=Symbol::NoSymbols) {
				painter->setOpacity(curve->symbolsOpacity());
				painter->setBrush(curve->symbolsBrush());
				painter->setPen(curve->symbolsPen());

				QPainterPath path = Symbol::pathFromStyle(curve->symbolsStyle());
				QTransform trafo;
				trafo.scale(curve->symbolsSize(), curve->symbolsSize());
				path = trafo.map(path);

				if (curve->symbolsRotationAngle() != 0) {
					trafo.reset();
					trafo.rotate(curve->symbolsRotationAngle());
					path = trafo.map(path);
				}

				painter->translate(QPointF(lineSymbolWidth/2, h/2));
				painter->drawPath(path);
				painter->translate(-QPointF(lineSymbolWidth/2, h/2));
			}

			//curve's name
			painter->setPen(QPen(labelColor));
			painter->setOpacity(1.0);
			//TODO: support HTML text?
			painter->drawText(QPoint(lineSymbolWidth+layoutHorizontalSpacing, h), curve->name());
			painter->translate(0,layoutVerticalSpacing+h);
		}

		//translate to the beginning of the next column
		painter->restore();
		int deltaX = lineSymbolWidth+layoutHorizontalSpacing+maxColumnTextWidths.at(c); //the width of the current columns
		deltaX += 2*layoutHorizontalSpacing; //spacing between two columns
		painter->translate(deltaX,0);
		painter->save();
	}

	painter->restore();
	painter->restore();

	if (m_hovered && !isSelected() && !m_printing) {
		painter->setPen(QPen(QApplication::palette().color(QPalette::Shadow), 2, Qt::SolidLine));
		painter->drawPath(shape());
	}

	if (isSelected() && !m_printing) {
		painter->setPen(QPen(QApplication::palette().color(QPalette::Highlight), 2, Qt::SolidLine));
		painter->drawPath(shape());
	}
}

QVariant CartesianPlotLegendPrivate::itemChange(GraphicsItemChange change, const QVariant &value) {
	if (suppressItemChangeEvent)
		return value;

	if (change == QGraphicsItem::ItemPositionChange) {
		//convert item's center point in parent's coordinates
		CartesianPlotLegend::PositionWrapper tempPosition;
			tempPosition.point = value.toPointF();
			tempPosition.horizontalPosition = CartesianPlotLegend::hPositionCustom;
			tempPosition.verticalPosition = CartesianPlotLegend::vPositionCustom;

		//emit the signals in order to notify the UI.
		//we don't set the position related member variables during the mouse movements.
		//this is done on mouse release events only.
		emit q->positionChanged(tempPosition);
	 }

	return QGraphicsItem::itemChange(change, value);
}

void CartesianPlotLegendPrivate::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
	//convert position of the item in parent coordinates to label's position
	QPointF point = pos();
	if (point != position.point) {
		//position was changed -> set the position related member variables
		suppressRetransform = true;
		CartesianPlotLegend::PositionWrapper tempPosition;
		tempPosition.point = point;
		tempPosition.horizontalPosition = CartesianPlotLegend::hPositionCustom;
		tempPosition.verticalPosition = CartesianPlotLegend::vPositionCustom;
		q->setPosition(tempPosition);
		suppressRetransform = false;
	}

	QGraphicsItem::mouseReleaseEvent(event);
}

void CartesianPlotLegendPrivate::hoverEnterEvent(QGraphicsSceneHoverEvent*) {
	if (!isSelected()) {
		m_hovered = true;
		emit q->hovered();
		update();
	}
}

void CartesianPlotLegendPrivate::hoverLeaveEvent(QGraphicsSceneHoverEvent*) {
	if (m_hovered) {
		m_hovered = false;
		emit q->unhovered();
		update();
	}
}

//##############################################################################
//##################  Serialization/Deserialization  ###########################
//##############################################################################
//! Save as XML
void CartesianPlotLegend::save(QXmlStreamWriter* writer) const {
	Q_D(const CartesianPlotLegend);

	writer->writeStartElement( "cartesianPlotLegend" );
	writeBasicAttributes( writer );
	writeCommentElement( writer );

	//general
	writer->writeStartElement( "general" );
	WRITE_QCOLOR(d->labelColor);
	WRITE_QFONT(d->labelFont);
	writer->writeAttribute( "columnMajor", QString::number(d->labelColumnMajor) );
	writer->writeAttribute( "lineSymbolWidth", QString::number(d->lineSymbolWidth) );
	writer->writeAttribute( "visible", QString::number(d->isVisible()) );
	writer->writeEndElement();

	//geometry
	writer->writeStartElement( "geometry" );
	writer->writeAttribute( "x", QString::number(d->position.point.x()) );
	writer->writeAttribute( "y", QString::number(d->position.point.y()) );
	writer->writeAttribute( "horizontalPosition", QString::number(d->position.horizontalPosition) );
	writer->writeAttribute( "verticalPosition", QString::number(d->position.verticalPosition) );
	writer->writeAttribute( "rotation", QString::number(d->rotationAngle) );
	writer->writeEndElement();

	//title
	d->title->save(writer);

	//background
	writer->writeStartElement( "background" );
	writer->writeAttribute( "type", QString::number(d->backgroundType) );
	writer->writeAttribute( "colorStyle", QString::number(d->backgroundColorStyle) );
	writer->writeAttribute( "imageStyle", QString::number(d->backgroundImageStyle) );
	writer->writeAttribute( "brushStyle", QString::number(d->backgroundBrushStyle) );
	writer->writeAttribute( "firstColor_r", QString::number(d->backgroundFirstColor.red()) );
	writer->writeAttribute( "firstColor_g", QString::number(d->backgroundFirstColor.green()) );
	writer->writeAttribute( "firstColor_b", QString::number(d->backgroundFirstColor.blue()) );
	writer->writeAttribute( "secondColor_r", QString::number(d->backgroundSecondColor.red()) );
	writer->writeAttribute( "secondColor_g", QString::number(d->backgroundSecondColor.green()) );
	writer->writeAttribute( "secondColor_b", QString::number(d->backgroundSecondColor.blue()) );
	writer->writeAttribute( "fileName", d->backgroundFileName );
	writer->writeAttribute( "opacity", QString::number(d->backgroundOpacity) );
	writer->writeEndElement();

	//border
	writer->writeStartElement( "border" );
	WRITE_QPEN(d->borderPen);
	writer->writeAttribute( "borderOpacity", QString::number(d->borderOpacity) );
	writer->writeEndElement();

	//layout
	writer->writeStartElement( "layout" );
	writer->writeAttribute( "topMargin", QString::number(d->layoutTopMargin) );
	writer->writeAttribute( "bottomMargin", QString::number(d->layoutBottomMargin) );
	writer->writeAttribute( "leftMargin", QString::number(d->layoutLeftMargin) );
	writer->writeAttribute( "rightMargin", QString::number(d->layoutRightMargin) );
	writer->writeAttribute( "verticalSpacing", QString::number(d->layoutVerticalSpacing) );
	writer->writeAttribute( "horizontalSpacing", QString::number(d->layoutHorizontalSpacing) );
	writer->writeAttribute( "columnCount", QString::number(d->layoutColumnCount) );
	writer->writeEndElement();

	writer->writeEndElement(); // close "cartesianPlotLegend" section
}

//! Load from XML
bool CartesianPlotLegend::load(XmlStreamReader* reader, bool preview) {
	Q_D(CartesianPlotLegend);

	if (!readBasicAttributes(reader))
		return false;

	KLocalizedString attributeWarning = ki18n("Attribute '%1' missing or empty, default value is used");
	QXmlStreamAttributes attribs;
	QString str;

	while (!reader->atEnd()) {
		reader->readNext();
		if (reader->isEndElement() && reader->name() == "cartesianPlotLegend")
			break;

		if (!reader->isStartElement())
			continue;

		if (!preview && reader->name() == "comment") {
			if (!readCommentElement(reader)) return false;
		} else if (!preview && reader->name() == "general") {
			attribs = reader->attributes();

			READ_QCOLOR(d->labelColor);
			READ_QFONT(d->labelFont);

			str = attribs.value("columnMajor").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.subs("columnMajor").toString());
			else
				d->labelColumnMajor = str.toInt();

			str = attribs.value("lineSymbolWidth").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.subs("lineSymbolWidth").toString());
			else
				d->lineSymbolWidth = str.toDouble();

			str = attribs.value("visible").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.subs("visible").toString());
			else
				d->setVisible(str.toInt());
		} else if (!preview && reader->name() == "geometry") {
			attribs = reader->attributes();

			str = attribs.value("x").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.subs("x").toString());
			else
				d->position.point.setX(str.toDouble());

			str = attribs.value("y").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.subs("y").toString());
			else
				d->position.point.setY(str.toDouble());

			str = attribs.value("horizontalPosition").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.subs("horizontalPosition").toString());
			else
				d->position.horizontalPosition = (CartesianPlotLegend::HorizontalPosition)str.toInt();

			str = attribs.value("verticalPosition").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.subs("verticalPosition").toString());
			else
				d->position.verticalPosition = (CartesianPlotLegend::VerticalPosition)str.toInt();

			READ_DOUBLE_VALUE("rotation", rotationAngle);
		} else if (reader->name() == "textLabel") {
			if (!d->title->load(reader, preview)) {
				delete d->title;
				d->title = nullptr;
				return false;
			}
		} else if (!preview && reader->name() == "background") {
			attribs = reader->attributes();

			str = attribs.value("type").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.subs("type").toString());
			else
				d->backgroundType = PlotArea::BackgroundType(str.toInt());

			str = attribs.value("colorStyle").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.subs("colorStyle").toString());
			else
				d->backgroundColorStyle = PlotArea::BackgroundColorStyle(str.toInt());

			str = attribs.value("imageStyle").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.subs("imageStyle").toString());
			else
				d->backgroundImageStyle = PlotArea::BackgroundImageStyle(str.toInt());

			str = attribs.value("brushStyle").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.subs("brushStyle").toString());
			else
				d->backgroundBrushStyle = Qt::BrushStyle(str.toInt());

			str = attribs.value("firstColor_r").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.subs("firstColor_r").toString());
			else
				d->backgroundFirstColor.setRed(str.toInt());

			str = attribs.value("firstColor_g").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.subs("firstColor_g").toString());
			else
				d->backgroundFirstColor.setGreen(str.toInt());

			str = attribs.value("firstColor_b").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.subs("firstColor_b").toString());
			else
				d->backgroundFirstColor.setBlue(str.toInt());

			str = attribs.value("secondColor_r").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.subs("secondColor_r").toString());
			else
				d->backgroundSecondColor.setRed(str.toInt());

			str = attribs.value("secondColor_g").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.subs("secondColor_g").toString());
			else
				d->backgroundSecondColor.setGreen(str.toInt());

			str = attribs.value("secondColor_b").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.subs("secondColor_b").toString());
			else
				d->backgroundSecondColor.setBlue(str.toInt());

			str = attribs.value("fileName").toString();
			d->backgroundFileName = str;

			str = attribs.value("opacity").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.subs("opacity").toString());
			else
				d->backgroundOpacity = str.toDouble();
		} else if (!preview && reader->name() == "border") {
			attribs = reader->attributes();

			READ_QPEN(d->borderPen);

			str = attribs.value("borderOpacity").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.subs("borderOpacity").toString());
			else
				d->borderOpacity = str.toDouble();
		} else if (!preview && reader->name() == "layout") {
			attribs = reader->attributes();

			str = attribs.value("topMargin").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.subs("topMargin").toString());
			else
				d->layoutTopMargin = str.toDouble();

			str = attribs.value("bottomMargin").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.subs("bottomMargin").toString());
			else
				d->layoutBottomMargin = str.toDouble();

			str = attribs.value("leftMargin").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.subs("leftMargin").toString());
			else
				d->layoutLeftMargin = str.toDouble();

			str = attribs.value("rightMargin").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.subs("rightMargin").toString());
			else
				d->layoutRightMargin = str.toDouble();

			str = attribs.value("verticalSpacing").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.subs("verticalSpacing").toString());
			else
				d->layoutVerticalSpacing = str.toDouble();

			str = attribs.value("horizontalSpacing").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.subs("horizontalSpacing").toString());
			else
				d->layoutHorizontalSpacing = str.toDouble();

			str = attribs.value("columnCount").toString();
			if (str.isEmpty())
				reader->raiseWarning(attributeWarning.subs("columnCount").toString());
			else
				d->layoutColumnCount = str.toInt();
		}
	}

	return true;
}

void CartesianPlotLegend::loadThemeConfig(const KConfig& config) {
	KConfigGroup groupLabel = config.group("Label");
	this->setLabelColor(groupLabel.readEntry("FontColor", QColor(Qt::white)));

	const KConfigGroup group = config.group("CartesianPlot");
	this->setBackgroundBrushStyle((Qt::BrushStyle)group.readEntry("BackgroundBrushStyle",(int) this->backgroundBrushStyle()));
	this->setBackgroundColorStyle((PlotArea::BackgroundColorStyle)(group.readEntry("BackgroundColorStyle",(int) this->backgroundColorStyle())));
	this->setBackgroundFirstColor(group.readEntry("BackgroundFirstColor",(QColor) this->backgroundFirstColor()));
	this->setBackgroundImageStyle((PlotArea::BackgroundImageStyle)group.readEntry("BackgroundImageStyle",(int) this->backgroundImageStyle()));
	this->setBackgroundOpacity(group.readEntry("BackgroundOpacity", this->backgroundOpacity()));
	this->setBackgroundSecondColor(group.readEntry("BackgroundSecondColor",(QColor) this->backgroundSecondColor()));
	this->setBackgroundType((PlotArea::BackgroundType)(group.readEntry("BackgroundType",(int) this->backgroundType())));

	QPen pen = this->borderPen();
	pen.setColor(group.readEntry("BorderColor", pen.color()));
	pen.setStyle((Qt::PenStyle)(group.readEntry("BorderStyle", (int) pen.style())));
	pen.setWidthF(group.readEntry("BorderWidth", pen.widthF()));
	this->setBorderPen(pen);
	this->setBorderCornerRadius(group.readEntry("BorderCornerRadius", this->borderCornerRadius()));
	this->setBorderOpacity(group.readEntry("BorderOpacity", this->borderOpacity()));

	title()->loadThemeConfig(config);
}
