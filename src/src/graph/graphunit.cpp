#include "graphunit.h"
#include <QDebug>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QLabel>
#include <QLayout>
#include <QPainter>
#include <QRandomGenerator>

QRectF GraphUnit::boundingRect() const {
    if(m_status->m_info->m_loopMovie) {
        // qDebug() << m_loopMovie->frameRect();
        auto tmp = m_status->m_info->m_loopMovie->frameRect();
        tmp.moveCenter(QPoint(0, 0));
        return tmp;
    }
    else
        return QRectF(-GraphInfo::unitSize - GraphInfo::penWidth,
                      -GraphInfo::unitSize - GraphInfo::penWidth,
                      2 * (GraphInfo::unitSize + GraphInfo::penWidth),
                      2 * (GraphInfo::unitSize + GraphInfo::penWidth));
}

QPainterPath GraphUnit::shape() const {
    return path;
}

// about movie:
// https://forum.qt.io/topic/123784/animated-gif-in-qgraphicsscene-qgraphicsview/5

void GraphUnit::setMovie(QMovie *unitMovie, QMovie *loopMovie) {
    prepareGeometryChange();
    QObject::disconnect(mConnection1);  // disconnect old object
    if(loopMovie) {
        mConnection1 =
            QObject::connect(loopMovie, &QMovie::frameChanged, [=](int f) {
                if(f % 2 == 0)
                    update();
            });
    }
    QObject::disconnect(mConnection2);  // disconnect old object
    if(unitMovie) {
        mConnection2 =
            QObject::connect(unitMovie, &QMovie::frameChanged, [=](int f) {
                if(f % 2 == 0)
                    update();
            });
    }
}

void GraphUnit::paintAroundLoop(QPainter *painter) {
    if(m_status->m_info->m_loopMovie != nullptr) {
        // qDebug() << "indeed paint around loop"
        //          << QRandomGenerator::global()->generate() << Qt::endl;
        painter->drawPixmap(
            -m_status->m_info->m_loopMovie->frameRect().bottomRight() / 2,
            m_status->m_info->m_loopMovie->currentPixmap(),
            m_status->m_info->m_loopMovie->frameRect());
    }
}

void GraphUnit::paintUnit(QPainter *painter) {
    if(m_status->m_info->m_unitMovie != nullptr) {
        painter->drawPixmap(
            -m_status->m_info->m_unitMovie->frameRect().bottomRight() / 2,
            m_status->m_info->m_unitMovie->currentPixmap(),
            m_status->m_info->m_unitMovie->frameRect());
    }
}

void GraphUnit::paint(QPainter *painter, const QStyleOptionGraphicsItem *,
                      QWidget *) {
    if(m_status->m_info->image == "") {
        // ????????????????????????
        if(m_status->isAlive()) {
            if(m_status->m_player == 1) {
                painter->setBrush(Qt::blue);
            }
            else {
                painter->setBrush(Qt::yellow);
            }
        }
        else {
            painter->setBrush(Qt::black);
        }
        painter->setPen(QPen(Qt::black, GraphInfo::penWidth));
        painter->drawEllipse({0, 0}, GraphInfo::unitSize, GraphInfo::unitSize);
        painter->setFont(QFont("Microsoft YaHei", 30, 2));
        painter->drawText(QPoint(0, 0), QString::number(m_status->m_type));
    }
    else {
        paintUnit(painter);
    }
    if((m_status->canMove() || m_status->canAttack()) && m_status->isAlive()) {
        paintAroundLoop(painter);
    }
    m_bloodBar->setPercentage(
        m_status->m_HPnow);  // ????????????????????????????????????????????????????????????
    // painter->drawRoundedRect(-100, -100, 200, 200, 50, 50);
    // painter->fillRect(0, 0, 100, 100, Qt::green);
}

void GraphUnit::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    qDebug() << "press unit: " << this->m_status->m_uid << Qt::endl;
    QGraphicsObject::mousePressEvent(event);
    event->ignore();
}

void GraphUnit::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    qDebug() << "release unit: " << this->m_status->m_uid << Qt::endl;
    QGraphicsObject::mouseReleaseEvent(event);
    event->ignore();
}

void GraphUnit::hoverEnterEvent(QGraphicsSceneHoverEvent *event) {
    qDebug() << "hover!" << nowCoord();
    auto v = scene()->views().first();
    if(dialogWidget == nullptr) {
        dialogWidget = scene()->addWidget(m_unitDialog);
        dialogWidget->setParent(this);
        dialogWidget->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
        dialogWidget->setZValue(GraphInfo::buttonZValue);
        dialogWidget->hide();
    }
    timer->singleShot(500, [=]() {
        dialogWidget->setPos(this->mapToScene(100, 100));
        m_unitDialog->updateInfo();
        m_unitDialog->show();
        // m_unitDialog->move(v->mapToGlobal(v->mapFromScene(this->mapToScene(100,
        // 100)))); m_unitDialog->show();
    });
    QGraphicsObject::hoverEnterEvent(event);
}

void GraphUnit::hoverLeaveEvent(QGraphicsSceneHoverEvent *event) {
    qDebug() << "hover leave" << nowCoord();
    // dialogWidget->hide();
    m_unitDialog->hide();
    QGraphicsObject::hoverLeaveEvent(event);
}
