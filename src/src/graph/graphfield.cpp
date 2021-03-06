#include "graphfield.h"
#include "graphblock.h"
#include <QGraphicsView>
#include <QPropertyAnimation>

GraphField::GraphField(const GameInfo &                 gameInfo,
                       QVector<QVector<BlockStatus *>> &blockStatus,
                       QVector<UnitStatus *> &unitStatus, QObject *parent)
    : QGraphicsScene(parent), m_gameInfo(gameInfo), m_nowCheckedBlock(nullptr) {
    this->setSceneRect(
        QRectF(-qSqrt(3) / 2 * GraphInfo::blockSize, -1 * GraphInfo::blockSize,
               (qSqrt(3) * (width() + 0.5)) * GraphInfo::blockSize,
               (1.5 * height() + 0.5) * GraphInfo::blockSize));
    this->addRect(this->sceneRect());
    m_blocks.resize(width() + 2);
    for(int i = 1; i <= width(); i++) {
        m_blocks[i].resize(height() + 2);
        for(int j = 1; j <= height(); j++) {
            m_blocks[i][j] =
                new GraphBlock(blockStatus[i][j], getBlockCenter(i, j));
            this->addItem(m_blocks[i][j]);
        }
    }
    for(int i = 0; i < unitStatus.size(); i++) {
        m_units.push_back(new GraphUnit(unitStatus[i]));
        this->addItem(m_units[i]);
    }
    for(int i = 1; i <= width(); i++) {
        for(int j = 1; j <= height(); j++) {
            connect(m_blocks[i][j], &GraphBlock::blockClicked, this,
                    &GraphField::onBlockClicked);

            connect(this, &GraphField::checkStateChange, m_blocks[i][j],
                    &GraphBlock::changeCheck);
            connect(this, &GraphField::moveRangeChange, m_blocks[i][j],
                    &GraphBlock::changeMoveRange);
        }
    }
    connect(this, &GraphField::checkStateChange, this,
            [=](QPoint coord, bool state) {
                qint32 uid = blocks(coord)->unitOnBlock();
                if(state == true && uid != -1 &&
                   m_units[uid]->player() == m_gameInfo.nowPlayer &&
                   m_units[uid]->m_status->isAlive() &&
                   m_units[uid]->m_status->canMove()) {
                    emit userShowMoveRange(uid);
                }
                else {
                    emit userHideMoveRange();
                }
            });
}

GraphField::~GraphField() {}

void GraphField::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    qDebug() << "scene: " << event->scenePos() << Qt::endl;

    qDebug() << "view: " << views().first()->mapFromScene(event->scenePos())
             << Qt::endl;
    qDebug() << "view: "
             << views().first()->mapToScene(
                    views().first()->mapFromScene(event->scenePos()))
             << Qt::endl;
    QGraphicsScene::mousePressEvent(event);
}

void GraphField::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    qDebug() << "release scene: " << event->scenePos() << Qt::endl;
    QGraphicsScene::mouseReleaseEvent(event);
}

void GraphField::moveUnit(qint32 uid, const QVector<QPoint> &path) {
    Q_ASSERT(uid < m_units.size());
    this->moveUnit(m_units[uid], path);
}

// ?????????graphunit????????????
void GraphField::moveUnit(GraphUnit *graphUnit, const QVector<QPoint> &path) {
    QPropertyAnimation *animation = new QPropertyAnimation(graphUnit, "pos");
    animation->setDuration(m_gameInfo.speed * path.size());
    animation->setStartValue(graphUnit->pos());
    animation->setEndValue(getBlockCenter(path.back()));
    qDebug() << "Path length:" << path.size() << Qt::endl;
    for(int i = 0; i < path.size() - 1; i++) {
        animation->setKeyValueAt(qreal(i + 1) / path.size(),
                                 getBlockCenter(path[i]));
    }
    animation->start();
    connect(animation, &QPropertyAnimation::finished, this,
            [=]() { emit finishOrder(); });
    // emit needUpdateDetail();
}

void GraphField::onBlockClicked(QPoint coord) {
    if(m_nowCheckedBlock == nullptr) {
        m_nowCheckedBlock = blocks(coord);
        emit checkStateChange(coord, true);
    }
    else {
        if(m_nowCheckedBlock->coord() == coord) {
            m_nowCheckedBlock = nullptr;
            emit checkStateChange(coord, false);
        }
        else {
            qint32 flag = -1;
            // flag == 0: ???????????????
            // flag == 1: ??????
            // flag == 2: ??????
            // 1. ????????????????????????????????????????????????????????????????????????
            // ????????????????????????????????????????????????????????????
            qint32 uidA = m_nowCheckedBlock->unitOnBlock(),
                   uidB = blocks(coord)->unitOnBlock();
            if(uidA != -1) {
                // A ??????????????????
                if(m_gameInfo.nowPlayer == m_units[uidA]->m_status->m_player &&
                   m_units[uidA]->m_status->isAlive()) {
                    // A ?????????????????????????????? ????????????
                    if(uidB == -1) {
                        // B ?????????????????????
                        if((blocks(coord)->m_status->m_type & campBlock) &&
                           blocks(coord)->m_status->getHP() > 0) {
                            if(blocks(coord)->m_isMoveRange &&
                               !notSameCamp(m_units[uidA]->m_status,
                                            blocks(coord)->m_status)) {
                                flag = 1;
                            }
                            else {
                                // ????????????????????????
                                if(m_units[uidA]->m_status->canAttack() &&
                                   isNearByPoint(coord,
                                                 m_nowCheckedBlock->coord()) &&
                                   m_units[uidA]->m_status->isAlive()) {
                                    // ????????????
                                    flag = 2;
                                }
                                else {
                                    flag = 0;
                                }
                            }
                        }
                        else {
                            // ???????????? ??????????????????
                            if(blocks(coord)->m_isMoveRange &&
                               m_units[uidA]->m_status->canMove()) {
                                flag = 1;
                            }
                            else {
                                flag = 0;
                            }
                        }
                    }
                    else {
                        // B ??????????????????
                        if(m_gameInfo.nowPlayer ==
                           m_units[uidB]->m_status->m_player) {
                            // B ??????????????????????????????
                            flag = 0;
                        }
                        else {
                            // B ?????????????????????????????????
                            if(m_units[uidB]->m_status->isAlive() &&
                               isNearByPoint(coord,
                                             m_nowCheckedBlock->coord()) &&
                               m_units[uidA]->m_status->canAttack()) {
                                // ?????? ??? ?????????????????????
                                flag = 2;
                            }
                            else {
                                // ??????
                                flag = 0;
                            }
                        }
                    }
                }
                else {
                    // A ?????????????????????????????????
                    flag = 0;
                }
            }
            else {
                flag = 0;
            }
            GraphBlock *tmp_block = m_nowCheckedBlock;
            switch(flag) {
                case 0:
                    emit checkStateChange(tmp_block->coord(), false);
                    m_nowCheckedBlock = nullptr;

                    m_nowCheckedBlock = blocks(coord);
                    emit checkStateChange(coord, true);
                    break;
                case 1:

                    emit checkStateChange(tmp_block->coord(), false);
                    m_nowCheckedBlock = nullptr;
                    emit userMoveUnit(uidA, coord);
                    break;
                case 2:

                    emit checkStateChange(tmp_block->coord(), false);
                    m_nowCheckedBlock = nullptr;
                    emit userAttackUnit(uidA, coord);
                    break;
                default:
                    break;
            }
        }
    }
}

void GraphField::newUnit(UnitStatus *unitStatus) {
    // qDebug() << "new unit: " << unitStatus->m_nowCoord << Qt::endl;
    GraphUnit *newUnit =
        new GraphUnit(unitStatus, getBlockCenter(unitStatus->m_nowCoord));
    m_units.append(newUnit);
    this->addItem(newUnit);
    // emit needUpdateDetail();
}

void GraphField::attackUnit(qint32 uid, qint32 tarid,
                            QPair<qreal, qreal> delta) {
    attackUnit(m_units[uid], tarid, delta);
}
void GraphField::attackUnit(GraphUnit *graphUnit, qint32 tarid,
                            QPair<qreal, qreal> delta) {
    graphUnit->update(graphUnit->boundingRect());
    m_units[tarid]->update(m_units[tarid]->boundingRect());
    showUnitAttackLabel(graphUnit->nowCoord(), delta.first);

    showUnitAttackLabel(m_units[tarid]->nowCoord(), delta.second);

    // QMessageBox msgBox;
    qDebug() << ("????????? " + QString::number(graphUnit->uid()) +
                 " ??? Unit ????????? " + QString::number(tarid) + " ??? Unit ???");
    // msgBox.exec();
    emit finishOrder();
    // emit needUpdateDetail();
}

void GraphField::showUnitAttackLabel(QPoint coord, qreal delta, qreal pos) {
    AttackLabel *labela = new AttackLabel(delta,nullptr);
    auto         it     = this->addWidget(labela);
    it->setZValue(1000);
    it->setPos(getBlockCenter(coord) - QPointF(0, pos * GraphInfo::blockSize));
    dynamic_cast<AttackLabel *>(it->widget())->show();
}

void GraphField::attackCamp(qint32 uid, QPoint coord,
                            QPair<qreal, qreal> delta) {
    GraphUnit *graphUnit = m_units[uid];
    graphUnit->update(graphUnit->boundingRect());
    if(delta.first != 0)
        showUnitAttackLabel(graphUnit->nowCoord(), delta.first);
    showUnitAttackLabel(coord, delta.second);
    // ???????????? block ???
    // QMessageBox msgBox;
    qDebug() << ("????????? " + QString::number(graphUnit->uid()) +
                 " ??? Unit ????????? ???" + QString::number(coord.x()) + "," +
                 QString::number(coord.y()) + " ) ?????? Camp ???Camp ?????????" +
                 QString::number(blocks(coord)->m_status->getHP()));
    emit finishOrder();
    // emit needUpdateDetail();
}

void GraphField::dieUnit(qint32 uid) {
    m_units[uid]->update(m_units[uid]->boundingRect());
    m_units[uid]->hide();
    dynamic_cast<UnitDialog *>(m_units[uid]->dialogWidget->widget())->hide();
    qDebug() << "die" << uid;
    // emit needUpdateDetail();
}

void GraphField::showMoveRange(qint32 uid, QVector<QPoint> range) {
    for(auto p : range) {
        emit moveRangeChange(uid, p, true);
    }
}

void GraphField::hideMoveRange() {
    for(int i = 1; i <= width(); i++) {
        for(int j = 1; j <= height(); j++) {
            emit moveRangeChange(-1, QPoint(i, j), false);
        }
    }
}

// a attack b
