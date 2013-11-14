/**********************************************************************
 *  Copyright (c) 2008-2013, Alliance for Sustainable Energy.  
 *  All rights reserved.
 *  
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *  
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *  
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 **********************************************************************/

#include "RefrigerationGraphicsItems.hpp"
#include "../utilities/core/Assert.hpp"
#include <QPainter>
#include <utility>
#include <QGraphicsSceneMouseEvent>

namespace openstudio {

const int RefrigerationSystemView::verticalSpacing = 50;

RefrigerationSystemGridView::RefrigerationSystemGridView()
{
}

QRectF RefrigerationSystemGridView::boundingRect() const
{
  int x = columns() * (RefrigerationSystemMiniView::cellSize().width() + spacing()) - spacing();
  int y = rows() * (RefrigerationSystemMiniView::cellSize().height() + spacing()) - spacing();

  return QRectF(0,0,x,y);
}

void RefrigerationSystemGridView::setDelegate(QSharedPointer<OSGraphicsItemDelegate> delegate)
{
  if( delegate )
  { 
    m_delegate = delegate;

    refreshAllItemViews();
  }
}

void RefrigerationSystemGridView::setListController(QSharedPointer<OSListController> listController)
{
  if( m_listController )
  {
    m_listController->disconnect(this);
  }

  m_listController = listController;

  connect(m_listController.data(),SIGNAL(itemInserted(int)),this,SLOT(insertItemView(int)));
  connect(m_listController.data(),SIGNAL(itemRemoved(int)),this,SLOT(removeItemView(int)));
  connect(m_listController.data(),SIGNAL(itemChanged(int)),this,SLOT(refreshItemView(int)));
  connect(m_listController.data(),SIGNAL(modelReset()),this,SLOT(refreshAllItemViews()));

  refreshAllItemViews();
}

QSharedPointer<OSListController> RefrigerationSystemGridView::listController() const
{
  return m_listController;
}

void RefrigerationSystemGridView::refreshAllItemViews()
{
  prepareGeometryChange();

  QList<QGraphicsItem *> itemList = childItems();
  for( QList<QGraphicsItem *>::iterator it = itemList.begin(); 
       it < itemList.end(); 
       it++ )
  {
    delete *it;
  }

  if( m_listController )
  {
    for( int i = 0; i < m_listController->count(); i++ )
    {
      insertItemView(i);
    }
  }
}

QGraphicsObject * RefrigerationSystemGridView::createNewItemView(int i)
{
  if( m_listController && m_delegate )
  {
    QSharedPointer<OSListItem> itemData = m_listController->itemAt(i);

    OS_ASSERT(itemData);

    QGraphicsObject * graphicsItem = m_delegate->view(itemData);

    OS_ASSERT(graphicsItem);

    graphicsItem->setParentItem(this);

    m_widgetItemPairs.insert( std::make_pair<QGraphicsObject *,QSharedPointer<OSListItem> >(graphicsItem,itemData) );

    bool bingo = connect(graphicsItem,SIGNAL(destroyed(QObject *)),this,SLOT(removePair(QObject *)));

    OS_ASSERT(bingo);

    return graphicsItem;
  }
  else
  {
    return NULL;
  }
}

void RefrigerationSystemGridView::setItemViewGridPos(QGraphicsObject * item,std::pair<int,int> gridPos)
{
  if( item )
  {
    int x = gridPos.second * (RefrigerationSystemMiniView::cellSize().width() + spacing());
    int y = gridPos.first * (RefrigerationSystemMiniView::cellSize().height() + spacing());

    item->setPos(x,y);

    //std::map<std::pair<int,int>,QObject *>::iterator it = m_gridPosItemViewPairs.find(gridPos)
    //if( it != m_gridPosItemViewPairs.end() )
    //{
    //  m_gridPosItemViewPairs.erase(it); 
    //}

    //std::map<QObject *,std::pair<int,int>::iterator it2 = m_itemViewGridPosPairs.find(item);
    //if( it2 != m_itemViewGridPosPairs.end() )
    //{
    //  m_itemViewGridPosPairs.erase(it2);
    //}

    //m_gridPosItemViewPairs.insert( std::make_pair<std::pair<int,int>,QObject *>(std::make_pair<int,int>(x,y),item) );

    //m_itemViewGridPosPairs.insert( std::make_pair<QObject *,std::pair<int,int> >(item,std::make_pair<int,int>(x,y)) );

    m_gridPosItemViewPairs[gridPos] = item;

    m_itemViewGridPosPairs[item] = gridPos;
  }
}

void RefrigerationSystemGridView::insertItemView(int index)
{
  if( m_listController )
  {
    prepareGeometryChange();

    // Move Everything after index forward one grid position
    for( int i = index + 1; i < m_listController->count(); i++ )
    {
      std::pair<int,int> oldPos = gridPos(i - 1);

      QGraphicsObject * item = viewFromGridPos(oldPos);

      std::pair<int,int> newPos = gridPos(i);

      setItemViewGridPos(item,newPos);
    }

    // Create new item and position it at index grid position
    if( QGraphicsObject * item = createNewItemView(index) )
    {
      std::pair<int,int> pos = gridPos(index);

      setItemViewGridPos(item,pos);
    }
  }
}

void RefrigerationSystemGridView::removeItemView(int index)
{
  if( m_listController )
  {
    prepareGeometryChange();

    // Remove Item
    if(QGraphicsObject * item = viewFromGridPos(gridPos(index)))
    {
      item->deleteLater();
    }

    // Move Everything after index back one grid position
    for( int i = index + 1; i < m_listController->count() + 1; i++ )
    {
      std::pair<int,int> oldPos = gridPos(i);

      QGraphicsObject * item = viewFromGridPos(oldPos);

      std::pair<int,int> newPos = gridPos(i - 1);

      setItemViewGridPos(item,newPos);
    }
  }
}

void RefrigerationSystemGridView::removePair(QObject * object)
{
  if( object )
  {
    std::map<QObject *,QSharedPointer<OSListItem> >::iterator it = m_widgetItemPairs.find(object);

    if( it != m_widgetItemPairs.end() )
    {
      m_widgetItemPairs.erase(it);
    }

    std::map<QObject *, std::pair<int,int> >::iterator it2 = m_itemViewGridPosPairs.find(object);

    std::pair<int,int> pos;

    if( it2 != m_itemViewGridPosPairs.end() )
    {
      m_itemViewGridPosPairs.erase(it2);

      pos = it2->second;

      std::map<std::pair<int,int>,QObject *>::iterator it3 = m_gridPosItemViewPairs.find(pos);

      if( it3 != m_gridPosItemViewPairs.end() )
      {
        m_gridPosItemViewPairs.erase(it3);
      }
    }
  }
}

void RefrigerationSystemGridView::refreshItemView(int i)
{
}

int RefrigerationSystemGridView::spacing() const
{
  return 25;
}

int RefrigerationSystemGridView::rows() const
{
  int result = 0;

  if( m_listController )
  {
    int count = m_listController->count();

    int _columns = columns();

    result = count / _columns;

    if( (count % _columns) != 0 )
    {
      result++;
    }
  }

  return result;
}

int RefrigerationSystemGridView::columns() const
{
  return 2;
}

std::pair<int,int> RefrigerationSystemGridView::gridPos(int index) 
{
  int row = 0;
  int column = 0;

  if( m_listController )
  {
    int _columns = columns();

    row = index / _columns;

    column = index % _columns;  
  }

  return std::pair<int,int>(row,column);
}

QGraphicsObject * RefrigerationSystemGridView::viewFromGridPos(std::pair<int,int> location)
{
  QGraphicsObject * result = NULL;

  std::map<std::pair<int,int>,QObject *>::iterator it = m_gridPosItemViewPairs.find(location);

  if( it != m_gridPosItemViewPairs.end() )
  {
    result = qobject_cast<QGraphicsObject *>(it->second);
  }

  return result;
}

RefrigerationSystemMiniView::RefrigerationSystemMiniView()
{
  refrigerationSystemView = new RefrigerationSystemView();
  refrigerationSystemView->setParentItem(this);

  refrigerationSystemView->setTransform(QTransform((contentRect().width() - 20) / refrigerationSystemView->boundingRect().width(),
                                        0,0,
                                        (contentRect().height() - 20)  / refrigerationSystemView->boundingRect().height(),
                                        0,0));

  refrigerationSystemView->setPos(contentRect().x() + 10,contentRect().y() + 10);

  removeButtonItem = new RemoveButtonItem();
  removeButtonItem->setParentItem(this);
  removeButtonItem->setPos(cellWidth() - removeButtonItem->boundingRect().width() - 10,10);

  zoomInButtonItem = new ZoomInButtonItem();
  zoomInButtonItem->setParentItem(this);
  zoomInButtonItem->setPos(removeButtonItem->pos().x() - 10 - zoomInButtonItem->boundingRect().width(),removeButtonItem->pos().y());
}

QRectF RefrigerationSystemMiniView::boundingRect() const
{
  return QRectF(QPoint(0,0),cellSize());
}

int RefrigerationSystemMiniView::cellWidth()
{
  return 350;
}

int RefrigerationSystemMiniView::headerHeight()
{
  return 50;
}

QSize RefrigerationSystemMiniView::cellSize()
{
  return QSize(cellWidth(),cellWidth() + headerHeight());
}

QRectF RefrigerationSystemMiniView::contentRect() const
{
  return QRectF(0,headerHeight(),cellWidth(),cellWidth());
}

QRectF RefrigerationSystemMiniView::headerRect() const
{
  return QRectF(0,0,cellWidth(),headerHeight());
}

void RefrigerationSystemMiniView::setName(const QString & name)
{
  m_name = name;

  update();
}

void RefrigerationSystemMiniView::paint( QPainter *painter, 
                                         const QStyleOptionGraphicsItem *option, 
                                         QWidget *widget )
{
  // Background and Border

  painter->setRenderHint(QPainter::Antialiasing, true);
  painter->setBrush(Qt::NoBrush);

  painter->setPen(QPen(Qt::black,1,Qt::SolidLine, Qt::RoundCap));
  painter->drawRect(boundingRect());

  painter->setPen(QPen(Qt::black,2,Qt::SolidLine, Qt::RoundCap));
  painter->drawRect(headerRect());

  painter->drawText(headerRect(),m_name);
}

RefrigerationSystemView::RefrigerationSystemView()
  : QGraphicsObject()
{
  // Condenser Item

  refrigerationCondenserView = new RefrigerationCondenserView();
  refrigerationCondenserView->setParentItem(this);

  // Sub Cooler Item

  refrigerationSubCoolerView = new RefrigerationSubCoolerView();
  refrigerationSubCoolerView->setParentItem(this);

  // Heat Reclaim Item

  refrigerationHeatReclaimView = new RefrigerationHeatReclaimView();
  refrigerationHeatReclaimView->setParentItem(this);

  // Compressor Item

  refrigerationCompressorView = new RefrigerationCompressorView();
  refrigerationCompressorView->setParentItem(this);

  // Liquid Suction HX Item

  refrigerationSHXView = new RefrigerationSHXView();
  refrigerationSHXView->setParentItem(this);

  // Cases Item

  refrigerationCasesView = new RefrigerationCasesView();
  refrigerationCasesView->setParentItem(this);

  // Secondary Item

  refrigerationSecondaryView = new RefrigerationSecondaryView();
  refrigerationSecondaryView->setParentItem(this);

  adjustLayout();
}

void RefrigerationSystemView::adjustLayout()
{
  prepareGeometryChange();

  // Condenser

  refrigerationCondenserView->setPos(centerXPos() - refrigerationCondenserView->boundingRect().width() / 2, 
                                     verticalSpacing);

  // Mechanical Sub Cooler

  refrigerationSubCoolerView->setPos(leftXPos() - refrigerationSubCoolerView->boundingRect().width() / 2, 
                                     refrigerationCondenserView->y() + refrigerationCondenserView->boundingRect().height() + verticalSpacing);

  // Heat Reclaim

  refrigerationHeatReclaimView->setPos(rightXPos() - refrigerationHeatReclaimView->boundingRect().width() / 2, 
                                       refrigerationCondenserView->y() + refrigerationCondenserView->boundingRect().height() + verticalSpacing);
  
  // Compressor

  refrigerationCompressorView->setPos(rightXPos() - refrigerationCompressorView->boundingRect().width() / 2, 
                                      refrigerationHeatReclaimView->y() + refrigerationHeatReclaimView->boundingRect().height() + verticalSpacing / 2);

  // SHX

  refrigerationSHXView->setPos(leftXPos() - refrigerationSHXView->boundingRect().width() / 2, 
                               refrigerationSubCoolerView->y() + refrigerationSubCoolerView->boundingRect().height() + verticalSpacing);

  // Cases

  refrigerationCasesView->setPos(centerXPos() - refrigerationCasesView->boundingRect().width() / 2, 
                                 refrigerationSHXView->y() + refrigerationSHXView->boundingRect().height() + verticalSpacing);

  // Cascade or Secondary

  refrigerationSecondaryView->setPos(centerXPos() - refrigerationSecondaryView->boundingRect().width() / 2, 
                                 refrigerationCasesView->y() + refrigerationCasesView->boundingRect().height() + verticalSpacing);
}

void RefrigerationSystemView::paint( QPainter *painter, 
                                     const QStyleOptionGraphicsItem *option, 
                                     QWidget *widget )
{
  // Background and Border

  painter->setRenderHint(QPainter::Antialiasing, true);
  painter->setBrush(Qt::NoBrush);
  painter->setPen(QPen(Qt::red,2,Qt::SolidLine, Qt::RoundCap));

  painter->drawRoundedRect(10,10,boundingRect().width() - 20, boundingRect().height() - 20,8,8);
}

QRectF RefrigerationSystemView::boundingRect() const
{
  return QRectF(0,0,800,refrigerationSecondaryView->y() + refrigerationSecondaryView->boundingRect().height() + verticalSpacing);
}

int RefrigerationSystemView::leftXPos() const
{
  return boundingRect().width() * 1 / 4;
}

int RefrigerationSystemView::centerXPos() const
{
  return boundingRect().width() * 2 / 4;
}

int RefrigerationSystemView::rightXPos() const
{
  return boundingRect().width() * 3 / 4;
}

RefrigerationCasesView::RefrigerationCasesView()
  : QGraphicsObject()
{
}

void RefrigerationCasesView::paint( QPainter *painter, 
                                    const QStyleOptionGraphicsItem *option, 
                                    QWidget *widget )
{
  painter->setRenderHint(QPainter::Antialiasing, true);
  painter->setBrush(Qt::NoBrush);
  painter->setPen(QPen(Qt::red,2,Qt::SolidLine, Qt::RoundCap));

  painter->drawRoundedRect(10,10,boundingRect().width() - 20, boundingRect().height() - 20,8,8);

  painter->drawText(boundingRect(),Qt::AlignCenter,"Cases");
}

QRectF RefrigerationCasesView::boundingRect() const
{
  return QRectF(0,0,200,100);
}

void RefrigerationCondenserView::paint( QPainter *painter, 
                                        const QStyleOptionGraphicsItem *option, 
                                        QWidget *widget )
{
  painter->setRenderHint(QPainter::Antialiasing, true);
  painter->setBrush(Qt::NoBrush);
  painter->setPen(QPen(Qt::red,2,Qt::SolidLine, Qt::RoundCap));

  painter->drawRoundedRect(10,10,boundingRect().width() - 20, boundingRect().height() - 20,8,8);

  painter->drawText(boundingRect(),Qt::AlignCenter,"Condenser");
}

QRectF RefrigerationCondenserView::boundingRect() const
{
  return QRectF(0,0,200,100);
}

void RefrigerationCompressorView::paint( QPainter *painter, 
                                         const QStyleOptionGraphicsItem *option, 
                                         QWidget *widget )
{
  painter->setRenderHint(QPainter::Antialiasing, true);
  painter->setBrush(Qt::NoBrush);
  painter->setPen(QPen(Qt::red,2,Qt::SolidLine, Qt::RoundCap));

  painter->drawRoundedRect(10,10,boundingRect().width() - 20, boundingRect().height() - 20,8,8);

  painter->drawText(boundingRect(),Qt::AlignCenter,"Compressor");
}

QRectF RefrigerationCompressorView::boundingRect() const
{
  return QRectF(0,0,200,100);
}

void RefrigerationSubCoolerView::paint( QPainter *painter, 
                                         const QStyleOptionGraphicsItem *option, 
                                         QWidget *widget )
{
  painter->setRenderHint(QPainter::Antialiasing, true);
  painter->setBrush(Qt::NoBrush);
  painter->setPen(QPen(Qt::red,2,Qt::SolidLine, Qt::RoundCap));

  painter->drawRoundedRect(10,10,boundingRect().width() - 20, boundingRect().height() - 20,8,8);

  painter->drawText(boundingRect(),Qt::AlignCenter,"Sub Cooler");
}

QRectF RefrigerationSubCoolerView::boundingRect() const
{
  return QRectF(0,0,200,100);
}

void RefrigerationHeatReclaimView::paint( QPainter *painter, 
                                         const QStyleOptionGraphicsItem *option, 
                                         QWidget *widget )
{
  painter->setRenderHint(QPainter::Antialiasing, true);
  painter->setBrush(Qt::NoBrush);
  painter->setPen(QPen(Qt::red,2,Qt::SolidLine, Qt::RoundCap));

  painter->drawRoundedRect(10,10,boundingRect().width() - 20, boundingRect().height() - 20,8,8);

  painter->drawText(boundingRect(),Qt::AlignCenter,"Heat Reclaim");
}

QRectF RefrigerationHeatReclaimView::boundingRect() const
{
  return QRectF(0,0,200,100);
}

void RefrigerationSHXView::paint( QPainter *painter, 
                                         const QStyleOptionGraphicsItem *option, 
                                         QWidget *widget )
{
  painter->setRenderHint(QPainter::Antialiasing, true);
  painter->setBrush(Qt::NoBrush);
  painter->setPen(QPen(Qt::red,2,Qt::SolidLine, Qt::RoundCap));

  painter->drawRoundedRect(10,10,boundingRect().width() - 20, boundingRect().height() - 20,8,8);

  painter->drawText(boundingRect(),Qt::AlignCenter,"Liquid Suction HX");
}

QRectF RefrigerationSHXView::boundingRect() const
{
  return QRectF(0,0,200,100);
}

void RefrigerationSecondaryView::paint( QPainter *painter, 
                                        const QStyleOptionGraphicsItem *option, 
                                        QWidget *widget )
{
  painter->setRenderHint(QPainter::Antialiasing, true);
  painter->setBrush(Qt::NoBrush);
  painter->setPen(QPen(Qt::red,2,Qt::SolidLine, Qt::RoundCap));

  painter->drawRoundedRect(10,10,boundingRect().width() - 20, boundingRect().height() - 20,8,8);

  painter->drawText(boundingRect(),Qt::AlignCenter,"Secondary System");
}

QRectF RefrigerationSecondaryView::boundingRect() const
{
  return QRectF(0,0,200,100);
}

RefrigerationSystemDropZoneView::RefrigerationSystemDropZoneView()
  : QGraphicsObject(), 
    m_mouseDown(false)
{
}

QRectF RefrigerationSystemDropZoneView::boundingRect() const
{
  return QRectF(QPoint(0,0),RefrigerationSystemMiniView::cellSize());
}

void RefrigerationSystemDropZoneView::paint( QPainter *painter, 
                                                 const QStyleOptionGraphicsItem *option, 
                                                 QWidget *widget )
{
  painter->setRenderHint(QPainter::Antialiasing, true);
  painter->setBrush(Qt::NoBrush);
  painter->setPen(QPen(Qt::black,2,Qt::DashLine, Qt::RoundCap));

  painter->drawRect(boundingRect());
}

void RefrigerationSystemDropZoneView::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
  m_mouseDown = true;

  this->update();

  event->accept();
}

void RefrigerationSystemDropZoneView::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
{
  if( m_mouseDown )
  {
    if( shape().contains(event->pos()) )
    {
      event->accept();

      emit mouseClicked();
    }
  }

  m_mouseDown = false;

  this->update();
}

} // openstudio

