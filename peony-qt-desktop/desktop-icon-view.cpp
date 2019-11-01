#include "desktop-icon-view.h"

#include "icon-view-style.h"
#include "desktop-icon-view-delegate.h"

#include "desktop-item-model.h"

#include "file-operation-manager.h"
#include "file-move-operation.h"
#include "file-copy-operation.h"
#include "file-trash-operation.h"
#include "clipboard-utils.h"

#include "desktop-index-widget.h"

#include <QAction>
#include <QMouseEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QDebug>

using namespace Peony;

DesktopIconView::DesktopIconView(QWidget *parent) : QListView(parent)
{
    m_edit_trigger_timer.setSingleShot(true);
    m_last_index = QModelIndex();

    setContentsMargins(0, 0, 0, 0);
    setAttribute(Qt::WA_TranslucentBackground);

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    //fix rubberband style.
    setStyle(DirectoryView::IconViewStyle::getStyle());

    setItemDelegate(new DesktopIconViewDelegate(this));

    setDefaultDropAction(Qt::MoveAction);

    setSelectionMode(QListView::ExtendedSelection);
    setEditTriggers(QListView::NoEditTriggers);
    setViewMode(QListView::IconMode);
    setMovement(QListView::Snap);
    setFlow(QListView::TopToBottom);
    setResizeMode(QListView::Adjust);
    setWordWrap(true);

    setDragDropMode(QListView::DragDrop);

    setContextMenuPolicy(Qt::CustomContextMenu);
    setSelectionMode(QListView::ExtendedSelection);

    auto zoomLevel = this->zoomLevel();
    setDeafultZoomLevel(zoomLevel);

    QTimer::singleShot(1, [=](){
        connect(this->selectionModel(), &QItemSelectionModel::selectionChanged, [=](const QItemSelection &selection, const QItemSelection &deselection){
            //qDebug()<<"selection changed";
            this->setIndexWidget(m_last_index, nullptr);
            this->resetEditTriggerTimer();
            auto currentSelections = this->selectionModel()->selection().indexes();

            if (currentSelections.count() == 1) {
                qDebug()<<"set index widget";
                m_last_index = currentSelections.first();
                auto delegate = qobject_cast<DesktopIconViewDelegate *>(itemDelegate());
                this->setIndexWidget(m_last_index, new DesktopIndexWidget(delegate, viewOptions(), m_last_index, this));
            } else {
                m_last_index = QModelIndex();
                for (auto index : deselection.indexes()) {
                    this->setIndexWidget(index, nullptr);
                }
            }
        });
    });

    m_model = new DesktopItemModel(this);

    setModel(m_model);
}

DesktopIconView::~DesktopIconView()
{

}

const QStringList DesktopIconView::getSelections()
{
    QStringList uris;
    auto indexes = selectionModel()->selection().indexes();
    for (auto index : indexes) {
        uris<<index.data(Qt::UserRole).toString();
    }
    return uris;
}

const QStringList DesktopIconView::getAllFileUris()
{

}

void DesktopIconView::setSelections(const QStringList &uris)
{

}

void DesktopIconView::invertSelections()
{

}

void DesktopIconView::scrollToSelection(const QString &uri)
{

}

int DesktopIconView::getSortType()
{
    //FIXME:
    return 0;
}

void DesktopIconView::setSortType(int sortType)
{

}

int DesktopIconView::getSortOrder()
{
    //FIXME:
    return Qt::AscendingOrder;
}

void DesktopIconView::setSortOrder(int sortOrder)
{

}

void DesktopIconView::editUri(const QString &uri)
{

}

void DesktopIconView::editUris(const QStringList uris)
{

}

void DesktopIconView::setCutFiles(const QStringList &uris)
{

}

void DesktopIconView::closeView()
{
    deleteLater();
}

void DesktopIconView::zoomIn()
{
    switch (m_zoom_level) {
    case Huge:
        setDeafultZoomLevel(Large);
        break;
    case Large:
        setDeafultZoomLevel(Normal);
        break;
    case Normal:
        setDeafultZoomLevel(Small);
        break;
    default:
        break;
    }
}

void DesktopIconView::zoomOut()
{
    switch (m_zoom_level) {
    case Small:
        setDeafultZoomLevel(Normal);
        break;
    case Normal:
        setDeafultZoomLevel(Large);
        break;
    case Large:
        setDeafultZoomLevel(Huge);
        break;
    default:
        break;
    }
}

/*
Small, //icon: 32x32; grid: 56x64; hover rect: 40x56; font: system*0.8
Normal, //icon: 48x48; grid: 64x72; hover rect = 56x64; font: system
Large, //icon: 64x64; grid: 115x135; hover rect = 105x118; font: system*1.2
Huge //icon: 96x96; grid: 130x150; hover rect = 120x140; font: system*1.4
*/
void DesktopIconView::setDeafultZoomLevel(ZoomLevel level)
{
    m_zoom_level = level;
    switch (level) {
    case Small:
        setIconSize(QSize(24, 24));
        setGridSize(QSize(64, 64));
        break;
    case Large:
        setIconSize(QSize(64, 64));
        setGridSize(QSize(115, 135));
        break;
    case Huge:
        setIconSize(QSize(96, 96));
        setGridSize(QSize(140, 170));
        break;
    default:
        //Normal
        setIconSize(QSize(48, 48));
        setGridSize(QSize(96, 96));
        break;
    }
    clearAllIndexWidgets();
}

DesktopIconView::ZoomLevel DesktopIconView::zoomLevel()
{
    //FIXME:
    return m_zoom_level != Invalid? m_zoom_level: Normal;
}

void DesktopIconView::mousePressEvent(QMouseEvent *e)
{
    QListView::mousePressEvent(e);

    //qDebug()<<m_last_index.data();
    if (e->button() != Qt::LeftButton) {
        return;
    }

    if (!indexAt(e->pos()).isValid()) {
        clearAllIndexWidgets();
    }

    if (indexAt(e->pos()) == m_last_index && m_last_index.isValid()) {
        //qDebug()<<"check";
        if (m_edit_trigger_timer.isActive()) {
            qDebug()<<"edit";
            setIndexWidget(m_last_index, nullptr);
            edit(m_last_index);
        }
    }
}

void DesktopIconView::mouseReleaseEvent(QMouseEvent *e)
{
    QListView::mouseReleaseEvent(e);

    if (e->button() != Qt::LeftButton) {
        return;
    }

    if (!m_edit_trigger_timer.isActive() && indexAt(e->pos()).isValid() && this->selectedIndexes().count() == 1) {
        resetEditTriggerTimer();
    }
}

void DesktopIconView::resetEditTriggerTimer()
{
    m_edit_trigger_timer.disconnect();
    m_edit_trigger_timer.stop();
    QTimer::singleShot(750, [&](){
        //qDebug()<<"start";
        m_edit_trigger_timer.setSingleShot(true);
        m_edit_trigger_timer.start(1000);
    });
}

void DesktopIconView::dragEnterEvent(QDragEnterEvent *e)
{
    qDebug()<<"drag enter event";
    if (e->mimeData()->hasUrls()) {
        e->setDropAction(Qt::MoveAction);
        e->acceptProposedAction();
    }
}

void DesktopIconView::dragMoveEvent(QDragMoveEvent *e)
{
    if (e->isAccepted())
        return;
    qDebug()<<"drag move event";
    if (this == e->source()) {
        e->accept();
        return QListView::dragMoveEvent(e);
    }
    e->setDropAction(Qt::CopyAction);
    e->accept();
}

void DesktopIconView::dropEvent(QDropEvent *e)
{
    qDebug()<<"drop event";
    m_edit_trigger_timer.stop();
    if (this == e->source()) {
        return QListView::dropEvent(e);
    }
    m_model->dropMimeData(e->mimeData(), Qt::MoveAction, -1, -1, this->indexAt(e->pos()));
    //FIXME: save item position
}

const QFont DesktopIconView::getViewItemFont(QStyleOptionViewItem *item)
{
    auto font = item->font;
    switch (zoomLevel()) {
    case DesktopIconView::Small:
        font.setPointSizeF(font.pointSizeF() * 0.8);
        break;
    case DesktopIconView::Large:
        font.setPointSizeF(font.pointSizeF() * 1.2);
        break;
    case DesktopIconView::Huge:
        font.setPointSizeF(font.pointSizeF() * 1.4);
        break;
    default:
        break;
    }
    return font;
}

void DesktopIconView::clearAllIndexWidgets()
{
    if (!model())
        return;

    int row = 0;
    auto index = model()->index(row, 0);
    while (index.isValid()) {
        setIndexWidget(index, nullptr);
        row++;
        index = model()->index(row, 0);
    }
}
