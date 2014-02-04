/**********************************************************************
*  Copyright (c) 2008-2014, Alliance for Sustainable Energy.  
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

#include "VRFController.hpp"
#include "VRFGraphicsItems.hpp"
#include "OSAppBase.hpp"
#include "OSDocument.hpp"
#include "OSItem.hpp"
#include "MainWindow.hpp"
#include "MainRightColumnController.hpp"
#include "../model/Model.hpp"
#include "../model/AirConditionerVariableRefrigerantFlow.hpp"
#include "../model/AirConditionerVariableRefrigerantFlow_Impl.hpp"
#include "../utilities/core/Compare.hpp"
#include "../shared_gui_components/GraphicsItems.hpp"
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QTimer>
#include <QMessageBox>

namespace openstudio {

VRFController::VRFController()
  : QObject(),
    m_detailView(0),
    m_dirty(false)
{
  m_currentSystem = boost::none;

  m_vrfView = new VRFView();
  bool bingo;
  bingo = connect(m_vrfView->zoomOutButton,SIGNAL(clicked()),this,SLOT(zoomOutToSystemGridView()));
  OS_ASSERT(bingo);

  m_vrfSystemGridView = new GridLayoutItem();
  m_vrfSystemGridView->setCellSize(VRFSystemMiniView::cellSize());
  m_vrfGridScene = QSharedPointer<QGraphicsScene>(new QGraphicsScene());
  m_vrfGridScene->addItem(m_vrfSystemGridView);

  m_vrfSystemListController = QSharedPointer<VRFSystemListController>(new VRFSystemListController(this));
  m_vrfSystemGridView->setListController(m_vrfSystemListController);
  m_vrfSystemGridView->setDelegate(QSharedPointer<VRFSystemItemDelegate>(new VRFSystemItemDelegate()));

  zoomOutToSystemGridView();
}

VRFController::~VRFController()
{
  delete m_vrfView;
}

VRFView * VRFController::vrfView() const
{
  return m_vrfView;
}

QSharedPointer<VRFSystemListController> VRFController::vrfSystemListController() const
{
  return m_vrfSystemListController;
}

void VRFController::refresh()
{
  m_dirty = true;

  QTimer::singleShot(0,this,SLOT(refreshNow()));
}

void VRFController::refreshNow()
{
  if( ! m_dirty ) return;

  if( m_detailView )
  {
    m_detailView->setId(OSItemId());

    if( m_currentSystem )
    {
      m_detailView->setId(OSItemId(m_currentSystem->handle(),QString(),false));

      bool bingo;
      bingo = connect(m_detailView,SIGNAL(inspectClicked(const OSItemId &)),
                      this,SLOT(inspectOSItem(const OSItemId &)));
      OS_ASSERT(bingo);
    }
  }
}

void VRFController::zoomInOnSystem(model::AirConditionerVariableRefrigerantFlow & system)
{
  m_currentSystem = system;

  boost::shared_ptr<OSDocument> doc = OSAppBase::instance()->currentDocument();
  model::OptionalModelObject mo;
  doc->mainRightColumnController()->inspectModelObject(mo,false);

  m_detailScene = QSharedPointer<QGraphicsScene>(new QGraphicsScene());
  m_detailView = new VRFSystemView();
  m_detailScene->addItem(m_detailView);
  m_vrfView->header->show();
  m_vrfView->graphicsView->setScene(m_detailScene.data());
  m_vrfView->graphicsView->setAlignment(Qt::AlignCenter);

  refresh();
}

void VRFController::zoomOutToSystemGridView()
{
  m_currentSystem = boost::none;

  model::OptionalModelObject mo;
  boost::shared_ptr<OSDocument> doc = OSAppBase::instance()->currentDocument();
  doc->mainRightColumnController()->inspectModelObject(mo,false);

  m_vrfView->graphicsView->setScene(m_vrfGridScene.data());
  m_vrfView->graphicsView->setAlignment(Qt::AlignLeft | Qt::AlignTop);
  m_vrfView->header->hide();

  refresh();
}

void VRFController::inspectOSItem(const OSItemId & itemid)
{
  OS_ASSERT(m_currentSystem);
  boost::optional<model::ModelObject> mo = m_currentSystem->model().getModelObject<model::ModelObject>(Handle(itemid.itemId()));

  boost::shared_ptr<OSDocument> doc = OSAppBase::instance()->currentDocument();
  OS_ASSERT(doc);
  doc->mainRightColumnController()->inspectModelObject(mo,false);
}

VRFSystemListController::VRFSystemListController(VRFController * vrfController)
  : m_vrfController(vrfController)
{
}

VRFController * VRFSystemListController::vrfController() const
{
  return m_vrfController;
}

QSharedPointer<OSListItem> VRFSystemListController::itemAt(int i)
{
  QSharedPointer<OSListItem> item;

  if( i == 0 )
  {
    item = QSharedPointer<VRFSystemListDropZoneItem>(new VRFSystemListDropZoneItem(this));
  }
  else if( i > 0 && i < count() )
  {
    item = QSharedPointer<VRFSystemListItem>(new VRFSystemListItem(systems()[i - 1],this));
  }

  return item;
}

int VRFSystemListController::count()
{
  return systems().size() + 1;
}

int VRFSystemListController::systemIndex(const model::AirConditionerVariableRefrigerantFlow & system) const
{
  std::vector<model::AirConditionerVariableRefrigerantFlow> _systems = systems();

  int i = 1;

  for( std::vector<model::AirConditionerVariableRefrigerantFlow>::const_iterator it = _systems.begin();
       it != _systems.end();
       ++it )
  {
    if( *it == system )
    {
      break;
    }

    i++;
  }

  return i;
}

void VRFSystemListController::createNewSystem()
{
  if( boost::optional<model::Model> model = OSAppBase::instance()->currentModel() )
  {
    model::AirConditionerVariableRefrigerantFlow system(model.get());

    emit itemInserted(systemIndex(system));
  }
}

void VRFSystemListController::addSystem(const OSItemId & itemid)
{
  boost::shared_ptr<OSDocument> doc = OSAppBase::instance()->currentDocument();

  if( doc->fromComponentLibrary(itemid) )
  {
    boost::optional<model::ModelObject> mo = doc->getModelObject(itemid);

    boost::optional<model::Model> model = OSAppBase::instance()->currentModel();

    if( mo && model )
    {
      if( boost::optional<model::AirConditionerVariableRefrigerantFlow> system = mo->optionalCast<model::AirConditionerVariableRefrigerantFlow>() )
      {
        model::AirConditionerVariableRefrigerantFlow systemClone = 
          system->clone(model.get()).cast<model::AirConditionerVariableRefrigerantFlow>();

        emit itemInserted(systemIndex(systemClone));
      }
    }
  }
}

void VRFSystemListController::removeSystem(model::AirConditionerVariableRefrigerantFlow & vrfSystem)
{
  int i = systemIndex(vrfSystem);

  vrfSystem.remove();

  emit itemRemoved(i);
}

void VRFSystemListItem::zoomInOnSystem()
{
  qobject_cast<VRFSystemListController *>(controller())->vrfController()->zoomInOnSystem(m_vrfSystem);
}

std::vector<model::AirConditionerVariableRefrigerantFlow> VRFSystemListController::systems() const
{
  std::vector<model::AirConditionerVariableRefrigerantFlow> result;

  if( boost::optional<model::Model> model = OSAppBase::instance()->currentModel() )
  {
    result = model->getModelObjects<model::AirConditionerVariableRefrigerantFlow>();
  }

  std::sort(result.begin(), result.end(), WorkspaceObjectNameLess());

  return result;
}

VRFSystemListItem::VRFSystemListItem(const model::AirConditionerVariableRefrigerantFlow & vrfSystem,OSListController * listController)
  : OSListItem(listController),
    m_vrfSystem(vrfSystem)
{
}

QString VRFSystemListItem::systemName() const
{
  return QString::fromStdString(m_vrfSystem.name().get());
}

void VRFSystemListItem::remove()
{
  qobject_cast<VRFSystemListController *>(controller())->removeSystem(m_vrfSystem);
}

VRFSystemListDropZoneItem::VRFSystemListDropZoneItem(OSListController * listController)
  : OSListItem(listController)
{
}

QGraphicsObject * VRFSystemItemDelegate::view(QSharedPointer<OSListItem> dataSource)
{
  QGraphicsObject * itemView = NULL;

  if( QSharedPointer<VRFSystemListItem> listItem = dataSource.dynamicCast<VRFSystemListItem>() )
  {
    VRFSystemMiniView * vrfSystemMiniView = new VRFSystemMiniView();

    bool bingo;
    bingo = connect(vrfSystemMiniView->removeButtonItem,SIGNAL(mouseClicked()),dataSource.data(),SLOT(remove()));
    OS_ASSERT(bingo);

    bingo = connect(vrfSystemMiniView->zoomInButtonItem,SIGNAL(mouseClicked()),dataSource.data(),SLOT(zoomInOnSystem()));
    OS_ASSERT(bingo);

    vrfSystemMiniView->setName(listItem->systemName());

    itemView = vrfSystemMiniView;
  }
  else if( dataSource.dynamicCast<VRFSystemListDropZoneItem>() )
  {
    VRFSystemDropZoneView * vrfSystemDropZoneView = new VRFSystemDropZoneView();

    bool bingo;
    bingo = connect(vrfSystemDropZoneView,SIGNAL(componentDropped(const OSItemId &)),
                    qobject_cast<VRFSystemListController *>(dataSource->controller()),SLOT(addSystem(const OSItemId &)));
    OS_ASSERT(bingo);

    itemView = vrfSystemDropZoneView;
  }

  return itemView;
}

} // openstudio

