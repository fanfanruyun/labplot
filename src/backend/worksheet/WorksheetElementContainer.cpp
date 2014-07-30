/***************************************************************************
    File                 : WorksheetElementContainer.cpp
    Project              : LabPlot/SciDAVis
    Description          : Generic WorksheetElement container.
    --------------------------------------------------------------------
    Copyright            : (C) 2009 Tilman Benkert (thzs*gmx.net)
    Copyright            : (C) 2012 by Alexander Semke (alexander.semke*web.de)
                           (replace * with @ in the email addresses)

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

#include "backend/worksheet/WorksheetElementContainer.h"
#include "backend/worksheet/WorksheetElementContainerPrivate.h"
#include "backend/lib/commandtemplates.h"
#include "backend/lib/macros.h"

#include <QtDebug>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QGraphicsSceneContextMenuEvent>
#include <QMenu>
#include <QPainter>

#include <KLocale>

/**
 * \class WorksheetElementContainer
 * \brief Generic AbstractWorksheetElement container.
 * \ingroup worksheet
 * This class combines functionality of worksheet element containers
 * such as groups, plots etc.
 *
 */

WorksheetElementContainer::WorksheetElementContainer(const QString &name)
	: AbstractWorksheetElement(name), d_ptr(new WorksheetElementContainerPrivate(this)) {

	connect(this, SIGNAL(aspectAdded(const AbstractAspect*)),
		this, SLOT(handleAspectAdded(const AbstractAspect*)));
}

WorksheetElementContainer::WorksheetElementContainer(const QString &name, WorksheetElementContainerPrivate *dd)
    : AbstractWorksheetElement(name), d_ptr(dd) {

	connect(this, SIGNAL(aspectAdded(const AbstractAspect*)),
		this, SLOT(handleAspectAdded(const AbstractAspect*)));
}

WorksheetElementContainer::~WorksheetElementContainer(){
	//no need to delete the d-pointer here - it inherits from QGraphicsItem
	//and is deleted during the cleanup in QGraphicsScene
}

QGraphicsItem *WorksheetElementContainer::graphicsItem() const {
	return const_cast<QGraphicsItem *>(static_cast<const QGraphicsItem *>(d_ptr));
}

QRectF WorksheetElementContainer::rect() const{
	Q_D(const WorksheetElementContainer);
	return d->rect;
}

STD_SWAP_METHOD_SETTER_CMD_IMPL(WorksheetElementContainer, SetVisible, bool, swapVisible)
void WorksheetElementContainer::setVisible(bool on){
	Q_D(WorksheetElementContainer);

	beginMacro(on ? i18n("%1: set visible", name()) : i18n("%1: set invisible", name()));

	//take care of proper ordering on the undo-stack,
	//when making the container and all its children visible/invisible.
	//if visible is set true, change the visibility of the container first
	if (on)
    	exec(new WorksheetElementContainerSetVisibleCmd(d, on, on ? i18n("%1: set visible") : i18n("%1: set invisible")));

	//change the visibility of all children
	QList<AbstractWorksheetElement *> childList = children<AbstractWorksheetElement>(AbstractAspect::IncludeHidden | AbstractAspect::Compress);
	foreach(AbstractWorksheetElement *elem, childList){
		elem->setVisible(on);
	}

	//if visible is set false, change the visibility of the container last
	if (!on)
    	exec(new WorksheetElementContainerSetVisibleCmd(d, on, on ? i18n("%1: set visible") : i18n("%1: set invisible")));

	endMacro();
}

bool WorksheetElementContainer::isVisible() const {
	Q_D(const WorksheetElementContainer);
	return d->isVisible();
}

bool WorksheetElementContainer::isFullyVisible() const {
	QList<AbstractWorksheetElement *> childList = children<AbstractWorksheetElement>(AbstractAspect::IncludeHidden | AbstractAspect::Compress);
	foreach(const AbstractWorksheetElement *elem, childList) {
		if (!elem->isVisible())
			return false;
	}
	return true;
}

void WorksheetElementContainer::setPrinting(bool on) {
	Q_D(WorksheetElementContainer);
	d->m_printing = on;
}

void WorksheetElementContainer::retransform() {
	QList<AbstractWorksheetElement *> childList = children<AbstractWorksheetElement>(AbstractAspect::IncludeHidden | AbstractAspect::Compress);
	foreach(AbstractWorksheetElement *elem, childList)
		elem->retransform();
}

void WorksheetElementContainer::handlePageResize(double horizontalRatio, double verticalRatio) {
	qDebug()<<"WorksheetElementContainer::handlePageResize";
	Q_D(WorksheetElementContainer);
	QRectF rect(d->rect);
	rect.setWidth(d->rect.width()*horizontalRatio);
	rect.setHeight(d->rect.height()*verticalRatio);
	setRect(rect);
}

void WorksheetElementContainer::handleAspectAdded(const AbstractAspect *aspect) {
	qDebug()<<"WorksheetElementContainer::handleAspectAdded "<< aspect->name();
	Q_D(WorksheetElementContainer);

	const AbstractWorksheetElement* element = qobject_cast<const AbstractWorksheetElement*>(aspect);
	if (element && (aspect->parentAspect() == this)) {
		connect(element, SIGNAL(hovered()), this, SLOT(childHovered()));
		connect(element, SIGNAL(unhovered()), this, SLOT(childUnhovered()));
		QGraphicsItem *item = element->graphicsItem();
		Q_ASSERT(item != NULL);
		item->setParentItem(d);

		qreal zVal = 0;
		QList<AbstractWorksheetElement *> childElements = children<AbstractWorksheetElement>(IncludeHidden);
		foreach(AbstractWorksheetElement *elem, childElements) {
			elem->graphicsItem()->setZValue(zVal++);
		}
	}
}

void WorksheetElementContainer::childHovered() {
	Q_D(WorksheetElementContainer);
	if (d->m_hovered)
		d->m_hovered = false;
	d->update();
}

void WorksheetElementContainer::childUnhovered() {
	Q_D(WorksheetElementContainer);
	d->m_hovered = true;
	d->update();
}

//################################################################
//################### Private implementation ##########################
//################################################################
WorksheetElementContainerPrivate::WorksheetElementContainerPrivate(WorksheetElementContainer *owner)
	: q(owner), m_hovered(false), m_printing(false) {
	setAcceptHoverEvents(true);
}

QString WorksheetElementContainerPrivate::name() const {
	return q->name();
}

void WorksheetElementContainerPrivate::contextMenuEvent(QGraphicsSceneContextMenuEvent *event) {
	scene()->clearSelection();
	setSelected(true);
	QMenu* menu = q->createContextMenu();
	menu->exec(event->screenPos());
}

void WorksheetElementContainerPrivate::hoverLeaveEvent(QGraphicsSceneHoverEvent*) {
	m_hovered = false;
	update();
}

void WorksheetElementContainerPrivate::hoverEnterEvent(QGraphicsSceneHoverEvent*) {
	m_hovered = true;
	update();
}

bool WorksheetElementContainerPrivate::swapVisible(bool on){
	bool oldValue = isVisible();
	setVisible(on);
	emit q->visibleChanged(on);
	return oldValue;
}

// Inherited from QGraphicsItem
QRectF WorksheetElementContainerPrivate::boundingRect() const {
	QRectF rect;
	QList<AbstractWorksheetElement *> childList = q->children<AbstractWorksheetElement>(AbstractAspect::IncludeHidden | AbstractAspect::Compress);
	foreach(const AbstractWorksheetElement *elem, childList)
		rect |= elem->graphicsItem()->mapRectToParent( elem->graphicsItem()->boundingRect() );

	return rect;
}

// Inherited from QGraphicsItem
void WorksheetElementContainerPrivate::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
	qDebug()<<"WorksheetElementContainerPrivate::paint";
	Q_UNUSED(option)
	Q_UNUSED(widget)

	if (!isVisible())
		return;

	if (m_hovered && !isSelected() && !m_printing){
		painter->setPen(q->hoveredPen);
		painter->setOpacity(q->hoveredOpacity);
		painter->drawRect(boundingRect());
	}

	if (isSelected() && !m_printing){
		painter->setPen(q->selectedPen);
		painter->setOpacity(q->selectedOpacity);
		painter->drawRect(boundingRect());
  }
}
